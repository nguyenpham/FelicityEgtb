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

#include <thread>

#include "../fegtb/egtb.h"
#include "../base/funcs.h"

#include "egtbgenfile.h"
#include "egtbgendb.h"
#include "compresslib.h"
#include "genlib.h"

std::mutex printMutex;

using namespace fegtb;
using namespace bslib;

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


EgtbGenFile::~EgtbGenFile()
{
}

//void EgtbGenFile::removeBuffers()
//{
//    EgtbFile::removeBuffers();
//}

void EgtbGenFile::setName(const std::string& s)
{
    header->setName(s);
}


void EgtbGenFile::addProperty(uint addprt) {
    header->addProperty(addprt);
}

void EgtbGenFile::create(const std::string& name, EgtbType _egtbType, u32 order) {
    if (header == nullptr) {
        header = new EgtbFileHeader();
    }
    header->reset();

    egtbType = _egtbType;

    loadStatus = EgtbLoadStatus::loaded;

    header->setOrder(order);
    
    if (EgtbGenDb::twoBytes) {
        header->addProperty(EGTB_PROP_2BYTES);
        assert(isTwoBytes());
    }

    setName(name);

    auto sz = setupIdxComputing(name, order); //, getVersion());
    setSize(sz); assert(sz);
}


bool EgtbGenFile::saveHeader(std::ofstream& outfile) const {
    //outfile.seekg(0LL, std::ios::beg);
    if (outfile.write(header->getData(), header->headerSize())) {
        return true;
    }

    std::cerr << "Error: can't save header" << std::endl;
    return true;
}

void EgtbGenFile::fillBufs(int score)
{
    char cell = scoreToCell(score);
    for(auto sd = 0; sd < 2; sd++) {
        if (pBuf[sd])
            std::memset(pBuf[sd], cell, size);
    }
}

int EgtbGenFile::getBufScore(i64 idx, Side side)
{
    assert (idx < getSize());
    
    int sd = static_cast<int>(side);

    if (isTwoBytes()) {
        
        assert(idx >= startpos[sd] && idx < endpos[sd]);
        const i16 * p = (const i16 * )pBuf[sd];
        i16 score = p[idx - startpos[sd]];
        return score;
    }
    
    char cell = pBuf[sd][idx - startpos[sd]];
    return cellToScore(cell);
}

bool EgtbGenFile::setBufScore(i64 idx, int score, Side side)
{
    if (isTwoBytes()) {
        return setBuf2Bytes(idx, score, side);
    }

    char cell = scoreToCell(score);
    return setBuf(idx, cell, side);
}

char EgtbGenFile::scoreToCell(int score) {
    if (score <= EGTB_SCORE_MATE) {
        if (score == EGTB_SCORE_DRAW) return TB_DRAW;
        
        auto mi = (EGTB_SCORE_MATE - abs(score)) / 2;
        
        if (mi > TB_RANGE_1_BYTE) {
            std::lock_guard<std::mutex> thelock(printMutex);
            std::cerr << "FATAL ERROR: overflown score: " << score << ". Please use 2 bytes per item (param: -2)." << std::endl;
            exit(-1);
        }

        auto k = mi + (score > 0 ? TB_START_MATING : TB_START_LOSING);
        assert(k >= 0 && k < 255);

        return (char)k;
    }

    switch (score) {
        case EGTB_SCORE_MISSING:
            return TB_MISSING;

#ifdef _FELICITY_XQ_
        case EGTB_SCORE_PERPETUATION_WIN:
            return TB_PERPETUATION_WIN;
        case EGTB_SCORE_PERPETUATION_LOSE:
            return TB_PERPETUATION_LOSE;
#endif

        case EGTB_SCORE_ILLEGAL:
            return TB_ILLEGAL;
            
        case EGTB_SCORE_UNSET:
            return TB_UNSET;
        default:
            assert(false);
            return TB_UNSET;
    }

    assert(false);
    return 0;
}


