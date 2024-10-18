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

static const int debugIdx = -10;
static const Side debugSide = Side::black; // Side::white;

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

        assert(rcd.board->isValid());
        
#ifdef _FELICITY_FLIP_MAP_
        if (check2Flip) {
            std::lock_guard<std::mutex> thelock(flipIdxMapMutex);
            if (flipIdxMap.find(idx) == flipIdxMap.end()) {
                auto flip = rcd.board->needSymmetryFlip();
                if (flip != FlipMode::none) {
                    rcd.board2->clone(rcd.board);
                    rcd.board2->flip(flip);
                    assert(rcd.board2->isValid());
                    auto sIdx = egtbFile->getKey(*rcd.board2).key; assert(sIdx >= 0);
                    
                    if (idx != sIdx) {
                        flipIdxMap[idx] = sIdx;
                        flipIdxMap[sIdx] = idx;
                    }
                }
            }
        }
#endif

        bool inchecks[] = {
            rcd.board->isIncheck(Side::black),
            rcd.board->isIncheck(Side::white)
        };
        
        for (auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto xsd = 1 - sd;
            auto bestScore = EGTB_SCORE_UNSET;
            auto legalCount = 0, capCnt = 0;

            auto d = idx == debugIdx; // && side == Side::black;

            if (inchecks[xsd]) {
                bestScore = EGTB_SCORE_ILLEGAL;
            } else {
                Hist hist;
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
                            int score;
                            if (!hist.cap.isEmpty() && !rcd.board->pieceList_isThereAttacker()) {
                                score = EGTB_SCORE_DRAW;
                            } else {
                                /// probe a sub-endgame for score
                                score = getScore(*rcd.board, xside);
                                
                                if (score == EGTB_SCORE_MISSING) {
                                    std::lock_guard<std::mutex> thelock(printMutex);
                                    rcd.board->printOut("Error: missing endagme for probing below board:");
                                    score = getScore(*rcd.board, xside);
                                    exit(-1);
                                }
                            }
                            
                            assert(score != EGTB_SCORE_UNSET);
                            EgtbFile::pickBestFromRivalScore(bestScore, score);
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
                    bestScore = inchecks[sd] ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
                    bestScore = -EGTB_SCORE_MATE;
#endif
                }
            } /// if (inchecks

            {
#ifdef _FELICITY_FLIP_MAP_
                std::lock_guard<std::mutex> thelock(flipIdxMapMutex);
#endif
                auto vec = setBufScore(rcd, idx, bestScore, side);
                
                if (capCnt > 0) {
                    assert(legalCount > 0);
                    egtbFile->flag_set_cap(idx, side);
                    if (vec.size() > 1) {
                        egtbFile->flag_set_cap(vec[1], side);
                    }
                }

                if (idx == debugIdx || vec.back() == debugIdx) {
                    std::string s = std::string("Inited data, vec=") + std::to_string(vec.front())
                    + ", " + std::to_string(vec.back());
                    showData(s, debugIdx, side, false);
                }
                
            }
        } /// for sd

    } /// for idx
} /// gen_backward_thread_init


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
                auto idx2 = egtbFile->getIdx(board).key;
                if (egtbFile->flag_is_cap(idx2, xside)) {
                    score = EGTB_SCORE_UNSET;
                } else {
                    score = egtbFile->getBufScore(idx2, xside);
                }
            } else { /// capture or promotion, the value is probed already
                if (capScore == EGTB_SCORE_UNSET && egtbFile->flag_is_cap(idx, side)) {
                    capScore = egtbFile->getBufScore(idx, side);
                    
                    /// Special increase
                    capScore = EgtbFile::revertScore(capScore, -1);

                }
                score = capScore;
            }
            
            if (score == EGTB_SCORE_UNSET) {
                unsetCount++;
            } else {
                EgtbFile::pickBestFromRivalScore(bestScore, score);
            }
        }
        board.takeBack(hist);
        
        if (unsetCount) {
            return EGTB_SCORE_UNSET;
        }
    }
    
    if (legalCount == 0) {
        board.printOut("Error");
    }
    assert(legalCount > 0 && unsetCount == 0);
    return bestScore;
}



