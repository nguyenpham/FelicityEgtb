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

#include "egtbgendb.h"
#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;

extern bool check2Flip;
bool check2Flip = true;

void EgtbGenDb::gen_backward_thread_init(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx);
    assert(!rcd.board);
    rcd.createBoards();

    /// Init loop
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        if (!egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white)
#ifdef _FELICITY_XQ_
            || !rcd.board->isLegal() /// don't need to check legal for chess variant
#endif
            ) {
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::black);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::white);
            continue;
        }

        if (!rcd.board->isValid()) {
            rcd.board->printOut();
        }
        assert(rcd.board->isValid());
        
        bool inchecks[] = {
            rcd.board->isIncheck(Side::black),
            rcd.board->isIncheck(Side::white)
        };
        
        for (auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto xsd = 1 - sd;
            auto score = EGTB_SCORE_UNSET;

            if (inchecks[xsd]) {
                score = EGTB_SCORE_ILLEGAL;
            } else {
                Hist hist;
                auto legalCount = 0, capCnt = 0;
                auto xside = getXSide(side);
                
                for(auto && move : rcd.board->gen(side)) {
                    if (legalCount > 0 && !move.hasPromotion() && rcd.board->isEmpty(move.dest)) {
                        continue;
                    }
                    rcd.board->make(move, hist);
                    
                    if (!rcd.board->isIncheck(side)) {
                        legalCount++;
                        
                        if (!hist.cap.isEmpty() || move.hasPromotion()) {
                            capCnt++;
                            int cScore;
                            if (!hist.cap.isEmpty() && !rcd.board->pieceList_isThereAttacker()) {
                                cScore = EGTB_SCORE_DRAW;
                            } else {
                                /// probe a sub-endgame for score
                                cScore = getScore(*rcd.board, xside);
                                
                                if (cScore == EGTB_SCORE_MISSING) {
                                    std::lock_guard<std::mutex> thelock(printMutex);
                                    rcd.board->printOut("Error: missing endagme for probing below board:");
                                    cScore = getScore(*rcd.board, xside);
                                    exit(-1);
                                }
                                
                                if (cScore != EGTB_SCORE_DRAW) {
                                    cScore = -cScore + (cScore < 0 ? -1 : +1);
                                }
                            }
                            
                            assert(cScore <= EGTB_SCORE_MATE);
                            if (score > EGTB_SCORE_MATE || score < cScore) {
                                score = cScore;
                            }
                        }
                    }
                    rcd.board->takeBack(hist);
                }
                
                if (legalCount > 0) {
                    if (capCnt > 0) {
                        egtbFile->flag_set_cap(idx, side);
                    }
                } else {
#ifdef _FELICITY_CHESS_
                    score = inchecks[sd] ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
                    score = -EGTB_SCORE_MATE;
#endif
                }
            } /// if (inchecks

            auto r = egtbFile->setBufScore(idx, score, side); assert(r);
            assert(score == egtbFile->getScore(idx, side));
        } /// for sd
    } /// for idx
}

