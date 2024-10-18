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

#ifdef _DEBUG_PRINT_
extern bool debug_print;
#endif

#ifdef _FELICITY_XQ_

using namespace fegtb;
using namespace bslib;

extern bool check2Flip;
static i64 debugIdx = 768389; // 588400; //-10;
static Side debugSide = Side::white;

static i64 debugCnt = 0;

std::string toResultString(const Result& result);


void EgtbGenDb::printChaseStats()
{
    std::cout << "\tchase_atk_cnt: " << chase_atk_cnt
        << ", evasion_cnt: " << chase_evasion_cnt
        << ", len_max: " << chase_len_max << std::endl;
}

void EgtbGenDb::showData(const std::string& msg)
{
//    showData(msg, 27263, Side::black, false);
//    showData(msg, 24833, Side::white, false);
//    showData(msg, 27246, Side::black, false);
}

void EgtbGenDb::perpetuation_process()
{
    elapsed_perpetuation = 0;
    if (!egtbFile->isBothArmed()) {
        if (egtbVerbose) {
            std::cout << "\tone side not armed!" << std::endl;
        }
        return;
    }
    
#ifdef _FELICITY_FLIP_MAP_
    assert(!flipIdxMap.empty());
#endif
    
    chase_atk_cnt = chase_evasion_cnt = chase_len_max = 0;
    
    showStringWithCurrentTime("\tperpetuation process");
    
    time_perpetuation = Funcs::now();
    
#ifdef _DEBUG_PRINT_
    debug_print = false;
#endif

    egtbFile->clearFlagBuffer();
    
//    showData("Before perpetuation_detect", debugIdx, debugSide, true);

    for (auto lap = 0; ; lap++) {
        auto changes = perpetuation_detect();
        std::cout << "\tperpetuation_process, lap = " << lap << ", changes = " << changes << std::endl;
        if (changes == 0) {
            break;
        }
        
#ifdef _DEBUG_PRINT_
        printChaseStats();
#endif
        showData("Before propaganda", debugIdx, debugSide, false);
        
//        perpetuation_propaganda0();
        perpetuation_propaganda(lap);
    }
    
    {
        /// All done, get some stats
        i64 check_win = 0, check_lose = 0, chase_win = 0, chase_lose = 0;
        for (auto idx = 0; idx < egtbFile->getSize(); idx++) {
            for (auto sd = 0; sd < 2; sd++) {
                auto score = egtbFile->getBufScore(idx, static_cast<Side>(sd));
                
                if (score == EGTB_SCORE_PERPETUAL_CHECK) check_win++;
                else if (score == -EGTB_SCORE_PERPETUAL_CHECK) check_lose++;
                if (score == EGTB_SCORE_PERPETUAL_CHASE) chase_win++;
                else if (score == -EGTB_SCORE_PERPETUAL_CHASE) chase_lose++;
            }
        }
        
        if (egtbVerbose) {
            std::cout << "\tperpetuation_process DONE. #check "
            << "wins: " << check_win << ", losses: " << check_lose
            << "; chase wins: " << chase_win << ", losses: " << chase_lose
            << "; total: " << check_win + check_lose + chase_win + chase_lose
            << std::endl;
        } else {
            auto checks = check_win + check_lose;
            auto chases = chase_win + chase_lose;
            if (checks + chases > 0) {
                std::cout << "\t#perpetual checks: " << checks << ", chases: " << chases << std::endl;
            }
        }
        
    }
    elapsed_perpetuation = Funcs::now() - time_perpetuation;
    total_elapsed_perpetuation += elapsed_perpetuation;
    
    if (egtbVerbose) {
        std::cout << "\tcompleted perpetuation process " << egtbFile->getName() << ", elapse: " << GenLib::formatPeriod(int(elapsed_perpetuation / 1000))
        << std::endl;
    }
}

i64 EgtbGenDb::perpetuation_detect()
{
    if (egtbVerbose) {
        std::cout << "\tperpetuation_detect checks & chases BEGIN" << std::endl;
    }
    
    resetAllThreadRecordCounters();
    
    std::vector<std::thread> threadVec;
    for (auto i = 1; i < threadRecordVec.size(); ++i) {
        threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_detect_checkchase, this, i));
    }

    perpetuation_thread_detect_checkchase(0);

    for (auto && t : threadVec) {
        t.join();
    }
    threadVec.clear();

    auto changeCnt = allThreadChangeCount();
    if (egtbVerbose) {
        std::cout << "\tperpetuation_detect END, changeCnt=" << changeCnt << std::endl;
    }

    return changeCnt;
}


////////
//bool EgtbGenDb::perpetuation_score_valid(int score)
//{
//    return 
//        score == EGTB_SCORE_MISSING     /// actually, it is illegal positions
//        || score == EGTB_SCORE_UNSET
//        || EgtbFile::isPerpetualScore(score)
//        /// wining
//        || (score >= EGTB_SCORE_DRAW && score <= EGTB_SCORE_MATE);
//}


//////////////////////////////////////////////////////////////////////////////////////////
/// Check
//////////////////////////////////////////////////////////////////////////////////////////

