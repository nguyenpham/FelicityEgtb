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
#include "genboard.h"

#ifdef _FELICITY_XQ_

using namespace fegtb;
using namespace bslib;

void EgtbGenDb::perpetuation_process()
{
//    if (!egtbFile->isBothArmed()) {
//        //if (egtbVerbose) {
//            std::cout << "\tOne side is not armed!" << std::endl;
//        //}
//        return;
//    }

    //if (egtbVerbose) {
        std::cout << "\tPerpetuation_process!" << std::endl;
    //}
    
    if (perpetuation_init() == 0) {
        return;
    }

    perpetuation_gen();

    
    i64 perpetuation_win = 0, perpetuation_lose = 0;
    for (auto idx = 0; idx < egtbFile->getSize(); idx++) {
        for (auto sd = 0; sd < 2; sd++) {
            auto score = egtbFile->getBufScore(idx, static_cast<Side>(sd));
            
            if (score == EGTB_SCORE_PERPETUATION_WIN) perpetuation_win++;
            else if (score == EGTB_SCORE_PERPETUATION_LOSE) perpetuation_lose++;
        }
    }
    
    //if (egtbVerbose) {
        std::cout << "Perpetuation_process DONE. perpetuations "
    << "wins: " << perpetuation_win << ", loses: " << perpetuation_lose
    << "; total: " << perpetuation_win + perpetuation_lose
    << std::endl;
    //}
}

i64 EgtbGenDb::perpetuation_init()
{
    /// 1 thread only
    {
        while(threadRecordVec.size() > 1) {
            threadRecordVec.pop_back();
        }
        threadRecordVec[0].toIdx = egtbFile->getSize();
    }
    
    resetAllThreadRecordCounters();
    egtbFile->clearFlagBuffer();

    std::vector<std::thread> threadVec;
    for (auto i = 1; i < threadRecordVec.size(); ++i) {
        threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_init, this, i));
    }
    
 
    perpetuation_thread_init(0);
    
    for (auto && t : threadVec) {
        t.join();
    }
    threadVec.clear();

    return allThreadChangeCount();
}



//////
bool EgtbGenDb::perpetuation_score_valid(int score)
{
    return 
        score == EGTB_SCORE_MISSING     /// actually, it is illegal positions
        || score == EGTB_SCORE_UNSET
    
        || (score == EGTB_SCORE_PERPETUATION_WIN || score == EGTB_SCORE_PERPETUATION_LOSE)
        /// Losing
        || (score >= EGTB_SCORE_DRAW && score <= EGTB_SCORE_MATE);
}


void EgtbGenDb::perpetuation_thread_init(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);

    Hist hist;

    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };
                
        if ((scores[0] != EGTB_SCORE_UNSET && scores[1] != EGTB_SCORE_UNSET)
            || (scores[0] != EGTB_SCORE_ILLEGAL && scores[1] != EGTB_SCORE_ILLEGAL)) {
            continue;
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        
        for(auto sd = 0; sd < 2; ++sd) {
            if (scores[sd] != EGTB_SCORE_UNSET) {
                continue;
            }
            assert(scores[1 - sd] == EGTB_SCORE_ILLEGAL);
            
            auto side = static_cast<Side>(sd);

            assert(rcd.board->isIncheck(side));

            std::set<i64> idxSet { idx };
            auto rMap = perpetuation_evasion(rcd, idx, side, idxSet, true);
            if (!rMap.empty()) {
                for(auto && p : rMap) {
                    auto rIdx = p.first;
                    auto rSide = p.second;
                    
                    auto value = side != rSide ? EGTB_SCORE_PERPETUATION_LOSE : EGTB_SCORE_PERPETUATION_WIN;
                    egtbFile->setBufScore(rIdx, value, rSide);
                    egtbFile->flag_set_cap(rIdx, rSide);
                    
//                    if (rIdx == 1025443) {
//                        rcd.board->printOut("starting");
//                        std::cout << "rIdx=" << rIdx << ", sd=" << static_cast<int>(rSide) << ", val=" << value << std::endl;
//                    }
                }
                rcd.changes++;
                rcd.board->printOut("\nperpetuation detected!");
                std::cout << std::endl;
            }
            break;
        } /// for sd
    } /// for idx
}


std::map<i64, Side> EgtbGenDb::perpetuation_evasion(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side, std::set<i64>& idxSet, bool evasion_checking)
{
    assert(rcd.board->isIncheck(side));
    
    Hist hist;

    /// test escape check for the side being checked
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetuation_probe(rcd, hist, false);
        
        std::map<i64, Side> rMap;
        auto score = r.first;

        assert(perpetuation_score_valid(score));
        
        /// it has an option to win by letting opposite be pertuation
        if (score == EGTB_SCORE_UNSET) {
            auto xside = getXSide(side);
            auto checking = evasion_checking && rcd.board->isIncheck(xside);
            assert(r.second >= 0);
            idxSet.insert(r.second);
            rMap = perpetuation_atk(rcd, r.second, xside, idxSet, checking);
            idxSet.erase(r.second);
            assert(idxSet.find(r.second) == idxSet.end());
        }

        rcd.board->takeBack(hist);

        if (!rMap.empty() || score == EGTB_SCORE_PERPETUATION_LOSE) {
//            if (idx == 1025443) {
//                rcd.board->printOut("perpetuation_evasion, idx=" + std::to_string(idx) + ", sd="  + std::to_string(static_cast<int>(side)));
//            }
            
            rMap[idx] = side;
            return rMap;
        }
    }

    return std::map<i64, Side>();
}

