/**
 This file is part of Felicity Egtb, distributed under MIT license.

 * Copyright (c) 2024 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2024 developers

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 */

#include <algorithm>
#include <vector>
#include <thread>
#include <assert.h>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

#include "compresslib.h"
#include "threadmng.h"

#include "../lzma/7zTypes.h"
#include "../lzma/LzmaDec.h"
#include "../lzma/LzmaEnc.h"

int compressBuf(const char* inputBuf, int inputLength, char* outputBuf);

using namespace fegtb;

//#define TEST_DECOMPRESS

#define GZIP_ENCODING    16

static void * _allocForLzma(ISzAllocPtr, size_t size) { return malloc(size); }
static void _freeForLzma(ISzAllocPtr, void *addr) { free(addr); }
static ISzAlloc _szAllocForLzma = { _allocForLzma, _freeForLzma };

static const Byte lzmaPropData[5] = { 93, 0, 0, 0, 1 };

int CompressLib::compressLzma(char *dest, const char *src, int slen) {
    /// set up properties
    CLzmaEncProps props;
    LzmaEncProps_Init(&props);

    props.level = 9;
    props.dictSize = 1 << 24;
    props.writeEndMark = 0;

    /// prepare space for the encoded properties
    SizeT propsSize = 5;
    Byte propsEncoded[5];

    SizeT outputSize64 = (SizeT)(slen * 1.5);
    if (outputSize64 < 1024) {
        outputSize64 = 1024;
    }

    int lzmaStatus = LzmaEncode(
                                (Byte *)dest, &outputSize64, (const Byte *)src, slen,
                                &props, propsEncoded, &propsSize, 0,
                                NULL, &_szAllocForLzma, &_szAllocForLzma);

    assert(lzmaStatus == SZ_OK && propsSize == LZMA_PROPS_SIZE);
    assert(memcmp(lzmaPropData, propsEncoded, LZMA_PROPS_SIZE) == 0);

    if (lzmaStatus == SZ_OK) {
        int dlen = (int)outputSize64;

#ifdef TEST_DECOMPRESS
        char *tmpBuf = (char*)malloc(slen * 3);
        auto k = decompressLzma(tmpBuf, slen, dest, dlen);
        assert(k == slen);
        free(tmpBuf);
#endif
        return dlen;
    }

    return -1;
}


int CompressLib::decompressLzma(char *dst, int uncompresslen, const char *src, int slen) {
    SizeT srcLen = slen; // - LZMA_PROPS_SIZE;
    SizeT dstLen = uncompresslen;
    ELzmaStatus lzmaStatus;

    SRes res = LzmaDecode(
                          (Byte *)dst, &dstLen,
                          (const Byte *)src, &srcLen,
                          lzmaPropData, LZMA_PROPS_SIZE,
                          LZMA_FINISH_ANY, //LZMA_FINISH_END,
                          &lzmaStatus, &_szAllocForLzma
                          );

    return res == SZ_OK ? (int)dstLen : -1;
}


///////
extern int MaxGenExtraThreads;

void compressABlock(int threadIdx, int blockIdx, char *dest, int* compSz, const char *src, int srcSize)
{
    assert(srcSize > 0 && src && dest && compSz);
    *compSz = CompressLib::compress(dest, src, srcSize); assert(*compSz > 0);
//    std::cout << "compressABlock DONE, threadIdx: " << threadIdx << ", blockIdx: " << blockIdx << ", srcSize: " << srcSize << ", *compSz: " << *compSz << std::endl;
}

