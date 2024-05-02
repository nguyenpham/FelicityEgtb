//
//  TbLookupWritting.cpp
//
//  Created by Tony Pham on 2/11/17.
//

#include <fstream>
#include <algorithm>

#include "../egtb/Egtb.h"
#include "EgtbLookupWritting.h"
#include "CompressLib.h"

using namespace egtb;

bool EgtbLookupWritting::save(const std::string& folder, int sd, CompressMode _compressMode) {

    auto compress = _compressMode != CompressMode::compress_none;

    const int blocksize = EGTBLU_COMPRESS_BLOCK_SIZE;
    int groupNum = 2;
    int64_t totalItems = 0, maxGroupSz = 0, blockNum = 0;
    for (int i = 0; i < groupNum; i++) {
        int64_t c = itemCnt[i];
        if (c) {
            totalItems += c;
            auto sz = (4 + luGroupSizes[i]) * c;
            maxGroupSz = std::max(maxGroupSz, sz);
            blockNum += (luGroupSizes[i] * c + blocksize - 1) / blocksize;
        }
    }

    if (totalItems == 0) {
        std::cout << "EgtbLookupWritting::save, ignored saving since data is empty for sd " << sd << ", name: " << theName << std::endl;
        return true;
    }

    int theSide = 1 << sd;
    property |= theSide;

    /*
     * Prepare data
     * - create keyTable
     * - remove key from data
     */
    int64_t keyTableSz = totalItems * sizeof(u32);
    keyTable = (u32*)malloc(keyTableSz + 64);

    char* buf = (char*)malloc(maxGroupSz + 64);
    int idx = 0;

    for (int grp = 0; grp < groupNum; grp++) {
        auto c = itemCnt[grp];
        if (c == 0) {
            continue;
        }
        i64 gsz = luGroupSizes[grp];
        i64 sz = (4 + gsz) * c;
        assert(sz <= maxGroupSz);

        memcpy(buf, data[grp], sz);
        int8_t* p = data[grp];
        for (int i = 0; i < c; i++, idx++) {
            i64 k = (4 + gsz) * i;
            keyTable[idx] = *(u32*)(buf + k);
            assert(i == 0 || keyTable[idx - 1] < keyTable[idx]);

            memcpy(p, buf + k + 4, gsz);
            p += gsz;
        }

        assert((int)(p - data[grp]) == c * gsz);
    }
    assert(idx == itemCnt[0] + itemCnt[1]);
    free(buf); buf = nullptr;

    char* keyTableCompressBuf = nullptr;
    if (compress) {
        if (totalItems < 1 * 1024) {
            compress = false;
        } else {
            property |= EGTBLU_PROPERTY_COMPRESS;
        }

        if (compress && keyTableSz > 256) { //} 1 * 1024) {
            keyTableCompressBuf = (char*)malloc(keyTableSz * 4 + 1024);
            keyCompressedSize = CompressLib::compress(keyTableCompressBuf, (const char*)keyTable, (int)keyTableSz);
            assert(keyCompressedSize > 0 && keyCompressedSize < keyTableSz);

//            // check
//            char* buf = (char*)malloc(keyTableSz * 4 + 1024);
//            auto n = CompressLib::decompress((char*)buf, (int)keyTableSz, keyTableCompressBuf, keyCompressedSize);
//            assert(n == keyTableSz);
//            assert(memcmp(buf, (const char*)keyTable, (int)keyTableSz) == 0);
//            free(buf);
        }
    }

    /*
     * write header
     */
    sign = EGTBLU_HEADER_SIGN;

    auto path = getFileName(folder, theName, sd, compress);
    std::ofstream outfile(path, std::ofstream::binary);
    if (!outfile || !outfile.write ((char*)&sign, EGTBLU_HEADER_SIZE)) {
        return false;
    }

    /*
     * write keytable
     */
    if (compress && keyTableCompressBuf) {
        assert(keyCompressedSize > 0 && keyCompressedSize < keyTableSz);
        if (!outfile.write ((char*)keyTableCompressBuf, keyCompressedSize)) {
            free(keyTableCompressBuf);
            outfile.close();
            return false;
        }
        free(keyTableCompressBuf);
    } else {
        if (!outfile.write ((char*)keyTable, keyTableSz)) {
            outfile.close();
            return false;
        }
    }

    bool r = true;
    if (compress) {
        std::cout << "\tEgtbLookupWritting::save compressing" << std::endl;

        char *compBuf[] = { nullptr, nullptr, nullptr, nullptr };
        i64 compSz[] = { 0, 0, 0, 0 };

        u32* blocktable = (u32*)malloc((blockNum + 32 * 1024) * sizeof(u32));

        auto pblocktable = blocktable;
        for (int grp = 0; grp < groupNum; grp++) {
            i64 k = itemCnt[grp];
            if (k == 0) {
                continue;
            }
            i64 sz = luGroupSizes[grp] * k;
            compBuf[grp] = (char *)malloc(sz * 2);
            compSz[grp] = CompressLib::compressAllBlocks(blocksize, (u8*)pblocktable, compBuf[grp], (char*)data[grp], sz);

            i64 blockCnt = (sz + blocksize - 1) / blocksize; assert(blockCnt);
            pblocktable += blockCnt;
            
            auto x = pblocktable - blocktable;
            assert(x <= blockNum);
        }

        if (!outfile.write ((char*)blocktable, blockNum * sizeof(u32))) {
            r = false;
        }

        free(blocktable);
        blocktable = nullptr;

        if (r) {
            for (int grp = 0; grp < groupNum; grp++) {
                if (compBuf[grp]) {
                    if (!outfile.write (compBuf[grp], compSz[grp])) {
                        r = false;
                    }
                    free(compBuf[grp]);
                    compBuf[grp] = nullptr;
                }
            }
        }

    } else {
        for (int grp = 0; grp < groupNum; grp++) {
            if (itemCnt[grp] && data[grp] && !outfile.write((const char*)data[grp], itemCnt[grp] * luGroupSizes[grp])) {
                r = false;
                break;
            }
        }
    }

    outfile.close();
    return r;
}


