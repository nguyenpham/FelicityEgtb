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

#include <chrono>
#include <map>
#include <sstream>
#include <iostream>

#include "base.h"

#include "funcs.h"

using namespace bslib;


Piece Piece::emptyPiece(PieceType::empty, Side::none);
Move Move::illegalMove(-1, -1);
MoveFull MoveFull::illegalMove(-1, -1);


bool BoardCore::fromOriginPosition() const
{
    return startFen.empty();
}

std::string BoardCore::getStartingFen() const
{
    return startFen;
}

std::string BoardCore::getFen(bool enpassantLegal) const
{
    auto k = std::max<int>(fullMoveCnt, (int)(histList.size() + 1) / 2);
    return getFen(enpassantLegal, quietCnt / 2, k);
}


MoveFull BoardCore::createFullMove(int from, int dest, PieceType promotion) const
{
    MoveFull move(from, dest, promotion);
    if (isPositionValid(from)) {
        move.piece = getPiece(from);
    }
    return move;
}

void BoardCore::newGame(std::string fen)
{
    histList.clear();
    setFen(fen);
}

int BoardCore::attackerCnt() const
{
    auto cnt = 0;
    
#ifdef _FELICITY_CHESS_
    auto fromPiece = PieceType::queen;
#else
    auto fromPiece = PieceType::rook;
#endif
    
    for(auto && piece : pieces) {
        if (piece.type >= fromPiece) cnt++;
    }
    return cnt;
}

bool BoardCore::hasAttackers() const
{
#ifdef _FELICITY_CHESS_
    auto i = 1; /// from Queen - ignored King
#else
    auto i = 5; /// from Rook, ignored King, 2 Avisors, 2 Elephants
#endif

    for (; i < 16; ++i) {
        if (pieceList[0][i] >= 0 || pieceList[1][i] >= 0) {
            return true;
        }
    }
    return false;
}


bool BoardCore::isLegalMove(int from, int dest, PieceType promotion)
{
    auto piece = getPiece(from), cap = getPiece(dest);

    if (piece.isEmpty()
        || piece.side != side
        || (piece.side == cap.side && (variant != ChessVariant::chess960 || piece.type != PieceType::king || cap.type != PieceType::rook))
        || (promotion > PieceType::king && !Move::isValidPromotion(promotion))) {
        return false;
    }
    
    std::vector<MoveFull> moveList;
    genLegal(moveList, piece.side, from, dest, promotion);
    return !moveList.empty();
}

void BoardCore::genLegalOnly(std::vector<MoveFull>& moveList, Side attackerSide)
{
    Hist hist;
    for (auto && move : gen(attackerSide)) {
        make(move, hist);
        if (!isIncheck(attackerSide)) {
            moveList.push_back(move);
        }
        takeBack(hist);
    }
}

void BoardCore::genLegal(std::vector<MoveFull>& moves, Side side, int from, int dest, PieceType promotion)
{
    Hist hist;

    auto isChess = Funcs::isChessFamily(variant);

    for (auto && move : gen(side)) {
        
        if ((from >= 0 && move.from != from)
            || (dest >= 0 && move.dest != dest)
            || (isChess && promotion > PieceType::king && promotion != move.promotion)
            ) {
            continue;
        }
        
        make(move, hist);
        if (!isIncheck(side)) {
            moves.push_back(move);
        }
        takeBack(hist);
    }
}

int BoardCore::findKing(Side side) const
{
    auto sd = static_cast<int>(side);
    auto kingpos = pieceList[sd][0];
    assert(kingpos >= 0 && kingpos < pieces.size() && pieces[kingpos] == Piece(PieceType::king, side));
    return kingpos;
}

void BoardCore::make(const MoveFull& move)
{
    Hist hist;
    make(move, hist);
    histList.push_back(hist);
    side = xSide(side);
}

void BoardCore::takeBack()
{
    if (!histList.empty()) {
        auto hist = histList.back();
        histList.pop_back();
        takeBack(hist);
        side = xSide(side);
    }
}

Move BoardCore::flip(const Move& move, FlipMode flipMode) const
{
    auto m = move;
    m.from = int8_t(Funcs::flip(m.from, flipMode));
    m.dest = int8_t(Funcs::flip(m.dest, flipMode));
    return m;
}

MoveFull BoardCore::flip(const MoveFull& move, FlipMode flipMode) const
{
    auto m = move;
    m.from = int8_t(Funcs::flip(m.from, flipMode));
    m.dest = int8_t(Funcs::flip(m.dest, flipMode));
    return m;
}

void BoardCore::printOut(const std::string& msg) const
{
    if (!msg.empty()) {
        std::cout << msg << std::endl;
    }
    std::cout << toString() << std::endl;
}

