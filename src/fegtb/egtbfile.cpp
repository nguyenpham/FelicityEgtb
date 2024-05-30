/**
 This file is part of Felicity Egtb, distributed under MIT license.

 * Copyright (c) 2024 Nguyen Hong Pham (github@nguyenpham)
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

#include <fstream>
#include <iomanip>
#include <ctime>

#include "egtb.h"
#include "egtbfile.h"
#include "egtbkey.h"

#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;


#ifdef _FELICITY_CHESS_
const char* EgtbFile::egtbFileExtensions[] = {
    ".fegtb",           /// .dtm / main extension
    ".fegtb_tmp",       /// .tmp / extension for temporary data
    nullptr
};
#else

const char* EgtbFile::egtbFileExtensions[] = {
    ".fexq",            /// .dtm / main extension
    ".fexq_tmp",        /// .tmp / extension for temporary data
    nullptr
};

#endif

//////////////////////////////////////////////////////////////////////
EgtbType EgtbFile::getExtensionType(const std::string& path) {
    auto p = EgtbType::none;

    for (int i = 0; egtbFileExtensions[i]; i++) {
        if (path.find(egtbFileExtensions[i]) != std::string::npos) {
            p = static_cast<EgtbType>(i);
            break;
        }
    }
    return p;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
EgtbFile::EgtbFile(const std::string& name, int order)
{
    pBuf[0] = pBuf[1] = pCompressBuf = nullptr;
    compressBlockTables[0] = compressBlockTables[1] = nullptr;

    header = nullptr;
    memMode = EgtbMemMode::tiny;
    loadStatus = EgtbLoadStatus::none;
    reset();
    
    if (!name.empty()) {
        setupIdxComputing(name, order);
    }
}

EgtbFile::~EgtbFile() {
    removeBuffers();
    
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
}

void EgtbFile::removeBuffers() {
    if (pCompressBuf) free(pCompressBuf);
    pCompressBuf = nullptr;

    for (auto i = 0; i < 2; i++) {
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

void EgtbFile::setPath(const std::string& s, Side side) {
    if (side != Side::white && side != Side::black) {
        auto ss = s;
        Funcs::toLower(ss);
        side = (ss.find(".w.") != std::string::npos) ? Side::white : Side::black;
    }
    path[static_cast<int>(side)] = s;
}

std::string EgtbFile::getPath(bslib::Side side) const
{
    return path[static_cast<int>(side)];
}

bool EgtbFile::createBuf(i64 len, Side side)
{
    auto sd = static_cast<int>(side);
    
    pBuf[sd] = (char *)malloc((size_t)len + 16);
    startpos[sd] = 0; endpos[sd] = 0;
    return pBuf[sd];
}

i64 EgtbFile::getBufItemCnt() const {
    if (memMode == EgtbMemMode::tiny) {
        return getCompressBlockSize();
    }
    return getSize();
}

//////////////////////////////////////////////////////////////////////
// Preload files by loading their headers
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
            assert(false);
            return false;
        }
        Funcs::toLower(theName);
        auto loadingSide = theName.find(".w") != std::string::npos ? Side::white : Side::black;
        setPath(path, loadingSide);

        theName = theName.substr(0, theName.length() - 2); // remove .w, .b
        egtbName = theName;
        egtbType = EgtbType::dtm;

        if (egtbName.empty()) {
            setupIdxComputing(getName(), 0);
        }
        return true;
    }

    auto r = loadHeaderAndTable(path);
    loadStatus = r ? EgtbLoadStatus::loaded : EgtbLoadStatus::error;
    return r;
}

/// Load all data if requested
bool EgtbFile::loadHeaderAndTable(const std::string& path) {
    assert(path.size() > 6);
    std::ifstream file(path, std::ios::binary);

    auto r = false;
    if (file.is_open()) {

        /// if there are files for both sides, header has been created already
        auto oldSide = Side::none;
        u32 oldProperty = 0;
        if (header == nullptr) {
            header = new EgtbFileHeader();
        } else {
            oldSide = header->isSide(Side::black) ? Side::black : Side::white;
            oldProperty = header->getProperty();
        }
        if (header) {
            oldSide = header->isSide(Side::black) ? Side::black : Side::white;
            oldProperty = header->getProperty();
        }
        auto loadingSide = Side::none;
        
        if (readHeader(file) && egtbName == header->getName()) {
            header->addProperty(oldProperty);
            loadingSide = path.find(".w.") != std::string::npos ? Side::white : Side::black;

            egtbType = EgtbType::dtm;

            setPath(path, loadingSide);
            //header->setOnlySide(loadingSide);
            assert(loadingSide == (path.find("w.") != std::string::npos ? Side::white : Side::black));
            r = loadingSide != Side::none;

            if (r) {
                header->addSide(loadingSide);

                setupIdxComputing(getName(), header->getOrder());

                auto sd = static_cast<int>(loadingSide);
                startpos[sd] = endpos[sd] = 0;
            }
        }

        if (r && isCompressed()) {
            if (!readCompressTable(file, loadingSide)) {
                if (egtbVerbose) {
                    std::cerr << "Error: cannot read compress table" << path << std::endl;
                }
                return false;
            }
        }

        if (r && memMode == EgtbMemMode::all) {
            r = loadAllData(file, loadingSide);

        }
        file.close();
    }

    if (egtbVerbose && !r) {
        std::cerr << "Error: cannot read " << path << std::endl;
    }

    return r;
}


void EgtbFile::merge(EgtbFile& otherEgtbFile)
{
    for(auto sd = 0; sd < 2; sd++) {
        auto side = static_cast<Side>(sd);

        if (header == nullptr) {
            auto path = otherEgtbFile.getPath(side);
            if (!path.empty()) {
                setPath(path, side);
            }
            continue;
        }

        if (otherEgtbFile.header && otherEgtbFile.header->isSide(side)) {

            header->addSide(side);
            setPath(otherEgtbFile.getPath(side), side);

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

bool EgtbFile::readCompressTable(std::ifstream& file, Side loadingSide) {
    auto sd = static_cast<int>(loadingSide);
    assert(compressBlockTables[sd] == nullptr);
    
    auto blockTableSz = getBlockTableSize(loadingSide);
    compressBlockTables[sd] = (u8*)malloc(blockTableSz + 64);
    
    i64 seekpos = header->headerSize();
    file.seekg(seekpos, std::ios::beg);

    if (!file.read((char *)compressBlockTables[sd], blockTableSz)) {
        if (egtbVerbose) {
            std::cerr << "Error: cannot read compress table from path " << path << std::endl;
        }
        file.close();
        free(compressBlockTables[sd]);
        compressBlockTables[sd] = nullptr;
        return false;
    }
    return true;
}

bool EgtbFile::loadAllData(std::ifstream& file, Side side) {

    assert(file.is_open());
    auto sd = static_cast<int>(side);

    startpos[sd] = endpos[sd] = 0;

    auto sz = getSize();
    auto bufSz = sz;
    if (isTwoBytes()) bufSz += bufSz;
    createBuf(bufSz, side); assert(pBuf[sd]);

    if (isCompressed()) {
        const auto compressBlockSz = getCompressBlockSize();
        auto blockCnt = (int)((bufSz + compressBlockSz - 1) / compressBlockSz);
        auto blockTableItemSize = (header->getProperty() & (EGTB_PROP_LARGE_COMPRESSTABLE_B << sd)) != 0 ? 5 : 4;
        auto blockTableSz = blockCnt * blockTableItemSize;
        
        i64 compDataSz;

        assert(compressBlockTables[sd]);
        u8 *p = compressBlockTables[sd] + blockTableItemSize * (blockCnt - 1);
        if (blockTableItemSize == 4) {
            u32 sz = *(u32*)p;
            sz &= u32(EGTB_SMALL_COMPRESS_SIZE);
            compDataSz = sz;
        } else {
            compDataSz = *(i64*)p & EGTB_LARGE_COMPRESS_SIZE;
        }
        assert(compDataSz > 0 && compDataSz <= getSize());

        char* tempBuf = (char*) malloc(compDataSz + 64);
        
        assert(blockTableSz == getBlockTableSize(side));
        i64 seekpos = header->headerSize() + blockTableSz;
        file.seekg(seekpos, std::ios::beg);

        if (file.read(tempBuf, compDataSz)) {
            auto originSz = decompressAllBlocks(compressBlockSz, blockCnt, (u32*)compressBlockTables[sd], (char*)pBuf[sd], bufSz, tempBuf, compDataSz);
            assert(originSz == bufSz);

            endpos[sd] = sz;
        }

        free(tempBuf);
        free(compressBlockTables[sd]);
        compressBlockTables[sd] = nullptr;
    } else {
        i64 seekpos = header->headerSize();
        file.seekg(seekpos, std::ios::beg);

        if (file.read(pBuf[sd], bufSz)) {
            endpos[sd] = sz;
        }
    }

    return startpos[sd] < endpos[sd];
}


void EgtbFile::checkToLoadHeaderAndTables(Side side) {
    auto sd = static_cast<int>(side);

    if (loadStatus != EgtbLoadStatus::none || (sd < 2 && path[sd].empty())) {
        return;
    }

    std::lock_guard<std::mutex> thelock(mtx);
    
    if (loadStatus != EgtbLoadStatus::none || (sd < 2 && path[sd].empty())) {
        return;
    }

    auto r = true;
    if (sd < 2) {
        assert(!path[sd].empty());
        r = loadHeaderAndTable(path[sd]);
    } else {
        if (path[0].empty() || path[1].empty()) {
            std::cerr << "Error: missing a path for " << getName() << std::endl;
        }
        if (r && !path[1].empty()) {
            r = loadHeaderAndTable(path[1]);
        }
        if (!path[0].empty()) {
            r = loadHeaderAndTable(path[0]);
        }
    }

    loadStatus = r ? EgtbLoadStatus::loaded : EgtbLoadStatus::error;
}

bool EgtbFile::forceLoadHeaderAndTable(Side side) {
    std::lock_guard<std::mutex> thelock(mtx);

    auto sd = static_cast<int>(side);
    bool r = !path[sd].empty() && loadHeaderAndTable(path[sd]);
    loadStatus = r ? EgtbLoadStatus::loaded : EgtbLoadStatus::error;
    return r;
}

bool EgtbFile::readBuf(i64 idx, Side side)
{
    auto sd = static_cast<int>(side);
    if (!pBuf[sd]) {
        auto bufSz = getBufSize();
        createBuf(bufSz, side);
    }

    auto r = false;
    std::ifstream file(getPath(side), std::ios::binary);
    if (file) {
        if (memMode == EgtbMemMode::all) {
            r = loadAllData(file, side);
        } else if (isCompressed() && compressBlockTables[sd]) {
            r = readCompressedBlock(file, idx, side, (char*)pBuf[sd]);
        } else {
            auto bufCnt = std::min<i64>(getBufItemCnt(), getSize() - idx);

            auto beginIdx = (idx + bufCnt <= getSize()) ? idx : 0;
            auto x = beginIdx;
            
            auto bufsz = bufCnt;
            if (isTwoBytes()) {
                bufsz += bufsz;
                x += beginIdx;
            }

            i64 seekpos = header->headerSize() + x;
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
        std::cerr << "Error: Cannot open file " << getPath(side) << std::endl;
    }

    return r;
}

bool EgtbFile::readCompressedBlock(std::ifstream& file, i64 idx, Side side, char* pDest)
{
    auto sd = static_cast<int>(side);
    auto blockCnt = getCompresseBlockCount();
    int blockTableSz = blockCnt * sizeof(u32);

    const int compressBlockSz = getCompressBlockSize();
    const int blockSize = isTwoBytes() ? compressBlockSz / 2 : compressBlockSz;
    auto blockIdx = idx / blockSize;
    startpos[sd] = endpos[sd] = blockIdx * blockSize;

    u32* table = (u32*)compressBlockTables[sd];
    assert(table);
    
    auto iscompressed = !(table[blockIdx] & EGTB_UNCOMPRESS_BIT);
    u32 blockOffset = blockIdx == 0 ? 0 : (table[blockIdx - 1] & u32(EGTB_SMALL_COMPRESS_SIZE));
    u32 compDataSz = (table[blockIdx] & u32(EGTB_SMALL_COMPRESS_SIZE)) - blockOffset;

    assert(compDataSz <= blockSize);

    i64 seekpos = header->headerSize() + blockTableSz + blockOffset;
    file.seekg(seekpos, std::ios::beg);

    if (iscompressed) {
        if (pCompressBuf == nullptr) {
            pCompressBuf = (char*) malloc(blockSize * 3 / 2);
        }
        if (file.read(pCompressBuf, compDataSz)) {
            auto m = getSize() - startpos[sd];
            if (isTwoBytes()) m += m;
            auto curBlockSize = (int)std::min<i64>(m, (i64)compressBlockSz);
            auto originSz = decompress(pDest, curBlockSize, pCompressBuf, compDataSz);
            endpos[sd] += isTwoBytes() ? originSz / 2 : originSz;
            assert(originSz <= getBufSize());
            return true;
        }
    } else if (file.read(pDest, compDataSz)) {
        endpos[sd] += isTwoBytes() ? compDataSz / 2 : compDataSz;
        return true;
    }

    if (egtbVerbose) {
        std::cerr << "Error: cannot read " << getPath(side) << std::endl;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// Get scores
//////////////////////////////////////////////////////////////////////

char EgtbFile::getCell(i64 idx, Side side)
{
    if (idx >= getSize() ||
        (!isDataReady(idx, side) && !readBuf(idx, side))) {
        return TB_MISSING;
    }

    auto sd = static_cast<int>(side);
    char ch = pBuf[sd][idx - startpos[sd]];
    return ch;
}

int EgtbFile::getScore(const EgtbBoard& board, Side side, bool useLock)
{
    auto r = getKey(board);
    if (r.flipSide) {
        side = getXSide(side);
    }
    return getScore(r.key, side, useLock);
}

int EgtbFile::getScoreNoLock(const EgtbBoard& board, Side side) {
    auto r = getKey(board);
    if (r.flipSide) {
        side = getXSide(side);
    }
    return getScoreNoLock(r.key, side);

}

int EgtbFile::getScoreNoLock(i64 idx, Side side)
{
    if (isTwoBytes()) {
        if (idx >= getSize()) {
            return TB_UNSET;
        }
        
        int sd = static_cast<int>(side);
        
        if (!isDataReady(idx, side)) {
            if (!readBuf(idx, side)) {
                return TB_UNSET;
            }
        }
        
        assert(idx >= startpos[sd] && idx < endpos[sd]);
        const i16 * p = (const i16 * )pBuf[sd];
        i16 score = p[idx - startpos[sd]];
        return score;
    }
    
    char cell = getCell(idx, side);
    return cellToScore(cell);
}

int EgtbFile::getScore(i64 idx, Side side, bool useLock)
{
    checkToLoadHeaderAndTables(Side::none); // load all sides
    if (loadStatus == EgtbLoadStatus::error) {
        return EGTB_SCORE_MISSING;
    }
    assert(isValidHeader());
    if (useLock && !isDataReady(idx, side)) {
        std::lock_guard<std::mutex> thelock(sdmtx[static_cast<int>(side)]);
        return getScoreNoLock(idx, side);
    }
    return getScoreNoLock(idx, side);
}


////////////////////////////////////////////////////////////////////

i64 EgtbFile::setupIdxComputing(const std::string& name, int order)
{
    egtbName = name;
    size = parseAttr(name, egtbIdxArray, (int*)pieceCount, order);

    attackerCount = 0;
#ifdef _FELICITY_CHESS_
    auto from = QUEEN;
#else
    auto from = ROOK;
#endif
    
    for(auto t = from; t <= PAWN; t++) {
        attackerCount += pieceCount[0][t] + pieceCount[1][t];
    }
    return size;
}

i64 EgtbFile::computeSize(const std::string &name)
{
    EgtbIdxRecord egtbIdxRecord[16];
    return parseAttr(name, egtbIdxRecord, nullptr, 0);
}


/////////////////////////////////////////////////////////////////////////
EgtbKeyRec EgtbFile::getKey(const EgtbBoard& board) const
{
    return EgtbKey::getKey(board, egtbIdxArray, 0);
}

int EgtbFile::cellToScore(char cell) {
    assert(!isTwoBytes());
    return _cellToScore(cell);
}


int EgtbFile::_cellToScore(char cell) {
    u8 s = (u8)cell;
    if (s >= TB_DRAW) {
        if (s == TB_DRAW) {
            return EGTB_SCORE_DRAW;
        }

        /// winning score, score should be positive 1 -> 1000
        if (s < TB_START_LOSING) {
            auto mi = int(s - TB_START_MATING) * 2 + 1;
            auto k = EGTB_SCORE_MATE - mi;
            assert(k > TB_DRAW && k <= EGTB_SCORE_MATE);
            return k;
        }

        /// lossing, score should be negative -1000 -> -870
        auto mi = int(s - TB_START_LOSING) * 2;
        auto k = -EGTB_SCORE_MATE + mi;
        assert(k >= -EGTB_SCORE_MATE && k <= -EGTB_SCORE_MATE + TB_START_LOSING);
        return k;
    }

    switch (s) {
        case TB_MISSING:
            return EGTB_SCORE_MISSING;
        case TB_UNKNOWN:
            return EGTB_SCORE_UNKNOWN;
        case TB_ILLEGAL:
            return EGTB_SCORE_ILLEGAL;
        case TB_UNSET:
            return EGTB_SCORE_UNSET;
        default:
            assert(false);
            return EGTB_SCORE_ILLEGAL;
    }
}

const std::vector<int> defaultOrderVec { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

std::vector<int> EgtbFile::order2Vec(int order)
{
    std::vector<int> vec;
    if (order) {
        vec = { order & 0x7, (order >> 3) & 0x7, (order >> 6) & 0x7, (order >> 9) & 0x7 , (order >> 12) & 0x7, (order >> 15) & 0x7 };
    } else {
        vec = defaultOrderVec;
    }

    return vec;
}

int EgtbFile::verifyAKey(EgtbBoard& board, i64 idx) const
{
    /// It is considered OK if it cann't setup the board
    if (!setupBoard(board, idx, FlipMode::none, Side::white)) {
        return 0;
    }
    
    auto bit = Verify_bit_setupOK;
    if (board.isValid() && getKey(board).key == idx) {
        bit |= Verify_bit_valid;
    }
    
    return bit;
}

/**
 Simple function to test the consistancy and correctness of keys, using only one (main) thread
  For faster, using multi threads, use functions in generator code
 */
bool EgtbFile::verifyKeys(bool printRandom) const
{
    EgtbBoard board;
    
    auto sz = getSize();
    std::cout << "Verifying Keys for " << getName() << ", size: " << sz << std::endl;

    int64_t cnt = 0;
    
    for (int64_t idx = 0; idx < sz; ++idx) {
        auto ok = verifyAKey(board, idx);

        if (ok) {
            cnt++;
        }
        
        auto prt = !ok || (printRandom && rand() % 500000 == 0);
        if (prt) {
            auto msg = std::string("idx: ") + std::to_string(idx);
            board.printOut(msg);
        }
        if (!ok) {
            std::cout << "The board is invalid or not matched keys, idx = " << idx << std::endl;
            return false;
        }
    }
    
    std::cout << " passed. Valid keys: " << cnt << std::endl;

    return true;
}



