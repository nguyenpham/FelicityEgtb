
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
#include <iomanip>
#include <ctime>

#include "Egtb.h"
#include "EgtbKey.h"


using namespace egtb;


//////////////////////////////////////////////////////////////////////
#define EGTB_SIZE_KAAEE                     1410
#define EGTB_SIZE_KAEE                      810
#define EGTB_SIZE_KAAE                      480
#define EGTB_SIZE_KAE                       275
#define EGTB_SIZE_KEE                       183
#define EGTB_SIZE_KAA                       70
#define EGTB_SIZE_KE                        62
#define EGTB_SIZE_KA                        40
#define EGTB_SIZE_K                         9

#define EGTB_SIZE_KM                        17
#define EGTB_SIZE_KAM                       75
#define EGTB_SIZE_KAAM                      130


#define    EGTB_MATERIAL_SIGN_NONE          (0)

#define    EGTB_MATERIAL_SIGN_1_BISHOP      (1<<0)
#define    EGTB_MATERIAL_SIGN_2_BISHOPS     (3<<0)

#define    EGTB_MATERIAL_SIGN_1_ELEPHANT    (1<<2)
#define    EGTB_MATERIAL_SIGN_2_ELEPHANTS   (3<<2)

#define    EGTB_MATERIAL_SIGN_1_ROOK        (1<<4)
#define    EGTB_MATERIAL_SIGN_2_ROOKS       (3<<4)
#define    EGTB_MATERIAL_SIGN_1_CANNON      (1<<6)
#define    EGTB_MATERIAL_SIGN_2_CANNONS     (3<<6)
#define    EGTB_MATERIAL_SIGN_1_KNIGHT      (1<<8)
#define    EGTB_MATERIAL_SIGN_2_KNIGHTS     (3<<8)

#define    EGTB_MATERIAL_SIGN_1_PAWN        (1<<10)
#define    EGTB_MATERIAL_SIGN_2_PAWNS       (3<<10)
#define    EGTB_MATERIAL_SIGN_3_PAWNS       (7<<10)
#define    EGTB_MATERIAL_SIGN_4_PAWNS       (0x0f<<10)
#define    EGTB_MATERIAL_SIGN_5_PAWNS       (0x1f<<10)

static const int m_pieceSign[7][6] = {
    { EGTB_MATERIAL_SIGN_NONE, EGTB_MATERIAL_SIGN_NONE },
    { EGTB_MATERIAL_SIGN_NONE, EGTB_MATERIAL_SIGN_1_BISHOP, EGTB_MATERIAL_SIGN_2_BISHOPS },
    { EGTB_MATERIAL_SIGN_NONE, EGTB_MATERIAL_SIGN_1_ELEPHANT, EGTB_MATERIAL_SIGN_2_ELEPHANTS },
    { EGTB_MATERIAL_SIGN_NONE, EGTB_MATERIAL_SIGN_1_ROOK, EGTB_MATERIAL_SIGN_2_ROOKS },
    { EGTB_MATERIAL_SIGN_NONE, EGTB_MATERIAL_SIGN_1_CANNON, EGTB_MATERIAL_SIGN_2_CANNONS },
    { EGTB_MATERIAL_SIGN_NONE, EGTB_MATERIAL_SIGN_1_KNIGHT, EGTB_MATERIAL_SIGN_2_KNIGHTS },
    { EGTB_MATERIAL_SIGN_NONE, EGTB_MATERIAL_SIGN_1_PAWN, EGTB_MATERIAL_SIGN_2_PAWNS, EGTB_MATERIAL_SIGN_3_PAWNS, EGTB_MATERIAL_SIGN_4_PAWNS, EGTB_MATERIAL_SIGN_5_PAWNS }
};

//////////////////////////////////////////////////////////////////////
static const char* egtbFileExtensions[] = {
    ".xtb", ".ntb", ".ltb", nullptr
};

static const char* egtbCompressFileExtensions[] = {
    ".ztb", ".znb", ".zlt", nullptr
};

