//
//  EgtbGenFileMng.cpp
//
//  Created by TonyPham
//

#include "EgtbGenFileMng.h"

using namespace egtb;

bool twoBytes = false; // per item
bool useBackward = true;
bool useTempFiles = true;
//bool optimizedZip = false;
i64 maxEndgameSize = -1;


////////////////////////////////////////////
void EgtbGenFileMng::genSingleEgtb_init(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx);
    ExtBoard board;
    
    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        
        if (egtbVerbose && (idx - rcd.fromIdx) % (16 * 1024 * 1024) == 0) {
            std::lock_guard<std::mutex> thelock(printMutex);
            std::cout << "init, threadIdx = " << threadIdx << ", idx = " << idx << ", toIdx = " << rcd.toIdx << ", " << (idx - rcd.fromIdx) * 100 / (rcd.toIdx - rcd.fromIdx) << "%" << std::endl;
        }
        
        if (!setup(egtbFile, board, idx, FlipMode::none, Side::white)) {
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, 0);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, 1);
            continue;
        }
        
        assert(board.isValid());

        bool inchecks[] = { board.isIncheck(Side::black), board.isIncheck(Side::white) };
        int scores[2];
        
        for (int sd = 0; sd < 2; sd++) {
            auto xsd = 1 - sd;
            if (inchecks[xsd]) {
                scores[sd] = EGTB_SCORE_ILLEGAL;
            } else {
                MoveList moveList;
                board.genLegalOnly(moveList, static_cast<Side>(sd));
                scores[sd] = moveList.isEmpty() ? -EGTB_SCORE_MATE : EGTB_SCORE_UNSET; // inchecks[sd] && egtbFile->isBothArmed() ? EGTB_SCORE_CHECKED : EGTB_SCORE_UNSET;
            }
        }
        
        for (int sd = 0; sd < 2; sd++) {
            bool r = egtbFile->setBufScore(idx, scores[sd], sd); assert(r);
            assert(scores[sd] == egtbFile->getScore(idx, static_cast<Side>(sd)));
        }
    }
    
    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "EgtbGenFileMng::genSingleEgtb_init done, threadIdx: " << threadIdx << std::endl;
    }
}


int EgtbGenFileMng::probe_gen(i64 idx, Side side, int ply, int oldScore) {
    ExtBoard board;
    bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white);
    assert(ok);
    
    int legalCount = 0, unsetCount = 0;
    int bestScore = EGTB_SCORE_UNSET;
    Side xside = getXSide(side);

    Hist hist;
    MoveList moveList;
    board.gen(moveList, side, false); assert(!moveList.isEmpty());
    
    for(int i = 0; i < moveList.end; i++) {
        auto m = moveList.list[i];

        board.make(m, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            bool internal = hist.cap.isEmpty();
//            if (internal && egtbFile->getSubPawnRank() >= 0 && m.type == PieceType::pawn && abs(m.from - m.dest) == 9) {
//                internal = egtbFile->getSubPawnRank() == EgtbPawnFiles::getSubRank((const int*)board.pieceList);
//            }

            int score;
            if (internal) {
                auto idx = egtbFile->getKey(board).key;
                score = egtbFile->getScore(idx, xside, false);

//                if (oldScore >= EGTB_SCORE_CHECKED && ply < 4 && score > EGTB_SCORE_MATE) {
//                    auto xsd = static_cast<int>(xside);
//                    if ((oldScore == EGTB_SCORE_CHECKED || oldScore == EGTB_SCORE_PERPETUAL_CHECKED) && (score == EGTB_SCORE_UNSET || score == EGTB_SCORE_EVASION)) {
//                        egtbFile->setBufScore(idx, oldScore == EGTB_SCORE_CHECKED ? EGTB_SCORE_EVASION : EGTB_SCORE_PERPETUAL_EVASION, xsd);
//                    } else if (oldScore == EGTB_SCORE_EVASION && score == EGTB_SCORE_CHECKED) {
//                        egtbFile->setBufScore(idx, EGTB_SCORE_PERPETUAL_CHECKED, xsd);
//                    }
//                }
            } else {
                if (board.pieceList_countStrong() == 0) {
                    score = EGTB_SCORE_DRAW;
                } else {
                    score = getScore(board, xside);

                    if (score == EGTB_SCORE_MISSING) {
                        board.printOut("Error: missing data for probing the board:");
                        exit(-1);
                    }
                    assert(score != EGTB_SCORE_MISSING);
                }
            }
            if (score <= EGTB_SCORE_MATE) {
                score = -score;
                if (score != EGTB_SCORE_DRAW) {
                    if (score > 0) score--;
                    else if (score < 0) score++;
                }

                bestScore = bestScore > EGTB_SCORE_MATE ? score : std::max(bestScore, score);
            } else {
                assert(score != EGTB_SCORE_MISSING);
                unsetCount++;
            }
        }
        board.takeBack(hist);
    }

    assert(legalCount > 0); assert(bestScore != EGTB_SCORE_ILLEGAL);
    return unsetCount == 0 || bestScore > EGTB_SCORE_DRAW || (bestScore == EGTB_SCORE_DRAW && side == Side::black && !egtbFile->isBothArmed()) ? bestScore : EGTB_SCORE_UNSET;
}

