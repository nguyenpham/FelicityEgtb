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


#ifndef bs_xq_chaserecord_h
#define bs_xq_chaserecord_h

#include "xq.h"

#ifdef _FELICITY_XQ_

namespace bslib {

class XqChaseRecord {
public:
    Piece attacker, victim;
    int attackerPos, victimPos;
    bool beProtected;

    XqChaseRecord(Piece _attacker, int _attackerPos, Piece _victim, int _victimPos, bool _beProtected);
    bool isValid() const;

    std::string toString() const;

    bool operator == (const XqChaseRecord& chaseRecord) const;
    bool updatePieceIndecesAfterMove(int from, int dest);

};

} // bslib

#endif // _FELICITY_XQ_

#endif