#define MAX_THREAD_NUM	300
i64 CompressLib::compressAllBlocks(int blockSize, u8* blocktable, char *dest, const char *src, i64 slen) {
    assert(blockSize > 128 && blocktable && dest && src && slen > 0);
    assert(EGTB_SMALL_COMPRESS_SIZE + 1 == EGTB_UNCOMPRESS_BIT);
	assert(MaxGenExtraThreads < MAX_THREAD_NUM);

    if (MaxGenExtraThreads == 0) {
        return compressAllBlocksSingleThread(blockSize, blocktable, dest, src, slen);
    }

    int compSizes[MAX_THREAD_NUM];
    char* tmpBuf[MAX_THREAD_NUM];
    for(auto i = 0; i <= MaxGenExtraThreads; i++) {
        tmpBuf[i] = (char *)malloc(blockSize * 3 / 2);
    }

    char *p = dest;
    auto blocknum = (slen + blockSize - 1) / blockSize;
    for (auto i = 0; i < blocknum; i += MaxGenExtraThreads + 1) {

        memset(compSizes, 0, sizeof(compSizes));
        std::vector<std::thread> threadVec;
        for (auto j = 0; j <= MaxGenExtraThreads && i + j < blocknum; ++j) {
            const char *s = src + (i + j) * blockSize;
            auto left = slen - (i64)(s - src);
            auto sSize = (int)std::min(left, (i64)blockSize); assert(sSize > 0);

            threadVec.push_back(std::thread(&compressABlock, j, i + j, tmpBuf[j],  compSizes + j, s, sSize));
        }

        for (auto && t : threadVec) {
            t.join();
        }
        
        for (auto j = 0; j <= MaxGenExtraThreads && i + j < blocknum; ++j) {
            const char *s = src + (i + j) * blockSize;
            auto left = slen - (i64)(s - src);
            auto curBlockSize = (int)std::min(left, (i64)blockSize); assert(curBlockSize > 0);

            i64 flag = 0;
            auto compSz = compSizes[j]; assert(compSz > 0);
            if (compSz > 0 && compSz + 128 < curBlockSize) {
                memcpy(p, tmpBuf[j], compSz);
                p += compSz;
            } else {
                memcpy(p, s, curBlockSize);
                p += curBlockSize;
                flag = EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE; // EGTB_UNCOMPRESS_BIT;
            }

            i64 *b = (i64 *)(blocktable + 5 * (i + j));
            auto sz = (i64)(p - dest); assert(sz < EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE);
            *b = sz | flag;
        }
    }

    for(auto i = 0; i <= MaxGenExtraThreads; i++) {
        free(tmpBuf[i]);
        tmpBuf[i] = nullptr;
    }

    i64 compressedLen = (i64)(p - dest); assert(compressedLen > 0);

    // Can be a small one
    if (compressedLen <= EGTB_SMALL_COMPRESS_SIZE) {
        u32* tb = (u32*)blocktable;
        for (auto i = 0; i < blocknum; i++) {
            i64 *b = (i64 *)(blocktable + 5 * i);
            auto sz = *b & EGTB_LARGE_COMPRESS_SIZE; assert(sz < EGTB_SMALL_COMPRESS_SIZE);
            u32 flag = (*b & EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE) != 0 ? EGTB_UNCOMPRESS_BIT : 0;
            tb[i] = (u32)sz | flag;
        }
    }

    return compressedLen;
}

////////////////
i64 CompressLib::compressAllBlocksSingleThread(int blocksize, u8* blocktable, char *dest, const char *src, i64 slen) {
    assert(blocksize > 128 && blocktable && dest && src && slen > 0);
    assert(EGTB_SMALL_COMPRESS_SIZE + 1 == EGTB_UNCOMPRESS_BIT);
    
#ifdef TEST_DECOMPRESS
    char* tmpbuf = (char *)malloc(blocksize * 3);
#endif
    
    const char *s = src;
    char *p = dest;
    auto blocknum = (slen + blocksize - 1) / blocksize;
    for (auto i = 0; i < blocknum; i++) {
        
        i64 flag = 0;
        auto left = slen - (i64)(s - src);
        
        auto curBlockSize = (int)std::min(left, (i64)blocksize); assert(curBlockSize > 0);
        
        auto compSz = compress(p, s, curBlockSize); assert(compSz > 0);
        if (compSz > 0 && compSz + 128 < curBlockSize) {
            
#ifdef TEST_DECOMPRESS
            auto x = decompress(tmpbuf, curBlockSize, p, compSz);
            assert(x > 0 && x == curBlockSize);
            assert(memcmp(s, tmpbuf, curBlockSize) == 0);
#endif
            p += compSz;
        } else {
            memcpy(p, s, curBlockSize);
            p += curBlockSize;
            flag = EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE; // EGTB_UNCOMPRESS_BIT;
        }
        
        s += blocksize;
        
        i64 *b = (i64 *)(blocktable + 5 * i);
        auto sz = (i64)(p - dest); assert(sz < EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE);
        *b = sz | flag;
    }
    
    i64 compressedLen = (i64)(p - dest); assert(compressedLen > 0);
    
    // Can be a small one
    if (compressedLen <= EGTB_SMALL_COMPRESS_SIZE) {
        u32* tb = (u32*)blocktable;
        for (auto i = 0; i < blocknum; i++) {
            i64 *b = (i64 *)(blocktable + 5 * i);
            auto sz = *b & EGTB_LARGE_COMPRESS_SIZE; assert(sz < EGTB_SMALL_COMPRESS_SIZE);
            u32 flag = (*b & EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE) != 0 ? EGTB_UNCOMPRESS_BIT : 0;
            tb[i] = (u32)sz | flag;
        }
    }
    
#ifdef TEST_DECOMPRESS
    free(tmpbuf);
    
    // test decompress
    tmpbuf = (char *)malloc(slen * 2);
    i64 k = decompressAllBlocks(blocksize, (int)blocknum, blocktable, tmpbuf, slen, dest, compressedLen);
    assert(k == slen);
    free(tmpbuf);
#endif
    
    return compressedLen;
}

