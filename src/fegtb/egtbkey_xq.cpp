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

#include "egtb.h"
#include "egtbkey.h"
#include "../base/funcs.h"



#ifdef _FELICITY_XQ_

using namespace fegtb;
using namespace bslib;

namespace fegtb {

    const int blackElephantPosWOMiddle[6] = { 2, 6, 18, 26, 38, 42 };
    const int blackElephantCombineWOMiddle[21] = {
         2 << 8,  2 << 8 |  6,  2 << 8 | 18,  2 << 8 | 26, 2 << 8 | 38, 2 << 8 | 42,
         6 << 8,  6 << 8 | 18,  6 << 8 | 26,  6 << 8 | 38, 6 << 8 | 42,
        18 << 8, 18 << 8 | 26, 18 << 8 | 38, 18 << 8 | 42,
        26 << 8, 26 << 8 | 38, 26 << 8 | 42,
        38 << 8, 38 << 8 | 42,
        42 << 8
    };


extern const char* pieceTypeName;
extern const int egtbPieceListStartIdxByType[8];
extern const PieceType egtbPieceListIdxToType[16];

const int egtbPieceListStartIdxByType[8] = { -1, 0, 1, 3, 5, 7, 9, 11 };


} // namespace

//extern const int exchangePieceValue[8];
static const int exchangePieceValue[8] = { -1, 10000, 200, 200, 1000, 500, 500, 200 };


extern const PieceType egtbPieceListIdxToType[16];
extern const int pieceValForOrdering[7];
const int pieceValForOrdering[7] = { 0, 0, 0, 50, 20, 5, 1 };

static const int a_pos[] = { 3, 5, 13, 21, 23 };
static const int e_pos[] = { 2, 6, 18, 22, 26, 38, 42 };

static const int tbkey_k[9] = { 3, 4, 5, 12, 13, 14, 21, 22, 23 };
static int tbkey_ka[40];
static int tbkey_kb[62];
static int tbkey_kaa[70];
static int tbkey_kab[275];
static int tbkey_kbb[183];
static int tbkey_kaab[480];
static int tbkey_kabb[810];
static int tbkey_kaabb[1410];

//static const int tbkey_km[17] = { 3, 4, 5, 12, 13, 14, 21, 22, 23, 3|22<<6, 4|22<<6, 5|22<<6, 12|22<<6, 13|22<<6, 14|22<<6, 21|22<<6, 23|22<<6 };
//static int tbkey_kam[40 + 35];
//static int tbkey_kaam[70 + 60];

static int tbkey_xx[EGTB_SIZE_XX_HALF];
static int tbkey_xx_full[EGTB_SIZE_XX];

static int tbkey_pp[EGTB_SIZE_PP_HALF];
static int tbkey_pp_full[EGTB_SIZE_PP];


const int egtbHalfBoard_PosToIdx[90] = {
    0,  1, 2, 3,  4,  3, 2, 1, 0,
    5,  6, 7, 8,  9,  8, 7, 6, 5,
    10,11,12,13, 14, 13,12,11,10,
    15,16,17,18, 19, 18,17,16,15,
    20,21,22,23, 24, 23,22,21,20,

    25,26,27,28, 29, 28,27,26,25,
    30,31,32,33, 34, 33,32,31,30,
    35,36,37,38, 39, 38,37,36,35,
    40,41,42,43, 44, 43,42,41,40,
    45,46,47,48, 49, 48,47,46,45
};

static int halfBoard_IdxToPos[] = {
    0, 1, 2, 3, 4,     // 5, 6, 7, 8,
    9,10,11,12,13,     //14,15,16,17,
    18,19,20,21,22,    //23,24,25,26,
    27,28,29,30,31,    //32,33,34,35,
    36,37,38,39,40,    //41,42,43,44,

    45,46,47,48,49,    //50,51,52,53,
    54,55,56,57,58,    //59,60,61,62,
    63,64,65,66,67,    //68,69,70,71,
    72,73,74,75,76,    //77,78,79,80,
    81,82,83,84,85,    //86,87,88,89
};

