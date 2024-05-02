//
//  EgtbGenFileMng2.cpp
//
//  Created by TonyPham on 30/4/17.
//

#include "EgtbGenFileMng.h"

using namespace egtb;

extern bool useTempFiles;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EgtbGenFileMng::genSingleEgtb_backward_init(int threadIdx) {
    auto record = threadRecordVec.at(threadIdx);

    // Init loop
    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "init. threadIdx: " << threadIdx << ", " << record.fromIdx << "->" << record.toIdx << ", sz: " << egtbFile->getSize() << std::endl;
    }

    ExtBoard board;
    
    for (i64 idx = record.fromIdx; idx < record.toIdx; idx++) {
        if (egtbVerbose && (idx - record.fromIdx) % (16 * 1024 * 1024) == 0) {
            std::lock_guard<std::mutex> thelock(printMutex);
            std::cout << "init, threadIdx = " << threadIdx << ", idx = " << idx << ", toIdx = " << record.toIdx << ", " << (idx - record.fromIdx) * 100 / (record.toIdx - record.fromIdx) << "%" << std::endl;
        }

        if (!setup(egtbFile, board, idx, FlipMode::none, Side::white)) {
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, 0);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, 1);
            continue;
        }

        assert(board.isValid());
        
//        auto db = idx == 23141757;
//        if (db) {
//            board.printOut("Starting");
//        }

        bool inchecks[] = { board.isIncheck(Side::black), board.isIncheck(Side::white) };
        int scores[2];
        
        for (int sd = 0; sd < 2; sd++) {
            auto xsd = 1 - sd;
            if (inchecks[xsd]) {
                scores[sd] = EGTB_SCORE_ILLEGAL;
                continue;
            }

            auto side = static_cast<Side>(sd), xside = getXSide(side);

            Hist hist;
            MoveList moveList;
            board.gen(moveList, side, false);

            auto legalCount = 0, capCnt = 0;
            auto score = EGTB_SCORE_UNSET;
            for(int i = 0; i < moveList.end; i++) {
                auto m = moveList.list[i];

                board.make(m, hist);
                
                if (!board.isIncheck(side)) {
                    legalCount++;

//                    if (db) {
//                        board.printOut("board after move " + m.toString());
//                    }

                    if (!hist.cap.isEmpty()) {
                        capCnt++;
                        int cScore;
                        if (board.pieceList_countStrong() == 0) {
                            cScore = EGTB_SCORE_DRAW;
                        } else {
                            cScore = getScore(board, xside);
                        }
                        
                        if (cScore <= EGTB_SCORE_MATE) {
                            if (cScore != EGTB_SCORE_DRAW) {
                                cScore = -cScore + (cScore < 0 ? -1 : +1);
                            }
                            if (score > EGTB_SCORE_MATE || cScore > score) {
                                score = cScore;
                            }
                        } else if (cScore == EGTB_SCORE_MISSING) {
                            std::lock_guard<std::mutex> thelock(printMutex);
                            board.printOut("Error: missing endagme for probing below board:");
                            exit(-1);
                        }
                    }
                }
                board.takeBack(hist);
            }

            if (!legalCount) {
                scores[sd] = -EGTB_SCORE_MATE;
                continue;
            }
            
            if (score < EGTB_SCORE_MATE) {
                egtbFile->flag_set_cap(idx, sd);
//            } else if (inchecks[sd] && egtbFile->isBothArmed()) {
//                assert(score == EGTB_SCORE_UNSET);
//                score = EGTB_SCORE_CHECKED;
            }
            
            scores[sd] = score;

        } // for sd
        
        for (int sd = 0; sd < 2; sd++) {
            bool r = egtbFile->setBufScore(idx, scores[sd], sd); assert(r);
            assert(scores[sd] == egtbFile->getScore(idx, static_cast<Side>(sd)));
        }

    } // for (i64 idx
}

