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

#include <sys/stat.h>
#include <time.h>
#include <iomanip>
#include <thread>
#include <set>

#include "egtbgenfilemng.h"

#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;

extern bool twoBytes;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void createSubName(const std::string& aName, std::map<std::string, NameRecord>& allNameMap) {
    if (allNameMap.find(aName) != allNameMap.end()) {
        return;
    }
    NameRecord record(aName);
    if (!record.isValid()) {
        return;
    }

    allNameMap[aName] = record;

    auto k = aName.find_last_of("k");
    std::string ss[2];
    ss[0] = aName.substr(0, k);
    ss[1] = aName.substr(k);
    
    for(auto sd = 0; sd < 2; sd++) {
        auto s = ss[sd];
        auto xs = ss[1 - sd];
        assert(std::count(s.begin(), s.end(), 'k') == 1);
        
        for(auto i = 1; i < s.length(); i++) {
            std::string ss = s.substr(0, i) + (i + 1 < s.length() ? s.substr(i + 1) : "");

            createSubName(ss + xs, allNameMap);
            createSubName(xs + ss, allNameMap);
        }
    }
}

void addName(std::vector<std::string>& names, int n, std::string l, std::string r)
{
    auto l0 = l.size(), l1 = r.size();
    if (l0 + l1 >= n) {
        if (l0 + l1 == n) {
#ifdef _FELICITY_CHESS_
            names.push_back("k" + l + "k" + r);
#else
            names.push_back("k" + l + "aabbk" + r + "aabb");
#endif
        }
        return;
    }

#ifdef _FELICITY_CHESS_
    auto from = QUEEN;
#else
    auto from = ROOK;
#endif

    for(auto t = from; t <= PAWN; t++) {
        auto c = Funcs::pieceTypeName[t];
        addName(names, n, l + c, r);
        addName(names, n, l, r + c);
        addName(names, n, l + c, r + c);
    }
}

/// from a given names, get all sub-endgames
std::vector<std::string> EgtbGenFileMng::parseName(const std::string& name, bool includeSubs)
{
    if (Funcs::is_integer(name)) {
        auto n = std::stoi(name);
        std::vector<std::string> names;
        addName(names, n, "", "");
        return parseNames(names);
    }

    if (includeSubs) {
        std::vector<std::string> names = { name };
        return parseNames(names);
    }
    
    std::vector<std::string> resultVec;
    NameRecord record(name);
    if (record.isValid()) {
        resultVec.push_back(name);
    }
    return resultVec;
}