static const int pawnPosToIdx[90] = {
    0, 1, 2, 3,   4,  5, 6, 7, 8,
    9,10,11,12,  13, 14,15,16,17,
    18,19,20,21, 22, 23,24,25,26,
    27,28,29,30, 31, 32,33,34,35,
    36,37,38,39, 40, 41,42,43,44,

    45,-1,46,-1, 47, -1,48,-1,49,
    50,-1,51,-1, 52, -1,53,-1,54,
    -1,-1,-1,-1, -1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1, -1,-1,-1,-1
};

static const int pawnIdxToPos[90] = {
    0, 1, 2, 3,   4,  5, 6, 7, 8,
    9,10,11,12,  13, 14,15,16,17,
    18,19,20,21, 22, 23,24,25,26,
    27,28,29,30, 31, 32,33,34,35,
    36,37,38,39, 40, 41,42,43,44,

    45,47,49,51, 53, 54,56,58,60,
    62,-1,-1,-1, -1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1, -1,-1,-1,-1
};

static const int halfBoard_PawnPosToIdx[90] = {
    0, 1, 2, 3,  4,  3, 2, 1, 0,
    5, 6, 7, 8,  9,  8, 7, 6, 5,
    10,11,12,13, 14, 13,12,11,10,
    15,16,17,18, 19, 18,17,16,15,
    20,21,22,23, 24, 23,22,21,20,

    25,-1,26,-1, 27, -1,26,-1,25,
    28,-1,29,-1, 30, -1,29,-1,28,
    -1,-1,-1,-1, -1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1, -1,-1,-1,-1
};

static int halfBoard_PawnIdxToPos[] = {
     0, 1, 2, 3, 4,     // 5, 6, 7, 8,
     9,10,11,12,13,     //14,15,16,17,
    18,19,20,21,22,     //23,24,25,26,
    27,28,29,30,31,     //32,33,34,35,
    36,37,38,39,40,     //41,42,43,44,

    45,  47,    49,
    54,  56,    58,
    -1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,
};



static int pppSearchKey(int pos0, int pos1, int pos2) {
    int x0 = std::min(pos0, std::min(pos1, pos2));
    int x2 = std::max(pos0, std::max(pos1, pos2));
    int x1 = pos0 != x0 && pos0 != x2 ? pos0 : pos1 != x0 && pos1 != x2 ? pos1 : pos2;

    int ppp = x0 << 16 | x1 << 8 | x2;
    return ppp;
}


void EgtbKey::createPawnKeys() {
    int pppIdx = 0;

    auto k = 0, previousppp = -1;
    for (int i = 0; ; i++) {
        auto p0 = halfBoard_PawnIdxToPos[i];
        if (p0 < 0) {
            break;
        }
        
        int x0 = Funcs::flip(p0, FlipMode::horizontal);

        int r = getRow(p0), c0 = getCol(p0), f4 = 4 - c0;

        for (int p1 = p0 + 1; p1 <= 62; p1++) {
            if (pawnPosToIdx[p1] < 0) { // || (p0 > 44 && p1 - p0 == 9)) {
                continue;
            }
            auto c1 = getCol(p1), r1 = getRow(p1);
            if ((r == r1 && f4 < abs(c1 - 4)) || (f4 == 0 && c1 > 4)) {
                continue;
            }

            int x1 = Funcs::flip(p1, FlipMode::horizontal);

            tbkey_pp[k] = p0 << 8 | p1;
            k++;

            if (p0 > 44 && p1 - p0 == 9) {
                continue;
            }

            for (int p2 = p1 + 1; p2 <= 62; p2++) {
                if (pawnPosToIdx[p2] < 0 || (p0 > 44 && p2 - p0 == 9) || (p1 > 44 && p2 - p1 == 9)) {
                    continue;
                }

                int x2 = Funcs::flip(p2, FlipMode::horizontal);

                int ppp = p0 << 16 | p1 << 8 | p2;
                int yyy = pppSearchKey(x0, x1, x2);
                
                auto a = pppPos2KeyMap.find(ppp);
                if (a == pppPos2KeyMap.end()) {
                    a = pppPos2KeyMap.find(yyy);
                }

                if (a == pppPos2KeyMap.end()) {
                    previousppp = ppp;
                    pppPos2KeyMap[ppp] = pppIdx;

                    if (ppp != yyy) {
                        pppPos2KeyMap[yyy] = pppIdx | static_cast<int>(FlipMode::horizontal) << 24;
                    }

                    pppKeyToPos[pppIdx] = ppp;
                    pppIdx++;
                }
            }
        }
    }
    
    int ppIdx = 0;
    for (int i = 0; i < 9 * 7; i++) {
        if (halfBoard_PawnPosToIdx[i] < 0) {
            continue;
        }
        for (int j = i + 1; j < 9 * 7; j++) {
            if (halfBoard_PawnPosToIdx[j] < 0) {
                continue;
            }
            tbkey_pp_full[ppIdx++] = i << 8 | j;
        }
    }
}

