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

#ifdef _FELICITY_XQ_

using namespace fegtb;
using namespace bslib;

/// Generate retro moves without captures
std::vector<MoveFull> GenBoard::gen_backward_quiet(Side side) const
{
    std::vector<MoveFull> moves;

    const int* pl = pieceList[static_cast<int>(side)];

    for (int l = 0; l < 16; l++) {
        auto pos = pl[l];
        if (pos < 0) {
            continue;
        }

        auto piece = pieces[pos]; assert(piece.side == side);

        switch (static_cast<PieceType>(piece.type)) {
            case PieceType::king:
            {
                auto col = pos % 9;
                if (col != 3 && isEmpty(pos - 1)) { // go left
                    gen_addMove(moves, pos, pos - 1);
                }
                if (col != 5 && isEmpty(pos + 1)) { // right
                    gen_addMove(moves, pos, pos + 1);
                }
                if ((pos > 72 || (pos > 8 && pos < 27)) && isEmpty(pos - 9)) { // up
                    gen_addMove(moves, pos, pos - 9);
                }
                if ((pos < 18 || (pos > 63 && pos < 81)) && isEmpty(pos + 9)) { // down
                    gen_addMove(moves, pos, pos + 9);
                }
                break;
            }

            case PieceType::advisor:
            {
                auto y = pos - 10;   /* go left up */
                if ((y == 3 || y == 13 || y == 66 || y == 76) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 8;        /* go right up */
                if ((y == 5 || y == 13 || y == 68 || y == 76) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 8;        /* go left down */
                if ((y == 13 || y == 21 || y == 84 || y == 76) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 10;       /* go right down */
                if ((y == 13 || y == 23 || y == 76 || y == 86) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }

            case PieceType::elephant:
            {
                auto y = pos - 20; /* go left up */
                if ((y == 2 || y == 6 || y == 18 || y == 22 || y == 47 || y == 51 || y == 63 || y == 67) && isEmpty(pos - 10) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 16; /* go right up */
                if ((y == 2 || y == 6 || y == 22 || y == 26 || y == 47 || y == 51 || y == 67 || y == 71) && isEmpty(pos - 8) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 16; /* go left down */
                if ((y == 18 || y == 22 || y == 38 || y == 42 || y == 63 || y == 67 || y == 83 || y == 87) && isEmpty(pos + 8) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 20; /* go right up */
                if ((y == 22 || y == 26 || y == 38 || y == 42 || y == 67 || y == 71 || y == 83 || y == 87) && isEmpty(pos + 10) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }

            case PieceType::cannon:
            case PieceType::rook:
            {
                auto col = pos % 9;
                for (int y=pos - 1; y >= pos - col; y--) { /* go left */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                
                for (auto y = pos + 1; y < pos - col + 9; y++) { /* go right */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                
                for (auto y = pos - 9; y >= 0; y -= 9) { /* go up */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                
                for (auto y = pos + 9; y < 90; y += 9) { /* go down */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                break;
            }
                
            case PieceType::horse:
            {
                auto col = pos % 9;
                auto y = pos - 11;
                if (y >= 0 && col > 1 && isEmpty(y + 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 19;
                if (y >= 0 && col > 0 && isEmpty(y + 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 17;
                if (y >= 0 && col < 8 && isEmpty(y + 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 7;
                if (y >= 0 && col < 7 && isEmpty(y - 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                
                y = pos + 7;
                if (y < 90 && col > 1 && isEmpty(y + 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 17;
                if (y < 90 && col > 0 && isEmpty(y - 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 19;
                if (y < 90 && col < 8 && isEmpty(y - 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 11;
                if (y < 90 && col < 7 && isEmpty(y - 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }
                
            case PieceType::pawn:
            {
                if ((side == Side::black && pos > 44) || (side == Side::white && pos < 45)) {
                    auto col = pos % 9;
                    /* go left */
                    if (col > 0 && isEmpty(pos - 1)) {
                        gen_addMove(moves, pos, pos - 1);
                    }
                    /* go right */
                    if (col < 8 && isEmpty(pos + 1)) {
                        gen_addMove(moves, pos, pos + 1);
                    }
                }
                
                if (side == Side::black) {
                    /* go up */
                    if ((pos >= 54 || (pos >= 36 && ((pos % 9) & 1) == 0)) && isEmpty(pos - 9)) {
                        gen_addMove(moves, pos, pos - 9);
                    }
                } else {
                    /* go down */
                    if ((pos < 36 || (pos < 54 && ((pos % 9) & 1) == 0)) && isEmpty(pos + 9)) {
                        gen_addMove(moves, pos, pos + 9);
                    }
                }
                break;
            }

            default:
                break;
        }
    }
    
//    std::vector<MoveFull> moves2;
    for (auto && move : moves) {
        assert(isEmpty(move.dest));
//        if (isEmpty(move.dest)) {
//            moves2.push_back(move);
//        }
    }
    return moves;
}


FlipMode GenBoard::needSymmetryFlip() const
{
    for(auto sd = 1; sd >= 0; sd--) {
        /// count attackers only
        for(auto i = 5; i < 16;) {
            auto n = i < 11 ? 2 : 5, atkCnt = 0;
            auto midCol = false;
            int cols[5];
            
            for(auto j = 0; j < n; j++) {
                auto pos = pieceList[sd][i + j];
                if (pos >= 0) {
                    auto f = pos % 9;
                    cols[atkCnt++] = f;
                    if (f == 4) midCol = true;
                }
            }
            if (atkCnt > 0) {
                if (atkCnt > 1 && !midCol) {
                    /// WARNING: work with 2 pieces only (NOT 3 PAWNS yet)
                    if (cols[0] > 4) cols[0] = 8 - cols[0];
                    if (cols[1] > 4) cols[1] = 8 - cols[1];
                    midCol = cols[0] == cols[1];
                }
                return midCol ? FlipMode::horizontal : FlipMode::none;
            }
            i += n;
        }
    }

    /// defenders
    for(auto sd = 1; sd >= 0; sd--) {
        for(auto i = 0; i < 5; ++i) {
            auto pos = pieceList[sd][i];
            if (pos >= 0 && pos % 9 != 4) {
                return FlipMode::horizontal;
            }
        }
    }
    return FlipMode::none;
}


#endif /// _FELICITY_XQ_