std::vector<std::string> EgtbGenFileMng::parseNames(const std::vector<std::string>& names)
{
    
    std::map<std::string, NameRecord> allNameMap;
    
    for(auto && name : names) {
        createSubName(name, allNameMap);
    }
    
    std::vector<NameRecord> recordVec;
    
    for(auto && x : allNameMap) {
        recordVec.push_back(x.second);
    }
    
    sort(recordVec.begin(), recordVec.end(), [ ]( const NameRecord& left, const NameRecord& right) {
        return left.isMeSmaller(right);
    });
    
    std::vector<std::string> resultVec;
    for(auto && x : recordVec) {
        resultVec.push_back(x.name);
    }
    
    
    return resultVec;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenFileMng::showSubTables(const std::string& name, EgtbType egtbType) {
    std::cout << "All sub endgames for " << name << " (order - name - index size)" << std::endl;
    auto egNames = parseName(name);
    showSubTables(egNames, egtbType);
}

void EgtbGenFileMng::showSubTables(const std::vector<std::string>& egNames, EgtbType)
{
    i64 total = 0;
    auto cnt = 0;
    for(auto && aName : egNames) {
        assert(std::count(aName.begin(), aName.end(), 'k') == 2);

        cnt++;
        auto sz = EgtbFile::computeSize(aName);
        total += sz;
        std::cout << std::setw(3) << cnt << ") " << std::setw(16) << aName << "  " << std::setw(15) << GenLib::formatString(sz) << std::endl;
    }

    std::cout << std::setw(0)<< std::endl;
    std::cout << "Total files: " << cnt << ", total size: " << GenLib::formatString(total) << std::endl;
}

void EgtbGenFileMng::showIntestingSubTables(EgtbType egtbType)
{
    std::cout << "All intesting sub endgames (order - name - index size)" << std::endl;
    
#ifdef _FELICITY_CHESS_
    /// 5 men (3 attackers)
    std::vector<std::string> names;
    
    for(auto t0 = QUEEN; t0 <= PAWN; t0++) {
        auto c0 = Funcs::pieceTypeName[t0];
        for(auto t1 = t0; t1 <= PAWN; t1++) {
            auto c1 = Funcs::pieceTypeName[t1];

            for(auto t2 = QUEEN; t2 <= PAWN; t2++) {
                auto c2 = Funcs::pieceTypeName[t2];

                std::string name = std::string("k") + c0 + c1 + c2 + "k";
                names.push_back(name);

                name = std::string("k") + c0 + c1 + "k" + c2;
                names.push_back(name);
            }
        }
    }
#else
    std::vector<std::string> names = {
        "kraabbkaabb", /// 1 attackers
        ///
        /// 2 attackers
        "kccaabbkaabb",
        "knnaabbkaabb",
        "kcnaabbkaabb",
        "kcpaabbkaabb",
        "knpaabbkaabb",
        "kppaabbkaabb",
    };
#endif
    
    auto egNames = parseNames(names);
    showSubTables(egNames, egtbType);
}

void EgtbGenFileMng::createStatsFiles()
{
    std::cout << "EgtbGenFileMng::createStatsFiles BEGIN\n";
    
    //    std::string folder = "";
    //    preload(folder, EgtbMemMode::all);
    //
    //    for (auto && egtbFile : egtbFileVec) {
    //        egtbFile->createStatsFile();
    //    }
    std::cout << "EgtbGenFileMng::createStatsFiles END\n";
}

std::vector<std::string> EgtbGenFileMng::showMissing(const std::string& startName) const {
    std::vector<std::string> vec = parseName(startName);

    i64 total = 0;
    int cnt = 0;
    
    std::vector<std::string> resultVec;
    
    for(auto && aName : vec) {
        if (nameMap.find(aName) != nameMap.end()) {
            continue;
        }
        
        auto sz = EgtbFile::computeSize(aName);
        cnt++;
        total += sz;
        
        std::cout << std::setw(3) << cnt << ") " << std::setw(16) << aName << "  " << std::setw(15) << GenLib::formatString(sz) << std::endl;
        
        resultVec.push_back(aName);
    }
    
    std::cout << "#missing: " << cnt << ", total sz: " << GenLib::formatString(total) << "\n\n";
    return resultVec;
}

/////////////////////////

bool EgtbGenFileMng::compress(std::string folder, const std::string& endgameName, bool includingSubEndgames, bool compress) {
    std::cout  << "Start " << (compress ? "compressing" : "uncompressing") << " " << endgameName << std::endl;

    std::vector<std::string> vec = parseName(endgameName);

    if (!folder.empty()) {
        GenLib::createFolder(folder);
        folder += "/";
    }
    
    auto writtenfolder = folder + "tmp";
    GenLib::createFolder(writtenfolder);
    writtenfolder += "/";

    auto compressMode = compress ? CompressMode::compress : CompressMode::compress_none;
    
    int count = 0, succ = 0;
    if (includingSubEndgames) {
        
        assert(false);
        /// WARNING
//        SubfolderParser parser0(endgameName);
//        auto attackingCnt = parser0.attackingCnt;
//        
//        for(auto && aName : vec) {
//            SubfolderParser parser(aName);
//            if (attackingCnt != parser.attackingCnt) {
//                continue;
//            }
//            
//            count++;
//            
//            if (nameMap.find(aName) == nameMap.end()) {
//                continue;
//            }
//            
//            auto egtbFile = (EgtbGenFile*) nameMap[aName];
//            
//            parser.createAllSubfolders(writtenfolder);
//            std::string wFolder = writtenfolder + parser.subfolder;
//            
//            if (compressEndgame(egtbFile, writtenfolder, compressMode)) {
//                succ++;
//            }
//            
//        }
    } else {
        count++;
        
        if (nameMap.find(endgameName) != nameMap.end()) {
            auto egtbFile = (EgtbGenFile*) nameMap[endgameName];
            
            assert(false);
            /// WARNING
//            SubfolderParser parser(endgameName);
//            parser.createAllSubfolders(writtenfolder);
//            std::string wFolder = writtenfolder + parser.subfolder;
//            
//            if (compressEndgame(egtbFile, writtenfolder, compressMode)) {
//                succ++;
//            }
        }
    }
    
    std::cout << "Compress/uncompress ENDED " << endgameName << ", total: " << count << ", successfully: " << succ << std::endl;
    return true;
}


bool EgtbGenFileMng::compressEndgame(EgtbGenFile* egtbFile, std::string writtenfolder, CompressMode compressMode) {
    auto aName = egtbFile->getName();
    std::cout  << "\t" << aName << std::endl;
    
    assert(false);
    /// WARNING

//    SubfolderParser parser(aName);
//    parser.createAllSubfolders(writtenfolder);
//    std::string wFolder = writtenfolder + parser.subfolder;
//    
//    if (!EgtbGenFile::existFileName(wFolder, aName, egtbFile->getEgtbType(), Side::none, compressMode != CompressMode::compress_none)) {
//        
//        for(auto sd = 0; sd < 2; ++sd) {
//            auto side = static_cast<Side>(sd);
//            if (egtbFile->forceLoadHeaderAndTable(side)) {
//                egtbFile->saveFile(wFolder, side, compressMode);
//                egtbFile->removeBuffers();
//                if (egtbVerbose) {
//                    std::cout << "\t\tSuccessfully save " << egtbFile->getPath(side) << std::endl;
//                }
//            } else {
//                std::cerr << "\n******Error: Cannot read " << egtbFile->getPath(side) << std::endl << std::endl << std::endl;
//                return false;
//            }
//        }
//        
//    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenFileMng::verifyData(const std::vector<std::string>& nameVec)
{
    std::cout << "Found total endgames: " << egtbFileVec.size() << std::endl;
    
    
    auto count = 0, succ = 0, missing = 0;

    for(auto && endgameName : nameVec) {
        std::cout << "\nVerify " << endgameName << std::endl;

        auto egtbFile = (EgtbGenFile*)nameMap[endgameName];
        if (egtbFile) {
            count++;
            if (verifyData(egtbFile)) {
                succ++;
            }
            removeAllBuffers();
        } else {
            std::cerr << "Error: missing " << endgameName << std::endl;
            missing++;
        }
    }
    std::cout << "Verify COMPLETED. total: " << count << ", passed: " << succ << ", missing: " << missing << std::endl;
}


bool EgtbGenFileMng::verifyData_chunk(int threadIdx, EgtbFile* pEgtbFile) {
    assert(pEgtbFile);
    auto& rcd = threadRecordVec.at(threadIdx);

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tverify " << pEgtbFile->getName() << ", started by thread " << threadIdx << ") range: " << rcd.fromIdx << " -> " << rcd.toIdx << std::endl;
    }

    EgtbBoard board;

    assert(rcd.fromIdx < rcd.toIdx);
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx && verifyDataOK; idx ++) {
        int curScore[] = {
            pEgtbFile->getScore(idx, Side::black, false),
            pEgtbFile->getScore(idx, Side::white, false)
        };

        auto k = idx - rcd.fromIdx;
        if (k % (64L * 1024 * 1024) == 0 && egtbVerbose) {
            auto elapsed = u64(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count()) / 1000.0;
            if (elapsed <= 0) elapsed = 1;
            if (egtbVerbose) {
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cout << "\tverify threadIdx = " << threadIdx << ", idx = " << GenLib::formatString(idx) << " of " << rcd.toIdx << " " << k * 100 / (rcd.toIdx - rcd.fromIdx) << "%, speed: " << GenLib::formatSpeed((int)(k / elapsed)) << std::endl;
            }
        }

        auto b = pEgtbFile->setupBoard(board, idx, FlipMode::none, Side::white);

        if (!b) {
            if (curScore[0] != EGTB_SCORE_ILLEGAL || curScore[1] != EGTB_SCORE_ILLEGAL) {
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cerr << "Error: cannot create a board even scores are not illegal. idx: " << idx << " scores: " << curScore[0] << ", " << curScore[1] << std::endl;
                board.printOut();
                //                exit(-1);
                verifyDataOK = false;
                return false;
            }
            continue;
        }
        
        for (auto sd = 0; sd < 2; sd ++) {
            auto side = static_cast<Side>(sd), xside = getXSide(side);
            auto bestScore = EGTB_SCORE_UNSET;
            
            if (board.isIncheck(xside)) {
                bestScore = EGTB_SCORE_ILLEGAL;
            } else {
                if ((curScore[sd] <= EGTB_SCORE_MATE && ((curScore[sd] > 0 && (curScore[sd] & 1) == 0) || (curScore[sd] < 0 && (-curScore[sd] & 1) != 0)))) {
                    verifyDataOK = false;
                    std::lock_guard<std::mutex> thelock(printMutex);
                    printf("Error: score incorrect odd/even rules, idx=%lld, sd=%d, curScore[sd]=%d\n", idx, sd, curScore[sd]);
                    board.printOut();
                    return false;
                }

                bestScore = -EGTB_SCORE_MATE;
                auto legalCount = 0;

//                if (idx == 74 && sd == 1) {
//                    board.printOut();
//                    sd = 1;
//                }
                
                for (auto && move : board.gen(side)) {
                    Hist hist;
                    board.make(move, hist);

                    if (!board.isIncheck(side)) {
                        legalCount++;
                        
                        /// If the move is a capture or a promotion, it should probe from sub-endgames
                        auto internal = hist.cap.isEmpty() && move.promotion == PieceType::empty;

                        int score;
                        
                        if (internal) {     /// score from current working buffers
                            auto idx2 = pEgtbFile->getKey(board).key;
                            score = pEgtbFile->getScore(idx2, xside, false);
//                            if (idx == 74 && sd == 1) {
//                                board.printOut();
//                                sd = 1;
//                            }

                        } else if (!board.hasAttackers()) {
                            score = EGTB_SCORE_DRAW;
                        } else {            /// probe from a sub-endgame
                            score = getScore(board, xside);
                        }
                        
                        if (score <= EGTB_SCORE_MATE) {
                            score = -score;
                            if (score > bestScore) {
                                bestScore = score;
                            }
                        }
                    }
                    board.takeBack(hist);
                }

                if (legalCount == 0) {
#ifdef _FELICITY_CHESS_
                    bestScore = board.isIncheck(side) ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
                    bestScore = -EGTB_SCORE_MATE;
#endif
                } else {
                    if (abs(bestScore) <= EGTB_SCORE_MATE && bestScore != EGTB_SCORE_DRAW) {
                        if (bestScore > 0) bestScore--;
                        else bestScore++;
                    }
                }
            }
            
            if (verifyDataOK) {
                bool ok = (curScore[sd] == bestScore || (bestScore > EGTB_SCORE_MATE && curScore[sd] > EGTB_SCORE_MATE));

                if (!ok) {
                    verifyDataOK = false;

                    std::lock_guard<std::mutex> thelock(printMutex);
                    std::cerr << "Verify FAILED " << threadIdx << ") " << pEgtbFile->getName() << ", idx: " << idx << ", sd: " << sd
                    << ", data score: " << curScore[sd] << ", calc score: " << bestScore 
                    << std::endl;
                    if (b) {
                        board.printOut();
                    }
                    
                    exit(1);
                    return false;
                }
            }
        }
    }
    
    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tverify DONE " << pEgtbFile->getName() << ", " << threadIdx << std::endl;
    }
    return true;
}

