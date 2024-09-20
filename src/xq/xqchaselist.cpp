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

#include "xqchaselist.h"
#include <sstream>
#include <iostream>
#include <assert.h>

//#include "OpBoard.h"

#ifdef _FELICITY_XQ_


using namespace bslib;

/*
 * ignoreSubverstProtection is used to run the special code to solve game diagrams 39, 40, 41 and 61
 * However it makes the game diagram 108 be failed
 */
bool XqChaseList::ignoreSubverstProtection = true; //false;

/*
 * Build the chase list for the given board and side
 * side will be the side of attackers
 */
void XqChaseList::build(XqBoard& board, Side side)
{
    isBuilt = true;

    auto xside = BoardCore::xSide(side);
    auto incheck = board.isIncheck(side);

    for (auto && move : board.gen(side)) {
        auto victim = board.getPiece(move.dest);

        /// Work with capture moves only
        if (victim.isEmpty()) {
            continue;
        }
        
        /// checking the opponent -> illegal status -> clear the list
        if (victim.type == PieceType::king) {
            list.clear();
            break;
        }

        /// allow to chase pawn at home
        if (victim.type == PieceType::pawn &&
             ( (victim.side == Side::white && move.dest >= 45)
            || (victim.side == Side::black && move.dest < 45))) {
            continue;
        }

        assert(victim.side == xside);

        /// Make a caputure
        Hist hist;
        board.make(move, hist);
#ifdef _FELICITY_USE_HASH_
        assert(board.isHashKeyValid());
#endif
        /// Make sure if the capture move is legal
        if (!board.isIncheck(side)) {
            auto attacker = board.getPiece(move.dest);
            auto beProtected = false;
            assert(attacker.side == side);

            /*
             * AFX RULES
             * 1 Common terms
             *   Protected: A piece is said to be "protected" (literally "rooted") if any opponet piece caputures it can be captured immediately.
             *              Otherwise, it is "unprotected". The protector of a piece is called the "root".
             *   Real root: When a protected piece is captured, and the protector can in turn capture the captor, the protector is a "real root".
             *   Fake root: When a protected piece is captured, the protector is immobilized and cannot take the captor immediately
             *
             * 3 General Rules
             *   6) Perpetual chase on a piece with real root will be declared a draw. It is a draw if the protector is a fake root.
             *      However, perpetual chase on a chariot (rook) by a horse or a cannon is prohibited
             */

            /*
             * Except attacker is a rook, capture a rook won't be considered having a protector
             */
            if (    !incheck
                    && (victim.type != PieceType::rook || attacker.type == PieceType::rook)
                    && attacker.type != PieceType::king
                    ) {
                /*
                 * We will check if the being attack side could capture back but count legal ones only. If it could legally, the capture piece is actually protected
                 */
                std::vector<MoveFull> reCapMoves;
                board.gen(reCapMoves, xside);

                for (int j = 0; j < reCapMoves.size(); j++)
                {
                    auto& reCapMove = reCapMoves[j];
                    if (move.dest != reCapMove.dest) {
                        continue;
                    }

                    Hist caphist;
                    board.make(reCapMove, caphist);
                    auto reCapIncheck = board.isIncheck(xside);
                    board.takeBack(caphist);

                    if (!reCapIncheck) {
                        beProtected = true;

                        /*
                         * Below code to solve special cases game diagrams 39, 40, 41 and 61
                         * However this code makes game diagram 108 be failed
                         */
                        /*
                         * Check subversted protection:
                         * If attacker is a rook, find all protecters if they are rooks or cannon and in the same line/columne but in opposite direction,
                         * that protection is subversted
                         */

                        if (ignoreSubverstProtection) {
                            if (attacker.type != PieceType::rook && attacker.type != PieceType::cannon) {
                                break;
                            }

                            auto guarder = board.getPiece(reCapMove.from).type;
                            if (guarder == PieceType::rook || guarder == PieceType::cannon) {
                                auto r1 = move.from / 9, r0 = reCapMove.from / 9;
                                if (r0 == r1) {
                                    auto c1 = move.from % 9, c0 = move.dest % 9, c2 = reCapMove.from % 9;
                                    if ((c0 < c1 && c1 < c2) || (c0 > c1 && c1 > c2)) {
                                        beProtected = false;
                                        break;
                                    }
                                } else if (move.from % 9 == reCapMove.from % 9) {
                                    auto r2 = move.dest / 9;
                                    if ((r0 < r1 && r1 < r2) || (r0 > r1 && r1 > r2)) {
                                        beProtected = false;
                                        break;
                                    }
                                }
                            }
                        } // end if (ignoreSubverstProtection)
                    }
                }
            }

            XqChaseRecord chaseRecord(attacker, move.from, victim, move.dest, beProtected);
            list.push_back(chaseRecord);

            //qDebug("XqChaseList::build, added chaseRecord = %s", chaseRecord.toString().c_str());
        }

        board.takeBack(hist);
    }
    
    if (!list.empty() && kingPawnChases()) {
//    if (!list.empty() && (kingPawnChases() || beingProtected())) {
        list.clear();
    }
}

/*
 * Keep common victims between two lists only
 */
void XqChaseList::sameVictims(const XqChaseList& otherXqChaseList) {
    std::vector<XqChaseRecord> tmpList;

    for (auto && record : list) {
        for (auto oRecord : otherXqChaseList.list) {
            if (record.victim.idx == oRecord.victim.idx) {
                if (oRecord.beProtected) {
                    record.beProtected = true;
                }
                tmpList.push_back(record);
                break;
            }
        }
    }
    list.clear();
    list = tmpList;
}

