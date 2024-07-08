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

#ifndef fegtb_key_h
#define fegtb_key_h

#include <map>

#include "egtb.h"

namespace fegtb {

    class EgtbKeyRec {
    public:
        i64 key;
        bool flipSide;
    };

#ifdef _FELICITY_CHESS_
    class EgtbKey {
    public:
        EgtbKey();

        static EgtbKeyRec getKey(const EgtbBoard& board, const EgtbIdxRecord* egtbIdxRecord, u32 order);

        bool setupBoard_x(bslib::BoardCore& board, int pos, bslib::PieceType type, bslib::Side side) const;
        bool setupBoard_xx(bslib::BoardCore& board, int key, bslib::PieceType type, bslib::Side side) const;
        
        bool setupBoard_xxx(bslib::BoardCore& board, int key, bslib::PieceType type, bslib::Side side) const;
        bool setupBoard_xxxx(bslib::BoardCore& board, int key, bslib::PieceType type, bslib::Side side) const;

    private:
        static int getKey_x(int pos0);
        static int getKey_xx(int p0, int p1);
        static int getKey_xxx(int p0, int p1, int p2);
        static int getKey_xxxx(int p0, int p1, int p2, int p3);

        static int getKey_p(int p0);
        static int getKey_pp(int p0, int p1);
        static int getKey_ppp(int p0, int p1, int p2);
        static int getKey_pppp(int p0, int p1, int p2, int p3);

        
        void initOnce();

        void createXXKeys();
        void createKingKeys();
    };

    extern int *tb_kk_2, *tb_kk_8;

#endif // _FELICITY_CHESS_

#ifdef _FELICITY_XQ_
class EgtbKey {
public:
    EgtbKey();

    static EgtbKeyRec getKey(const EgtbBoard& board, const EgtbIdxRecord* egtbIdxRecord, u32 order);

    static int getKey_defence(int k, int a1, int a2, int e1, int e2, bslib::FlipMode flipMode);

    bool setupBoard_twoStrongPieces(EgtbBoard& board, int attr, int key, bslib::Side side, bslib::FlipMode flip);
    bool setupBoard_oneStrongPiece(EgtbBoard& board, int attr, int key, bslib::Side side, bslib::FlipMode flip);
    bool setupBoard_defence(EgtbBoard& board, int attr, int idx, bslib::Side side, bslib::FlipMode flip);

    bool setupBoard_x(bslib::XqBoard& board, int key, bslib::PieceType type, bslib::Side side) const;

private:
    int getKey_pp(int p0, int p1) const;
    int getKey_pp_full(int p0, int p1) const;
    int getKeyFlip_ppp(int pos0, int pos1, int pos2) const;
    int getKeyFlip_ppp_full(int pos0, int pos1, int pos2) const;

    static int getKey_xx(int p0, int p1);
    static int getKey_xx_full(int p0, int p1);

    void initOnce();
    void createPawnKeys();

private:
    std::map<int, int> pppPos2KeyMap;
    int pppKeyToPos[EGTB_SIZE_PPP_HALF];

};

#endif

    extern EgtbKey egtbKey;

} /// namespace fegtb

#endif

