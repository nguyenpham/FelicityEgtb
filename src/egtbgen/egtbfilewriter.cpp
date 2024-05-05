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

#include <thread>

#include "../egtb/egtb.h"
#include "../base/funcs.h"

#include "egtbfilewriter.h"
#include "compresslib.h"
#include "genlib.h"

std::mutex printMutex;

using namespace fegtb;
using namespace bslib;

extern bool twoBytes;
extern const int pieceValForOrdering[7];

#define COPYRIGHT       "Copyright 2024 by Felicity EGTB"

static const int TmpHeaderSize = 16;
static const i16 TmpHeaderSign = 2345;
static const i16 TmpHeaderSign2 = 2346;

struct TmpHeader {
    i16 sign;
    i16 loop;
    u32 checksum;
    char resert[16];
};


EgtbFileWriter::~EgtbFileWriter()
{
    removeFlagBuffer();
}

void EgtbFileWriter::removeBuffers()
{
    EgtbFile::removeBuffers();
    removeFlagBuffer();
}

void EgtbFileWriter::setName(const std::string& s)
{
    header->setName(s);
}


void EgtbFileWriter::addProperty(uint addprt) {
    header->addProperty(addprt);
}

void EgtbFileWriter::create(const std::string& name, EgtbType _egtbType, u32 order) {
    if (header == nullptr) {
        header = new EgtbFileHeader();
    }
    header->reset();

    egtbType = _egtbType;

    loadStatus = EgtbLoadStatus::loaded;

    header->setOrder(order);
    
    if (twoBytes) {
        header->addProperty(EGTB_PROP_2BYTES);
        assert(isTwoBytes());
    }

    setName(name);

    i64 sz = setupIdxComputing(name, order); //, getVersion());
    setSize(sz); assert(sz);
}

int EgtbFileWriter::getVersion() const {
    auto signature = header ? header->getSignature() : 0;
    switch (signature) {
        case TB_ID_MAIN_V1:
            return 0;
        case EGTB_ID_MAIN_V2:
            return 1;
        case EGTB_ID_MAIN_V3:
            return 2;
        case EGTB_ID_MAIN:
            return 3;
    }
    assert(false);
    return -1;
}

bool EgtbFileWriter::saveHeader(std::ofstream& outfile) const {
    //            outfile.seekg(0LL, std::ios::beg);
    outfile.write (header->getData(), header->headerSize());
    return true;
}

void EgtbFileWriter::fillBufs(int score)
{
    char cell = scoreToCell(score, getVersion() > 0);
    for(auto sd = 0; sd < 2; sd++) {
        if (pBuf[sd])
            std::memset(pBuf[sd], cell, size);
    }
}

bool EgtbFileWriter::setBufScore(i64 idx, int score, int sd)
{
    if (isTwoBytes()) {
        return setBuf2Bytes(idx, score, sd);
    }

    char cell = scoreToCell(score, getVersion() > 0);
    return setBuf(idx, cell, sd);
}


int EgtbFileWriter::cellToScore(char cell) {
    assert(!isTwoBytes());
    return cellToScore(cell, getVersion() > 0);
}


char EgtbFileWriter::scoreToCell(int score, bool ver2) {
    if (ver2) {
        if (score <= EGTB_SCORE_MATE) {
            if (score == EGTB_SCORE_DRAW) return TB_DRAW;
            
            auto mi = (EGTB_SCORE_MATE - abs(score)) / 2;

            int k = mi + (score > 0 ? EGTB_START_MATING : EGTB_START_LOSING);
            assert(k >= 0 && k < 255);
            
            if (mi > 125 || k < 0 || k >= 255 || (score > 0 && k >= (EGTB_START_LOSING - 2))) {
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cerr << "FATAL ERROR: overflown score: " << score << ". Please use 2 bytes per item (param: -2)." << std::endl;
                exit(-1);
            }

            return (char)k;
        }

        switch (score) {
            case EGTB_SCORE_MISSING:
                return TB_MISSING;

            case EGTB_SCORE_WINNING:
                return TB_WINING;

            case EGTB_SCORE_UNKNOWN:
                return TB_UNKNOWN;

            case EGTB_SCORE_ILLEGAL:
                return TB_ILLEGAL;
                
            case EGTB_SCORE_PERPETUAL_CHECKED:
                return (char)TB_PERPETUAL_CHECKED;
            case EGTB_SCORE_PERPETUAL_CHECKED_EVASION:
                return (char)TB_PERPETUAL_CHECKED_EVASION;
            case EGTB_SCORE_PERPETUAL_EVASION:
                return (char)TB_PERPETUAL_EVASION;

            case EGTB_SCORE_UNSET:
                return TB_UNSET;
            default:
                assert(false);
                return TB_UNSET;
        }
    }
    
    std::cerr << "Fatal error: using old version endgames." << std::endl;
    exit(-1);

    assert(false);
    return 0;
}

