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

#include "egtb.h"
#include "egtbkey.h"
#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;


#ifdef _FELICITY_CHESS_

static const int exchangePieceValue[8] = { -1, 10000, 1100, 500, 300, 250, 100, 0 };

extern const int tb_kIdxToPos[10];
extern int *kk_2, *kk_8;

/// Flip board into 1/8
static const int tb_flipMode[64] = {
    0, 0, 0, 0, 1, 1, 1, 1,
    3, 0, 0, 0, 1, 1, 1, 7,
    3, 3, 0, 0, 1, 1, 7, 7,
    3, 3, 3, 0, 1, 7, 7, 7,
    5, 5, 5, 2, 6, 4, 4, 4,
    5, 5, 2, 2, 6, 6, 4, 4,
    5, 2, 2, 2, 6, 6, 6, 4,
    2, 2, 2, 2, 6, 6, 6, 6
};

/// Convert King position (1/8) into index 0-9
static const int tb_kIdx[64] = {
    0, 1, 2, 3, -1,-1,-1,-1,
    -1, 4, 5, 6, -1,-1,-1,-1,
    -1,-1, 7, 8, -1,-1,-1,-1,
    -1,-1,-1, 9, -1,-1,-1,-1,

    -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1
};

static int* tb_xx, *tb_xxx, *tb_xxxx;
static int* tb_pp, *tb_ppp, *tb_pppp;
int *kk_2, *kk_8;

const int tb_kIdxToPos[10] = {
    0, 1, 2, 3, 9, 10, 11, 18, 19, 27
};


void EgtbKey::initOnce() {
    createKingKeys();
    createXXKeys();
}


void EgtbKey::createKingKeys() {
    kk_8 = new int[EGTB_SIZE_KK8];
    auto x = 0;

    for(auto i = 0; i < sizeof(tb_kIdxToPos) / sizeof(int); i++) {
        auto k0 = tb_kIdxToPos[i];
        auto r0 = ROW(k0), f0 = COL(k0);
        for(auto k1 = 0; k1 < 64; k1++) {
            if (k0 == k1 || (abs(ROW(k1) - r0) <= 1 && abs(COL(k1) - f0) <= 1)) {
                continue;
            }

            kk_8[x++] = k0 << 8 | k1;
        }
    }

    kk_2 = new int[EGTB_SIZE_KK2];
    x = 0;

    for(auto k0 = 0; k0 < 64; k0++) {
        auto f0 = COL(k0);
        if (f0 > 3) {
            continue;
        }
        auto r0 = ROW(k0);

        for(auto k1 = 0; k1 < 64; k1++) {
            if (k0 == k1 || (abs(ROW(k1) - r0) <= 1 && abs(COL(k1) - f0) <= 1)) {
                continue;
            }

            kk_2[x++] = k0 << 8 | k1;
        }
    }
}

void EgtbKey::createXXKeys() {
    tb_xx = new int[EGTB_SIZE_XX];
    tb_xxx = new int[EGTB_SIZE_XXX];
    tb_xxxx = new int[EGTB_SIZE_XXXX];

    auto k0 = 0, k1 = 0, k2 = 0;

    for(auto i0 = 0; i0 < 64; i0++) {
        for(auto i1 = i0 + 1; i1 < 64; i1++) {
            tb_xx[k0++] = i0 << 8 | i1;
            for(auto i2 = i1 + 1; i2 < 64; i2++) {
                tb_xxx[k1++] = i0 << 16 | i1 << 8 | i2;
                for(auto i3 = i2 + 1; i3 < 64; i3++) {
                    tb_xxxx[k2++] = i0 << 24 | i1 << 16 | i2 << 8 | i3;
                }
            }
        }
    }

    /// Pawns
    tb_pp = new int[EGTB_SIZE_PP];
    tb_ppp = new int[EGTB_SIZE_PPP];
    tb_pppp = new int[EGTB_SIZE_PPPP];

    k0 = k1 = k2 = 0;

    for(auto i0 = 8; i0 < 56; i0++) {
        for(auto i1 = i0 + 1; i1 < 56; i1++) {
            tb_pp[k0++] = i0 << 8 | i1;
            for(auto i2 = i1 + 1; i2 < 56; i2++) {
                tb_ppp[k1++] = i0 << 16 | i1 << 8 | i2;
                for(auto i3 = i2 + 1; i3 < 56; i3++) {
                    tb_pppp[k2++] = i0 << 24 | i1 << 16 | i2 << 8 | i3;
                }
            }
        }
    }
}

