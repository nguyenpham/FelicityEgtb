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

#define SAMPLE_CNT 10

static const int firstWidth = 23;

void printFormatedString(std::ostringstream& stringStream, const std::string& s0, const std::string& s1)
{
    stringStream << std::left << std::setw(firstWidth) << s0 << ": " << s1 << std::endl;
}

void EgtbFileStats::createStats(EgtbFile* egtbFile, bool pickSamples)
{
    assert(egtbFile);
    name = egtbFile->getName();
    size = egtbFile->getSize();
    isBothArmed = egtbFile->isBothArmed();
    
    for(i64 idx = 0; idx < size; idx++) {
        int scores[2] = {
            egtbFile->getScore(idx, Side::black),
            egtbFile->getScore(idx, Side::white)
        };

        for (auto sd = 0; sd < 2; sd++) {
            auto score = scores[sd];
            if (score == EGTB_SCORE_ILLEGAL) {
                continue;
            }
            validCnt[sd]++;
            if (score == EGTB_SCORE_DRAW) {
                wdl[sd][1]++;
#ifdef _FELICITY_XQ_
            } else if (EgtbFile::isPerpetualScore(score)) {
                auto k = EgtbFile::perpetualScoreToIdx(score);
                if (k >= 0) {
                    perpetuals[sd][k]++; perpCnt++;
                    if (pickSamples && perpVecs[k].size() < SAMPLE_CNT) {
                        std::pair<i64, int> p;
                        p.first = idx;
                        p.second = sd;
                        perpVecs[k].push_back(p);
                    }
                }
#endif
            } else if (score <= EGTB_SCORE_MATE) {
                if (score > 0) wdl[sd][0]++;
                else wdl[sd][2]++;
                auto absScore = abs(score);
                if (smallestCell > absScore) {
                    sampleMap.clear();
                    smallestCell = absScore;
                }
                if (smallestCell == absScore) {
                    sampleMap[idx] = sd;
                    if (pickSamples && sampleMap.size() > SAMPLE_CNT) {
                        auto p = sampleMap.begin();
                        for (auto k = 0; (rand() & 1) && k < 4; k++) {
                            p++;
                        }
                        sampleMap.erase(p);
                    }
                }
            }
            
        }
    }

}

