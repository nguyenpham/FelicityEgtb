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


#ifndef bs_xq_chasejudge_h
#define bs_xq_chasejudge_h

#include <vector>
#include <string>

#include "xqchaselist.h"
#include "xq.h"

#ifdef _FELICITY_XQ_

namespace bslib {


#define DRAW_LEN            (60*2)

class XqChaseJudge
{
public:
    static Result evaluate(XqBoard& board, int repeatLen);
    static void testRules();
    static void testRules2();

    bool addBoard(XqBoard& board, Side);
    void removeLastBoard();
    
    Result evaluate();
    Result evaluate2();

private:
    static bool perpetual_check(const XqBoard& board, XqBoard& tmpBoard, int repeatLen, Result&);

    static bool testRules(const std::string& path);
    static bool testRules(const std::vector<std::string>& contentVec);

    /*
     * Full chase lists for two sides
     */
    std::vector<XqChaseListPair> chaseVec;
    std::vector<XqChaseListPair> atkVec;
    
    /*
     * Working chase list
     */
    XqChaseListPair workingPair;

    std::string toString() const;

private:
    Result ruleRepetition(const XqBoard& board, XqBoard& tmpBoard, int repeatLen);
    Result ruleRepetition2(const XqBoard& board, int repeatLen);

    bool areAllChasesLegal(int attackerSd) const;

public:
    int addingCnt = 0;
};


} // bslib

#endif // _FELICITY_XQ_

#endif // bs_xq_chasejudge_h
