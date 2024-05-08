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

#ifndef Defs_h
#define Defs_h

#define EGTB_SCORE_MAX                      245
//#define EGTB_SCORE_UNSET                    1007 //(127 + EGTB_SCORE_BASE)


enum class GameResultType { // Based on white side
    win,    // white wins
    loss,   // white loses
    draw,
    unknown
};

enum class GameReasonType {
    draw_repetition,
    draw_check_idle,
    draw_same_violate,
    perpetual_check,
    perpetual_chase,
    unknown
};

#ifdef _WIN32
    #define uint unsigned int
#endif

#define i64 int64_t

enum CompressMode {
    compress_none, compress, compress_optimizing
};

#define EGTBLU_FILENAME_EXT             ".ltb"
#define EGTBLU_ZIP_FILENAME_EXT         ".zlt"

////////////////////////////////////

/*
 * The rank and file are actually not full file & rank but only internal bits 9-2=7 & 10-2=8
 */
#define MASKRANK			0x7f // 0x1ff
#define MASKFILE			0xff //0x3ff

//#define Status_incheck      (1<<0)
//#define Status_notincheck   (1<<1)
//
//#define isPosValid(pos) ((pos)>=0 && (pos)<90)

//#define getXSide(side) ((side)==bslib::Side::white ? bslib::Side::black : bslib::Side::white)
//
//#define getRow(pos) ((pos)/9)
//#define getCol(pos) ((pos)%9)
//
//#define sider(side) (static_cast<int>(side))


#define MATE_BEGIN                      8000
#define MATE_EVAL0                      8200
#define MATE_EVAL1                      8400
#define MATE_EVAL2                      8600
#define MATE                            10000

#define VALUE_ROOK                      1000
#define VALUE_CANNON                    500
#define VALUE_KNIGHT                    450
#define VALUE_PAWN                      100
#define VALUE_ELEPHANT                  250
#define VALUE_ADVISOR                   200


enum ScoreType {
    none = 0, exact = 1, lower = 2, upper = 3
};


#define MoveMAX                         200
#define PlyMAX                          200

#define TB_ID_MAIN_V1                   556678
#define EGTB_ID_MAIN_V2                 556679
#define EGTB_ID_MAIN_V3                 556681
//#define EGTB_ID_MAIN_V4                 556682

//enum class SearchMode {
//    normal, ponder, infinite, test
//};

#endif /* Defs_h */