std::map<i64, Side> EgtbGenDb::perpetuation_atk(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side, std::set<i64>& idxSet, bool evasion_checking)
{
//    auto d = idx == 38976;
//    if (d) {
//        rcd.board->printOut("perpetuation_atk");
//    }
    Hist hist;
    
    std::map<i64, Side> rMap;
    
    /// for the side checking
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetuation_probe(rcd, hist, true);
        
        auto score = r.first;

        assert(perpetuation_score_valid(score));
        
        auto b = true;

        if (score == EGTB_SCORE_DRAW) {
            b = false;
        } else if (score == EGTB_SCORE_UNSET) {
            auto xside = getXSide(side);
            assert(rcd.board->isIncheck(xside));
            if (idxSet.find(r.second) == idxSet.end()) {
                assert(r.second >= 0);
                idxSet.insert(r.second);
                auto theMap = perpetuation_evasion(rcd, r.second, xside, idxSet, evasion_checking);
                idxSet.erase(r.second);
                
                if (theMap.empty()) {
                    b = false;
                } else if (theMap.size() > rMap.size()) {
                    rMap = theMap;
                }
            } else {
                b = !evasion_checking;
            }
        }

        rcd.board->takeBack(hist);

        if (!b) {
            return std::map<i64, Side>();
        }
    }
    
//    if (idx == 1025443) {
//        rcd.board->printOut("perpetuation_atk, idx=" + std::to_string(idx) + ", sd="  + std::to_string(static_cast<int>(side)));
//    }
    
    rMap[idx] = side;
    return rMap;
}


///////////////
void EgtbGenDb::perpetuation_gen()
{
    std::cout << "perpetuation_gen BEGIN" << std::endl;

    while (true) {
        resetAllThreadRecordCounters();
        
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_gen, this, i));
        }
        
        perpetuation_thread_gen(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();

        auto changeCnt = allThreadChangeCount();
        
        std::cout << "perpetuation_gen changeCnt=" << changeCnt << std::endl;

        if (changeCnt == 0) {
            break;
        }
    }
}


std::pair<int, i64> EgtbGenDb::perpetuation_probe(EgtbGenThreadRecord& rcd, const bslib::Hist& hist, bool drawIfNotIncheck)
{
    std::pair<int, i64> r;
    
    auto side = hist.move.piece.side;
    
    if (rcd.board->isIncheck(side)) {
        r.first = EGTB_SCORE_MISSING;
        return r;
    }
    
    auto xside = getXSide(side);
    if (hist.cap.isEmpty()) {
        r.second = egtbFile->getKey(*rcd.board).key; assert(r.second >= 0);
        r.first = egtbFile->getBufScore(r.second, xside);
        
        /// Use for testing escape opptions
        if (r.first == EGTB_SCORE_UNSET && drawIfNotIncheck && egtbFile->getBufScore(r.second, side) != EGTB_SCORE_ILLEGAL) {
            r.first = EGTB_SCORE_DRAW;
        }
    } else {
        r.second = -1;
        if (rcd.board->pieceList_isThereAttacker()) {
            /// probe a sub-endgame for score
            r.first = getScore(*rcd.board, xside);
            if (r.first == EGTB_SCORE_MISSING) {
                std::lock_guard<std::mutex> thelock(printMutex);
                rcd.board->printOut("Error: missing endagme for probing below board:");
                exit(-1);
            }
            
            if (r.first != EGTB_SCORE_DRAW && drawIfNotIncheck && !rcd.board->isIncheck(xside)) {
                r.first = EGTB_SCORE_DRAW;
            }

        } else {
            r.first = EGTB_SCORE_DRAW;
        }
    }

    return r;
}