bool EgtbGenFile::createBuffersForGenerating() {
    memMode = EgtbMemMode::all;
    auto sz = getSize();
    auto bufSz = sz;
    if (isTwoBytes()) {
        bufSz += bufSz;
    }

    bool r = createBuf(bufSz, Side::black) && createBuf(bufSz, Side::white);
    startpos[0] = startpos[1] = 0;
    endpos[0] = endpos[1] = sz;
    return r;
}

//std::string EgtbGenFile::getTmpFileName(const std::string& folder, Side side) const {
//    return createFileName(folder, getName(), EgtbType::tmp, side, false);
//}
//
//std::string EgtbGenFile::getFlagTmpFileName(const std::string& folder) const {
//    auto fileName = getTmpFileName(folder, Side::white);
//    fileName = fileName.substr(0, fileName.length() - 5) + "f.tmt";
//    return fileName;
//}
//
//
//bool EgtbGenFile::readFromTmpFiles(const std::string& folder, int& ply, int& mPly)
//{
//    ply = readFromTmpFile(folder, Side::black);
//    mPly = readFromTmpFile(folder, Side::white);
//    return ply >= 0 && mPly >= 0 && (flags == nullptr || readFlagTmpFile(folder));
//}
//
//bool EgtbGenFile::writeTmpFiles(const std::string& folder, int ply, int mPly)
//{
//    return writeTmpFile(folder, Side::black, ply) && writeTmpFile(folder, Side::white, mPly) && (flags == nullptr || writeFlagTmpFile(folder));
//}
//
//int EgtbGenFile::readFromTmpFile(const std::string& folder, Side side)
//{
//    auto sz = getSize();
//    auto bufsz = sz;
//    if (isTwoBytes()) bufsz += bufsz;
//
//    auto sd = static_cast<int>(side);
//    if (!pBuf[sd]) {
//        createBuf(bufsz, side);
//    }
//    assert(pBuf[sd]);
//
//    return readFromTmpFile(folder, side, 0, sz, pBuf[sd]);
//}
//
//u32 EgtbGenFile::checksum(void* data, i64 len) const
//{
//    assert(data && len > 0);
//    
//    u32 checksum = 0;
//    unsigned shift = 0;
//    
//    auto n = len / sizeof(u32);
//    u32 *p = (u32*)data, *e = p + n;
//    
//    for (; p < e; p++) {
//        checksum += (*p << shift);
//        shift += 8;
//        if (shift == 32) {
//            shift = 0;
//        }
//    }
//    
//    return checksum;
//}
//
//int EgtbGenFile::readFromTmpFile(const std::string& folder, Side side, i64 fromIdx, i64 toIdx, char * toBuf)
//{
//    assert(toBuf);
//    auto fromOfs = fromIdx, toOfs = toIdx;
//    if (isTwoBytes()) {
//        fromOfs += fromOfs;
//        toOfs += toOfs;
//    }
//    i64 bufsz = toOfs - fromOfs;
//    if (bufsz <= 0) {
//        return -1;
//    }
//
//    auto fileName = getTmpFileName(folder, side);
//
//    std::ifstream file(fileName.c_str(), std::ios::binary);
//
//    if (!file) {
//        if (egtbVerbose)
//            printf("Not found temp file %s\n", fileName.c_str());
//        return -1;
//    }
//
//    TmpHeader tmpHeader;
//    file.seekg(0);
//    i16 sign = flags == nullptr ? TmpHeaderSign : TmpHeaderSign2;
//    bool ok = file.read((char *)&tmpHeader, TmpHeaderSize) && tmpHeader.sign == sign && tmpHeader.loop >= 0;
//
//    if (ok) {
//        file.seekg((i64)(TmpHeaderSize + fromOfs));
//        file.read(toBuf, bufsz);
//    }
//
//    if (!ok) {
//        if (egtbVerbose) {
//            std::cerr << "Error: cannot read tmp file: " << fileName << ", sd: " << Funcs::side2String(side, false) << std::endl;
//        }
//        file.close();
//        return -1;
//    }
//
//    auto sd = static_cast<int>(side);
//    startpos[sd] = fromIdx;
//    endpos[sd] = toIdx;
//
//    file.close();
//    
//    if (fromIdx == 0 && toIdx == getSize()) {
//        auto sum = checksum(pBuf[sd], bufsz);
//        if (tmpHeader.checksum != sum) {
//            std::cerr << "Error: checksum failed for temp file side " << Funcs::side2String(side, false) << ". Ignored that file." << std::endl;
//            return -1;
//        }
//    }
//    return tmpHeader.loop;
//}
//
//bool EgtbGenFile::readFlagTmpFile(const std::string& folder)
//{
//    assert(flags);
//    
//    auto fileName = getFlagTmpFileName(folder);
//    
//    auto bufsz = (getSize() + 1) / 2;
//    std::ifstream file(fileName.c_str(), std::ios::binary);
//    
//    if (!file) {
//        if (egtbVerbose)
//            std::cerr << "Not found temp file " << fileName << std::endl;
//        return false;
//    }
//    
//    TmpHeader tmpHeader;
//    file.seekg(0);
//    bool ok = file.read((char *)&tmpHeader, TmpHeaderSize) && tmpHeader.sign == TmpHeaderSign2;
//    
//    if (ok) {
//        file.seekg((i64)(TmpHeaderSize));
//        file.read((char*)flags, bufsz);
//    }
//    
//    if (!ok) {
//        if (egtbVerbose)
//            std::cerr << "Error: cannot read data for flags with temp file " << fileName << std::endl;
//        file.close();
//        return false;
//    }
//    
//    file.close();
//    
//    auto sum = checksum(flags, bufsz);
//    if (tmpHeader.checksum != sum) {
//        if (egtbVerbose)
//            std::cerr << "Error: cannot read data for flags (checksum failed) with temp file " << fileName << std::endl;
//        return false;
//    }
//    
//    return true;
//}
//
//bool EgtbGenFile::writeFlagTmpFile(const std::string& folder)
//{
//    auto bufsz = (getSize() + 1) / 2;
//    
//    auto fileName = getFlagTmpFileName(folder);
//
//    std::ofstream outfile(fileName.c_str(), std::ios::binary);
//    
//    if (!outfile) {
//        std::cerr << "Error: cannot open to write temp file " << fileName << std::endl;
//        return false;
//    }
//    
//    TmpHeader tmpHeader;
//    tmpHeader.sign = TmpHeaderSign2;
//    tmpHeader.checksum = checksum(flags, bufsz);
//    outfile.seekp(0);
//    
//    if (outfile.write((char *)&tmpHeader, TmpHeaderSize) && outfile.write((char*)flags, bufsz)) {
//        outfile.close();
//        if (egtbVerbose)
//            std::cout << "Successfully save to flag tmp, file: " << fileName << std::endl;
//        return true;
//    }
//    
//    std::cerr << "Error: cannot write flag tmp file " << fileName << std::endl;
//    outfile.close();
//    return false;
//}
//
//
//bool EgtbGenFile::writeTmpFile(const std::string& folder, Side side, int loop)
//{
//    auto sd = static_cast<int>(side);
//    auto sz = getSize(), bufSz = sz;
//    if (isTwoBytes()) bufSz += bufSz;
//
//    assert(pBuf[sd]);
//
//    auto fileName = getTmpFileName(folder, side);
//    std::ofstream outfile(fileName.c_str(), std::ios::binary);
//
//    if (egtbVerbose) {
//        std::cout << "writeTmpFile, starting sd: " << sd << ", bufsz: " << GenLib::formatString(bufSz) << std::endl;
//    }
//    
//    if (!outfile) {
//        printf("EgtbFileWriter::writeTmpFile, Error: Cannot open file %s\n", fileName.c_str());
//        return false;
//    }
//
//    TmpHeader tmpHeader;
//    i16 sign = flags == nullptr ? TmpHeaderSign : TmpHeaderSign2;
//    tmpHeader.sign = sign;
//    tmpHeader.loop = loop;
//    tmpHeader.checksum = checksum(pBuf[sd], bufSz);
//
//    outfile.seekp(0);
//
//    if (outfile.write((char *)&tmpHeader, TmpHeaderSize) &&
//        outfile.write(pBuf[sd], bufSz)) {
//        outfile.close();
//        if (egtbVerbose)
//            std::cout << "Successfully save to tmp sd " << sd << ", at " << loop << ", file: " << fileName << std::endl;
//        return true;
//    }
//
//    printf("EgtbFileWriter::writeTmpFile Error: cannot write data, sd=%d, %s\n", sd, fileName.c_str());
//    outfile.close();
//    return false;
//}
//
//void EgtbGenFile::removeTmpFiles(const std::string& folder) const
//{
//    for(int sd = 0; sd < 2; sd++) {
//        auto fileName = getTmpFileName(folder, static_cast<Side>(sd));
//        std::remove(fileName.c_str());
//    }
//    auto fileName = getFlagTmpFileName(folder);
//    std::remove(fileName.c_str());
//}

