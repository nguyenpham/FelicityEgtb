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

#include "chess.h"
#include "../base/funcs.h"

#ifdef _FELICITY_CHESS_

using namespace bslib;

extern const int pieceListStartIdxByType[7];
const int pieceListStartIdxByType[7] = { -1, 0, 1, 2, 4, 6, 8 };

extern const PieceType pieceListIdxToType[16];

const PieceType pieceListIdxToType[16] = {
    PieceType::king,
    PieceType::queen,
    PieceType::rook, PieceType::rook,
    PieceType::bishop, PieceType::bishop,
    PieceType::knight, PieceType::knight,
    PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn,
    PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn
};


ChessBoard::ChessBoard(ChessVariant _variant)
{
    variant = _variant;
    assert(Funcs::isChessFamily(variant));

    pieces.clear();
    for(int i = 0; i < 64; i++) {
        pieces.push_back(Piece::emptyPiece);
    }
}

ChessBoard::ChessBoard(const ChessBoard& other)
{
    clone(&other);
}

ChessBoard::~ChessBoard()
{
}

int ChessBoard::columnCount() const
{
    return 8;
}

int ChessBoard::getColumn(int pos) const
{
    return pos & 7;
}

int ChessBoard::getRank(int pos) const
{
    return pos >> 3;
}

std::string ChessBoard::posToCoordinateString(int pos) const
{
    return Funcs::chessPosToCoordinateString(pos);
}


bool ChessBoard::isValidCastleRights() const
{
    if (castleRights[B]) {
        if (!isPiece(castleRights_column_king, KING, Side::black)) {
            return false;
        }
        if (((castleRights[B] & CastleRight_long ) && !isPiece(castleRights_column_rook_left, static_cast<int>(PieceType::rook), Side::black)) ||
            ((castleRights[B] & CastleRight_short) && !isPiece(castleRights_column_rook_right, static_cast<int>(PieceType::rook), Side::black))) {
            return false;
        }
    }

    if (castleRights[W]) {
        if (!isPiece(56 + castleRights_column_king, KING, Side::white)) {
            return false;
        }
        if (((castleRights[W] & CastleRight_long ) && !isPiece(56 + castleRights_column_rook_left, static_cast<int>(PieceType::rook), Side::white)) ||
            ((castleRights[W] & CastleRight_short) && !isPiece(56 + castleRights_column_rook_right, static_cast<int>(PieceType::rook), Side::white))) {
            return false;
        }
    }
    return true;
}

bool ChessBoard::isValid() const
{
    int pieceCout[2][7] = { { 0, 0, 0, 0, 0, 0, 0}, { 0, 0, 0, 0, 0, 0, 0} };
    
    for (int i = 0; i < 64; i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) {
            continue;
        }
        
        pieceCout[static_cast<int>(piece.side)][static_cast<int>(piece.type)] += 1;
        if (piece.type == static_cast<int>(PieceType::pawn)) {
            if (i < 8 || i >= 56) {
                return false;
            }
        }
    }
    
    if (castleRights[0] + castleRights[1] && !isValidCastleRights()) {
        return false;
    }
    
    if (enpassant > 0) {
        auto row = getRank(enpassant);
        if (row != 2 && row != 5) {
            return false;
        }
        auto pawnPos = row == 2 ? (enpassant + 8) : (enpassant - 8);
        if (!isPiece(pawnPos, static_cast<int>(PieceType::pawn), row == 2 ? Side::black : Side::white)) {
            return false;
        }
    }
    
    bool b =
    pieceCout[0][1] == 1 && pieceCout[1][1] == 1 &&     // king
    pieceCout[0][2] <= 9 && pieceCout[1][2] <= 9 &&     // queen
    pieceCout[0][3] <= 10 && pieceCout[1][3] <= 10 &&     // rook
    pieceCout[0][4] <= 10 && pieceCout[1][4] <= 10 &&     // bishop
    pieceCout[0][5] <= 10 && pieceCout[1][5] <= 10 &&     // knight
    pieceCout[0][6] <= 8 && pieceCout[1][6] <= 8 &&       // pawn
    pieceCout[0][2]+pieceCout[0][3]+pieceCout[0][4]+pieceCout[0][5] + pieceCout[0][6] <= 15 &&
    pieceCout[1][2]+pieceCout[1][3]+pieceCout[1][4]+pieceCout[1][5] + pieceCout[1][6] <= 15;

    return b;
}