int EgtbGenDb::gen_backward_probe(GenBoard& board, i64 idx, Side side)
{
    auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white);
    assert(ok);
    
    auto legalCount = 0, unsetCount = 0;
    auto bestScore = EGTB_SCORE_UNSET, capScore = EGTB_SCORE_UNSET;

    auto xside = getXSide(side);

    Hist hist;
    for(auto && move : board.gen(side)) {
        board.make(move, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            int score;
            if (hist.cap.isEmpty() && !move.hasPromotion()) {
                auto idx2 = egtbFile->getKey(board).key;
                if (egtbFile->flag_is_cap(idx2, xside)) {
                    score = EGTB_SCORE_UNSET;
                } else {
                    score = egtbFile->getBufScore(idx2, xside);
                }
            } else { /// capture or promotion, the value is probed already
                if (capScore == EGTB_SCORE_UNSET && egtbFile->flag_is_cap(idx, side)) {
                    capScore = egtbFile->getBufScore(idx, side);
                    if (capScore > 0) capScore++;
                    else if (capScore < 0) capScore--;
                    capScore = -capScore;
                    //assert(abs(capScore) <= EGTB_SCORE_MATE);
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
        
        if (unsetCount) {
            return EGTB_SCORE_UNSET;
        }
    }
    
    assert(legalCount > 0 && unsetCount == 0);
    return bestScore;
}


void EgtbGenDb::gen_backward_thread(int threadIdx, int ply, int sd, int phase)
{
    auto& rcd = threadRecordVec.at(threadIdx); assert(rcd.board && rcd.board2);
    
    /// In phase 0, all position with curMate will be considered
    auto curMate = EGTB_SCORE_MATE - ply;
    if ((ply & 1) == 0) {
        curMate = -curMate;
    }
    
    auto fillScore = -curMate + (curMate > 0 ? +1 : -1);
    
    auto side = static_cast<Side>(sd), xside = getXSide(side);


    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        
        auto oScore = egtbFile->getBufScore(idx, side);
        auto d = false; //idx == 36671;
        if (phase == 0) {
            /// The position has capture values, not last one
            auto is_cap = egtbFile->flag_is_cap(idx, side);
            if (is_cap) {
                if (fillScore == oScore) {
                    if (fillScore > 0) {
                        egtbFile->setBufScore(idx, fillScore, side);
                        egtbFile->flag_clear_cap(idx, side);
                        rcd.changes++;
                    } else {
                        egtbFile->flag_set_side(idx, side);
                    }
                } else if (curMate == oScore && curMate > 0) {
                    egtbFile->setBufScore(idx, curMate, side);
                    egtbFile->flag_clear_cap(idx, side);
                    rcd.changes++;
                }
                continue;
            }
            
            /// In phase 0, consider only positions with curMate
            if (oScore != curMate) {
                continue;
            }
            
            auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white);
            assert(ok);
            
            if (d) {
                rcd.board->printOut("idx = " + std::to_string(idx));
            }

            /// Retrive the parents' positions of the current board, using quiet-backward move generator
            Hist hist;
            for(auto && move : rcd.board->gen_backward_quiet(xside)) {
                
                rcd.board->make(move, hist); assert(hist.cap.isEmpty());
                
                if (!rcd.board->isIncheck(side)) {
#ifdef _FELICITY_CHESS_
                    /// clear enpassant, it is not for backward move
                    rcd.board->enpassant = -1;
#endif
                    
                    assert(rcd.board->isValid());
                    
                    auto rIdx = egtbFile->getKey(*rcd.board).key; assert(rIdx >= 0);
                    auto rScore = egtbFile->getBufScore(rIdx, xside);

                    if (d) {
                        rcd.board->printOut("idx = " + std::to_string(idx));
                        std::cout << "" << rIdx << std::endl;
                    }

                    i64 sIdx = -1;
                    if (check2Flip) {
                        auto flip = rcd.board->needSymmetryFlip();
                        if (flip != FlipMode::none) {
                            rcd.board2->clone(rcd.board);
                            rcd.board2->flip(flip);
                            assert(rcd.board2->isValid());
                            sIdx = egtbFile->getKey(*rcd.board2).key;
                            
                            if (d) {
                                rcd.board2->printOut("idx = " + std::to_string(idx));
                                std::cout << "sIdx=" << sIdx << std::endl;
                            }

                            if (rIdx == sIdx) {
                                sIdx = -1;
                            }
                        }
                    }
                    
                    /// Winning score will be filled right now (they are parents's positions of the given one)
                    if (fillScore > 0) {
                        if(rScore == EGTB_SCORE_ILLEGAL) {
                            rcd.board->printOut("rIdx = " + std::to_string(rIdx));
                            auto rIdx2 = egtbFile->getKey(*rcd.board).key;
                            
                            auto ok = egtbFile->setupBoard(*rcd.board2, rIdx, FlipMode::none, Side::white);
                            
                            rcd.board2->printOut("rIdx = " + std::to_string(rIdx));
                            assert(ok);
                        }
                        
                        assert(rScore != EGTB_SCORE_ILLEGAL);
                        if (rScore > EGTB_SCORE_MATE || rScore <= fillScore) {
                            egtbFile->setBufScore(rIdx, fillScore, xside);
                            egtbFile->flag_clear_cap(rIdx, xside);
                            
                            if (sIdx >= 0) {
                                egtbFile->setBufScore(sIdx, fillScore, xside);
                                egtbFile->flag_clear_cap(sIdx, xside);
                            }
                            
                            rcd.changes++;
                        }
                        
                    } else if (rScore > EGTB_SCORE_MATE || egtbFile->flag_is_cap(rIdx, xside)) {
                        /// Losing positions, mark them to consider later in phase 1
                        egtbFile->flag_set_side(rIdx, xside);
                        
                        if (sIdx >= 0) {
                            egtbFile->flag_set_side(sIdx, xside);
                        }
                    }
                }
                
                rcd.board->takeBack(hist);
            }
            
        } else if (egtbFile->flag_is_side(idx, side)) {
            /// phase 1 - work with marked positions only, they are lossing ones
            /// those positions have at least one lossing child but they may have
            /// better choices/children to draw or win back. We need to probe fully
            
            auto bestScore = gen_backward_probe(*rcd.board, idx, side);
            if (bestScore != EGTB_SCORE_UNSET) {
                egtbFile->setBufScore(idx, bestScore, side);
                egtbFile->flag_clear_cap(idx, side);
                
                if (check2Flip) {
                    auto flip = rcd.board->needSymmetryFlip();
                    if (flip != FlipMode::none) {
                        rcd.board->flip(flip);
                        auto sIdx = egtbFile->getKey(*rcd.board).key;
                        if (idx != sIdx) {
                            egtbFile->setBufScore(sIdx, bestScore, side);
                            egtbFile->flag_clear_cap(sIdx, side);
                        }
                    }
                }
                
                rcd.changes++;
            }
        }
    }
}



