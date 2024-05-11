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

#include <fstream>
#include <iomanip>
#include <ctime>

#include "egtb.h"
#include "egtbfile.h"
#include "egtbkey.h"

#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;



#ifdef _FELICITY_CHESS_

extern int subppp_sizes[7];
extern int *kk_2, *kk_8;

i64 EgtbFile::parseAttr(const std::string& name, EgtbIdxRecord* egtbIdxArray, int* pieceCount, u16 order)
{
    if (pieceCount) {
        memset(pieceCount, 0, 2 * 10 * sizeof(int));
        pieceCount[KING] = pieceCount[KING + 10] = 1;
    }
    
    static const int sizeArrX[] = { EGTB_SIZE_X, EGTB_SIZE_XX, EGTB_SIZE_XXX, EGTB_SIZE_XXXX, EGTB_SIZE_XXXXX, 0, 0 };
    static const int sizeArrP[] = { EGTB_SIZE_P, EGTB_SIZE_PP, EGTB_SIZE_PPP, EGTB_SIZE_PPPP, EGTB_SIZE_PPPPP, 0, 0 };

    i64 sz = 1;
    auto side = Side::white;
    auto k = 0;
    for (auto i = 0, atkCnt = 0, d = 10; i < (int)name.size(); i++) {
        auto ch = name[i];
        auto type = Funcs::charactorToPieceType(ch);
        assert(type != PieceType::empty);
        
        switch (ch) {
            case 'k':
            {
                if (pieceCount) {
                    pieceCount[d + KING] = 1;
                }

                auto idx = 0, h = 0;
                if (i == 0) {
                    auto hasPawns = name.find('p') != std::string::npos;
                    idx = hasPawns ? EGTB_IDX_KK_2 : EGTB_IDX_KK_8;
                    h = hasPawns ? EGTB_SIZE_KK2 : EGTB_SIZE_KK8;
                    sz *= h;
                } else {
                    side = Side::black;
                    d = 0;
                    /// Ignore this king, avoid increasing k
                    continue;
                }


                egtbIdxArray[k].idx = (EgtbIdx)idx;
                egtbIdxArray[k].side = side;
                egtbIdxArray[k].factor = h;
                k++;
                break;
            }
            case 'q':
            case 'r':
            case 'b':
            case 'n':
            case 'p':
            {
                auto t = 1;
                while (name[i + t] == ch) {
                    t++;
                }

                assert(t <= 5);
                i += t - 1;
                
                if (pieceCount) {
                    pieceCount[d + static_cast<int>(type)] = t;
                }
                
                auto idx = EGTB_IDX_Q + static_cast<int>(type) - QUEEN + 5 * (t - 1);
                egtbIdxArray[k].idx = (EgtbIdx)idx;
                egtbIdxArray[k].side = side;

                auto h = type == PieceType::pawn ? sizeArrP[t - 1] : sizeArrX[t - 1];
                sz *= h;
                egtbIdxArray[k].factor = h;
                atkCnt++;
                k++;
                break;
            }
        }
    }
    
    assert(k >= 2);
    egtbIdxArray[k].idx = EGTB_IDX_NONE;
    egtbIdxArray[k].side = Side::none;


    for(auto i = 0; i < k; i++) {
        egtbIdxArray[i].mult = 1;
        for (int j = i + 1; j < k; j++) {
            egtbIdxArray[i].mult *= (i64)egtbIdxArray[j].factor;
        }
    }

    assert(sz);
    return sz;
}

extern const int tb_kIdxToPos[10];

bool EgtbFile::setupBoard(EgtbBoard& board, i64 idx, FlipMode flipMode, Side firstsider) const
{
    board.reset();

    auto order = header ? header->getOrder() : 0;
    auto orderVec = order2Vec(order);
    
    i64 rest = idx;

    for(auto i = 0; ; i++) {
        auto x = orderVec[i]; assert(x >= 0 && x < 16);
        auto rec = egtbIdxArray[x];
        if (rec.idx == EGTB_IDX_NONE) {
            break;
        }

        auto side = rec.side;
        if (firstsider == Side::black) {
            side = getXSide(side);
        }

        auto key = (int)(rest / rec.mult);
        rest = rest % rec.mult;

        switch (rec.idx) {
            case EGTB_IDX_KK_2:
            {
                int kk = kk_2[key];
                int k0 = kk >> 8, k1 = kk & 0xff;
                board.setPiece(k0, Piece(PieceType::king, side));
                board.setPiece(k1, Piece(PieceType::king, getXSide(side)));
                break;
            }

            case EGTB_IDX_KK_8:
            {
                auto kk = kk_8[key];
                auto k0 = kk >> 8, k1 = kk & 0xff;
                assert(k0 != k1 && board.isPositionValid(k0) && board.isPositionValid(k1));
                board.setPiece(k0, Piece(PieceType::king, side));
                board.setPiece(k1, Piece(PieceType::king, getXSide(side)));
                break;
            }

            case EGTB_IDX_Q:
            case EGTB_IDX_R:
            case EGTB_IDX_B:
            case EGTB_IDX_N:
            case EGTB_IDX_P:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_Q + QUEEN);
                if (!egtbKey.setupBoard_x(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_QQ:
            case EGTB_IDX_RR:
            case EGTB_IDX_BB:
            case EGTB_IDX_NN:
            case EGTB_IDX_PP:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_QQ + QUEEN);
                if (!egtbKey.setupBoard_xx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_QQQ:
            case EGTB_IDX_RRR:
            case EGTB_IDX_BBB:
            case EGTB_IDX_HHH:
            case EGTB_IDX_PPP:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_QQQ + QUEEN);
                if (!egtbKey.setupBoard_xxx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_QQQQ:
            case EGTB_IDX_RRRR:
            case EGTB_IDX_BBBB:
            case EGTB_IDX_HHHH:
            case EGTB_IDX_PPPP:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_QQQQ + QUEEN);
                if (!egtbKey.setupBoard_xxxx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            default:
                assert(false);
                break;
        }
    }

    board.flip(flipMode);
    return true;
}

#endif // _FELICITY_CHESS_