std::string ChessBoard::toString() const
{
    std::ostringstream stringStream;
    
    stringStream << getFen() << std::endl;
    
    for (int i = 0; i<64; i++) {
        auto piece = getPiece(i);
        
        stringStream << toString(Piece(piece.type, piece.side)) << " ";
        
        if (i > 0 && getColumn(i) == 7) {
            int row = 8 - getRank(i);
            stringStream << " " << row << "\n";
        }
    }
    
    stringStream << "a b c d e f g h  " << Funcs::side2String(side, false) << std::endl;
//    stringStream << "key: " << key() << std::endl;
    
    return stringStream.str();
}

void ChessBoard::checkEnpassant()
{
    if ((enpassant >= 16 && enpassant < 24) || (enpassant >= 40 && enpassant < 48))  {
        return;
    }
    enpassant = -1;
}

void ChessBoard::setFenCastleRights_clear()
{
    castleRights[0] = castleRights[1] = 0;
}

void ChessBoard::setFenCastleRights(const std::string& str) {
    for(auto && ch : str) {
        switch (ch) {
            case 'K':
                castleRights[W] |= CastleRight_short;
                break;
            case 'k':
                castleRights[B] |= CastleRight_short;
                break;
            case 'Q':
                castleRights[W] |= CastleRight_long;
                break;
            case 'q':
                castleRights[B] |= CastleRight_long;
                break;

            default:
                break;
        }
    }
}

void ChessBoard::setFen(const std::string& fen)
{
    reset();
    pieceList_reset((int *)pieceList);
    
    std::string str = fen;
    startFen = fen;
    auto originalFen = Funcs::getOriginFen(variant);
    if (fen.empty()) {
        str = originalFen;
    } else {
        if (memcmp(fen.c_str(), originalFen.c_str(), originalFen.size()) == 0) {
            startFen = "";
        }
    }
    
    side = Side::none;
    enpassant = -1;
    status = 0;
    setFenCastleRights_clear();
    
    auto vec = Funcs::splitString(str, ' ');
    auto thefen = vec.front();
    
    for (size_t i = 0, pos = 0; i < thefen.length(); i++) {
        char ch = thefen.at(i);
        
        if (ch=='/') {
            //std::string str = fen.substr();
            continue;
        }
        
        if (ch>='0' && ch <= '8') {
            int num = ch - '0';
            pos += num;
            continue;
        }
        
        auto side = Side::black;
        if (ch >= 'A' && ch < 'Z') {
            side = Side::white;
            ch += 'a' - 'A';
        }
        
        auto pieceType = charactorToPieceType(ch);
        
        if (pieceType != EMPTY) {
            setPiece(int(pos), Piece(pieceType, side));
        }
        pos++;
    }
    
    /// side
    if (vec.size() >= 2) {
        auto str = vec.at(1);
        side = str.length() > 0 && str.at(0) == 'w' ? Side::white : Side::black;
    }
    
    /// castle rights
    if (vec.size() >= 3 && vec.at(2) != "-") {
        auto str = vec.at(2);
        setFenCastleRights(str);
    }
    
    /// enpassant
    if (vec.size() >= 4 && vec.at(3).size() >= 2) {
        // enpassant
        auto str = vec.at(3);
        auto pos = Funcs::chessCoordinateStringToPos(str);
        if (isPositionValid(pos)) {
            enpassant = pos;
        }
    }
    
    /// Half move
    quietCnt = 0;
    if (vec.size() >= 5) {
        // half move
        auto str = vec.at(4);
        auto k = std::atoi(str.c_str());
        if (k >= 0 && k <= 50) {
            quietCnt = k * 2;
            if (side == Side::black) quietCnt++;
        }
    }

    /// full move
    if (vec.size() >= 6) {
        auto str = vec.at(5);
        auto k = std::atoi(str.c_str());
        if (k >= 1 && k <= 2000) {
            fullMoveCnt = k * 2;
        }
    }

    checkEnpassant();    
    setupPieceIndexes();

    assert(pieceList_isValid());
}

bool ChessBoard::isFenValid(const std::string& fen) const
{
    return isChessFenValid(fen);
}

