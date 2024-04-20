/*
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

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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
    auto k = std::max<int>(fullMoveCnt, (histList.size() + 1) / 2);
    return getFen(enpassantLegal, quietCnt / 2, k);
}


MoveFull BoardCore::createFullMove(int from, int dest, int promotion) const
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

void BoardCore::clear()
{
    for(auto && piece : pieces) {
        piece.setEmpty();
    }
}

int BoardCore::attackerCnt() const
{
    auto cnt = 0;
    for(auto && piece : pieces) {
        if (!piece.isEmpty() && piece.type != PieceType::king) cnt++;
    }
    return cnt;
}

bool BoardCore::isLegalMove(int from, int dest, int promotion)
{
    auto piece = getPiece(from), cap = getPiece(dest);

    if (piece.isEmpty()
        || piece.side != side
        || (piece.side == cap.side && (variant != ChessVariant::chess960 || piece.type != PieceType::king || cap.type != PieceType::rook))
        || (promotion > KING && !Move::isValidPromotion(promotion))) {
        return false;
    }
    
    std::vector<MoveFull> moveList;
    genLegal(moveList, piece.side, from, dest, promotion);
    return !moveList.empty();
}

void BoardCore::genLegalOnly(std::vector<MoveFull>& moveList, Side attackerSide)
{
    gen(moveList, attackerSide);
    
    std::vector<MoveFull> moves;
    Hist hist;
    for (auto && move : moveList) {
        make(move, hist);
        if (!isIncheck(attackerSide)) {
            moves.push_back(move);
        }
        takeBack(hist);
    }
    moveList = moves;
}

void BoardCore::genLegal(std::vector<MoveFull>& moves, Side side, int from, int dest, int promotion)
{
    std::vector<MoveFull> moveList;
    gen(moveList, side);
    
    Hist hist;

    auto isChess = Funcs::isChessFamily(variant);

    for (auto && move : moveList) {
        
        if ((from >= 0 && move.from != from)
            || (dest >= 0 && move.dest != dest)
            || (isChess && promotion > KING && promotion != move.promotion)
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
    m.from = int8_t(flip(m.from, flipMode));
    m.dest = int8_t(flip(m.dest, flipMode));
    return m;
}

MoveFull BoardCore::flip(const MoveFull& move, FlipMode flipMode) const
{
    auto m = move;
    m.from = int8_t(flip(m.from, flipMode));
    m.dest = int8_t(flip(m.dest, flipMode));
    return m;
}

static const FlipMode flipflip_h[] = { FlipMode::horizontal, FlipMode::none, FlipMode::rotate, FlipMode::vertical };
static const FlipMode flipflip_v[] = { FlipMode::vertical, FlipMode::rotate, FlipMode::none, FlipMode::horizontal };
static const FlipMode flipflip_r[] = { FlipMode::rotate, FlipMode::vertical, FlipMode::horizontal, FlipMode::none };

FlipMode BoardCore::flip(FlipMode oMode, FlipMode flipMode)
{
    switch (flipMode) {
        case FlipMode::none:
            break;
        case FlipMode::horizontal:
            return flipflip_h[static_cast<int>(oMode)];
            
        case FlipMode::vertical:
            return flipflip_v[static_cast<int>(oMode)];
        case FlipMode::rotate:
            return flipflip_r[static_cast<int>(oMode)];
    }
    return oMode;
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
            for(int r = 0; r < mr; r++) {
                auto pos = r * columnCount();
                for(int f = 0; f < halfc; f++) {
                    std::swap(pieces[pos + f], pieces[pos + columnCount() - 1 - f]);
                }
            }
            return;
        }
            
        case FlipMode::vertical:
        case FlipMode::rotate: {
            auto halfsz = size() / 2;
            auto mr = size() / columnCount();
            for(int r0 = 0; r0 < halfsz; r0++) {
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

    std::vector<MoveFull> moveList;
    gen(moveList, side);

    Hist hist;
    auto theSide = side;
    for (auto && move : moveList) {
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
    for(int i = 0; i < 16; i++) {
        pieceList[i] = pieceList[16 + i] = -1;
    }
}

bool BoardCore::pieceList_setEmpty(int *pieceList, int pos) {
    for (int sd = 0; sd < 2; sd ++) {
        if (pieceList_setEmpty(pieceList, pos, sd)) {
            return true;
        }
    }
    return false;
}

bool BoardCore::pieceList_setEmpty(int *pieceList, int pos, int sd) {
    int d = sd == 0 ? 0 : 16;
    for(int i = 0; i < 16; i++) {
        if (pieceList[i + d] == pos) {
            pieceList[i + d] = -1;
            return true;
        }
    }

    return false;
}

bool BoardCore::pieceList_isThereAttacker(const int *pieceList) {
    for(int i = 5; i < 16; i ++) {
        if (pieceList[i] >= 0 || pieceList[i + 16] >= 0) return true;
    }
    return false;
}

    
    
