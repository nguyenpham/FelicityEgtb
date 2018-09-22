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

#if !(defined    __EGTB_H__)

#define    __EGTB_H__

#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>
#include <vector>

namespace egtb {

#define EGTB_VERSION                    0x100


    ////////////////////////////////////////
#define EGTB_SCORE_DRAW                 0
#define EGTB_SCORE_MATE                 500
#define EGTB_SCORE_WINNING              503

#define EGTB_SCORE_ILLEGAL              504
#define EGTB_SCORE_UNKNOWN              505
#define EGTB_SCORE_MISSING              506

#define EGTB_SCORE_BASE                 380


    ////////////////////////////////////////
#define MIN(a, b)                       (((a) <= (b)) ? (a) : (b))
#define MAX(a, b)                       (((a) >= (b)) ? (a) : (b))

#define getXSide(side)                  ((side)==Side::white ? Side::black : Side::white)
#define getRow(pos)                     ((pos)/9)
#define getCol(pos)                     ((pos)%9)

#define i16                             int16_t
#define i32                             int32_t
#define i64                             int64_t

#define u8                              uint8_t
#define u16                             uint16_t
#define u32                             uint32_t
#define u64                             uint64_t

    ////////////////////////////////////////
    const int EGTB_SIZE_R_HALF          = 50;
    const int EGTB_SIZE_R               = 90;
    const int EGTB_SIZE_P_HALF          = 31;
    const int EGTB_SIZE_P               = 55;

    const int EGTB_SIZE_RR_HALF         = 2045;
    const int EGTB_SIZE_PP_HALF         = 765;
    const int EGTB_SIZE_PPP_HALF        = 13084;

    const int EGTB_SIZE_RC_HALF         = (EGTB_SIZE_R_HALF * EGTB_SIZE_R);
    const int EGTB_SIZE_RP_HALF         = (EGTB_SIZE_R_HALF * EGTB_SIZE_P);

    const int B                         = 0;
    const int W                         = 1;

    const int EGTB_UNCOMPRESS_BIT       = 1 << 31;
    const int64_t EGTB_SMALL_COMPRESS_SIZE  = (1LL << 31) - 1;
    const int64_t EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE = (1LL << 39);
    const int64_t EGTB_LARGE_COMPRESS_SIZE  = (EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE - 1); // 7fffffffff

    enum EgtbMemMode {
        tiny,           // load minimum data into memory
        all,            // load all data into memory, no access hard disk after loading
        smart           // depend on data size, load as small or all mode
    };

    enum EgtbLoadMode {
        loadnow,        // load data
        onrequest       // not load in the begining, load an endgame when having a request
    };

    enum Squares {
        a9, b9, c9, d9, e9, f9, g9, h9, i9,
        a8, b8, c8, d8, e8, f8, g8, h8, i8,
        a7, b7, c7, d7, e7, f7, g7, h7, i7,
        a6, b6, c6, d6, e6, f6, g6, h6, i6,
        a5, b5, c5, d5, e5, f5, g5, h5, i5,
        a4, b4, c4, d4, e4, f4, g4, h4, i4,
        a3, b3, c3, d3, e3, f3, g3, h3, i3,
        a2, b2, c2, d2, e2, f2, g2, h2, i2,
        a1, b1, c1, d1, e1, f1, g1, h1, i1,
        a0, b0, c0, d0, e0, f0, g0, h0, i0
    };

    enum class FlipMode {
        none, horizontal, vertical, rotate
    };

    enum class Side {
        black = 0, white = 1, none = 2, offboard = 3
    };

    enum class PieceType {
        king, advisor, elephant, rook, cannon, horse, pawn, empty, offboard
    };

    enum class EgtbType {
        dtm, newdtm, lookup, tmp, none
    };

    enum EgtbLoadStatus {
        none, loaded, error
    };

    enum AcceptScore {
        real,           // return real / correct scores
        winning         // accept sometimes the score is EGTB_SCORE_WINNING, says the position is winning but it is not real value of Distance To Mate
    };

    void toLower(std::string& str);
    void toLower(char* str);
    std::string posToCoordinateString(int pos);
    std::string getFileName(const std::string& path);
    std::string getVersion();
    std::vector<std::string> listdir(std::string dirname);

    int decompress(char *dst, int uncompresslen, const char *src, int slen);
//    i64 decompressAllBlocks(int blocksize, int blocknum, u32* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);
    i64 decompressAllBlocks(int blocksize, int blocknum, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);

    extern const int egtbPieceListStartIdxByType[7];
    extern const PieceType egtbPieceListIdxToType[16];

    // set it to true if you want to print out more messages
    extern bool egtbVerbose;
    extern int availableAttackerTotal;

    class Piece;
    class Move;
    class MoveList;
    class EgtbFile;
    class EgtbLookup;
    class _EgtbDb;
    class EgtbBoard;
    class EgtbKeyRec;
    class EgtbKey;

} // namespace egtb

#include "EgtbBoard.h"
#include "EgtbLookup.h"
#include "EgtbFile.h"
#include "EgtbDb.h"
#include "EgtbKey.h"

#endif