std::string EgtbGenFile::createStatsString()
{
    std::ostringstream stringStream;
    stringStream << "Name:\t\t\t" << getName() << std::endl;

    i64 validCnt[2] = { 0, 0 };
    auto smallestCell = EGTB_SCORE_MATE;
    i64 wdl[2][3] = {{0, 0, 0}, {0, 0, 0}};

#ifdef _FELICITY_XQ_
    i64 perpetuations_win[2] = { 0, 0 }, perpetuations_lose[2] = { 0, 0 };
#endif
    
    for (auto sd = 0; sd < 2; sd++) {
        auto side = static_cast<Side>(sd);
        for(i64 idx = 0; idx < getSize(); idx++) {
            auto score = getScore(idx, side);
            if (score == EGTB_SCORE_ILLEGAL) {
                continue;
            }
            validCnt[sd]++;
            if (score == EGTB_SCORE_DRAW) {
                wdl[sd][1]++;
            } else if (score <= EGTB_SCORE_MATE) {
                if (score > 0) wdl[sd][0]++;
                else wdl[sd][2]++;
                auto absScore = abs(score);
                smallestCell = std::min(smallestCell, absScore);
            }
            
#ifdef _FELICITY_XQ_
            else if (score == EGTB_SCORE_PERPETUATION_WIN) {
                perpetuations_win[sd]++;
            } else if (score == EGTB_SCORE_PERPETUATION_LOSE) {
                perpetuations_lose[sd]++;
            }
#endif
        }
    }

    stringStream << "Total positions:               " << getSize() << std::endl;
    i64 total = validCnt[0] + validCnt[1];
    stringStream << "Legal positions:               " << total << " (" << (total * 50 / getSize()) << "%) (2 sides)" << std::endl;  // count size for both sides, thus x 50 insteal of 100

    for(auto sd = 1; sd >= 0; sd--) {
        i64 w = wdl[sd][0] * 100 / validCnt[sd];
        stringStream << (sd == W ? "White" : "Black") << " to move (WDL):      " << w << "%";
        if (w == 0 &&  wdl[sd][0] > 0) {
            stringStream << " (" << wdl[sd][0] << ")";
        }
        stringStream << ", " << wdl[sd][1] * 100 / validCnt[sd] << "%";
        if (wdl[sd][2] || isBothArmed()) {
            stringStream << ", " << wdl[sd][2] * 100 / validCnt[sd] << "%";
        }
        stringStream << std::endl;
        
#ifdef _FELICITY_XQ_
        if (perpetuations_win[sd] + perpetuations_lose[sd] > 0) {
            stringStream << "Perpetuations (WL):            " << perpetuations_win[sd] << ", " << perpetuations_lose[sd] << std::endl;
        }
#endif
        
    }

//    stringStream << "Black to move,\t\t";
//    if (wdl[0][0] || isBothArmed()) {
//        w = wdl[0][0] * 100 / validCnt[0];
//        stringStream << "win: " << w << "%";
//        if (wdl[0][0] > 0 && (w == 0 || !isBothArmed())) {
//            stringStream << " (" << wdl[0][0] << ")";
//        }
//        stringStream << ", ";
//    }
//    stringStream << "draw: " << wdl[0][1] * 100 / validCnt[0] << "%, loss: " << wdl[0][2] * 100 / validCnt[0] << "%" << std::endl;

    stringStream << "Max distance to mate:\t" << abs(EGTB_SCORE_MATE - smallestCell) << std::endl;
    return stringStream.str();
}

