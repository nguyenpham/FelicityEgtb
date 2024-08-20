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

#include <map>
#include <sstream>
#include <iostream>
#include <assert.h>

#include <fstream>

#include "xqchasejudge.h"

#include "../base/funcs.h"
#include "../base/types.h"

#ifdef _FELICITY_XQ_
using namespace bslib;

namespace bslib {


bool xqRepetitionFofeiteForLastMoveOnly = true;

std::string XqChaseJudge::toString() const {
    std::ostringstream stringStream;

    stringStream << "workingChaseList[0]=" << workingPair.pair[0].toString() << std::endl;
    stringStream << "workingChaseList[1]=" << workingPair.pair[1].toString() << std::endl;

    return stringStream.str();
}

Result XqChaseJudge::evaluate(XqBoard& board, int repeatLen) {
    XqBoard tmpBoard;
    Result r0;
    auto b0 = XqChaseJudge::perpetual_check(board, tmpBoard, repeatLen, r0);

    /// In case of draw, need to verify if any side chased
    if (b0) {
        return r0;
    }

    assert(r0.isNone() || r0.result == GameResultType::draw);

    XqChaseJudge xqChaseJudge;
    auto r1 = xqChaseJudge.ruleRepetition(board, tmpBoard, repeatLen);
    auto r = r1.isNone() ? r0 : r1;

    /// It is better to rule when one side violates the rule for hist last move
    /// it means the opposite won and in turn
    /// otherwise, wait for him for one more move
    if (xqRepetitionFofeiteForLastMoveOnly &&
        ((r.result == GameResultType::win && board.side == Side::black)
        || (r.result == GameResultType::loss && board.side == Side::white))) {
        return Result(); /// say nothing, wait for him for the next move
    }

    return r;
}

/// return true to stop immediately, false if needed to check chase
bool XqChaseJudge::perpetual_check(const XqBoard& board, XqBoard& tmpBoard, int repeatLen, Result& result) {

    const int Checking_yes = 1 << 0;
    const int Checking_no = 1 << 1;

    tmpBoard.clone(&board);
    assert(tmpBoard.getHistListSize() == board.getHistListSize());

    int status[2] = { 0, 0 };

    status[static_cast<int>(tmpBoard.side)] = tmpBoard.isIncheck(tmpBoard.side) ? Checking_yes : Checking_no;

    /*
     * Go back to the first position of the repetition
     */
    auto cnt = 1;
    int i = tmpBoard.getHistListSize() - 1;
    for (int j = 1; j < repeatLen; j++, i--) {
        tmpBoard.takeBack();
        auto check = tmpBoard.isIncheck(tmpBoard.side);
        cnt++;

        status[static_cast<int>(tmpBoard.side)] |= check ? Checking_yes : Checking_no;
    }

    assert(tmpBoard.getHistListSize() + repeatLen - 1 == board.getHistListSize());
    assert(status[0] != 0 && status[1] != 0);

    /* AXF Rules
     * 1. Common terms
     *    Perpetual check: A string of consecutive checks, causing a similar situation to recur endlessly. This definition can also be used for "perpetual block", "perpetual exchange" and "perpetual offer".
     *  Counter check: A move that resolves a check and checks the opponent king at the same time. Similar definitions apply to "Counter kill", "counter chase", "resolve-kill" and "counter chase" etc.
     * 2 Basic Principles of Judgement
     *   2) When both players violate the same rules simultaneously, the game shall be declared a draw
     * 3 General Rules
     *   1) Under all circumstances, perpetual checking will be ruled as a loss
     */
    /*
     * Diagrams: 1, 2, 3, 4, 5, 6, 11, 12, 13, 14, 15, 16
     */
    if ((status[0] | status[1]) & Checking_yes) { // opposite in check
        if ((status[0] & Checking_no) == 0 || (status[1] & Checking_no) == 0) { // none stop checking -> perpetual check
            if (status[0] == status[1]) { // opposite is doing the same -> draw (both violate the same rules simultaneously)
                /*
                 * Diagrams: 4
                 */
                result = Result(GameResultType::draw, ReasonType::extracomment, "both players are pertually checking"); // (diagram: 4)");
                return true;
            }
            /*
             * Diagrams: 1, 2, 3, 5
             */
            auto losssd = (status[0] & Checking_no) == 0 ? 1 : 0; // opposite side
            result = Result(losssd == B ? GameResultType::win : GameResultType::loss,
                             ReasonType::extracomment,
                            "perpetual check"); // (diagrams: 1, 2, 3, 5)");
            return true;
        }

        /* One check, one idle
         * Diagrams: 6, 11, 12, 13, 14, 15, 16
         */
        result = Result(GameResultType::draw, ReasonType::extracomment, "one check one idle"); // (diagrams: 6, 11, 12, 13, 14, 15, 16)");
        return (status[0] & Checking_yes) && (status[1] & Checking_yes);
    }

    return false;
}

Result XqChaseJudge::ruleRepetition(const XqBoard& board, XqBoard& wBoard, int repeatLen) {
#ifdef _DEBUG_PRINT_
    wBoard.printOut("wBoard Start repetition");
#endif
    
    assert(repeatLen >= 4 && static_cast<int>(board.getHistListSize()) >= repeatLen);
    assert(wBoard.getHistListSize() + repeatLen - 1 == board.getHistListSize());
    
    auto wSide = wBoard.side;
    auto wSd = static_cast<int>(wSide);
    auto oppSide = board.xSide(wSide);
    
    
    /*
     * Initialise data
     */
    atkVec = std::vector<XqChaseListPair>(repeatLen);
    chaseVec = std::vector<XqChaseListPair>(repeatLen);
    
    /// evasion/chase by opposite side
    XqChaseList x0(wBoard, oppSide);
    
    atkVec[0].pair[wSd] = x0;
    workingPair.pair[wSd] = x0;
    
    auto move = board.getMoveAt(wBoard.getHistListSize());
    wBoard.make(move);
    
    /// start chasing by working side
    XqChaseList x1(wBoard, wSide);
    if (x1.isEmpty()) {
        return Result(GameResultType::draw, ReasonType::repetition);
    }
    
    atkVec[1].pair[1 - wSd] = x1;
    workingPair.pair[1 - wSd] = x1;
    
#ifdef _DEBUG_PRINT_
    std::cout << "Start of workingPair[" << wSd << "]: " << workingPair.pair[wSd].toString() << std::endl;
    std::cout << "Start of workingPair[" << 1 - wSd     << "]: " << workingPair.pair[1 - wSd].toString() << std::endl;
    wBoard.printOut("Pos after the first move");
#endif
    
    std::swap(wSide, oppSide);
    wSd = 1 - wSd;
    
    /// Build up data
    for (auto x = 1; !workingPair.isEmpty(); x++) {
        
        /// sd is the side of victims
        for (auto sd = 1; sd >= 0; --sd) {
            if (!atkVec[x].pair[sd].isBuilt) {
                atkVec[x].pair[sd] = XqChaseList(wBoard, static_cast<Side>(1 - sd));
            }
            if (workingPair.pair[sd].list.empty()) {
                continue;
            }
            
            assert(workingPair.pair[sd].list.begin()->victim.side != workingPair.pair[sd].list.begin()->attacker.side);
            assert(workingPair.pair[sd].list.begin()->victim.side == static_cast<Side>(sd));
            
            if (wSd != sd) {
                workingPair.pair[sd].subtract(atkVec[x].pair[sd]);
                
#ifdef _DEBUG_PRINT_
                std::cout << "workingPair[" << sd << "] after - " << workingPair.pair[sd].toString() << std::endl;
#endif
                /// Store
                if (!workingPair.pair[sd].isEmpty()) {
                    chaseVec[x - 1].pair[sd] = workingPair.pair[sd];
                }
            } else
                if (x > 1) {
                    
                    auto listC = atkVec[x].pair[sd];
                    auto listB = atkVec[x-1].pair[sd];
                    
                    listC.subtract(listB);
                    listC.sameVictims(workingPair.pair[sd]);
                    
#ifdef _DEBUG_PRINT_
                    std::cout << "list B " << listB.toString() << std::endl;
                    std::cout << "list C = workingChaseList[" << sd << "] = " << listC.toString() << std::endl;
#endif
                    workingPair.pair[sd] = listC;
                    
                    if (!workingPair.pair[sd].isEmpty()) {
                        chaseVec[x].pair[sd] = workingPair.pair[sd];
                    }
                }
        } /// for (auto x
        
        auto l = wBoard.getHistListSize();
        if (l >= board.getHistListSize()) {
            break;
        }
        
        auto move = board.getMoveAt(l);
        wBoard.make(move);
        
        std::swap(wSide, oppSide);
        wSd = 1 - wSd;
    }
    
    assert(workingPair.pair[0].list.empty() || workingPair.pair[0].list.begin()->victim.side == Side::black);
    assert(workingPair.pair[1].list.empty() || workingPair.pair[1].list.begin()->victim.side == Side::white);
    
#ifdef _DEBUG_PRINT_
    std::cout << "Last 0 (black) " << workingPair.pair[0].toString() << std::endl;
    std::cout << "Last 1 (white) " << workingPair.pair[1].toString() << std::endl;
#endif
    
    return evaluate();
}


Result XqChaseJudge::evaluate() {
    /*
     * Check if all chases are allowed (such as King/Pawn chase, Pawn being chased, protected), clear the workingPair for that side
     */

    for (auto sd = 0; sd < 2; sd++) {
        if (!workingPair.pair[sd].list.empty() && areAllChasesLegal(sd)) {
            workingPair.pair[sd].list.clear();
        }
    }

    /// not-empty-list means being chased -> winning
    auto scoreB = workingPair.pair[0].list.empty() ? 0 : 1; /// for Black
    auto scoreW = workingPair.pair[1].list.empty() ? 0 : 1; /// for White

    /// one chase, one not chase
    if (scoreW != scoreB) {
        return Result(scoreW > scoreB ? GameResultType::win : GameResultType::loss,
                         ReasonType::perpetualchase);
    } else if (scoreB > 0) {
        return Result(GameResultType::draw, ReasonType::bothperpetualchase);
    }

    /// if none chased, game is draw
    return Result(GameResultType::draw, ReasonType::repetition);
}


bool XqChaseJudge::areAllChasesLegal(int attackerSd) const {
    for (auto x = 1; x < chaseVec.size(); ++x) {
        auto theList = &chaseVec[x].pair[attackerSd];
        if (!theList->isBuilt || theList->list.empty()) {
            continue;
        }

        if (theList->kingPawnChases()) {
            return true;
        }
        if (theList->beingProtected()) {
            return true;
        }

        /*
         * AXF RULES
         * 1 Common terms
         *   Exchange: A move that trades a piece with an oppoent's piece.
         * 3 General Rules
         *    7) Perpetual chase on a piece of the same type will be ruled as a draw. It will not be allowed, however,
         *       if the piece attacked is pinned down to a file or a rank.
         *       Besides, a "free" horse may not perpetually chases an "impaired" horse.
         *
         * Diagrams: 51, 52, 101, 102, 103 are good exchanges
         * Diagrams: 40, 53, 54, 55, 64, 65, 66 other side cannot exchanged because of being blocked, pinned...
         */
        if (theList->couldBeXchange()) {
            auto r = theList->list[0];
            auto list = &atkVec[x].pair[1 - attackerSd];
            if (list->isBuilt && list->isThereAttack(r.victim.idx, r.attacker.idx)) { /// attack back from victim
                /// comments.push_back("It is an exchange (diagrams: 51, 52, 101, 102, 103)");
                return true;
            }
            /// comments.push_back("It is not an exchange because piece of other side is blocked, pinned... (diagrams: 40, 53, 54, 55, 64, 65, 66)");
        }
    }

    return false;
}



} // namespace Chess


#endif // _FELICITY_XQ_
