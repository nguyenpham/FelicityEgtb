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

#include "egtbgendb.h"
#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;


void EgtbGenDb::gen_thread_init_backward(int threadIdx)
{
    auto record = threadRecordVec.at(threadIdx);

    // Init loop
//    if (egtbVerbose) {
//        std::lock_guard<std::mutex> thelock(printMutex);
//        std::cout << "init. threadIdx: " << threadIdx << ", " << record.fromIdx << "->" << record.toIdx << ", sz: " << egtbFile->getSize() << std::endl;
//    }

    for (auto idx = record.fromIdx; idx < record.toIdx; idx++) {
//        if (egtbVerbose && (idx - record.fromIdx) % (16 * 1024 * 1024) == 0) {
//            std::lock_guard<std::mutex> thelock(printMutex);
//            std::cout << "init, threadIdx = " << threadIdx << ", idx = " << idx << ", toIdx = " << record.toIdx << ", " << (idx - record.fromIdx) * 100 / (record.toIdx - record.fromIdx) << "%" << std::endl;
//        }

        if (!egtbFile->setupBoard(*record.board, idx, FlipMode::none, Side::white)) {
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::black);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::white);
            continue;
        }

        assert(record.board->isValid());
        
        bool inchecks[] = { record.board->isIncheck(Side::black), record.board->isIncheck(Side::white) };
        int scores[2];
        
        for (auto sd = 0; sd < 2; sd++) {
            auto xsd = 1 - sd;
            if (inchecks[xsd]) {
                scores[sd] = EGTB_SCORE_ILLEGAL;
                continue;
            }

            auto side = static_cast<Side>(sd), xside = getXSide(side);

            Hist hist;
            auto moveList = record.board->gen(side);

            auto legalCount = 0, capCnt = 0;
            auto score = EGTB_SCORE_UNSET;
            for(auto && m : moveList) {
                record.board->make(m, hist);
                
                if (!record.board->isIncheck(side)) {
                    legalCount++;

                    if (!hist.cap.isEmpty()) {
                        capCnt++;
                        int cScore;
                        if (!record.board->pieceList_isThereAttacker()) {
                            cScore = EGTB_SCORE_DRAW;
                        } else {
                            cScore = getScore(*record.board, xside);
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
                            record.board->printOut("Error: missing endagme for probing below board:");
                            exit(-1);
                        }
                    }
                }
                record.board->takeBack(hist);
            }

            if (!legalCount) {
                scores[sd] = -EGTB_SCORE_MATE;
                continue;
            }
            
            if (score < EGTB_SCORE_MATE) {
                egtbFile->flag_set_cap(idx, side);
//            } else if (inchecks[sd] && egtbFile->isBothArmed()) {
//                assert(score == EGTB_SCORE_UNSET);
//                score = EGTB_SCORE_CHECKED;
            }
            
            scores[sd] = score;

        } // for sd
        
        for (auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto r = egtbFile->setBufScore(idx, scores[sd], side); assert(r);
            assert(scores[sd] == egtbFile->getScore(idx, side));
        }

    } // for (i64 idx
}