void EgtbGenFile::createStatsFile()
{
    auto str = createStatsString();
    auto statsFileName = getPath(Side::black);
    auto pos = statsFileName.find_last_of(".");
    if (pos != std::string::npos) {
        statsFileName = statsFileName.substr(0, pos);
    }
    statsFileName = statsFileName.substr(0, statsFileName.length() - 2) + ".txt"; /// 2 is the length of ".b"

    std::remove(statsFileName.c_str());
    GenLib::writeTextFile(statsFileName, str);
}

std::string EgtbGenFile::createFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, Side side, bool compressed)
{
    auto t = static_cast<int>(egtbType);
    const char* ext = EgtbFile::egtbFileExtensions[t];
    auto theName = name;
    Funcs::toLower(theName);
    return folderName + STRING_PATH_SLASH + theName + (side == Side::black ? ".b" : ".w") + ext;
}

bool EgtbGenFile::existFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, Side side, bool compressed)
{
    if (side == Side::none) {
        return GenLib::existFile(createFileName(folderName, name, egtbType, Side::white, compressed).c_str()) &&
            GenLib::existFile(createFileName(folderName, name, egtbType, Side::black, compressed).c_str());
    }
    return GenLib::existFile(createFileName(folderName, name, egtbType, side, compressed).c_str());
}