void EgtbGenFileMng::genSingleEgtb_loop(int threadIdx, int sd, int ply) {
    auto& rcd = threadRecordVec.at(threadIdx);

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tLoop, threadIdx: " << threadIdx << ", start ply: " << ply << std::endl;
    }

    auto side = static_cast<Side>(sd);

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        auto oldScore = egtbFile->getScore(idx, side, false);
        
//        assert(ply < 6 || oldScore != EGTB_SCORE_CHECKED || oldScore != EGTB_SCORE_EVASION);
        if (abs(oldScore) >= EGTB_SCORE_MATE - 1 - ply && oldScore < EGTB_SCORE_UNSET) { //} != EGTB_SCORE_UNSET) {
            continue;
        }

        auto bestScore = probe_gen(idx, side, ply, oldScore);

        assert(bestScore >= -EGTB_SCORE_MATE);
        if (bestScore != oldScore && bestScore <= EGTB_SCORE_MATE) { //} || (bestScore >= EGTB_SCORE_CHECKED))) {
            assert(bestScore != EGTB_SCORE_WINNING && bestScore != EGTB_SCORE_UNKNOWN);
            auto saveok = egtbFile->setBufScore(idx, bestScore, sd);
            assert(saveok);
            rcd.changes++;
//        } else if ((oldScore == EGTB_SCORE_CHECKED || oldScore == EGTB_SCORE_EVASION) && ply >= 4) {
//            egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, sd);
        }
    }

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tLoop, threadIdx: " << threadIdx << ", completed ply: " << ply << std::endl;
    }
}

void EgtbGenFileMng::genSingleEgtb_main(const std::string& folder) {
    if (egtbVerbose) {
        std::cout << "\tGenerate forwardly!" << std::endl;
    }

    int ply = 0, sd = W;
    
    // Load temp files
    int wLoop = 0, bLoop = 0;
    if (useTempFiles) {
        wLoop = egtbFile->readFromTmpFile(folder, W);
        bLoop = egtbFile->readFromTmpFile(folder, B);
    }
    
    if (wLoop > 0 && bLoop > 0) {
        ply = std::max(wLoop, bLoop);
        sd = wLoop > bLoop ? W : B;
        
        ply++; // next loop
        sd = 1 - sd;
        
        if (egtbVerbose) {
            std::cout << "\tLoaded temp file, start from ply: " << ply << ", sd: " << sd << std::endl;
        }
    }
    
    // Init loop
    if (ply == 0) {
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::genSingleEgtb_init, this, i));
        }
        
        genSingleEgtb_init(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }

    egtbFile->printPerpetuationStats("FORWARD: genSingleEgtb after initializing:");

    // Main loops
    i64 totalChangeCnt = 0;
    
    for(int tryCnt = 2; tryCnt > 0; ply++, sd = 1 - sd) {
        auto startLoop = time(NULL);

        resetAllThreadRecordCounters();
        
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::genSingleEgtb_loop, this, i, sd, ply));
        }
        genSingleEgtb_loop(0, sd, ply);
        
        for (auto && t : threadVec) {
            t.join();
        }

        auto callLoopChangeCnt = allThreadChangeCount();
        
        totalChangeCnt += callLoopChangeCnt;
        
        if (egtbVerbose) {
            int elapsed_secs = std::max(1, (int)(time(NULL) - startLoop));
            std::cout << "\tChanged: " << Lib::formatString(callLoopChangeCnt) << ", elapsed: " << Lib::formatPeriod(elapsed_secs) << " (" << elapsed_secs << " s), speed: " << Lib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
        }
        
        if (useTempFiles && ply > 0 && callLoopChangeCnt > 0) {
            egtbFile->writeTmpFile(folder, sd, ply);
        }
        
        if (callLoopChangeCnt == 0) {
            tryCnt--;
        } else {
            tryCnt = 2;
        }
    }

    egtbFile->printPerpetuationStats("genSingleEgtb after main loops:");
}

