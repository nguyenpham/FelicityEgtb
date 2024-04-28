/*
 This file is part of NhatMinh Egtb, distributed under MIT license.

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
#include "EgtbFile.h"
#include "EgtbKey.h"

using namespace egtb;

extern int subppp_sizes[7];
extern int *kk_2, *kk_8;

//////////////////////////////////////////////////////////////////////

#define TB_ILLEGAL              0
#define TB_UNSET                1
#define TB_MISSING              2
#define TB_WINING               3
#define TB_UNKNOWN              4
#define TB_DRAW                 5

#define TB_START_MATING         (TB_DRAW + 1)
#define TB_START_LOSING         130

#define TB_SPECIAL_DRAW         0
#define TB_SPECIAL_START_MATING (TB_SPECIAL_DRAW + 1)
#define TB_SPECIAL_START_LOSING 128

//////////////////////////////////////////////////////////////////////

const char* egtbFileExtensions[] = {
    ".mtb", ".zmt", nullptr
};

bool EgtbFile::knownExtension(const std::string& path) {

    for (int i = 0; egtbFileExtensions[i]; i++) {
        if (path.find(egtbFileExtensions[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
EgtbFile::EgtbFile() {
    pBuf[0] = pBuf[1] = pCompressBuf = nullptr;
    compressBlockTables[0] = compressBlockTables[1] = nullptr;
    header = nullptr;
    memMode = EgtbMemMode::tiny;
    loadStatus = EgtbLoadStatus::none;
    reset();
}

EgtbFile::~EgtbFile() {
    removeBuffers();
    if (header) {
        delete header;
        header = nullptr;
    }
}

void EgtbFile::reset() {
    if (header != nullptr) {
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

        startpos[i] = endpos[i] = 0;
    }
    loadStatus = EgtbLoadStatus::none;
}

//////////////////////////////////////////////////////////////////////
void EgtbFile::merge(EgtbFile& otherEgtbFile)
{
    for(int sd = 0; sd < 2; sd++) {
        auto side = static_cast<bslib::Side>(sd);
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
        sd = (ss.find("w.") != std::string::npos) ? bslib::W : bslib::B;
    }
    path[sd] = s;
}

bool EgtbFile::createBuf(i64 len, int sd) {
    pBuf[sd] = (char *)malloc(len + 16);
    startpos[sd] = 0; endpos[sd] = 0;
    return pBuf[sd];
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
        int loadingSd = theName.find("w") != std::string::npos ? bslib::W : bslib::B;
        setPath(path, loadingSd);

        theName = theName.substr(0, theName.length() - 1); // remove W / B
        egtbName = theName;

        setupIdxComputing(getName(), 0, 3);
        return true;
    }

    bool r = loadHeaderAndTable(path);
    loadStatus = r ? EgtbLoadStatus::loaded : EgtbLoadStatus::error;
    return r;
}

// Load all data too if requested
bool EgtbFile::loadHeaderAndTable(const std::string& path) {
    assert(path.size() > 8);
    std::ifstream file(path, std::ios::binary);

    // if there are files for both sides, header has been created already
    auto oldSide = bslib::Side::none;
    if (header == nullptr) {
        header = new EgtbFileHeader();
    } else {
        oldSide = header->isSide(bslib::Side::black) ? bslib::Side::black : bslib::Side::white;
    }
    auto loadingSide = bslib::Side::none;

    bool r = false;
    if (file && header->readFile(file) && header->isValid()) {
        loadingSide = header->isSide(bslib::Side::white) ? bslib::Side::white : bslib::Side::black;
        assert(loadingSide == (path.find("w.") != std::string::npos ? bslib::Side::white : bslib::Side::black));
        egtbName = header->name;

        setPath(path, static_cast<int>(loadingSide));
        header->setOnlySide(loadingSide);
        assert(loadingSide == (path.find("w.") != std::string::npos ? bslib::Side::white : bslib::Side::black));
        r = loadingSide != bslib::Side::none;

        if (r) {
            setupIdxComputing(getName(), header->order, header->getVersion());

            if (oldSide != bslib::Side::none) {
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

bool EgtbFile::loadAllData(std::ifstream& file, bslib::Side side) {

    auto sd = static_cast<int>(side);
    startpos[sd] = endpos[sd] = 0;

    if (isCompressed()) {
        auto blockCnt = getCompresseBlockCount();
        int blockTableSz = blockCnt * sizeof(u32);

        i64 seekpos = EGTB_HEADER_SIZE + blockTableSz;
        file.seekg(seekpos, std::ios::beg);

        createBuf(getSize(), sd); assert(pBuf[sd]);

        auto compDataSz = compressBlockTables[sd][blockCnt - 1] & ~EGTB_UNCOMPRESS_BIT;

        char* tempBuf = (char*) malloc(compDataSz + 64);
        if (file.read(tempBuf, compDataSz)) {
            auto originSz = decompressAllBlocks(EGTB_SIZE_COMPRESS_BLOCK, blockCnt, compressBlockTables[sd], (char*)pBuf[sd], getSize(), tempBuf, compDataSz);
            assert(originSz == getSize());

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

void EgtbFile::checkToLoadHeaderAndTable() {
    if (header != nullptr && loadStatus != EgtbLoadStatus::none) {
        return;
    }

    std::lock_guard<std::mutex> thelock(mtx);
    if (header != nullptr && loadStatus != EgtbLoadStatus::none) {
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

    auto bufCnt = MIN(getBufItemCnt(), getSize() - idx);
    auto bufsz = bufCnt;

    bool r = false;
    std::ifstream file(getPath(sd), std::ios::binary);
    if (file) {
        if (memMode == EgtbMemMode::all) {
            auto side = static_cast<bslib::Side>(sd);
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
        std::cerr << "Error: cannot read " << getPath(sd) << std::endl;
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
            pCompressBuf = (char*) malloc(EGTB_SIZE_COMPRESS_BLOCK * 3 / 2);
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
int EgtbFile::cellToScore(char cell) {
    if (header->property & EGTB_PROP_SPECIAL_SCORE_RANGE) {
        u8 s = (u8)cell;
        if (s >= TB_SPECIAL_DRAW) {
            if (s == TB_SPECIAL_DRAW) return EGTB_SCORE_DRAW;
            if (s < TB_SPECIAL_START_LOSING) {
                int mi = (s - TB_SPECIAL_START_MATING) * 2 + 1;
                return EGTB_SCORE_MATE - mi;
            }

            int mi = (s - TB_SPECIAL_START_LOSING) * 2;
            return -EGTB_SCORE_MATE + mi;
        }
        return EGTB_SCORE_DRAW;
    }

    u8 s = (u8)cell;
    if (s >= TB_DRAW) {
        if (s == TB_DRAW) return EGTB_SCORE_DRAW;
        if (s < TB_START_LOSING) {
            int mi = (s - TB_START_MATING) * 2 + 1;
            return EGTB_SCORE_MATE - mi;
        }

        int mi = (s - TB_START_LOSING) * 2;
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
            return EGTB_SCORE_UNSET;
    }
}

char EgtbFile::getCell(i64 idx, bslib::Side side)
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

char EgtbFile::getCell(const bslib::BoardCore& board, bslib::Side side) {
    i64 key = getKey(board).key;
    return getCell(key, side);
}

int EgtbFile::getScoreNoLock(const bslib::BoardCore& board, bslib::Side side) {
    EgtbKeyRec r = getKey(board);
    if (r.flipSide) {
        side = getXSide(side);
    }
    return getScoreNoLock(r.key, side);
}

int EgtbFile::getScoreNoLock(i64 idx, bslib::Side side)
{
    char cell = getCell(idx, side);
    return cellToScore(cell);
}

int EgtbFile::getScore(const bslib::BoardCore& board, bslib::Side side, bool useLock) {
    EgtbKeyRec r = getKey(board);
    if (r.flipSide) {
        side = getXSide(side);
    }

    return getScore(r.key, side, useLock);
}

int EgtbFile::getScore(i64 idx, bslib::Side side, bool useLock)
{
    checkToLoadHeaderAndTable();

    if (useLock && (memMode != EgtbMemMode::all || !isDataReady(idx, static_cast<int>(side)))) {
        std::lock_guard<std::mutex> thelock(sdmtx[static_cast<int>(side)]);
        return getScoreNoLock(idx, side);
    }
    return getScoreNoLock(idx, side);
}

//////////////////////////////////////////////////////////////////////
// Parse name
//////////////////////////////////////////////////////////////////////


i64 EgtbFile::parseAttr(const std::string& name, int* idxArr, i64* idxMult, int* pieceCount, u16 order, int version)
{
    auto havingPawns = name.find("p") != std::string::npos;

    int k = 0;
    int pawnCnt[] = { 0, 0};

    for (int i = 0, sd = bslib::W; i < (int)name.size(); i++) {
        char ch = name[i];
        if (ch == 'k') { // king
            if (i == 0) {
                idxArr[k++] = (havingPawns ? EGTB_IDX_KK_2 : EGTB_IDX_KK_8) | (bslib::W << 8);
                assert(idxArr[k - 1] >> 8 == sd);
            } else {
                sd = bslib::B;
            }
            continue;
        }
        if (ch == 'p') {
            pawnCnt[sd]++;
        }

//        const char* p = strchr(pieceTypeName, ch);
//        int t = (int)(p - pieceTypeName);
        auto p = bslib::ChessBoard::pieceTypeName.find(ch); assert(p != std::string::npos);
        auto t = int(p);

        t += EGTB_IDX_Q - 1;

        for (int j = 0;; j++) {
            if (ch != name[i + 1]) {
                break;
            }
            i++;
            t += 5;
        }
        idxArr[k++] = t | (sd << 8);
    }

    bool enpassantable = pawnCnt[0] > 0 && pawnCnt[1] > 0;
    idxArr[k] = EGTB_IDX_NONE;

    // permutation
    if (order != 0) {
        const int orderArray[] = { order & 0x7, (order >> 3) & 0x7, (order >> 6) & 0x7, (order >> 9) & 0x7 , (order >> 12) & 0x7, (order >> 15) & 0x7 };
        int tmpArr[10];
        memcpy(tmpArr, idxArr, k * sizeof(int));
        for(int i = 0; i < k; i++) {
            int x = orderArray[i];
            idxArr[x] = tmpArr[i];
        }
        return parseAttr(idxArr, idxMult, pieceCount, (int*)orderArray, enpassantable);
    }

    return parseAttr(idxArr, idxMult, pieceCount, nullptr, enpassantable);
}

i64 EgtbFile::parseAttr(const int* idxArr, i64* idxMult, int* pieceCount, const int* orderArray, bool enpassantable) {
    memset(pieceCount, 0, 2 * 7 * sizeof(int));

    pieceCount[static_cast<int>(bslib::PieceType::king)] = 1;
    pieceCount[6 + static_cast<int>(bslib::PieceType::king)] = 1;

    i64 sz = 1;
    int n = 0;
    for (int i = 0; ; i++, n++) {
        auto a = idxArr[i];
        if (a == EGTB_IDX_NONE) {
            break;
        }

        i64 h = 0;
        int sd = a >> 8;
        int d = sd == bslib::W ? 6 : 0;

        a &= 0xff;
        switch (a) {
            case EGTB_IDX_K_2:
                h = EGTB_SIZE_K2;
                break;
            case EGTB_IDX_K_8:
                h = EGTB_SIZE_K8;
                break;
            case EGTB_IDX_K:
                h = EGTB_SIZE_K;
                break;

            case EGTB_IDX_KK_2:
                h = EGTB_SIZE_KK2;
                break;
            case EGTB_IDX_KK_8:
                h = EGTB_SIZE_KK8;
                break;

            case EGTB_IDX_Q:
            case EGTB_IDX_R:
            case EGTB_IDX_B:
            case EGTB_IDX_H:
            case EGTB_IDX_P:
            {
                h = a != EGTB_IDX_P ? EGTB_SIZE_X : EGTB_SIZE_P;
                int type = 1 + a - EGTB_IDX_Q;
                pieceCount[d + type] = 1;
                break;
            }

            case EGTB_IDX_QQ:
            case EGTB_IDX_RR:
            case EGTB_IDX_BB:
            case EGTB_IDX_HH:
            case EGTB_IDX_PP:
            {
                h = a != EGTB_IDX_PP ? EGTB_SIZE_XX : EGTB_SIZE_PP;
                int type = 1 + a - EGTB_IDX_QQ;
                pieceCount[d + type] = 2;
                break;
            }

            case EGTB_IDX_QQQ:
            case EGTB_IDX_RRR:
            case EGTB_IDX_BBB:
            case EGTB_IDX_HHH:
            case EGTB_IDX_PPP:
            {
                h = a != EGTB_IDX_PPP ? EGTB_SIZE_XXX : EGTB_SIZE_PPP;
                int type = 1 + a - EGTB_IDX_QQQ;
                pieceCount[d + type] = 3;
                break;
            }

            case EGTB_IDX_QQQQ:
            case EGTB_IDX_RRRR:
            case EGTB_IDX_BBBB:
            case EGTB_IDX_HHHH:
            case EGTB_IDX_PPPP:
            {
                h = a != EGTB_IDX_PPPP ? EGTB_SIZE_XXXX : EGTB_SIZE_PPPP;
                int type = 1 + a - EGTB_IDX_QQQQ;
                pieceCount[d + type] = 4;
                break;
            }

            default:
                assert(false);
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

i64 EgtbFile::setupIdxComputing(const std::string& name, int order, int version)
{
    size = EgtbFile::parseAttr(name.c_str(), idxArr, idxMult, (int*)pieceCount, order, version);
    enpassantable = pieceCount[0][static_cast<int>(bslib::PieceType::pawn)] > 0 && pieceCount[1][static_cast<int>(bslib::PieceType::pawn)] > 0;
    return size;
}

i64 EgtbFile::computeSize(const std::string &name)
{
    int idxArr[32];
    i64 idxMult[32];
    int pieceCount[2][7];
    return parseAttr(name, idxArr, idxMult, (int*)pieceCount, 0, 3);
}

std::string EgtbFile::pieceListToName(const bslib::Piece* pieceList) {
    int pieceCnt[2][6];
    memset(pieceCnt, 0, sizeof(pieceCnt));

    for(int sd = 0, d = 0; sd < 2; sd++, d = 16) {
        for (int i = 0; i < 16; i++) {
            if (!pieceList[d + i].isEmpty()) {
                pieceCnt[sd][static_cast<int>(pieceList[d + i].type)]++;
            }
        }
    }

    std::string s;
    for(int sd = 1; sd >= 0; sd--) {
        for (int type = 0; type < 6; type++) {
            auto n = pieceCnt[sd][type];
            if (n) {
                char ch = bslib::ChessBoard::pieceTypeName[type];
                for(int j = 0; j < n; j++) {
                    s += ch;
                }
            }
        }
    }

    return s;
}

EgtbKeyRec EgtbFile::getKey(const bslib::BoardCore& board) const {
    EgtbKeyRec rec;
    EgtbKey::getKey(rec, board, idxArr, idxMult, header ? header->order : 0);
    return rec;
}

extern const int tb_kIdxToPos[10];

bool EgtbFile::setupBoard(bslib::BoardCore& _board, i64 idx, bslib::FlipMode flip, bslib::Side firstsider) const
{
    auto board = static_cast<bslib::ChessBoard>(_board);
    board.enpassant = -1;
    board._status = 0;
    board.castleRights[0] = board.castleRights[1] = 0;

    int order = header ? header->order : 0;
    if (!order) {
        order = 0 | 1 << 3 | 2 << 6 | 3 << 9 | 4 << 12 | 5 << 15;
    }
    const int orderArray[] = { order & 0x7, (order >> 3) & 0x7, (order >> 6) & 0x7, (order >> 9) & 0x7 , (order >> 12) & 0x7, (order >> 15) & 0x7 };

    bslib::BoardCore::pieceList_reset(board.pieceList);

    i64 rest = idx;

    int sds[20] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    for(int i = 0, sd = static_cast<int>(firstsider), stdSd = bslib::W; idxArr[i] != EGTB_IDX_NONE; i++) {
        int j = orderArray[i];
        if (idxArr[j] >> 8 != stdSd) {
            sd = 1 - sd;
            stdSd = 1 - stdSd;
        }
        sds[j] = sd;
    }

    for(int i = 0; idxArr[i] != EGTB_IDX_NONE; i++) {
        auto arr = idxArr[i] & 0xff;
        auto mul = idxMult[i];

        auto key = (int)(rest / mul);
        rest = rest % mul;

        int sd = sds[i];
        auto side = static_cast<bslib::Side>(sd);

        switch (arr) {
            case EGTB_IDX_K_2:
            {
                assert(key >= 0 && key < 32);
                int r = key >> 2, f = key & 0x3;
                auto pos = (r << 3) + f;
                board.pieceList[sd][0].type = bslib::PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = pos;
                break;
            }

            case EGTB_IDX_K_8:
            {
                auto pos = tb_kIdxToPos[key];
                board.pieceList[sd][0].type = bslib::PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = pos;
                break;
            }
            case EGTB_IDX_K:
            {
                board.pieceList[sd][0].type = bslib::PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = key;
                break;
            }

            case EGTB_IDX_KK_2:
            {
                int kk = kk_2[key];
                int k0 = kk >> 8, k1 = kk & 0xff;
                board.pieceList[sd][0].type = bslib::PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = k0;

                board.pieceList[1 - sd][0].type = bslib::PieceType::king;
                board.pieceList[1 - sd][0].side = getXSide(side);
                board.pieceList[1 - sd][0].idx = k1;
                break;
            }

            case EGTB_IDX_KK_8:
            {
                auto kk = kk_8[key];
                auto k0 = kk >> 8, k1 = kk & 0xff;
                board.pieceList[sd][0].type = bslib::PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = k0;

                board.pieceList[1 - sd][0].type = bslib::PieceType::king;
                board.pieceList[1 - sd][0].side = getXSide(side);
                board.pieceList[1 - sd][0].idx = k1;
                break;
            }

            case EGTB_IDX_Q:
            case EGTB_IDX_R:
            case EGTB_IDX_B:
            case EGTB_IDX_H:
            case EGTB_IDX_P:
            {
                auto type = static_cast<bslib::PieceType>(arr - EGTB_IDX_Q + 1);
                if (!egtbKey.setupBoard_x(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_QQ:
            case EGTB_IDX_RR:
            case EGTB_IDX_BB:
            case EGTB_IDX_HH:
            case EGTB_IDX_PP:
            {
                auto type = static_cast<bslib::PieceType>(arr - EGTB_IDX_QQ + 1);
                if (!egtbKey.setupBoard_xx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_QQQ:
            case EGTB_IDX_RRR:
            case EGTB_IDX_BBB:
            case EGTB_IDX_HHH:
            case EGTB_IDX_PPP:
            {
                auto type = static_cast<bslib::PieceType>(arr - EGTB_IDX_QQQ + 1);
                if (!egtbKey.setupBoard_xxx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_QQQQ:
            case EGTB_IDX_RRRR:
            case EGTB_IDX_BBBB:
            case EGTB_IDX_HHHH:
            case EGTB_IDX_PPPP:
            {
                auto type = static_cast<bslib::PieceType>(arr - EGTB_IDX_QQQQ + 1);
                if (!egtbKey.setupBoard_xxxx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            default:
                assert(false);
                break;
        }
    }

    return board.pieceList_setupBoard();
}