static void createXXKeys() {
    int k = 0;
    for (int i = 0; i < 50; i++) {
        auto p0 = halfBoard_IdxToPos[i];
        int r = getRow(p0), f4 = 4 - getCol(p0);

        for (int p1 = p0 + 1; p1 < 90; p1++) {
            auto c = getCol(p1);
            if ((r == getRow(p1) && f4 < abs(c - 4)) || (f4 == 0 && c > 4)) {
                continue;
            }

            tbkey_xx[k++] = p0 << 8 | p1;
        }
    }

    k = 0;
    for (int p0 = 0; p0 < 90; p0++) {
        for (int p1 = p0 + 1; p1 < 90; p1++) {
            tbkey_xx_full[k++] = p0 << 8 | p1;
        }
    }
}

int EgtbKey::getKey_pp(int pos0, int pos1) const
{
    int r0 = getRow(pos0), r1 = getRow(pos1);

    if (r0 > r1 || (r0 == r1 && abs(getCol(pos0) - 4) < abs(getCol(pos1) - 4) )) {
        int x = pos0; pos0 = pos1; pos1 = x;
    }

    int c0 = getCol(pos0);
    if (c0 > 4 || (c0 == 4 && getCol(pos1) > 4)) {
        pos0 = Funcs::flip(pos0, FlipMode::horizontal);
        pos1 = Funcs::flip(pos1, FlipMode::horizontal);
    }

    int x = pos0 << 8 | pos1;
    int sz = sizeof(tbkey_pp) / sizeof(int);
    int key = Funcs::bSearch(tbkey_pp, sz, x);
    return key;
}

int EgtbKey::getKey_pp_full(int pos0, int pos1) const
{
    if (pos0 > pos1) {
        std::swap(pos0, pos1);
    }

    int x = pos0 << 8 | pos1;
    int key = Funcs::bSearch(tbkey_pp_full, EGTB_SIZE_PP, x); assert(key >= 0 && key < EGTB_SIZE_PP);
    return key;

}

int EgtbKey::getKeyFlip_ppp(int pos0, int pos1, int pos2) const
{
    assert(pos0 >= 0 && pos1 >= 0 && pos2 >= 0);
    auto ppp = pppSearchKey(pos0, pos1, pos2);
    
    auto it = pppPos2KeyMap.find(ppp);
    return it == pppPos2KeyMap.end() ? -1 : it->second;
}

int EgtbKey::getKeyFlip_ppp_full(int pos0, int pos1, int pos2) const
{
    assert(false);
    return 0;
}


int EgtbKey::getKey_xx(int pos0, int pos1)
{
    int r0 = getRow(pos0), r1 = getRow(pos1);

    if (r0 > r1 || (r0 == r1 && abs(getCol(pos0) - 4) < abs(getCol(pos1) - 4) )) {
        int x = pos0; pos0 = pos1; pos1 = x;
    }

    int c0 = getCol(pos0);
    if (c0 > 4 || (c0 == 4 && getCol(pos1) > 4)) {
        pos0 = Funcs::flip(pos0, FlipMode::horizontal);
        pos1 = Funcs::flip(pos1, FlipMode::horizontal);
    }

    int x = pos0 << 8 | pos1;
    int key = Funcs::bSearch(tbkey_xx, EGTB_SIZE_XX_HALF, x);
    return key;
}

int EgtbKey::getKey_xx_full(int pos0, int pos1)
{
    if (pos0 > pos1) {
        std::swap(pos0, pos1);
    }
    
    int x = pos0 << 8 | pos1;
    int key = Funcs::bSearch(tbkey_xx_full, EGTB_SIZE_XX, x);
    return key;
}