/// Perpetual checks
void EgtbGenDb::perpetuation_thread_detect_check(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    rcd.createBoards();
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);
    
    Hist hist;
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };
        
        auto d = false;//idx == 12686640 || idx == 23621659;
        
        if ((scores[0] != EGTB_SCORE_UNSET && scores[1] != EGTB_SCORE_UNSET)
            || (scores[0] != EGTB_SCORE_ILLEGAL && scores[1] != EGTB_SCORE_ILLEGAL)) {
            continue;
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        rcd.board->setFenComplete();
        
        if (d) {
            rcd.board->printOut("board idx=" + std::to_string(idx));
        }

        auto sd = scores[0] == EGTB_SCORE_UNSET ? 0 : 1;
        assert(scores[1 - sd] == EGTB_SCORE_ILLEGAL);
        
        auto side = static_cast<Side>(sd);
        
        assert(rcd.board->isIncheck(side));
        
        std::unordered_map<i64, int> idxplyMap { {idx, 0} };
        auto v = perpetual_check_evasion(rcd, idx, side, idx, idxplyMap, 1, true);
        
        if (d) {
            rcd.board->printOut("perpetuation_thread_detect_check, v sz=" + std::to_string(v.size()));
        }
        
        if (!v.empty()) {
            for(auto && m : v) {
                for(auto && p : m) {
                    auto rIdx = p.first;
                    auto rSide = p.second;
                    
                    auto value = side != rSide ? -EGTB_SCORE_PERPETUAL_CHECK : EGTB_SCORE_PERPETUAL_CHECK;
                    
                    egtbFile->setBufScore(rIdx, value, rSide);
                    auto ok = egtbFile->setupBoard(*rcd.board, rIdx, FlipMode::none, Side::white); assert(ok);
                    
                    auto idx2 = getFlipIdx(rcd, rIdx);
                    if (idx2 >= 0) {
                        assert(rcd.board->needSymmetryFlip() != FlipMode::none);
                        egtbFile->setBufScore(idx2, value, rSide);
                    }
                }
            }
            rcd.changes++;
        }
    } /// for idx
}

/// The side being checked tries to escape
std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_check_evasion(EgtbGenThreadRecord& rcd, const i64 idx, const Side side, const i64 startIdx, std::unordered_map<i64, int>& idxplyMap, int ply, bool evasion_checking)
{
    assert(rcd.board->isIncheck(side));
    
    Hist hist;

    std::vector<std::map<i64, Side>> rVec;
    auto check = true;

    /// test escape check for the side being checked
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetual_check_probe(rcd, hist, false);
        
        auto score = r.first;

        /// it has an option to win by letting opposite be pertuation
//        if (score == -EGTB_SCORE_PERPETUAL_CHECK) {
//            std::map<i64, Side> m;
//            m[idx] = side;
//            rVec.push_back(m);
//        } else if (score < EGTB_SCORE_DRAW) {   /// winning score
//            check = false;
//        } else
        if (score == EGTB_SCORE_UNSET 
            || (EgtbFile::isPerpetualScore(score) && score < 0)) {
            if (idxplyMap.find(r.second) == idxplyMap.end()
                && (move.piece.type != PieceType::pawn || abs(move.dest - move.from) != 9)
                ) {
                assert(r.second >= 0);
                
                auto xside = getXSide(side);
                auto checking = evasion_checking && rcd.board->isIncheck(xside);
                assert(r.second >= 0);
                idxplyMap[r.second] = ply;
                auto v = perpetual_check_attack(rcd, r.second, xside, startIdx, idxplyMap, ply + 1, checking);
                idxplyMap.erase(r.second);
                assert(idxplyMap.find(r.second) == idxplyMap.end());

                if (!v.empty()) {
                    for(auto && m : v) {
                        m[idx] = side;
                        rVec.push_back(m);
                    }
                }
            }
        } else if (score < EGTB_SCORE_DRAW) {   /// winning score
            check = false;
        }

        rcd.board->takeBack(hist);

        if (!check) {
            return std::vector<std::map<i64, Side>>();
        }
    }

    return rVec;
}

std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_check_attack(EgtbGenThreadRecord& rcd, const i64 idx, const Side side, const i64 startIdx, std::unordered_map<i64, int>& idxplyMap, int ply, bool evasion_checking)
{
    Hist hist;
    auto xside = getXSide(side);

    std::vector<std::map<i64, Side>> rVec;

//    if (idxplyMap.size() >= DRAW_LEN / 3) {
//        return rVec;
//    }

    std::vector<std::pair<i64, MoveFull>> list;
    auto d = false; //startIdx == 12686640

    /// for the side checking
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetual_check_probe(rcd, hist, true);
        
        auto score = r.first;

        auto check = true;

        if (score <= EGTB_SCORE_DRAW) {         /// not loss scores
            check = false;
        } else if (score == EGTB_SCORE_UNSET 
                   || EgtbFile::isPerpetualScore(score)
                   ) {
            assert(rcd.board->isIncheck(xside));
            if (move.piece.type == PieceType::pawn && abs(move.dest - move.from) == 9) {
                check = false;
            } else if (idxplyMap.find(r.second) == idxplyMap.end()) {
                /// verify later
                std::pair<i64, MoveFull> p;
                p.first = r.second;
                p.second = move;
                list.push_back(p);

            } else if (r.second != startIdx) {
                check = false;
            } else {
                /// repetition
                assert(r.second >= 0 && rcd.board->isIncheck(xside));
                
                check = !evasion_checking;
                if (check) {
                    std::map<i64, Side> m;
                    m[idx] = side;
                    rVec.push_back(m);
                }
            }
        }

        rcd.board->takeBack(hist);

        if (!check) {
            return std::vector<std::map<i64, Side>>();
        }
    }
    
    for(auto && m : list) {
        rcd.board->make(m.second, hist);
        assert(idxplyMap.find(m.first) == idxplyMap.end());
        assert(m.first >= 0);
        
        idxplyMap[m.first] = ply;
        auto v = perpetual_check_evasion(rcd, m.first, getXSide(side), startIdx, idxplyMap, ply + 1, evasion_checking);
        idxplyMap.erase(m.first);
        
        rcd.board->takeBack(hist);
        
        if (v.empty()) {
            /// this move is a better alternative -> no chase
            rVec.clear();
            break;
        }
        for(auto && m : v) {
            m[idx] = side;
            rVec.push_back(m);
        }
    }

    return rVec;
}