int EgtbGenDb::probe_gen_backward(EgtbBoard& board, i64 idx, Side side, int ply)
{
    auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white);
    assert(ok);

    auto legalCount = 0, unsetCount = 0;
    auto bestScore = EGTB_SCORE_UNSET, capScore = EGTB_SCORE_UNSET;
    auto sd = static_cast<int>(side);
    auto xsd = 1 - sd;
    auto xside = getXSide(side);

    Hist hist;
    auto moveList = board.gen(side); assert(!moveList.empty());

    for(auto && m : moveList) {
        board.make(m, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            auto internal = hist.cap.isEmpty();
            
            int score;
            if (internal) {
                auto idx2 = egtbFile->getKey(board).key;
                if (egtbFile->flag_is_cap(idx2, xside)) {
                    score = EGTB_SCORE_UNSET;
                } else {
                    score = egtbFile->getScore(idx2, xside, false);
                }
            } else {
                if (capScore == EGTB_SCORE_UNSET) {
                    if (egtbFile->flag_is_cap(idx, side)) {
                        capScore = egtbFile->getScore(idx, side, false);
                        if (capScore > 0) capScore++;
                        else if (capScore < 0) capScore--;
                        capScore = -capScore;
                        assert(abs(capScore) <= EGTB_SCORE_MATE);
                    }
                }
                score = capScore;
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
    
    if (legalCount == 0) {
        return -EGTB_SCORE_MATE;
    }

    assert(legalCount > 0); assert(bestScore != EGTB_SCORE_ILLEGAL);
    return unsetCount == 0 ? bestScore : EGTB_SCORE_UNSET;
}

void EgtbGenDb::gen_thread_backward(int threadIdx, int sd, int ply, int task)
{
    auto& rcd = threadRecordVec.at(threadIdx);

    auto side = static_cast<Side>(sd), xside = getXSide(side);
    auto xsd = 1 - sd;
    
    auto curMate = EGTB_SCORE_MATE - ply;
    if ((ply & 1) == 0)
        curMate = -curMate;

    auto fillScore = -curMate + (curMate > 0 ? +1 : -1);

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        if (task == 0) {
            if (egtbVerbose && sd == 0 && (idx - rcd.fromIdx) % (16 * 1024 * 1024) == 0) {
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cout << "loop " << ply << ", threadIdx = " << threadIdx << ", idx = " << idx << ", toIdx = " << rcd.toIdx << ", " << (idx - rcd.fromIdx) * 100 / (rcd.toIdx - rcd.fromIdx) << "%" << std::endl;
            }

            int oScore = egtbFile->getScore(idx, side, false);

            if (egtbFile->flag_is_cap(idx, side)) {
                if (fillScore == oScore) {
                    if (fillScore > 0) {
                        egtbFile->setBufScore(idx, fillScore, side);
                        egtbFile->flag_clear_cap(idx, side);
                    } else {
                        egtbFile->flag_set_side(idx, side);
                    }
                } else if (curMate == oScore && curMate > 0) {
                    egtbFile->setBufScore(idx, curMate, side);
                    egtbFile->flag_clear_cap(idx, side);
                } else
                    continue;
            }

            if (oScore != curMate) {
                continue;
            }

            auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white);
            assert(ok);

            auto moveList = rcd.board->gen_backward_nocap(xside);

            for(auto && move : moveList) {
                Hist hist;
                rcd.board->make(move, hist);
                
                assert(hist.cap.isEmpty());
                
                if (!rcd.board->isIncheck(side)) {
                    assert(rcd.board->isValid());
                    
                    auto rIdx = egtbFile->getKey(*rcd.board).key;
                    if (rIdx >= 0) {
                        i64 sIdx = -1;
                        
//                        if (needFlip(board)) {
//                            board.flip(FlipMode::horizontal);
//                            sIdx = egtbFile->getKey(board).key;
//                            board.flip(FlipMode::horizontal);
//                        }
                        
                        if (fillScore > 0) {
                            auto theScore = egtbFile->getScore(rIdx, xside, false);
                            if (theScore > EGTB_SCORE_MATE || theScore <= fillScore) {
                                egtbFile->setBufScore(rIdx, fillScore, xside);
                                egtbFile->flag_clear_cap(rIdx, xside);
                                
                                if (sIdx >= 0) {
                                    
                                    theScore = egtbFile->getScore(sIdx, xside, false);
                                    if (theScore > EGTB_SCORE_MATE || theScore < fillScore) {
                                        egtbFile->setBufScore(sIdx, fillScore, xside);
                                        egtbFile->flag_clear_cap(sIdx, xside);
                                    }
                                }
                                
                                rcd.changes++;
                            }
                            
                        } else {
                            egtbFile->flag_set_side(rIdx, xside);
                            
                            if (sIdx >= 0) {
                                egtbFile->flag_set_side(sIdx, xside);
                            }
                        }
                    }
                }
                
                rcd.board->takeBack(hist);
            }
            continue;
        }

        // Task 1 - Losing
        if (!egtbFile->flag_is_side(idx, side)) {
            continue;
        }

        int oScore = egtbFile->getScore(idx, side, false);
        if (oScore == EGTB_SCORE_ILLEGAL) {
            auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white);
            rcd.board->printOut("Illegal");
            assert(ok);
        }
        
        auto bestScore = probe_gen_backward(*rcd.board, idx, side, ply);

        if (bestScore != EGTB_SCORE_UNSET) {
            egtbFile->setBufScore(idx, bestScore, side);
            egtbFile->flag_clear_cap(idx, side);
            rcd.changes++;
        }
    }
}