std::pair<EgtbType, bool> EgtbFile::getExtensionType(const std::string& path) {
    std::pair<EgtbType, bool> p;
    p.first = EgtbType::none; p.second = false;

    for (int i = 0; egtbFileExtensions[i]; i++) {
        if (path.find(egtbFileExtensions[i]) != std::string::npos || path.find(egtbCompressFileExtensions[i]) != std::string::npos) {
            p.first = static_cast<EgtbType>(i);
            p.second = path.find(egtbCompressFileExtensions[i]) != std::string::npos;
            break;
        }
    }
    return p;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
EgtbFile::EgtbFile() {
    pBuf[0] = pBuf[1] = pCompressBuf = nullptr;
    compressBlockTables[0] = compressBlockTables[1] = nullptr;
    lookups[0] = lookups[1] = nullptr;
    header = nullptr;
    memMode = EgtbMemMode::tiny;
    loadStatus = EgtbLoadStatus::none;
    reset();
}

EgtbFile::~EgtbFile() {
    removeBuffers();
    if (lookups[0]) delete lookups[0];
    if (lookups[1]) delete lookups[1];
    lookups[0] = lookups[1] = nullptr;
    if (header) {
        delete header;
        header = nullptr;
    }
}

void EgtbFile::reset() {
    if (header) {
        header->reset();
    }

    path[0] = path[1]= "";
    startpos[0] = 0; endpos[0] = 0; startpos[1] = 0; endpos[1] = 0;
};

void EgtbFile::removeBuffers() {
    if (pCompressBuf) free(pCompressBuf);
    pCompressBuf = nullptr;

    for (int i = 0; i < 2; i++) {
        if (pBuf[i]) {
            free(pBuf[i]);
            pBuf[i] = nullptr;
        }

        if (compressBlockTables[i]) {
            free(compressBlockTables[i]);
            compressBlockTables[i] = nullptr;
        }

        if (lookups[i]) {
            lookups[i]->removeBuffers();
        }

        startpos[i] = endpos[i] = 0;
    }

    loadStatus = EgtbLoadStatus::none;
}

//////////////////////////////////////////////////////////////////////
bool EgtbFile::addLookup(EgtbLookup* lookup) {
    auto sd = static_cast<int>(lookup->getSide());
    if ((sd == 0 || sd == 1) && !lookups[sd]) {
        lookups[sd] = lookup;
        return true;
    }
    return false;
}

void EgtbFile::merge(EgtbFile& otherEgtbFile)
{
    for(int sd = 0; sd < 2; sd++) {
        Side side = static_cast<Side>(sd);

        if (header == nullptr) {
            auto path = otherEgtbFile.getPath(sd);
            if (!path.empty()) {
                setPath(path, sd);
            }
            continue;
        }

        if (otherEgtbFile.header->isSide(side)) {

            header->addSide(side);
            setPath(otherEgtbFile.getPath(sd), sd);

            if (compressBlockTables[sd]) {
                free(compressBlockTables[sd]);
            }
            compressBlockTables[sd] = otherEgtbFile.compressBlockTables[sd];

            if (pBuf[sd] == nullptr && otherEgtbFile.pBuf[sd] != nullptr) {
                pBuf[sd] = otherEgtbFile.pBuf[sd];
                startpos[sd] = otherEgtbFile.startpos[sd];
                endpos[sd] = otherEgtbFile.endpos[sd];

                otherEgtbFile.pBuf[sd] = nullptr;
                otherEgtbFile.startpos[sd] = 0;
                otherEgtbFile.endpos[sd] = 0;
                otherEgtbFile.compressBlockTables[sd] = nullptr;
            }
        }
    }
}

void EgtbFile::setPath(const std::string& s, int sd) {
    auto ss = s;
    toLower(ss);
    if (sd != 0 && sd != 1) {
        sd = (ss.find("w.") != std::string::npos) ? W : B;
    }
    path[sd] = s;
}

bool EgtbFile::createBuf(i64 len, int sd) {
    pBuf[sd] = (char *)malloc((size_t)len + 16);
    startpos[sd] = 0; endpos[sd] = 0;
    return pBuf[sd];
}

i64 EgtbFile::getBufItemCnt() const {
    if (memMode == EgtbMemMode::tiny) {
        return EGTB_SIZE_COMPRESS_BLOCK;
    }
    return getSize();
}

//////////////////////////////////////////////////////////////////////
// Preload files
//////////////////////////////////////////////////////////////////////
bool EgtbFile::preload(const std::string& path, EgtbMemMode _memMode, EgtbLoadMode _loadMode) {
    if (_memMode == EgtbMemMode::smart) {
        _memMode = getSize() < EGTB_SMART_MODE_THRESHOLD ? EgtbMemMode::all : EgtbMemMode::tiny;
    }

    memMode = _memMode;
    loadMode = _loadMode;

    loadStatus = EgtbLoadStatus::none;
    if (loadMode == EgtbLoadMode::onrequest) {
        auto theName = getFileName(path);
        if (theName.length() < 4) {
            return false;
        }
        toLower(theName);
        int loadingSd = theName.find("w") != std::string::npos ? W : B;
        setPath(path, loadingSd);

        theName = theName.substr(0, theName.length() - 1); // remove W / B
        egtbName = theName;
        egtbType = theName.find("m") != std::string::npos ? EgtbType::newdtm : EgtbType::dtm;

        setupIdxComputing(getName(), 0);
        return true;
    }

    bool r = loadHeaderAndTable(path);
    loadStatus = r ? EgtbLoadStatus::loaded : EgtbLoadStatus::error;
    return r;
}

// Load all data too if requested
bool EgtbFile::loadHeaderAndTable(const std::string& path) {

    std::ifstream file(path, std::ios::binary);

    // if there are files for both sides, header has been created already
    auto oldSide = Side::none;
    if (header == nullptr) {
        header = new EgtbFileHeader();
    } else {
        oldSide = header->isSide(Side::black) ? Side::black : Side::white;
    }
    auto loadingSide = Side::none;

    bool r = false;
    if (file && header->readFile(file) && header->isValid()) {
        loadingSide = header->isSide(Side::white) ? Side::white : Side::black;
        toLower(header->name);
        egtbName = header->name;
        egtbType = (header->property & EGTB_PROP_NEW) ? EgtbType::newdtm : EgtbType::dtm;

        setPath(path, static_cast<int>(loadingSide));
        header->setOnlySide(loadingSide);
        r = loadingSide != Side::none;

        if (r) {
            setupIdxComputing(getName(), header->order);
            if (oldSide != Side::none) {
                header->addSide(oldSide);
            }
        }
    }

    auto sd = static_cast<int>(loadingSide);
    startpos[sd] = endpos[sd] = 0;

    if (r && isCompressed()) {
        // Create & read compress block table
        auto blockCnt = getCompresseBlockCount();
        int blockTableSz = blockCnt * sizeof(u32);

        compressBlockTables[sd] = (u32*) malloc(blockTableSz + 64);

        if (!file.read((char *)compressBlockTables[sd], blockTableSz)) {
            if (egtbVerbose) {
                std::cerr << "Error: cannot read " << path << std::endl;
            }
            file.close();
            free(compressBlockTables[sd]);
            compressBlockTables[sd] = nullptr;
            return false;
        }
    }

    if (r && memMode == EgtbMemMode::all) {
        r = loadAllData(file, loadingSide);
    }
    file.close();

    if (!r && egtbVerbose) {
        std::cerr << "Error: cannot read " << path << std::endl;
    }

    return r;
}

bool EgtbFile::loadAllData(std::ifstream& file, Side side)
{
    auto sd = static_cast<int>(side);
    startpos[sd] = endpos[sd] = 0;

    if (isCompressed()) {
        auto blockCnt = getCompresseBlockCount();
        int blockTableSz = blockCnt * sizeof(u32);

        i64 seekpos = EGTB_HEADER_SIZE + blockTableSz;
        file.seekg(seekpos, std::ios::beg);

        createBuf(getSize(), sd);

        auto compDataSz = compressBlockTables[sd][blockCnt - 1] & ~EGTB_UNCOMPRESS_BIT;
        char* tempBuf = (char*) malloc(compDataSz + 64);
        if (file.read(tempBuf, compDataSz)) {
            auto originSz = decompressAllBlocks(EGTB_SIZE_COMPRESS_BLOCK, blockCnt, compressBlockTables[sd], (char*)pBuf[sd], getSize(), tempBuf, compDataSz);
            endpos[sd] = originSz;
        }

        free(tempBuf);
        free(compressBlockTables[sd]);
        compressBlockTables[sd] = nullptr;
    } else {
        auto sz = getSize();
        createBuf(sz, sd);
        i64 seekpos = EGTB_HEADER_SIZE;
        file.seekg(seekpos, std::ios::beg);

        if (file.read(pBuf[sd], sz)) {
            endpos[sd] = sz;
        }
    }

    return startpos[sd] < endpos[sd];
}

void EgtbFile::checkToLoadHeaderAndTable(Side side) {
    if (loadStatus != EgtbLoadStatus::none) {
        return;
    }

    std::lock_guard<std::mutex> thelock(mtx);
    if (loadStatus != EgtbLoadStatus::none) {
        return;
    }

    bool r = false;
    if (!path[0].empty() && !path[1].empty()) {
        r = loadHeaderAndTable(path[0]) && loadHeaderAndTable(path[1]);
    } else {
        auto thepath = path[0].empty() ? path[1] : path[0];
        r = loadHeaderAndTable(thepath);
    }

    loadStatus = r ? EgtbLoadStatus::loaded : EgtbLoadStatus::error;
}

bool EgtbFile::readBuf(i64 idx, int sd)
{
    if (!pBuf[sd]) {
        createBuf(getBufSize(), sd);
    }

    auto bufCnt = std::min(getBufItemCnt(), getSize() - idx);
    auto bufsz = bufCnt;

    bool r = false;
    std::ifstream file(getPath(sd), std::ios::binary);
    if (file) {
        if (memMode == EgtbMemMode::all) {
            Side side = static_cast<Side>(sd);
            r = loadAllData(file, side);
        } else if (isCompressed() && compressBlockTables[sd]) {
            r = readCompressedBlock(file, idx, sd, (char*)pBuf[sd]);
        } else {
            auto beginIdx = (idx + bufCnt <= getSize()) ? idx : 0;
            auto x = beginIdx;
            i64 seekpos = EGTB_HEADER_SIZE + x;
            file.seekg(seekpos, std::ios::beg);

            if (file.read(pBuf[sd], bufsz)) {
                startpos[sd] = beginIdx;
                endpos[sd] = beginIdx + bufCnt;
                r = true;
            }
        }
    }
    file.close();

    if (!r && egtbVerbose) {
        std::cerr << "Error: Cannot open file " << getPath(sd) << std::endl;
    }

    return r;
}

bool EgtbFile::readCompressedBlock(std::ifstream& file, i64 idx, int sd, char* pDest)
{
    auto blockCnt = getCompresseBlockCount();
    int blockTableSz = blockCnt * sizeof(u32);

    const int blockSize = EGTB_SIZE_COMPRESS_BLOCK;
    auto blockIdx = idx / blockSize;
    startpos[sd] = endpos[sd] = blockIdx * blockSize;

    auto iscompressed = !(compressBlockTables[sd][blockIdx] & EGTB_UNCOMPRESS_BIT);
    auto blockOffset = blockIdx == 0 ? 0 : (compressBlockTables[sd][blockIdx - 1] & ~EGTB_UNCOMPRESS_BIT);
    auto compDataSz = (compressBlockTables[sd][blockIdx] & ~EGTB_UNCOMPRESS_BIT) - blockOffset;

    i64 seekpos = EGTB_HEADER_SIZE + blockTableSz + blockOffset;
    file.seekg(seekpos, std::ios::beg);

    if (iscompressed) {
        if (pCompressBuf == nullptr) {
            pCompressBuf = (char*) malloc(blockSize * 3 / 2);
        }
        if (file.read(pCompressBuf, compDataSz)) {
            auto curBlockSize = (int)MIN(getSize() - startpos[sd], (i64)blockSize);
            auto originSz = decompress(pDest, curBlockSize, pCompressBuf, compDataSz);
            endpos[sd] += originSz;
            return true;
        }
    } else if (file.read(pDest, compDataSz)) {
        endpos[sd] += compDataSz;
        return true;
    }

    if (egtbVerbose) {
        std::cerr << "Error: cannot read " << getPath(sd) << std::endl;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// Get scores
//////////////////////////////////////////////////////////////////////
#define TB_ILLEGAL              0
#define TB_UNSET                1
#define TB_MISSING              2
#define TB_WINING               3
#define TB_UNKNOWN              4
#define TB_DRAW                 5

#define EGTB_START_MATING       (TB_DRAW + 1)
#define EGTB_START_LOSING       130

int EgtbFile::cellToScore(char cell) {
    u8 s = (u8)cell;
    if (s >= TB_DRAW) {
        if (s == TB_DRAW) return EGTB_SCORE_DRAW;
        if (s < EGTB_START_LOSING) {
            int mi = (s - EGTB_START_MATING) * 2 + 1;
            return EGTB_SCORE_MATE - mi;
        }

        int mi = (s - EGTB_START_LOSING) * 2;
        return -EGTB_SCORE_MATE + mi;
    }

    switch (s) {
        case TB_MISSING:
            return EGTB_SCORE_MISSING;

        case TB_WINING:
            return EGTB_SCORE_WINNING;

        case TB_UNKNOWN:
            return EGTB_SCORE_UNKNOWN;

        case TB_ILLEGAL:
            return EGTB_SCORE_ILLEGAL;

        case TB_UNSET:
        default:
            return EGTB_SCORE_ILLEGAL;
    }
}

char EgtbFile::getCell(i64 idx, Side side)
{
    if (idx >= getSize()) {
        return TB_MISSING;
    }

    int sd = static_cast<int>(side);

    if (!isDataReady(idx, sd)) {
        if (!readBuf(idx, sd)) {
            return TB_MISSING;
        }
    }

    char ch = pBuf[sd][idx - startpos[sd]];
    return ch;
}

char EgtbFile::getCell(const int* pieceList, Side side) {
    i64 key = getKey(pieceList).first;
    return getCell(key, side);
}

int EgtbFile::getScore(const EgtbBoard& board, Side side, bool useLock)
{
    return getScore((const int*) board.pieceList, side, useLock);
}

int EgtbFile::getScoreNoLock(const EgtbBoard& board, Side side) {
    return getScoreNoLock((const int*) board.pieceList, side);
}

int EgtbFile::getScoreNoLock(const int* pieceList, Side side) {
    auto r = getKey(pieceList);
    if (r.second) {
        side = getXSide(side);
    }
    return getScoreNoLock(r.first, side);
}

int EgtbFile::getScoreNoLock(i64 idx, Side side)
{
    char cell = getCell(idx, side);
    return cellToScore(cell);
}

int EgtbFile::getScore(i64 idx, Side side, bool useLock)
{
    checkToLoadHeaderAndTable(side);
    if (useLock && (memMode != EgtbMemMode::all || !isDataReady(idx, static_cast<int>(side)))) {
        std::lock_guard<std::mutex> thelock(sdmtx[static_cast<int>(side)]);
        return getScoreNoLock(idx, side);
    }
    return getScoreNoLock(idx, side);
}

int EgtbFile::getScore(const int* pieceList, Side side, bool useLock) {
    auto r = getKey(pieceList);
    if (r.second) {
        side = getXSide(side);
    }
    return getScore(r.first, side, useLock);
}

int EgtbFile::lookup(const int *pieceList, Side side) {
    auto loopup = lookups[static_cast<int>(side)];
    return loopup ? loopup->lookup(pieceList, side, idxArr, idxMult, header->order) : EGTB_SCORE_MISSING;
}

//////////////////////////////////////////////////////////////////////
i64 EgtbFile::parseAttr(const std::string& name, int* idxArr, i64* idxMult, int* pieceCount, EgtbType egtbType, u16 order)
{
    int k = 0;
    for (int i = 0, sd = W; i < (int)name.size(); i++) {
        switch (name[i]) {
            case 'r': {
                idxArr[k++] = EGTB_IDX_R;
                break;
            }

            case 'c': {
                switch (name[i + 1]) {
                    case 'c':
                        i++;
                        idxArr[k++] = EGTB_IDX_CC;
                        break;

                    case 'h':
                        i++;
                        idxArr[k++] = EGTB_IDX_C;
                        idxArr[k++] = EGTB_IDX_H_Full;
                        break;

                    case 'p':
                        i++;
                        idxArr[k++] = EGTB_IDX_C;
                        idxArr[k++] = EGTB_IDX_P_Full;
                        break;

                    default:
                        idxArr[k++] = EGTB_IDX_C;
                        break;
                }
                break;
            }

            case 'h': {
                switch (name[i + 1]) {
                    case 'h':
                        i++;
                        idxArr[k++] = EGTB_IDX_HH;
                        break;

                    case 'p':
                        i++;
                        idxArr[k++] = EGTB_IDX_H;
                        idxArr[k++] = EGTB_IDX_P_Full;
                        break;

                    default:
                        idxArr[k++] = EGTB_IDX_H;
                        break;
                }
                break;
            }

            case 'p': {
                if (name[i + 1] == 'p') {
                    i++;
                    if (name[i + 1] == 'p') {
                        idxArr[k++] = EGTB_IDX_PPP;
                        i++;
                    } else {
                        idxArr[k++] = EGTB_IDX_PP;
                    }
                } else {
                    idxArr[k++] = EGTB_IDX_P;
                }
                break;
            }

            case 'a':
            case 'e':
            case 'm':
                break;

            case 'k':
            {
                sd = i == 0 ? W : B;
                int a = 0, e = 0;
                for(int j = i + 1; j < name.length(); j++) {
                    char ch = name[j];
                    if (ch == 'k') {
                        break;
                    }
                    if (ch == 'a') {
                        a++;
                    } else if (ch == 'e') {
                        e++;
                    } else if (ch == 'm') {
                        e = 1;
                    }
                }

                switch (a + e) {
                    case 0:
                        idxArr[k++] = EGTB_IDX_DK;
                        break;
                    case 1:
                        if (a) {
                            idxArr[k++] = EGTB_IDX_DA;
                        } else {
                            auto t = (sd == W && egtbType == EgtbType::newdtm) ? EGTB_IDX_DM : EGTB_IDX_DE;
                            idxArr[k++] = t;
                        }
                        break;
                    case 2:
                        if (sd == W && egtbType == EgtbType::newdtm) {
                            idxArr[k++] = EGTB_IDX_DAM;
                        } else {
                            if (a == 0) {
                                idxArr[k++] = EGTB_IDX_DEE;
                            } else if (a == 1) {
                                idxArr[k++] = EGTB_IDX_DAE;
                            } else {
                                idxArr[k++] = EGTB_IDX_DAA;
                            }
                        }
                        break;
                    case 3:
                        if (sd == W && egtbType == EgtbType::newdtm) {
                            idxArr[k++] = EGTB_IDX_DAAM;
                        } else {
                            if (a==1) {
                                idxArr[k++] = EGTB_IDX_DAEE;
                            } else {
                                idxArr[k++] = EGTB_IDX_DAAE;
                            }
                        }
                        break;
                    case 4:
                        idxArr[k++] = EGTB_IDX_DAAEE;
                        break;
                }
                break;
            }

            default:
                break;
        }
    }

    idxArr[k] = EGTB_IDX_NONE;

    if (order != 0) {
        const int orderArray[] = { order & 0x7, (order >> 3) & 0x7, (order >> 6) & 0x7, (order >> 9) & 0x7 };
        int tmpArr[10];
        memcpy(tmpArr, idxArr, k * sizeof(int));
        for(int i = 0; i < k; i++) {
            int x = orderArray[i];
            idxArr[x] = tmpArr[i];
        }
        return parseAttr(idxArr, idxMult, pieceCount, (int*)orderArray);
    }

    return parseAttr(idxArr, idxMult, pieceCount, nullptr);
}

i64 EgtbFile::parseAttr(const int* idxArr, i64* idxMult, int* pieceCount, int* orderArray) {
    memset(pieceCount, 0, 2 * 7 * sizeof(int));

    pieceCount[static_cast<int>(PieceType::king)] = 1;
    pieceCount[7 + static_cast<int>(PieceType::king)] = 1;

    i64 sz = 1;
    int n = 0;
    for (int i = 0; ; i++, n++) {
        auto a = idxArr[i];
        if (a == EGTB_IDX_NONE) {
            break;
        }

        i64 h = 0;
        int pieceCountSd = 7;
        switch (a) {
            case EGTB_IDX_DK:
                h = EGTB_SIZE_K;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                break;

            case EGTB_IDX_DA:
                h = EGTB_SIZE_KA;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 1;
                break;

            case EGTB_IDX_DAA:
                h = EGTB_SIZE_KAA;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 2;
                break;

            case EGTB_IDX_DE:
                h = EGTB_SIZE_KE;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::elephant)] = 1;
                break;

            case EGTB_IDX_DEE:
                h = EGTB_SIZE_KEE;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::elephant)] = 2;
                break;

            case EGTB_IDX_DAE:
                h = EGTB_SIZE_KAE;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 1;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::elephant)] = 1;
                break;

            case EGTB_IDX_DAEE:
                h = EGTB_SIZE_KAEE;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 1;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::elephant)] = 2;
                break;

            case EGTB_IDX_DAAE:
                h = EGTB_SIZE_KAAE;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 2;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::elephant)] = 1;
                break;

            case EGTB_IDX_DAAEE:
                h = EGTB_SIZE_KAAEE;
                if ((orderArray && orderArray[0] != i) || (!orderArray && i >= 2)) {
                    pieceCountSd = 0;
                }
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 2;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::elephant)] = 2;
                break;

            case EGTB_IDX_DM:
                h = EGTB_SIZE_KM;
                break;

            case EGTB_IDX_DAM:
                h = EGTB_SIZE_KAM;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 1;
                break;

            case EGTB_IDX_DAAM:
                h = EGTB_SIZE_KAAM;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::advisor)] = 2;
                break;

            case EGTB_IDX_R:
                h = EGTB_SIZE_R_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::rook)] = 1;
                break;

            case EGTB_IDX_C:
                h = EGTB_SIZE_R_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::cannon)] = 1;
                break;

            case EGTB_IDX_H:
                h = EGTB_SIZE_R_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::horse)] = 1;
                break;

            case EGTB_IDX_P:
                h = EGTB_SIZE_P_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::pawn)] = 1;
                break;

            case EGTB_IDX_R_Full:
                h = EGTB_SIZE_R;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::rook)] = 1;
                break;

            case EGTB_IDX_C_Full:
                h = EGTB_SIZE_R;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::cannon)] = 1;
                break;

            case EGTB_IDX_H_Full:
                h = EGTB_SIZE_R;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::horse)] = 1;
                break;

            case EGTB_IDX_P_Full:
                h = EGTB_SIZE_P;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::pawn)] = 1;
                break;

            case EGTB_IDX_CC:
                h = EGTB_SIZE_RR_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::cannon)] = 2;
                break;

            case EGTB_IDX_CH:
                h = EGTB_SIZE_RC_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::cannon)] = 1;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::horse)] = 1;
                break;

            case EGTB_IDX_CP:
                h = EGTB_SIZE_RP_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::cannon)] = 1;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::pawn)] = 1;
                break;

            case EGTB_IDX_HH:
                h = EGTB_SIZE_RR_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::horse)] = 2;
                break;

            case EGTB_IDX_HP:
                h = EGTB_SIZE_RP_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::horse)] = 1;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::pawn)] = 1;
                break;

            case EGTB_IDX_PP:
                h = EGTB_SIZE_PP_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::pawn)] = 2;
                break;

            case EGTB_IDX_PPP:
                h = EGTB_SIZE_PPP_HALF;
                pieceCount[pieceCountSd + static_cast<int>(PieceType::pawn)] = 3;
                break;

            default:
                break;
        }

        idxMult[i] = h;
        sz *= h;
    }

    for(int i = 0; i < n; i++) {
        idxMult[i] = 1;
        for (int j = i + 1; j < n; j++) {
            idxMult[i] *= idxMult[j];
        }
    }

    return sz;
}

