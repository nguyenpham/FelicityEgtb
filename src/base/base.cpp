/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <chrono>
#include <map>
#include <sstream>
#include <iostream>

#include "base.h"

#include "funcs.h"

using namespace bslib;


Piece Piece::emptyPiece(0, Side::none);
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

std::string BoardCore::getUciPositionString(const Move& pondermove) const
{
    std::string str = "position " + (fromOriginPosition() ? "startpos" : ("fen " + getStartingFen()));

    if (!histList.empty()) {
        str += " moves";
        for(auto && hist : histList) {
            str += " " + toString_coordinate(hist.move);
        }
    }

    if (pondermove.isValid()) {
        if (histList.empty()) {
            str += " moves";
        }
        str += " " + toString(pondermove);
    }
    return str;
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

void BoardCore::setupPieceIndexes()
{
    int idxs[] = { 0, 0};
    for(auto && piece : pieces) {
        if (piece.isEmpty()) continue;
        piece.idx = idxs[static_cast<int>(piece.side)]++;
    }
}

bool BoardCore::equalMoveLists(const BoardCore* oBoard, bool embeded) const
{
    assert(oBoard && startFen == oBoard->startFen);

    auto n0 = histList.size(), n1 = oBoard->histList.size();
    if (n0 != n1 && !embeded) {
        return false;
    }

    auto n = std::min(n0, n1);
    for(size_t i = 0; i < n; ++i) {
        if (histList.at(i).move != oBoard->histList.at(i).move) {
            return false;
        }
    }

    return true;
}

int BoardCore::attackerCnt() const
{
    auto cnt = 0;
    for(auto && piece : pieces) {
        if (!piece.isEmpty() && piece.type != KING) cnt++;
    }
    return cnt;
}

bool BoardCore::isLegalMove(int from, int dest, int promotion)
{
    auto piece = getPiece(from), cap = getPiece(dest);

    if (piece.isEmpty()
        || piece.side != side
        || (piece.side == cap.side && (variant != ChessVariant::chess960 || piece.type != KING || cap.type != static_cast<int>(PieceType::rook)))
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
    assert(kingpos >= 0 && kingpos < pieces.size() && pieces[kingpos] == Piece(KING, side));
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

extern int incheckCnt;

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

    
    
