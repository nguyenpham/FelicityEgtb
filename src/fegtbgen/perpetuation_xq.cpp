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
static i64 debugIdx = -1; //123355051; // 3aka3/7R1/7c1/9/9/9/9/9/4A4/3AK1p2 b 0 1
static Side debugSide = Side::black;

static i64 debugIdx2 = -1;//23558020;
static i64 debugCnt = 0;

std::string toResultString(const Result& result);

static i64 chase_atk_cnt = 0, chase_evasion_cnt = 0, chase_len_max = 0;


void printChaseStats()
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
//    if (!egtbFile->isBothArmed()) {
//        if (egtbVerbose) {
//            std::cout << "\tOne side is not armed!" << std::endl;
//        }
//        return;
//    }
    
#ifdef _FELICITY_FLIP_MAP_
    assert(!flipIdxMap.empty());
#endif
    
    showStringWithCurrentTime("\tperpetuation process");
    
    time_perpetuation = Funcs::now();
    
#ifdef _DEBUG_PRINT_
    debug_print = false;
#endif

//    showData("Before perpetuation_detect", debugIdx, debugSide, false);
    
    showData("Before perpetuation_detect");

//    /// 1 thread only
//    {
//        while(threadRecordVec.size() > 1) {
//            threadRecordVec.pop_back();
//        }
//        threadRecordVec[0].toIdx = egtbFile->getSize();
//        std::cout << "WARNING: 1 thread only" << std::endl;
//    }

//    perpetuation_debug();

    if (perpetuation_detect() != 0) {
        
#ifdef _DEBUG_PRINT_
        printChaseStats();
#endif
//        showData("Before propaganda", debugIdx, debugSide, false);
        showData("Before propaganda");
        
        {
            EgtbFileStats stats;
            stats.createStats(egtbFile);
            std::cout << "Before propaganda, perpCnt = " << stats.perpCnt << std::endl;
        }

        perpetuation_propaganda();
        
        {
            EgtbFileStats stats;
            stats.createStats(egtbFile);
            std::cout << "After propaganda, perpCnt = " << stats.perpCnt << std::endl;
        }
    }
    
//    showData("After propaganda", debugIdx, debugSide, true);

    showData("After propaganda");

    {
        /// All done, get some stats
        i64 check_win = 0, check_lose = 0, chase_win = 0, chase_lose = 0;
        for (auto idx = 0; idx < egtbFile->getSize(); idx++) {
            for (auto sd = 0; sd < 2; sd++) {
                auto score = egtbFile->getBufScore(idx, static_cast<Side>(sd));
                
                if (score == EGTB_SCORE_PERPETUAL_CHECK_WIN) check_win++;
                else if (score == EGTB_SCORE_PERPETUAL_CHECK_LOSS) check_lose++;
                if (score == EGTB_SCORE_PERPETUAL_CHASE_WIN) chase_win++;
                else if (score == EGTB_SCORE_PERPETUAL_CHASE_LOSS) chase_lose++;
            }
        }
        
        if (egtbVerbose) {
            std::cout << "Perpetuation_process DONE. #check "
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
    
//    std::cerr << "WARNING: stop here!" << std::endl;
//    exit(0);
}

i64 EgtbGenDb::perpetuation_detect()
{
    resetAllThreadRecordCounters();

    /// Perpetual checks
    {
        if (egtbVerbose) {
            std::cout << "\tperpetuation_detect checks BEGIN" << std::endl;
        }
        
        egtbFile->clearFlagBuffer();

        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_detect_check, this, i));
        }

        perpetuation_thread_detect_check(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }
    
    /// Perpetual chases
    {
        if (egtbVerbose) {
            std::cout << "\tperpetuation_detect chases BEGIN" << std::endl;
        }
        
        egtbFile->clearFlagBuffer();

        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_detect_chase, this, i));
        }

        perpetuation_thread_detect_chase(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }

    if (egtbVerbose) {
        std::cout << "\tperpetuation_detect END." << std::endl;
    }
    
    return allThreadChangeCount();
}