int EgtbFileWriter::cellToScore(char cell, bool ver2) {
    if (ver2) {
        u8 s = (u8)cell;
        if (s >= TB_DRAW) {
            // Researching
            switch (s) {
                case TB_DRAW:
                    return EGTB_SCORE_DRAW;
                case TB_PERPETUAL_CHECKED:
                    return EGTB_SCORE_PERPETUAL_CHECKED;
                case TB_PERPETUAL_CHECKED_EVASION:
                    return EGTB_SCORE_PERPETUAL_CHECKED_EVASION;
                case TB_PERPETUAL_EVASION:
                    return EGTB_SCORE_PERPETUAL_EVASION;

                default:
                    break;
            }
            
            if (s < EGTB_START_LOSING) {
                int mi = (s - EGTB_START_MATING) * 2 + 1;
                return EGTB_SCORE_MATE - mi;
            }
            
            auto mi = (s - EGTB_START_LOSING) * 2;
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
    
    std::cerr << "Fatal error: using old version endgames." << std::endl;
    exit(-1);

    assert(false);
    return 0;
}


bool EgtbFileWriter::createBuffersForGenerating() {
    memMode = EgtbMemMode::all;
    auto sz = getSize();
    auto bufSz = sz;
    if (isTwoBytes()) {
        bufSz += bufSz;
    }

    bool r = createBuf(bufSz, 0) && createBuf(bufSz, 1);
    startpos[0] = startpos[1] = 0;
    endpos[0] = endpos[1] = sz;
    return r;
}


void EgtbFileWriter::createFlagBuffer() {
    removeFlagBuffer();
    auto flagLen = getSize() / 2 + 16;
    flags = (uint8_t*) malloc(flagLen);
    memset(flags, 0, flagLen);
}


void EgtbFileWriter::removeFlagBuffer()
{
    if (flags) {
        free(flags);
        flags = nullptr;
    }
}

void EgtbFileWriter::clearFlagBuffer() {
    if (flags) {
        auto flagLen = getSize() / 2 + 16;
        memset(flags, 0, flagLen);
    }
}


std::string EgtbFileWriter::getLogFileName() const
{
    std::string fileName = getPath(W);
    GenLib::replaceString(fileName, "w.xtb", ".ini");
    return fileName;
}

int EgtbFileWriter::readFromLogFile() const {
    auto fileName = getLogFileName();

    auto vec = GenLib::readFileToLineArray(fileName);

    for (auto && str : vec) {
        if (!str.empty()) {
            auto n = std::stoi(str);
            return n;
        }
    }

    return -1;
}

void EgtbFileWriter::writeLogFile(int completedPly) const {
    auto fileName = getLogFileName();
    std::ofstream outfile(fileName.c_str());
    outfile << completedPly << "\n\n";
}

std::string EgtbFileWriter::getTmpFileName(const std::string& folder, int sd) const {
    Side side = static_cast<Side>(sd);
    return createFileName(folder, getName(), EgtbType::tmp, side, false);
}

std::string EgtbFileWriter::getFlagTmpFileName(const std::string& folder) const {
    auto fileName = getTmpFileName(folder, W);
    fileName = fileName.substr(0, fileName.length() - 5) + "f.tmt";
    return fileName;
}


bool EgtbFileWriter::readFromTmpFiles(const std::string& folder, int& ply, int& mPly)
{
    ply = readFromTmpFile(folder, 0);
    mPly = readFromTmpFile(folder, 1);
    return ply >= 0 && mPly >= 0 && (flags == nullptr || readFlagTmpFile(folder));
}

bool EgtbFileWriter::writeTmpFiles(const std::string& folder, int ply, int mPly)
{
    return writeTmpFile(folder, 0, ply) && writeTmpFile(folder, 1, mPly) && (flags == nullptr || writeFlagTmpFile(folder));
}

int EgtbFileWriter::readFromTmpFile(const std::string& folder, int sd)
{
    auto sz = getSize();
    auto bufsz = sz;
    if (isTwoBytes()) bufsz += bufsz;

    if (!pBuf[sd]) {
        createBuf(bufsz, sd);
    }
    assert(pBuf[sd]);

    return readFromTmpFile(folder, sd, 0, sz, pBuf[sd]);
}

u32 EgtbFileWriter::checksum(void* data, i64 len) const
{
    assert(data && len > 0);
    
    u32 checksum = 0;
    unsigned shift = 0;
    
    auto n = len / sizeof(u32);
    u32 *p = (u32*)data, *e = p + n;
    
    for (; p < e; p++) {
        checksum += (*p << shift);
        shift += 8;
        if (shift == 32) {
            shift = 0;
        }
    }
    
    return checksum;
}

int EgtbFileWriter::readFromTmpFile(const std::string& folder, int sd, i64 fromIdx, i64 toIdx, char * toBuf)
{
    assert(toBuf);
    auto fromOfs = fromIdx, toOfs = toIdx;
    if (isTwoBytes()) {
        fromOfs += fromOfs;
        toOfs += toOfs;
    }
    i64 bufsz = toOfs - fromOfs;
    if (bufsz <= 0) {
        return -1;
    }

    auto fileName = getTmpFileName(folder, sd);

    std::ifstream file(fileName.c_str(), std::ios::binary);

    if (!file) {
        if (egtbVerbose)
            printf("Not found temp file %s\n", fileName.c_str());
        return -1;
    }

    TmpHeader tmpHeader;
    file.seekg(0);
    i16 sign = flags == nullptr ? TmpHeaderSign : TmpHeaderSign2;
    bool ok = file.read((char *)&tmpHeader, TmpHeaderSize) && tmpHeader.sign == sign && tmpHeader.loop >= 0;

    if (ok) {
        file.seekg((i64)(TmpHeaderSize + fromOfs));
        file.read(toBuf, bufsz);
    }

    if (!ok) {
        if (egtbVerbose) {
            std::cerr << "Error: cannot read tmp file: " << fileName << ", sd: " << sd << std::endl;
        }
        file.close();
        return -1;
    }

    startpos[sd] = fromIdx;
    endpos[sd] = toIdx;

    file.close();
    
    if (fromIdx == 0 && toIdx == getSize()) {
        auto sum = checksum(pBuf[sd], bufsz);
        if (tmpHeader.checksum != sum) {
            std::cerr << "Error: checksum failed for temp file side " << (sd == B ? "Black" : "White") << ". Ignored that file." << std::endl;
            return -1;
        }
    }
    return tmpHeader.loop;
}

bool EgtbFileWriter::readFlagTmpFile(const std::string& folder)
{
    assert(flags);
    
    auto fileName = getFlagTmpFileName(folder);
    
    auto bufsz = (getSize() + 1) / 2;
    std::ifstream file(fileName.c_str(), std::ios::binary);
    
    if (!file) {
        if (egtbVerbose)
            std::cerr << "Not found temp file " << fileName << std::endl;
        return false;
    }
    
    TmpHeader tmpHeader;
    file.seekg(0);
    bool ok = file.read((char *)&tmpHeader, TmpHeaderSize) && tmpHeader.sign == TmpHeaderSign2;
    
    if (ok) {
        file.seekg((i64)(TmpHeaderSize));
        file.read((char*)flags, bufsz);
    }
    
    if (!ok) {
        if (egtbVerbose)
            std::cerr << "Error: cannot read data for flags with temp file " << fileName << std::endl;
        file.close();
        return false;
    }
    
    file.close();
    
    auto sum = checksum(flags, bufsz);
    if (tmpHeader.checksum != sum) {
        if (egtbVerbose)
            std::cerr << "Error: cannot read data for flags (checksum failed) with temp file " << fileName << std::endl;
        return false;
    }
    
    return true;
}

bool EgtbFileWriter::writeFlagTmpFile(const std::string& folder)
{
    auto bufsz = (getSize() + 1) / 2;
    
    auto fileName = getFlagTmpFileName(folder);

    std::ofstream outfile(fileName.c_str(), std::ios::binary);
    
    if (!outfile) {
        std::cerr << "Error: cannot open to write temp file " << fileName << std::endl;
        return false;
    }
    
    TmpHeader tmpHeader;
    tmpHeader.sign = TmpHeaderSign2;
    tmpHeader.checksum = checksum(flags, bufsz);
    outfile.seekp(0);
    
    if (outfile.write((char *)&tmpHeader, TmpHeaderSize) && outfile.write((char*)flags, bufsz)) {
        outfile.close();
        if (egtbVerbose)
            std::cout << "Successfully save to flag tmp, file: " << fileName << std::endl;
        return true;
    }
    
    std::cerr << "Error: cannot write flag tmp file " << fileName << std::endl;
    outfile.close();
    return false;
}


bool EgtbFileWriter::writeTmpFile(const std::string& folder, int sd, int loop)
{
    auto sz = getSize(), bufSz = sz;
    if (isTwoBytes()) bufSz += bufSz;

    assert(pBuf[sd]);

    auto fileName = getTmpFileName(folder, sd);
    std::ofstream outfile(fileName.c_str(), std::ios::binary);

    if (egtbVerbose) {
        std::cout << "writeTmpFile, starting sd: " << sd << ", bufsz: " << GenLib::formatString(bufSz) << std::endl;
    }
    
    if (!outfile) {
        printf("EgtbFileWriter::writeTmpFile, Error: Cannot open file %s\n", fileName.c_str());
        return false;
    }

    TmpHeader tmpHeader;
    i16 sign = flags == nullptr ? TmpHeaderSign : TmpHeaderSign2;
    tmpHeader.sign = sign;
    tmpHeader.loop = loop;
    tmpHeader.checksum = checksum(pBuf[sd], bufSz);

    outfile.seekp(0);

    if (outfile.write((char *)&tmpHeader, TmpHeaderSize) &&
        outfile.write(pBuf[sd], bufSz)) {
        outfile.close();
        if (egtbVerbose)
            std::cout << "Successfully save to tmp sd " << sd << ", at " << loop << ", file: " << fileName << std::endl;
        return true;
    }

    printf("EgtbFileWriter::writeTmpFile Error: cannot write data, sd=%d, %s\n", sd, fileName.c_str());
    outfile.close();
    return false;
}

void EgtbFileWriter::removeTmpFiles(const std::string& folder) const
{
    for(int sd = 0; sd < 2; sd++) {
        auto fileName = getTmpFileName(folder, sd);
        std::remove(fileName.c_str());
    }
    auto fileName = getFlagTmpFileName(folder);
    std::remove(fileName.c_str());
}

std::string EgtbFileWriter::createStatsString()
{
    std::ostringstream stringStream;
    stringStream << "Name:\t\t\t" << getName() << std::endl;

    i64 validCnt[2] = { 0, 0 };
    auto smallestCell = EGTB_SCORE_MATE;
    i64 wdl[2][3] = {{0, 0, 0}, {0, 0, 0}};

    for (auto sd = 0; sd < 2; sd++) {
        Side side = static_cast<Side>(sd);
        for(i64 idx = 0; idx < getSize(); idx++) {
            auto score = getScore(idx, side);
            if (score == EGTB_SCORE_ILLEGAL) {
                continue;
            }
            validCnt[sd]++;
            if (score == EGTB_SCORE_DRAW) {
                wdl[sd][1]++;
            } else if (score <= EGTB_SCORE_MATE || score >= EGTB_SCORE_PERPETUAL_CHECKED) {
                if (score > 0 || score == EGTB_SCORE_PERPETUAL_CHECKED || score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) wdl[sd][0]++;
                else wdl[sd][2]++;
                if (score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) wdl[sd][2]++;
                int absScore = abs(score);
                smallestCell = std::min(smallestCell, absScore);
            }
        }
    }

    stringStream << "Total positions:\t" << getSize() << std::endl;
    i64 total = validCnt[0] + validCnt[1];
    stringStream << "Legal positions:\t" << total << " (" << (total * 50 / getSize()) << "%) (2 sides)" << std::endl;  // count size for both sides, thus x 50 insteal of 100

    i64 w = wdl[1][0] * 100 / validCnt[1];
    stringStream << "White to move,\t\twin: " << w << "%";
    if (w == 0 &&  wdl[1][0] > 0) {
        stringStream << " (" << wdl[1][0] << ")";
    }
    stringStream << ", draw: " << wdl[1][1] * 100 / validCnt[1] << "%";
    if (wdl[1][2] || isBothArmed()) {
        stringStream << ", loss: " << wdl[1][2] * 100 / validCnt[1] << "%";
    }
    stringStream << std::endl;

    stringStream << "Black to move,\t\t";
    if (wdl[0][0] || isBothArmed()) {
        w = wdl[0][0] * 100 / validCnt[0];
        stringStream << "win: " << w << "%";
        if (wdl[0][0] > 0 && (w == 0 || !isBothArmed())) {
            stringStream << " (" << wdl[0][0] << ")";
        }
        stringStream << ", ";
    }
    stringStream << "draw: " << wdl[0][1] * 100 / validCnt[0] << "%, loss: " << wdl[0][2] * 100 / validCnt[0] << "%" << std::endl;

    stringStream << "Max distance to mate:\t" << abs(EGTB_SCORE_MATE - smallestCell) << std::endl;
    return stringStream.str();
}

void EgtbFileWriter::createStatsFile()
{
    std::string str = createStatsString();
    auto statsFileName = getPath(B);
    statsFileName = statsFileName.substr(0, statsFileName.length() - 5) + ".txt"; // 5 = "b.xtb"

    std::remove(statsFileName.c_str());
    GenLib::writeTextFile(statsFileName, str);
}

std::string EgtbFileWriter::createFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, Side side, bool compressed)
{
    auto t = static_cast<int>(egtbType);
    const char* ext = compressed ? EgtbFile::egtbCompressFileExtensions[t] : EgtbFile::egtbFileExtensions[t];
    auto theName = name;
    Funcs::toLower(theName);
    return folderName + "/" + theName + (side == Side::black ? "b" : "w") + ext;
}

bool EgtbFileWriter::existFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, Side side, bool compressed)
{
    if (side == Side::none) {
        return GenLib::existFile(createFileName(folderName, name, egtbType, Side::white, compressed).c_str()) &&
            GenLib::existFile(createFileName(folderName, name, egtbType, Side::black, compressed).c_str());
    }
    return GenLib::existFile(createFileName(folderName, name, egtbType, side, compressed).c_str());
}