void EgtbGenDb::gen_backward_thread(int threadIdx, int ply, int sd, int phase)
{
    /// In phase 0, all position with curMate will be considered
    auto curMate = EGTB_SCORE_MATE - ply;
    if ((ply & 1) == 0) {
        curMate = -curMate;
    }
    
    auto fillScore = -curMate + (curMate > 0 ? +1 : -1);
    
    auto& rcd = threadRecordVec.at(threadIdx); assert(rcd.board && rcd.board2);
    
    const auto side = static_cast<Side>(sd), xside = getXSide(side);


    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        if (phase == 0) {
            auto oScore = egtbFile->getBufScore(idx, side);
            
            /// The position has capture values, not last one
            auto is_cap = egtbFile->flag_is_cap(idx, side);
            if (is_cap) {
                if (fillScore == oScore) {
                    auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
                    auto idx2 = getFlipIdx(rcd, idx);
                    if (fillScore > 0) {
                        egtbFile->setBufScore(idx, fillScore, side);
                        egtbFile->flag_clear_cap(idx, side);
                        if (idx2 >= 0) {
                            egtbFile->setBufScore(idx2, fillScore, side);
                            egtbFile->flag_clear_cap(idx2, side);
                        }
                        rcd.changes++;

                    } else {
                        egtbFile->flag_set_side(idx, side);
                        if (idx2 >= 0) {
                            egtbFile->flag_set_side(idx2, side);
                        }
                        if (idx == debugIdx || idx2 == debugIdx) {
                            std::cout << "IMHERE 2, set side flag at " << idx
                            << ", ply = " << ply
                            << ", idx = " << idx
                            << ", idx2 = " << idx2 << std::endl;
                            rcd.board->printOut("IMHERE 2");
                        }
                    }


                } else if (curMate == oScore && curMate > 0) {
                    
                    auto vec = setBufScore(rcd, idx, curMate, side);
                    assert(vec.size() > 0 && vec.size() <= 2);
                    egtbFile->flag_clear_cap(idx, side);
                    if (vec.size() > 1) {
                        egtbFile->flag_clear_cap(vec[1], side);
                    }
                    rcd.changes++;
                    
                    if (vec.front() == debugIdx || vec.back() == debugIdx) {
                        std::cout << "IMHERE 4, curMate = " << curMate << std::endl;
                    }
                }
                continue;
            }
            
            /// In phase 0, consider only positions with curMate
            if (oScore != curMate
                || (oScore == EGTB_SCORE_ILLEGAL && egtbFile->getBufScore(idx, xside) == EGTB_SCORE_ILLEGAL)) {
                continue;
            }
            
            auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white);
            
            if (!ok) {
                std::cout << "Something WRONG, idx=" << idx
                << ", oScore=" << oScore
                << ", xScore=" << egtbFile->getBufScore(idx, xside)
                << std::endl;
            }
            assert(ok);
            
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
                    
                    auto rIdx = egtbFile->getIdx(*rcd.board).key; assert(rIdx >= 0);
                    
                    auto rScore = egtbFile->getBufScore(rIdx, xside);
                    
                    /// Winning score will be filled right now (they are parents's positions of the given one)
                    if (fillScore > 0) {
                        if(rScore == EGTB_SCORE_ILLEGAL) {
                        } else {
                            if (rScore == EGTB_SCORE_UNSET
                                || rScore == fillScore
                                || EgtbFile::isSmallerScore(rScore, fillScore)
                                || egtbFile->flag_is_cap(rIdx, xside)
                                ) {
                                auto vec = setBufScore(rcd, rIdx, fillScore, xside);
                                assert(vec.size() > 0 && vec.size() <= 2);
                                egtbFile->flag_clear_cap(rIdx, xside);
                                if (vec.size() > 1) {
                                    egtbFile->flag_clear_cap(vec[1], xside);
                                }
                                rcd.changes++;
                                
                                if (vec.front() == debugIdx || vec.back() == debugIdx
                                    ) {
                                    std::cout << "IMHERE 8, fillScore = " << fillScore
                                    << ", vec sz=" << vec.size()
                                    << ", rIdx=" << rIdx
                                    << ", idx2=" << vec.back()
                                    << ", rScore=" << rScore
                                    << std::endl;
                                    
//                                    rcd.board->printOut("rIdx = " + std::to_string(rIdx));
                                    showData("IMHERE 8, fillScore", debugIdx, debugSide, true);
                                    
//                                    showData("IMHERE 8, fillScore", 1848018, debugSide, true);

                                }
                                
                            }
                        }
                        
                    } else if (rScore == EGTB_SCORE_UNSET || egtbFile->flag_is_cap(rIdx, xside)) {
                        /// Losing positions, mark them to consider later in phase 1
                        egtbFile->flag_set_side(rIdx, xside);
                        
                        auto sIdx = getFlipIdx(rcd, rIdx);
                        if (sIdx >= 0) {
                            egtbFile->flag_set_side(sIdx, xside);
                        }
                        
                        if (rIdx == debugIdx || sIdx == debugIdx) {
                            std::cout << "IMHERE 9, set side flag at " << idx
                            << ", ply = " << ply
                            << ", rIdx = " << rIdx
                            << ", sIdx = " << sIdx << std::endl;
                            rcd.board->printOut("IMHERE 9");
                        }
                    }
                }
                
                rcd.board->takeBack(hist);
            }
            
            /// phase == 1
        } else if (egtbFile->flag_is_side(idx, side)) {
            /// phase 1 - work with marked positions only, they are lossing ones
            /// those positions have at least one lossing child but they may have
            /// better choices/children to draw or win back. We need to probe fully
            
            auto bestScore = gen_backward_probe(*rcd.board, idx, side);
            

            if (bestScore != EGTB_SCORE_UNSET) {
                auto vec = setBufScore(rcd, idx, bestScore, side);
                assert(vec.size() > 0 && vec.size() <= 2);
                egtbFile->flag_clear_cap(idx, side);
                if (vec.size() > 1) {
                    egtbFile->flag_clear_cap(vec[1], side);
                }
                rcd.changes++;

                if (vec.front() == debugIdx || vec.back() == debugIdx) {
                    std::cout
                    << "IMHERE 16, bestScore = " << bestScore
                    << ", idx = " << idx
                    << ", fillScore = " << fillScore
                    << std::endl;
//                    rcd.board->printOut("idx = " + std::to_string(idx));
                    showData("IMHERE 16", debugIdx, debugSide, true);
                }
            }
        }
    }
}