bool ChessBoard::isChessFenValid(const std::string& fen)
{
    if (fen.length() < 10) {
        return false;
    }

    int pieceCnt[2][10];
    memset(pieceCnt, 0, sizeof (pieceCnt));

    auto vec = Funcs::splitString(fen, ' ');
    auto thefen = vec.front();

    auto errCnt = 0, pos = 0;
    for (size_t i = 0; i < thefen.length(); i++) {
        char ch = thefen.at(i);

        if (ch=='/') {
            continue;
        }

        if (ch>='0' && ch <= '8') {
            int num = ch - '0';
            pos += num;
            continue;
        }

        auto side = Side::black;
        if (ch >= 'A' && ch < 'Z') {
            side = Side::white;
            ch += 'a' - 'A';
        }

        auto pieceType = Funcs::chessCharactorToPieceType(ch);
        if (pieceType == EMPTY) {
            errCnt++;
        } else {
            pieceCnt[static_cast<int>(side)][static_cast<int>(pieceType)]++;
        }
        pos++;
    }

    return pos > 50 && pos <= 64 && errCnt < 3 && pieceCnt[0][KING] == 1 && pieceCnt[1][KING] == 1;
}

std::string ChessBoard::getFenCastleRights() const {
    std::string s;
    if (castleRights[W] + castleRights[B]) {
        if (castleRights[W] & CastleRight_short) {
            s += "K";
        }
        if (castleRights[W] & CastleRight_long) {
            s += "Q";
        }
        if (castleRights[B] & CastleRight_short) {
            s += "k";
        }
        if (castleRights[B] & CastleRight_long) {
            s += "q";
        }
    } else {
        s = "-";
    }

    return s;
}

/// check if opponent could capture the last advanced-2-rows Pawn
bool ChessBoard::canRivalCaptureEnpassant() const
{
    if (enpassant > 0 && enpassant < 64) {
        auto col = getColumn(enpassant), row = getRank(enpassant);
        if (row == 2) {
            return ((col && isPiece(enpassant + 7, static_cast<int>(PieceType::pawn), Side::white)) ||
                    (col < 7 && isPiece(enpassant + 9, static_cast<int>(PieceType::pawn), Side::white)));
        } else {
            return ((col && isPiece(enpassant - 9, static_cast<int>(PieceType::pawn), Side::black)) ||
                    (col < 7 && isPiece(enpassant - 7, static_cast<int>(PieceType::pawn), Side::black)));
        }
    }

    return false;
}

std::string ChessBoard::getFen(bool enpassantLegal, int halfCount, int fullMoveCount) const
{
    std::ostringstream stringStream;
    
    int e = 0;
    for (int i = 0; i < 64; i++) {
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
        
        if (i % 8 == 7) {
            if (e) {
                stringStream << e;
            }
            if (i < 63) {
                stringStream << "/";
            }
            e = 0;
        }
    }
    
    std::string epStr = "-";
    if (enpassant > 0 && (!enpassantLegal || canRivalCaptureEnpassant())) {
        epStr = posToCoordinateString(enpassant);
    }

    stringStream << (side == Side::white ? " w " : " b ")
                 << getFenCastleRights() << " " << epStr;

    if (halfCount >= 0 && fullMoveCount >= 0) {
        stringStream << " " << halfCount << " " << fullMoveCount;
    }

    return stringStream.str();

}

void ChessBoard::gen_addMove(std::vector<MoveFull>& moveList, int from, int dest) const
{
    auto toSide = getPiece(dest).side;
    auto movingPiece = getPiece(from);
    auto fromSide = movingPiece.side;
    
    if (fromSide != toSide) {
        moveList.push_back(MoveFull(movingPiece, from, dest));
    }
}

void ChessBoard::gen_addPawnMove(std::vector<MoveFull>& moveList, int from, int dest) const
{
    auto toSide = getPiece(dest).side;
    auto movingPiece = getPiece(from);
    auto fromSide = movingPiece.side;
    
    assert(movingPiece.type == static_cast<int>(PieceType::pawn));
    if (fromSide != toSide) {
        if (dest >= 8 && dest < 56) {
            moveList.push_back(MoveFull(movingPiece, from, dest));
        } else {
            moveList.push_back(MoveFull(movingPiece, from, dest, static_cast<int>(PieceType::queen)));
            moveList.push_back(MoveFull(movingPiece, from, dest, static_cast<int>(PieceType::rook)));
            moveList.push_back(MoveFull(movingPiece, from, dest, BISHOP));
            moveList.push_back(MoveFull(movingPiece, from, dest, static_cast<int>(PieceType::knight)));
        }
    }
}