i64 CompressLib::decompressAllBlocks(int blocksize, int blocknum, int fromBlockIdx, int toBlockIdx, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen) {
    assert(slen > 0 && uncompressedlen >= slen);
    assert(fromBlockIdx >= 0 && fromBlockIdx < toBlockIdx && toBlockIdx <= blocknum);

    int blockTableItemSize = slen > EGTB_SMALL_COMPRESS_SIZE ? 5 : 4;

    auto *s = src;
    auto startDest = dest;

    if (fromBlockIdx > 0) {
        for(auto i = 0; i < fromBlockIdx; i++) {
            int blocksz;
            if (blockTableItemSize == 4) {
                const u32* p = (u32*)blocktable;
                blocksz = (p[i] & EGTB_SMALL_COMPRESS_SIZE) - (i == 0 ? 0 : (p[i - 1] & EGTB_SMALL_COMPRESS_SIZE));
            } else {
                const u8* p = blocktable + i * blockTableItemSize;
                i64 sz1 = *((i64*)p) & EGTB_LARGE_COMPRESS_SIZE;
                i64 sz0 = i == 0 ? 0 : (*((i64*)(p - blockTableItemSize)) & EGTB_LARGE_COMPRESS_SIZE);
                blocksz = (int)(sz1 - sz0);
            }
//            int blocksz = (blocktable[i] & ~EGTB_UNCOMPRESS_BIT) - (i == 0 ? 0 : (blocktable[i - 1] & ~EGTB_UNCOMPRESS_BIT));
            assert(blocksz > 0 && blocksz <= blocksize);
            s += blocksz;
            startDest += blocksize;
        }
    }

    auto *p = startDest;

    for(auto i = fromBlockIdx; i < toBlockIdx; i++) {
        bool uncompressed = false;
        int blocksz;
        if (blockTableItemSize == 4) {
            const u32* p = (u32*)blocktable;
            blocksz = (p[i] & EGTB_SMALL_COMPRESS_SIZE) - (i == 0 ? 0 : (p[i - 1] & EGTB_SMALL_COMPRESS_SIZE));
            uncompressed = (p[i] & EGTB_UNCOMPRESS_BIT) != 0;
        } else {
            const u8* p = blocktable + i * blockTableItemSize;
            i64 sz1 = *((i64*)p) & EGTB_LARGE_COMPRESS_SIZE;
            i64 sz0 = i == 0 ? 0 : (*((i64*)(p - blockTableItemSize)) & EGTB_LARGE_COMPRESS_SIZE);
            blocksz = (int)(sz1 - sz0);
            uncompressed = (*((i64*)p) & EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE) != 0;
        }

        if (uncompressed) {
            assert(i + 1 == blocknum || blocksz == blocksize);
            memcpy(p, s, blocksz);
            p += blocksz;
        } else {
            // Size of uncompressed data (dest, not src)
            auto left = uncompressedlen - (i64)(p - dest); assert(left > 0);
            auto curBlockSize = (int)std::min(left, (i64)blocksize);

            auto originSz = decompress((char*)p, curBlockSize, s, blocksz);
            assert(originSz == curBlockSize || (i + 1 == blocknum && originSz > 0));
            p += originSz;
        }
        s += blocksz;
    }

    auto len = (i64)(p - startDest);
    assert(len > 0 && len <= (toBlockIdx - fromBlockIdx) * blocksize);
    return len;
}

i64 CompressLib::decompressAllBlocks(int blocksize, int blocknum, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen) {
    assert(blocksize > 0 && blocknum > 0 && blocktable && dest && uncompressedlen > 0 && src && slen > 0);
    auto len = decompressAllBlocks(blocksize, blocknum, 0, blocknum, blocktable, dest, uncompressedlen, src, slen);
    assert(uncompressedlen == len);
    return len;
}

i64 decompressAllBlockSizes[20];

void call_decompressAllBlocks(int threadIdx, int blocksize, int blocknum, int fromBlockIdx, int toBlockIdx, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen) {
    auto sz = CompressLib::decompressAllBlocks(blocksize, blocknum, fromBlockIdx, toBlockIdx, blocktable, dest, uncompressedlen, src, slen);
    assert(sz > 0 && sz <= uncompressedlen);
    decompressAllBlockSizes[threadIdx] = sz;
}

i64 CompressLib::decompressAllBlocks(int numExtraThreads, int blocksize, int blocknum, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen) {
    if (numExtraThreads <= 0 || slen < 2 * 1024 * 1024) {
        return decompressAllBlocks(blocksize, blocknum, blocktable, dest, uncompressedlen, src, slen);
    }

    memset(decompressAllBlockSizes, 0, sizeof(decompressAllBlockSizes));

    std::vector<std::thread> threadVec;
    int bsz = blocknum / (numExtraThreads + 1);

    for (auto i = 0; i < numExtraThreads; ++i) {
        int fromIdx = bsz * (i + 1);
        int toIdx = i + 1 == numExtraThreads ? blocknum : (fromIdx + bsz);
        threadVec.push_back(std::thread(call_decompressAllBlocks, i + 1, blocksize, blocknum,
                                        fromIdx, toIdx,
                                        blocktable, dest, uncompressedlen, src, slen));
    }

    auto sz = decompressAllBlocks(blocksize, blocknum, 0, bsz, blocktable, dest, uncompressedlen, src, slen);
    decompressAllBlockSizes[0] = sz;

    for (auto && t : threadVec) {
        t.join();
    }
    threadVec.clear();

    i64 total = 0;
    for (auto i = 0; i <= numExtraThreads; ++i) {
        total += decompressAllBlockSizes[i];
    }
    assert(total == uncompressedlen);

    return total;
}