std::string EgtbFileStats::createStatsString(EgtbFile* egtbFile)
{
    assert(egtbFile);
    createStats(egtbFile);
    

    std::ostringstream stringStream;
    
    printFormatedString(stringStream, "Name", name);

 
    printFormatedString(stringStream, "Total positions", std::to_string(size));

    i64 total = validCnt[0] + validCnt[1];
    
    std::string s = std::to_string(total) + " (" + std::to_string(total * 50 / size) + "%) (2 sides)";
    printFormatedString(stringStream, "Legal positions", s);

    
    for(auto sd = 1; sd >= 0; sd--) {
        i64 w = wdl[sd][0] * 100 / validCnt[sd];
        s = std::to_string(w) + "%";
        if (w == 0 && wdl[sd][0] > 0) {
            s += " (" + std::to_string(wdl[sd][0]) + ")";
        }
        s += ", " + std::to_string(wdl[sd][1] * 100 / validCnt[sd]) + "%";

        if (wdl[sd][2] || isBothArmed) {
            s += ", " + std::to_string(wdl[sd][2] * 100 / validCnt[sd]) + "%";
        }
        
        printFormatedString(stringStream, std::string(sd == W ? "White" : "Black") + " to move (WDL)", s);

#ifdef _FELICITY_XQ_
        if (perpCnt > 0) {
            s = std::to_string(perpetuals[sd][0]) + ", " + std::to_string(perpetuals[sd][1]);
            printFormatedString(stringStream, " Perpetual checks (WL)", s);

            s = std::to_string(perpetuals[sd][2]) + ", " + std::to_string(perpetuals[sd][3]);
            printFormatedString(stringStream, " Perpetual chases (WL)", s);
        }
#endif

    }

    auto draws = wdl[0][1] + wdl[1][1];
    s = std::to_string(draws) + ", " + std::to_string(draws * 100 / total) + "% of total legal";
    printFormatedString(stringStream, "Total draws", s);

#ifdef _FELICITY_XQ_
    if (perpCnt > 0) {
        auto checks = perpetuals[0][0] + perpetuals[0][1] + perpetuals[1][0] + perpetuals[1][1];
        auto chases = perpetuals[0][2] + perpetuals[0][3] + perpetuals[1][2] + perpetuals[1][3];
        s = std::to_string(perpCnt) + ", "
            + std::to_string(perpCnt * 100 / std::max<i64>(1, draws)) + "% of draws. #checks: "
            + std::to_string(checks) + ", #chases: " + std::to_string(chases);
        printFormatedString(stringStream, "Total perpetuations", s);
    }
#endif

    s = std::to_string(abs(EGTB_SCORE_MATE - smallestCell));
    printFormatedString(stringStream, "Max DTM", s);

    /// Examples

    if (!sampleMap.empty()) {
        stringStream << "\n\nSamples with max DTM:\n";
        
        GenBoard board;
        for(auto && p : sampleMap) {
            auto idx = p.first;
            auto sd = p.second;
            
            auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white); assert(ok);
            board.side = static_cast<Side>(sd);
            board.setFenComplete();

            stringStream << board.getFen() << std::endl;
        }
        
#ifdef _FELICITY_XQ_
//        std::vector<std::pair<i64, int>> perpVecs[4];
        auto k = EgtbFile::perpetualScoreToIdx(EGTB_SCORE_PERPETUAL_CHECK_LOSS);
//        auto k = EGTB_SCORE_PERPETUAL_CHECK_LOSS - EGTB_SCORE_PERPETUAL_CHECK_WIN;
        if (!perpVecs[k].empty()) {
            stringStream << "\n\nSamples of perpetual checks:\n";
            
            for(auto && p : perpVecs[k]) {
                auto idx = p.first;
                auto sd = p.second;
                
                auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white); assert(ok);
                board.side = static_cast<Side>(sd);
                board.setFenComplete();

                stringStream << board.getFen() << std::endl;
            }
        }
        
        k = EgtbFile::perpetualScoreToIdx(EGTB_SCORE_PERPETUAL_CHASE_LOSS);
        if (!perpVecs[k].empty()) {
            stringStream << "\n\nSamples of perpetual chases:\n";
            
            for(auto && p : perpVecs[k]) {
                auto idx = p.first;
                auto sd = p.second;
                
                auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white); assert(ok);
                board.side = static_cast<Side>(sd);
                board.setFenComplete();

                stringStream << board.getFen() << std::endl;
            }
        }
#endif
    }
    return stringStream.str();
}




EgtbGenFile::~EgtbGenFile()
{
}


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
    
    for(auto k = FirstAttacker; k < PAWN; k++) {
        if (pieceCount[B][k] > 0) {
            bothArmed = true;
            break;
        }
    }
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
    
    auto sd = static_cast<int>(side);

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
//    if ((idx == 2198 || idx == 51806) && score == 9983) {
//        std::cout << "setBufScore, idx=" + std::to_string(idx) + ", score=" + std::to_string(score) << std::endl;
//    }

    if (isTwoBytes()) {
        return setBuf2Bytes(idx, score, side);
    }

    char cell = scoreToCell(score);
    return setBuf(idx, cell, side);
}

