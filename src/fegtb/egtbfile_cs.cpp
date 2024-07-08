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


u64 EgtbFile::parseAttr(const std::string& name, EgtbIdxRecord* egtbIdxArray, int* pieceCount, u16 order)
{
    if (pieceCount) {
        memset(pieceCount, 0, 2 * 10 * sizeof(int));
        pieceCount[KING] = pieceCount[KING + 10] = 1;
    }
    
    static const i64 sizeArrX[] = { EGTB_SIZE_X, EGTB_SIZE_XX, EGTB_SIZE_XXX, EGTB_SIZE_XXXX, EGTB_SIZE_XXXXX, EGTB_SIZE_XXXXXX, EGTB_SIZE_XXXXXXX, EGTB_SIZE_XXXXXXXX, EGTB_SIZE_XXXXXXXXX, EGTB_SIZE_XXXXXXXXXX, 0, 0 };
    static const int sizeArrP[] = { EGTB_SIZE_P, EGTB_SIZE_PP, EGTB_SIZE_PPP, EGTB_SIZE_PPPP, EGTB_SIZE_PPPPP, EGTB_SIZE_PPPPPP, EGTB_SIZE_PPPPPPP, EGTB_SIZE_PPPPPPPP, 0, 0 };

    u64 sz = 1;
    auto side = Side::white;
    auto k = 0, pCnt = 2; /// two Kings
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
                    sz *= u64(h);
                    assert(h > 0);
                    assert(sz > 0);
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

                assert(t <= 10);
                i += t - 1;
                
                if (pieceCount) {
                    pieceCount[d + static_cast<int>(type)] = t;
                }
                
                auto idx = EGTB_IDX_Q + static_cast<int>(type) - QUEEN + 5 * (t - 1);
                egtbIdxArray[k].idx = (EgtbIdx)idx;
                egtbIdxArray[k].side = side;

                auto h = type == PieceType::pawn ? sizeArrP[t - 1] : sizeArrX[t - 1];
                
                if (h == EGTB_SIZE_X) {
                    assert(t == 1 && type != PieceType::pawn);
                    h -= pCnt;
                }
                pCnt += t;

                sz *= u64(h);
                assert(h > 0);
                assert(sz > 0);

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

    assert(sz > 0);
    return sz;
}

bool EgtbFile::setupBoard(EgtbBoard& board, i64 idx, FlipMode flipMode, Side firstSide) const
{
    board.reset();

    i64 rest = idx;

    std::vector<int> piecePosVec;

    for(auto i = 0; ; i++) {
        assert(i < 16);
        auto rec = egtbIdxArray[i];
        if (rec.idx == EGTB_IDX_NONE) {
            break;
        }

        auto side = rec.side;
        if (firstSide == Side::black) {
            side = getXSide(side);
        }

        auto key = (int)(rest / rec.mult);
        rest = rest % rec.mult;

        switch (rec.idx) {
            case EGTB_IDX_KK_2:
            {
//                int kk = tb_kk_2[key];
                auto kk = EgtbKey::tb_kk_2_key_map[key];
                auto k0 = kk >> 8, k1 = kk & 0xff;
                board.setPiece(k0, Piece(PieceType::king, side));
                board.setPiece(k1, Piece(PieceType::king, getXSide(side)));
                piecePosVec.push_back(k0);
                piecePosVec.push_back(k1);
                break;
            }

            case EGTB_IDX_KK_8:
            {
//                auto kk = tb_kk_8[key];
                assert(EgtbKey::tb_kk_8_key_map.find(key) != EgtbKey::tb_kk_8_key_map.end());
                auto kk = EgtbKey::tb_kk_8_key_map[key];
                auto k0 = kk >> 8, k1 = kk & 0xff;
                assert(k0 != k1 && board.isPositionValid(k0) && board.isPositionValid(k1));
                board.setPiece(k0, Piece(PieceType::king, side));
                board.setPiece(k1, Piece(PieceType::king, getXSide(side)));
                piecePosVec.push_back(k0);
                piecePosVec.push_back(k1);
                break;
            }

            case EGTB_IDX_Q:
            case EGTB_IDX_R:
            case EGTB_IDX_B:
            case EGTB_IDX_N:
            case EGTB_IDX_P:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_Q + QUEEN);
                
                if (type != PieceType::pawn) {
                    std::sort(piecePosVec.begin(), piecePosVec.end());

                    for(auto && p : piecePosVec) {
                        if (p <= key) {
                            key++;
                        }
                    }
                }
                
                auto pos = egtbKey.setupBoard_x(board, key, type, side);
                if (pos < 0) {
                    return false;
                }
                
                piecePosVec.push_back(pos);

                break;
            }

            case EGTB_IDX_QQ:
            case EGTB_IDX_RR:
            case EGTB_IDX_BB:
            case EGTB_IDX_NN:
            case EGTB_IDX_PP:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_QQ + QUEEN);
                auto vec = egtbKey.setupBoard_xx(board, key, type, side);
                if (vec.empty()) {
                    return false;
                }
                assert(vec.size() == 2);
                //piecePosVec.insert(piecePosVec.end(), vec.begin(), vec.end());
                piecePosVec.push_back(vec[0]);
                piecePosVec.push_back(vec[1]);
                break;
            }

            case EGTB_IDX_QQQ:
            case EGTB_IDX_RRR:
            case EGTB_IDX_BBB:
            case EGTB_IDX_HHH:
            case EGTB_IDX_PPP:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_QQQ + QUEEN);
                auto vec = egtbKey.setupBoard_xxx(board, key, type, side);
                if (vec.empty()) {
                    return false;
                }
                assert(vec.size() == 3);
                piecePosVec.insert(piecePosVec.end(), vec.begin(), vec.end());
                break;
            }

            case EGTB_IDX_QQQQ:
            case EGTB_IDX_RRRR:
            case EGTB_IDX_BBBB:
            case EGTB_IDX_HHHH:
            case EGTB_IDX_PPPP:
            {
                auto type = static_cast<PieceType>(rec.idx - EGTB_IDX_QQQQ + QUEEN);
                auto vec = egtbKey.setupBoard_xxxx(board, key, type, side);
                if (vec.empty()) {
                    return false;
                }
                assert(vec.size() == 4);
                piecePosVec.insert(piecePosVec.end(), vec.begin(), vec.end());
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
