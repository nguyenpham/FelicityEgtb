/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
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

#else
enum class PieceType {
    empty, king, advisor, elephant, rook, cannon, horse, pawn
};

const int ADVISOR = static_cast<int>(PieceType::advisor);
const int ELEPHANT = static_cast<int>(PieceType::elephant);
const int CANNON = static_cast<int>(PieceType::cannon);
const int HORSE = static_cast<int>(PieceType::horse);

#endif


const int MaxMoveBranch = 250;

const int EMPTY = 0;
const int KING = 1;
const int PAWN = static_cast<int>(PieceType::pawn);
const int ROOK = static_cast<int>(PieceType::rook);

const int B = 0;
const int W = 1;

enum class Side {
    black = 0, white = 1, none = 2
};

enum class FlipMode {
    none, horizontal, vertical, rotate
};


//enum ChessPos {
//    pos_a8, pos_b8, pos_c8, pos_d8, pos_e8, pos_f8, pos_g8, pos_h8,
//    pos_a7, pos_b7, pos_c7, pos_d7, pos_e7, pos_f7, pos_g7, pos_h7,
//    pos_a6, pos_b6, pos_c6, pos_d6, pos_e6, pos_f6, pos_g6, pos_h6,
//    pos_a5, pos_b5, pos_c5, pos_d5, pos_e5, pos_f5, pos_g5, pos_h5,
//    pos_a4, pos_b4, pos_c4, pos_d4, pos_e4, pos_f4, pos_g4, pos_h4,
//    pos_a3, pos_b3, pos_c3, pos_d3, pos_e3, pos_f3, pos_g3, pos_h3,
//    pos_a2, pos_b2, pos_c2, pos_d2, pos_e2, pos_f2, pos_g2, pos_h2,
//    pos_a1, pos_b1, pos_c1, pos_d1, pos_e1, pos_f1, pos_g1, pos_h1,
//};


class Piece {
public:
    int type, idx;
    Side side;

public:
    Piece() {}
    Piece(int _type, Side _side) {
        set(_type, _side);
    }

    static Piece emptyPiece;

    void set(int _type, Side _side) {
        type = _type;
        side = _side;
        assert(isValid());
    }

    void setEmpty() {
        set(EMPTY, Side::none);
    }

    bool isEmpty() const {
        return type == EMPTY;
    }

    bool isPiece(int _type, Side _side) const {
        return type == _type && side == _side;
    }

    bool isValid() const {
        return (side == Side::none && type == EMPTY) || (side != Side::none && type != EMPTY);
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
    Move() {}
    Move(int from, int dest, int promotion = EMPTY)
    : from(from), dest(dest), promotion(promotion)
    {}

    static Move illegalMove;

    static bool isValidPromotion(int promotion) {
        return promotion == EMPTY || promotion > KING;
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

    std::string toCoordinateString(ChessVariant chessVariant) const;

public:
    int from, dest;
    int promotion;
};

class MoveFull : public Move {
public:
    Piece piece;
    int score = 0;

    static MoveFull illegalMove;

public:
    MoveFull() {}
    MoveFull(Piece piece, int from, int dest, int promotion = EMPTY)
    : Move(from, dest, promotion), piece(piece)
    {}
    MoveFull(int from, int dest, int promotion = EMPTY)
    : Move(from, dest, promotion)
    {}


    void set(Piece _piece, int _from, int _dest, int _promote = EMPTY) {
        piece = _piece;
        from = _from;
        dest = _dest;
        promotion = _promote;
    }

    void set(int _from, int _dest, int _promote) {
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

class HistBasic {
public:
    MoveFull move;
    std::string sanString;
};


class Hist : public HistBasic {
public:
    Piece cap;
    int enpassant, status, castled;
    int8_t castleRights[2];
    int quietCnt;
    
    void set(const MoveFull& _move) {
        move = _move;
    }

    bool isValid() const {
        return move.isValid() && cap.isValid();
    }
    
};


} // namespace bslib

#endif // bs_type_h
