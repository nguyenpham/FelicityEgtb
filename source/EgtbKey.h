
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

#ifndef EgtbKey_h
#define EgtbKey_h

#include <map>

#include "Egtb.h"

namespace egtb {

    class EgtbKeyRec {
    public:
        i64 key;
        bool flipSide;

        // for loop-up
        int subKey;
        int groupIdx;
    };

    class EgtbKey {
    public:
        EgtbKey();

        static void getKey(EgtbKeyRec& rec, const int* pieceList, EgtbType egtbType, bool forLookupKey, const int* idxArr, const i64* idxMult, u16 order);
        static int getKey_defence(int k, int a1, int a2, int e1, int e2, EgtbType egtbType);

        bool parseKey_twoStrongPieces(int key, int* pieceList, int sd, FlipMode flip, int attr);
        bool parseKey_oneStrongPiece(int key, int* pieceList, int sd, FlipMode flip, int attr);
        bool parseKey_defence(int attr, int idx, int* pieceList, int sd, FlipMode flip);

    private:
        int getKey_pp(int p0, int p1, EgtbType egtbType) const;
        int getKeyFlip_ppp(int pos0, int pos1, int pos2, EgtbType egtbType) const;

        static int getKey_xx(int p0, int p1, EgtbType egtbType);
        static int getKey_xy(int p0, int p1, EgtbType egtbType);
        static int getKey_xp(int p0, int p1, EgtbType egtbType);

        void initOnce();
        void createPawnKeys();

    private:
        std::map<int, int> pppPos2KeyMap;
        int pppKeyToPos[EGTB_SIZE_PPP_HALF];

    };

    extern EgtbKey egtbKey;

} // namespace egtb

#endif /* TbKey_hpp */

