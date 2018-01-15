
/*
 This file is part of Felicity Egtb, distributed under MIT license.

 Copyright (c) 2018 Nguyen Hong Pham

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "Egtb.h"
#include "EgtbKey.h"

namespace egtb {

    EgtbKey egtbKey;

    const int blackElephantPosWOMiddle[6] = { 2, 6, 18, 26, 38, 42 };
    const int blackElephantCombineWOMiddle[21] = {
        2 << 8, 2 << 8 | 6, 2 << 8 | 18, 2 << 8 | 26, 2 << 8 | 38, 2 << 8 | 42,
        6 << 8, 6 << 8 | 18, 6 << 8 | 26, 6 << 8 | 38, 6 << 8 | 42,
        18 << 8, 18 << 8 | 26, 18 << 8 | 38, 18 << 8 | 42,
        26 << 8, 26 << 8 | 38, 26 << 8 | 42,
        38 << 8, 38 << 8 | 42,
        42 << 8
    };
} // namespace


using namespace egtb;

extern const PieceType egtbPieceListIdxToType[16];

static const int a_pos[] = { 3, 5, 13, 21, 23 };
static const int e_pos[] = { 2, 6, 18, 22, 26, 38, 42 };

static const int tbkey_k[9] = { 3, 4, 5, 12, 13, 14, 21, 22, 23 };
static int tbkey_ka[40];
static int tbkey_ke[62];
static int tbkey_kaa[70];
static int tbkey_kae[275];
static int tbkey_kee[183];
static int tbkey_kaae[480];
static int tbkey_kaee[810];
static int tbkey_kaaee[1410];

static const int tbkey_km[17] = { 3, 4, 5, 12, 13, 14, 21, 22, 23, 3|22<<6, 4|22<<6, 5|22<<6, 12|22<<6, 13|22<<6, 14|22<<6, 21|22<<6, 23|22<<6 };
static int tbkey_kam[40 + 35];
static int tbkey_kaam[70 + 60];

static int tbkey_xx[2045];
static int tbkey_pp[EGTB_SIZE_PP_HALF];

const int egtbHalfBoard_PosToIdx[90] = {
    0, 1, 2, 3,  4,  3, 2, 1, 0,
    5, 6, 7, 8,  9,  8, 7, 6, 5,
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
    0, 1, 2, 3, 4,    // 5, 6, 7, 8,
    9,10,11,12,13,    //14,15,16,17,
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
    0, 1, 2, 3, 4,    // 5, 6, 7, 8,
    9,10,11,12,13,    //14,15,16,17,
    18,19,20,21,22,    //23,24,25,26,
    27,28,29,30,31,    //32,33,34,35,
    36,37,38,39,40,    //41,42,43,44,

    45,  47,    49,
    54,  56,    58,
    -1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,
};

static int bSearch(const int* array, int sz, int key) {
    int i = 0, j = sz - 1;

    while (i <= j) {
        int idx = (i + j) / 2;
        if (key == array[idx]) {
            return idx;
        }
        if (key < array[idx]) j = idx - 1;
        else i = idx + 1;
    }

    return -1;
}

static int pppSearchKey(int pos0, int pos1, int pos2) {
    int x0 = MIN(pos0, MIN(pos1, pos2));
    int x2 = MAX(pos0, MAX(pos1, pos2));
    int x1 = pos0 != x0 && pos0 != x2 ? pos0 : pos1 != x0 && pos1 != x2 ? pos1 : pos2;

    int ppp = x0 << 16 | x1 << 8 | x2;
    return ppp;
}

void EgtbKey::createPawnKeys() {
    int pppIdx = 0;

    int k = 0, previousppp = -1;
    for (int i = 0; ; i++) {
        auto p0 = halfBoard_PawnIdxToPos[i];
        if (p0 < 0) {
            break;
        }
        int x0 = EgtbBoard::flip(p0, FlipMode::horizontal);

        int r = getRow(p0), c0 = getCol(p0), f4 = 4 - c0;

        for (int p1 = p0 + 1; p1 <= 62; p1++) {
            if (pawnPosToIdx[p1] < 0) { // || (p0 > 44 && p1 - p0 == 9)) {
                continue;
            }
            auto c1 = getCol(p1), r1 = getRow(p1);
            if ((r == r1 && f4 < abs(c1 - 4)) || (f4 == 0 && c1 > 4)) {
                continue;
            }

            int x1 = EgtbBoard::flip(p1, FlipMode::horizontal);

            tbkey_pp[k] = p0 << 8 | p1;
            k++;

            if (p0 > 44 && p1 - p0 == 9) {
                continue;
            }

            for (int p2 = p1 + 1; p2 <= 62; p2++) {
                if (pawnPosToIdx[p2] < 0 || (p0 > 44 && p2 - p0 == 9) || (p1 > 44 && p2 - p1 == 9)) {
                    continue;
                }

                int x2 = EgtbBoard::flip(p2, FlipMode::horizontal);

                int ppp = p0 << 16 | p1 << 8 | p2;
                int yyy = pppSearchKey(x0, x1, x2);

                auto a = pppPos2KeyMap.find(ppp);
                if (a == pppPos2KeyMap. end()) {
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

            tbkey_xx[k] = p0 << 8 | p1;
            k++;
        }
    }
}

int EgtbKey::getKey_pp(int pos0, int pos1, EgtbType egtbType) const
{
    int r0 = getRow(pos0), r1 = getRow(pos1);

    if (r0 > r1 || (r0 == r1 && abs(getCol(pos0) - 4) < abs(getCol(pos1) - 4) )) {
        int x = pos0; pos0 = pos1; pos1 = x;
    }

    int c0 = getCol(pos0);
    if (c0 > 4 || (c0 == 4 && getCol(pos1) > 4)) {
        pos0 = EgtbBoard::flip(pos0, FlipMode::horizontal);
        pos1 = EgtbBoard::flip(pos1, FlipMode::horizontal);
    }

    int x = pos0 << 8 | pos1;
    int sz = sizeof(tbkey_pp) / sizeof(int);
    int key = bSearch(tbkey_pp, sz, x);
    return key;
}

int EgtbKey::getKeyFlip_ppp(int pos0, int pos1, int pos2, EgtbType egtbType) const
{
    int ppp = pppSearchKey(pos0, pos1, pos2);
    int key = pppPos2KeyMap.at(ppp);
    return key;
}


int EgtbKey::getKey_xx(int pos0, int pos1, EgtbType egtbType)
{
    int r0 = getRow(pos0), r1 = getRow(pos1);

    if (r0 > r1 || (r0 == r1 && abs(getCol(pos0) - 4) < abs(getCol(pos1) - 4) )) {
        int x = pos0; pos0 = pos1; pos1 = x;
    }

    int c0 = getCol(pos0);
    if (c0 > 4 || (c0 == 4 && getCol(pos1) > 4)) {
        pos0 = EgtbBoard::flip(pos0, FlipMode::horizontal);
        pos1 = EgtbBoard::flip(pos1, FlipMode::horizontal);
    }

    int x = pos0 << 8 | pos1;

    int sz = sizeof(tbkey_xx) / sizeof(int);
    int key = bSearch(tbkey_xx, sz, x);
    return key;
}

int EgtbKey::getKey_xy(int pos0, int pos1, EgtbType egtbType)
{
    int c0 = getCol(pos0);
    if (c0 > 4) {
        pos0 = EgtbBoard::flip(pos0, FlipMode::horizontal);
        pos1 = EgtbBoard::flip(pos1, FlipMode::horizontal);
    }

    int idx0 = egtbHalfBoard_PosToIdx[pos0];
    int key = idx0 * EGTB_SIZE_R + pos1;
    return key;
}

int EgtbKey::getKey_xp(int pos0, int pos1, EgtbType egtbType)
{
    int c0 = getCol(pos0);
    if (c0 > 4) {
        pos0 = EgtbBoard::flip(pos0, FlipMode::horizontal);
        pos1 = EgtbBoard::flip(pos1, FlipMode::horizontal);
    }

    int idx0 = egtbHalfBoard_PosToIdx[pos0];
    int idx1 = pawnPosToIdx[pos1];
    int key = idx0 * EGTB_SIZE_P + idx1;
    return key;
}

int EgtbKey::getKey_defence(int k, int a1, int a2, int e1, int e2, EgtbType egtbType)
{
    const int* array;
    int sz;
    int key = k;

    if (a1 > 0) {
        if (a2 > 0) {
            key |= a1 < a2 ? (a1 << 18 | a2 << 24) : (a2 << 18 | a1 << 24);
            if (egtbType == EgtbType::newdtm) {
                array = tbkey_kaam; sz = sizeof(tbkey_kaam)/sizeof(int);
            } else if (e1 > 0) {
                if (e2 > 0) {
                    array = tbkey_kaaee; sz = sizeof(tbkey_kaaee)/sizeof(int);
                } else {
                    array = tbkey_kaae; sz = sizeof(tbkey_kaae)/sizeof(int);
                }
            } else {
                array = tbkey_kaa; sz = sizeof(tbkey_kaa)/sizeof(int);
            }
        } else {
            key |= a1 << 18;

            if (egtbType == EgtbType::newdtm) {
                array = tbkey_kam; sz = sizeof(tbkey_kam)/sizeof(int);
            } else if (e1 > 0) {
                if (e2 > 0) {
                    array = tbkey_kaee; sz = sizeof(tbkey_kaee)/sizeof(int);
                } else {
                    array = tbkey_kae; sz = sizeof(tbkey_kae)/sizeof(int);
                }
            } else {
                array = tbkey_ka; sz = sizeof(tbkey_ka)/sizeof(int);
            }
        }
    } else if (egtbType == EgtbType::newdtm) {
        array = tbkey_km; sz = sizeof(tbkey_km)/sizeof(int);
    } else {
        if (e1 > 0) {
            if (e2 > 0) {
                array = tbkey_kee; sz = sizeof(tbkey_kee)/sizeof(int);
            } else {
                array = tbkey_ke; sz = sizeof(tbkey_ke)/sizeof(int);
            }
        } else {
            array = tbkey_k; sz = sizeof(tbkey_k)/sizeof(int);
        }
    }

    if (e1 > 0) {
        if (egtbType == EgtbType::newdtm) {
            if (e1 == 22 || e2 == 22) {
                key |= 22 << 6;
            }
        } else {
            if (e2 > 0) {
                key |= e1 < e2 ? (e1 << 6 | e2 << 12) : (e2 << 6 | e1 << 12);
            } else {
                key |= e1 << 6;
            }
        }
    }

    return bSearch(array, sz, key);
}

// create pieceList
bool EgtbKey::parseKey_defence(int attr, int idx, int* pieceList, int sd, FlipMode flip) {
    const int* array;
    int sz;

    switch (attr) {
        case EGTB_IDX_DK:
            array = tbkey_k; sz = sizeof(tbkey_k)/sizeof(int);
            break;

        case EGTB_IDX_DA:
            array = tbkey_ka; sz = sizeof(tbkey_ka)/sizeof(int);
            break;
        case EGTB_IDX_DE:
            array = tbkey_ke; sz = sizeof(tbkey_ke)/sizeof(int);
            break;

        case EGTB_IDX_DAA:
            array = tbkey_kaa; sz = sizeof(tbkey_kaa)/sizeof(int);
            break;

        case EGTB_IDX_DEE:
            array = tbkey_kee; sz = sizeof(tbkey_kee)/sizeof(int);
            break;

        case EGTB_IDX_DAE:
            array = tbkey_kae; sz = sizeof(tbkey_kae)/sizeof(int);
            break;


        case EGTB_IDX_DAAE:
            array = tbkey_kaae; sz = sizeof(tbkey_kaae)/sizeof(int);
            break;

        case EGTB_IDX_DAEE:
            array = tbkey_kaee; sz = sizeof(tbkey_kaee)/sizeof(int);
            break;
        case EGTB_IDX_DAAEE:
            array = tbkey_kaaee; sz = sizeof(tbkey_kaaee)/sizeof(int);
            break;

        case EGTB_IDX_DM:
            array = tbkey_km; sz = sizeof(tbkey_km)/sizeof(int);
            break;

        case EGTB_IDX_DAM:
            array = tbkey_kam; sz = sizeof(tbkey_kam)/sizeof(int);
            break;

        case EGTB_IDX_DAAM:
            array = tbkey_kaam; sz = sizeof(tbkey_kaam)/sizeof(int);
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

    int *p = sd == 0 ? pieceList : (pieceList + 16);

    auto x = egtbPieceListStartIdxByType[static_cast<int>(PieceType::king)];
    p[x] = EgtbBoard::flip(k, flip);

    if (a1 > 0) {
        auto x = egtbPieceListStartIdxByType[static_cast<int>(PieceType::advisor)];
        p[x] = EgtbBoard::flip(a1, flip);
        if (a2 > 0) {
            p[x + 1] = EgtbBoard::flip(a2, flip);
        }
    }

    // elephant
    if (e1 > 0) {
        auto x = egtbPieceListStartIdxByType[static_cast<int>(PieceType::elephant)];
        p[x] = EgtbBoard::flip(e1, flip);
        if (e2 > 0) {
            p[x + 1] = EgtbBoard::flip(e2, flip);
        }
    }

    return true;
}

bool EgtbKey::parseKey_oneStrongPiece(int key, int* pieceList, int sd, FlipMode flip, int attr) {
    int pos = key;
    PieceType type;
    switch (attr) {
        case EGTB_IDX_R:
            pos = halfBoard_IdxToPos[key];
            type = PieceType::rook;
            break;

        case EGTB_IDX_C:
            pos = halfBoard_IdxToPos[key];
            type = PieceType::cannon;
            break;

        case EGTB_IDX_H:
            pos = halfBoard_IdxToPos[key];
            type = PieceType::horse;
            break;

        case EGTB_IDX_P:
            pos = halfBoard_PawnIdxToPos[key];
            type = PieceType::pawn;
            break;

        case EGTB_IDX_R_Full:
            type = PieceType::rook;
            break;

        case EGTB_IDX_C_Full:
            type = PieceType::cannon;
            break;

        case EGTB_IDX_H_Full:
            type = PieceType::horse;
            break;

        case EGTB_IDX_P_Full:
            pos = pawnIdxToPos[key];
            type = PieceType::pawn;
            break;

        default:
            type = PieceType::empty;
            break;
    }

    int *p = sd == 0 ? pieceList : (pieceList + 16);
    p += egtbPieceListStartIdxByType[static_cast<int>(type)];

    *p = EgtbBoard::flip(pos, flip);
    return true;
}

bool EgtbKey::parseKey_twoStrongPieces(int key, int* pieceList, int sd, FlipMode flip, int attr) {
    int *pList = sd == 0 ? pieceList : (pieceList + 16);

    int pos0 = -1, pos1 = -1;
    PieceType type, second;
    switch (attr) {
        case EGTB_IDX_CC:
            type = PieceType::cannon;
            second = PieceType::cannon;
            break;
        case EGTB_IDX_CH:
            type = PieceType::cannon;
            second = PieceType::horse;
            break;
        case EGTB_IDX_CP:
            type = PieceType::cannon;
            second = PieceType::pawn;
            break;

        case EGTB_IDX_HH:
            type = PieceType::horse;
            second = PieceType::horse;
            break;
        case EGTB_IDX_HP:
            type = PieceType::horse;
            second = PieceType::pawn;
            break;

        case EGTB_IDX_PP:
        case EGTB_IDX_PPP:
            type = PieceType::pawn;
            second = PieceType::pawn;
            break;

        default:
            second = type = PieceType::empty;
            break;
    }

    int *p0 = pList + egtbPieceListStartIdxByType[static_cast<int>(type)];
    int *p1 = pList + egtbPieceListStartIdxByType[static_cast<int>(second)];
    if (p0 == p1) {
        p1++;
    }

    switch (attr) {
        case EGTB_IDX_CC:
        case EGTB_IDX_HH:
        {
            int x = tbkey_xx[key];
            pos0 = x >> 8;
            pos1 = x & 0xff;
            break;
        }

        case EGTB_IDX_PP:
        {
            int x = tbkey_pp[key];
            pos0 = x >> 8;
            pos1 = x & 0xff;
            break;
        }

        case EGTB_IDX_PPP:
        {
            int ppp = pppKeyToPos[key];
            pos0 = ppp & 0xff;
            pos1 = (ppp >> 8) & 0xff;
            int pos2 = (ppp >> 16) & 0xff;
            *(p1 + 1) = EgtbBoard::flip(pos2, flip);
            break;
        }

        default:
        {
            if (attr == EGTB_IDX_CP || attr == EGTB_IDX_HP) {
                int idx0 = key / EGTB_SIZE_P;
                int idx1 = key % EGTB_SIZE_P;
                pos0 = halfBoard_IdxToPos[idx0];
                pos1 = pawnIdxToPos[idx1];
            } else {
                int idx0 = key / EGTB_SIZE_R;
                pos0 = halfBoard_IdxToPos[idx0];
                pos1 = key % EGTB_SIZE_R;
            }

            if (pos0 == pos1) {
                return false;
            }
            break;
        }

    }

    *p0 = EgtbBoard::flip(pos0, flip);
    *p1 = EgtbBoard::flip(pos1, flip);
    return true;
}

static void sort_tbkeys(int* tbkeys, int sz) {
    std::qsort(tbkeys, sz, sizeof(int), [](const void* a, const void* b) {
        const int* x = static_cast<const int*>(a);
        const int* y = static_cast<const int*>(b);
        return (int)(*x - *y);
    });
}

static void createDefenderKeys() {
    int ka = 0, ke = 0, kaa = 0, kae = 0, kee = 0, kaee = 0, kaae = 0, kaaee = 0;
    int kam = 0, kaam = 0;
    for (int i = 0; i < 9; i++) {
        auto k = tbkey_k[i];

        // A, AE, AA
        for (int i0 = 0; i0 < 5; i0++) {

            auto a0 = a_pos[i0];
            if (a0 == k) {
                continue;
            }

            // A
            int t = k | a0 << 18;
            tbkey_ka[ka++] = t;
            tbkey_kam[kam++] = t;

            // AE
            for (int j0 = 0; j0 < 7; j0++) {
                auto e0 = e_pos[j0];
                if (e0 == k) {
                    continue;
                }

                t =  k | a0 << 18 | e0 << 6;
                tbkey_kae[kae++] = t;

                if (e0 == 22) {
                    tbkey_kam[kam++] = t;
                }

                // AEE
                for (int j1 = j0 + 1; j1 < 7; j1++) {
                    auto e1 = e_pos[j1];
                    if (e1 == k) {
                        continue;
                    }

                    tbkey_kaee[kaee++] = k | a0 << 18 | e0 << 6 | e1 << 12;
                }
            }

            // AA
            for (int i1 = i0 + 1; i1 < 5; i1++) {
                auto a1 = a_pos[i1];
                if (a1 == k) {
                    continue;
                }

                t = k | a0 << 18 | a1 << 24;
                tbkey_kaa[kaa++] = t;
                tbkey_kaam[kaam++] = t;

                // AAE, AAEE
                for (int j0 = 0; j0 < 7; j0++) {
                    auto e0 = e_pos[j0];
                    if (e0 == k) {
                        continue;
                    }

                    int t = k | a0 << 18 | a1 << 24 | e0 << 6;
                    tbkey_kaae[kaae++] = t;

                    if (e0 == 22) {
                        tbkey_kaam[kaam++] = t;
                    }

                    // AAEE
                    for (int j1 = j0 + 1; j1 < 7; j1++) {
                        auto e1 = e_pos[j1];
                        if (e1 == k) {
                            continue;
                        }

                        auto idx = k | a0 << 18 | a1 << 24 | e0 << 6 | e1 << 12;
                        tbkey_kaaee[kaaee++] = idx;
                    }
                }

            }
        }

        // E & EE
        for (int j0 = 0; j0 < 7; j0++) {
            auto e0 = e_pos[j0];
            if (e0 == k) {
                continue;
            }

            tbkey_ke[ke++] = k | e0 << 6;

            for (int j1 = j0 + 1; j1 < 7; j1++) {
                auto e1 = e_pos[j1];
                if (e1 == k) {
                    continue;
                }

                tbkey_kee[kee++] = k | e0 << 6 | e1 << 12;
            }
        }
    }

    sort_tbkeys(tbkey_ka, sizeof(tbkey_ka) / sizeof(int));
    sort_tbkeys(tbkey_ke, sizeof(tbkey_ke) / sizeof(int));
    sort_tbkeys(tbkey_kaa, sizeof(tbkey_kaa) / sizeof(int));
    sort_tbkeys(tbkey_kae, sizeof(tbkey_kae) / sizeof(int));
    sort_tbkeys(tbkey_kam, sizeof(tbkey_kam) / sizeof(int));

    sort_tbkeys(tbkey_kee, sizeof(tbkey_kee) / sizeof(int));
    sort_tbkeys(tbkey_kaee, sizeof(tbkey_kaee) / sizeof(int));
    sort_tbkeys(tbkey_kaae, sizeof(tbkey_kaae) / sizeof(int));
    sort_tbkeys(tbkey_kaaee, sizeof(tbkey_kaaee) / sizeof(int));
    sort_tbkeys(tbkey_kaam, sizeof(tbkey_kaam) / sizeof(int));
}

void EgtbKey::initOnce() {
    createDefenderKeys();
    createPawnKeys();
    createXXKeys();
}

EgtbKey::EgtbKey() {
    initOnce();
}

static std::pair<int, int> getPiecePosition(const int* pieceList, PieceType pieceType, Side& side) {
    std::pair<int, int> r;
    auto t = egtbPieceListStartIdxByType[static_cast<int>(pieceType)];
    int pos = -1;
    if (pieceType == PieceType::pawn) {
        if (side == Side::none) {
            side = Side::black;
            for (int i = 16; i < 16 + 5; i++) {
                if (pieceList[t + i] >= 0) {
                    side = Side::white;
                    break;
                }
            }
        }

        if (side == Side::white) t += 16;

        for (int i = 0; i < 5; i++) {
            if (pieceList[t + i] >= 0) {
                t += i;
                pos = pieceList[t];
                r.first = pos;
                r.second = t;
                break;
            }
        }
    } else {
        if (side == Side::none) {
            side = (pieceList[t] >= 0 || pieceList[t + 1] >= 0) ? Side::black : Side::white;
        }
        if (side == Side::white) t += 16;

        pos = pieceList[t] >= 0 ? pieceList[t] : pieceList[t + 1];
        r.first = pos;
        r.second = t;
    }
    return r;
}


static const PieceType strongEgtbPieces[][3] = {
    // EGTB_IDX_R, EGTB_IDX_C, EGTB_IDX_H, EGTB_IDX_P,
    {PieceType::rook, PieceType::empty, PieceType::empty},
    {PieceType::cannon, PieceType::empty, PieceType::empty},
    {PieceType::horse, PieceType::empty, PieceType::empty},
    {PieceType::pawn, PieceType::empty, PieceType::empty},

    // EGTB_IDX_R_Full, EGTB_IDX_C_Full, EGTB_IDX_H_Full, EGTB_IDX_P_Full,
    {PieceType::rook, PieceType::empty, PieceType::empty},
    {PieceType::cannon, PieceType::empty, PieceType::empty},
    {PieceType::horse, PieceType::empty, PieceType::empty},
    {PieceType::pawn, PieceType::empty, PieceType::empty},

    // EGTB_IDX_CC,
    {PieceType::cannon, PieceType::cannon, PieceType::empty},
    // EGTB_IDX_CH,
    {PieceType::cannon, PieceType::horse, PieceType::empty},
    // EGTB_IDX_CP
    {PieceType::cannon, PieceType::pawn, PieceType::empty},

    // EGTB_IDX_HH
    {PieceType::horse, PieceType::horse, PieceType::empty},
    // EGTB_IDX_HP
    {PieceType::horse, PieceType::pawn, PieceType::empty},

    // EGTB_IDX_PP,
    {PieceType::pawn, PieceType::pawn, PieceType::empty},

    //EGTB_IDX_CPP
    {PieceType::cannon, PieceType::pawn, PieceType::pawn},
    //EGTB_IDX_HPP
    {PieceType::horse, PieceType::pawn, PieceType::pawn},
    //EGTB_IDX_PPP
    {PieceType::pawn, PieceType::pawn, PieceType::pawn},
};


void EgtbKey::getKey(EgtbKeyRec& rec, const int* pieceList, EgtbType egtbType, bool forLookupKey, const int* idxArr, const i64* idxMult, u16 order) {
    PieceType piece0;
    PieceType piece1;
    PieceType piece2;

    int o0 = order ? order & 0x7 : 0;
    int o1 = order ? order >> 3 & 0x7 : 1;
    int o2 = order ? order >> 6 & 0x7 : 2;

    int strongIdxArr1 = idxArr[o1];
    if (strongIdxArr1 >= EGTB_IDX_R && strongIdxArr1 <= EGTB_IDX_PPP) {
        auto k = strongIdxArr1 - EGTB_IDX_R;
        piece0 = strongEgtbPieces[k][0];
        piece1 = strongEgtbPieces[k][1];
        piece2 = strongEgtbPieces[k][2];
    } else {
        rec.key = -1L;
        return;
    }

    int strongIdxArr2 = idxArr[o2];
    if (strongIdxArr2 >= EGTB_IDX_R && strongIdxArr2 <= EGTB_IDX_PPP) {
        auto k = strongIdxArr2 - EGTB_IDX_R;
        piece1 = strongEgtbPieces[k][0];
    }

    int pos1 = -1, pos2 = -1;

    auto strongSide = Side::none;
    auto r = getPiecePosition(pieceList, piece0, strongSide);

    auto pos0 = r.first;

    if (piece0 == PieceType::pawn) {
        auto t = r.second + 1;
        if (piece1 == PieceType::pawn) {
            for (int i = 0; i < 5; i++, t++) {
                if (pieceList[t] >= 0) {
                    pos1 = pieceList[t];
                    t++;
                    break;
                }
            }
        }
        if (piece2 == PieceType::pawn) {
            for (int i = 0; i < 5; i++, t++) {
                if (pieceList[t] >= 0) {
                    pos2 = pieceList[t];
                    t++;
                    break;
                }
            }
        }
    } else {
        if (piece1 != PieceType::empty) {
            if (piece0 == piece1) {
                auto t = r.second + 1;
                pos1 = pieceList[t];
            } else {
                auto r = getPiecePosition(pieceList, piece1, strongSide);
                pos1 = r.first;

            }
        }
    }

    i64 key = 0;
    int strongSd = static_cast<int>(strongSide);
    FlipMode flip, flipKB, flipK;

    if (strongSide == Side::white) {
        flip = FlipMode::none; flipKB = FlipMode::none; flipK = FlipMode::vertical;
    } else {
        flip = FlipMode::vertical; flipK = FlipMode::none; flipKB = FlipMode::vertical;
    }

    bool needFlipHorizontal = false;
    if (strongIdxArr1 == EGTB_IDX_PP || strongIdxArr1 == EGTB_IDX_CC || strongIdxArr1 == EGTB_IDX_HH) {
        auto p0 = EgtbBoard::flip(pos0, flip);
        int p1 = -1, p2 = -1;
        if (pos1 >= 0) {
            p1 = EgtbBoard::flip(pos1, flip);
        }
        if (pos2 >= 0) {
            p2 = EgtbBoard::flip(pos2, flip);
        }

        int r0 = getRow(p0), r1 = getRow(p1);

        if (r0 > r1 || (r0 == r1 && abs(getCol(p0) - 4) < abs(getCol(p1) - 4) )) {
            int x = p0; p0 = p1; p1 = x;
        }

        int c0 = getCol(p0);
        if (c0 > 4 || (c0 == 4 && getCol(p1) > 4)) {
            needFlipHorizontal = true;
        }
    } else if (strongIdxArr1 == EGTB_IDX_PPP) {
        int flip_key = egtbKey.getKeyFlip_ppp(pos0, pos1, pos2, egtbType);
        auto flip = static_cast<FlipMode>(flip_key >> 24);
        if (flip != FlipMode::none) {
            pos0 = EgtbBoard::flip(pos0, FlipMode::horizontal);
            pos1 = EgtbBoard::flip(pos1, FlipMode::horizontal);
            pos2 = EgtbBoard::flip(pos2, FlipMode::horizontal);

            needFlipHorizontal = true;
        }
    } else if ((pos0 % 9)>4) {
        needFlipHorizontal = true;
    }

    if (needFlipHorizontal) {
        flip = EgtbBoard::flip(flip, FlipMode::horizontal);
        flipK = EgtbBoard::flip(flipK, FlipMode::horizontal);
        flipKB = EgtbBoard::flip(flipKB, FlipMode::horizontal);
    }

    pos0 = EgtbBoard::flip(pos0, flip);

    if (pos1 >= 0) {
        pos1 = EgtbBoard::flip(pos1, flip);
    }
    if (pos2 >= 0) {
        pos2 = EgtbBoard::flip(pos2, flip);
    }

    for(int i = 0; ; i++) {
        auto attr = idxArr[i];
        if (attr == EGTB_IDX_NONE) {
            break;
        }
        auto mul = idxMult[i];

        switch (attr) {
            case EGTB_IDX_DM:
            case EGTB_IDX_DAM:
            case EGTB_IDX_DAAM:

            case EGTB_IDX_DK:
            case EGTB_IDX_DA:
            case EGTB_IDX_DE:
            case EGTB_IDX_DAA:
            case EGTB_IDX_DEE:
            case EGTB_IDX_DAE:
            case EGTB_IDX_DAAE:
            case EGTB_IDX_DAEE:
            case EGTB_IDX_DAAEE:
            {
                EgtbType _egtbType = attr == EGTB_IDX_DM || attr == EGTB_IDX_DAM || attr == EGTB_IDX_DAAM ? EgtbType::newdtm : EgtbType::dtm;

                auto kFlip = flipK;
                int wsd = strongSd;
                if (_egtbType != EgtbType::newdtm && i != o0) {
                    wsd = 1 - wsd; kFlip = flipKB;
                }

                int d = wsd == B ? 0 : 16;
                auto k = EgtbBoard::flip(pieceList[d + 0], kFlip);
                auto t = egtbPieceListStartIdxByType[static_cast<int>(PieceType::advisor)];
                auto a1 = pieceList[d + t];
                auto a2 = pieceList[d + t + 1];
                t = egtbPieceListStartIdxByType[static_cast<int>(PieceType::elephant)];
                auto e1 = pieceList[d + t];
                auto e2 = pieceList[d + t + 1];

                if (a1 > 0) {
                    a1 = EgtbBoard::flip(a1, kFlip);
                }
                if (a2 > 0) {
                    a2 = EgtbBoard::flip(a2, kFlip);
                    if (a1 < 0) {
                        a1 = a2; a2 = -1;
                    }
                }
                if (e1 > 0) {
                    e1 = EgtbBoard::flip(e1, kFlip);
                }
                if (e2 > 0) {
                    e2 = EgtbBoard::flip(e2, kFlip);
                    if (e1 < 0) {
                        e1 = e2; e2 = -1;
                    }
                }

                auto ee1 = e1;
                auto ee2 = e2;

                if (forLookupKey && _egtbType == EgtbType::newdtm) {
                    if (ee2 != 22) {
                        ee2 = -1;
                    }
                    if (ee1 != 22) {
                        ee1 = -1;
                        ee1 = ee2; ee2 = -1;
                    }

                    rec.groupIdx = e1 == 22 || e2 == 22 ? 0 : 1;
                    rec.subKey = 0;

                    if (rec.groupIdx == 0) {
                        int e = e1 > 0 && e1 != 22 ? e1 : e2;

                        if (e > 0) {
                            for (int i = 0; i < sizeof(blackElephantPosWOMiddle) / sizeof(int); i++) {
                                if (blackElephantPosWOMiddle[i] == e) {
                                    rec.subKey = i + 1;
                                    break;
                                }
                            }
                        }
                    } else if (e1 > 0 || e2 > 0) {
                        if (e1 < 0) e1 = 0;
                        if (e2 < 0) e2 = 0;
                        if (e2 < e1 && e2 != 0) {
                            int x = e2; e2 = e1; e1 = x;
                        }
                        int ee = e1 << 8 | e2;
                        for(int i = 0; i < sizeof(blackElephantCombineWOMiddle) / sizeof(int); i++) { // 21 items
                            if (blackElephantCombineWOMiddle[i] == ee) {
                                rec.subKey = i + 1;
                                break;
                            }
                        }
                    }
                }

                auto z = EgtbKey::getKey_defence(k, a1, a2, ee1, ee2, _egtbType) * mul;
                key += z;
                break;
            }

            case EGTB_IDX_R:
            case EGTB_IDX_C:
            case EGTB_IDX_H:
            {
                pos0 = egtbHalfBoard_PosToIdx[pos0];
                key += pos0 * mul;
                break;
            }

            case EGTB_IDX_P:
            {
                pos0 = halfBoard_PawnPosToIdx[pos0];
                key += pos0 * mul;
                break;
            }


            case EGTB_IDX_R_Full:
            case EGTB_IDX_C_Full:
            case EGTB_IDX_H_Full:
            {
                key += pos1 * mul;
                break;
            }

            case EGTB_IDX_P_Full:
            {
                pos1 = pawnPosToIdx[pos1];
                key += pos1 * mul;
                break;
            }


            case EGTB_IDX_CC:
            case EGTB_IDX_HH:
            {
                key += EgtbKey::getKey_xx(pos0, pos1, egtbType) * mul;
                break;
            }

            case EGTB_IDX_PP:
            {
                key += egtbKey.getKey_pp(pos0, pos1, egtbType) * mul;
                break;
            }

            case EGTB_IDX_PPP:
            {
                key += (egtbKey.getKeyFlip_ppp(pos0, pos1, pos2, egtbType) & 0xffffff) * mul;
                break;
            }

            case EGTB_IDX_CH:
            {
                key += EgtbKey::getKey_xy(pos0, pos1, egtbType) * mul;
                break;
            }

            case EGTB_IDX_CP:
            case EGTB_IDX_HP:
            {
                key += EgtbKey::getKey_xp(pos0, pos1, egtbType) * mul;
                break;
            }

            default:
                break;
        }
    }

    rec.key = key;
    rec.flipSide = strongSide != Side::white;
}