std::pair<int, i64> EgtbGenDb::perpetual_check_probe(EgtbGenThreadRecord& rcd, const bslib::Hist& hist, bool drawIfNotIncheck)
{
    std::pair<int, i64> r;
    
    auto side = hist.move.piece.side;

    /// illegal move
    if (rcd.board->isIncheck(side)) {
        r.first = EGTB_SCORE_ILLEGAL;
        return r;
    }

    auto xside = getXSide(side);
    if (hist.cap.isEmpty()) {
        r.second = egtbFile->getIdx(*rcd.board).key; assert(r.second >= 0);
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
            
        } else {
            r.first = EGTB_SCORE_DRAW;
        }
    }

    return r;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Check & chase
//////////////////////////////////////////////////////////////////////////////////////////
/// Will flip the board
void EgtbGenDb::perpetuation_fill(EgtbGenThreadRecord& rcd, std::vector<std::map<i64, Side>>& v, const Side side, bool check)
{
    for(auto && m : v) {
        for(auto && p : m) {
            auto rIdx = p.first;
            auto rSide = p.second;
            
            int value;
            
            if (check) {
                value = side != rSide ? -EGTB_SCORE_PERPETUAL_CHECK : EGTB_SCORE_PERPETUAL_CHECK;
            } else {
                value = side != rSide ? -EGTB_SCORE_PERPETUAL_CHASE : EGTB_SCORE_PERPETUAL_CHASE;
            }
            
            egtbFile->setBufScore(rIdx, value, rSide);
            egtbFile->flag_clear_cap(rIdx, rSide);

            assert(rcd.board);
            auto ok = egtbFile->setupBoard(*rcd.board, rIdx, FlipMode::none, Side::white); assert(ok);
            
            auto idx2 = getFlipIdx(rcd, rIdx);
            if (idx2 >= 0) {
                assert(rcd.board->needSymmetryFlip() != FlipMode::none);
                egtbFile->setBufScore(idx2, value, rSide);
                egtbFile->flag_clear_cap(idx2, rSide);
            }
        }
    }
}


void EgtbGenDb::perpetuation_thread_detect_checkchase(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    rcd.createBoards();
    assert(rcd.fromIdx < rcd.toIdx && rcd.board && rcd.board2);
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };

        /// cap flag is caputre with perpetual scores
        bool caps[2] = {
            egtbFile->flag_is_cap(idx, Side::black),
            egtbFile->flag_is_cap(idx, Side::white)
        };

        auto d = idx == debugIdx;
        if (d) {
            std::cout << std::endl;
        }

        if (scores[0] != EGTB_SCORE_UNSET && !caps[0] && !EgtbFile::isPerpetualForwardScore(scores[0])
            && scores[1] != EGTB_SCORE_UNSET && !caps[1] && !EgtbFile::isPerpetualForwardScore(scores[1])) {
            continue;
        }

        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        rcd.board->setFenComplete();

        if (d) {
//            rcd.board->printOut();
            showData("perpetuation_thread_detect_checkchase", debugIdx, debugSide, true);
        }

        for(auto sd = 0; sd < 2; ++sd) {
            if (scores[sd] == EGTB_SCORE_UNSET
                || EgtbFile::isPerpetualForwardScore(scores[sd])
                || caps[sd]) {
                auto side = static_cast<Side>(sd);
                std::unordered_map<i64, int> idxplyMap { { idx, 0 } };

                /// check
                if (scores[1 - sd] == EGTB_SCORE_ILLEGAL) {
                    assert(rcd.board->isIncheck(side));
                    
                    auto v = perpetual_check_evasion(rcd, idx, side, idx, idxplyMap, 1, true);
                    
//                    if (d) {
//                        showData("hey", idx, side, true);
//                        rcd.board->printOut();
//                        
//                        showData("perpetuation_thread_detect_checkchase v sz=" + std::to_string(v.size()), debugIdx, debugSide, true);
//                    }

                    if (!v.empty()) {
                        perpetuation_fill(rcd, v, side, true);
                        rcd.changes++;
                        break;
                    }
                }

                
                /// chase
                {
                    XqChaseJudge xqChaseJudge;
                    
                    auto v = perpetual_chase_evasion(rcd, idx, side, idx, idxSet, xqChaseJudge);
                    
                    if (!v.empty()) {
                        perpetuation_fill(rcd, v, side, false);
                        rcd.changes++;
                    }
                }

            }
        }
    } /// for idx
}



