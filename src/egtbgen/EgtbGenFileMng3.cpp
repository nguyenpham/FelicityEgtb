//
//  EgtbGenFileMng3.cpp
//
//  Created by TonyPham on 8/6/17.
//

#include <sys/stat.h>
#include <time.h>
#include <iomanip>
#include <thread>
#include <set>

#include "EgtbGenFileMng.h"

using namespace egtb;

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
    
    auto isNewdtm = aName.find("m") != std::string::npos;
    
    for(int sd = 0; sd < 2; sd++) {
        auto s = ss[sd];
        auto xs = ss[1 - sd];
        assert(std::count(s.begin(), s.end(), 'k') == 1);
        
        for(int i = 1; i < s.length(); i++) {
            std::string ss = s.substr(0, i) + (i + 1 < s.length() ? s.substr(i + 1) : "");
            
            if (!isNewdtm || ss.find("m") != std::string::npos)
                createSubName(ss + xs, allNameMap);
            if (!isNewdtm || xs.find("m") != std::string::npos)
                createSubName(xs + ss, allNameMap);
        }
    }
}

std::vector<std::string> EgtbGenFileMng::parseName(const std::string& name, bool includeSubs)
{
    std::vector<std::string> resultVec;
    
    if (includeSubs) {
        std::map<std::string, NameRecord> allNameMap;
        createSubName(name, allNameMap);
        
        std::vector<NameRecord> recordVec;
        
        for(auto && x : allNameMap) {
            recordVec.push_back(x.second);
        }

        sort(recordVec.begin(), recordVec.end(), [ ]( const NameRecord& left, const NameRecord& right) {
            return left.isMeSmaller(right);
        });
        
        for(auto && x : recordVec) {
            resultVec.push_back(x.name);
        }
        
    } else {
        
        NameRecord record(name);
        if (record.isValid()) {
            resultVec.push_back(name);
        }
    }
    
    
    return resultVec;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenFileMng::showSubTables(const std::string& name, EgtbType egtbType) {
    std::cout << "All sub endgames for " << name << " (order - name - index size)" << std::endl;
    std::vector<std::string> vec = parseName(name);
    
    i64 total = 0;
    int cnt = 0;
    for(auto && aName : vec) {
        assert(std::count(aName.begin(), aName.end(), 'k') == 2);

        cnt++;
        auto sz = EgtbFile::computeSize(aName);
        total += sz;
        std::cout << std::setw(3) << cnt << ") " << std::setw(16) << aName << "  " << std::setw(15) << Lib::formatString(sz) << std::endl;
    }

    std::cout << std::setw(0)<< std::endl;
    std::cout << "Total files: " << cnt << ", total size: " << Lib::formatString(total) << std::endl;
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
        
        std::cout << std::setw(3) << cnt << ") " << std::setw(16) << aName << "  " << std::setw(15) << Lib::formatString(sz) << std::endl;
        
        resultVec.push_back(aName);
    }
    
    std::cout << "#missing: " << cnt << ", total sz: " << Lib::formatString(total) << "\n\n";
    return resultVec;
}


void EgtbGenFileMng::convert() {
}

void EgtbGenFileMng::convertScores() {
    std::cout << "EgtbGenFileMng::convertScores BEGIN\n";
}

void EgtbGenFileMng::verifyConvertScore() {
    
}


/////////////////////////

bool EgtbGenFileMng::compress(std::string folder, const std::string& endgameName, bool includingSubEndgames, bool compress) {
    std::cout  << "Start " << (compress ? "compressing" : "uncompressing") << " " << endgameName << std::endl;

    std::vector<std::string> vec = parseName(endgameName);

    if (!folder.empty()) {
        Lib::createFolder(folder);
        folder += "/";
    }
    
    auto writtenfolder = folder + "tmp";
    Lib::createFolder(writtenfolder);
    writtenfolder += "/";

    auto compressMode = compress ? CompressMode::compress : CompressMode::compress_none;
    
    int count = 0, succ = 0;
    if (includingSubEndgames) {
        SubfolderParser parser0(endgameName);
        auto attackingCnt = parser0.attackingCnt;
        
        for(auto && aName : vec) {
            SubfolderParser parser(aName);
            if (attackingCnt != parser.attackingCnt) {
                continue;
            }
            
            count++;
            
            if (nameMap.find(aName) == nameMap.end()) {
                continue;
            }
            
            auto egtbFile = (EgtbFileWritting*) nameMap[aName];
            
            parser.createAllSubfolders(writtenfolder);
            std::string wFolder = writtenfolder + parser.subfolder;
            
            if (compressEndgame(egtbFile, writtenfolder, compressMode)) {
                succ++;
            }
            
        }
    } else {
        count++;
        
        if (nameMap.find(endgameName) != nameMap.end()) {
            auto egtbFile = (EgtbFileWritting*) nameMap[endgameName];
            
            SubfolderParser parser(endgameName);
            parser.createAllSubfolders(writtenfolder);
            std::string wFolder = writtenfolder + parser.subfolder;
            
            if (compressEndgame(egtbFile, writtenfolder, compressMode)) {
                succ++;
            }
        }
    }
    
    std::cout << "Compress/uncompress ENDED " << endgameName << ", total: " << count << ", successfully: " << succ << std::endl;
    return true;
}



