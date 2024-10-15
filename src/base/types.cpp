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

#include <chrono>
#include <map>
#include <sstream>
#include <iostream>

#include "base.h"

#include "funcs.h"

using namespace bslib;

extern const char* reasonStrings[];
extern const std::string resultStrings[];

const char* reasonStrings[] = {
    "*", "mate", "stalemate", "repetition", "resign", "fifty moves",
    "insufficient material",
    "illegal move", "timeout",
    "adjudication by lengths", "adjudication by egtb",
    "adjudication by online egtb", "adjudication by engines' scores",
    "adjudication by human",
    "perpetual chase",
    "both perpetual chases", "extra comment", "crash", "abort",
    nullptr
};

/// noresult, win, draw, loss
const std::string resultStrings[] = {
    "*", "1-0", "1/2-1/2", "0-1", "", ""
};

const std::string resultStrings_short[] = {
    "*", "1-0", "0.5", "0-1", "", ""
};

std::string Result::reasonString(ReasonType reason)
{
    return reasonStrings[static_cast<int>(reason)];
}

std::string Result::resultType2String(GameResultType type, bool shortFrom)
{
    auto t = static_cast<int>(type);
    if (t < 0 || t > 3) t = 0;
    return shortFrom ? resultStrings_short[t] : resultStrings[t];
}

std::string Result::toString(const Result& result)
{
    auto str = Result::resultType2String(result.result, false);
    if (result.reason != ReasonType::noreason) {
        str += " (" + Result::reasonString(result.reason) + ")";
    }
    return str;
}

//GameResultType string2ResultType(const std::string& s) {
//    for(int i = 0; !resultStrings[i].empty(); i++) {
//        if (resultStrings[i] == s) {
//            return static_cast<GameResultType>(i);
//        }
//    }
//    return GameResultType::unknown;
//}