//////////////////////////////////////////////////////////////////////////////////////////
/// Chase
//////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenDb::perpetuation_debug()
{
    std::cout << "perpetuation_debug BEGIN..." << std::endl;
    
    i64 idx = debugIdx;
    auto side = debugSide;
    
    int scores[2] = {
        egtbFile->getBufScore(idx, Side::black),
        egtbFile->getBufScore(idx, Side::white)
    };
    
    auto& rcd = threadRecordVec.at(0);
    rcd.createBoards();

    auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
    rcd.board->setFenComplete();
    
    rcd.board->printOut("Starting");
    
    auto sd = static_cast<int>(side);
    assert (scores[sd] == EGTB_SCORE_UNSET
            && !egtbFile->flag_is_side(idx, side));
    
//    perpetual_chase(rcd, idx, side);
    
    std::cout << "perpetuation_debug END." << std::endl;

    exit(0);
}

void EgtbGenDb::perpetuation_thread_detect_chase(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);

    Hist hist;

    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };
                
        if (scores[0] != EGTB_SCORE_UNSET && scores[1] != EGTB_SCORE_UNSET) {
            continue;
        }

        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        rcd.board->setFenComplete();
        
        for(auto sd = 0; sd < 2; ++sd) {
            auto side = static_cast<Side>(sd);
            if (scores[sd] == EGTB_SCORE_UNSET
//                && !egtbFile->flag_is_side(idx, side)
                ) {
//                perpetual_chase(rcd, idx, side);
                                
                std::set<i64> idxSet { idx };
                XqChaseJudge xqChaseJudge;

                auto v = perpetual_chase_evasion(rcd, idx, side, idx, idxSet, xqChaseJudge);
                
                if (!v.empty()) {
                    perpetuation_fill(rcd, v, side, false);
                    rcd.changes++;
                }
            }
        }
    } /// for idx
}

//std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_chase(EgtbGenThreadRecord& rcd, const i64 idx, const Side side)
//{
//    std::set<i64> idxSet { idx };
//    XqChaseJudge xqChaseJudge;
//
//    auto v = perpetual_chase_evasion(rcd, idx, side, idx, idxSet, xqChaseJudge);
//    
//    if (v.empty()) {
//        egtbFile->flag_set_side(idx, side);
//    } else {
//        perpetuation_fill(rcd, v, side, false);
//        rcd.changes++;
//    }
//    return v;
//}


/*
 * The side tries to escape its pieces from being chased.
 * If it has a chance, it will remain the status of being chased
 * for further moves
 */
std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_chase_evasion(EgtbGenThreadRecord& rcd, const i64 idx, const Side side, const i64 startIdx, std::set<i64>& idxSet, XqChaseJudge& chaseJudge)
{
    chase_evasion_cnt++;
    
    std::vector<std::map<i64, Side>> rVec;
    
    if (!chaseJudge.addBoard(*rcd.board, side)) {
        chaseJudge.removeLastBoard();
        return rVec;
    }

    chase_len_max = std::max<i64>(chase_len_max, chaseJudge.addingCnt);
    auto beingChased = true;

    /// test escape chasing for the side being chased
    Hist hist;
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetual_chase_probe(rcd, hist);
        
        auto score = r.first; /// in the view of xside
        
        if (score == EGTB_SCORE_UNSET
            || (EgtbFile::isPerpetualScore(score) && score < 0 && score != -EGTB_SCORE_PERPETUAL_CHECK)
            ) {

            if (idxSet.find(r.second) == idxSet.end()
                && (move.piece.type != PieceType::pawn || abs(move.dest - move.from) != 9)
                ) {
                assert(r.second >= 0);
                idxSet.insert(r.second);
                auto v = perpetual_chase_attack(rcd, r.second, getXSide(side), startIdx, idxSet, chaseJudge);
                idxSet.erase(r.second);
                assert(idxSet.find(r.second) == idxSet.end());

                if (!v.empty()) {
                    for(auto && m : v) {
                        m[idx] = side;
                        rVec.push_back(m);
                    }
                }
            }

        } else if (score < EGTB_SCORE_DRAW) { /// wining scores
            beingChased = false;
            rVec.clear();
        }

        rcd.board->takeBack(hist);
        
        if (!beingChased) {
            break;
        }
    }

    chaseJudge.removeLastBoard();
    return rVec;
}

/*
 * The side tries to chase some pieces
 * if it has any chance to be win or draw, it will take instead
 */
