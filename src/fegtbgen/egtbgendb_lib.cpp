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

#include <sys/stat.h>
#include <time.h>
#include <iomanip>
#include <thread>
#include <set>

#include "egtbgendb.h"

#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void createSubName(const std::string& aName, std::set<std::string>& allNameSet) {
    if (allNameSet.find(aName) != allNameSet.end() || !NameRecord::isValid(aName)) {
        return;
    }

    allNameSet.insert(aName);

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

            createSubName(ss + xs, allNameSet);
            createSubName(xs + ss, allNameSet);
        }
    }
}

void addName(std::vector<std::string>& names, std::string l, std::string r)
{
#ifdef _FELICITY_CHESS_
    names.push_back("k" + l + "k" + r);
#else
    names.push_back("k" + l + "aabbk" + r + "aabb");
#endif
}

void createNameByAttackerNumber(std::vector<std::string>& names, const std::string& s, int fromType, int len) {
    if (s.size() >= len) {
        return;
    }
    
#ifdef _FELICITY_CHESS_
    auto n = fromType == QUEEN ? 9 : fromType == PAWN ? 8 : 10;
#else
    auto n = fromType == PAWN ? 5 : 2;
#endif
    
    for(auto t = fromType; t <= PAWN; t++) {
        auto c = Funcs::pieceTypeName[t];
        auto ss = s;
        for(auto i = 0; i < n; i++) {
            ss += c;
            if (ss.size() == len) {
                names.push_back(ss);
                break;
            }
            createNameByAttackerNumber(names, ss, t + 1, len);
        }
    }
}

void addName(std::vector<std::string>& names, int ln, int rn)
{
    if (ln + rn == 0) {
        return;
    }
#ifdef _FELICITY_CHESS_
    auto fromType = QUEEN;
#else
    auto fromType = ROOK;
#endif
    
    std::vector<std::string> lvec, rvec;
    createNameByAttackerNumber(lvec, "", fromType, ln);
    if (rn == 0) {
        rvec.push_back("");
    } else {
        createNameByAttackerNumber(rvec, "", fromType, rn);
    }
    
    for(auto && l : lvec) {
        for(auto && r : rvec) {
#ifdef _FELICITY_CHESS_
            auto s = "k" + l + "k" + r;
#else
            auto s = "k" + l + "aabbk" + r + "aabb";
#endif
            names.push_back(s);
        }
    }
}

