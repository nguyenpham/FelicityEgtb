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

#include <fstream>
#include <iostream>
#include <sstream>

#include "xq.h"
#include "../base/funcs.h"

#ifdef _FELICITY_XQ_

using namespace bslib;

extern const int pieceListStartIdxByType[8];
const int pieceListStartIdxByType[8] = { -1, 0, 1, 3, 5, 7, 9, 11 };

extern const PieceType pieceListIdxToType[16];

const PieceType pieceListIdxToType[16] = {
    PieceType::king,
    PieceType::advisor, PieceType::advisor,
    PieceType::elephant, PieceType::elephant,
    PieceType::rook, PieceType::rook,
    PieceType::cannon, PieceType::cannon,
    PieceType::horse, PieceType::horse,
    PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn,
    PieceType::pawn
};


static const std::string originalFen_xq = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";

XqBoard::XqBoard(ChessVariant _variant)
{
    variant = _variant;
    assert(!Funcs::isChessFamily(variant));

    for(auto i = 0; i < BOARD_SZ; i++) {
        pieces[i] = Piece::emptyPiece;
    }
}

XqBoard::XqBoard(const XqBoard& other)
{
    clone(&other);
}

XqBoard::~XqBoard()
{
}

int XqBoard::columnCount() const
{
    return 9;
}

int XqBoard::rankCount() const
{
    return 10;
}


int XqBoard::getColumn(int pos) const
{
    return pos % 9;
}

int XqBoard::getRank(int pos) const
{
    return pos / 9;
}

int XqBoard::coordinateStringToPos(const std::string& str) const
{
    auto colChr = str[0], rowChr = str[1];
    if (colChr >= 'a' && colChr <= 'j' && rowChr >= '0' && rowChr <= '9') {
        int col = colChr - 'a';
        int row = rowChr - '0';

        return (9 - row) * 9 + col;
    }

    return -1;
}

std::string XqBoard::posToCoordinateString(int pos) const
{
    auto row = pos / 9, col = pos % 9;
    std::ostringstream stringStream;
    stringStream << char('a' + col) << 9 - row;
    return stringStream.str();

}


bool XqBoard::isValid() const
{
    int pieceCout[2][8] = { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0} };
    
    for (auto i = 0; i < BOARD_SZ; i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) {
            continue;
        }
        
        pieceCout[static_cast<int>(piece.side)][static_cast<int>(piece.type)] += 1;
    }
    
    bool b =
    pieceCout[0][1] == 1 && pieceCout[1][1] == 1 &&     // king
    pieceCout[0][2] <= 2 && pieceCout[1][2] <= 2 &&     // advisor
    pieceCout[0][3] <= 2 && pieceCout[1][3] <= 2 &&     // elephant
    pieceCout[0][4] <= 2 && pieceCout[1][4] <= 2 &&     // rook
    pieceCout[0][5] <= 2 && pieceCout[1][5] <= 2 &&     // cannon
    pieceCout[0][6] <= 2 && pieceCout[1][6] <= 2 &&     // knight
    pieceCout[0][7] <= 5 && pieceCout[1][7] <= 5;       // pawn

    return b;
}

/// Two Kings should not face to each other
bool XqBoard::isLegal() const
{
    auto bK = pieceList[0][0]; assert(isPositionValid(bK));
    auto wK = pieceList[1][0]; assert(isPositionValid(wK));
    
    auto f = bK % 9;
    if (f != wK % 9) {
        return true;
    }
    
    for(auto x = bK + 9; x < wK; x += 9) {
        if (!isEmpty(x)) {
            return true;
        }
    }
    
    return false;
}