//////
bool EgtbGenDb::perpetuation_score_valid(int score)
{
    return 
        score == EGTB_SCORE_MISSING     /// actually, it is illegal positions
        || score == EGTB_SCORE_UNSET
        || EgtbFile::isPerpetualScore(score)
        /// wining
        || (score >= EGTB_SCORE_DRAW && score <= EGTB_SCORE_MATE);
}


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
        
        if ((scores[0] != EGTB_SCORE_UNSET && scores[1] != EGTB_SCORE_UNSET)
            || (scores[0] != EGTB_SCORE_ILLEGAL && scores[1] != EGTB_SCORE_ILLEGAL)) {
            continue;
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        rcd.board->setFenComplete();
        
        auto sd = scores[0] == EGTB_SCORE_UNSET ? 0 : 1;
        assert(scores[1 - sd] == EGTB_SCORE_ILLEGAL);
        
        auto side = static_cast<Side>(sd);
        
        assert(rcd.board->isIncheck(side));
        
        std::set<i64> idxSet { idx };
        auto v = perpetual_check_evasion(rcd, idx, side, idxSet, true);
        if (!v.empty()) {
            for(auto && m : v) {
                for(auto && p : m) {
                    auto rIdx = p.first;
                    auto rSide = p.second;
                    
                    auto value = side != rSide ? EGTB_SCORE_PERPETUAL_CHECK_LOSS : EGTB_SCORE_PERPETUAL_CHECK_WIN;
                    
                    egtbFile->setBufScore(rIdx, value, rSide);
                    auto ok = egtbFile->setupBoard(*rcd.board, rIdx, FlipMode::none, Side::white); assert(ok);
                    
                    auto idx2 = getFlipIdx(rcd, rIdx);
                    if (idx2 >= 0) {
                        egtbFile->setBufScore(idx2, value, rSide);
                    }
                    
//                    auto vec = setBufScore(rcd, rIdx, value, rSide);
//                    assert(vec.size() > 0 && vec.size() <= 2);
                }
            }
            rcd.changes++;
        }
    } /// for idx
}

/// The side being checked tries to escape
std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_check_evasion(EgtbGenThreadRecord& rcd, const i64 idx, const Side side, std::set<i64>& idxSet, bool evasion_checking)
{
    assert(rcd.board->isIncheck(side));
    
    Hist hist;

    std::vector<std::map<i64, Side>> rVec;

    /// test escape check for the side being checked
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetual_check_probe(rcd, hist, false);
        
        auto score = r.first;
        auto check = true;

        assert(perpetuation_score_valid(score));
        
        /// it has an option to win by letting opposite be pertuation
        if (score == EGTB_SCORE_UNSET) {
            auto xside = getXSide(side);
            auto checking = evasion_checking && rcd.board->isIncheck(xside);
            assert(r.second >= 0);
            idxSet.insert(r.second);
            auto v = perpetual_check_attack(rcd, r.second, xside, idxSet, checking);
            if (!v.empty()) {
                for(auto && m : v) {
                    m[idx] = side;
                    rVec.push_back(m);
                }
            }
            idxSet.erase(r.second);
            assert(idxSet.find(r.second) == idxSet.end());
        } else if (score == EGTB_SCORE_PERPETUAL_CHECK_LOSS 
                   || score == EGTB_SCORE_PERPETUAL_CHASE_LOSS) {
            check = false;
        } else {
        }

        rcd.board->takeBack(hist);

        if (!check) {
            return std::vector<std::map<i64, Side>>();
        }
    }

    return rVec;
}