char EgtbGenFile::scoreToCell(int score) {
    switch (score) {
        case EGTB_SCORE_DRAW:
            return TB_DRAW;
        case EGTB_SCORE_MISSING:
            return TB_MISSING;
        case EGTB_SCORE_ILLEGAL:
            return TB_ILLEGAL;
        case EGTB_SCORE_UNSET:
            return TB_UNSET;

#ifdef _FELICITY_XQ_
        case EGTB_SCORE_PERPETUAL_CHECK_WIN:
            return TB_PERPETUAL_CHECK_WIN;
        case EGTB_SCORE_PERPETUAL_CHECK_LOSS:
            return TB_PERPETUAL_CHECK_LOSS;
        case EGTB_SCORE_PERPETUAL_CHASE_WIN:
            return TB_PERPETUAL_CHASE_WIN;
        case EGTB_SCORE_PERPETUAL_CHASE_LOSS:
            return TB_PERPETUAL_CHASE_LOSS;
#endif

        default:
            break;
    }

    if (score <= EGTB_SCORE_MATE) {
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


std::string EgtbGenFile::createStatsString()
{
    return createStatsString(this);
}

std::string EgtbGenFile::createStatsString(EgtbFile* egtbFile)
{
    assert(egtbFile);
 
    EgtbFileStats stats;
    return stats.createStatsString(egtbFile);
}

//    std::ostringstream stringStream;
//    
//    printFormatedString(stringStream, "Name", egtbFile->getName());
//
//    EgtbGenFileStats stats;
//    stats.createStats(egtbFile);
//    
////    i64 validCnt[2] = { 0, 0 };
////    auto smallestCell = EGTB_SCORE_MATE;
////    i64 wdl[2][3] = {{0, 0, 0}, {0, 0, 0}};
////
////
////#ifdef _FELICITY_XQ_
////    i64 perpetuals[2][4] = {{ 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
////    auto perpCnt = 0;
////    std::vector<std::pair<i64, int>> perpVecs[4];
////#endif
////    
////    std::unordered_map<i64, int> sampleMap;
////    
////    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
////        int scores[2] = {
////            egtbFile->getScore(idx, Side::black),
////            egtbFile->getScore(idx, Side::white)
////        };
////
////        for (auto sd = 0; sd < 2; sd++) {
////            auto score = scores[sd];
////            if (score == EGTB_SCORE_ILLEGAL) {
////                continue;
////            }
////            validCnt[sd]++;
////            if (score == EGTB_SCORE_DRAW) {
////                wdl[sd][1]++;
////#ifdef _FELICITY_XQ_
////            } else if (isPerpetualScore(score)) {
////                auto k = EgtbFile::perpetualScoreToIdx(score);
////                if (k >= 0) {
////                    perpetuals[sd][k]++; perpCnt++;
////                    if (perpVecs[k].size() < SAMPLE_CNT) {
////                        std::pair<i64, int> p;
////                        p.first = idx;
////                        p.second = sd;
////                        perpVecs[k].push_back(p);
////                    }
////                }
////#endif
////            } else if (score <= EGTB_SCORE_MATE) {
////                if (score > 0) wdl[sd][0]++;
////                else wdl[sd][2]++;
////                auto absScore = abs(score);
////                if (smallestCell > absScore) {
////                    sampleMap.clear();
////                    smallestCell = absScore;
////                }
////                if (smallestCell == absScore) {
////                    sampleMap[idx] = sd;
////                    if (sampleMap.size() > SAMPLE_CNT) {
////                        auto p = sampleMap.begin();
////                        for (auto k = 0; (rand() & 1) && k < 4; k++) {
////                            p++;
////                        }
////                        sampleMap.erase(p);
////                    }
////                }
////            }
////            
////        }
////    }
//
//    printFormatedString(stringStream, "Total positions", std::to_string(egtbFile->getSize()));
//
//    i64 total = stats.validCnt[0] + stats.validCnt[1];
//    
//    std::string s = std::to_string(total) + " (" + std::to_string(total * 50 / egtbFile->getSize()) + "%) (2 sides)";
//    printFormatedString(stringStream, "Legal positions", s);
//
//    
//    for(auto sd = 1; sd >= 0; sd--) {
//        i64 w = wdl[sd][0] * 100 / validCnt[sd];
//        s = std::to_string(w) + "%";
//        if (w == 0 && wdl[sd][0] > 0) {
//            s += " (" + std::to_string(wdl[sd][0]) + ")";
//        }
//        s += ", " + std::to_string(wdl[sd][1] * 100 / validCnt[sd]) + "%";
//
//        if (wdl[sd][2] || egtbFile->isBothArmed()) {
//            s += ", " + std::to_string(wdl[sd][2] * 100 / validCnt[sd]) + "%";
//        }
//        
//        printFormatedString(stringStream, std::string(sd == W ? "White" : "Black") + " to move (WDL)", s);
//
//#ifdef _FELICITY_XQ_
//        if (perpCnt > 0) {
//            s = std::to_string(perpetuals[sd][0]) + ", " + std::to_string(perpetuals[sd][1]);
//            printFormatedString(stringStream, " Perpetual checks (WL)", s);
//
//            s = std::to_string(perpetuals[sd][2]) + ", " + std::to_string(perpetuals[sd][3]);
//            printFormatedString(stringStream, " Perpetual chases (WL)", s);
//        }
//#endif
//
//    }
//
//    auto draws = wdl[0][1] + wdl[1][1];
//    s = std::to_string(draws) + ", " + std::to_string(draws * 100 / total) + "% of total legal";
//    printFormatedString(stringStream, "Total draws", s);
//
//#ifdef _FELICITY_XQ_
//    if (perpCnt > 0) {
//        auto checks = perpetuals[0][0] + perpetuals[0][1] + perpetuals[1][0] + perpetuals[1][1];
//        auto chases = perpetuals[0][2] + perpetuals[0][3] + perpetuals[1][2] + perpetuals[1][3];
//        s = std::to_string(perpCnt) + ", "
//            + std::to_string(perpCnt * 100 / std::max<i64>(1, draws)) + "% of draws. #checks: "
//            + std::to_string(checks) + ", #chases: " + std::to_string(chases);
//        printFormatedString(stringStream, "Total perpetuations", s);
//    }
//#endif
//
//    s = std::to_string(abs(EGTB_SCORE_MATE - smallestCell));
//    printFormatedString(stringStream, "Max DTM", s);
//
//    /// Examples
//
//    if (!sampleMap.empty()) {
//        stringStream << "\n\nSamples with max DTM:\n";
//        
//        GenBoard board;
//        for(auto && p : sampleMap) {
//            auto idx = p.first;
//            auto sd = p.second;
//            
//            auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white); assert(ok);
//            board.side = static_cast<Side>(sd);
//            board.setFenComplete();
//
//            stringStream << board.getFen() << std::endl;
//        }
//        
//#ifdef _FELICITY_XQ_
////        std::vector<std::pair<i64, int>> perpVecs[4];
//        auto k = EgtbFile::perpetualScoreToIdx(EGTB_SCORE_PERPETUAL_CHECK_LOSS);
////        auto k = EGTB_SCORE_PERPETUAL_CHECK_LOSS - EGTB_SCORE_PERPETUAL_CHECK_WIN;
//        if (!perpVecs[k].empty()) {
//            stringStream << "\n\nSamples of perpetual checks:\n";
//            
//            for(auto && p : perpVecs[k]) {
//                auto idx = p.first;
//                auto sd = p.second;
//                
//                auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white); assert(ok);
//                board.side = static_cast<Side>(sd);
//                board.setFenComplete();
//
//                stringStream << board.getFen() << std::endl;
//            }
//        }
//        
//        k = EgtbFile::perpetualScoreToIdx(EGTB_SCORE_PERPETUAL_CHASE_LOSS);
//        if (!perpVecs[k].empty()) {
//            stringStream << "\n\nSamples of perpetual chases:\n";
//            
//            for(auto && p : perpVecs[k]) {
//                auto idx = p.first;
//                auto sd = p.second;
//                
//                auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white); assert(ok);
//                board.side = static_cast<Side>(sd);
//                board.setFenComplete();
//
//                stringStream << board.getFen() << std::endl;
//            }
//        }
//#endif
//    }
//    return stringStream.str();
//}

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
        if (score < EGTB_SCORE_MATE && score != EGTB_SCORE_DRAW
#ifdef _FELICITY_XQ_
            && !EgtbFile::isPerpetualScore(score)
#endif
            ) {
            auto mi = TB_START_LOSING + (EGTB_SCORE_MATE - std::abs(score)) / 2;
            
            auto confirm = mi > 255;
            if (confirm) {
                std::cout << "\t\tconfirmed: 2 bytes per item." << std::endl;
                return;
            }
        }
    }
    
    std::cout << "\t\t2 bytes redundant. Converting into 1 byte per item." << std::endl;

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
    
    std::cout << "\tconverted into 1 byte per item." << std::endl;
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

bool EgtbGenFile::verifyIndex(int threadIdx, i64 idx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.board);
    
    auto bit = verifyIndex(*rcd.board, idx);
    
    /// ok if it cannot setup board or the board is valid and key is correct
    auto ok = bit == 0 || (bit & Verify_bit_valid);
    if (ok) {
        rcd.cnt++;
        if (bit & Verify_bit_valid) {
            rcd.changes++;
        }
    } else {
        std::lock_guard<std::mutex> thelock(printMutex);
        auto idx2 = getIdx(*rcd.board).key;
        std::string s = "FAILED verifyKey, key: " + std::to_string(idx);
        if (bit & Verify_bit_bad_flip) {
            s += ", flip error";
        }
        rcd.board->printOut(s);
        
        idx2 = getIdx(*rcd.board).key;

        if (!setupBoard(*rcd.board, idx, FlipMode::none, Side::white)) {
            std::cout << "Wrong" << std::endl;
        }
        
        s += ", idx2=" + std::to_string(idx2);
        rcd.board->printOut(s);
        
        if (bit & Verify_bit_bad_flip) {
            auto flip = rcd.board->needSymmetryFlip();
            if (flip != FlipMode::none) {
                rcd.board->flip(flip);

                auto fidx2 = getIdx(*rcd.board).key;
                rcd.board->printOut("Flipped once, flipped idx2: " + std::to_string(fidx2));
                if (fidx2 != idx) {
                    auto flip3 = rcd.board->needSymmetryFlip();
                    if (flip3 != FlipMode::none) {
                        rcd.board->flip(flip3);
                        auto idx3 = getIdx(*rcd.board).key;
                        rcd.board->printOut("Flipped twice, flipped idx3: " + std::to_string(idx3));
                    }
                }
            }
        }

    }

    assert(ok);
    return ok;
}