bool EgtbGenFileMng::genSingleEgtb(const std::string& folder, const std::string& name, EgtbType egtbType, EgtbProduct egtbProduct, CompressMode compressMode) {
    
    startTime = time(NULL);

    assert(egtbFile == nullptr);
    egtbFile = new EgtbFileWritting();
    egtbFile->create(name, egtbType, egtbProduct);

    auto sz = egtbFile->getSize();
    auto bufsz = sz * 2;
    if (egtbFile->isTwoBytes()) bufsz += bufsz;
    if (useBackward) bufsz += sz / 2;

    std::cout << std::endl << "Start generating " << name << ", " << Lib::formatString(sz) << (egtbFile->isTwoBytes() ? ", 2b/item" : "") << ", main mem sz: " << Lib::formatString(bufsz) << ", " << Lib::currentTimeDate() << std::endl;

    egtbFile->createBuffersForGenerating();
    assert(egtbFile->isValidHeader());
    
//    std::cout << std::endl << "Start generating " << name << ", " << Lib::formatString(egtbFile->getSize()) << ", " << Lib::currentTimeDate() << std::endl;
    setupThreadRecords(egtbFile->getSize());

    if (useBackward) {
        genSingleEgtb_backward_main(folder);
    } else {
        genSingleEgtb_main(folder);
    }

    auto r = genSingleEgtb_finish(folder, compressMode);
    if (r) {
        std::cout << "Generated successfully " << name << std::endl << std::endl;
    }
    return r;

}

bool EgtbGenFileMng::genSingleEgtb_finish(const std::string& folder, CompressMode compressMode, bool needVerify)
{
    perpetuationFix_finish();

    assert(egtbFile->isValidHeader());
    egtbFile->getHeader()->addSide(Side::white);
    egtbFile->getHeader()->addSide(Side::black);

    if (egtbVerbose) {
        int elapsed_secs = std::max(1, (int)(time(NULL) - startTime));
        std::cout << "Completed generating " << egtbFile->getName() << ", elapse: " << Lib::formatPeriod(elapsed_secs) << ", speed: " << Lib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
    }

    addEgtbFile(egtbFile);

    egtbFile->printPerpetuationStats("genSingleEgtb_finish:");

    // Verify
    if (needVerify && !verifyData(egtbFile)) {
        std::cerr << "Error: verify FAILED for " << egtbFile->getName() << std::endl;
        exit(-1);
        return false;
    }
    
    egtbFile->checkAndConvert2bytesTo1();

    if (egtbFile->saveFile(folder, EgtbProduct::std, compressMode)) {
        egtbFile->createStatsFile();

        egtbFile->removeTmpFiles(folder);
        egtbFile = nullptr;
        return true;
    }

    std::cerr << "Error: Generator ended UNSUCCESSFULLY " << egtbFile->getName() << std::endl;
    exit(-1);
    return false;
}

bool EgtbGenFileMng::genEgtb(std::string folder, const std::string& name, EgtbType egtbType, EgtbProduct fileFor, CompressMode compressMode) {
    std::cout << "Missing endgames, they will be generated:\n";

    std::vector<std::string> vec = showMissing(name);

    if (!folder.empty()) {
        Lib::createFolder(folder);
        folder += "/";
    }

    for(auto && aName : vec) {
        if (nameMap.find(aName) != nameMap.end()) {
//            if (nameMap[aName]->getSubPawnRank() < 0) {
                continue;
//            }
        }

        SubfolderParser subfolder(aName);
        subfolder.createAllSubfolders(folder);
        auto writtingFolder = folder + subfolder.subfolder;
        
        auto sz = EgtbFile::computeSize(aName);

        if (maxEndgameSize > 0 && sz > maxEndgameSize) {
            std::cout << aName << " is larger than maxsize limit (" << (maxEndgameSize >> 30) << " G). Ignored!" << std::endl;
            continue;
        }

//        if (sz > 7LL * 1024 * 1024 * 1024 && aName.find("ppp") != std::string::npos) {
//            for(int i = 0; i < 7; i++) {
//                auto str = aName.substr(0, 4) + Lib::itoa(i) + aName.substr(4);
//
//                const std::string noCompPath = EgtbFileWritting::createFileName(folder, str, egtbType, Side::white, false);
//                const std::string compPath = EgtbFileWritting::createFileName(folder, str, egtbType, Side::white, true);
//                if (Lib::existFile(noCompPath.c_str()) || Lib::existFile(compPath.c_str())) {
//                    continue;
//                }
//
//                if (!genSingleEgtb(folder, str, egtbType, compressMode)) {
//                    std::cerr << "ERROR generating " << aName << std::endl;
//                    break;
//                }
//            }
//        } else
        if (!genSingleEgtb(writtingFolder, aName, egtbType, fileFor, compressMode)) {
            std::cerr << "ERROR: generating UNSUCCESSFULLY " << aName << std::endl;
            exit(-1);
            break;
        }
        removeAllBuffers();
    }

    std::cout << "Generating ENDED for " << name << std::endl;
    return true;
}