/// Using backward move-generator
void EgtbGenDb::gen_backward(const std::string& folder) {
    if (egtbVerbose) {
        std::cout << "\tGenerate backwardly!" << std::endl;
    }
    egtbFile->createFlagBuffer();

#ifdef _FELICITY_CHESS_
    check2Flip = egtbFile->getName().find('p') == std::string::npos;
#endif

    auto ply = 0, mPly = 0;
    auto side = Side::black;
    
    /// Init loop
    if (ply == 0) {
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::gen_backward_thread_init, this, i));
        }
        
        gen_backward_thread_init(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
        
        /// remove buffers of all sub-endgames since they have been probed already, to save RAM
        removeAllProbedBuffers();

        /// Calculate ply
        ply = mPly = 0;
        auto maxScore = EGTB_SCORE_UNSET, minScore = EGTB_SCORE_UNSET;

        for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            for(auto sd = 0; sd < 2; sd++) {
                auto score = abs(egtbFile->getBufScore(idx, static_cast<Side>(sd)));
                if (score > EGTB_SCORE_DRAW && score <= EGTB_SCORE_MATE) {
                    minScore = minScore == EGTB_SCORE_UNSET ? score : std::min(minScore, score);
                    maxScore = maxScore == EGTB_SCORE_UNSET ? score : std::max(maxScore, score);
                }
            }
        }
        
        if (maxScore <= EGTB_SCORE_MATE && minScore <= EGTB_SCORE_MATE) {
            auto p0 = EGTB_SCORE_MATE - std::min(maxScore, minScore);
            auto p1 = EGTB_SCORE_MATE - std::max(maxScore, minScore);
            //ply = std::min(p0, p1);
            mPly = std::max(p0, p1);
            if (mPly == EGTB_SCORE_MATE) mPly = ply;
        }
        
        assert(ply >= 0 && ply <= mPly && mPly < EGTB_SCORE_MATE);

        if (egtbVerbose) {
            std::cout << "Init for backward successfully." << std::endl;
        }
    }


    /// Main loops
    i64 totalChangeCnt = 0;
    
    for(auto tryCnt = 2; tryCnt > 0; ply++, side = getXSide(side)) {
        assert(ply < 1000);
        
        if (ply > 300) {
            ply = ply;
        }
        
        resetAllThreadRecordCounters();
        
        /// Clear all side-marks
        for(i64 idx = 0; idx < egtbFile->getSize() / 2 + 1; idx++) {
            egtbFile->flags[idx] &= ~(3 | 3 << 4);
        }
        
        /// Fill positions by two phrases and two sides, we should not combine them info one to avoid being
        /// conflicted on writting between threads
        /// phase 0: fill winning positions, mark losing positions by retro/backward moves
        /// phase 1: probe and fill marked positions
        for(auto phase = 0; phase < 2; phase++) {
            for(auto sd = 0; sd < 2; sd++) {
                
                std::vector<std::thread> threadVec;
                for (auto i = 1; i < threadRecordVec.size(); ++i) {
                    threadVec.push_back(std::thread(&EgtbGenDb::gen_backward_thread, this, i, ply, sd, phase));
                }
                gen_backward_thread(0, ply, sd, phase);
                
                for (auto && t : threadVec) {
                    t.join();
                }
            }
        }
        
        auto callLoopChangeCnt = allThreadChangeCount();
        totalChangeCnt += callLoopChangeCnt;

        if (callLoopChangeCnt == 0) {
            if (ply > mPly) {
                tryCnt--;
            }
        } else {
            tryCnt = 2;
        }
    }
    
    /// Generated all posible positions, fill the rest
    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
        for(auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            if (egtbFile->flag_is_cap(idx, side)) {
                egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, side);
            }
        }
    }

    if (egtbVerbose) {
        std::cout << "Completed main loops." << std::endl;
    }
}