#ifdef _FELICITY_CHESS_
void EgtbGenDb::gen_fillCapturesAfterGenerating()
{
    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
        for(auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            if (egtbFile->flag_is_cap(idx, side)) {
                egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, side);
//                auto vec = setBufScore(idx, EGTB_SCORE_UNSET, side);
//                assert(vec.size() > 0 && vec.size() <= 2);
            }
        }
    }

}
#else

/*
 It is similar to EgtbDb::probeByChildren, except it won't work with cap-flag
 */
int EgtbGenDb::gen_probeByChildren(EgtbBoard& board, Side side, bool debugging)
{
    auto bestScore = EGTB_SCORE_UNSET, legalCount = 0, unsetCount = 0;
    auto xside = getXSide(side);

    if (debugging) {
        board.printOut("EgtbGenDb::gen_probeByChildren, debugging, side=" + Funcs::side2String(side, false));
    }
    Hist hist;
    for(auto && move : board.gen(side)) {
        board.make(move, hist);
        
        if (!board.isIncheck(side)) {
            legalCount++;

            int score;
            i64 sIdx = -1;
            
            if (hist.cap.isEmpty() && !move.hasPromotion()) {     /// score from current working buffers
                auto r = egtbFile->getIdx(board);
                auto xs = r.flipSide ? side : xside;
                sIdx = r.key;
                if (egtbFile->flag_is_cap(sIdx, xs)) {
                    score = EGTB_SCORE_UNSET;
                } else {
                    score = egtbFile->getScore(r.key, xs, false);
                }
            } else if (!board.hasAttackers()) {
                score = EGTB_SCORE_DRAW;
            } else {            /// probe from a sub-endgame
                score = getScore(board, xside);
                if (score == EGTB_SCORE_MISSING) {
                    board.printOut("Missing endgame for probling the board:");
                    exit(-1);
                }
            }
            
            if (!EgtbFile::pickBestFromRivalScore(bestScore, score)) {
                unsetCount++;
                bestScore = EGTB_SCORE_UNSET;
            }
            
            if (debugging) {
                board.printOut("after a move=" + board.toString(move)
                               + ", sIdx=" + std::to_string(sIdx)
                               + ", score=" + std::to_string(score)
                               + ", bestScore=" + std::to_string(bestScore)
                               + ", unsetCount=" + std::to_string(unsetCount));
            }
        }
        board.takeBack(hist);

        if (unsetCount && !debugging) {
            return EGTB_SCORE_UNSET;
        }
    }
    
    if (legalCount == 0) {
#ifdef _FELICITY_CHESS_
        bestScore = board.isIncheck(side) ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
        bestScore = -EGTB_SCORE_MATE;
#endif
    }

    if (debugging) {
        std::cout << "Debugging completed, bestScore=" << bestScore << ", legalCount=" << legalCount << std::endl;
    }
    
    return unsetCount ? EGTB_SCORE_UNSET : bestScore;
}

void EgtbGenDb::gen_fillCapturesAfterGenerating()
{
    GenBoard board;
    auto changCnt = 1;
    while (changCnt > 0) {
        changCnt = 0;
        for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            for(auto sd = 0; sd < 2; sd++) {
                auto side = static_cast<Side>(sd);
                if (!egtbFile->flag_is_cap(idx, side)) {
                    continue;
                }
                
                auto score = egtbFile->getBufScore(idx, side);
                /// Wining perpetual captures now become officially scores since there are no better options
                
                if (!EgtbFile::isPerpetualScore(score)) {
                    continue;
                }
                
                auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white);
                assert(ok);
                
                auto sc = gen_probeByChildren(board, side);
                if (sc != EGTB_SCORE_UNSET) {
                    egtbFile->setBufScore(idx, sc, side);
                    egtbFile->flag_clear_cap(idx, side);
                    changCnt++;
                    
                    auto flip = board.needSymmetryFlip();
                    if (flip != FlipMode::none) {
                        board.flip(flip);
                        auto idx2 = egtbFile->getIdx(board).key;
                        if (idx != idx2) {
                            egtbFile->setBufScore(idx2, sc, side);
                            egtbFile->flag_clear_cap(idx2, side);
                        }
                    }
                }
            }
        }
    }
    
    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
        for(auto sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            if (egtbFile->flag_is_cap(idx, side)) {
                egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, side);
                egtbFile->flag_clear_cap(idx, side);
            }
        }
    }

}