int EgtbKey::getKey_xy(int pos0, int pos1)
{
    int c0 = getCol(pos0);
    if (c0 > 4) {
        pos0 = Funcs::flip(pos0, FlipMode::horizontal);
        pos1 = Funcs::flip(pos1, FlipMode::horizontal);
    }

    int idx0 = egtbHalfBoard_PosToIdx[pos0];
    int key = idx0 * EGTB_SIZE_X + pos1;
    return key;
}

int EgtbKey::getKey_xp(int pos0, int pos1)
{
    int c0 = getCol(pos0);
    if (c0 > 4) {
        pos0 = Funcs::flip(pos0, FlipMode::horizontal);
        pos1 = Funcs::flip(pos1, FlipMode::horizontal);
    }

    int idx0 = egtbHalfBoard_PosToIdx[pos0];
    int idx1 = pawnPosToIdx[pos1];
    int key = idx0 * EGTB_SIZE_P + idx1;
    return key;
}

int EgtbKey::getKey_defence(int k, int a1, int a2, int e1, int e2)
{
    assert(k < 45 && a1 < 45 && a2 < 45 && e1 < 45 && e2 < 45);
    const int* array;
    int sz;
    int key = k;

    if (a1 > 0) {
        if (a2 > 0) {
            key |= a1 < a2 ? (a1 << 18 | a2 << 24) : (a2 << 18 | a1 << 24);
            if (e1 > 0) {
                if (e2 > 0) {
                    array = tbkey_kaabb; sz = sizeof(tbkey_kaabb)/sizeof(int);
                } else {
                    array = tbkey_kaab; sz = sizeof(tbkey_kaab)/sizeof(int);
                }
            } else {
                array = tbkey_kaa; sz = sizeof(tbkey_kaa)/sizeof(int);
            }
        } else {
            key |= a1 << 18;

            if (e1 > 0) {
                if (e2 > 0) {
                    array = tbkey_kabb; sz = sizeof(tbkey_kabb)/sizeof(int);
                } else {
                    array = tbkey_kab; sz = sizeof(tbkey_kab)/sizeof(int);
                }
            } else {
                array = tbkey_ka; sz = sizeof(tbkey_ka)/sizeof(int);
            }
        }
    } else {
        if (e1 > 0) {
            if (e2 > 0) {
                array = tbkey_kbb; sz = sizeof(tbkey_kbb)/sizeof(int);
            } else {
                array = tbkey_kb; sz = sizeof(tbkey_kb)/sizeof(int);
            }
        } else {
            array = tbkey_k; sz = sizeof(tbkey_k)/sizeof(int);
        }
    }

    if (e1 > 0) {
            if (e2 > 0) {
                key |= e1 < e2 ? (e1 << 6 | e2 << 12) : (e2 << 6 | e1 << 12);
            } else {
                key |= e1 << 6;
            }
    }

    return Funcs::bSearch(array, sz, key);
}


