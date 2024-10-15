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

#ifndef fegtb_h
#define fegtb_h


#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include <assert.h>

#ifdef _FELICITY_CHESS_
#include "../chess/chess.h"
#endif

#ifdef _FELICITY_XQ_
#include "../xq/xq.h"
#endif


//#ifdef _FELICITY_FLIP_MAP_

namespace fegtb {

#ifdef _FELICITY_CHESS_
const std::string EGTB_MAJOR_VARIANT = "Chess";
#else
const std::string EGTB_MAJOR_VARIANT = "Xiangqi/Jeiqi";
#endif

const std::string EGTB_VERSION_STRING = "0.01";

const int EGTB_VERSION = 1;




////////////////////////////////////


#define EGTB_ID_MAIN_V0                 23456

#define EGTB_PROP_SPECIAL_SCORE_RANGE   (1 << 3)


#define i16 int16_t
#define u16 uint16_t
#define i32 int32_t
#define u32 uint32_t
#define u8  uint8_t
#define u64 uint64_t
#define i64 int64_t

const u32       EGTB_UNCOMPRESS_BIT       = 1 << 31;
const int64_t   EGTB_SMALL_COMPRESS_SIZE  = (1LL << 31) - 1;
const int64_t   EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE = (1LL << 39);
const int64_t   EGTB_LARGE_COMPRESS_SIZE  = (EGTB_UNCOMPRESS_BIT_FOR_LARGE_COMPRESSTABLE - 1); // 7fffffffff


const static int COPYRIGHT_BUFSZ = 64;

//////////////////////////////////////////////////////////////////////

#define EGTB_SCORE_DRAW                     0
#define EGTB_SCORE_MATE                     10000

#define EGTB_SCORE_ILLEGAL                  10004

#define EGTB_SCORE_MISSING                  10006
#define EGTB_SCORE_UNSET                    10007


//////////////////////////////////////////////////////////////////////
#ifdef _FELICITY_CHESS_
#define TB_DRAW                             5

#else
#define TB_DRAW                             7


#define EGTB_SCORE_PERPETUAL_CHECK          8199

#define EGTB_SCORE_PERPETUAL_CHASE          8198

#define EGTB_SCORE_PERPETUAL_ABOVE          (EGTB_SCORE_PERPETUAL_CHECK+1)

#define EGTB_SCORE_PERPETUAL_120            (EGTB_SCORE_PERPETUAL_CHASE-120)

#define EGTB_SCORE_PERPETUAL_BELOW          8001

#endif


/// chess: 130, xq: 131
#define TB_START_LOSING                     ((255-TB_DRAW)/2+TB_DRAW)

/// chess: 124, xq: 123
#define TB_RANGE_1_BYTE                     ((255-TB_DRAW)/2-1)

#define TB_ILLEGAL                          0
#define TB_UNSET                            1
#define TB_MISSING                          2


#define TB_START_MATING                     (TB_DRAW + 1)


//////////////////////////////////////////////////////////////////////




    /*
     * The rank and file are actually not full file & rank but only internal bits 9-2=7 & 10-2=8
     */
#define isPosValid(pos) ((pos)>=0 && (pos)<90)


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

    enum class EgtbType {
        dtm, tmp, none
    };

    std::string getFileName(const std::string& path);
    std::string getVersion();
    std::vector<std::string> listdir(std::string dirname);

    int decompress(char *dst, int uncompresslen, const char *src, int slen);
    i64 decompressAllBlocks(int blocksize, int blocknum, u32* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);

    /// set it to true if you want to print out more messages
    extern bool egtbVerbose;

    class EgtbFile;
    class EgtbDb;
    class EgtbKeyRec;
    class EgtbKey;


#ifdef _FELICITY_CHESS_


const int EGTB_SIZE_K2            = 32;
const int EGTB_SIZE_K8            = 10;
const int EGTB_SIZE_K             = 64;

const int EGTB_SIZE_KK8           = 462;

const int EGTB_SIZE_KK2           = 1806;

const int EGTB_SIZE_X             = 64;

/// Combines of similar pieces
const int EGTB_SIZE_XX            = 2016;
const int EGTB_SIZE_XXX           = 41664;
const int EGTB_SIZE_XXXX          = 635376;
const int EGTB_SIZE_XXXXX         = 7624512;

const int EGTB_SIZE_XXXXXX        = 74974368;
const int EGTB_SIZE_XXXXXXX       = 621216192;

const i64 EGTB_SIZE_XXXXXXXX      = 4426165368;
const i64 EGTB_SIZE_XXXXXXXXX     = 27540584512;
const i64 EGTB_SIZE_XXXXXXXXXX    = 151473214816;

const int EGTB_SIZE_P             = 48;
const int EGTB_SIZE_PP            = 1128;
const int EGTB_SIZE_PPP           = 17296;
const int EGTB_SIZE_PPPP          = 194580;
const int EGTB_SIZE_PPPPP         = 1712304;

const int EGTB_SIZE_PPPPPP        = 12271512;
const int EGTB_SIZE_PPPPPPP       = 73629072;
const int EGTB_SIZE_PPPPPPPP      = 377348994;

enum EgtbIdx {
    EGTB_IDX_KK_8,
    EGTB_IDX_KK_2,