bool EgtbGenFileMng::verifyData(EgtbFile* egtbFile)
{
    assert(egtbFile);
    
    begin = std::chrono::steady_clock::now();

    egtbFile->getScore(0LL, Side::black, false);
    if (egtbFile->getLoadStatus() == EgtbLoadStatus::error) {
        return false;
    }

    std::cout << " verifying " << egtbFile->getName() << " sz: " << GenLib::formatString(egtbFile->getSize()) << " at: " << GenLib::currentTimeDate() << std::endl;
    
    
    verifyDataOK = true;
    
    setupThreadRecords(egtbFile->getSize());
    {

        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::verifyData_chunk, this, i, egtbFile));
        }

        verifyData_chunk(0, egtbFile);
        
        for (auto && t : threadVec) {
            t.join();
        }
        if (!verifyDataOK) {
            return false;
        }
    }
    
    std::cout << "       " <<  egtbFile->getName() << " passed" << std::endl;
    return true;
}

bool EgtbGenFileMng::verifyKeys(const std::string& name, EgtbType egtbType) const {
    std::cout << "TbKey::verify STARTED for " << name << std::endl;
    
    EgtbGenFile egtbFile;
    egtbFile.create(name, egtbType);
    return egtbFile.verifyKeys();
}

void EgtbGenFileMng::verifyKeys(const std::vector<std::string>& endgameNames) const
{
    std::cout << "\nVerify keys for " << endgameNames.size() << " endgames\n" << std::endl;

    EgtbType egtbType = EgtbType::dtm;
    auto count = 0, succ = 0, missing = 0;

    for(auto && aName : endgameNames) {
        std::cout << "\nVerify keys: " << aName << std::endl;
        count++;
        EgtbGenFile egtbFile;
        egtbFile.create(aName, egtbType);
        if (egtbFile.verifyKeys()) {
            succ++;
        }
    }

    std::cout << "Verify keys COMPLETED. total: " << count << ", passed: " << succ << ", missing: " << missing << std::endl;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EgtbGenFileMng::compare(EgtbFile* egtbFile0, EgtbFile* egtbFile1) const {
    auto r = true;
    auto noMatches = 20;
    i64 cnt = 0;
    for(i64 idx = 0; idx < egtbFile0->getSize(); ++idx) {
        for(auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto score0 = egtbFile0->getScore(idx, side);
            auto score1 = egtbFile1->getScore(idx, side);
            if (score0 != score1) {
                std::cerr << "Endgames " << egtbFile0->getName() << " are not matched at idx "
                << idx << " " << (sd == B ? "black" : "white")
                << ", scores " << score0 << ", " << score1
                << std::endl;
                
                EgtbBoard board;
                auto b = egtbFile0->setupBoard(board, idx, FlipMode::none, Side::white);

                if (b) {
                    board.printOut("Not matched board:");
                }

                cnt++;
                
                if (noMatches > 0) {
                    noMatches--;
                    if (noMatches == 0)
                        return false;
                }
            }
        }
    }
    
    if (noMatches < 0) {
        std::cerr << "\tTotal unmatched: " << cnt << std::endl;
    }
    return r;
}


void EgtbGenFileMng::compare(EgtbGenFileMng& otherEgtbGenFileMng, std::string endgameName, bool includingSubEndgames) {
    std::cout << "Found total endgames: " << egtbFileVec.size() << std::endl;
    std::vector<std::string> vec = parseName(endgameName, includingSubEndgames);
    
    std::cout << "\nCompare " << endgameName;
    
    if (includingSubEndgames) {
        std::cout << " and all sub-endgames, #" << vec.size();
    }
    
    std::cout << std::endl << std::endl;
    
    auto count = 0, succ = 0, err = 0;
    if (includingSubEndgames) {
        assert(false);
        /// WARNING

//
//        SubfolderParser parser(endgameName);
//        auto attackingCnt = parser.attackingCnt;
//        
//        for(auto && aName : vec) {
//            SubfolderParser parser(aName);
//            if (attackingCnt != parser.attackingCnt) {
//                continue;
//            }
//            
//            auto egtbFile0 = nameMap[aName];
//            auto egtbFile1 = otherEgtbGenFileMng.nameMap[aName];
//            
//            if (!egtbFile0 || !egtbFile1) {
//                std::cerr << "Error: missing one or both " << aName << std::endl;
//                continue;
//            }
//
//            count++;
//            if (compare(egtbFile0, egtbFile1)) {
//                succ++;
//            } else {
//                std::cerr << "Error: failed " << endgameName << std::endl;
//                err++;
//            }
//        }
    } else {
        auto egtbFile0 = nameMap[endgameName];
        auto egtbFile1 = otherEgtbGenFileMng.nameMap[endgameName];
        count++;
        if (egtbFile0 && egtbFile1 && compare(egtbFile0, egtbFile1)) {
            succ++;
        } else {
            std::cerr << "Error: failed " << endgameName << std::endl;
            err++;
        }
    }
    std::cout << "Compare COMPLETED. total: " << count << ", passed: " << succ << ", failed: " << err << std::endl;
}