static void createDefenderKeys() {
    auto ka = 0, ke = 0, kaa = 0, kae = 0, kee = 0, kaee = 0, kaae = 0, kaaee = 0;
    for (auto i = 0; i < 9; i++) {
        auto k = tbkey_k[i];

        /// A, AB, AA
        for (auto i0 = 0; i0 < 5; i0++) {

            auto a0 = a_pos[i0];
            if (a0 == k) {
                continue;
            }

            /// A
            auto t = k | a0 << 18;
            tbkey_ka[ka++] = t;

            /// AB
            for (auto j0 = 0; j0 < 7; j0++) {
                auto e0 = e_pos[j0];
                if (e0 == k) {
                    continue;
                }

                t =  k | a0 << 18 | e0 << 6;
                tbkey_kab[kae++] = t;

                /// ABB
                for (auto j1 = j0 + 1; j1 < 7; j1++) {
                    auto e1 = e_pos[j1];
                    if (e1 == k) {
                        continue;
                    }

                    tbkey_kabb[kaee++] = k | a0 << 18 | e0 << 6 | e1 << 12;
                }
            }

            /// AA
            for (auto i1 = i0 + 1; i1 < 5; i1++) {
                auto a1 = a_pos[i1];
                if (a1 == k) {
                    continue;
                }

                t = k | a0 << 18 | a1 << 24;
                tbkey_kaa[kaa++] = t;

                /// AAB,  AABB
                for (auto j0 = 0; j0 < 7; j0++) {
                    auto e0 = e_pos[j0];
                    if (e0 == k) {
                        continue;
                    }

                    auto t = k | a0 << 18 | a1 << 24 | e0 << 6;
                    tbkey_kaab[kaae++] = t;

                    /// AABB
                    for (auto j1 = j0 + 1; j1 < 7; j1++) {
                        auto e1 = e_pos[j1];
                        if (e1 == k) {
                            continue;
                        }

                        auto idx = k | a0 << 18 | a1 << 24 | e0 << 6 | e1 << 12;
                        tbkey_kaabb[kaaee++] = idx;
                    }
                }

            }
        }

        // B & BB
        for (auto j0 = 0; j0 < 7; j0++) {
            auto e0 = e_pos[j0];
            if (e0 == k) {
                continue;
            }

            tbkey_kb[ke++] = k | e0 << 6;

            for (auto j1 = j0 + 1; j1 < 7; j1++) {
                auto e1 = e_pos[j1];
                if (e1 == k) {
                    continue;
                }

                tbkey_kbb[kee++] = k | e0 << 6 | e1 << 12;
            }
        }
    }

    Funcs::sort_tbkeys(tbkey_ka, sizeof(tbkey_ka) / sizeof(int));
    Funcs::sort_tbkeys(tbkey_kb, sizeof(tbkey_kb) / sizeof(int));
    Funcs::sort_tbkeys(tbkey_kaa, sizeof(tbkey_kaa) / sizeof(int));
    Funcs::sort_tbkeys(tbkey_kab, sizeof(tbkey_kab) / sizeof(int));

    Funcs::sort_tbkeys(tbkey_kbb, sizeof(tbkey_kbb) / sizeof(int));
    Funcs::sort_tbkeys(tbkey_kabb, sizeof(tbkey_kabb) / sizeof(int));
    Funcs::sort_tbkeys(tbkey_kaab, sizeof(tbkey_kaab) / sizeof(int));
    Funcs::sort_tbkeys(tbkey_kaabb, sizeof(tbkey_kaabb) / sizeof(int));
}

void EgtbKey::initOnce() {
    createDefenderKeys();
    createPawnKeys();
    createXXKeys();
}


/// Convert board into key