#endif

/// Using backward move-generator
void EgtbGenDb::gen_backward(const std::string& folder)
{
    if (egtbVerbose) {
        std::cout << "\tGenerate backwardly!" << std::endl;
    }
    egtbFile->createFlagBuffer();
    
#ifdef _FELICITY_CHESS_
    check2Flip = egtbFile->getName().find('p') == std::string::npos;
#endif
    
#ifdef _FELICITY_FLIP_MAP_
    flipIdxMap.clear();
#endif

    auto ply = 0, mPly = 0;
    auto side = Side::black;
    
    if (egtbFile->readFromTmpFiles(folder, ply, mPly)) {
        std::cout << "\treadFromTmpFiles Success" << std::endl;
        
#ifdef _FELICITY_FLIP_MAP_
        if (check2Flip) {
            std::vector<std::thread> threadVec;
            for (auto i = 1; i < threadRecordVec.size(); ++i) {
                threadVec.push_back(std::thread(&EgtbGenDb::gen_backward_thread_init_flipMap, this, i));
            }
            
            gen_backward_thread_init_flipMap(0);
            
            for (auto && t : threadVec) {
                t.join();
            }
            threadVec.clear();
            
            std::cout << "flipIdxMap.size=" << flipIdxMap.size() << std::endl;
        }
#endif
        return;
    } else {
        ply = 0; mPly = 0;
    }
    
    
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
        
        /// Calculate ply for starting
        ply = mPly = 0;
        auto maxScore = EGTB_SCORE_UNSET, minScore = EGTB_SCORE_UNSET;
        
        for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            for(auto sd = 0; sd < 2; sd++) {
                auto score = abs(egtbFile->getBufScore(idx, static_cast<Side>(sd)));
#ifdef _FELICITY_CHESS_
                if (score > EGTB_SCORE_DRAW && score <= EGTB_SCORE_MATE)
#else
                    if (score > EGTB_SCORE_PERPETUAL_ABOVE && score <= EGTB_SCORE_MATE)
#endif
                    {
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
            std::cout << "\tInit for backward successfully." << std::endl;
        }
    }
    
#ifdef _FELICITY_FLIP_MAP_
    std::cout << "flipIdxMap, sz=" << flipIdxMap.size()
    << ", flip debugIdx = " << getFlipIdx(debugIdx)
    << std::endl;
#endif
    
    showData("After init", debugIdx, Side::none, false);

    /// Main loops
    i64 totalChangeCnt = 0;
    
    for(auto tryCnt = 2; tryCnt > 0; ply++, side = getXSide(side)) {
        
        assert(ply < 1000);
        
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
    
    showData("after gen_backward_thread, before filling", debugIdx, Side::none, false);

    /// All posible positions are generated , now fill the rest
    gen_fillCapturesAfterGenerating();

    showData("Finishing, after filling", debugIdx, Side::none, false);

    if (egtbVerbose) {
        std::cout << "\tCompleted main loops." << std::endl;
    }
    
    egtbFile->writeTmpFiles(folder, ply, mPly);
    std::cout << "\twriteTmpFiles." << std::endl;
}

#ifdef _FELICITY_FLIP_MAP_
        
void EgtbGenDb::gen_backward_thread_init_flipMap(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx);
    assert(!rcd.board);
    rcd.createBoards();
    
    /// Init loop
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };
        
        if (scores[0] == EGTB_SCORE_ILLEGAL && scores[1] == EGTB_SCORE_ILLEGAL) {
            continue;
        }
        
        {
            std::lock_guard<std::mutex> thelock(flipIdxMapMutex);
            if (getFlipIdx(idx) >= 0) {
                continue;
            }
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        
        auto flip = rcd.board->needSymmetryFlip();
        if (flip == FlipMode::none) {
            continue;
        }
        rcd.board->flip(flip);
        auto sIdx = egtbFile->getKey(*rcd.board).key; assert(sIdx >= 0);
        
        if (idx != sIdx) {
            std::lock_guard<std::mutex> thelock(flipIdxMapMutex);
            flipIdxMap[idx] = sIdx;
            flipIdxMap[sIdx] = idx;
        }
        
    } /// for idx
}
#endif
