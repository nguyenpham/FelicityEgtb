
/*
 This file is part of Felicity Egtb, distributed under MIT license.

 Copyright (c) 2018 Nguyen Hong Pham

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include <fstream>

#include "Egtb.h"
#include "EgtbKey.h"

using namespace egtb;


///////////////////////////////////////////////////////////////////////////////
// Constructors/Destructors
///////////////////////////////////////////////////////////////////////////////
const int EgtbLookup::luGroupSizes[2] = { 7, 22 };

EgtbLookup::EgtbLookup() {
    reset();
}

EgtbLookup::EgtbLookup(const std::string& path, EgtbMemMode memMode, EgtbLoadMode loadMode) {
    memset((void*)&sign, 0, EGTBLU_HEADER_SIZE);
    reset();
    preload(path, memMode, loadMode);
}

EgtbLookup::~EgtbLookup() {
    removeBuffers();
}

void EgtbLookup::removeBuffers() {
    for (int i = 0; i < 2; i++) {
        if (data[i]) {
            free(data[i]);
        }
    }
    if (blockTable) free(blockTable);
    if (keyTable) free(keyTable);
    if (pCompressBuf) free(pCompressBuf);

    reset();

    loadStatus = EgtbLoadStatus::none;
}

std::string EgtbLookup::getName() const {
    return luName;
}

Side EgtbLookup::getSide() const {
    return property & (1 << B) ? Side::black : Side::white;
}

void EgtbLookup::reset() {
    blockTable = nullptr;
    keyTable = nullptr;
    pCompressBuf = nullptr;
    for (int i = 0; i < 2; i++) {
        data[i] = nullptr;
        startpos[i] = endpos[i] = 0;
        bufsz[i] = 0;
    }
}


///////////////////////////////////////////////////////////////////////////////
// Preload data
///////////////////////////////////////////////////////////////////////////////

bool EgtbLookup::preload(const std::string& _path, EgtbMemMode _memMode, EgtbLoadMode _loadMode) {
    path = _path;
    memMode = _memMode;
    loadMode = _loadMode;

    loadStatus = EgtbLoadStatus::none;

    if (loadMode == EgtbLoadMode::onrequest) {
        luName = getFileName(path);
        if (luName.length() < 4) {
            return false;
        }
        toLower(luName);
        int loadingSd = luName.find("w") != std::string::npos ? W : B;
        luName = luName.substr(0, luName.length() - 1); // remove W / B
        property = (1 << loadingSd);
        return true;
    }

    return _preload();
}

bool EgtbLookup::_preload() {
    std::ifstream file(path, std::ios::binary);

    bool r = file
    && file.read((char *)&sign, EGTBLU_HEADER_SIZE) && sign == EGTBLU_HEADER_SIGN
    && itemCnt[0] >= 0 && itemCnt[1] >= 0 && itemCnt[0] + itemCnt[1] > 0;

    if (r) {
        toLower(theName);
        luName = theName;

        if (memMode == EgtbMemMode::smart) {
            memMode = (itemCnt[0] * luGroupSizes[0] + itemCnt[1] * luGroupSizes[1] < EGTBLU_SMART_MODE_THRESHOLD) ? EgtbMemMode::all : EgtbMemMode::tiny;
        }

        r = preload_tables(file);
    }

    file.close();

    if (!r && egtbVerbose) {
        std::cerr << "Error: cannot read data with path " << path << std::endl;
    }

    if (r && memMode == EgtbMemMode::all) {
        for(int grp = 0; grp < 2; grp++) {
            if (itemCnt[grp]) {
                getCell(grp, 0, 0);
            }
        }
    }

    loadStatus = r ? EgtbLoadStatus::loaded : EgtbLoadStatus::error;
    return r;
}


bool EgtbLookup::preload_tables(std::ifstream& file) {
    i64 seekpos = EGTBLU_HEADER_SIZE;
    file.seekg(seekpos, std::ios::beg);

    /*
     * Read key table
     */
    int totalCnt = itemCnt[0] + itemCnt[1];

    int keyTbSz = totalCnt * sizeof(u32);
    keyTable = (u32*) malloc(keyTbSz + 64);

    bool r = true;
    if (keyCompressedSize == 0) {
        if (!file.read((char *)keyTable, keyTbSz)) {
            r = false;
        }
    } else {
        char* buf = (char*) malloc(keyCompressedSize + 64);
        if (file.read(buf, keyCompressedSize)) {
            decompress((char*)keyTable, keyTbSz, buf, keyCompressedSize);
        } else {
            r = false;
        }
        free(buf);
    }

    if (!r) {
        free(keyTable);
        keyTable = nullptr;
        return false;
    }

    int blockNum = 0, maxLockSz = 0;
    for(int i = 0; i < 2; i++) {
        auto k = itemCnt[i];
        auto bufsz = k * luGroupSizes[i];
        maxLockSz = std::max(maxLockSz, bufsz);
        blockNum += (bufsz + EGTBLU_COMPRESS_BLOCK_SIZE - 1) / EGTBLU_COMPRESS_BLOCK_SIZE;
    }

    // Read compress blockTable
    if (property & EGTBLU_PROPERTY_COMPRESS) {
        int compSz = blockNum * sizeof(u32);
        blockTable = (u32*) malloc(compSz + 64);

        if (!file.read((char *)blockTable, compSz)) {
            r = false;
            free(blockTable);
            blockTable = nullptr;
        }
    }

    return r;
}