i64 totalSize = 0, illegalCnt = 0, drawCnt = 0, compressedUndeterminedCnt = 0;

bool EgtbFileWriter::saveFile(const std::string& folder, int sd, CompressMode compressMode) {
    assert(sd == 0 || sd == 1);
    auto side = static_cast<Side>(sd);
    auto compress = compressMode != CompressMode::compress_none;
    const std::string thePath = createFileName(folder, getName(), getEgtbType(), side, compress);

    setPath(thePath, sd);
    std::ofstream outfile (thePath, std::ofstream::binary);
    outfile.seekp(0);

    auto oldProperty = header->getProperty();

    if (compress) {
        header->addProperty(EGTB_PROP_COMPRESSED);
    } else {
        header->setProperty(header->getProperty() & ~EGTB_PROP_COMPRESSED);
    }

    header->setProperty(header->getProperty() & ~EGTB_PROP_NEW);
    header->setProperty(header->getProperty() & ~(EGTB_PROP_LARGE_COMPRESSTABLE_B | EGTB_PROP_LARGE_COMPRESSTABLE_W));
    
    if (compressMode == CompressMode::compress_optimizing) {
        header->addProperty(EGTB_PROP_COMPRESS_OPTIMIZED);
    } else {
        header->setProperty(header->getProperty() & ~EGTB_PROP_COMPRESS_OPTIMIZED);
    }

    header->setOnlySide(side);

#ifdef _WIN32
    strncpy_s(header->getCopyright(), COPYRIGHT_BUFSZ, COPYRIGHT, strlen(COPYRIGHT));
#else
    strcpy(header->getCopyright(), COPYRIGHT);
#endif

//    header->setSignature(EGTB_ID_MAIN);
    header->resetSignature();

    auto r = true;

    if (outfile) {
        auto size = getSize();
        auto bufSz = size;
        if (isTwoBytes()) bufSz += bufSz;

        if (compress) {
            totalSize += size;
            // TODO
            int blocksize = getCompressBlockSize();
            int blockNum = (int)((bufSz + blocksize - 1) / blocksize);
            assert(blockNum > 0);

            // 5 bytes per item
            u8* blocktable = (u8*)malloc(blockNum * 5 + 64);
            i64 compBufSz = bufSz + 2 * blockNum + 2 * blocksize;
            char *compBuf = (char *)malloc(compBufSz);

            /*
             * Convert all illegal to previous one to improve compress ratio
             */
            if (compressMode == CompressMode::compress_optimizing) {
                
                auto sameLastCell = false;
                int lastScore = 0;
                for (i64 i = 0; i < size; i++) {
                    auto score = getScore(i, side);
                    if (score == EGTB_SCORE_ILLEGAL) {
                        auto b = true;
                        if (!sameLastCell && i + 1 < size) {
                            auto score2 = getScore(i + 1, side);
                            if (score2 != EGTB_SCORE_ILLEGAL) {
                                score = score2;
                                b = false;
                            }
                        }
                        if (b) {
                            sameLastCell = true;
                            score = lastScore;
                        }
                        setBufScore(i, score, sd);
                    } else {
                        sameLastCell = lastScore == score;
                    }
                    
                    lastScore = score;
                }
            }

            int64_t compSz = CompressLib::compressAllBlocks(blocksize, blocktable, compBuf, (char*)pBuf[sd], bufSz);
            assert(compSz < bufSz);

            if (compSz > bufSz || compSz > EGTB_LARGE_COMPRESS_SIZE) {
                std::cerr << "\nError: cannot compress compSz (" << compSz << " > size (" << size << ")\n";
                exit(-1);
            }

            int bytePerItem = 4;

            if (compSz > EGTB_SMALL_COMPRESS_SIZE) {
                bytePerItem = 5;
                assert( (*((i64*)(blocktable + 5 * (blockNum - 1))) & EGTB_LARGE_COMPRESS_SIZE) == compSz);

                header->addProperty(EGTB_PROP_LARGE_COMPRESSTABLE_B << sd);
                std::cout << "NOTE: Using 5 bytes per item for compress table\n\n";
            } else {
                assert((*((u32*)blocktable + blockNum - 1) & EGTB_SMALL_COMPRESS_SIZE) == compSz);
            }

            auto blockTableSize = blockNum * bytePerItem;

            if (r && !saveHeader(outfile)) {
                r = false;
            }
            
            if (r && !outfile.write ((char*)blocktable, blockTableSize)) {
                r = false;
            }

            if (r && compBuf && !outfile.write((char*)compBuf, compSz)) {
                r = false;
            }

            if (blocktable) {
                free(blocktable);
            }
            if (compBuf) {
                free(compBuf);
            }

        } else
        if (!saveHeader(outfile) || !outfile.write (pBuf[sd], bufSz)) {
            r = false;
        }
    }

    header->setProperty(oldProperty);

    return r;
}