/*
 * Keep the diffirent records of the first list
 */
void XqChaseList::subtract(const XqChaseList& otherXqChaseList) {
    std::vector<XqChaseRecord> tmpList;

    for (auto && record : list) {
        bool keep = true;
        for (auto && oRecord : otherXqChaseList.list) {
            if (record == oRecord) {
                keep = false;
                break;
            }
        }

        if (keep) {
            tmpList.push_back(record);
        }
    }

    list.clear();
    list = tmpList;
}

/*
 * AFX RULES
 * 3. General Rules
 *    7) Perpetual chase on a piece of the same type will be ruled as a draw. It will not be allowed, however, if the piece attacked is pinned down to a file or a rank.
 *       Besides, a "free" horse may not perpetually chases an "impaired" horse.
 */
/*
 * The victim could attack back the attacker -> it is an exchange
 * Diagrams:
 */
bool XqChaseList::isThereAttack(int fromPieceIdx, int toPieceIdx) const {
    for (auto && record : list) {
        if (record.attacker.idx == fromPieceIdx && record.victim.idx == toPieceIdx) {
            return true;
        }
    }
    return false;
}

/*
 * AFX RULES
 * 3. General Rules
 *   3) Perpetual chase of one piece on another (other than a pawn that has yet to cross the river) will ruled as a loss.
 *      It is also illegal for two or more pieces to make perpetual on one piece, except when one of chasing pieces is a pawn or a king.
 *   9) A king or a pawn chasing any piece perpetually will be ruled as a draw. A king or a pawn,
 *      in combination with another piece, chasing pertually on a piece will also be ruled as a draw
 */
bool XqChaseList::kingPawnChases() const {
    assert(!list.empty());

//    std::string aComment;
    for (auto && record : list) {
        /*
         * AFX RULES
         * 3. General Rules
         *   3) "when one of chasing pieces is a pawn or a king"
         *   9) "A king or a pawn chasing any piece perpetually will be ruled as a draw. A king or a pawn,
         *      in combination with another piece, chasing pertually on a piece will also be ruled as a draw"
         * diagrams:
         */
        /*
         * King and/or pawns can chase
         * Diagrams: 67, 68, 68, 70, 72, 73,
         */
        if (record.attacker.type == PieceType::king || record.attacker.type == PieceType::pawn) {
//            aComment = "King or Pawns can legally chase (term: 3. General Rules, 3) allow \"when one of chasing pieces is a pawn or a king\" & 9) \"A king or a pawn chasing any piece perpetually will be ruled as a draw. A king or a pawn, in combination with another piece, chasing pertually on a piece will also be ruled as a draw\" (diagrams: 67, 68, 68, 70, 72, 73))";
            continue;
        }
        /*
         * 3. General Rules, 3) Perpetual chase of one piece on another (other than a pawn that has yet to cross the river) will ruled as a loss.
         * Pawns which are not crossed the river could be chased
         * Diagrams: 42, 43, 44, 60
         */
        assert(record.victimPos >= 0 && record.victimPos < 90);
        if (record.victim.type == PieceType::pawn &&
            ((record.victim.side == Side::white && record.victimPos >= 45) ||
             (record.victim.side == Side::black && record.victimPos < 45))) {
            // the chase is allow (the Pawn must be at home)
//            aComment = "Pawns which are not crossed the river can be legally chased (term: 3. General Rules, 3) Perpetual chase of one piece on another (other than a pawn that has yet to cross the river) will ruled as a loss (diagrams: 42, 43, 44, 60).";
            continue;
        }
        return false;

    }
//    comments.push_back(aComment);
    return true;
}

/*
 * AFX RULES
 * 3. General Rules
 *    7) Perpetual chase on a piece of the same type will be ruled as a draw. It will not be allowed, however, if the piece attacked is pinned down to a file or a rank.
 *       Besides, a "free" horse may not perpetually chases an "impaired" horse.
 */
/*
 * Two pieces could be exchanged if they are same type (R-R, C-C, H-H). All attacks should be from 1 piece only
 */
bool XqChaseList::couldBeXchange() const {
    assert(!list.empty());
    auto idx = list[0].attacker.idx;
    for (auto && record : list) {
        if (record.attacker.idx != idx || record.attacker.type != record.victim.type) {
            return false;
        }
    }
    return true;
}

/*
 * AFX RULES
 * 3. General Rules
 *    6) Perpetual chase on a piece with real root will be declared a draw. It is a draw if the protector is a fake root.
 *       However, perpetual chase on a chariot (rook) by a horse or a cannon is prohibited
 */
bool XqChaseList::beingProtected() const {
    assert(!list.empty());

    auto victimIdx = list[0].victim.idx;
    for (auto && record : list) {
        if (record.victim.idx != victimIdx || !record.beProtected) {
            return false;
        }
    }
    return true;
}

std::string XqChaseList::toString() const {
    std::ostringstream stringStream;

    for(auto && record : list) {
        stringStream << record.toString() << "; "; // std::endl;
    }
//    stringStream << std::endl;
    return stringStream.str();
}

void XqChaseList::updatePieceIndecesAfterMove(int from, int dest) {
    for (auto && record : list) {
        record.updatePieceIndecesAfterMove(from, dest);
    }
}


#endif // _FELICITY_XQ_