std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_check_attack(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side, std::set<i64>& idxSet, bool evasion_checking)
{
    Hist hist;
    
    std::vector<std::map<i64, Side>> rVec;
    
    /// for the side checking
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetual_check_probe(rcd, hist, true);
        
        auto score = r.first;

        assert(perpetuation_score_valid(score));
        
        auto check = true;

        if (score == EGTB_SCORE_DRAW 
            || score == EGTB_SCORE_PERPETUAL_CHECK_LOSS
            || score == EGTB_SCORE_PERPETUAL_CHASE_LOSS) {
            check = false;
        } else if (score == EGTB_SCORE_UNSET) {
            auto xside = getXSide(side);
            assert(rcd.board->isIncheck(xside));
            if (idxSet.find(r.second) == idxSet.end()) {
                assert(r.second >= 0);
                idxSet.insert(r.second);
                auto v = perpetual_check_evasion(rcd, r.second, xside, idxSet, evasion_checking);
                idxSet.erase(r.second);
                
                if (v.empty()) {
                    check = false;
                } else {
                    for(auto && m : v) {
                        m[idx] = side;
                        rVec.push_back(m);
                    }
                }
            } else {
                /// the position is in a repitition
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
    
    return rVec;
}



std::pair<int, i64> EgtbGenDb::perpetual_check_probe(EgtbGenThreadRecord& rcd, const bslib::Hist& hist, bool drawIfNotIncheck)
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
    
    perpetual_chase(rcd, idx, side);
    
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
                perpetual_chase(rcd, idx, side);
            }
        }
    } /// for idx
}

std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_chase(EgtbGenThreadRecord& rcd, const i64 idx, const Side side)
{
    std::set<i64> idxSet { idx };
    XqChaseJudge xqChaseJudge;

    auto v = perpetual_chase_evasion(rcd, idx, side, idx, idxSet, xqChaseJudge);
    
    auto d = idx == debugIdx;
    if (v.empty()) {
        egtbFile->flag_set_side(idx, side);
    } else {
        rcd.changes++;
        
        if (d) {
            rcd.board->printOut(std::string("perpetual chase detected idx=") + std::to_string(idx)
                                + ", side=" + Funcs::side2String(side, false));
        }
        
        for(auto && m : v) {
            
            if (d) {
                std::cout << "a line, sz: " << std::to_string(m.size()) << std::endl;
            }

            for(auto && p : m) {
                auto rIdx = p.first;
                auto rSide = p.second;
                
                auto value = side != rSide ? EGTB_SCORE_PERPETUAL_CHASE_LOSS : EGTB_SCORE_PERPETUAL_CHASE_WIN;
                
//                auto vec = setBufScore(rcd, rIdx, value, rSide);
//                assert(vec.size() > 0 && vec.size() <= 2);
                
                egtbFile->setBufScore(rIdx, value, rSide);
                auto ok = egtbFile->setupBoard(*rcd.board, rIdx, FlipMode::none, Side::white); assert(ok);
                
                auto idx2 = getFlipIdx(rcd, rIdx);
                if (idx2 >= 0) {
                    egtbFile->setBufScore(idx2, value, rSide);
                }

            }
            
        }
        
//        if (d) {
//            Hist hist;
//            for(auto && move : rcd.board->gen(side)) {
//                rcd.board->make(move, hist);
//                
//                auto r = perpetual_chase_probe(rcd, hist);
//                
//                auto score = r.first;
//                if (score != EGTB_SCORE_ILLEGAL) {
//                    std::string s = "idx: " + std::to_string(r.second) + ", score: " + std::to_string(score);
//                    rcd.board->printOut(s);
//                }
//                
//                rcd.board->takeBack(hist);
//            }
//
//            auto v2 = perpetual_chase_evasion(rcd, idx, side, idx, idxSet, xqChaseJudge);
//        }

    }
    
    if (d) {
        std::cout << "debugCnt = " << debugCnt << std::endl;
    }
    return v;
}


/*
 * The side tries to escape its pieces from being chased.
 * If it has a chance, it will remain the status of being chased
 * for further moves
 */