std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_chase_attack(EgtbGenThreadRecord& rcd, const i64 idx, const Side side, const i64 startIdx, std::set<i64>& idxSet, XqChaseJudge& chaseJudge)
{
    chase_atk_cnt++;
    auto d = false;//startIdx == debugIdx;
    if (d) {
        debugCnt++;
    }
    
    std::vector<std::map<i64, Side>> rVec;
    if (idxSet.size() >= DRAW_LEN / 3) {
        return rVec;
    }
    
    if (!chaseJudge.addBoard(*rcd.board, side)) {
        chaseJudge.removeLastBoard();
        return rVec;
    }
    chase_len_max = std::max<i64>(chase_len_max, chaseJudge.addingCnt);
    
    if (d) {
        rcd.board->printOut("chase_attack in progress, startIdx=" + std::to_string(startIdx) +
                            ", idx=" + std::to_string(idx) + ", side=" + Funcs::side2String(side));
    }
    
    Hist hist;
    auto chasing = true;
    
    /// Find any better alternative moves, before going deeper
    std::vector<std::pair<i64, MoveFull>> list;
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetual_chase_probe(rcd, hist);
        
        auto score = r.first; /// score in xside view
        
        if (score == EGTB_SCORE_UNSET
            || (score > 0 && EgtbFile::isPerpetualScore(score))
            ) {
            if (move.piece.type == PieceType::pawn && abs(move.dest - move.from) == 9) {
                chasing = false;
            } else {
                
                /// no repetition - will be visted later in the next step
                if (idxSet.find(r.second) == idxSet.end()
                    ) {
                    std::pair<i64, MoveFull> p;
                    p.first = r.second;
                    p.second = move;
                    list.push_back(p);
                } else {
                    /// repetition
                    if (r.second != startIdx) {
                        chasing = false;
                    } else {
                        /// the position is in a repetition -> query chase result
                        auto r = chaseJudge.evaluate();
                        chasing = r.result == GameResultType::win || r.result == GameResultType::loss;
                        if (chasing) {
                            std::map<i64, Side> m;
                            m[idx] = side;
                            rVec.push_back(m);
                        }
                    }
                }
            }
            
        } else if (score <= EGTB_SCORE_DRAW) {     /// not lossing scores
            chasing = false;
        }
        
        rcd.board->takeBack(hist);
                
        if (!chasing) {
            /// if there is any better alternative/draw -> return empty
            chaseJudge.removeLastBoard();
            return std::vector<std::map<i64, Side>>();
        }
    }

    
    for(auto && m : list) {
        rcd.board->make(m.second, hist);
        assert(idxSet.find(m.first) == idxSet.end());
        assert(m.first >= 0);
        
        idxSet.insert(m.first);
        auto v = perpetual_chase_evasion(rcd, m.first, getXSide(side), startIdx, idxSet, chaseJudge);
        idxSet.erase(m.first);
        
        rcd.board->takeBack(hist);
        
        if (v.empty()) {
            /// this move is a better alternative -> no chase
            rVec.clear();
            break;
        }
        for(auto && m : v) {
            m[idx] = side;
            rVec.push_back(m);
        }
    }
    
    chaseJudge.removeLastBoard();
    return rVec;
} /// perpetual_chase_attack

/// return score-idx
std::pair<int, i64> EgtbGenDb::perpetual_chase_probe(EgtbGenThreadRecord& rcd, const bslib::Hist& hist)
{
    std::pair<int, i64> r;
    
    auto side = hist.move.piece.side;
    
    /// illegal move
    if (rcd.board->isIncheck(side)) {
        r.first = EGTB_SCORE_MISSING;
        return r;
    }
    
    auto xside = getXSide(side);
    if (hist.cap.isEmpty()) {
        r.second = egtbFile->getIdx(*rcd.board).key; assert(r.second >= 0);
        r.first = egtbFile->getBufScore(r.second, xside);
    } else { /// capturing
        r.second = -1;
        if (rcd.board->pieceList_isThereAttacker()) {
            /// probe a sub-endgame for score
            r.first = getScore(*rcd.board, xside);
            if (r.first == EGTB_SCORE_MISSING) {
                std::lock_guard<std::mutex> thelock(printMutex);
                rcd.board->printOut("Error: missing endagme for probing the board:");
                exit(-1);
            }
        } else {
            r.first = EGTB_SCORE_DRAW;
        }
    }

    return r;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Propaganda
//////////////////////////////////////////////////////////////////////////////////////////

void EgtbGenDb::perpetuation_propaganda(int lap)
{
    resetAllThreadRecordCounters();
    egtbFile->clearFlagBuffer();

    /// Init
    {
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_propaganda_init, this, i, lap));
        }
        
        perpetuation_thread_propaganda_init(0, lap);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }

    showData("After perpetuation_thread_propaganda_init", debugIdx, debugSide, false);
    
    auto maxPly = 0;
    for(auto && rcd : threadRecordVec) {
        maxPly = std::max(maxPly, int(rcd.n));
    }
    maxPly++;
    

    i64 allChangeCnt = 0;
    auto side = Side::black;
    for(auto tryCnt = 2, ply = 0; ply < maxPly; ply++, side = getXSide(side)) {
        
        resetAllThreadRecordCounters();
        
//        /// Clear all side-marks
//        for(i64 idx = 0; idx < egtbFile->getSize() / 2 + 1; idx++) {
//            egtbFile->flags[idx] &= ~(3 | 3 << 4);
//        }
        
        /// Fill positions by two phrases and two sides, we should not combine them info one to avoid being
        /// conflicted on writting between threads
        /// phase 0: fill winning positions, mark losing positions by retro/backward moves
        /// phase 1: probe and fill marked positions

        for(auto phase = 0; phase < 2; phase++) {
            for(auto sd = 0; sd < 2; sd++) {
                
                std::vector<std::thread> threadVec;
                for (auto i = 1; i < threadRecordVec.size(); ++i) {
                    threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_propaganda, this, i, ply, sd, phase));
                }
                
                perpetuation_thread_propaganda(0, ply, sd, phase);
                
                for (auto && t : threadVec) {
                    t.join();
                }
                
                showData("After a lap perpetuation_thread_propaganda lap = " + std::to_string(lap), debugIdx, debugSide, true);

            }
        }

        auto changeCnt = allThreadChangeCount();
        allChangeCnt += changeCnt;
        //if (egtbVerbose) {
            std::cout << " Ply: " << ply << ", perpetuation_propaganda changeCnt = " << changeCnt << std::endl;
        //}
        
        if (changeCnt == 0 && ply + 1 >= maxPly) {
            maxPly++;
        }
    }
    
    showData("After a call perpetuation_thread_propaganda", debugIdx, debugSide, true);

}