bool EgtbGenFileMng::compressEndgame(EgtbFileWritting* egtbFile, std::string writtenfolder, CompressMode compressMode) {
    auto aName = egtbFile->getName();
    std::cout  << "\t" << aName << std::endl;
    
    
    SubfolderParser parser(aName);
    parser.createAllSubfolders(writtenfolder);
    std::string wFolder = writtenfolder + parser.subfolder;
    
    
//    if (egtbFile->getSubPawnRank() >= 0) {
//        EgtbPawnFiles* egtbPawnFiles = (EgtbPawnFiles*)egtbFile;
//        for(int i = 0; i< 7; i++) {
//            if (egtbPawnFiles->subFiles[i]) {
//                auto p = (EgtbFileWritting*)egtbPawnFiles->subFiles[i];
//                if (!EgtbFileWritting::existFileName(wFolder, p->getName(), p->getEgtbType(), Side::none, compressMode != CompressMode::compress_none)) {
//                    // Load data to buf
//                    for(int sd = 0; sd < 2; ++sd) {
//                        //                            i64 idx = 0;
//                        auto side = static_cast<Side>(sd);
//                        if (p->forceLoadHeaderAndTable(side)) {
//                            p->saveFile(wFolder, sd, compressMode);
//                            p->removeBuffers();
//                            if (egtbVerbose) {
//                                std::cout << "Successfully create " << p->getPath(sd) << std::endl;
//                            }
//                        } else {
//                            std::cerr << "\n******Error: Cannot read " << p->getPath(sd) << std::endl << std::endl << std::endl;
//                            return false;
//                        }
//                        //                            p->getScore(idx, Side::black, false);
//                    }
//                }
//            }
//        }
//    } else {
        if (!EgtbFileWritting::existFileName(wFolder, aName, egtbFile->getEgtbType(), Side::none, compressMode != CompressMode::compress_none)) {
            
            for(int sd = 0; sd < 2; ++sd) {
                auto side = static_cast<Side>(sd);
                if (egtbFile->forceLoadHeaderAndTable(side)) {
                    egtbFile->saveFile(wFolder, sd, EgtbProduct::std, compressMode);
                    egtbFile->removeBuffers();
                    if (egtbVerbose) {
                        std::cout << "\t\tSuccessfully save " << egtbFile->getPath(sd) << std::endl;
                    }
                } else {
                    std::cerr << "\n******Error: Cannot read " << egtbFile->getPath(sd) << std::endl << std::endl << std::endl;
                    return false;
                }
            }
            
        }
//    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenFileMng::verifyData(const std::string& endgameName, bool includingSubEndgames) {
    std::cout << "Found total endgames: " << getEgtbFileVec().size() << std::endl;
    std::vector<std::string> vec = parseName(endgameName);
    
    std::cout << "\nVerify " << endgameName;
    
    if (includingSubEndgames) {
        std::cout << " and all sub-endgames, #" << vec.size();
    }
    
    std::cout << std::endl << std::endl;
    
    int count = 0, succ = 0, missing = 0;
    if (includingSubEndgames) {
        SubfolderParser parser(endgameName);
        auto attackingCnt = parser.attackingCnt;

        for(auto && aName : vec) {
            SubfolderParser parser(aName);
            if (attackingCnt != parser.attackingCnt) {
                continue;
            }
            
            auto egtbFile = nameMap[aName];
            if (!egtbFile) {
                std::cerr << "Error: missing " << aName << std::endl;
                missing++;
            }
        }

        for(auto && aName : vec) {
            SubfolderParser parser(aName);
            if (attackingCnt != parser.attackingCnt) {
                continue;
            }
            
            auto egtbFile = (EgtbFileWritting*)nameMap[aName];
            if (egtbFile) {
                count++;
                if (verifyData(egtbFile)) {
                    succ++;
                }
                removeAllBuffers();
            }
        }
    } else {
        auto egtbFile = (EgtbFileWritting*)nameMap[endgameName];
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

static bool verifyDataOK = true;

bool EgtbGenFileMng::verifyData_chunk(int threadIdx, EgtbFile* pEgtbFile) {
    assert(pEgtbFile);
    auto& rcd = threadRecordVec.at(threadIdx);

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tverify " << pEgtbFile->getName() << ", started by thread " << threadIdx << ") range: " << rcd.fromIdx << " -> " << rcd.toIdx << std::endl;
    }

    MoveList moveList;
    ExtBoard board;
    bool scoreRangeOK = true;

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx && verifyDataOK && scoreRangeOK; idx ++) {
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
                std::cout << "\tverify threadIdx = " << threadIdx << ", idx = " << Lib::formatString(idx) << " of " << rcd.toIdx << " " << k * 100 / (rcd.toIdx - rcd.fromIdx) << "%, speed: " << Lib::formatSpeed((int)(k / elapsed)) << std::endl;
            }
        }

        auto b = setup(pEgtbFile, board, idx, FlipMode::none, Side::white);
        if (!b) {
            if (curScore[0] != EGTB_SCORE_ILLEGAL || curScore[1] != EGTB_SCORE_ILLEGAL) {
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cerr << "Error: cannot create a board even scores are not illegal. idx: " << idx << " scores: " << curScore[0] << ", " << curScore[1] << std::endl;
                board.show();
                //                exit(-1);
                verifyDataOK = false;
                return false;
            }
            continue;
        }
        