std::vector<std::map<i64, Side>> EgtbGenDb::perpetual_chase_evasion(EgtbGenThreadRecord& rcd, const i64 idx, const Side side, const i64 startIdx, std::set<i64>& idxSet, XqChaseJudge& chaseJudge)
{
    chase_evasion_cnt++;
    
    auto d = startIdx == debugIdx;
    if (d) {
        debugCnt++;
    }
    
    std::vector<std::map<i64, Side>> rVec;

    if (!chaseJudge.addBoard(*rcd.board, side)) {
        chaseJudge.removeLastBoard();
        if (d) {
            rcd.board->printOut("chase_evasion chaseJudge not chase, side = " + Funcs::side2String(side));
        }
        return rVec;
    }

    if (d) {
        rcd.board->printOut("chase_evasion in progress, startIdx=" + std::to_string(startIdx) +
                            ", idx=" + std::to_string(idx) + ", side=" + Funcs::side2String(side));
    }

    chase_len_max = std::max<i64>(chase_len_max, chaseJudge.addingCnt);
    auto beingChased = true;

    /// test escape chasing for the side being chased
    Hist hist;
    for(auto && move : rcd.board->gen(side)) {
        rcd.board->make(move, hist);
        
        auto r = perpetual_chase_probe(rcd, hist);
        
        auto score = r.first; /// in the view of xside
        
        assert(perpetuation_score_valid(score));
        

        switch (score) {
            case EGTB_SCORE_UNSET:
                if (d) {
                    std::string s = "chase_evasion after move=" + rcd.board->toString(move) + ", score=" + std::to_string(score) +  + ", sIdx=" + std::to_string(r.second);
                    rcd.board->printOut(s);
                }

                
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
                    
                    if (d) {
                        std::string s = "chase_evasion after move=" + rcd.board->toString(move) + ", v size=" + std::to_string(v.size());
                        rcd.board->printOut(s);
                    }
//                } else {
//                    /// something wrong
//                    beingChased = false;
                }
                break;
                
            case EGTB_SCORE_PERPETUAL_CHASE_LOSS:
//            {
//                std::map<i64, Side> m;
//                m[idx] = side;
//                rVec.push_back(m);
//                if (d) {
//                    std::cout << "IMHERE 01" << std::endl;
//                }
//                break;
//            }
//                
            case EGTB_SCORE_PERPETUAL_CHECK_LOSS:
                beingChased = false;
                rVec.clear();
                if (d) {
                    std::cout << "IMHERE 03" << std::endl;
                }
                break;
                
            default:
                break;
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
    auto d = startIdx == debugIdx;
    if (d) {
        debugCnt++;
    }

    std::vector<std::map<i64, Side>> rVec;
//    if (egtbFile->flag_is_side(idx, side) || chaseJudge.addingCnt >= 120) {
//        if (d) {
//            rcd.board->printOut("chase_attack flag side not chase, side = " + Funcs::side2String(side) + ", idxSet sz = " + std::to_string(idxSet.size()));
//        }
//        return rVec;
//    }
    if (idxSet.size() > 120) {
        return rVec;
    }

    if (!chaseJudge.addBoard(*rcd.board, side)) {
        chaseJudge.removeLastBoard();
//        /// mark it as not-chasing
//        egtbFile->flag_set_side(idx, side);
//        if (d) {
//            rcd.board->printOut("chase_attack chaseJudge not chase, side = " + Funcs::side2String(side));
//        }
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
        
//        /// All captures must be losing, not better as alternative ones, avoid anyway
//        if (!rcd.board->getPiece(move.dest).isEmpty()) {
//            continue;
//        }

        rcd.board->make(move, hist);
        
        auto r = perpetual_chase_probe(rcd, hist);
        
        auto score = r.first; /// score in xside view
        
        /// can't have wining captures
//        assert(hist.cap.isEmpty() || score < 0);
        assert(perpetuation_score_valid(score));
        
        switch (score) {
            case EGTB_SCORE_UNSET:
                if (d) {
                    std::string s = "chase_attack after move=" + rcd.board->toString(move) + ", score=" + std::to_string(score) +  + ", sIdx=" + std::to_string(r.second);
                    rcd.board->printOut(s);
                }
                if (move.piece.type == PieceType::pawn && abs(move.dest - move.from) == 9) {
                    chasing = false;
                } else
                /// no repetition - will be visted later in the next step
                if (idxSet.find(r.second) == idxSet.end()
                    ) {
                    std::pair<i64, MoveFull> p;
                    p.first = r.second;
                    p.second = move;
                    list.push_back(p);
                } else {
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
                            
//                            std::string s = "chase_attack chased after move=" + rcd.board->toString(move) + ", score=" + std::to_string(score);
//                            rcd.board->printOut(s);

                        }
                        
                        if (d) {
                            std::cout << "Repetition with startIdx=" << startIdx
                            << ", chasing=" << chasing << std::endl;
                        }
                    }
                }
                break;
                /// score from opposite view -> winning for the current side
            case EGTB_SCORE_PERPETUAL_CHASE_LOSS:
            case EGTB_SCORE_PERPETUAL_CHECK_LOSS:
            case EGTB_SCORE_DRAW:
                /// this move is a better alternative -> no chase
                chasing = false;
                if (d) {
                    rcd.board->printOut("IMHERE 04, Not chased because score = " + std::to_string(score));
                }
                break;

            default:
                break;
        }
        
        rcd.board->takeBack(hist);
        
        if (!chasing) {
            /// if there is any better alternative/draw -> return empty
            chaseJudge.removeLastBoard();
//            egtbFile->flag_set_side(idx, side);
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
//            if (!rVec.empty()) {
//                if (d) {
//                    std::cout << "Chase cancelled with startIdx=" << startIdx
//                    << ", rVec.size()=" << rVec.size() << std::endl;
//                    
//                    std::string s = "chase_attack after move=" + rcd.board->toString(m.second);
//                    rcd.board->printOut(s);
//                }
//            }

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
//    if (rVec.empty()) {
//        egtbFile->flag_set_side(idx, side);
//    }
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

void EgtbGenDb::perpetuation_propaganda()
{
//    std::cout << "\tperpetuation_propaganda BEGIN" << std::endl;

    resetAllThreadRecordCounters();
    egtbFile->clearFlagBuffer();

    /// Init
    {
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_propaganda_init, this, i));
        }
        
        perpetuation_thread_propaganda_init(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }

    showData("After perpetuation_thread_propaganda_init", debugIdx, Side::none, false);

    for (auto cnt = 0; ; ++cnt) {
        resetAllThreadRecordCounters();
        
        for (i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            bool flags[2] = {
                egtbFile->flag_is_cap(idx, Side::black),
                egtbFile->flag_is_cap(idx, Side::white)
            };
            
            for(auto sd = 0; sd < 2; sd++) {
                if (flags[sd]) {
                    auto side = static_cast<Side>(sd);
                    egtbFile->flag_clear_cap(idx, side);
                    egtbFile->flag_set_side(idx, side);
                }
            }
        }
        
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::perpetuation_thread_propaganda, this, i));
        }
        
        perpetuation_thread_propaganda(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();

        auto changeCnt = allThreadChangeCount();
        
        if (egtbVerbose) {
            std::cout << " Loop: " << cnt << ", perpetuation_propaganda changeCnt=" << changeCnt << std::endl;
        }
        
        if (changeCnt == 0) {
            break;
        }
    }
}