void ChessBoard::clearCastleRights(int rookPos, Side rookSide) {
    auto col = rookPos % 8;
    if ((col != castleRights_column_rook_left && col != castleRights_column_rook_right)
        || (rookPos > 7 && rookPos < 56)) {
        return;
    }
    auto pos = col + (rookSide == Side::white ? 56 : 0);
    if (pos != rookPos) {
        return;
    }
    auto sd = static_cast<int>(rookSide);
    if (col == castleRights_column_rook_left) {
        castleRights[sd] &= ~CastleRight_long;
    } else if (col == castleRights_column_rook_right) {
        castleRights[sd] &= ~CastleRight_short;
    }
}


bool ChessBoard::isIncheck(Side beingAttackedSide) const {
    auto kingPos = findKing(beingAttackedSide);
    auto attackerSide = xSide(beingAttackedSide);
    return kingPos >= 0 ? beAttacked(kingPos, attackerSide) : false;
}

////////////////////////////////////////////////////////////////////////

void ChessBoard::gen(std::vector<MoveFull>& moves, Side side) const
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
                genBishop(moves, side, pos, true);
                genRook(moves, side, pos, true);
                gen_castling(moves, pos);
                break;
            }

            case PieceType::queen:
            {
                genBishop(moves, side, pos, false);
                genRook(moves, side, pos, false);
                break;
            }

            case PieceType::bishop:
            {
                genBishop(moves, side, pos, false);
                break;
            }

            case PieceType::rook: // both queen and rook here
            {
                genRook(moves, side, pos, false);
                break;
            }

            case PieceType::knight:
            {
                genKnight(moves, side, pos);
                break;
            }

            case PieceType::pawn:
            {
                genPawn(moves, side, pos);
                break;
            }

            default:
                break;
        }
    }
}

void ChessBoard::genKnight(std::vector<MoveFull>& moves, Side, int pos) const
{
    auto col = getColumn(pos);
    auto y = pos - 6;

    if (y >= 0 && col < 6)
        gen_addMove(moves, pos, y);
    y = pos - 10;
    if (y >= 0 && col > 1)
        gen_addMove(moves, pos, y);
    y = pos - 15;
    if (y >= 0 && col < 7)
        gen_addMove(moves, pos, y);
    y = pos - 17;
    if (y >= 0 && col > 0)
        gen_addMove(moves, pos, y);
    y = pos + 6;
    if (y < 64 && col > 1)
        gen_addMove(moves, pos, y);
    y = pos + 10;
    if (y < 64 && col < 6)
        gen_addMove(moves, pos, y);
    y = pos + 15;
    if (y < 64 && col > 0)
        gen_addMove(moves, pos, y);
    y = pos + 17;
    if (y < 64 && col < 7)
        gen_addMove(moves, pos, y);
}

void ChessBoard::genRook(std::vector<MoveFull>& moves, Side, int pos, bool oneStep) const
{
    auto col = getColumn(pos);
    for (int y = pos - 1; y >= pos - col; y--) { /* go left */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }

    for (int y = pos + 1; y < pos - col + 8; y++) { /* go right */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }

    for (int y = pos - 8; y >= 0; y -= 8) { /* go up */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }

    for (int y = pos + 8; y < 64; y += 8) { /* go down */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }
}

void ChessBoard::genBishop(std::vector<MoveFull>& moves, Side, int pos, bool oneStep) const
{
    for (int y = pos - 9; y >= 0 && getColumn(y) != 7; y -= 9) {        /* go left up */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }
    for (int y = pos - 7; y >= 0 && getColumn(y) != 0; y -= 7) {        /* go right up */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }
    for (int y = pos + 9; y < 64 && getColumn(y) != 0; y += 9) {        /* go right down */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }
    for (int y = pos + 7; y < 64 && getColumn(y) != 7; y += 7) {        /* go right down */
        gen_addMove(moves, pos, y);
        if (oneStep || !isEmpty(y)) {
            break;
        }
    }
}