//        auto debugging = idx == 21780507;
//        if (debugging) {
//            board.printOut("Starting, idx: " + Lib::itoa(idx));
//        }

        for (int sd = 0; sd < 2; sd ++) {
//            noLossCnt = permanentCheckCnt = permanentEvacuateCnt = 0;
            int noLossCnt = 0, permanentCheckCnt = 0, permanentEvacuateCnt = 0, numerisePerpetualWinCnt = 0, numerisePerpetualLostCnt = 0;
            Side side = static_cast<Side>(sd), xside = getXSide(side);
            auto bestScore = EGTB_SCORE_UNSET, bestPerpetualScore = -EGTB_SCORE_PERPETUAL_MATE;
            
            if (board.isIncheck(xside)) {
                bestScore = EGTB_SCORE_ILLEGAL;
            } else {
                if ((curScore[sd] > EGTB_SCORE_MATE && curScore[sd] != EGTB_SCORE_ILLEGAL && curScore[sd] < EGTB_SCORE_PERPETUAL_CHECKED) ||
                    (curScore[sd] <= EGTB_SCORE_MATE && ((curScore[sd] > 0 && (curScore[sd] & 1) == 0) || (curScore[sd] < 0 && (-curScore[sd] & 1) != 0)))) {
                    //                if (curScore[sd] > EGTB_SCORE_MATE || (curScore[sd] > 0 && (curScore[sd] & 1) == 0)) {
                    verifyDataOK = false;
                    scoreRangeOK = false;
                    std::lock_guard<std::mutex> thelock(printMutex);
                    printf("Error: score incorrect odd/even rules, idx=%lld, sd=%d, curScore[sd]=%d\n", idx, sd, curScore[sd]);
                    board.show();
                    //                    exit(-1);
                    return false;
                }

                bestScore = -EGTB_SCORE_MATE;
                board.gen(moveList, side, false);
                int legalCount = 0;

                for (int i = 0; i < moveList.end; i++) {
                    auto move = moveList.list[i];
                    Hist hist;
                    board.make(move, hist);

                    if (!board.isIncheck(side)) {
                        legalCount++;
                        int score;
                        
                        i64 sIdx = -1;
                        if (hist.cap.isEmpty()) {
                            sIdx = pEgtbFile->getKey(board).key;
                            score = pEgtbFile->getScore(sIdx, xside, false);
                        } else if (board.pieceList_countStrong() == 0) {
                            score = EGTB_SCORE_DRAW;
                        } else {
                            score = getScore(board, xside, AcceptScore::winning);
                        }
                        
//                        if (debugging) {
//                            board.printOut("verifyData_chunk after move: " + move.toString() + ", sd: " + Lib::itoa(sd) + ", score: " + Lib::itoa(score) + ", sIdx: " + Lib::itoa(sIdx));
//                        }

                        if (abs(score) >= EGTB_SCORE_PERPETUAL_BEGIN) {
                            bestPerpetualScore = std::max(bestPerpetualScore, score);
                            if (score > 0) numerisePerpetualWinCnt++;
                            else numerisePerpetualLostCnt++;
                        } else  if (score <= EGTB_SCORE_MATE) {
                            score = -score;
                            if (score > bestScore) {
                                bestScore = score;
                            }

                            if (score >= EGTB_SCORE_DRAW) {
                                noLossCnt++;
                            }
                        } else if (score >= EGTB_SCORE_PERPETUAL_CHECKED) {
                            if (score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) {
                                permanentCheckCnt++;
                                permanentEvacuateCnt++;
                            } else if (score == EGTB_SCORE_PERPETUAL_CHECKED) {
                                permanentCheckCnt++;
                            } else {
                                permanentEvacuateCnt++;
                            }
                        }
                    }
                    board.takeBack(hist);
                }

                if (legalCount == 0) bestScore = -EGTB_SCORE_MATE;
                else {
                    if (abs(bestScore) <= EGTB_SCORE_MATE && bestScore != EGTB_SCORE_DRAW) {
                        if (bestScore > 0) bestScore--;
                        else bestScore++;
                        
                        if (bestScore == EGTB_SCORE_DRAW || abs(bestScore) >= EGTB_SCORE_MATE) {
                            scoreRangeOK = false;
                        }
                    }
                    
                    if (scoreRangeOK && bestScore != EGTB_SCORE_DRAW) {
                        if (bestScore > EGTB_SCORE_MATE && bestScore != EGTB_SCORE_ILLEGAL) {
                            scoreRangeOK = false;
                        }
                    }
                }
            }
            
            if (verifyDataOK) {
                bool ok;
                if (abs(curScore[sd]) >= EGTB_SCORE_PERPETUAL_BEGIN) {
                    ok = true; //numerisePerpetualWinCnt + numerisePerpetualLostCnt > 0 || ;
                } else if (curScore[sd] < EGTB_SCORE_PERPETUAL_CHECKED) {
                    ok = scoreRangeOK && (curScore[sd] == bestScore || (bestScore > EGTB_SCORE_MATE && curScore[sd] > EGTB_SCORE_MATE));
                    
                    if (!ok) {
                        if (permanentCheckCnt > 0) {
                            ok = curScore[sd] >= EGTB_SCORE_DRAW;
                        } else {
                            ok = permanentEvacuateCnt > 0;
                        }
                    }
                } else {
                    if (curScore[sd] == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) {
                        ok = permanentCheckCnt > 0 || permanentEvacuateCnt > 0;
                    } else if (curScore[sd] == EGTB_SCORE_PERPETUAL_EVASION) {
                        ok = permanentCheckCnt > 0;
                    } else if (curScore[sd] == EGTB_SCORE_PERPETUAL_CHECKED) {
                        ok = permanentEvacuateCnt > 0;
                    } else {
                        ok = noLossCnt == 0 && permanentCheckCnt > 0;
                    }
//                    if (!ok) {
//                        board.printOut("FAILED, idx: " + Lib::itoa(idx) + ", sd: " + Lib::itoa(sd) + ", curScore[sd]: " + Lib::itoa(curScore[sd]));
//                    }
//                    assert(ok);
                }

                if (!ok) {
                    verifyDataOK = false;

                    std::lock_guard<std::mutex> thelock(printMutex);
                    std::cerr << "Error: verify FAILED " << threadIdx << ") " << pEgtbFile->getName() << ", idx: " << idx << ", sd: " << sd
                    << ", data score: " << curScore[sd] << ", calc score: " << bestScore << ", scoreRange: " << scoreRangeOK
                    << ", permanentCheckCnt: " << permanentCheckCnt << ", permanentEvacuateCnt: " << permanentEvacuateCnt << std::endl;
                    if (b) {
                        board.show();
                    }
                    
                    exit(-1);
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

bool EgtbGenFileMng::verifyData(EgtbFile* egtbFile) {
    assert(egtbFile);
    
    std::cout << "verify " << egtbFile->getName() << " sz: " << Lib::formatString(egtbFile->getSize()) << " at: " << Lib::currentTimeDate() << std::endl;
    
    begin = std::chrono::steady_clock::now();
    
    verifyDataOK = true;
    
//    auto n = MaxGenExtraThreads;
//    MaxGenExtraThreads = 0;
    setupThreadRecords(egtbFile->getSize());
//    MaxGenExtraThreads = n;
    {
        egtbFile->getScore(0LL, Side::black, false);
        if (egtbFile->getLoadStatus() == EgtbLoadStatus::error) {
            return false;
        }

        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
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
    
    EgtbFileWritting egtbFile;
    egtbFile.create(name, egtbType, EgtbProduct::std);
    return egtbFile.verifyKeys();
}


void EgtbGenFileMng::verifyKeys(const std::string& endgameName, bool includingSubEndgames) const {
    std::vector<std::string> vec = parseName(endgameName, includingSubEndgames);
    
    std::cout << "\nVerify keys " << endgameName;
    
    EgtbType egtbType = EgtbType::dtm;
    
    if (includingSubEndgames) {
        std::cout << " and all sub-endgames, #" << vec.size();
    }
    
    std::cout << std::endl << std::endl;
    
    int count = 0, succ = 0, missing = 0;
    if (includingSubEndgames) {
        SubfolderParser parser(endgameName);
        auto attackingCnt = parser.attackingCnt;
        
        for(auto && aName : vec) {
            SubfolderParser parser(aName);
            if (attackingCnt != parser.attackingCnt) {
                continue;
            }
            
            count++;
            EgtbFileWritting egtbFile;
            egtbFile.create(aName, egtbType, EgtbProduct::std);
            assert(egtbFile.getSize() > 0);
            if (egtbFile.verifyKeys()) {
                succ++;
            }
        }
    } else {
        count++;
        EgtbFileWritting egtbFile;
        egtbFile.create(endgameName, egtbType, EgtbProduct::std);
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
        for(int sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto score0 = egtbFile0->getScore(idx, side);
            auto score1 = egtbFile1->getScore(idx, side);
            if (score0 != score1) {
                std::cerr << "Endgames " << egtbFile0->getName() << " are not matched at idx "
                << idx << " " << (sd == B ? "black" : "white")
                << ", scores " << score0 << ", " << score1
                << std::endl;
                
                ExtBoard board;
                if (setup(egtbFile0, board, idx, FlipMode::none, Side::white)) {
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
    std::cout << "Found total endgames: " << getEgtbFileVec().size() << std::endl;
    std::vector<std::string> vec = parseName(endgameName, includingSubEndgames);
    
    std::cout << "\nCompare " << endgameName;
    
    if (includingSubEndgames) {
        std::cout << " and all sub-endgames, #" << vec.size();
    }
    
    std::cout << std::endl << std::endl;
    
    int count = 0, succ = 0, err = 0;
    if (includingSubEndgames) {
        SubfolderParser parser(endgameName);
        auto attackingCnt = parser.attackingCnt;
        
        for(auto && aName : vec) {
            SubfolderParser parser(aName);
            if (attackingCnt != parser.attackingCnt) {
                continue;
            }
            
            auto egtbFile0 = nameMap[aName];
            auto egtbFile1 = otherEgtbGenFileMng.nameMap[aName];
            
            if (!egtbFile0 || !egtbFile1) {
                std::cerr << "Error: missing one or both " << aName << std::endl;
                continue;
            }

            count++;
            if (compare(egtbFile0, egtbFile1)) {
                succ++;
            } else {
                std::cerr << "Error: failed " << endgameName << std::endl;
                err++;
            }
        }
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenFileMng::testSpeed(const std::string& name, bool randomIndex) {
    std::srand((unsigned int)std::time(nullptr));
    auto start = std::chrono::steady_clock::now();

    int getScoreCnt = 0;

    auto k = std::count(name.begin(), name.end(), 'k');
    if (k > 0) {
        auto vec = parseName(name);
        for(auto && endgameName : vec) {
            auto egtbFile = nameMap[endgameName];
            if (egtbFile == nullptr) {
                std::cerr << "Error: missing endgame " << endgameName << std::endl;
                exit(-1);
            }
            for(int sd = 0; sd < 2; sd++) {
                if (!egtbFile->getPath(sd).empty()) {
                    i64 idx = 0LL;
                    if (randomIndex) {
                        int val = std::rand();
                        idx = egtbFile->getSize() * val / RAND_MAX;
                    }
                    auto score = egtbFile->getScore(idx, static_cast<Side>(sd));
                    getScoreCnt++;
                }
            }
        }
    } else {
        std::string left = "k", right;
        auto p = name.find('-');
        if (p == std::string::npos) {
            left += name;
        } else {
            left += name.substr(0, p);
            right = name.substr(p + 1);
            if (!right.empty()) {
                right = "k" + right;
            }
        }

        for(auto && egtbFile : egtbFileVec) {
            auto str = egtbFile->getName();
            auto lp = str.find(left);
            if (lp != 0) {
                continue;
            }
            char ch = str.at(left.length());
            if (ch != 'k' && ch != 'a' && ch != 'e' && ch != 'm') {
                continue;
            }
            if (!right.empty()) {
                auto rp = str.rfind(right);
                if (rp == 0 || rp == std::string::npos) {
                    continue;
                }

                auto l = rp + right.length();
                char ch = str.at(l >= str.length() ? 0 : l);
                if (ch != 'k' && ch != 'a' && ch != 'e') {
                    continue;
                }
            }

            for(int sd = 0; sd < 2; sd++) {
                if (!egtbFile->getPath(sd).empty()) {
                    i64 idx = 0LL;
                    if (randomIndex) {
                        int val = std::rand();
                        idx = egtbFile->getSize() * val / RAND_MAX;
                    }
                    auto score = egtbFile->getScore(idx, static_cast<Side>(sd));
                    getScoreCnt++;
                }
            }
        }
    }

    auto elapsed = u64(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()); // / 1000.0;
    std::cout << "Loaded data from #" << getScoreCnt << " files, period: " << elapsed << " ms" << std::endl;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenFileMng::createProduct(EgtbGenFileMng& stdEgtbFileMng, std::string folder, const std::string& name, EgtbProduct egtbProduct, bool forAllSides)
{
    std::vector<std::string> vec = showMissing(name);
    
    if (!folder.empty()) {
        Lib::createFolder(folder);
        folder += "/";
    }
    
    for(auto && aName : vec) {
        if (nameMap.find(aName) != nameMap.end()) {
            continue;
        }
 
        auto egtbFile = (EgtbFileWritting*)stdEgtbFileMng.getEgtbFile(aName);
        if (egtbFile == nullptr) {
            std::cout << "Missing endgame " << aName << std::endl;
            continue;
        }

        SubfolderParser subfolder(aName);
        subfolder.createAllSubfolders(folder);
        auto writtingFolder = folder + subfolder.subfolder;
        
        if (!createProduct(writtingFolder, *egtbFile, egtbProduct, forAllSides)) {
            std::cerr << "ERROR: create product UNSUCCESSFULLY " << aName << std::endl;
            exit(-1);
            break;
        }
        removeAllBuffers();
    }
    
    std::cout << "Generating ENDED for " << name << std::endl;
}

bool EgtbGenFileMng::createProduct(std::string writtingFolder, EgtbFileWritting& egtbFile, EgtbProduct egtbProduct, bool forAllSides)
{
    std::cout << "createProduct " << egtbFile.getName() << std::endl;

    egtbFile.getScore(0LL, Side::black);
    twoBytes = egtbFile.isTwoBytes();
    
    std::vector<int> sideVec;
    if (forAllSides || egtbFile.getPath(0).empty() || egtbFile.getPath(1).empty()) {
        if (!egtbFile.getPath(0).empty()) {
            sideVec.push_back(0);
        }
        if (!egtbFile.getPath(1).empty()) {
            sideVec.push_back(1);
        }
    } else {
        auto sz0 = Lib::getFileSize(egtbFile.getPath(0));
        auto sz1 = Lib::getFileSize(egtbFile.getPath(1));
        
        if (sz0 < sz1) sideVec.push_back(0); else sideVec.push_back(1);
    }
    
    for(auto && sd : sideVec) {
        EgtbFileWritting newEgtbFile;
        newEgtbFile.create(egtbFile.getName(), EgtbType::dtm, egtbProduct);
        newEgtbFile.createBuffersForGenerating();
        auto sz = egtbFile.getSize();
        auto side = static_cast<Side>(sd);
        for(i64 idx = 0; idx < sz; idx++) {
            auto score = egtbFile.getScore(idx, side);
            newEgtbFile.setBufScore(idx, score, sd);
        }
        newEgtbFile.saveFile(writtingFolder, sd, egtbProduct, CompressMode::compress_optimizing);
    }
    
    return !sideVec.empty();
}