int EgtbKey::getKey_x(int pos0)
{
    return pos0;
}

int EgtbKey::getKey_xx(int pos0, int pos1)
{
    auto p0 = std::min(pos0, pos1);
    auto p1 = std::max(pos0, pos1);

    auto x = p0 << 8 | p1;

    return Funcs::bSearch(tb_xx, EGTB_SIZE_XX, x);
}

int EgtbKey::getKey_xxx(int pos0, int pos1, int pos2)
{
    int p[3] = { pos0, pos1, pos2 };
    Funcs::sort_tbkeys(p, 3);

    auto x = p[0] << 16 | p[1] << 8 | p[2];
    return Funcs::bSearch(tb_xxx, EGTB_SIZE_XXX, x);
}

int EgtbKey::getKey_xxxx(int pos0, int pos1, int pos2, int pos3)
{
    int p[4] = { pos0, pos1, pos2, pos3 };
    Funcs::sort_tbkeys(p, 4);

    auto x = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
    return Funcs::bSearch(tb_xxxx, EGTB_SIZE_XXXX, x);
}


int EgtbKey::getKey_p(int pos0)
{
    assert(pos0 >= 8 && pos0 < 56);
    return pos0 - 8;
}

int EgtbKey::getKey_pp(int pos0, int pos1)
{
    auto p0 = std::min(pos0, pos1);
    auto p1 = std::max(pos0, pos1);

    auto x = p0 << 8 | p1;
    return Funcs::bSearch(tb_pp, EGTB_SIZE_PP, x);
}

int EgtbKey::getKey_ppp(int pos0, int pos1, int pos2)
{
    int p[3] = { pos0, pos1, pos2 };
    Funcs::sort_tbkeys(p, 3);
    auto x = p[0] << 16 | p[1] << 8 | p[2];
    return Funcs::bSearch(tb_ppp, EGTB_SIZE_PPP, x);
}

int EgtbKey::getKey_pppp(int pos0, int pos1, int pos2, int pos3)
{
    int p[4] = { pos0, pos1, pos2, pos3 };
    Funcs::sort_tbkeys(p, 4);

    auto x = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
    return Funcs::bSearch(tb_pppp, EGTB_SIZE_PPPP, x);
}

bool EgtbKey::setupBoard_x(BoardCore& board, int key, PieceType type, Side side) const
{
    if (type == PieceType::pawn) {
        key += 8;
    }

    if (!board.isEmpty(key)) {
        return false;
    }

    auto piece = Piece(type, side);
    board.setPiece(key, piece);
    return true;
}

bool EgtbKey::setupBoard_xx(BoardCore& board, int key, PieceType type, Side side) const
{
    int xx;

    if (type != PieceType::pawn) {
        xx = tb_xx[key];
    } else {
        assert(key < EGTB_SIZE_PP);
        xx = tb_pp[key];
    }

    auto pos0 = xx >> 8, pos1 = xx & 0xff;
    
    if (!board.isEmpty(pos0) || !board.isEmpty(pos1)) {
        return false;
    }

    auto piece = Piece(type, side);
    board.setPiece(pos0, piece);
    board.setPiece(pos1, piece);
    return true;
    return false;
}

bool EgtbKey::setupBoard_xxx(BoardCore& board, int key, PieceType type, Side side) const
{
    int xx;
    if (type != PieceType::pawn) {
        xx = tb_xxx[key];
    } else {
        assert(key >= 0 && key < EGTB_SIZE_PPP);
        xx = tb_ppp[key];
    }
    auto pos0 = xx >> 16, pos1 = (xx >> 8) & 0xff, pos2 = xx & 0xff;

    if (!board.isEmpty(pos0) || !board.isEmpty(pos1) || !board.isEmpty(pos2)) {
        return false;
    }

    auto piece = Piece(type, side);
    board.setPiece(pos0, piece);
    board.setPiece(pos1, piece);
    board.setPiece(pos2, piece);
    return true;
}