void ChessBoard::genPawn(std::vector<MoveFull>& moves, Side side, int pos) const
{
    auto col = getColumn(pos);

    if (side == Side::black) {
        if (isEmpty(pos + 8)) {
            gen_addPawnMove(moves, pos, pos + 8);
        }
        if (pos < 16 && isEmpty(pos + 8) && isEmpty(pos + 16)) {
            gen_addMove(moves, pos, pos + 16);
        }

        if (col && (getPiece(pos + 7).side == Side::white || (pos + 7 == enpassant && getPiece(pos + 7).side == Side::none))) {
            gen_addPawnMove(moves, pos, pos + 7);
        }
        if (col < 7 && (getPiece(pos + 9).side == Side::white || (pos + 9 == enpassant && getPiece(pos + 9).side == Side::none))) {
            gen_addPawnMove(moves, pos, pos + 9);
        }
    } else {
        if (isEmpty(pos - 8)) {
            gen_addPawnMove(moves, pos, pos - 8);
        }
        if (pos >= 48 && isEmpty(pos - 8) && isEmpty(pos - 16)) {
            gen_addMove(moves, pos, pos - 16);
        }

        if (col < 7 && (getPiece(pos - 7).side == Side::black || (pos - 7 == enpassant && getPiece(pos - 7).side == Side::none)))
            gen_addPawnMove(moves, pos, pos - 7);
        if (col && (getPiece(pos - 9).side == Side::black || (pos - 9 == enpassant && getPiece(pos - 9).side == Side::none)))
            gen_addPawnMove(moves, pos, pos - 9);
    }
}

void ChessBoard::gen_castling(std::vector<MoveFull>& moves, int kingPos) const
{
    if ((kingPos != 4 || !castleRights[B]) && (kingPos != 60 || !castleRights[W])) {
        return;
    }

    if (kingPos == 4) {
        if ((castleRights[B] & CastleRight_long) &&
            pieces[1].isEmpty() && pieces[2].isEmpty() && pieces[3].isEmpty() &&
            !beAttacked(2, Side::white) && !beAttacked(3, Side::white) && !beAttacked(4, Side::white)) {
            assert(isPiece(0, static_cast<int>(PieceType::rook), Side::black));
            gen_addMove(moves, 4, 2);
        }
        if ((castleRights[B] & CastleRight_short) &&
            pieces[5].isEmpty() && pieces[6].isEmpty() &&
             !beAttacked(4, Side::white) && !beAttacked(5, Side::white) && !beAttacked(6, Side::white)) {
            assert(isPiece(7, static_cast<int>(PieceType::rook), Side::black));
            gen_addMove(moves, 4, 6);
        }
    } else {
        if ((castleRights[W] & CastleRight_long) &&
            pieces[57].isEmpty() && pieces[58].isEmpty() && pieces[59].isEmpty() &&
            !beAttacked(58, Side::black) && !beAttacked(59, Side::black) && !beAttacked(60, Side::black)) {
            assert(isPiece(56, static_cast<int>(PieceType::rook), Side::white));
            gen_addMove(moves, 60, 58);
        }
        if ((castleRights[W] & CastleRight_short) &&
            pieces[61].isEmpty() && pieces[62].isEmpty() &&
            !beAttacked(60, Side::black) && !beAttacked(61, Side::black) && !beAttacked(62, Side::black)) {
            assert(isPiece(63, static_cast<int>(PieceType::rook), Side::white));
            gen_addMove(moves, 60, 62);
        }
    }
}