i64 EgtbFile::setupIdxComputing(const std::string& name, int order)
{
    auto egtbType = name.find("m") != std::string::npos ? EgtbType::newdtm : EgtbType::dtm;
    size = parseAttr(name.c_str(), idxArr, idxMult, (int*)pieceCount, egtbType, order);

    int materialsignW = 0, materialsignB = 0;
    for(int type = 1; type < 7; type++) {
        int countW = m_pieceSign[type][pieceCount[W][type]];
        int countB = m_pieceSign[type][pieceCount[B][type]];
        if (egtbType == EgtbType::newdtm && type == static_cast<int>(PieceType::elephant)) {
            countW = 0;
        }
        materialsignW |= countW;
        materialsignB |= countB;
    }

    materialsignWB = materialsignW | (materialsignB << 16);
    materialsignBW = materialsignB | (materialsignW << 16);

    return size;
}

i64 EgtbFile::computeSize(const std::string &name, EgtbType egtbType)
{
    int     idxArr[32];
    i64     idxMult[32];
    int     pieceCount[2][7];
    return  parseAttr(name, idxArr, idxMult, (int*)pieceCount, egtbType, 0);
}


i64 EgtbFile::computeMaterialSigns(const std::string &name, EgtbType egtbType, int* idxArr, i64* idxMult, u16 order)
{
    int pieceCount[2][7];
    parseAttr(name, idxArr, idxMult, (int*)pieceCount, egtbType, order);

    int materialsignW = 0, materialsignB = 0;
    for(int type = 1; type < 7; type++) {
        int countW = m_pieceSign[type][pieceCount[W][type]];
        int countB = m_pieceSign[type][pieceCount[B][type]];
        if (egtbType == EgtbType::newdtm && strchr(name.c_str(), 'm') && type == static_cast<int>(PieceType::elephant)) {
            countW = 0;
        }
        materialsignW |= countW;
        materialsignB |= countB;
    }

    int materialsignWB = materialsignW | (materialsignB << 16);
    int materialsignBW = materialsignB | (materialsignW << 16);

    return materialsignWB | (i64)materialsignBW << 32;
}