/// Using backward move-generator
void EgtbGenDb::gen_backward(const std::string& folder) {
    if (egtbVerbose) {
        std::cout << "\tGenerate backwardly!" << std::endl;
    }

    egtbFile->createFlagBuffer();

    auto ply = 0, mPly = 0;

    auto side = Side::white;
    
    /// Load temp files
    auto wLoop = 0, bLoop = 0;
//    if (useTempFiles) {
//        wLoop = egtbFile->readFromTmpFile(folder, Side::white);
//        bLoop = egtbFile->readFromTmpFile(folder, Side::black);
//    }
//    
//    if (wLoop > 0 && bLoop > 0) {
//        ply = std::max(wLoop, bLoop);
//        side = wLoop > bLoop ? Side::white : Side::black;
//        
//        ply++; /// next loop
//        side = getXSide(side);
//        
//        if (egtbVerbose) {
//            std::cout << "\tLoaded temp file, start from ply: " << ply << ", sd: " << Funcs::side2String(side, false) << std::endl;
//        }
//    }
    
    /// Init loop
    if (ply == 0) {
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::gen_thread_init_backward, this, i));
        }
        
        gen_thread_init_backward(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
        
        removeAllBuffers();

        /// Calculate ply
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


    /// Main loops
    i64 totalChangeCnt = 0;
    
    for(auto tryCnt = 2; tryCnt > 0; ply++, side = getXSide(side)) {
        auto startLoop = time(NULL);

        resetAllThreadRecordCounters();
        
        for(i64 idx = 0; idx < egtbFile->getSize() / 2 + 1; idx++) {
            egtbFile->flags[idx] &= ~(3 | 3 << 4);
        }

        
        
        for(int task = 0; task < 2; task++) {
            for(int sd = 0; sd < 2; sd++) {
                
                std::vector<std::thread> threadVec;
                auto side = static_cast<Side>(sd);
                for (auto i = 1; i < threadRecordVec.size(); ++i) {
                    threadVec.push_back(std::thread(&EgtbGenDb::gen_thread_backward, this, i, sd, ply, task));
                }
                gen_thread_backward(0, sd, ply, task);
                
                for (auto && t : threadVec) {
                    t.join();
                }
                
                auto callLoopChangeCnt = allThreadChangeCount();
                
                totalChangeCnt += callLoopChangeCnt;
            }
        }
        
//        if (egtbVerbose) {
//            int elapsed_secs = std::max(1, (int)(time(NULL) - startLoop));
//            std::cout << "\tChanged: " << GenLib::formatString(callLoopChangeCnt) << ", elapsed: " << GenLib::formatPeriod(elapsed_secs) << " (" << elapsed_secs << " s), speed: " << GenLib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
//        }
        
//        if (useTempFiles && ply > 0 && callLoopChangeCnt > 0) {
//            egtbFile->writeTmpFile(folder, side, ply);
//        }
        
        if (allThreadChangeCount() == 0) {
            if (ply > mPly) {
                tryCnt--;
            }
        } else {
//            if (useTempFiles && (ply & 7) == 0) {
//                egtbFile->writeTmpFiles(folder, ply, ply);
//            }
            tryCnt = 2;
        }
    }
    
    if (egtbVerbose) {
        std::cout << "Completed main loops." << std::endl;
    }

    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
        for(auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            if (egtbFile->flag_is_cap(idx, side)) {
                egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, side);
            }
        }
    }

}
