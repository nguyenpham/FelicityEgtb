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

#ifndef Egtb_h
#define Egtb_h


#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include <assert.h>

namespace egtb {

    const int EGTB_VERSION = 0x001;

    enum {
        EGTB_IDX_K_8,
        EGTB_IDX_K_2,
        EGTB_IDX_K,
        EGTB_IDX_KK_8,
        EGTB_IDX_KK_2,

        EGTB_IDX_Q = 16,
        EGTB_IDX_R,
        EGTB_IDX_B,
        EGTB_IDX_H,
        EGTB_IDX_P,

        EGTB_IDX_QQ,
        EGTB_IDX_RR,
        EGTB_IDX_BB,
        EGTB_IDX_HH,
        EGTB_IDX_PP,

        EGTB_IDX_QQQ,
        EGTB_IDX_RRR,
        EGTB_IDX_BBB,
        EGTB_IDX_HHH,
        EGTB_IDX_PPP,

        EGTB_IDX_QQQQ,
        EGTB_IDX_RRRR,
        EGTB_IDX_BBBB,
        EGTB_IDX_HHHH,
        EGTB_IDX_PPPP,


        EGTB_IDX_LAST = EGTB_IDX_PPPP,

        EGTB_IDX_NONE = 254
    };


#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

#define EGTB_SCORE_DRAW         0

#define EGTB_SCORE_MATE         1000

#define EGTB_SCORE_WINNING      1003

#define EGTB_SCORE_ILLEGAL      1004
#define EGTB_SCORE_UNKNOWN      1005
#define EGTB_SCORE_MISSING      1006
#define EGTB_SCORE_UNSET        1007


    ////////////////////////////////////
#define EGTB_SIZE_K2            32
#define EGTB_SIZE_K8            10
#define EGTB_SIZE_K             64

#define EGTB_SIZE_KK8           564
#define EGTB_SIZE_KK2           1806

#define EGTB_SIZE_X             64
#define EGTB_SIZE_XX            2016
#define EGTB_SIZE_XXX           41664
#define EGTB_SIZE_XXXX          635376

#define EGTB_SIZE_P             48
#define EGTB_SIZE_PP            1128
#define EGTB_SIZE_PPP           17296
#define EGTB_SIZE_PPPP          194580


#define EGTB_ID_MAIN_V0                 23456

#define EGTB_SIZE_COMPRESS_BLOCK        (4 * 1024)
#define EGTB_PROP_COMPRESSED            (1 << 2)
#define EGTB_PROP_SPECIAL_SCORE_RANGE   (1 << 3)

#define EGTB_HEADER_SIZE                128

//#define DARK                            8
//#define LIGHT                           16
//
//#define B                               0
//#define W                               1

#define EGTB_SMART_MODE_THRESHOLD       10L * 1024 * 1024L

    const int EGTB_UNCOMPRESS_BIT       = 1 << 31;

//    enum class Side {
//        black = 0, white = 1, none = 2, offboard = 3
//    };
//
//    enum class PieceType {
//        king, queen, rook, bishop, knight, pawn, empty, offboard
//    };

    enum class GameResultType { // Based on white side
        win,    // white wins
        loss,   // white loses
        draw,
        unknown
    };



#define i16 int16_t
#define u16 uint16_t
#define i32 int32_t
#define u32 uint32_t
#define u8  uint8_t
#define u64 uint64_t
#define i64 int64_t

//    enum class FlipMode {
//        none, horizontal, vertical, flipVH, flipHV, rotate90, rotate180, rotate270
//    };

    /*
     * The rank and file are actually not full file & rank but only internal bits 9-2=7 & 10-2=8
     */
#define isPosValid(pos) ((pos)>=0 && (pos)<90)


#define CASTLERIGHT_LONG        (1<<0)
#define CASTLERIGHT_SHORT       (1<<1)

#define CASTLERIGHT_MASK        (CASTLERIGHT_LONG|CASTLERIGHT_SHORT)

#define Status_incheck      (1<<4)
#define Status_notincheck   (1<<5)

    enum EgtbMemMode {
        tiny,          // load minimum to memory
        all,            // load all data into memory, no access hard disk after loading
        smart           // depend on data size, load as small or all mode
    };

    enum EgtbLoadMode {
        loadnow,
        onrequest
    };

    enum EgtbLoadStatus {
        none, loaded, error
    };

    void toLower(std::string& str);
    void toLower(char* str);
    std::string posToCoordinateString(int pos);
    std::string getFileName(const std::string& path);
    std::string getVersion();
    std::vector<std::string> listdir(std::string dirname);

    int decompress(char *dst, int uncompresslen, const char *src, int slen);
    i64 decompressAllBlocks(int blocksize, int blocknum, u32* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);

    // set it to true if you want to print out more messages
    extern bool egtbVerbose;

    class Piece;
    class Move;
    class MoveList;
    class EgtbFile;
    class EgtbDb;
    class EgtbBoardCore;
    class EgtbMailBoard;
    class EgtbKeyRec;
    class EgtbKey;

} // namespace egtb

//#include "EgtbBoard.h"
#include "../chess/chess.h"


#include "EgtbFile.h"
#include "EgtbDb.h"
#include "EgtbKey.h"


#endif

