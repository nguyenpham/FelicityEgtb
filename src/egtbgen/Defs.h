//
//  Defs.h
//
//  Created by Nguyen Hong Pham on 1/12/16.
//

#ifndef Defs_h
#define Defs_h

#define EGTB_GENERATOR_VERSION              0x107

#define EGTB_SCORE_MAX                      245
#define EGTB_SCORE_UNSET                    1007 //(127 + EGTB_SCORE_BASE)

//#define EGTB_SCORE_START_CHECKING        1008
//#define EGTB_SCORE_START_EVASION         1009
//
//// Temporary scores
//#define EGTB_SCORE_CHECKED                  1008 // EGTB_SCORE_START_CHECKING       // 508
//#define EGTB_SCORE_EVASION                  1009 // EGTB_SCORE_START_EVASION     //509
//
////// Temporary scores
//#define TB_CHECKED                          125
//#define TB_EVASION                          126 //TB_START_CHECKING //


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

enum class SearchMode {
    normal, ponder, infinite, test
};

#endif /* Defs_h */
