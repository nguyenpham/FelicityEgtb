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



#ifdef _FELICITY_XQ_


u64 EgtbFile::parseAttr(const std::string& name, EgtbIdxRecord* egtbIdxArray, int* pieceCount, u16 order)
{
    auto k = 0;
    auto side = Side::white;

    for (auto i = 0, sd = W, atkCnt = 0; i < (int)name.size(); i++) {
        auto ch = name[i];
        switch (ch) {
            case 'r':
            case 'c':
            case 'n':
            {
                EgtbIdx egtbIdx;
                if (name[i + 1] != ch) {
                    egtbIdx = atkCnt > 0 ? EGTB_IDX_R_FULL : EGTB_IDX_R_HALF;
                } else {
                    i++;
                    egtbIdx = atkCnt > 0 ? EGTB_IDX_RR_FULL : EGTB_IDX_RR_HALF;
                }
                
                auto d = static_cast<int>(Funcs::charactorToPieceType(ch)) - ROOK;
                egtbIdxArray[k].idx = EgtbIdx(egtbIdx + d);
                egtbIdxArray[k].side = side;
                atkCnt++;
                k++;
                break;
            }

            case 'p': {
                if (name[i + 1] != 'p') {
                    egtbIdxArray[k].idx = atkCnt > 0 ? EGTB_IDX_P_FULL : EGTB_IDX_P_HALF;
                } else {
                    i++;
                    if (name[i + 1] != 'p') {
                        egtbIdxArray[k].idx = atkCnt > 0 ? EGTB_IDX_PP_FULL : EGTB_IDX_PP_HALF;
                    } else {
                        i++;
                        egtbIdxArray[k].idx = atkCnt > 0 ? EGTB_IDX_PPP_FULL : EGTB_IDX_PPP_HALF;
                    }
                }
                egtbIdxArray[k].side = side;
                atkCnt++;
                k++;
                break;
            }

                /// Ignored
            case 'a':
            case 'b':
                break;

            case 'k':
            {
                sd = i == 0 ? W : B;
                side = static_cast<Side>(sd);

                auto a = 0, e = 0;
                for(auto j = i + 1; j < name.length(); j++) {
                    char ch = name[j];
                    if (ch == 'k') {
                        break;
                    }
                    if (ch == 'a') {
                        a++;
                    } else if (ch == 'b') {
                        e++;
                    }
                }

                switch (a + e) {
                    case 0:
                        egtbIdxArray[k].idx = EGTB_IDX_DK;
                        break;
                    case 1:
                        if (a) {
                            egtbIdxArray[k].idx = EGTB_IDX_DA;
                        } else {
                            egtbIdxArray[k].idx = EGTB_IDX_DB;
                        }
                        break;
                    case 2:
                            if (a == 0) {
                                egtbIdxArray[k].idx = EGTB_IDX_DBB;
                            } else if (a == 1) {
                                egtbIdxArray[k].idx = EGTB_IDX_DAB;
                            } else {
                                egtbIdxArray[k].idx = EGTB_IDX_DAA;
                            }
                        break;
                    case 3:
                            if (a==1) {
                                egtbIdxArray[k].idx = EGTB_IDX_DABB;
                            } else {
                                egtbIdxArray[k].idx = EGTB_IDX_DAAB;
                            }
                        break;
                    case 4:
                        egtbIdxArray[k].idx = EGTB_IDX_DAABB;
                        break;
                        
                    default:
                        assert(false);
                }
                
                egtbIdxArray[k++].side = side;
                break;
            }

            default:
                std::cout << "Fatal error for " << name << ", don't know piece charactor " << ch << std::endl;
                exit(0);
                break;
        }
    }

    assert(k >= 2);
    egtbIdxArray[k].idx = EGTB_IDX_NONE;
    egtbIdxArray[k].side = Side::none;
    
    
    /// pieceCount, size
    if (pieceCount) {
        memset(pieceCount, 0, sizeof(int) * 2 * 10);
        pieceCount[KING] = pieceCount[KING + 10] = 1;
    }

    i64 sz = 1;
    for (auto i = 0; i < k; i++) {
        auto a = egtbIdxArray[i].idx;
        assert(a != EGTB_IDX_NONE);
        
        auto pieceCountSd = egtbIdxArray[i].side == Side::white ? 10 : 0;
        auto pieceType = PieceType::empty;

        auto cnt = 1, h = 0;
        switch (a) {
            case EGTB_IDX_DK:
                h = EGTB_SIZE_K;
                break;

            case EGTB_IDX_DA:
                h = EGTB_SIZE_KA;
                pieceType = PieceType::advisor;
                break;

            case EGTB_IDX_DAA:
                h = EGTB_SIZE_KAA;
                pieceType = PieceType::advisor; cnt = 2;
                break;

            case EGTB_IDX_DB:
                h = EGTB_SIZE_KE;
                pieceType = PieceType::elephant;
                break;

            case EGTB_IDX_DBB:
                h = EGTB_SIZE_KEE;
                pieceType = PieceType::elephant; cnt = 2;
                break;

            case EGTB_IDX_DAB:
                h = EGTB_SIZE_KAE;
                pieceType = PieceType::advisor;
                if (pieceCount) {
                    pieceCount[pieceCountSd + ELEPHANT] = 1;
                }
                break;

            case EGTB_IDX_DABB:
                h = EGTB_SIZE_KAEE;
                pieceType = PieceType::advisor;
                if (pieceCount) {
                    pieceCount[pieceCountSd + ELEPHANT] = 2;
                }
                break;

            case EGTB_IDX_DAAB:
                h = EGTB_SIZE_KAAE;
                pieceType = PieceType::advisor; cnt = 2;
                if (pieceCount) {
                    pieceCount[pieceCountSd + ELEPHANT] = 1;
                }
                break;

            case EGTB_IDX_DAABB:
                h = EGTB_SIZE_KAAEE;
                pieceType = PieceType::advisor; cnt = 2;
                if (pieceCount) {
                    pieceCount[pieceCountSd + ELEPHANT] = 2;
                }
                break;

            case EGTB_IDX_R_HALF:
                h = EGTB_SIZE_X_HALF;
                pieceType = PieceType::rook;
                break;

            case EGTB_IDX_C_HALF:
                h = EGTB_SIZE_X_HALF;
                pieceType = PieceType::cannon;
                break;

            case EGTB_IDX_N_HALF:
                h = EGTB_SIZE_X_HALF;
                pieceType = PieceType::horse;
                break;

            case EGTB_IDX_P_HALF:
                h = EGTB_SIZE_P_HALF;
                pieceType = PieceType::pawn;
                break;

            case EGTB_IDX_R_FULL:
                h = EGTB_SIZE_X;
                pieceType = PieceType::rook;
                break;

            case EGTB_IDX_C_FULL:
                h = EGTB_SIZE_X;
                pieceType = PieceType::cannon;
                break;

            case EGTB_IDX_N_FULL:
                h = EGTB_SIZE_X;
                pieceType = PieceType::horse;
                break;

            case EGTB_IDX_P_FULL:
                h = EGTB_SIZE_P;
                pieceType = PieceType::pawn;
                break;

            case EGTB_IDX_RR_HALF:
                h = EGTB_SIZE_XX_HALF;
                pieceType = PieceType::rook; cnt = 2;
                break;

            case EGTB_IDX_RR_FULL:
                h = EGTB_SIZE_XX;
                pieceType = PieceType::rook; cnt = 2;
                break;

            case EGTB_IDX_CC_HALF:
                h = EGTB_SIZE_XX_HALF;
                pieceType = PieceType::cannon; cnt = 2;
                break;
                
            case EGTB_IDX_CC_FULL:
                h = EGTB_SIZE_XX;
                pieceType = PieceType::cannon; cnt = 2;
                break;

            case EGTB_IDX_NN_HALF:
                h = EGTB_SIZE_XX_HALF;
                pieceType = PieceType::horse; cnt = 2;
                break;
            case EGTB_IDX_NN_FULL:
                h = EGTB_SIZE_XX;
                pieceType = PieceType::horse; cnt = 2;
                break;

            case EGTB_IDX_PP_HALF:
                h = EGTB_SIZE_PP_HALF;
                pieceType = PieceType::pawn; cnt = 2;
                break;

            case EGTB_IDX_PP_FULL:
                h = EGTB_SIZE_PP;
                pieceType = PieceType::pawn; cnt = 2;
                break;

            case EGTB_IDX_PPP_HALF:
                h = EGTB_SIZE_PPP_HALF;
                pieceType = PieceType::pawn; cnt = 3;
                break;

            default:
                assert(false);
                break;
        }

        if (pieceCount && pieceType != PieceType::empty) {
            assert(cnt > 0);
            pieceCount[pieceCountSd + static_cast<int>(pieceType)] = cnt;
        }

        assert(h > 0);
        egtbIdxArray[i].factor = h;
        sz *= (i64)h;
    }

    if (order != 0) {
        const int orderArray[] = { order & 0x7, (order >> 3) & 0x7, (order >> 6) & 0x7, (order >> 9) & 0x7, (order >> 12) & 0x7 };
        EgtbIdxRecord tmpArr[10];
        memcpy(tmpArr, egtbIdxArray, k * sizeof(EgtbIdxRecord));
        for(auto i = 0; i < k; i++) {
            auto x = orderArray[i];
            egtbIdxArray[x] = tmpArr[i];
        }
    }

    for(auto i = 0; i < k; i++) {
        egtbIdxArray[i].mult = 1;
        for (auto j = i + 1; j < k; j++) {
            egtbIdxArray[i].mult *= (i64)egtbIdxArray[j].factor;
        }
    }

    assert(sz);
    return sz;
}