u64 EgtbFile::pieceListToMaterialSign(const int* pieceList)
{
    int materialsignW = 0, materialsignB = 0;
    Side strongSide = Side::none;

    for (int type = 1; type < 6; type++) {
        int k = egtbPieceListStartIdxByType[type];
        int countB = (pieceList[k] >= 0 ? 1 : 0) + (pieceList[k + 1] >= 0 ? 1 : 0);
        int countW = (pieceList[k + 16] >= 0 ? 1 : 0) + (pieceList[k + 1 + 16] >= 0 ? 1 : 0);

        materialsignW |= m_pieceSign[type][countW];
        materialsignB |= m_pieceSign[type][countB];

        if (strongSide == Side::none && type >= static_cast<int>(PieceType::rook)) {
            if (countW != countB) {
                strongSide = countW > countB ? Side::white : Side::black;
            }
        }
    }

    // Pawns
    int countW = 0, countB = 0;
    int k = egtbPieceListStartIdxByType[6];
    for(int i = 0; i < 5; i++) {
        if (pieceList[k + i] >= 0) {
            countB++;
        }
        if (pieceList[k + i + 16] >= 0) {
            countW++;
        }
    }
    materialsignW |= m_pieceSign[6][countW];
    materialsignB |= m_pieceSign[6][countB];

    if (strongSide == Side::none && countW != countB) {
        strongSide = countW > countB ? Side::white : Side::black;
    }

    u64 fullMat = materialsignW | (materialsignB << 16);

    if (strongSide == Side::white) {
        materialsignW &= ~EGTB_MATERIAL_SIGN_2_ELEPHANTS;
    } else {
        materialsignB &= ~EGTB_MATERIAL_SIGN_2_ELEPHANTS;
    }

    u64 mat = (u64)(materialsignW | (materialsignB << 16)) << 32 | fullMat;
    return mat;
}

/////////////////////////////////////////////////////////////////////////
std::pair<i64, bool> EgtbFile::getKey(const EgtbBoard& board) const {
    return getKey((const int*)board.pieceList);
}

std::pair<i64, bool> EgtbFile::getKey(const int* pieceList) const {
    EgtbKeyRec rec;
    EgtbKey::getKey(rec, pieceList, getEgtbType(), false, idxArr, idxMult, header->order);

    std::pair<i64, bool> r;
    r.first = rec.key;
    r.second = rec.flipSide;
    return r;
}