std::string XqBoard::toString() const
{
    std::ostringstream stringStream;

    stringStream << getFen() << std::endl;

    for (auto i = 0; i < 90; i++) {
        auto piece = getPiece(i);
        stringStream << toString(Piece(piece.type, piece.side)) << " ";

        if (i > 0 && i % 9 == 8) {
            int row = 9 - i / 9;
            stringStream << " " << row << "\n";
        }
    }

    stringStream << "a b c d e f g h i  ";

    if (side != Side::none) {
        stringStream << (side == Side::white ? "w turns" : "b turns") << "\n";
    }
    return stringStream.str();
}


void XqBoard::setFen(const std::string& fen)
{
    reset();
//    pieceList_reset((int *)pieceList);

    std::string str = fen;
    startFen = fen;
    auto originalFen = originalFen_xq;
    if (fen.empty()) {
        str = originalFen;
    } else {
        if (memcmp(fen.c_str(), originalFen.c_str(), originalFen.size()) == 0) {
            startFen = "";
        }
    }
    
    side = Side::none;
    for (auto && p : pieces) {
        p.setEmpty();
    }

    std::string thefen = fen;
    if (fen.empty()) {
        thefen = originalFen;
    }

    auto last = false;
    side = Side::white;

    for (auto i=0, pos=0; i < (int)thefen.length(); i++) {
        char ch = thefen.at(i);

        if (ch==' ') {
            last = true;
            continue;
        }

        if (last) {
            if (ch=='w' || ch=='W') {
                side = Side::white;
            } else if (ch=='b' || ch=='B') {
                side = Side::black;
            }

            continue;
        }

        if (ch=='/') {
            continue;
        }

        if (ch>='0' && ch <= '9') {
            int num = ch - '0';
            pos += num;
            continue;
        }

        Side side = Side::black;
        if (ch >= 'A' && ch < 'Z') {
            side = Side::white;
            ch += 'a' - 'A';
        }

        auto pieceType = Funcs::charactorToPieceType(ch);

        if (pieceType != PieceType::empty) {
            setPiece(int(pos), Piece(pieceType, side));
        }
        pos ++;
    }

    assert(pieceList_isValid());
}


std::string XqBoard::getFen(bool enpassantLegal, int halfCount, int fullMoveCount) const
{
    std::ostringstream stringStream;
    
    int e = 0;
    for (int i = 0; i < BOARD_SZ; i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) {
            e += 1;
        } else {
            if (e) {
                stringStream << e;
                e = 0;
            }
            stringStream << toString(piece);
        }
        
        if (i % 9 == 8) {
            if (e) {
                stringStream << e;
            }
            if (i < 89) {
                stringStream << "/";
            }
            e = 0;
        }
    }
    
    stringStream << (side == Side::white ? " w " : " b ") << halfCount << " " << fullMoveCount;

    return stringStream.str();

}

void XqBoard::gen_addMove(std::vector<MoveFull>& moveList, int from, int dest) const
{
    auto toSide = getPiece(dest).side;
    auto movingPiece = getPiece(from);
    auto fromSide = movingPiece.side;
    
    if (fromSide != toSide) {
        moveList.push_back(MoveFull(movingPiece, from, dest));
    }
}

