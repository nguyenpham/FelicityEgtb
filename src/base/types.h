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

#ifndef bs_type_h
#define bs_type_h

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <assert.h>
#include <mutex>
#include <functional>
#include <cstring>


#ifdef _WIN32
#define STRING_PATH_SLASH   "\\"
#define CHAR_PATH_SLASH     '\\'

#else
#define STRING_PATH_SLASH   "/"
#define CHAR_PATH_SLASH     '/'
#endif


namespace bslib {

enum class ChessVariant {
    standard, chess960, xingqi, jeiqi, none
};


#ifdef _FELICITY_CHESS_
enum class PieceType {
    empty, king, queen, rook, bishop, knight, pawn
};

const int QUEEN = static_cast<int>(PieceType::queen);
const int BISHOP = static_cast<int>(PieceType::bishop);
const int KNIGHT = static_cast<int>(PieceType::knight);
const int ROOK = static_cast<int>(PieceType::rook);

const int FirstAttacker = QUEEN;

#define EgtbBoard bslib::ChessBoard

#define BOARD_SZ 64

#else
enum class PieceType {
    empty, king, advisor, elephant, rook, cannon, horse, pawn
};

const int ADVISOR = static_cast<int>(PieceType::advisor);
const int ELEPHANT = static_cast<int>(PieceType::elephant);
const int CANNON = static_cast<int>(PieceType::cannon);
const int HORSE = static_cast<int>(PieceType::horse);
const int ROOK = static_cast<int>(PieceType::rook);

const int FirstAttacker = ROOK;

#define EgtbBoard bslib::XqBoard

#define BOARD_SZ 90

#endif


const int MaxMoveBranch = 250;

const int EMPTY = 0;
const int KING = 1;
const int PAWN = static_cast<int>(PieceType::pawn);

const int B = 0;
const int W = 1;

enum class Side {
    black = 0, white = 1, none = 2
};

enum class GameResultType { // Based on white side
    win,    // white wins
    loss,   // white loses
    draw,
    unknown
};

#ifdef _FELICITY_CHESS_
enum class FlipMode {
    none, horizontal, vertical,
    flipVH, flipHV,     /// for chess only
    rotate90,           /// for chess only
    rotate,             /// 180
    rotate270           /// for chess only
};
#else
enum class FlipMode {
    none, horizontal, vertical, rotate
};
#endif



class Piece {
public:
    PieceType type;
    Side side;

public:
    Piece() {}
    Piece(PieceType _type, Side _side) {
        set(_type, _side);
    }

    static Piece emptyPiece;

    void set(PieceType _type, Side _side) {
        type = _type;
        side = _side;
        assert(isValid());
    }

    void setEmpty() {
        set(PieceType::empty, Side::none);
    }

    bool isEmpty() const {
        return type == PieceType::empty;
    }

    bool isPiece(PieceType _type, Side _side) const {
        return type == _type && side == _side;
    }

    bool isValid() const {
        return (side == Side::none && type == PieceType::empty) 
        || ((side == Side::white || side == Side::black) && type > PieceType::empty);
    }

    bool operator == (const Piece & o) const {
        return type == o.type && side == o.side;
    }
    bool operator != (const Piece & o) const {
        return type != o.type || side != o.side;
    }

};

class Move {
public:
    int from, dest;
    PieceType promotion; /// use for chess and Jeiqi
    
public:
    Move() {}
    Move(int from, int dest, PieceType promotion = PieceType::empty)
    : from(from), dest(dest), promotion(promotion)
    {}

    static Move illegalMove;

    static bool isValidPromotion(PieceType promotion) {
        return promotion > PieceType::king;
    }

    bool hasPromotion() {
        return promotion > PieceType::king;
    }

    bool isValid() const {
        return isValid(from, dest);
    }

    static bool isValid(int from, int dest) {
        return from != dest  && from >= 0 && dest >= 0;
    }

    bool operator == (const Move& other) const {
        return from == other.from && dest == other.dest && promotion == other.promotion;
    }

    bool operator != (const Move& other) const {
        return from != other.from || dest != other.dest || promotion != other.promotion;
    }
};

class MoveFull : public Move {
public:
    Piece piece;
    static MoveFull illegalMove;

public:
    MoveFull() {}
    MoveFull(Piece piece, int from, int dest, PieceType promotion = PieceType::empty)
    : Move(from, dest, promotion), piece(piece)
    {}
    MoveFull(int from, int dest, PieceType promotion = PieceType::empty)
    : Move(from, dest, promotion)
    {}


    void set(Piece _piece, int _from, int _dest, PieceType _promote = PieceType::empty) {
        piece = _piece;
        from = _from;
        dest = _dest;
        promotion = _promote;
    }

    void set(int _from, int _dest, PieceType _promote) {
        from = _from;
        dest = _dest;
        promotion = _promote;
    }

    bool operator == (const MoveFull& other) const {
        return from == other.from && dest == other.dest && promotion == other.promotion;
    }
    bool operator != (const MoveFull& other) const {
        return from != other.from || dest != other.dest || promotion != other.promotion;
    }
    bool operator == (const Move& other) const {
        return from == other.from && dest == other.dest && promotion == other.promotion;
    }
};


//class Hist {
//    friend class BoardData;
//
//public:
//    MoveFull move;
//    Piece cap;
//
////    int quietCnt;
////    
////#ifdef _FELICITY_CHESS_
////    int enpassant, castled;
////    int8_t castleRights[2];
////#endif
//    
//    BoardData boardData;
//    
//    void set(const MoveFull& _move) {
//        move = _move;
//    }
//
//    bool isValid() const {
//        return move.isValid() && cap.isValid();
//    }
//    
//};


#define getXSide(side) ((side)==bslib::Side::white ? bslib::Side::black : bslib::Side::white)

#define sider(side) (static_cast<int>(side))


#ifdef _FELICITY_CHESS_
enum Squares {
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1,
    NoSquare
};

#define COL(pos) ((pos)&7)
#define ROW(pos) ((pos)>>3)

#endif

#ifdef _FELICITY_XQ_
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
#endif

} // namespace bslib

#endif // bs_type_h