void EgtbGenDb::perpetuation_thread_propaganda_init(int threadIdx, int lap)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);
    rcd.n = 0;
    
    Hist hist;
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };
        
        for(auto sd = 0; sd < 2; ++sd) {
            if (!EgtbFile::isPerpetualScore(scores[sd])) {
                continue;
            }
            auto aScore = std::abs(scores[sd]);
            if (aScore < EGTB_SCORE_PERPETUAL_CHASE) {
                auto n = EGTB_SCORE_PERPETUAL_CHASE - aScore;
                assert(n > 0 && n <= DRAW_LEN);
                rcd.n = std::max<i64>(n, rcd.n);
            }
        }
        
        if (lap > 0 ||
            (scores[0] != EGTB_SCORE_UNSET && scores[1] != EGTB_SCORE_UNSET)) {
            continue;
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        
        /// perpetual captures, set flag caps
        for(auto sd = 0; sd < 2; ++sd) {
            if (scores[sd] != EGTB_SCORE_UNSET) {
                continue;
            }
            
            auto side = static_cast<Side>(sd);
            auto xside = static_cast<Side>(1 - sd);
            
            auto bestScore = EGTB_SCORE_UNSET;
            for(auto && move : rcd.board->gen(side)) {
                if (rcd.board->isEmpty(move.dest)) {
                    continue;
                }
                
                rcd.board->make(move, hist);
                assert(!hist.cap.isEmpty());
                if (!rcd.board->isIncheck(side)) {
                    auto score = getScore(*rcd.board, xside);
                    EgtbFile::pickBestFromRivalScore(bestScore, score);
                }
                rcd.board->takeBack(hist);
            }
            
            if (EgtbGenFile::isPerpetualScore(bestScore)) {
                auto vec = setBufScore(rcd, idx, bestScore, side);
                egtbFile->flag_set_cap(idx, side);
                
                if (vec.size() > 1) {
                    egtbFile->flag_set_cap(vec[1], side);
                }
                
                auto aScore = std::abs(bestScore);
                if (aScore < EGTB_SCORE_PERPETUAL_CHASE) {
                    auto n = EGTB_SCORE_PERPETUAL_CHASE - aScore;
                    assert(n > 0 && n <= DRAW_LEN);
                    rcd.n = std::max<i64>(n, rcd.n);
                }

            }
        } /// for sd
    }
}



void EgtbGenDb::perpetuation_thread_propaganda(int threadIdx, int ply, int sd, int phase)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);
    
    Hist hist;
    
    const auto score_check = EGTB_SCORE_PERPETUAL_CHECK - ply;
    const auto score_chase = EGTB_SCORE_PERPETUAL_CHASE - ply;
    auto fillScore_check = -score_check + 1;
    if (ply == 0) {
        fillScore_check++;
    }
    const auto fillScore_chase = -score_chase + 1;
    
    const auto side = static_cast<Side>(sd), xside = getXSide(side);
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        
        if (phase == 0) {
            auto oScore = egtbFile->getBufScore(idx, side);
            if (!EgtbFile::isPerpetualScore(oScore)) {
                continue;
            }

            auto aScore = std::abs(oScore);
            
            /// process captures
            if (egtbFile->flag_is_cap(idx, side)) {
                if (aScore == std::abs(fillScore_check) || aScore == std::abs(fillScore_chase)) {
                    auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
                    auto idx2 = getFlipIdx(rcd, idx);
                    
                    if (oScore > 0) {
                        egtbFile->flag_clear_side(idx, side);
                        egtbFile->setBufScore(idx, oScore, side);
                        egtbFile->flag_clear_cap(idx, side);
                        if (idx2 >= 0) {
                            egtbFile->setBufScore(idx2, oScore, side);
                            egtbFile->flag_clear_cap(idx2, side);
                            egtbFile->flag_clear_side(idx2, side);
                        }
                        rcd.changes++;
                    } else {
                        egtbFile->flag_set_side(idx, side);
                        if (idx2 >= 0) {
                            egtbFile->flag_set_side(idx2, side);
                        }
                        
                    }
                } /// if (oScore == fillScore_check
                else if ((score_check == oScore || score_chase == oScore) && oScore > 0)
                {
                    egtbFile->flag_clear_side(idx, side);
                    
                    auto vec = setBufScore(rcd, idx, oScore, side);
                    assert(vec.size() > 0 && vec.size() <= 2);
                    egtbFile->flag_clear_cap(idx, side);
                    if (vec.size() > 1) {
                        egtbFile->flag_clear_cap(vec[1], side);
                        egtbFile->flag_clear_side(vec[1], side);
                    }
                    rcd.changes++;
                    
                }
                
                continue;
            }  /// flag_is_cap
            
            /// In phase 0, consider only positions with perpetual check, chase
            if (aScore != score_check && aScore != score_chase) {
                continue;
            }
            
            auto fillScore = aScore == score_check ? fillScore_check : fillScore_chase;
            
            if (oScore != aScore) {
                fillScore = -fillScore;
            }
            
            auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
            
            for(auto && move : rcd.board->gen_backward_quiet(xside)) {
                
                rcd.board->make(move, hist); assert(hist.cap.isEmpty());
                
                if (!rcd.board->isIncheck(side)) {
                    
                    assert(rcd.board->isValid());
                    
                    auto rIdx = egtbFile->getIdx(*rcd.board).key; assert(rIdx >= 0);
                    
                    auto rScore = egtbFile->getBufScore(rIdx, xside);
                    
                    if (rScore != EGTB_SCORE_ILLEGAL) {
                        
                        /// Winning score will be filled right now (they are parents's positions of the given one)
                        if (fillScore > 0) {
                            if (rScore == EGTB_SCORE_UNSET
                                || egtbFile->flag_is_cap(rIdx, xside)
                                || rScore == fillScore
                                || EgtbFile::isSmallerScore(rScore, fillScore)
                                ) {
                                egtbFile->flag_clear_cap(rIdx, xside);
                                egtbFile->flag_clear_side(rIdx, xside);

                                auto vec = setBufScore(rcd, rIdx, fillScore, xside);
                                assert(vec.size() > 0 && vec.size() <= 2);
                                egtbFile->flag_clear_cap(rIdx, xside);
                                if (vec.size() > 1) {
                                    egtbFile->flag_clear_cap(vec[1], xside);
                                    egtbFile->flag_clear_side(vec[1], xside);
                                }
                                rcd.changes++;
                            }
                            
                            
                        } else if (rScore == EGTB_SCORE_UNSET || egtbFile->flag_is_cap(rIdx, xside)) {
                            /// Losing positions, mark them to consider later in phase 1
                            egtbFile->flag_set_side(rIdx, xside);
                            
                            auto sIdx = getFlipIdx(rcd, rIdx);
                            if (sIdx >= 0) {
                                egtbFile->flag_set_side(sIdx, xside);
                            }
                        }
                    } /// if (rScore != EGTB_SCORE_ILLEGAL)
                }
                
                rcd.board->takeBack(hist);
            }
            
            continue;
        } /// if phase == 0
        
        /// phase == 1
        if (!egtbFile->flag_is_side(idx, side)) {
            continue;
        }
        
        /// phase 1 - work with marked positions only, they are lossing ones
        /// those positions have at least one lossing child but they may have
        /// better choices/children to draw or win back. We need to probe fully
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        
        auto bestScore = perpetuation_propaganda_probe(*rcd.board, side);
        
        if (bestScore != EGTB_SCORE_UNSET) {
            egtbFile->flag_clear_side(idx, side);
            
            auto vec = setBufScore(rcd, idx, bestScore, side);
            assert(vec.size() > 0 && vec.size() <= 2);
            egtbFile->flag_clear_cap(idx, side);
            egtbFile->flag_clear_side(idx, side);

            if (vec.size() > 1) {
                egtbFile->flag_clear_cap(vec[1], side);
                egtbFile->flag_clear_side(vec[1], side);
            }
            rcd.changes++;
        }
    }
}