int EgtbGenDb::perpetuation_thread_propaganda_set_parent_flag(EgtbGenThreadRecord& rcd, i64 idx, GenBoard* board, Side side, int oscore)
{
    Hist hist;
    auto xside = getXSide(side);
    
    auto cnt = 0;
    
//    auto d = false;//idx == debugIdx2 && side == Side::white;
//    if (d) {
//        board->printOut("perpetuation_thread_propaganda_set_parent_flag idx=" + std::to_string(idx));
//    }
    
    for(auto && move : board->gen_backward_quiet(side)) {
        board->make(move, hist); assert(hist.cap.isEmpty());
        
        if (!board->isIncheck(xside)) {
            assert(board->isValid());
            
            auto rIdx = egtbFile->getIdx(*board).key; assert(rIdx >= 0);
            
            /// parents
            auto sc = egtbFile->getBufScore(rIdx, side);
            
            if (sc == EGTB_SCORE_UNSET 
                || (EgtbFile::isPerpetualScore(sc) && sc != -oscore)
//                || EgtbFile::isPerpetualScore(sc)
                ) {
//                egtbFile->flag_set_side(rIdx, side);
                egtbFile->flag_set_cap(rIdx, side);
                cnt++;
//                if (d) {
//                    std::cout << "set flag for rIdx=" << rIdx << ", side=" << Funcs::side2String(side, true) << std::endl;
//                }
                auto sIdx = getFlipIdx(rcd, rIdx);
                if (sIdx >= 0) {
//                    egtbFile->flag_set_side(sIdx, side);
                    egtbFile->flag_set_cap(sIdx, side);
                    cnt++;
//                    if (d) {
//                        std::cout << "set flag for sIdx=" << sIdx << ", side=" << Funcs::side2String(side, true) << std::endl;
//                    }
                }
            }
            
        }
        
        board->takeBack(hist);
    }
    return cnt;
}