///////////////////////////////////////////////////////////////////////////////
// Get scores
///////////////////////////////////////////////////////////////////////////////

int EgtbLookup::getScore(i64 key64, int groupIdx, int subIdx) {
    if (!keyTable) {
        std::ifstream file(path, std::ios::binary);
        bool r = file && preload_tables(file) && keyTable;
        file.close();
        if (!r) {
            return EGTB_SCORE_MISSING;
        }
    }

    const u32* pKey = keyTable + (groupIdx == 0 ? 0 : itemCnt[0]);
    u32 key = (u32)key64;

    for (int i = 0, j = itemCnt[groupIdx] - 1; i <= j; ) {
        int idx = (i + j) / 2;
        auto t = pKey[idx];
        if (key == t) {
            return getCell(groupIdx, idx, subIdx);
        }
        if (key < t) j = idx - 1;
        else i = idx + 1;
    }

    return EGTB_SCORE_MISSING;
}

int EgtbLookup::getCell(int groupIdx, int itemidx, int subIdx) {
    std::lock_guard<std::mutex> lock(mtx);

    bool r = true;
    if (!data[groupIdx] || itemidx < startpos[groupIdx] || itemidx >= endpos[groupIdx]) {
        r = false;
        if (!data[groupIdx]) {
            if (memMode == EgtbMemMode::all || !(property & EGTBLU_PROPERTY_COMPRESS)) {
                bufsz[groupIdx] = itemCnt[groupIdx] * luGroupSizes[groupIdx];
            } else {
                bufsz[groupIdx] = EGTBLU_COMPRESS_BLOCK_SIZE;
            }

            data[groupIdx] = (int8_t*) malloc(bufsz[groupIdx] + 64);
        }

        std::ifstream file(path, std::ios::binary);
        i64 seekpos = EGTBLU_HEADER_SIZE + (keyCompressedSize > 0 ? keyCompressedSize : ((itemCnt[0] + itemCnt[1]) * sizeof(u32)));

        int sz[2];
        sz[0] = itemCnt[0] * luGroupSizes[0];
        sz[1] = itemCnt[1] * luGroupSizes[1];

        if (property & EGTBLU_PROPERTY_COMPRESS) {
            int blockCnt[2];
            blockCnt[0] = (sz[0] + EGTBLU_COMPRESS_BLOCK_SIZE - 1) / EGTBLU_COMPRESS_BLOCK_SIZE;
            blockCnt[1] = (sz[1] + EGTBLU_COMPRESS_BLOCK_SIZE - 1) / EGTBLU_COMPRESS_BLOCK_SIZE;

            int compTableSz = (blockCnt[0] + blockCnt[1]) * sizeof(u32);
            seekpos += compTableSz;

            if (memMode == EgtbMemMode::all) {
                int compressSz0 = blockCnt[0] > 0 ? blockTable[blockCnt[0] - 1] & ~EGTB_UNCOMPRESS_BIT : 0;
                int compressSz = groupIdx == 0 ? compressSz0 : (blockTable[blockCnt[0] + blockCnt[1] - 1] & ~EGTB_UNCOMPRESS_BIT);

                pCompressBuf = (char*)malloc(compressSz + 64);

                if (groupIdx == 1) {
                    seekpos += compressSz0;
                }
                file.seekg(seekpos, std::ios::beg);
                if (file.read(pCompressBuf, compressSz)) {
                    int addBCnt = groupIdx == 0 ? 0 : blockCnt[0];

                    auto originSz = decompressAllBlocks(EGTBLU_COMPRESS_BLOCK_SIZE, blockCnt[groupIdx], (u8*)(blockTable + addBCnt), (char*)data[groupIdx], sz[groupIdx], pCompressBuf, compressSz);
                    if (originSz > 0) {
                        startpos[groupIdx] = 0;
                        endpos[groupIdx] = itemCnt[groupIdx];
                        r = true;
                    }
                }
                free(pCompressBuf);
                pCompressBuf = nullptr;
            } else {
                int addBCnt = 0;
                if (groupIdx == 1) {
                    addBCnt = blockCnt[0];
                    int compressSz0 = blockCnt[0] > 0 ? blockTable[blockCnt[0] - 1] & ~EGTB_UNCOMPRESS_BIT : 0;
                    seekpos += compressSz0;
                }

                int blockIdx = itemidx * luGroupSizes[groupIdx] / EGTBLU_COMPRESS_BLOCK_SIZE;
                int offset = blockIdx == 0 ? 0 : (blockTable[addBCnt + blockIdx - 1] & ~EGTB_UNCOMPRESS_BIT);
                int readsz = (blockTable[addBCnt + blockIdx] & ~EGTB_UNCOMPRESS_BIT) - offset;

                if (!pCompressBuf) {
                    pCompressBuf = (char*)malloc(EGTBLU_COMPRESS_BLOCK_SIZE * 2 + 64);
                }

                seekpos += offset;
                file.seekg(seekpos, std::ios::beg);
                if (file.read(pCompressBuf, readsz)) {
                    int leftSz = sz[groupIdx] - itemidx * luGroupSizes[groupIdx];
                    int blockSize = MIN(leftSz, EGTBLU_COMPRESS_BLOCK_SIZE);
                    auto originSz = decompress((char*)data[groupIdx], blockSize, pCompressBuf, readsz);

                    if (originSz > 0) {
                        startpos[groupIdx] = blockIdx * EGTBLU_COMPRESS_BLOCK_SIZE / luGroupSizes[groupIdx];
                        endpos[groupIdx] = startpos[groupIdx] + (originSz + luGroupSizes[groupIdx] - 1) / luGroupSizes[groupIdx];
                        r = true;
                    }
                }
            }
        } else { // Not compressed
            if (groupIdx == 1) {
                seekpos += sz[0];
            }
            file.seekg(seekpos, std::ios::beg);
            if (file.read((char*)data[groupIdx], bufsz[groupIdx])) {
                startpos[groupIdx] = 0;
                endpos[groupIdx] = itemCnt[groupIdx];
                r = true;
            }
        }
    }

    if (!r) {
        return EGTB_SCORE_MISSING;
    }

    const int8_t* pScore = data[groupIdx];
    i64 idx = (itemidx - startpos[groupIdx]) * luGroupSizes[groupIdx] + subIdx;

    int score = (int)pScore[idx];
    if (isVersion2()) {
        score = EgtbFile::_cellToScore(score);
    } else if (score != 0) {
        score += score > 0 ? EGTB_SCORE_BASE : -EGTB_SCORE_BASE;
    }
    return score;
}

int EgtbLookup::lookup(const int* pieceList, Side side, const int* idxArr, const i64* idxMult, u32 order) {
    if (loadStatus == EgtbLoadStatus::none) {
        std::lock_guard<std::mutex> thelock(loadMutex);
        if (loadStatus == EgtbLoadStatus::none) {
            _preload();
        }
    }

    if (loadStatus == EgtbLoadStatus::error) {
        return EGTB_SCORE_MISSING;
    }

    EgtbKeyRec rec;
    EgtbKey::getKey(rec, pieceList, EgtbType::newdtm, true, idxArr, idxMult, order);
    return getScore(rec.key, rec.groupIdx, rec.subKey);
}