int EgtbGenFileMng::probe_gen_backward(i64 idx, Side side, int ply) {
    ExtBoard board;
    auto ok = setup(egtbFile, board, idx, FlipMode::none, Side::white);
    assert(ok);

    auto legalCount = 0, unsetCount = 0;
    auto bestScore = EGTB_SCORE_UNSET, capScore = EGTB_SCORE_UNSET;
    auto sd = static_cast<int>(side);
    auto xsd = 1 - sd;
    auto xside = getXSide(side);

    Hist hist;
    MoveList moveList;
    board.gen(moveList, side, false); assert(!moveList.isEmpty());

//    auto debugging = idx == 23141757;
//    if (debugging) {
//        board.printOut("Starting");
//    }

    for(int i = 0; i < moveList.end; i++) {
        auto m = moveList.list[i];

        board.make(m, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            auto internal = hist.cap.isEmpty();
            
            int score;
            if (internal) {
                auto idx2 = egtbFile->getKey(board).key;
                if (egtbFile->flag_is_cap(idx2, xsd)) {
                    score = EGTB_SCORE_UNSET;
                } else {
                    score = egtbFile->getScore(idx2, xside, false);
                }
            } else {
                if (capScore == EGTB_SCORE_UNSET) {
                    if (egtbFile->flag_is_cap(idx, static_cast<int>(side))) {
                        capScore = egtbFile->getScore(idx, side, false);
                        if (capScore > 0) capScore++;
                        else if (capScore < 0) capScore--;
                        capScore = -capScore;
                        assert(abs(capScore) <= EGTB_SCORE_MATE);
                    }
                }
                score = capScore;
            }
            
//            if (debugging) {
//                board.printOut("board after move " + m.toString() + " , score: " + Lib::itoa(score));
//            }

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
    
    if (legalCount == 0) {
        return -EGTB_SCORE_MATE;
    }

    assert(legalCount > 0); assert(bestScore != EGTB_SCORE_ILLEGAL);
    return unsetCount == 0 ? bestScore : EGTB_SCORE_UNSET;
}


bool needFlip(const ExtBoard& board)
{
    for(int sd = 1; sd >= 0; sd--) {
        for(auto i = 5; i < 16;) {
            int n = i < 11 ? 2 : 5;
            bool midCol = false;
            int atkCnt = 0, cols[5];
            
            for(int j = 0; j < n; j++) {
                auto pos = board.pieceList[sd][i + j];
                if (pos >= 0) {
                    auto f = pos % 9;
                    cols[atkCnt++] = f;
                    if (f == 4) midCol = true;
                }
            }
            if (atkCnt > 0) {
                if (atkCnt > 1 && !midCol) {
                    // WARNING: work with 2 pieces only (NOT 3 PAWNS)
                    if (cols[0] > 4) cols[0] = 8 - cols[0];
                    if (cols[1] > 4) cols[1] = 8 - cols[1];
                    midCol = cols[0] == cols[1];
                }
                return midCol;
            }
            i += n;
        }
    }

    for(int sd = 1; sd >= 0; sd--) {
        for(auto i = 0; i < 5; ++i) {
            auto pos = board.pieceList[sd][i];
            if (pos >= 0 && pos % 9 != 4) {
                return true;
            }
        }
    }
    return false;
}

void EgtbGenFileMng::genSingleEgtb_backward_loop(int threadIdx, int sd, int ply, int task) {
    auto& rcd = threadRecordVec.at(threadIdx);

    auto side = static_cast<Side>(sd), xside = getXSide(side);
    auto xsd = 1 - sd;
    
    auto curMate = EGTB_SCORE_MATE - ply;
    if ((ply & 1) == 0)
        curMate = -curMate;

    auto fillScore = -curMate + (curMate > 0 ? +1 : -1);

    MoveList moveList;
    ExtBoard board;

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
//        auto debugging = idx == 23141757 && fillScore == -494;
//        if (debugging) {
//            std::cout << "Iamhere\n";
//        }
        
        if (task == 0) {
            if (egtbVerbose && sd == 0 && (idx - rcd.fromIdx) % (16 * 1024 * 1024) == 0) {
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cout << "loop " << ply << ", threadIdx = " << threadIdx << ", idx = " << idx << ", toIdx = " << rcd.toIdx << ", " << (idx - rcd.fromIdx) * 100 / (rcd.toIdx - rcd.fromIdx) << "%" << std::endl;
            }

            int oScore = egtbFile->getScore(idx, side, false);

            if (egtbFile->flag_is_cap(idx, sd)) {
                if (fillScore == oScore) {
                    if (fillScore > 0) {
                        egtbFile->setBufScore(idx, fillScore, sd);
                        egtbFile->flag_clear_cap(idx, sd);
                    } else {
                        egtbFile->flag_set_side(idx, sd);
                    }
                } else if (curMate == oScore && curMate > 0) {
                    egtbFile->setBufScore(idx, curMate, sd);
                    egtbFile->flag_clear_cap(idx, sd);
                } else
                    continue;
            }

            if (oScore != curMate) {
                continue;
            }

            bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white);
            assert(ok);

            board.genBackward(moveList, xside, false);

//            if (debugging) {
//                board.printOut("Starting");
//            }

            for(int i = 0; i < moveList.end; i++) {
                auto move = moveList.list[i];
                
                Hist hist;
                board.make(move, hist);
                
                assert(hist.cap.isEmpty());
                
                if (!board.isIncheck(side)) {
                    assert(board.isValid());
                    
                    auto rIdx = egtbFile->getKey(board).key;
                    if (rIdx >= 0) {
                        i64 sIdx = -1;
                        
                        if (needFlip(board)) {
                            board.flip(FlipMode::horizontal);
                            sIdx = egtbFile->getKey(board).key;
                            board.flip(FlipMode::horizontal);
                        }
                        
                        if (fillScore > 0) {
                            auto theScore = egtbFile->getScore(rIdx, xside, false);
                            if (theScore > EGTB_SCORE_MATE || theScore <= fillScore) {
                                egtbFile->setBufScore(rIdx, fillScore, xsd);
                                egtbFile->flag_clear_cap(rIdx, xsd);
                                
                                if (sIdx >= 0) {
                                    
                                    theScore = egtbFile->getScore(sIdx, xside, false);
                                    if (theScore > EGTB_SCORE_MATE || theScore < fillScore) {
                                        egtbFile->setBufScore(sIdx, fillScore, xsd);
                                        egtbFile->flag_clear_cap(sIdx, xsd);
                                    }
                                }
                                
                                rcd.changes++;
                            }
                            
                        } else {
                            egtbFile->flag_set_side(rIdx, xsd);
                            
                            if (sIdx >= 0) {
                                egtbFile->flag_set_side(sIdx, xsd);
                            }
                        }
                    }
                }
                
                board.takeBack(hist);
            }
            continue;
        }

        // Task 1 - Losing
        if (!egtbFile->flag_is_side(idx, sd)) {
            continue;
        }

        int oScore = egtbFile->getScore(idx, side, false);
        if (oScore == EGTB_SCORE_ILLEGAL) {
            ExtBoard board;
            bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white);
            board.printOut("Illegal");
            assert(ok);
        }
        
        auto bestScore = probe_gen_backward(idx, side, ply);

        if (bestScore != EGTB_SCORE_UNSET) {
            
            egtbFile->setBufScore(idx, bestScore, sd);
            egtbFile->flag_clear_cap(idx, sd);
            rcd.changes++;
        }
    }
}