EgtbKeyRec EgtbKey::getKey(const EgtbBoard& board, const EgtbIdxRecord* egtbIdxRecord, u32 order)
{
    EgtbKeyRec rec;
    
    /// Check which side for left hand side (stronger side)
    int mat[] = { 0, 0 }, strongmat[] = { 0, 0 };
    
    for (auto sd = 0; sd < 2; sd++) {
        for(auto i = 1; i < 16; i++) {
            auto pos = board.pieceList[sd][i];
            if (pos >= 0) {
                auto piece = board.getPiece(pos);
                auto v = exchangePieceValue[static_cast<int>(piece.type)];
                mat[sd] += v;
                if (piece.type >= PieceType::rook) {
                    strongmat[sd] += v;
                }
            }
        }
    }
    
    auto strongSd = W;
    auto flipMode = FlipMode::none;
    if (strongmat[B] > strongmat[W] || (strongmat[B] == strongmat[W] && mat[B] > mat[W])) {
        strongSd = B;
        flipMode = FlipMode::rotate;
    }
    
    rec.flipSide = strongSd == B;
    
    /// Calculate key
    i64 key = 0;
    
    for(auto i = 0; ; i++) {
        auto attr = egtbIdxRecord[i].idx;
        if (attr == EGTB_IDX_NONE) {
            break;
        }
        auto mul = egtbIdxRecord[i].mult;
        auto side = egtbIdxRecord[i].side;
        if (rec.flipSide) {
            side = getXSide(side);
        }
        const auto sd = static_cast<int>(side);
        
        switch (attr) {
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
                /// King
                auto king = board.pieceList[sd][0];
                assert(board.isPositionValid(king)); // king
                
                auto d = egtbPieceListStartIdxByType[ADVISOR];
                
                /// Advisors, Elephants
                auto a0 = board.pieceList[sd][d];
                auto a1 = board.pieceList[sd][d + 1];
                auto e0 = board.pieceList[sd][d + 2];
                auto e1 = board.pieceList[sd][d + 3];
                
                
                auto dFlip = side == Side::black ? flipMode :
                Funcs::flip(flipMode, FlipMode::rotate);
                
                king = Funcs::flip(king, dFlip);
                if (a0 > 0) {
                    a0 = Funcs::flip(a0, dFlip);
                }
                if (a1 > 0) {
                    a1 = Funcs::flip(a1, dFlip);
                }
                if (e0 > 0) {
                    e0 = Funcs::flip(e0, dFlip);
                }
                if (e1 > 0) {
                    e1 = Funcs::flip(e1, dFlip);
                }
                
                auto h = getKey_defence(king, a0, a1, e0, e1);
                assert(h >= 0);
                key += h * mul;
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
                auto isHalf = attr < EGTB_IDX_R_FULL;
                int c = attr - (isHalf ? EGTB_IDX_R_HALF : EGTB_IDX_R_FULL) + ROOK;
                
                auto type = static_cast<PieceType>(c);
                assert(type >= PieceType::rook && type <= PieceType::pawn);
                
                auto d = egtbPieceListStartIdxByType[c];
                
                for(auto t = 0, n = type == PieceType::pawn ? 5 : 2; t < n; t++) {
                    auto pos = board.pieceList[sd][d + t];
                    if (pos < 0) {
                        continue;
                    }
                    assert(board.getPiece(pos).type == type);
                    
                    pos = Funcs::flip(pos, flipMode);
                    assert(pos >= 0 && pos < 90);
                    
                    if (type == PieceType::pawn) {
                        pos = isHalf ? halfBoard_PawnPosToIdx[pos] : pawnPosToIdx[pos];
                    } else {
                        if (isHalf) {
                            pos = egtbHalfBoard_PosToIdx[pos];
                        }
                    }
                    key += pos * mul;
                    break;
                }
                break;
            }
                
            case EGTB_IDX_RR_HALF:
            case EGTB_IDX_RR_FULL:
            case EGTB_IDX_CC_HALF:
            case EGTB_IDX_CC_FULL:
            case EGTB_IDX_NN_HALF:
            case EGTB_IDX_NN_FULL:
            case EGTB_IDX_PP_HALF:
            case EGTB_IDX_PP_FULL:
            {
                auto isHalf = attr < EGTB_IDX_RR_FULL;
                
                int c = attr - (isHalf ? EGTB_IDX_RR_HALF : EGTB_IDX_RR_FULL) + ROOK;
                
                auto type = static_cast<PieceType>(c);
                assert(type >= PieceType::rook && type <= PieceType::pawn);
                
                auto d = egtbPieceListStartIdxByType[c];
                
                auto pos0 = -1, pos1 = -1;
                
                for(auto t = 0, n = type == PieceType::pawn ? 5 : 2; t < n; t++) {
                    auto pos = board.pieceList[sd][d + t];
                    if (pos < 0) {
                        continue;
                    }
                    assert(board.getPiece(pos).type == type);
                    pos = Funcs::flip(pos, flipMode);
                    assert(pos >= 0 && pos < 90);
                    if (pos0 < 0) pos0 = pos;
                    else {
                        pos1 = pos;
                        break;
                    }
                }
                
                assert(pos0 >= 0 && pos1 >= 0);
                
                i64 h = 0;
                
                if (type == PieceType::pawn) {
                    h = isHalf ? egtbKey.getKey_pp(pos0, pos1)
                    : egtbKey.getKey_pp_full(pos0, pos1);
                } else {
                    h = isHalf ? EgtbKey::getKey_xx(pos0, pos1) : EgtbKey::getKey_xx_full(pos0, pos1);
                }
                key += h * mul;
                break;
            }
                
            case EGTB_IDX_PPP_HALF:
            case EGTB_IDX_PPP_FULL:
            {
                
                auto d = egtbPieceListStartIdxByType[PAWN];
                
                auto k = 0;
                int pawns[3] = { 0, 0, 0 };
                
                for(auto t = 0, n = 5; t < n; t++) {
                    auto pos = board.pieceList[sd][d + t];
                    if (pos < 0) {
                        continue;
                    }
                    assert(board.getPiece(pos).type == PieceType::pawn);
                    pos = Funcs::flip(pos, flipMode);
                    assert(pos >= 0 && pos < 90);
                    pawns[k++] = pos;
                    if (k >= 3) {
                        break;
                    }
                }
                assert(k == 3);
                
                auto h = attr == EGTB_IDX_PPP_HALF ? egtbKey.getKeyFlip_ppp(pawns[0], pawns[1], pawns[2])
                : egtbKey.getKeyFlip_ppp_full(pawns[0], pawns[1], pawns[2]);
                key += h * mul;
                
                break;
            }
                
            default:
                assert(false);
        } /// switch (attr)
        
    } /// for i
    
    assert(key >= 0);
    rec.key = key;
    
    return rec;
}