bool EgtbFile::setupBoard(EgtbBoard& board, i64 idx, FlipMode flipMode, Side firstsider) const
{
    board.reset();

    auto order = header ? header->getOrder() : 0;

    i64 rest = idx;

    for(auto i = 0; ; i++) {
        assert(i < 16);
        auto rec = egtbIdxArray[i];
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
            case EGTB_IDX_DK:
            case EGTB_IDX_DA:
            case EGTB_IDX_DB:
            case EGTB_IDX_DAA:
            case EGTB_IDX_DBB:
            case EGTB_IDX_DAB:
            case EGTB_IDX_DAAB:
            case EGTB_IDX_DABB:
            case EGTB_IDX_DAABB:
            {
                if (!egtbKey.setupBoard_defence(board, rec.idx, key, side, flipMode)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_R_HALF:
            case EGTB_IDX_C_HALF:
            case EGTB_IDX_N_HALF:
            case EGTB_IDX_P_HALF:

            case EGTB_IDX_R_FULL:
            case EGTB_IDX_C_FULL:
            case EGTB_IDX_N_FULL:
            case EGTB_IDX_P_FULL:
            {
                if (!egtbKey.setupBoard_oneStrongPiece(board, rec.idx, key, side, flipMode)) {
                    return false;
                }
                break;
            }

                /// WARNING: not yet 3 strong pieces
            default:
            {
                if (!egtbKey.setupBoard_twoStrongPieces(board, rec.idx, key, side, flipMode)) {
                    return false;
                }
                break;
            }
        }
    }

    board.flip(flipMode);
    return true;
}

#endif // _FELICITY_XQ_
