/*
 This file is part of NhatMinh Egtb, distributed under MIT license.

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

#ifndef EgtbKey_h
#define EgtbKey_h

#include <map>

#include "Egtb.h"

namespace egtb {

    class EgtbKeyRec {
    public:
        i64 key;
        bool flipSide;
    };

    class EgtbKey {
    public:
        EgtbKey();

        static void getKey(EgtbKeyRec& rec, const bslib::BoardCore& board, const int* idxArr, const i64* idxMult, u32 order);

        bool setupBoard_x(bslib::BoardCore& board, int key, PieceType type, Side side) const;
        bool setupBoard_xx(bslib::BoardCore& board, int key, PieceType type, Side side) const;
        bool setupBoard_xxx(bslib::BoardCore& board, int key, PieceType type, Side side) const;
        bool setupBoard_xxxx(bslib::BoardCore& board, int key, PieceType type, Side side) const;

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

    private:

    };

    extern EgtbKey egtbKey;

} // namespace egtb

#endif /* TbKey_hpp */