void EgtbGenFileMng::genSingleEgtb_backward_main(const std::string& folder) {
    egtbFile->createFlagBuffer();

    int ply = 0, mPly = 0;

    // Init loop
    if (useTempFiles && egtbFile->readFromTmpFiles(folder, ply, mPly)) {
        if (egtbVerbose) {
            std::cout << "Loaded successfully temp files." << std::endl;
        }
    } else {
        {
            std::vector<std::thread> threadVec;
            for (int i = 1; i < threadRecordVec.size(); ++i) {
                threadVec.push_back(std::thread(&EgtbGenFileMng::genSingleEgtb_backward_init, this, i));
            }
            genSingleEgtb_backward_init(0);
            
            for (auto && t : threadVec) {
                t.join();
            }
        }

        removeAllBuffers();
        
        // Calculate ply
        ply = mPly = 0;
        int maxScore = EGTB_SCORE_UNSET, minScore = EGTB_SCORE_UNSET;
        for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            for(auto sd = 0; sd < 2; sd++) {
                auto score = abs(egtbFile->getScore(idx, static_cast<Side>(sd)));
                if (score > EGTB_SCORE_DRAW && score <= EGTB_SCORE_MATE) {
                    minScore = minScore == EGTB_SCORE_UNSET ? score : std::min(minScore, score);
                    maxScore = maxScore == EGTB_SCORE_UNSET ? score : std::max(maxScore, score);
                }
            }
        }
        
        if (maxScore <= EGTB_SCORE_MATE && minScore <= EGTB_SCORE_MATE) {
            auto p0 = EGTB_SCORE_MATE - std::min(maxScore, minScore);
            auto p1 = EGTB_SCORE_MATE - std::max(maxScore, minScore);
            ply = std::min(p0, p1);
            mPly = std::max(p0, p1);
            if (mPly == EGTB_SCORE_MATE) mPly = ply;
        }
        
        assert(ply >= 0 && ply <= mPly && mPly < EGTB_SCORE_MATE);

        if (egtbVerbose) {
            std::cout << "Init successfully." << std::endl;
        }
    }