bool ChessBoard::beAttacked(int pos, Side attackerSide) const
{
    int row = getRank(pos), col = getColumn(pos);
    /* Check attacking of Knight */
    if (col > 0 && row > 1 && isPiece(pos - 17, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    if (col < 7 && row > 1 && isPiece(pos - 15, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    if (col > 1 && row > 0 && isPiece(pos - 10, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    if (col < 6 && row > 0 && isPiece(pos - 6, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    if (col > 1 && row < 7 && isPiece(pos + 6, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    if (col < 6 && row < 7 && isPiece(pos + 10, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    if (col > 0 && row < 6 && isPiece(pos + 15, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    if (col < 7 && row < 6 && isPiece(pos + 17, static_cast<int>(PieceType::knight), attackerSide))
        return true;
    
    /* Check horizontal and vertical lines for attacking of Queen, Rook, King */
    /* go down */
    for (int y = pos + 8; y < 64; y += 8) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == ROOK ||
                    (piece.type == KING && y == pos + 8)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go up */
    for (int y = pos - 8; y >= 0; y -= 8) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == ROOK ||
                    (piece.type == KING && y == pos - 8)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go left */
    for (int y = pos - 1; y >= pos - col; y--) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == ROOK ||
                    (piece.type == KING && y == pos - 1)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go right */
    for (int y = pos + 1; y < pos - col + 8; y++) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == ROOK ||
                    (piece.type == KING && y == pos + 1)) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* Check diagonal lines for attacking of Queen, Bishop, King, Pawn */
    /* go right down */
    for (int y = pos + 9; y < 64 && getColumn(y) != 0; y += 9) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == BISHOP ||
                    (y == pos + 9 && (piece.type == KING || (piece.type == PAWN && piece.side == Side::white)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go left down */
    for (int y = pos + 7; y < 64 && getColumn(y) != 7; y += 7) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == BISHOP ||
                    (y == pos + 7 && (piece.type == KING || (piece.type == PAWN && piece.side == Side::white)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go left up */
    for (int y = pos - 9; y >= 0 && getColumn(y) != 7; y -= 9) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == BISHOP ||
                    (y == pos - 9 && (piece.type == KING || (piece.type == PAWN && piece.side == Side::black)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    /* go right up */
    for (int y = pos - 7; y >= 0 && getColumn(y) != 0; y -= 7) {
        auto piece = getPiece(y);
        if (!piece.isEmpty()) {
            if (piece.side == attackerSide) {
                if (piece.type == QUEEN || piece.type == BISHOP ||
                    (y == pos - 7 && (piece.type == KING || (piece.type == PAWN && piece.side == Side::black)))) {
                    return true;
                }
            }
            break;
        }
    }
    
    return false;
}

void ChessBoard::make(const MoveFull& move, Hist& hist)
{
    hist.enpassant = enpassant;
    hist.status = status;
    hist.castleRights[0] = castleRights[0];
    hist.castleRights[1] = castleRights[1];
    hist.castled = 0;
    hist.move = move;
    hist.cap = pieces[move.dest];
    hist.quietCnt = quietCnt;
    
    auto p = pieces[move.from];
    pieces[move.dest] = p;
    pieces[move.from].setEmpty();
    hist.move.piece = p;
    
    quietCnt++;
    enpassant = -1;
    
    if ((castleRights[0] + castleRights[1]) && hist.cap.type == static_cast<int>(PieceType::rook)) {
        clearCastleRights(move.dest, hist.cap.side);
    }
    
    switch (static_cast<PieceType>(p.type)) {
        case PieceType::king: {
            castleRights[static_cast<int>(p.side)] &= ~(CastleRight_long|CastleRight_short);
            if (abs(move.from - move.dest) == 2) { // castle
                auto rookPos = move.from + (move.from < move.dest ? 3 : -4);
                assert(pieces[rookPos].type == static_cast<int>(PieceType::rook));
                int newRookPos = (move.from + move.dest) / 2;
                assert(pieces[newRookPos].isEmpty());

                pieces[newRookPos] = pieces[rookPos];
                pieces[rookPos].setEmpty();
                quietCnt = 0;
                hist.castled = move.dest == 2 || move.dest == 56 + 2 ? CastleRight_long : CastleRight_short;
            }
            break;
        }
            
        case PieceType::rook: {
            if (castleRights[0] + castleRights[1]) {
                clearCastleRights(move.from, p.side);
            }
            break;
        }
            
        case PieceType::pawn: {
            int d = abs(move.from - move.dest);
            quietCnt = 0;

            if (d == 16) {
                enpassant = (move.from + move.dest) / 2;
            } else if (move.dest == hist.enpassant) {
                int ep = move.dest + (p.side == Side::white ? +8 : -8);
                hist.cap = pieces[ep];
                
                pieces[ep].setEmpty();
            } else {
                if (move.promotion != EMPTY) {
                    pieces[move.dest].type = move.promotion;
                }
            }
            break;
        }
        default:
            break;
    }
    
    if (!hist.cap.isEmpty()) {
        quietCnt = 0;
    }
    
    if (hist.castleRights[W] != castleRights[W]) {
        if ((hist.castleRights[W] & CastleRight_short) != (castleRights[W] & CastleRight_short)) {
            quietCnt = 0;
        }
        if ((hist.castleRights[W] & CastleRight_long) != (castleRights[W] & CastleRight_long)) {
            quietCnt = 0;
        }
    }
    if (hist.castleRights[B] != castleRights[B]) {
        if ((hist.castleRights[B] & CastleRight_short) != (castleRights[B] & CastleRight_short)) {
            quietCnt = 0;
        }
        if ((hist.castleRights[B] & CastleRight_long) != (castleRights[B] & CastleRight_long)) {
            quietCnt = 0;
        }
    }
    
    assert(hist.move.piece.idx == pieces[move.dest].idx);
            
    pieceList_make(hist);
}

void ChessBoard::takeBack(const Hist& hist)
{
    auto movep = pieces[hist.move.dest];
    pieces[hist.move.from] = movep;

    int capPos = hist.move.dest;

    if (movep.type == static_cast<int>(PieceType::pawn) && hist.enpassant == hist.move.dest) {
        capPos = hist.move.dest + (movep.side == Side::white ? +8 : -8);
        setEmpty(hist.move.dest);
    }
    pieces[capPos] = hist.cap;

    if (movep.type == static_cast<int>(PieceType::king)) {
        if (abs(hist.move.from - hist.move.dest) == 2) {
            assert(hist.castled == CastleRight_long || hist.castled == CastleRight_short);
            int rookPos = hist.move.from + (hist.move.from < hist.move.dest ? 3 : -4);
            assert(isEmpty(rookPos));

            int newRookPos = (hist.move.from + hist.move.dest) / 2;
            pieces[rookPos] = pieces[newRookPos];
            setEmpty(newRookPos);
        }
    }
    
    if (hist.move.promotion != EMPTY) {
        pieces[hist.move.from].type = static_cast<int>(PieceType::pawn);
    }
    
    status = hist.status;
    castleRights[0] = hist.castleRights[0];
    castleRights[1] = hist.castleRights[1];
    enpassant = hist.enpassant;
    quietCnt = hist.quietCnt;
    
    assert(hist.move.piece.idx == pieces[hist.move.from].idx);
    assert(hist.cap.isEmpty() || hist.cap.idx == pieces[capPos].idx);
    
    pieceList_takeback(hist);
}


std::string ChessBoard::chessPiece2String(const Piece& piece, bool alwayLowerCase)
{
    char ch = Funcs::chessPieceType2Char(piece.type);
    if (!alwayLowerCase && piece.side == Side::white) {
        ch += 'A' - 'a';
    }
    return std::string(1, ch);
}

std::string ChessBoard::piece2String(const Piece& piece, bool alwayLowerCase)
{
    return chessPiece2String(piece, alwayLowerCase);
}

char ChessBoard::pieceType2Char(int pieceType) const
{
    return Funcs::chessPieceType2Char(pieceType);
}

std::string ChessBoard::toString(const Piece& piece) const
{
    return chessPiece2String(piece, false);
}

std::string ChessBoard::moveString_coordinate(const Move& move)
{
    std::ostringstream stringStream;
    stringStream << Funcs::chessPosToCoordinateString(move.from) << Funcs::chessPosToCoordinateString(move.dest);
    if (move.promotion > KING && move.promotion < PAWN) {
        stringStream << chessPiece2String(Piece(move.promotion, Side::white), true);
    }
    return stringStream.str();
}

std::string ChessBoard::toString(const Move& move) const
{
    return moveString_coordinate(move);
}

std::string ChessBoard::toString(const MoveFull& move) const
{
    return toString(Move(move));
}

std::string ChessBoard::hist2String(const HistBasic& hist)
{
    return moveString_coordinate(Move(hist.move));
}


int ChessBoard::charactorToPieceType(char ch) const
{
    return Funcs::chessCharactorToPieceType(ch);
}

Move ChessBoard::moveFromString_castling(const std::string& str, Side side) const
{
    assert(str == "O-O" || str == "O-O+" || str == "0-0" || str == "O-O-O" || str == "O-O-O+" || str == "0-0-0");

    auto from = side == Side::black ? 4 : 60;
    auto dest = from + (str.length() < 5 ? 2 : -2);
    return Move(from, dest, EMPTY);
}


void ChessBoard::clone(const BoardCore* oboard)
{
    BoardCore::clone(oboard);
    assert(Funcs::isChessFamily(oboard->variant));
    auto ob = static_cast<const ChessBoard*>(oboard);
    enpassant = ob->enpassant;
    castleRights[0] = ob->castleRights[0];
    castleRights[1] = ob->castleRights[1];

    castleRights_column_king = ob->castleRights_column_king;
    castleRights_column_rook_left = ob->castleRights_column_rook_left;
    castleRights_column_rook_right = ob->castleRights_column_rook_right;
}


int ChessBoard::toPieceCount(int* pieceCnt) const
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

void ChessBoard::createFullMoves(std::vector<MoveFull>& moveList, MoveFull m) const
{
    moveList.push_back(m);
}

bool ChessBoard::pieceList_setPiece(int *pieceList, int pos, int type, Side side) {
    auto d = side == Side::white ? 16 : 0;
    
    auto k = type <= QUEEN ? 1 : type == PAWN ? 8 : 2;
    for (auto t = pieceListStartIdxByType[static_cast<int>(type)]; k > 0; t++, k--) {
        assert (t >= 0 && t < 16);
        if (pieceList[d + t] < 0 || pieceList[d + t] == pos) {
            pieceList[d + t] = pos;
            return true;
        }
    }
    
    if (type != PAWN) {
        for (auto t = pieceListStartIdxByType[PAWN]; t < 16; t++) {
            assert (t >= 0 && t < 16);
            if (pieceList[d + t] < 0 || pieceList[d + t] == pos) {
                pieceList[d + t] = pos;
                return true;
            }
        }

    }
    return false;
}

bool ChessBoard::pieceList_isValid() const {
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
            
            if (tp < PAWN && piece.type != tp) {
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

bool ChessBoard::pieceList_setEmpty(int *pieceList, int pos, int type, Side side) {
    auto k = type <= QUEEN ? 1 : type == PAWN ? 8 : 2;
    int d = side == Side::white ? 16 : 0;
    for (auto t = pieceListStartIdxByType[static_cast<int>(type)]; k > 0; t++, k--) {
        assert (t >= 0 && t < 16);
        if (pieceList[d + t] == pos) {
            pieceList[d + t] = -1;
            return true;
        }
    }
    
    if (type != PAWN) {
        for (auto t = pieceListStartIdxByType[PAWN]; t < 16; t++) {
            assert (t >= 0 && t < 16);
            if (pieceList[d + t] == pos) {
                pieceList[d + t] = -1;
                return true;
            }
        }
    }
    return false;
}


bool ChessBoard::pieceList_make(const Hist& hist) {
    if (!hist.cap.isEmpty()) {
        auto dest = hist.move.dest;
        if (hist.cap.type == PAWN && dest == hist.enpassant) {
            dest = dest + (hist.cap.side == Side::black ? +8 : -8);
        }
        for (auto t = pieceListStartIdxByType[static_cast<int>(hist.cap.type)], sd = static_cast<int>(hist.cap.side); ; t++) {
            assert (t >= 0 && t < 16);
            if (pieceList[sd][t] == dest) {
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
    assert(false);
    return false;
}

bool ChessBoard::pieceList_takeback(const Hist& hist) {
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
    
    auto dest = hist.move.dest;
    if (isEmpty(dest)) {
        assert(hist.move.piece.type == PAWN && hist.cap.type == PAWN);
        
        dest += (hist.cap.side == Side::black ? +8 : -8);
    }

    auto k = hist.cap.type == QUEEN ? 1 : hist.cap.type == PAWN ? 8 : 2;
    for (auto t = pieceListStartIdxByType[static_cast<int>(hist.cap.type)], sd = static_cast<int>(hist.cap.side); k > 0; t++, k--) {
        assert (t >= 0 && t < 16);

        if (pieceList[sd][t] < 0) {
            pieceList[sd][t] = dest;
            return true;
        }
    }
    for (auto t = pieceListStartIdxByType[PAWN], sd = static_cast<int>(hist.cap.side); ; t++) {
        assert (t >= 0 && t < 16);

        if (pieceList[sd][t] < 0) {
            pieceList[sd][t] = dest;
            return true;
        }
    }
    
    return false;
}


#endif // #ifdef _FELICITY_CHESS_