bool XqBoard::isIncheck(Side beingAttackedSide) const {
    auto kingPos = findKing(beingAttackedSide);
    auto attackerSide = xSide(beingAttackedSide);
    
//    incheckCnt ++;
    /*
     * Check horizontal and vertical lines for attacking of Rook, Cannon and
     * King face
     */

    /* go down */
    int y = kingPos + 9;
    if (y < 90) {
        int f = 0;
        auto p = pieces[y];
        if (!p.isEmpty()) {
            f = 1;
            if (p.side == attackerSide && (p.type ==PieceType::rook || (p.type ==PieceType::pawn && attackerSide == Side::white))) {
                return true;
            }
        }

        for (int yy = y+9; yy < 90; yy+=9) {
            auto p = pieces[yy];
            if (p.isEmpty()) {
                continue;
            }
            f++;
            if (p.side == attackerSide) {
                if ((f == 1 && (p.type ==PieceType::rook || p.type ==PieceType::king)) || (f == 2 && p.type ==PieceType::cannon)) {
                    return true;
                }
            }
            if (f == 2) {
                break;
            }
        }
    }

    /* go left */
    y = kingPos - 1;
    int f = 0;

    auto p = pieces[y];
    if (!p.isEmpty()) {
        f = 1;
        if (p.side == attackerSide && (p.type ==PieceType::rook || p.type ==PieceType::pawn)) {
            return true;
        }
    }

    int col = kingPos % 9;

    for (int yy = y-1; yy >= kingPos - col; yy--) {
        auto p = pieces[yy];
        if (p.isEmpty()) {
            continue;
        }
        f++;
        if (p.side == attackerSide) {
            if ((f == 1 && p.type ==PieceType::rook) || (f == 2 && p.type ==PieceType::cannon)) {
                return true;
            }
        }
        if (f == 2) {
            break;
        }
    }

    /* go right */
    y = kingPos + 1;
    f = 0;
    p = pieces[y];
    if (!p.isEmpty()) {
        f = 1;
        if (p.side == attackerSide && (p.type ==PieceType::rook || p.type ==PieceType::pawn)) {
            return true;
        }
    }

    for (int yy = y+1; yy < kingPos - col + 9; yy++) {
        auto p = pieces[yy];
        if (p.isEmpty()) {
            continue;
        }
        f++;
        if (p.side == attackerSide) {
            if ((f == 1 && p.type ==PieceType::rook) || (f == 2 && p.type ==PieceType::cannon)) {
                return true;
            }
        }
        if (f == 2) {
            break;
        }
    }

    /* go up */
    y = kingPos - 9;
    if (y >= 0) {
        f = 0;
        p = pieces[y];
        if (!p.isEmpty()) {
            f = 1;
            if (p.side == attackerSide && (p.type ==PieceType::rook || (p.type ==PieceType::pawn && attackerSide == Side::black))) {
                return true;
            }
        }

        for (int yy = y-9; yy >= 0; yy-=9) {
            auto p = pieces[yy];
            if (p.isEmpty()) {
                continue;
            }
            f++;
            if (p.side == attackerSide) {
                if ((f == 1 && (p.type ==PieceType::rook || p.type ==PieceType::king)) || (f == 2 && p.type ==PieceType::cannon)) {
                    return true;
                }
            }
            if (f == 2) {
                break;
            }
        }
    }

    /* Check attacking of Knight */
    if (kingPos > 9 && isPiece(kingPos - 11, PieceType::horse, attackerSide) && isEmpty(kingPos - 10)) {
        return true;
    }
    if (kingPos > 18 && isPiece(kingPos - 19, PieceType::horse, attackerSide) && isEmpty(kingPos - 10)) {
        return true;
    }
    if (kingPos > 18 && isPiece(kingPos - 17, PieceType::horse, attackerSide) && isEmpty(kingPos - 8)) {
        return true;
    }
    if (kingPos > 9 && isPiece(kingPos - 7, PieceType::horse, attackerSide) && isEmpty(kingPos - 8)) {
        return true;
    }
    if (kingPos < 81 && isPiece(kingPos + 7, PieceType::horse, attackerSide) && isEmpty(kingPos + 8)) {
        return true;
    }
    if (kingPos < 72 && isPiece(kingPos + 17, PieceType::horse, attackerSide) && isEmpty(kingPos + 8)) {
        return true;
    }
    if (kingPos < 72 && isPiece(kingPos + 19, PieceType::horse, attackerSide) && isEmpty(kingPos + 10)) {
        return true;
    }
    if (kingPos < 81 && isPiece(kingPos + 11, PieceType::horse, attackerSide) && isEmpty(kingPos + 10)) {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////

void XqBoard::gen(std::vector<MoveFull>& moves, Side side) const
{
    moves.reserve(MaxMoveBranch);

    const int* pl = pieceList[static_cast<int>(side)];

    for (auto l = 0; l < 16; l++) {
        auto pos = pl[l];
        if (pos < 0) {
            continue;
        }

        auto piece = pieces[pos]; assert(piece.side == side);

        switch (static_cast<PieceType>(piece.type)) {
            case PieceType::king:
            {
                auto col = pos % 9;
                if (col != 3) { /// go left
                    gen_addMove(moves, pos, pos - 1);
                }
                if (col != 5) { /// right
                    gen_addMove(moves, pos, pos + 1);
                }
                if (pos > 72 || (pos > 8 && pos < 27)) { // up
                    gen_addMove(moves, pos, pos - 9);
                }
                if (pos < 18 || (pos > 63 && pos < 81)) { // down
                    gen_addMove(moves, pos, pos + 9);
                }
                break;
            }

            case PieceType::advisor:
            {
                auto y = pos - 10;   /* go left up */
                if (y == 3 || y == 13 || y == 66 || y == 76) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 8;        /* go right up */
                if (y == 5 || y == 13 || y == 68 || y == 76) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 8;        /* go left down */
                if (y == 13 || y == 21 || y == 84 || y == 76) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 10;       /* go right down */
                if (y == 13 || y == 23 || y == 76 || y == 86) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }

            case PieceType::elephant:
            {
                auto y = pos - 20; /* go left up */
                if ((y == 2 || y == 6 || y == 18 || y == 22 || y == 47 || y == 51 || y == 63 || y == 67) && isEmpty(pos - 10)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 16; /* go right up */
                if ((y == 2 || y == 6 || y == 22 || y == 26 || y == 47 || y == 51 || y == 67 || y == 71) && isEmpty(pos - 8)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 16; /* go left down */
                if ((y == 18 || y == 22 || y == 38 || y == 42 || y == 63 || y == 67 || y == 83 || y == 87) && isEmpty(pos + 8)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 20; /* go right up */
                if ((y == 22 || y == 26 || y == 38 || y == 42 || y == 67 || y == 71 || y == 83 || y == 87) && isEmpty(pos + 10)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }

            case PieceType::cannon: {
                int col = pos % 9;
                /*
                 * go left
                 */
                auto f = 0;

                for (auto y = pos - 1; y >= pos - col; y--) {
                    if (isEmpty(y)) {
                        if (f == 0) {
                            gen_addMove(moves, pos, y);
                        }
                        continue;
                    }
                    f++;
                    if (f == 2) {
                        gen_addMove(moves, pos, y);
                        break;
                    }
                }
                /*
                 * go right
                 */
                f = 0;
                for (auto y = pos + 1; y < pos - col + 9; y++) {
                    if (isEmpty(y)) {
                        if (f == 0) {
                            gen_addMove(moves, pos, y);
                        }
                        continue;
                    }
                    f++;
                    if (f == 2) {
                        gen_addMove(moves, pos, y);
                        break;
                    }
                }

                f = 0;
                for (auto y = pos - 9; y >= 0; y -= 9) { /* go up */
                    if (isEmpty(y)) {
                        if (f == 0) {
                            gen_addMove(moves, pos, y);
                        }
                        continue;
                    }
                    f += 1 ;
                    if (f == 2) {
                        gen_addMove(moves, pos, y);
                        break;
                    }
                }

                f = 0;
                for (auto y = pos + 9; y < 90; y += 9) { /* go down */
                    if (isEmpty(y)) {
                        if (f == 0) {
                            gen_addMove(moves, pos, y);
                        }
                        continue;
                    }
                    f += 1 ;
                    if (f == 2) {
                        gen_addMove(moves, pos, y);
                        break;
                    }
                }

                break;
            }

            case PieceType::rook:
            {
                auto col = pos % 9;
                for (auto y = pos - 1; y >= pos - col; y--) { /* go left */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }
                }

                for (auto y = pos + 1; y < pos - col + 9; y++) { /* go right */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }
                }

                for (auto y = pos - 9; y >= 0; y -= 9) { /* go up */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }

                }

                for (auto y = pos + 9; y < 90; y += 9) { /* go down */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }

                }
                break;
            }

            case PieceType::horse:
            {
                auto col = pos % 9;
                auto y = pos - 11;
                auto z = pos - 1;
                if (y >= 0 && col > 1 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 19;
                z = pos - 9;
                if (y >= 0 && col > 0 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 17;
                z = pos - 9;
                if (y >= 0 && col < 8 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 7;
                z = pos + 1;
                if (y >= 0 && col < 7 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }

                y = pos + 7;
                z = pos - 1;
                if (y < 90 && col > 1 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 17;
                z = pos + 9;
                if (y < 90 && col > 0 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 19;
                z = pos + 9;
                if (y < 90 && col < 8 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 11;
                z = pos + 1;
                if (y < 90 && col < 7 && isEmpty(z)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }

            case PieceType::pawn:
            {
                if ((side == Side::black && pos > 44) || (side == Side::white && pos < 45)) {
                    auto col = pos % 9;
                    /* go left */
                    if (col > 0) {
                        gen_addMove(moves, pos, pos - 1);
                    }
                    /* go right */
                    if (col < 8) {
                        gen_addMove(moves, pos, pos + 1);
                    }
                }

                if (side == Side::black) {
                    /* go down */
                    if (pos < 81) {
                        gen_addMove(moves, pos, pos + 9);
                    }
                } else {
                    /* go up */
                    if (pos > 8) {
                        gen_addMove(moves, pos, pos - 9);
                    }
                }
                break;
            }

            default:
                break;
        }
    }
}


void XqBoard::make(const MoveFull& move, Hist& hist)
{
    hist.move = move;
    hist.cap = pieces[move.dest];
    pieces[move.dest] = pieces[move.from];
    pieces[move.from].setEmpty();

    pieceList_make(hist);
}

void XqBoard::takeBack(const Hist& hist)
{
    pieces[hist.move.from] = pieces[hist.move.dest];
    pieces[hist.move.dest] = hist.cap;

    pieceList_takeback(hist);
}


std::string XqBoard::piece2String(const Piece& piece, bool alwayLowerCase) const
{
    char ch = Funcs::pieceTypeName[static_cast<int>(piece.type)];
    if (!alwayLowerCase && piece.side == Side::white) {
        ch += 'A' - 'a';
    }
    return std::string(1, ch);
}

char XqBoard::pieceType2Char(int pieceType) const
{
    return Funcs::pieceTypeName[pieceType];
}

std::string XqBoard::toString(const Piece& piece) const
{
    return piece2String(piece, false);
}


std::string XqBoard::moveString_coordinate(const Move& move) const
{
    std::ostringstream stringStream;
    stringStream << posToCoordinateString(move.from) << posToCoordinateString(move.dest);
    if (move.promotion > PieceType::king) {
        stringStream << piece2String(Piece(static_cast<PieceType>(move.promotion), Side::white), true);
    }
    return stringStream.str();
}

std::string XqBoard::toString(const Move& move) const
{
    return moveString_coordinate(move);
}

std::string XqBoard::toString(const MoveFull& move) const
{
    return toString(Move(move));
}


int XqBoard::toPieceCount(int* pieceCnt) const
{
    if (pieceCnt) {
        memset(pieceCnt, 0, 2 * 10 * sizeof(int));
    }
    auto totalCnt = 0;;
    for(auto i = 0; i < 64; i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) continue;
        totalCnt++;
        auto sd = static_cast<int>(piece.side), type = static_cast<int>(piece.type);
        if (pieceCnt) {
            pieceCnt[sd * 7 + type]++;
        }
    }

    assert(totalCnt >= 2 && totalCnt <= 32);
    return totalCnt;
}

void XqBoard::createFullMoves(std::vector<MoveFull>& moveList, MoveFull m) const
{
    moveList.push_back(m);
}

bool XqBoard::pieceList_setPiece(int *pieceList, int pos, PieceType type, Side side) {
    if (type != PieceType::empty) {
        auto d = side == Side::white ? 16 : 0;
        
        auto k = type == PieceType::king ? 1 : type == PieceType::pawn ? 5 : 2;
        for (auto t = pieceListStartIdxByType[static_cast<int>(type)]; k > 0; t++, k--) {
            assert (t >= 0 && t < 16);
            if (pieceList[d + t] < 0 || pieceList[d + t] == pos) {
                pieceList[d + t] = pos;
                return true;
            }
        }
    }
    
    return false;
}

bool XqBoard::pieceList_isValid() const {
    auto cnt = 0;
    for(auto sd = 0; sd < 2; sd++) {
        for(auto i = 0; i < 16; i++) {
            auto k = pieceList[sd][i];
            if (k < 0) {
                continue;
            }
            if (k >= BOARD_SZ) {
                return false;
            }
            auto piece = pieces[k];
            if (static_cast<int>(piece.side) != sd) {
                return false;
            }
            
            auto tp = pieceListIdxToType[i];
            
            if (piece.type != tp) {
                return false;
            }
            cnt++;
        }
    }
    
    for (auto && p : pieces) {
        if (!p.isEmpty()) cnt--;
    }
    
    return cnt == 0;
}

bool XqBoard::pieceList_setEmpty(int *pieceList, int pos, PieceType type, Side side) {
    auto k = type == PieceType::king ? 1 : type == PieceType::pawn ? 5 : 2;
    auto d = side == Side::white ? 16 : 0;
    for (auto t = pieceListStartIdxByType[static_cast<int>(type)]; k > 0; t++, k--) {
        assert (t >= 0 && t < 16);
        if (pieceList[d + t] == pos) {
            pieceList[d + t] = -1;
            return true;
        }
    }
    
    return false;
}


bool XqBoard::pieceList_make(const Hist& hist) {
    if (!hist.cap.isEmpty()) {
        for (auto t = pieceListStartIdxByType[static_cast<int>(hist.cap.type)], sd = static_cast<int>(hist.cap.side); ; t++) {
            assert (t >= 0 && t < 16);
            if (pieceList[sd][t] == hist.move.dest) {
                pieceList[sd][t] = -1;
                break;
            }
        }
    }
    for (auto t = pieceListStartIdxByType[static_cast<int>(hist.move.piece.type)], sd = static_cast<int>(hist.move.piece.side); ; t++) {
        assert (t >= 0 && t < 16);
        if (pieceList[sd][t] == hist.move.from) {
            pieceList[sd][t] = hist.move.dest;
            return true;
        }
    }
    return false;
}

bool XqBoard::pieceList_takeback(const Hist& hist) {
    for (auto t = pieceListStartIdxByType[static_cast<int>(hist.move.piece.type)], sd = static_cast<int>(hist.move.piece.side); ; t++) {
        assert (t >= 0 && t < 16);
        if (pieceList[sd][t] == hist.move.dest) {
            pieceList[sd][t] = hist.move.from;
            break;
        }
    }
    if (hist.cap.isEmpty()) {
        return true;
    }
    for (auto t = pieceListStartIdxByType[static_cast<int>(hist.cap.type)], sd = static_cast<int>(hist.cap.side); ; t++) {
        assert (t >= 0 && t < 16);
        if (pieceList[sd][t] < 0) {
            pieceList[sd][t] = hist.move.dest;
            return true;
        }
    }
    return false;
}

bool XqBoard::pieceList_isDraw(const int *pieceList) const {
    for(auto t = ROOK; t <= PAWN; t++) {
        if (pieceList[t] >= 0 || pieceList[t + 16] >= 0) {
            return false;
        }
    }
    
    return true;
}

#endif // _FELICITY_XQ_