//    {
//        i64 idx = 23141757;
//        int scores[] = { egtbFile->getScore(idx, Side::black), egtbFile->getScore(idx, Side::white) };
//        std::cout << "scores: " << scores[0] << ", " << scores[1] << std::endl;
//    }
    // Main loops
    i64 totalChangeCnt = 0;
    
    for(int tryCnt = 2; tryCnt > 0; ply++) {
        auto startLoop = time(NULL);
        auto startClockLoop = clock();
        
        resetAllThreadRecordCounters();
        for(i64 idx = 0; idx < egtbFile->getSize() / 2 + 1; idx++)
            egtbFile->flags[idx] &= ~(3 | 3 << 4);
        
        for(int task = 0; task < 2; task++) {
            for(int sd = 0; sd < 2; sd++) {
                std::vector<std::thread> threadVec;
                for (int i = 1; i < threadRecordVec.size(); ++i) {
                    threadVec.push_back(std::thread(&EgtbGenFileMng::genSingleEgtb_backward_loop, this, i, sd, ply, task));
                }
                genSingleEgtb_backward_loop(0, sd, ply, task);
                
                for (auto && t : threadVec) {
                    t.join();
                }

                totalChangeCnt += allThreadChangeCount();
            }
        }

        if (egtbVerbose) {
            std::lock_guard<std::mutex> thelock(printMutex);
            auto periodClock = (clock() - startClockLoop) / CLOCKS_PER_SEC;
            int elapsed_secs = std::max(1, (int)(time(NULL) - startLoop));
            std::cout << "\tply: " << ply << ") changed: " << Lib::formatString(allThreadChangeCount()) << ", elapsed: " << Lib::formatPeriod(elapsed_secs) << " (" << elapsed_secs << ", clk: " << periodClock << "), speed: " << Lib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
        }

        if (allThreadChangeCount() == 0) {
            if (ply > mPly) {
                tryCnt--;
            }
        } else {
            if (useTempFiles && (ply & 7) == 0) {
                egtbFile->writeTmpFiles(folder, ply, ply);
            }
            tryCnt = 2;
        }
    }

    if (egtbVerbose) {
        std::cout << "Completed main loops." << std::endl;
    }

    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
        for(int sd = 0; sd < 2; sd++) {
            if (egtbFile->flag_is_cap(idx, sd)) {
                egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, sd);
            }
        }
    }
    
//    perpetuationFixSingle_main();
}