/////////////////////////
/*
int EgtbGenDb::perpetuation_thread_propaganda_set_parent_flag(EgtbGenThreadRecord& rcd, i64 idx, GenBoard* board, Side side, int oscore)
 {
     Hist hist;
     auto xside = getXSide(side);
     
     auto cnt = 0;
     
     auto d = false;//idx == 297104599;
     if (d) {
         board->printOut("perpetuation_thread_propaganda_set_parent_flag, starting");
     }
     
     for(auto && move : board->gen_backward_quiet(side)) {
         board->make(move, hist); assert(hist.cap.isEmpty());
         
         if (!board->isIncheck(xside)) {
             assert(board->isValid());
             
             auto rIdx = egtbFile->getIdx(*board).key; assert(rIdx >= 0);
             
             /// parents
             auto sc = egtbFile->getBufScore(rIdx, side);
             auto asc = std::abs(sc);
             
             if (d) {
                 std::string s = "set_parent_flag, idx=" + std::to_string(idx)
                 + ", move=" + board->toString(move)
                 + ", rIdx=" + std::to_string(rIdx)
                 + ", sc=" + std::to_string(sc);

                 board->printOut(s);
             }
             
             if (sc == EGTB_SCORE_UNSET
                 || (EgtbFile::isPerpetualScore(sc)
 //                    && sc != -oscore
                     && asc != EGTB_SCORE_PERPETUAL_CHECK && asc != EGTB_SCORE_PERPETUAL_CHASE)
                 ) {
                 egtbFile->flag_set_cap(rIdx, side);
                 cnt++;
                 auto sIdx = getFlipIdx(rcd, rIdx);
                 if (sIdx >= 0) {
                     egtbFile->flag_set_cap(sIdx, side);
                     cnt++;
                 }
             }
             
         }
         
         board->takeBack(hist);
     }
     rcd.changes += cnt;
     return cnt;
 }



void EgtbGenDb::perpetuation_propaganda0()
{
    if (egtbVerbose) {
        std::cout << "\tperpetuation_propaganda BEGIN" << std::endl;
    }
    
    resetAllThreadRecordCounters();
    egtbFile->clearFlagBuffer();
    
    /// Init
    {
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_propaganda_init0, this, i));
        }
        
        perpetuation_thread_propaganda_init0(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }
    
    for(auto ply = 0; ply < 200; ply++) {
        resetAllThreadRecordCounters();
        
        /// convert flags
        for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            for(auto sd = 0; sd < 2; sd++) {
                auto side = static_cast<Side>(sd);
                if (egtbFile->flag_is_cap(idx, side)) {
                    egtbFile->flag_set_side(idx, side);
                    egtbFile->flag_clear_cap(idx, side);
                }
            }
        }
        
        
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_propaganda0, this, i, ply));
        }
        
        perpetuation_thread_propaganda0(0, ply);
        
        for (auto && t : threadVec) {
            t.join();
        }
        
        auto changeCnt = allThreadChangeCount();
        
        if (egtbVerbose) {
            std::cout << "\tPly: " << ply << ", changeCnt = " << changeCnt << std::endl;
        }
        
        if (changeCnt == 0) {
            break;
        }
    }
    
    if (egtbVerbose) {
        std::cout << "\tperpetuation_propaganda DONE!" << std::endl;
    }
}

void EgtbGenDb::perpetuation_thread_propaganda_init0(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);
    
    Hist hist;
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };
        
        if (scores[0] == EGTB_SCORE_ILLEGAL && scores[1] == EGTB_SCORE_ILLEGAL) {
            continue;
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        
        auto d = false; //idx == debugIdx;
        if (d) {
            rcd.board->printOut("perpetuation_thread_propaganda_init");
        }
        
        for(auto sd = 0; sd < 2; ++sd) {
            auto side = static_cast<Side>(sd);
            auto xside = static_cast<Side>(1 - sd);
            if (EgtbGenFile::isPerpetualScore(scores[sd])) {
                rcd.changes += perpetuation_thread_propaganda_set_parent_flag(rcd, idx, rcd.board, xside, scores[sd]);
            }
            
            /// If the position can capture and it is a perpetuations, mark itself
            if (!rcd.board->isIncheck(xside)) {
                auto b = false;
                for(auto && move : rcd.board->gen(side)) {
                    if (rcd.board->isEmpty(move.dest)) {
                        continue;
                    }
                    
                    rcd.board->make(move, hist);
                    if (!rcd.board->isIncheck(side)) {
                        assert(!hist.cap.isEmpty());
                        auto score = getScore(*rcd.board, xside);
                        
                        if (d) {
                            rcd.board->printOut("perpetuation_thread_propaganda_init, score: " + std::to_string(score));
                        }
                        
                        if (EgtbGenFile::isPerpetualScore(score)) {
                            egtbFile->flag_set_side(idx, side);
                            b = true;
                        }
                    }
                    rcd.board->takeBack(hist);
                    if (b) {
                        break;
                    }
                }
            }
            
        } /// for sd
    } /// for idx
}

void EgtbGenDb::perpetuation_thread_propaganda0(int threadIdx, int ply)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);
    
    Hist hist;
    
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        bool flags[2] = {
            egtbFile->flag_is_side(idx, Side::black),
            egtbFile->flag_is_side(idx, Side::white)
        };

        auto d = false; //idx == 136753;
        
        if (d) {
            d = d;
        }

        if (!flags[0] && !flags[1]) {
            continue;
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        
        if (d) {
            rcd.board->printOut("Imhere");
            showData("", idx, Side::black, true);
        }
        for(auto sd = 0; sd < 2; ++sd) {
            if (!flags[sd]) {
                continue;
            }
            
            auto side = static_cast<Side>(sd);
            
            auto score = perpetuation_propaganda_probe(*rcd.board, side);
            
            if (idx == debugIdx) {
                std::cout << "perpetuation_thread_propaganda0, idx=" << idx << ", score=" << score << std::endl;
            }

            if (score == EGTB_SCORE_UNSET) {
                continue;
            }
            assert(!EgtbFile::isPerpetualScoreOver120(score));
            
            auto sc = egtbFile->getBufScore(idx, side);
            if (score == sc) {
                egtbFile->flag_clear_side(idx, side);
                auto sIdx = getFlipIdx(rcd, idx);
                if (sIdx >= 0) {
                    egtbFile->flag_clear_side(sIdx, side);
                }
                continue;
            }
            
            auto vec = setBufScore(rcd, idx, score, side);
            assert(vec.size() > 0 && vec.size() <= 2);
            egtbFile->flag_clear_side(idx, side);
            
            if (vec.size() > 1) {
                egtbFile->flag_clear_side(vec[1], side);
            }
            
            if (vec.front() == debugIdx || vec.back() == debugIdx) {
                auto b = EgtbGenFile::isPerpetualScore(score);
                showData("Set data score = " + std::to_string(score) + ", b=" + std::to_string(b), vec.front(), Side::none, true);
            }
            
            if (EgtbGenFile::isPerpetualScore(score))
            {
                auto xside = static_cast<Side>(1 - sd);
                perpetuation_thread_propaganda_set_parent_flag(rcd, idx, rcd.board, xside, score);
            }
            
            rcd.changes++;
        } /// for sd
    } /// for idx
}
*/