bool EgtbKey::setupBoard_x(XqBoard& board, int pos, PieceType type, Side side) const
{
    if (!board.isEmpty(pos)) {
        return false;
    }

    board.setPiece(pos, Piece(type, side));
    return true;
}

bool EgtbKey::setupBoard_defence(EgtbBoard& board, int attr, int idx, Side side, FlipMode flipMode)
{
    auto flip = flipMode;
    if (side == Side::white) {
        flip = Funcs::flip(flip, FlipMode::rotate);
    }

    const int* array;
    int sz;

    switch (attr) {
        case EGTB_IDX_DK:
            array = tbkey_k; sz = sizeof(tbkey_k)/sizeof(int);
            break;

        case EGTB_IDX_DA:
            array = tbkey_ka; sz = sizeof(tbkey_ka)/sizeof(int);
            break;
        case EGTB_IDX_DB:
            array = tbkey_kb; sz = sizeof(tbkey_kb)/sizeof(int);
            break;

        case EGTB_IDX_DAA:
            array = tbkey_kaa; sz = sizeof(tbkey_kaa)/sizeof(int);
            break;

        case EGTB_IDX_DBB:
            array = tbkey_kbb; sz = sizeof(tbkey_kbb)/sizeof(int);
            break;

        case EGTB_IDX_DAB:
            array = tbkey_kab; sz = sizeof(tbkey_kab)/sizeof(int);
            break;


        case EGTB_IDX_DAAB:
            array = tbkey_kaab; sz = sizeof(tbkey_kaab)/sizeof(int);
            break;

        case EGTB_IDX_DABB:
            array = tbkey_kabb; sz = sizeof(tbkey_kabb)/sizeof(int);
            break;
        case EGTB_IDX_DAABB:
            array = tbkey_kaabb; sz = sizeof(tbkey_kaabb)/sizeof(int);
            break;

        default:
            return false;
    }

    if (idx < 0 || idx >= (int)sz) {
        return false;
    }
    auto key = array[idx];

    int k = key & 0x3f;
    if (k <= 0) {
        return false;
    }


    int e1 = (key >> 6) & 0x3f;
    int e2 = (key >> 12) & 0x3f;

    // advisor
    int a1 = (key >> 18) & 0x3f;
    int a2 = (key >> 24) & 0x3f;

    /// Black Defenders may be pushed over a White piece
    k = Funcs::flip(k, flip);
    if (!setupBoard_x(board, k, PieceType::king, side)) {
        return false;
    }

    if (a1 > 0) {
        a1 = Funcs::flip(a1, flip);
        if (!setupBoard_x(board, a1, PieceType::advisor, side)) {
            return false;
        }

        if (a2 > 0) {
            a2 = Funcs::flip(a2, flip);
            if (!setupBoard_x(board, a2, PieceType::advisor, side)) {
                return false;
            }
        }
    }

    // elephant
    if (e1 > 0) {
        e1 = Funcs::flip(e1, flip);
        if (!setupBoard_x(board, e1, PieceType::elephant, side)) {
            return false;
        }

        if (e2 > 0) {
            e2 = Funcs::flip(e2, flip);
            if (!setupBoard_x(board, e2, PieceType::elephant, side)) {
                return false;
            }
        }
    }

    return true;
}

