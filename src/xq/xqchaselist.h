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

/*
 This code is based on work: Nguyen Hong Pham (2018). A completed implementation for Xiangqi rules. ICGA Journal, Vol. 40, No. 3
 */

#ifndef bs_xq_chaselist_h
#define bs_xq_chaselist_h

#include <vector>
#include "xqchaserecord.h"

#include "xq.h"

#ifdef _FELICITY_XQ_

#define _DEBUG_PRINT_

namespace bslib {


class XqChaseList
{
public:
    std::vector<XqChaseRecord> list;
    bool isBuilt = false;

    XqChaseList() {}

    XqChaseList(XqBoard& board, Side side) {
        build(board, side);
    }

    bool isEmpty() const { return list.empty(); }

    void reset() {
        list.clear();
        isBuilt = false;
    }
    
    std::string toString() const;

    void sameVictims(const XqChaseList& otherChaseList);
    void subtract(const XqChaseList& otherChaseList);

    bool operator == (const XqChaseRecord& chaseRecord) const;

    bool kingPawnChases() const;

    bool couldBeXchange() const;
    bool isThereAttack(int fromPieceIdx, int toPieceIdx) const;
    bool beingProtected() const;

    void updatePieceIndecesAfterMove(int from, int dest);

    static bool ignoreSubverstProtection;

private:
    void build(XqBoard& board, Side side);

};

class XqChaseListPair
{
public:
    XqChaseList pair[2];
    
    bool isEmpty() const {
        return pair[0].isEmpty() && pair[1].isEmpty();
    }
    
    void reset() {
        pair[0].reset();
        pair[1].reset();
    }
};

} // bslib

#endif // _FELICITY_XQ_

#endif // bs_xq_chaselist_h