int EgtbGenDb::perpetuation_propaganda_probe(GenBoard& board, Side side)
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
                auto idx2 = egtbFile->getIdx(board).key;
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
            
            
            if (score == EGTB_SCORE_UNSET) {
                unsetCount++;
            } else {
                if (score <= EGTB_SCORE_MATE && score != EGTB_SCORE_DRAW) {
                    score = -score;
                    if (score == EGTB_SCORE_PERPETUAL_CHECK || score == -EGTB_SCORE_PERPETUAL_CHECK) {
                        if (score > 0) score -= 2;
                        else score += 2;
                    } else {
                        if (score > 0) score -= 1;
                        else score += 1;
                        if (EgtbFile::isPerpetualScoreOver120(score)) {
                            score = EGTB_SCORE_UNSET;
                            unsetCount++;
                        }
                    }
                    
                }
                
                if (score != EGTB_SCORE_UNSET && EgtbFile::isSmallerScore(bestScore, score)) {
                    bestScore = score;
                }
            }
        }
        board.takeBack(hist);
    }

    assert(legalCount > 0);

    if (bestScore > 0 && bestScore <= EGTB_SCORE_MATE) {
        return bestScore;
    }

    return unsetCount ? EGTB_SCORE_UNSET : bestScore;
}

#endif