std::string BoardCore::toString(const Hist& hist) const
{
    return toString_coordinate(hist.move);
}

std::string BoardCore::toString_coordinate(const MoveFull& move) const
{
    return toString(move);
}


void BoardCore::flip(FlipMode flipMode)
{
    switch (flipMode) {
        case FlipMode::none:
            return;
        case FlipMode::horizontal: {
            auto mr = size() / columnCount();
            auto halfc = columnCount() / 2;
            for(auto r = 0; r < mr; r++) {
                auto pos = r * columnCount();
                for(auto f = 0; f < halfc; f++) {
                    std::swap(pieces[pos + f], pieces[pos + columnCount() - 1 - f]);
                }
            }
            return;
        }
            
        case FlipMode::vertical:
        case FlipMode::rotate: {
            auto halfsz = size() / 2;
            auto mr = size() / columnCount();
            for(auto r0 = 0; r0 < halfsz; r0++) {
                auto r1 = flipMode == FlipMode::vertical ? (mr - 1 - r0 / columnCount()) * columnCount() + r0 % columnCount() : size() - 1 - r0;
                std::swap(pieces[r0], pieces[r1]);
                if (!pieces[r0].isEmpty()) {
                    pieces[r0].side = xSide(pieces[r0].side);
                }
                if (!pieces[r1].isEmpty()) {
                    pieces[r1].side = xSide(pieces[r1].side);
                }
            }
            
            side = xSide(side);
            setupPieceCount();
            return;
        }
        default:
            assert(false);
    }
}

void BoardCore::flipPieceColors()
{
    for(auto && piece : pieces) {
        if (piece.isEmpty()) {
            continue;
        }

        piece.side = xSide(piece.side);
    }

    side = xSide(side);
    setupPieceCount();
}

bool BoardCore::sameContent(BoardCore* board) const
{
    auto same = false;
    if (board &&
            variant == board->variant &&
            histList.size() == board->histList.size() &&
            getStartingFen() == board->getStartingFen()) {
        same = true;
        for(size_t i = 0; i < board->histList.size(); i++) {
            auto hist0 = histList.at(i);
            auto hist1 = board->histList.at(i);
            if (hist0.move != hist1.move) {
                same = false;
                break;
            }
        }
    }
    return same;
}


uint64_t BoardCore::perft(int depth, int ply)
{
    if (depth == 0) return 1;

    uint64_t nodes = 0;

    std::chrono::milliseconds::rep start = 0;

    if (ply == 0) {
        start = Funcs::now();
    }

    Hist hist;
    auto theSide = side;
    for(auto move : gen(side)) {
        make(move);
        if (!isIncheck(theSide)) {
            auto n = perft(depth - 1, ply + 1);
            nodes += n;

            if (ply == 0) {
                std::cout << toString_coordinate(move) << ": " << n << std::endl;
            }
        }
        takeBack();
    }

    if (ply == 0) {
        auto elapsed = Funcs::now() - start;
        std::cout << "\nDepth         : " << depth
                  << "\nNodes         : " << nodes
                  << "\nElapsed (ms)  : " << elapsed
                  << "\nNodes/second  : " << nodes * 1000 / (elapsed + 1)
                  << "\nincheckCnt    : " << incheckCnt
                  << std::endl;
    }
    return nodes;
}


void BoardCore::pieceList_reset(int *pieceList) {
    for(auto i = 0; i < 16; i++) {
        pieceList[i] = pieceList[16 + i] = -1;
    }
}

bool BoardCore::pieceList_setEmpty(int *pieceList, int pos) {
    for (auto sd = 0; sd < 2; sd ++) {
        if (pieceList_setEmpty(pieceList, pos, sd)) {
            return true;
        }
    }
    return false;
}

bool BoardCore::pieceList_setEmpty(int *pieceList, int pos, int sd) {
    int d = sd == 0 ? 0 : 16;
    for(auto i = 0; i < 16; i++) {
        if (pieceList[i + d] == pos) {
            pieceList[i + d] = -1;
            return true;
        }
    }

    return false;
}

bool BoardCore::pieceList_isThereAttacker(const int *pieceList) {
#ifdef _FELICITY_CHESS_
    auto from = 1;   /// ignored King
#else
    auto from = 5;   /// ignored King, Advisors, Elephants
#endif
    
    for(auto i = from; i < 16; i ++) {
        if (pieceList[i] >= 0 || pieceList[i + 16] >= 0) return true;
    }
    return false;
}

bool BoardCore::pieceList_isThereAttacker() const
{
    return pieceList_isThereAttacker((const int*) pieceList);
}

bool BoardCore::pieceList_isDraw() const
{
    return pieceList_isDraw((const int*) pieceList);
}
    