    EGTB_IDX_Q = 16,
    EGTB_IDX_R,
    EGTB_IDX_B,
    EGTB_IDX_N,
    EGTB_IDX_P,

    EGTB_IDX_QQ,
    EGTB_IDX_RR,
    EGTB_IDX_BB,
    EGTB_IDX_NN,
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

    EGTB_IDX_QQQQQ,
    EGTB_IDX_RRRRR,
    EGTB_IDX_BBBBB,
    EGTB_IDX_HHHHH,
    EGTB_IDX_PPPPP,

    EGTB_IDX_QQQQQQ,
    EGTB_IDX_RRRRRR,
    EGTB_IDX_BBBBBB,
    EGTB_IDX_HHHHHH,
    EGTB_IDX_PPPPPP,

    EGTB_IDX_QQQQQQQ,
    EGTB_IDX_RRRRRRR,
    EGTB_IDX_BBBBBBB,
    EGTB_IDX_HHHHHHH,
    EGTB_IDX_PPPPPPP,

    EGTB_IDX_QQQQQQQQ,
    EGTB_IDX_RRRRRRRR,
    EGTB_IDX_BBBBBBBB,
    EGTB_IDX_HHHHHHHH,
    EGTB_IDX_PPPPPPPP,

    EGTB_IDX_QQQQQQQQQ,
    EGTB_IDX_RRRRRRRRR,
    EGTB_IDX_BBBBBBBBB,
    EGTB_IDX_HHHHHHHHH,
    EGTB_IDX_PPPPPPPP_, /// not existent

    EGTB_IDX_QQQQQQQQQ_, /// not existent
    EGTB_IDX_RRRRRRRRRR,
    EGTB_IDX_BBBBBBBBBB,
    EGTB_IDX_HHHHHHHHHH,
    EGTB_IDX_PPPPPPPP__, /// not existent


    EGTB_IDX_LAST = EGTB_IDX_PPPPPPPP__,

    EGTB_IDX_NONE = 254
};

#else

const int EGTB_SIZE_X_HALF              = 50;
const int EGTB_SIZE_X                   = 90;
const int EGTB_SIZE_P_HALF              = 31;
const int EGTB_SIZE_P                   = 55;

const int EGTB_SIZE_XX_HALF             = 2045;
const int EGTB_SIZE_XX                  = 4005;

const int EGTB_SIZE_PP_HALF             = 765;
const int EGTB_SIZE_PP                  = 1485;

const int EGTB_SIZE_PPP_HALF            = 13084;

const int EGTB_SIZE_XY_HALF             = (EGTB_SIZE_X_HALF * EGTB_SIZE_X);
const int EGTB_SIZE_XP_HALF             = (EGTB_SIZE_X_HALF * EGTB_SIZE_P);


const int EGTB_SIZE_KAAEE               = 1410;
const int EGTB_SIZE_KAEE                = 810;
const int EGTB_SIZE_KAAE                = 480;
const int EGTB_SIZE_KAE                 = 275;
const int EGTB_SIZE_KEE                 = 183;
const int EGTB_SIZE_KAA                 = 70;
const int EGTB_SIZE_KE                  = 62;
const int EGTB_SIZE_KA                  = 40;
const int EGTB_SIZE_K                   = 9;

enum EgtbIdx {
    /// Defenders
    EGTB_IDX_DK,
    EGTB_IDX_DA,
    EGTB_IDX_DB,
    EGTB_IDX_DAA,
    EGTB_IDX_DBB,
    EGTB_IDX_DAB,
    EGTB_IDX_DAAB,
    EGTB_IDX_DABB,
    EGTB_IDX_DAABB,

    /// Attackers
    EGTB_IDX_R_HALF = 16,
    EGTB_IDX_C_HALF,
    EGTB_IDX_N_HALF,
    EGTB_IDX_P_HALF,

    EGTB_IDX_R_FULL,
    EGTB_IDX_C_FULL,
    EGTB_IDX_N_FULL,
    EGTB_IDX_P_FULL,

    EGTB_IDX_RR_HALF,
    EGTB_IDX_CC_HALF,
    EGTB_IDX_NN_HALF,
    EGTB_IDX_PP_HALF,

    EGTB_IDX_RR_FULL,
    EGTB_IDX_CC_FULL,
    EGTB_IDX_NN_FULL,
    EGTB_IDX_PP_FULL,

    /// not supported yet
    EGTB_IDX_PPP_HALF,
    EGTB_IDX_PPP_FULL,

    EGTB_IDX_PPPP_HALF,
    EGTB_IDX_PPPP_FULL,

    EGTB_IDX_PPPPP_HALF,
    EGTB_IDX_PPPPP_FULL,

    EGTB_IDX_NONE = 254
};

#endif



} // namespace fegtb

#include "fegtbfile.h"
#include "fegtbdb.h"
#include "fegtbkey.h"

#endif