bool EgtbFileWriter::verifyKey(int threadIdx, i64 idx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.board);
    
    auto r = verifyAKey(*rcd.board, idx);
    if (r) {
        rcd.cnt++;
        
        if (rcd.board->isLegal()) {
            threadRecordVec.at(threadIdx).changes++;
        }

    } else {
        std::lock_guard<std::mutex> thelock(printMutex);
        rcd.board->printOut("FAILED verifyKey, idx: " + std::to_string(idx));
    }

    assert(r);
    return r;
}


bool EgtbFileWriter::verifyKeys_loop(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    if (!rcd.board) {
        rcd.board = new EgtbBoard();
    }

    for(i64 idx = rcd.fromIdx; idx < rcd.toIdx && rcd.ok; idx++) {
        if (!verifyKey(threadIdx, idx)) {
            assert(false);
            setAllThreadNotOK();
            return false;
        }
    }
    return true;
}

i64 totalConflictLocCnt = 0, totalFacingCnt = 0, allSize = 0;

bool EgtbFileWriter::verifyKeys() {
//    std::cout << "verifyKeys: " << getName() << ", sz: " << GenLib::formatString(getSize()) << std::endl;

    setupThreadRecords(getSize());
    resetAllThreadRecordCounters();

    std::vector<std::thread> threadVec;
    for (auto i = 1; i < threadRecordVec.size(); ++i) {
        threadVec.push_back(std::thread(&EgtbFileWriter::verifyKeys_loop, this, i));
    }
    verifyKeys_loop(0);
    
    for (auto && t : threadVec) {
        t.join();
    }
    
    i64 conflictLocCnt = 0, facingCnt = 0;
    for(auto && rcd : threadRecordVec) {
        conflictLocCnt += rcd.cnt;
        facingCnt += rcd.changes;
    }
    totalConflictLocCnt += conflictLocCnt;
    totalFacingCnt += facingCnt;
    allSize += getSize();
    
    std::cout << getName() << "," << getSize()
        << "," << conflictLocCnt << "," << conflictLocCnt * 100 / getSize()
        << "," << facingCnt << "," << facingCnt * 100 / getSize()
        << "," << totalConflictLocCnt << "," << totalFacingCnt
        << "," << totalConflictLocCnt * 100 / allSize << "," << totalFacingCnt * 100 / allSize
        << "," << (totalConflictLocCnt + totalFacingCnt) * 100 / allSize
        << std::endl;

    auto r = allThreadOK();
    if (r) {
//        std::cout << "    passed: " << getName() << std::endl;
    } else {
        std::cerr << "    FAILED: " << getName() << std::endl;
    }
    return r;
}