bool EgtbGenFile::verifyIndexes_thread(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    if (!rcd.board) {
        rcd.createBoards();
    }

    for(i64 idx = rcd.fromIdx; idx < rcd.toIdx && rcd.ok; idx++) {
        if (!verifyIndex(threadIdx, idx)) {
            assert(false);
            setAllThreadNotOK();
            return false;
        }
    }
    return true;
}

i64 totalConflictLocCnt = 0, totalFacingCnt = 0, allSize = 0;

bool EgtbGenFile::verifyIndexes()
{
    setupThreadRecords(getSize());
    resetAllThreadRecordCounters();

    std::vector<std::thread> threadVec;
    for (auto i = 1; i < threadRecordVec.size(); ++i) {
        threadVec.push_back(std::thread(&EgtbGenFile::verifyIndexes_thread, this, i));
    }
    verifyIndexes_thread(0);
    
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



int EgtbGenFile::verifyIndex(GenBoard& board, i64 idx) const
{
    /// It is considered OK if it cann't setup the board
    if (!setupBoard(board, idx, FlipMode::none, Side::white)) {
        return 0;
    }
    
    auto bit = Verify_bit_setupOK;
    if (board.isValid() && getIdx(board).key == idx) {
        auto flip = board.needSymmetryFlip();
        if (flip != FlipMode::none) {
            board.flip(flip);
            auto idx2 = getIdx(board).key;
            
            if (idx2 != idx) {
                auto flip2 = board.needSymmetryFlip();
                if (flip2 == FlipMode::none) {
                    bit |= Verify_bit_bad_flip;
                    return bit;
                } else {
                    board.flip(flip2);
                    auto idx3 = getIdx(board).key;
                    
                    if (idx != idx3) {
                        bit |= Verify_bit_bad_flip;
                        return bit;
                    }
                }
            }
        }
        bit |= Verify_bit_valid;
    }
    
    return bit;
}

/**
 Simple function to test the consistancy and correctness of keys, using only one (main) thread
  For faster, using multi threads, use functions in generator code
 */
bool EgtbGenFile::verifyIndexes(bool printRandom) const
{
    GenBoard board;
    
    auto sz = getSize();
    std::cout << "Verifying Keys for " << getName() << ", size: " << sz << std::endl;

    int64_t cnt = 0;
    
    for (int64_t idx = 0; idx < sz; ++idx) {
        auto ok = verifyIndex(board, idx);

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