void EgtbGenDb::perpetuation_thread_propaganda_init(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);

    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getBufScore(idx, Side::black),
            egtbFile->getBufScore(idx, Side::white)
        };

        if (scores[0] == EGTB_SCORE_ILLEGAL && scores[1] == EGTB_SCORE_ILLEGAL) {
            continue;
        }
        
        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);
        
        auto d = false;//idx == debugIdx2;
        if (d) {
            rcd.board->printOut("perpetuation_thread_propaganda_init");
        }
        
        Hist hist;
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
                    if (!rcd.board->isIncheck(side) && !hist.cap.isEmpty()) {
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


void EgtbGenDb::perpetuation_thread_propaganda(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && rcd.board);
    
    Hist hist;
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        bool flags[2] = {
            egtbFile->flag_is_side(idx, Side::black),
            egtbFile->flag_is_side(idx, Side::white)
        };
        
        if (!flags[0] && !flags[1]) {
            continue;
        }

        auto ok = egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white); assert(ok);

        for(auto sd = 0; sd < 2; ++sd) {
            if (!flags[sd]) {
                continue;
            }
            
            auto side = static_cast<Side>(sd);
//            auto xside = static_cast<Side>(1 - sd);

//            auto sc = egtbFile->getBufScore(idx, side);
            
            /// Chase-win could be replace by check-win
//            if (sc != EGTB_SCORE_UNSET
//                && !EgtbFile::isPerpetualScore(sc)
//                ) {
//                egtbFile->flag_clear_side(idx, side);
//                continue;
//            }

            auto score = perpetuation_propaganda_probe(*rcd.board, side);
            
            if (score != EGTB_SCORE_UNSET) {
                auto sc = egtbFile->getBufScore(idx, side);
                if (score == sc) {
                    egtbFile->flag_clear_side(idx, side);
                    auto sIdx = getFlipIdx(rcd, idx);
                    if (sIdx >= 0) {
                        egtbFile->flag_clear_side(sIdx, side);
                    }
                } else {
                    auto vec = setBufScore(rcd, idx, score, side);
                    assert(vec.size() > 0 && vec.size() <= 2);
                    egtbFile->flag_clear_side(idx, side);
                    
                    if (vec.size() > 1) {
                        egtbFile->flag_clear_side(vec[1], side);
                    }
                    
                    if (vec.front() == debugIdx || vec.back() == debugIdx) {
                        showData("Set data", debugIdx, debugSide);
                    }
                    
                    if (EgtbGenFile::isPerpetualScore(score)) {
                        auto xside = static_cast<Side>(1 - sd);
                        perpetuation_thread_propaganda_set_parent_flag(rcd, idx, rcd.board, xside, score);
                    }
                    
                    rcd.changes++;
                }
            }
        } /// for sd
    } /// for idx
}

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
            
            if (!EgtbFile::pickBestFromRivalScore(bestScore, score)) {
                assert(score != EGTB_SCORE_MISSING);
                unsetCount++;
            }
        }
        board.takeBack(hist);
    }

    assert(legalCount > 0);

    if (bestScore == EGTB_SCORE_PERPETUAL_CHECK_WIN || bestScore == EGTB_SCORE_PERPETUAL_CHASE_WIN) {
        return bestScore;
    }
    
    return unsetCount ? EGTB_SCORE_UNSET : bestScore;
}

#endif