int EgtbGenDb::perpetuation_children_probe(GenBoard& board, Side side)
{
    auto legalCount = 0, unsetCount = 0;
    auto bestScore = EGTB_SCORE_UNSET;

    auto xside = getXSide(side);

    Hist hist;
    for(auto && move : board.gen(side)) {
        board.make(move, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            int score;
            if (hist.cap.isEmpty()) {
                auto idx2 = egtbFile->getKey(board).key;
                score = egtbFile->getBufScore(idx2, xside);
            } else {
                if (board.pieceList_isThereAttacker()) {
                    /// probe a sub-endgame for score
                    score = getScore(board, xside);
                    if (score == EGTB_SCORE_MISSING) {
                        std::lock_guard<std::mutex> thelock(printMutex);
                        board.printOut("Error: missing endagme for probing below board:");
                        exit(-1);
                    }
                } else {
                    score = EGTB_SCORE_DRAW;
                }
            }
            
            if (score <= EGTB_SCORE_MATE) {
                score = -score;
                if (score != EGTB_SCORE_DRAW) {
                    if (score > 0) score--;
                    else if (score < 0) score++;
                }
                
                bestScore = bestScore > EGTB_SCORE_MATE ? score : std::max(bestScore, score);
            } else if (score == EGTB_SCORE_PERPETUATION_LOSE || score == EGTB_SCORE_PERPETUATION_WIN) {
                
                score = score == EGTB_SCORE_PERPETUATION_LOSE ? EGTB_SCORE_PERPETUATION_WIN : EGTB_SCORE_PERPETUATION_LOSE;
                
                if (bestScore == EGTB_SCORE_UNSET
                    || (score == EGTB_SCORE_PERPETUATION_WIN && bestScore <= EGTB_SCORE_DRAW)
                    || (score == EGTB_SCORE_PERPETUATION_LOSE && bestScore < EGTB_SCORE_DRAW)
                    ) {
                    bestScore = score;
                }
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

extern bool check2Flip;

void EgtbGenDb::perpetuation_thread_gen(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);
    
    Hist hist;
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        bool caps[2] = {
            egtbFile->flag_is_cap(idx, Side::black),
            egtbFile->flag_is_cap(idx, Side::white)
        };
        
        if (!caps[0] && !caps[1]) {
            continue;
        }

        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);

        auto d = false; // idx == 1348728;
        if (d) {
            rcd.board->printOut("perpetuation_thread_gen");
        }
        for(auto sd = 0; sd < 2; ++sd) {
            if (!caps[sd]) {
                continue;
            }

            auto side = static_cast<Side>(sd);
            auto xside = getXSide(side);
            auto oscore = egtbFile->getBufScore(idx, side);
            
            if (oscore == EGTB_SCORE_PERPETUATION_LOSE
                || (oscore == EGTB_SCORE_PERPETUATION_WIN && egtbFile->getBufScore(idx, xside) == EGTB_SCORE_ILLEGAL)) {
                
            } else {
                rcd.board->printOut("Bug");
            }
            assert(oscore == EGTB_SCORE_PERPETUATION_LOSE
                    || (oscore == EGTB_SCORE_PERPETUATION_WIN && egtbFile->getBufScore(idx, xside) == EGTB_SCORE_ILLEGAL));
            
            egtbFile->flag_clear_cap(idx, side);

            for(auto && move : rcd.board->gen_backward_quiet(xside)) {
                rcd.board->make(move, hist); assert(hist.cap.isEmpty());
                                
                if (!rcd.board->isIncheck(xside)) {
                    assert(rcd.board->isValid());
                    
                    auto rIdx = egtbFile->getKey(*rcd.board).key; assert(rIdx >= 0);
                    auto rScore = egtbFile->getBufScore(rIdx, xside);

                    if (d) {
                        rcd.board->printOut("move made");
                    }
                    
                    if (rScore == EGTB_SCORE_UNSET) {
                        
//                        rcd.board->printOut("propagadaring");
                        auto score = perpetuation_children_probe(*rcd.board, xside);
                        if (score != EGTB_SCORE_UNSET && score != EGTB_SCORE_D9RAW) {
                            
                            if (score <= EGTB_SCORE_MATE) {
                                rcd.board->printOut("problem");
                                std::cout << "score: " << score << std::endl;
                                score = perpetuation_children_probe(*rcd.board, xside);
                            }
                            assert(score == EGTB_SCORE_PERPETUATION_WIN || score == EGTB_SCORE_PERPETUATION_LOSE);
                            
                            score = score == EGTB_SCORE_PERPETUATION_WIN ? EGTB_SCORE_PERPETUATION_LOSE : score == EGTB_SCORE_PERPETUATION_LOSE ? EGTB_SCORE_PERPETUATION_WIN : score;

                            egtbFile->setBufScore(rIdx, score, xside);
                            egtbFile->flag_set_cap(rIdx, xside);

                            rcd.changes++;
                            
                            if (check2Flip) {
                                auto flip = rcd.board->needSymmetryFlip();
                                if (flip != FlipMode::none) {
                                    rcd.board2->clone(rcd.board);
                                    rcd.board2->flip(flip);
                                    assert(rcd.board2->isValid());
                                    auto sIdx = egtbFile->getKey(*rcd.board2).key;
                                    
                                    if (rIdx != sIdx) {
                                        egtbFile->setBufScore(rIdx, score, xside);
                                        egtbFile->flag_set_cap(rIdx, xside);
                                    }
                                }
                            }
                        }
                    }
                    
                }
                
                rcd.board->takeBack(hist);
            }
            
        } /// for sd
    } /// for idx
}


#endif