bool EgtbKey::setupBoard_xxxx(BoardCore& board, int key, PieceType type, Side side) const
{
    int xx;
    if (type != PieceType::pawn) {
        xx = tb_xxxx[key];
    } else {
        assert(key >= 0 && key < EGTB_SIZE_PPPP);
        xx = tb_pppp[key];
    }

    auto pos0 = xx >> 24, pos1 = (xx >> 16) & 0xff, pos2 = (xx >> 8) & 0xff, pos3 = xx & 0xff;

    if (!board.isEmpty(pos0) || !board.isEmpty(pos1) || !board.isEmpty(pos2) || !board.isEmpty(pos3)) {
        return false;
    }

    auto piece = Piece(type, side);
    board.setPiece(pos0, piece);
    board.setPiece(pos1, piece);
    board.setPiece(pos2, piece);
    board.setPiece(pos3, piece);
    return true;
}

EgtbKeyRec EgtbKey::getKey(const EgtbBoard& board, const EgtbIdxRecord* egtbIdxRecord, u32 order)
{
    EgtbKeyRec rec;
    
    auto sd = W;

    /// Check which side for left hand side (stronger side)
    {
        int pieceCnt[2][10];
        memset(pieceCnt, 0, sizeof(pieceCnt));
        
        for (auto sd = 0; sd < 2; sd++) {
            for(auto i = 1; i < 16; i++) {
                auto pos = board.pieceList[sd][i];
                if (pos >= 0) {
                    auto piece = board.getPiece(pos);
                    pieceCnt[sd][static_cast<int>(piece.type)]++;
                }
            }
        }
        
        for(auto i = FirstAttacker; i <= PAWN; i++) {
            if (pieceCnt[0][i] != pieceCnt[1][i]) {
                if (pieceCnt[W][i] < pieceCnt[B][i]) {
                    sd = B;
                }
                break;
            }
        }
    }

    rec.flipSide = sd == B;
    auto xsd = 1 - sd;
    auto flipMode = FlipMode::none;

    auto orderVec = EgtbFile::order2Vec(order);


    /// Calculate key
    i64 key = 0;

    for(auto i = 0; ; i++) {
        auto x = orderVec[i];
        auto attr = egtbIdxRecord[x].idx;
        if (attr == EGTB_IDX_NONE) {
            break;
        }
        auto mul = egtbIdxRecord[x].mult; assert(mul > 0);
        auto side = egtbIdxRecord[x].side; assert(side == Side::white || side == Side::black);
        if (rec.flipSide) {
            side = getXSide(side);
        }
        const auto sd = static_cast<int>(side);

        switch (attr) {
            case EGTB_IDX_KK_2:
            {
                assert(flipMode == FlipMode::none);
                auto pos0 = board.pieceList[sd][0];
                auto pos1 = board.pieceList[xsd][0];

                if (COL(pos0) > 3) {
                    flipMode = FlipMode::horizontal;
                    pos0 = board.flip(pos0, flipMode);
                    pos1 = board.flip(pos1, flipMode);
                }

                auto kk = pos0 << 8 | pos1;
                auto idx = Funcs::bSearch(kk_2, EGTB_SIZE_KK2, kk);
                assert(idx >= 0 && idx < EGTB_SIZE_KK2);

                key += idx * mul;
                break;
            }

            case EGTB_IDX_KK_8:
            {
                assert(flipMode == FlipMode::none);
                auto pos0 = board.pieceList[sd][0];
                auto pos1 = board.pieceList[xsd][0];

                auto flip = tb_flipMode[pos0];

                if (flip) {
                    flipMode = static_cast<FlipMode>(flip);
                    pos0 = Funcs::flip(pos0, flipMode);
                    pos1 = Funcs::flip(pos1, flipMode);
                }

                assert(pos0 >= 0 && pos0 <= tb_kIdxToPos[9]);
                auto kk = pos0 << 8 | pos1;
                auto idx = Funcs::bSearch(kk_8, EGTB_SIZE_KK8, kk);
                assert(idx >= 0);
                key += idx * mul;
                assert(key >= 0);
                break;
            }

            case EGTB_IDX_Q:
            case EGTB_IDX_R:
            case EGTB_IDX_B:
            case EGTB_IDX_N:
            case EGTB_IDX_P:
            {
                auto type = static_cast<PieceType>(attr - EGTB_IDX_Q + QUEEN);
                for(auto t = 1; t < 16; t++) {
                    auto p = board.pieceList[sd][t];
                    if (p < 0) {
                        continue;
                    }
                    auto piece = board.getPiece(p);
                    if (piece.type == type) {
                        auto pos = board.flip(p, flipMode);
                        assert(pos >= 0 && pos < 64);

                        if (attr == EGTB_IDX_P) {
                            pos = EgtbKey::getKey_p(pos);
                        } else {
                            pos = EgtbKey::getKey_x(pos);
                        }
                        key += pos * mul;
                        assert(key >= 0);
                        break;
                    }
                }
                break;
            }

            case EGTB_IDX_QQ:
            case EGTB_IDX_RR:
            case EGTB_IDX_BB:
            case EGTB_IDX_NN:
            case EGTB_IDX_PP:
            {
                auto type = static_cast<PieceType>(attr - EGTB_IDX_QQ + QUEEN);
                
                std::vector<int> idxVec;
                for(auto t = 1; t < 16; t++) {
                    auto pos = board.pieceList[sd][t];
                    if (pos >= 0 && board.getPiece(pos).type == type) {
                        auto idx = board.flip(pos, flipMode);
                        idxVec.push_back(idx);
                        if (idxVec.size() == 2) {
                            auto subKey = type != PieceType::pawn ? EgtbKey::getKey_xx(idxVec[0], idxVec[1]) : EgtbKey::getKey_pp(idxVec[0], idxVec[1]);
                            key += subKey * mul;
                            assert(key >= 0);
                            break;
                        }
                    }
                }
                break;
            }

            case EGTB_IDX_QQQ:
            case EGTB_IDX_RRR:
            case EGTB_IDX_BBB:
            case EGTB_IDX_HHH:
            case EGTB_IDX_PPP:
            {
                auto type = static_cast<PieceType>(attr - EGTB_IDX_QQQ + QUEEN);

                std::vector<int> idxVec;
                for(auto t = 1; t < 16; t++) {
                    auto pos = board.pieceList[sd][t];
                    if (pos >= 0 && board.getPiece(pos).type == type) {
                        auto idx = board.flip(pos, flipMode);
                        idxVec.push_back(idx);
                        if (idxVec.size() == 3) {
                            auto subKey = type != PieceType::pawn ? EgtbKey::getKey_xxx(idxVec[0], idxVec[1], idxVec[2]) : EgtbKey::getKey_ppp(idxVec[0], idxVec[1], idxVec[2]);
                            key += subKey * mul;
                            assert(key >= 0);
                            break;
                        }
                    }
                }
                break;
            }

            case EGTB_IDX_QQQQ:
            case EGTB_IDX_RRRR:
            case EGTB_IDX_BBBB:
            case EGTB_IDX_HHHH:
            case EGTB_IDX_PPPP:
            {
                auto type = static_cast<PieceType>(attr - EGTB_IDX_QQQQ + QUEEN);

                std::vector<int> idxVec;
                for(auto t = 1; t < 16; t++) {
                    auto pos = board.pieceList[sd][t];
                    if (pos >= 0 && board.getPiece(pos).type == type) {
                        auto idx = board.flip(pos, flipMode);
                        idxVec.push_back(idx);
                        if (idxVec.size() == 4) {
                            auto subKey = type != PieceType::pawn ? EgtbKey::getKey_xxxx(idxVec[0], idxVec[1], idxVec[2], idxVec[3]) : EgtbKey::getKey_pppp(idxVec[0], idxVec[1], idxVec[2], idxVec[3]);
                            key += subKey * mul;
                            assert(key >= 0);
                            break;
                        }
                    }
                }
                break;
            }
                
            default:
                assert(false);
                break;
        }
        assert(key >= 0);
    }

    assert(key >= 0);
    rec.key = key;
    
    return rec;
}



#endif /// _FELICITY_CHESS_
