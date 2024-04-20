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

int main(int argc, const char * argv[])
{
    std::cout << "Welcome to Felicity Endgame databases - version: 0.00001" << std::endl;

#ifdef _FELICITY_CHESS_

    bslib::ChessBoard board;
    
    board.newGame();
    board.perft(5);
#endif

#ifdef _FELICITY_XQ_

    bslib::XqBoard board;
    
    board.newGame();
    board.perft(5);
#endif

    return 0;
}