static i64 totalSize = 0, illegalCnt = 0, drawCnt = 0, compressedUndeterminedCnt = 0;

bool EgtbGenFile::saveFile(const std::string& folder, Side side, CompressMode compressMode)
{
    assert(compressMode != compress_none);
    auto sd = static_cast<int>(side);

    assert(sd == 0 || sd == 1);
    auto compress = compressMode != CompressMode::compress_none;
    auto thePath = createFileName(folder, getName(), getEgtbType(), side, compress);

    setPath(thePath, side);
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

    header->setCopyright(COPYRIGHT);
    header->resetSignature();

    auto r = true;

    if (outfile) {
        auto size = getSize();
        auto bufSz = size;
        if (isTwoBytes()) bufSz += bufSz;

        if (compress) {
            totalSize += size;
            /// TODO
            auto blocksize = getCompressBlockSize();
            auto blockNum = (int)((bufSz + blocksize - 1) / blocksize);
            assert(blockNum > 0);

            /// 5 bytes per item
            u8* blocktable = (u8*)malloc(blockNum * 5 + 64);
            i64 compBufSz = bufSz + 2 * blockNum + 2 * blocksize;
            char *compBuf = (char *)malloc(compBufSz);

            /*
             * Convert all illegal to previous one to improve compress ratio
             */
            if (compressMode == CompressMode::compress_optimizing) {
                
                auto sameLastCell = false;
                auto lastScore = 0;
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
                        setBufScore(i, score, side);
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

            auto bytePerItem = 4;

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

            assert(compBuf);
            if (r && compBuf && !outfile.write((char*)compBuf, compSz)) {
                r = false;
                std::cerr << "\nError: cannot save compBuf compSz=" << compSz << std::endl;
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
            std::cerr << "\nError: cannot save header or buf for sd=" << sd << std::endl;
        }
    }

    header->setProperty(oldProperty);

    if (outfile) {
        outfile.close();
    }

    return r;
}


void EgtbGenFile::checkAndConvert2bytesTo1() {
    if (!isTwoBytes()) {
        return;
    }
    
    for(i64 idx = 0; idx < getSize(); ++idx) {
        auto score = getBufScore(idx, Side::white);
        if (score < EGTB_SCORE_MATE && score != EGTB_SCORE_DRAW) {
            auto mi = TB_START_LOSING + (EGTB_SCORE_MATE - std::abs(score)) / 2;
            
            auto confirm = mi > 255;
            if (confirm) {
                std::cout << "\t\tconfirmed: 2 bytes per item." << std::endl;
                return;
            }
        }
    }
    
    std::cout << "\t\t2 bytes are redundant. Converting into 1 byte per item." << std::endl;

    /// Convert into 1 byte
    header->setProperty(header->getProperty() & ~EGTB_PROP_2BYTES);
    assert(!isTwoBytes());
    
    for(i64 idx = 0; idx < getSize(); ++idx) {
        for(auto sd = 0; sd < 2; sd++) {
            i16* p = (i16*)pBuf[sd];
            i16 score = p[idx];
            setBufScore(idx, score, static_cast<Side>(sd));
        }
    }
    
    std::cout << "\t\tconverted into 1 byte per item." << std::endl;
}

void EgtbGenFile::convert1byteTo2() {
    if (isTwoBytes()) {
        return;
    }
    
    for(auto sd = 0; sd < 2; sd++) {
        auto side = static_cast<Side>(sd);
        auto p = (i16*)malloc(getSize() * 2 + 64);
        for(i64 idx = 0; idx < getSize(); ++idx) {
            auto score = getScore(idx, side);
            p[idx] = score;
        }
        
        free(pBuf[sd]);
        pBuf[sd] = (char*)p;
    }

    header->setProperty(header->getProperty() | EGTB_PROP_2BYTES);
    assert(isTwoBytes());

    std::cout << "\t\tConverted into 2 bytes per item." << std::endl;
}

bool EgtbGenFile::verifyKey(int threadIdx, i64 idx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.board);
    
    auto bit = verifyAKey(*rcd.board, idx);
    
    /// ok if it cannot setup board or the board is valid and key is correct
    auto ok = bit == 0 || (bit & Verify_bit_valid);
    if (ok) {
        rcd.cnt++;
        if (bit & Verify_bit_valid) {
            rcd.changes++;
        }
    } else {
        std::lock_guard<std::mutex> thelock(printMutex);
        auto idx2 = getKey(*rcd.board).key;
        rcd.board->printOut("FAILED verifyKey, key: " + std::to_string(idx));
        
        idx2 = getKey(*rcd.board).key;

        if (!setupBoard(*rcd.board, idx, FlipMode::none, Side::white)) {
            std::cout << "Wrong" << std::endl;
        }
        
        rcd.board->printOut();
        std::cout << "idx2 = " << idx2 << std::endl;

//        auto b0 = setupBoard(*rcd.board, idx, FlipMode::none, Side::white);
//        rcd.board->printOut();
//        
//        if (rcd.board->isValid()) {
//            auto idx2 = getKey(*rcd.board).key;
//            rcd.board->printOut();
//        }

    }

    assert(ok);
    return ok;
}


bool EgtbGenFile::verifyKeys_loop(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    if (!rcd.board) {
        rcd.createBoards();
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

bool EgtbGenFile::verifyKeys()
{
    setupThreadRecords(getSize());
    resetAllThreadRecordCounters();

    std::vector<std::thread> threadVec;
    for (auto i = 1; i < threadRecordVec.size(); ++i) {
        threadVec.push_back(std::thread(&EgtbGenFile::verifyKeys_loop, this, i));
    }
    verifyKeys_loop(0);
    
    for (auto && t : threadVec) {
        t.join();
    }
    
    i64 cnt = 0, validCnt = 0;
    for(auto && rcd : threadRecordVec) {
        cnt += rcd.cnt;
        validCnt += rcd.changes;
    }
    
    auto r = allThreadOK();
    if (r) {
        std::cout << "    passed: " << getName() << ", size: " << getSize() << ", verified " << cnt << ", valid keys: " << validCnt << std::endl;
    } else {
        std::cerr << "    FAILED: " << getName() << std::endl;
    }

    return r;
}

void EgtbGenFile::createFlagBuffer() {
    removeFlagBuffer();
    auto flagLen = getSize() / 2 + 16;
    flags = (uint8_t*) malloc(flagLen);
    memset(flags, 0, flagLen);
}


void EgtbGenFile::removeFlagBuffer()
{
    if (flags) {
        free(flags);
        flags = nullptr;
    }
}

void EgtbGenFile::clearFlagBuffer() {
    if (flags) {
        auto flagLen = getSize() / 2 + 16;
        memset(flags, 0, flagLen);
    }
}

