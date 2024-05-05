/**
 This file is part of Felicity Egtb, distributed under MIT license.

 * Copyright (c) 2024 Nguyen Pham (github@nguyenpham)
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

#include <iostream>
#ifdef _FELICITY_CHESS_
#include "chess/chess.h"
#else
#include "xq/xq.h"
#endif

#include "egtb/egtb.h"

using namespace fegtb;
using namespace bslib;

int __main(int argc, const char * argv[])
{
    std::cout << "Welcome to Felicity Endgame databases for " << EGTB_MAJOR_VARIANT << " - version: " << EGTB_VERSION_STRING << std::endl;
    
#ifdef _FELICITY_CHESS_
    
    std::string names[] = {
        "kqk", "krk", "kpk",
        "kqbk", "kbbk", "krpk", "kbpk",
        "kqrnk", "kqrpk", "kqqpk", "knnnk", "kpppk", "krppk", "kbppk",
    };
    
    for (auto && name : names) {
        EgtbFile egtbFile(name);
        if (!egtbFile.verifyKeys()) {
            std::cout << "Error: " << egtbFile.getName() << std::endl;
        }
    }
    
    std::cout << "All DONE!" << std::endl;
    
#endif
    
#ifdef _FELICITY_XQ_
    
    std::string names[] = {
        "krk", "kck", "knk", "kpk",
        "kaabbk", "kaabbrk",
        "krkaabb", "kckaabb", "kckaabb", "kpkaabb",
        "kabrkabb", "kaabbrkaabb",
        "karpkaabb",
        "karckaabb",
        "kaabbrnkaabb"
    };
    
    for (auto && name : names) {
        std::cout << "Name: " << name << ", size: " << EgtbFile::computeSize(name) << std::endl;

        EgtbFile egtbFile(name);
        if (!egtbFile.verifyKeys(true)) {
            std::cout << "Error: " << egtbFile.getName() << std::endl;
        }
    }
    
    std::cout << "All DONE!" << std::endl;
    
#endif
    
    return 0;
}

