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
                break;
            }

            case PieceType::elephant:
            {
                break;
            }

            case PieceType::advisor:
            {
                break;
            }

            case PieceType::rook:
            {
                break;
            }

            case PieceType::cannon:
            {
                break;
            }

            case PieceType::horse:
            {
                break;
            }

            case PieceType::pawn:
            {
                genPawn_backward_quiet(moves, side, pos);
                break;
            }

            default:
                break;
        }
    }
    
    std::vector<MoveFull> moves2;
    for (auto && move : moves) {
        if (isEmpty(move.dest)) {
            moves2.push_back(move);
        }
    }
    return moves2;
}

void GenBoard::genPawn_backward_quiet(std::vector<MoveFull>& moves, Side side, int pos) const
{
//    if (side == Side::white) {
//        if (isEmpty(pos + 8)) {
//            gen_addPawnMove(moves, pos, pos + 8);
//        }
//        if (pos < 16 && isEmpty(pos + 8) && isEmpty(pos + 16)) {
//            gen_addMove(moves, pos, pos + 16);
//        }
//    } else {
//        if (isEmpty(pos - 8)) {
//            gen_addPawnMove(moves, pos, pos - 8);
//        }
//        if (pos >= 48 && isEmpty(pos - 8) && isEmpty(pos - 16)) {
//            gen_addMove(moves, pos, pos - 16);
//        }
//    }
}

FlipMode GenBoard::needSymmetryFlip() const
{
    assert(false);
    return FlipMode::none;
}


#endif /// _FELICITY_XQ_