bool EgtbKey::setupBoard_oneStrongPiece(EgtbBoard& board, int attr, int key, bslib::Side side, FlipMode flip) {
    int pos = key;
    PieceType type;
    switch (attr) {
        case EGTB_IDX_R_HALF:
            pos = halfBoard_IdxToPos[key];
        case EGTB_IDX_R_FULL:
            type = PieceType::rook;
            break;

        case EGTB_IDX_C_HALF:
            pos = halfBoard_IdxToPos[key];
        case EGTB_IDX_C_FULL:
            type = PieceType::cannon;
            break;

        case EGTB_IDX_N_HALF:
            pos = halfBoard_IdxToPos[key];
        case EGTB_IDX_N_FULL:
            type = PieceType::horse;
            break;

        case EGTB_IDX_P_HALF:
            pos = halfBoard_PawnIdxToPos[key];
            type = PieceType::pawn;
            break;

        case EGTB_IDX_P_FULL:
            pos = pawnIdxToPos[key];
            type = PieceType::pawn;
            break;

        default:
            assert(false);
            return false;
    }

    pos = Funcs::flip(pos, flip);
    return setupBoard_x(board, pos, type, side);
}

bool EgtbKey::setupBoard_twoStrongPieces(EgtbBoard& board, int attr, int key, bslib::Side side, FlipMode flip)
{
    int pos0 = -1, pos1 = -1, pos2 = -1;
    PieceType type, second;
    switch (attr) {
            
        case EGTB_IDX_RR_HALF:
        case EGTB_IDX_RR_FULL:
            type = PieceType::rook;
            second = PieceType::rook;
            break;
            
        case EGTB_IDX_CC_HALF:
        case EGTB_IDX_CC_FULL:
            type = PieceType::cannon;
            second = PieceType::cannon;
            break;

        case EGTB_IDX_NN_HALF:
        case EGTB_IDX_NN_FULL:
            type = PieceType::horse;
            second = PieceType::horse;
            break;

        case EGTB_IDX_PP_HALF:
        case EGTB_IDX_PP_FULL:
        case EGTB_IDX_PPP_HALF:
        case EGTB_IDX_PPP_FULL:
            type = PieceType::pawn;
            second = PieceType::pawn;
            break;

        default:
            second = type = PieceType::empty;
            break;
    }

    switch (attr) {
        case EGTB_IDX_RR_HALF:
        case EGTB_IDX_CC_HALF:
        case EGTB_IDX_NN_HALF:
        {
            int x = tbkey_xx[key];
            pos0 = x >> 8;
            pos1 = x & 0xff;
            break;
        }

        case EGTB_IDX_RR_FULL:
        case EGTB_IDX_CC_FULL:
        case EGTB_IDX_NN_FULL:
        {
            assert(key >= 0 && key < EGTB_SIZE_XX);
            int x = tbkey_xx_full[key];
            pos0 = x >> 8;
            pos1 = x & 0xff;
            break;
        }
            
        case EGTB_IDX_PP_HALF:
        {
            int x = tbkey_pp[key];
            pos0 = x >> 8;
            pos1 = x & 0xff;
            break;
        }

        case EGTB_IDX_PP_FULL:
        {
            int x = tbkey_pp_full[key];
            pos0 = x >> 8;
            pos1 = x & 0xff;
            assert(pos0 < pos1 && pos0 >= 0 && pos0 < 90 && pos1 >= 0 && pos1 < 90);
            break;
        }
            
        case EGTB_IDX_PPP_HALF:
        {
            int ppp = pppKeyToPos[key];
            pos0 = ppp & 0xff;
            pos1 = (ppp >> 8) & 0xff;
            pos2 = (ppp >> 16) & 0xff;
            pos2 = Funcs::flip(pos2, flip);
            break;
        }

        default:
        {
            int idx0 = key / EGTB_SIZE_X;
            pos0 = halfBoard_IdxToPos[idx0];
            pos1 = key % EGTB_SIZE_X;
            
            if (pos0 == pos1) {
                return false;
            }
            break;
        }

    }

    pos0 = Funcs::flip(pos0, flip);
    pos1 = Funcs::flip(pos1, flip);
    
    if (!setupBoard_x(board, pos0, type, side)) {
        return false;
    }
    if (!setupBoard_x(board, pos1, second, side)) {
        return false;
    }

    return pos2 < 0 || setupBoard_x(board, pos2, PieceType::pawn, side);
}

#endif /// _FELICITY_XQ_