/// from a given names, get all sub-endgames
std::vector<std::string> EgtbGenDb::parseName(const std::string& name, bool includeSubs)
{
    auto p = name.find('-');
    if (p != std::string::npos) {
        auto s0 = name.substr(0, p);
        auto s1 = name.substr(p + 1);
        
        if (Funcs::is_integer(s0) && Funcs::is_integer(s1)) {
            auto n0 = std::stoi(s0);
            auto n1 = std::stoi(s1);
            std::vector<std::string> names;
            addName(names, n0, n1);
            return parseNames(names);
        }
        return std::vector<std::string>();
    }

    if (Funcs::is_integer(name)) {
        auto n = std::stoi(name);
        std::vector<std::string> names;
        
        for (auto n0 = 1; n0 <= n; n0++) {
            for (auto n1 = 0; n0 + n1 <= n; n1++) {
                if (n0 + n1 == n) {
                    addName(names, n0, n1);
                }
            }
        }
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

std::vector<std::string> EgtbGenDb::parseNames(const std::vector<std::string>& names)
{
    std::set<std::string> allNameSet;
    for(auto && name : names) {
        createSubName(name, allNameSet);
    }

    std::vector<NameRecord> recordVec;
    
    for(auto && name : allNameSet) {
        NameRecord record(name);
        if (record.isValid()) {
            recordVec.push_back(record);
        }
    }
    
    sort(recordVec.begin(), recordVec.end(), [ ]( const NameRecord& left, const NameRecord& right) {
        return left.isSmaller(right);
    });
    
    std::vector<std::string> resultVec;
    for(auto && x : recordVec) {
        resultVec.push_back(x.name);
    }

    return resultVec;


}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenDb::showSubTables(const std::string& name, EgtbType egtbType) {
    std::cout << "All sub endgames for " << name << " (order - name - index size)" << std::endl;
    auto egNames = parseName(name);
    showSubTables(egNames, egtbType);
}

void EgtbGenDb::showSubTables(const std::vector<std::string>& egNames, EgtbType)
{
    u64 total = 0, largestSz = 0;
    std::string largestName;
    auto cnt = 0;
    for(auto && aName : egNames) {
        assert(std::count(aName.begin(), aName.end(), 'k') == 2);

        cnt++;
        auto sz = EgtbFile::computeSize(aName); assert(sz > 0);
        total += sz;
        if (sz > largestSz) {
            largestSz = sz;
            largestName = aName;
        }
        std::cout << std::setw(3) << cnt << ") " << std::setw(16) << aName << "  " << std::setw(28) << GenLib::formatString(sz) << std::endl;
    }

    std::cout << std::setw(0)<< std::endl;
    std::cout << "Total endgames: " << cnt 
    << ", total size: " << GenLib::formatString(total)
    << std::endl;

    if (!largestName.empty()) {
        std::cout << "One of the largest endgames: " << largestName
        << ", size: " << GenLib::formatString(largestSz)
        << std::endl;
    }
}

void EgtbGenDb::showIntestingSubTables(EgtbType egtbType)
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

void EgtbGenDb::createStatsFiles()
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

std::vector<std::string> EgtbGenDb::showMissing(const std::string& startName) const {
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

bool EgtbGenDb::compress(std::string folder, const std::string& endgameName, bool includingSubEndgames, bool compress) {
    std::cout  << "Start " << (compress ? "compressing" : "uncompressing") << " " << endgameName << std::endl;

    std::vector<std::string> vec = parseName(endgameName);

    if (!folder.empty()) {
        GenLib::createFolder(folder);
        folder += STRING_PATH_SLASH;
    }
    
    auto writtenfolder = folder + "tmp";
    GenLib::createFolder(writtenfolder);
    writtenfolder += STRING_PATH_SLASH;

//    auto compressMode = compress ? CompressMode::compress : CompressMode::compress_none;
    
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
//            auto egtbFile = (EgtbGenFile*) nameMap[endgameName];
            
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


bool EgtbGenDb::compressEndgame(EgtbGenFile* egtbFile, std::string writtenfolder, CompressMode compressMode) {
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
void EgtbGenDb::verifyData(const std::vector<std::string>& nameVec)
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
            removeAllProbedBuffers();
        } else {
            std::cerr << "Error: missing " << endgameName << std::endl;
            missing++;
        }
    }
    std::cout << "Verify COMPLETED. total: " << count << ", passed: " << succ << ", missing: " << missing << std::endl;
}


bool EgtbGenDb::verifyData_chunk(int threadIdx, EgtbFile* pEgtbFile) {
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
            if (egtbVerbose) {
                auto elapsed = (Funcs::now() - time_start_verify) / 1000.0;
                if (elapsed <= 0) elapsed = 1;

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

                for (auto && move : board.gen(side)) {
                    Hist hist;
                    board.make(move, hist);

                    if (!board.isIncheck(side)) {
                        legalCount++;
                        
                        /// If the move is a capture or a promotion, it should probe from sub-endgames
                        auto internal = hist.cap.isEmpty() && move.promotion == PieceType::empty;

                        int score;
                        
                        if (internal) {     /// score from current working buffers
                            auto r = pEgtbFile->getKey(board);
                            auto xs = r.flipSide ? side : xside;
                            score = egtbFile->getScore(r.key, xs, false);
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
#ifdef _FELICITY_XQ_
                        else if (score == EGTB_SCORE_PERPETUATION_WIN) {
                            // EGTB_SCORE_PERPETUATION_LOSE;
                            if (bestScore == EGTB_SCORE_UNSET || bestScore < EGTB_SCORE_DRAW) {
                                bestScore = EGTB_SCORE_PERPETUATION_LOSE;
                            }
                        } else if (score == EGTB_SCORE_PERPETUATION_LOSE) {
                            //score = EGTB_SCORE_PERPETUATION_WIN;
                            if (bestScore <= EGTB_SCORE_DRAW) {
                                bestScore = EGTB_SCORE_PERPETUATION_WIN;
                            }
                        }
#endif
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
//                    verifyDataOK = false;

                    std::lock_guard<std::mutex> thelock(printMutex);
                    std::cerr << "Verify FAILED " << threadIdx << ") " << pEgtbFile->getName() << ", idx: " << idx << ", sd: " << sd
                    << ", data score: " << curScore[sd] << ", relative score: " << bestScore
                    << std::endl;
                    if (b) {
                        board.printOut();
                    }
                    
                    /// Debugging
                    {
                        Hist hist;
                        for (auto&& move : board.gen(side)) {
                            board.make(move, hist);

                            if (!board.isIncheck(side)) {
                                /// If the move is a capture or a promotion, it should probe from sub-endgames
                                auto internal = hist.cap.isEmpty() && move.promotion == PieceType::empty;

                                int score;
                                i64 idx2 = -1;

                                if (internal) {     /// score from current working buffers
                                    idx2 = pEgtbFile->getKey(board).key;
                                    score = pEgtbFile->getScore(idx2, xside, false);
                                }
                                else if (!board.hasAttackers()) {
                                    score = EGTB_SCORE_DRAW;
                                }
                                else {            /// probe from a sub-endgame
                                    score = getScore(board, xside);
                                }

                                std::string msg = "move: " + board.toString_coordinate(move) + ", score: " + std::to_string(score) + ", idx2: " + std::to_string(idx2);
                                board.printOut(msg);
                            }
                            board.takeBack(hist);
                        }

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

bool EgtbGenDb::verifyData(EgtbFile* egtbFile)
{
    assert(egtbFile);
    
    time_start_verify = Funcs::now();

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
            threadVec.push_back(std::thread(&EgtbGenDb::verifyData_chunk, this, i, egtbFile));
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

bool EgtbGenDb::verifyKeys(const std::string& name, EgtbType egtbType) const {
    std::cout << "TbKey::verify STARTED for " << name << std::endl;
    
    EgtbGenFile egtbFile;
    egtbFile.create(name, egtbType);
    return egtbFile.verifyKeys();
}

void EgtbGenDb::verifyKeys(const std::vector<std::string>& endgameNames) const
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

bool EgtbGenDb::compare(EgtbFile* egtbFile0, EgtbFile* egtbFile1) const {
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


void EgtbGenDb::compare(EgtbGenDb& otherEgtbGenFileMng, std::string endgameName, bool includingSubEndgames) {
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

