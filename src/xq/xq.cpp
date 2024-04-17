/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
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

static const std::string originalFen_xq = "rneakaenr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNEAKAENR w - - 0 1";

XqBoard::XqBoard(ChessVariant _variant)
{
    variant = _variant;
    assert(!Funcs::isChessFamily(variant));

    pieces.clear();
    for(int i = 0; i < 90; i++) {
        pieces.push_back(Piece::emptyPiece);
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

int XqBoard::getColumn(int pos) const
{
    return pos % 9;
}

int XqBoard::getRank(int pos) const
{
    return pos / 9;
}

std::string XqBoard::posToCoordinateString(int pos) const
{
//    return Funcs::chessPosToCoordinateString(pos);
    
    int row = pos / 9, col = pos % 9;
    std::ostringstream stringStream;
    stringStream << char('a' + col) << 9 - row;
    return stringStream.str();

}


bool XqBoard::isValid() const
{
    int pieceCout[2][8] = { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0} };
    
    for (int i = 0; i < pieces.size(); i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) {
            continue;
        }
        
        pieceCout[static_cast<int>(piece.side)][static_cast<int>(piece.type)] += 1;
//        if (piece.type == static_cast<int>(PieceType::pawn)) {
//            if (i < 8 || i >= 56) {
//                return false;
//            }
//        }
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

std::string XqBoard::toString() const
{
    std::ostringstream stringStream;

    stringStream << getFen() << std::endl;

    for (int i = 0; i < 90; i++) {
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
    pieceList_reset((int *)pieceList);
    status = 0;

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

    bool last = false;
    side = Side::white;

    for (int i=0, pos=0; i < (int)thefen.length(); i++) {
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

//        auto pieceType = PieceType::empty;
//        const char* p = strchr(pieceTypeName, ch);
//        if (p==NULL) {
//            if (ch=='n') {
//                pieceType = PieceType::horse;
//            } else if (ch=='b') {
//                pieceType = PieceType::elephant;
//            }
//        } else {
//            int k = (int)(p - pieceTypeName);
//            pieceType = static_cast<PieceType>(k);
//
//        }
        
        auto pieceType = charactorToPieceType(ch);

        if (pieceType != EMPTY) {
            setPiece(int(pos), Piece(pieceType, side));
        }
        pos ++;
    }

    setupPieceIndexes();
    assert(pieceList_isValid());
}


std::string XqBoard::getFen(bool enpassantLegal, int halfCount, int fullMoveCount) const
{
    std::ostringstream stringStream;
    
    int e = 0;
    for (int i = 0; i < pieces.size(); i++) {
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
            if (p.side == attackerSide && (p.type == ROOK || (p.type == PAWN && attackerSide == Side::white))) {
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
                if ((f == 1 && (p.type == ROOK || p.type == KING)) || (f == 2 && p.type == CANNON)) {
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
        if (p.side == attackerSide && (p.type == ROOK || p.type == PAWN)) {
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
            if ((f == 1 && p.type == ROOK) || (f == 2 && p.type == CANNON)) {
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
        if (p.side == attackerSide && (p.type == ROOK || p.type == PAWN)) {
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
            if ((f == 1 && p.type == ROOK) || (f == 2 && p.type == CANNON)) {
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
            if (p.side == attackerSide && (p.type == ROOK || (p.type == PAWN && attackerSide == Side::black))) {
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
                if ((f == 1 && (p.type == ROOK || p.type == KING)) || (f == 2 && p.type == CANNON)) {
                    return true;
                }
            }
            if (f == 2) {
                break;
            }
        }
    }

    /* Check attacking of Knight */
    if (kingPos > 9 && isPiece(kingPos - 11, HORSE, attackerSide) && isEmpty(kingPos - 10)) {
        return true;
    }
    if (kingPos > 18 && isPiece(kingPos - 19, HORSE, attackerSide) && isEmpty(kingPos - 10)) {
        return true;
    }
    if (kingPos > 18 && isPiece(kingPos - 17, HORSE, attackerSide) && isEmpty(kingPos - 8)) {
        return true;
    }
    if (kingPos > 9 && isPiece(kingPos - 7, HORSE, attackerSide) && isEmpty(kingPos - 8)) {
        return true;
    }
    if (kingPos < 81 && isPiece(kingPos + 7, HORSE, attackerSide) && isEmpty(kingPos + 8)) {
        return true;
    }
    if (kingPos < 72 && isPiece(kingPos + 17, HORSE, attackerSide) && isEmpty(kingPos + 8)) {
        return true;
    }
    if (kingPos < 72 && isPiece(kingPos + 19, HORSE, attackerSide) && isEmpty(kingPos + 10)) {
        return true;
    }
    if (kingPos < 81 && isPiece(kingPos + 11, HORSE, attackerSide) && isEmpty(kingPos + 10)) {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////

void XqBoard::gen(std::vector<MoveFull>& moves, Side side) const
{
    moves.reserve(MaxMoveBranch);

    const int* pl = pieceList[static_cast<int>(side)];

    for (int l = 0; l < 16; l++) {
        auto pos = pl[l];
        if (pos < 0) {
            continue;
        }

        auto piece = pieces[pos]; assert(piece.side == side);

        switch (static_cast<PieceType>(piece.type)) {
            case PieceType::king:
            {
                auto col = pos % 9;
                if (col != 3) { // go left
                    gen_addMove(moves, pos, pos - 1);
                }
                if (col != 5) { // right
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
                int y = pos - 10;   /* go left up */
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
                int y = pos - 20; /* go left up */
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
                int f = 0;

                for (int y=pos - 1; y >= pos - col; y--) {
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
                for (int y=pos + 1; y < pos - col + 9; y++) {
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
                for (int y=pos - 9; y >= 0; y -= 9) { /* go up */
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
                for (int y=pos + 9; y < 90; y += 9) { /* go down */
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
                int col = pos % 9;
                for (int y=pos - 1; y >= pos - col; y--) { /* go left */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }
                }

                for (int y=pos + 1; y < pos - col + 9; y++) { /* go right */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }
                }

                for (int y=pos - 9; y >= 0; y -= 9) { /* go up */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }

                }

                for (int y=pos + 9; y < 90; y += 9) { /* go down */
                    gen_addMove(moves, pos, y);
                    if (!isEmpty(y)) {
                        break;
                    }

                }
                break;
            }

            case PieceType::horse:
            {
                int col = pos % 9;
                int y = pos - 11;
                int z = pos - 1;
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
                    int col = pos % 9;
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


std::string XqBoard::chessPiece2String(const Piece& piece, bool alwayLowerCase)
{
    char ch = Funcs::xqPieceType2Char(piece.type);
    if (!alwayLowerCase && piece.side == Side::white) {
        ch += 'A' - 'a';
    }
    return std::string(1, ch);
}

std::string XqBoard::piece2String(const Piece& piece, bool alwayLowerCase)
{
    return chessPiece2String(piece, alwayLowerCase);
}

char XqBoard::pieceType2Char(int pieceType) const
{
    return Funcs::chessPieceType2Char(pieceType);
}

std::string XqBoard::toString(const Piece& piece) const
{
    return chessPiece2String(piece, false);
}

std::string XqBoard::moveString_coordinate(const Move& move)
{
    std::ostringstream stringStream;
    stringStream << Funcs::xqPosToCoordinateString(move.from) << Funcs::xqPosToCoordinateString(move.dest);
    if (move.promotion > KING) {
        stringStream << chessPiece2String(Piece(move.promotion, Side::white), true);
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

std::string XqBoard::hist2String(const HistBasic& hist)
{
    return moveString_coordinate(Move(hist.move));
}


int XqBoard::charactorToPieceType(char ch) const
{
    return Funcs::xqCharactorToPieceType(ch);
}

void XqBoard::clone(const BoardCore* oboard)
{
    BoardCore::clone(oboard);
    assert(!Funcs::isChessFamily(oboard->variant));
    auto ob = static_cast<const XqBoard*>(oboard);
}


int XqBoard::toPieceCount(int* pieceCnt) const
{
    if (pieceCnt) {
        memset(pieceCnt, 0, 14 * sizeof(int));
    }
    auto totalCnt = 0;;
    for(int i = 0; i < 64; i++) {
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

bool XqBoard::pieceList_setPiece(int *pieceList, int pos, int type, Side side) {
    auto d = side == Side::white ? 16 : 0;
    
    auto k = type == KING ? 1 : type == PAWN ? 5 : 2;
    for (auto t = pieceListStartIdxByType[static_cast<int>(type)]; k > 0; t++, k--) {
        assert (t >= 0 && t < 16);
        if (pieceList[d + t] < 0 || pieceList[d + t] == pos) {
            pieceList[d + t] = pos;
            return true;
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
            if (k >= pieces.size()) {
                return false;
            }
            auto piece = pieces[k];
            if (static_cast<int>(piece.side) != sd) {
                return false;
            }
            
            auto tp = static_cast<int>(pieceListIdxToType[i]);
            
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

bool XqBoard::pieceList_setEmpty(int *pieceList, int pos, int type, Side side) {
    auto k = type == KING ? 1 : type == PAWN ? 5 : 2;
    int d = side == Side::white ? 16 : 0;
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


#endif // _FELICITY_XQ_
