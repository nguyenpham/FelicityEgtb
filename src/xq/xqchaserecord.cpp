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

#include <sstream>
#include <iostream>

#include "xqchaserecord.h"

#ifdef _FELICITY_XQ_

using namespace bslib;

XqChaseRecord::XqChaseRecord(Piece _attacker, int _attackerPos,
                             Piece _victim, int _victimPos,
                             bool _beProtected
                             )
    : attacker(_attacker),
      victim(_victim),
      attackerPos(_attackerPos),
      victimPos(_victimPos),
      beProtected(_beProtected)
{
    assert(_attackerPos >= 0 && _attackerPos < 90);
    assert(_victimPos >= 0 && _victimPos < 90);

//    attacker = _attacker;
//    attackerPos = _attackerPos;
//    victim = _victim;
//    victimPos = _victimPos;
//    beProtected = _beProtected;
}

bool XqChaseRecord::isValid() const {
    return !attacker.isEmpty() && !victim.isEmpty()
            && victim.type != PieceType::king
            && attackerPos != victimPos
//            && isPosValid(attackerPos) && isPosValid(victimPos)
            ;
}

std::string XqChaseRecord::toString() const {
    std::ostringstream stringStream;
    Piece a(attacker.type, attacker.side);
    Piece v(victim.type, victim.side);
    
    XqBoard board;
    stringStream << board.piece2String(a, true) << "(" << attacker.idx << ") -> ";
    stringStream << board.piece2String(v, true) << "(" << victim.idx << ") " << (beProtected ? "*" : "");
    return stringStream.str();
}

bool XqChaseRecord::operator == (const XqChaseRecord& chaseRecord) const {
    return attacker.idx == chaseRecord.attacker.idx && victim.idx == chaseRecord.victim.idx && beProtected == chaseRecord.beProtected;
}

bool XqChaseRecord::updatePieceIndecesAfterMove(int from, int dest) {
//    if (attacker.idx == dest) {
//        attacker.idx = from;
//        return true;
//    }
//    if (victim.idx == dest) {
//        victim.idx = from;
//        return true;
//    }
    return false;
}


#endif // _FELICITY_XQ_
