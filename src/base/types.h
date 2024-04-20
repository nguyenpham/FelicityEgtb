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
        return (side == Side::none && type == PieceType::empty) || (side != Side::none && type != PieceType::empty);
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

class Hist {
public:
    MoveFull move;
    Piece cap;
    int status, quietCnt;
    
#ifdef _FELICITY_CHESS_
    int enpassant, castled;
    int8_t castleRights[2];
#endif
    
    
    void set(const MoveFull& _move) {
        move = _move;
    }

    bool isValid() const {
        return move.isValid() && cap.isValid();
    }
    
};


} // namespace bslib

#endif // bs_type_h
