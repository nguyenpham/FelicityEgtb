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

#ifdef _FELICITY_XQ_
void BoardCore::setupPieceIndexes()
{
    int idxs[] = { 0, 0};
    for(auto && piece : pieces) {
        if (piece.isEmpty()) continue;
        piece.idx = idxs[static_cast<int>(piece.side)]++;
    }
}
#endif

void BoardCore::setFenComplete()
{
#ifdef _FELICITY_XQ_
    setupPieceIndexes();
#endif

    setupPieceCount();

#ifdef _FELICITY_USE_HASH_
    setupHashKey();
    assert(isHashKeyValid());
#endif
    

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
    assert(kingpos >= 0 && kingpos < BOARD_SZ && pieces[kingpos] == Piece(PieceType::king, side));
    return kingpos;
}

#ifdef _FELICITY_USE_HASH_
extern uint64_t zobristWhite;
#endif

void BoardCore::flipSide()
{
    side = xSide(side);

#ifdef _FELICITY_USE_HASH_
    hashKey ^= zobristWhite;
    assert(isHashKeyValid());
#endif
}

void BoardCore::make(const MoveFull& move)
{
    Hist hist;
    make(move, hist);
    histList.push_back(hist);
    
    flipSide();
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
            pieceList_reset((int *)pieceList);
            auto mr = rankCount();
#ifdef _FELICITY_CHESS_
            auto halfc = 4;
#else
            auto halfc = 5;
#endif
            for(auto r = 0; r < mr; r++) {
                auto pos = r * columnCount();
                for(auto f = 0; f < halfc; f++) {
                    auto pos0 = pos + f, pos1 = pos + columnCount() - 1 - f;
                    auto piece0 = pieces[pos0];
                    auto piece1 = pieces[pos1];
                    
                    if (piece0.isEmpty() && piece1.isEmpty()) {
                        continue;
                    }
//                    if (piece0.type == PieceType::king || piece1.type == PieceType::king) {
//                        printOut();
//                    }
                    setPiece(pos1, piece0);

                    if (pos0 != pos1) {
                        setPiece(pos0, piece1);
                    }
                }
            }
            assert(pieceList[0][0] >= 0 && pieceList[1][0] >= 0);
            return;
        }

        case FlipMode::vertical:
        case FlipMode::rotate: {
            pieceList_reset((int *)pieceList);
            auto halfsz = size() / 2;
            auto mr = size() / columnCount();
            for(auto r0 = 0; r0 < halfsz; r0++) {
                auto r1 = flipMode == FlipMode::vertical ? (mr - 1 - r0 / columnCount()) * columnCount() + r0 % columnCount() : size() - 1 - r0;

                auto piece0 = pieces[r0];
                auto piece1 = pieces[r1];

#ifdef _FELICITY_XQ_
                if (!piece0.isEmpty()) {
                    piece0.side = xSide(piece0.side);
                }
                if (!piece1.isEmpty()) {
                    piece1.side = xSide(piece1.side);
                }
#endif

                setPiece(r0, piece1);
                setPiece(r1, piece0);

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
        std::cout << "\nPerft depth   : " << depth
                  << "\nNodes         : " << nodes
                  << "\nElapsed (ms)  : " << elapsed
                  << "\nNodes/second  : " << nodes * 1000 / (elapsed + 1)
//                  << "\nincheckCnt    : " << incheckCnt
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
    
/// Check mate, 50 moves rule
Result BoardCore::rule()
{
    Result result;

    /// 50 moves
    if (quietCnt >= 50 * 2) {
        result.result = GameResultType::draw;
        result.reason = ReasonType::fiftymoves;
        return result;
    }

    /// Mated or stalemate
    auto haveLegalMove = false;

#ifdef _FELICITY_USE_HASH_
    assert(isHashKeyValid());
#endif
    
    for(auto && move : gen(side)) {
        Hist hist;
        make(move, hist);
        haveLegalMove = !isIncheck(side);
        takeBack(hist);
        if (haveLegalMove) break;
    }

    if (!haveLegalMove) {
        if (isIncheck(side)) {
            result.result = side == Side::white ? GameResultType::loss : GameResultType::win;
            result.reason = ReasonType::mate;
        } else {
#if _FELICITY_CHESS_
            result.result = GameResultType::draw;
#else
            result.result = side == Side::white ? GameResultType::loss : GameResultType::win;
#endif
            result.reason = ReasonType::stalemate;
        }
//        return result;
    }

//    if (quietCnt < repetitionThreatHold * 4) {
//        return result;
//    }
//
//    auto cnt = 0;
//    auto i = int(histList.size()), k = i - quietCnt;
//    for(i -= 2; i >= 0 && i >= k; i -= 2) {
//        auto hist = histList.at(i);
//        if (hist.hashKey == hashKey) {
//            cnt++;
//            if (cnt >= repetitionThreatHold) {
//                auto repeatLen = int(histList.size()) - i;
//                //result = XqChaseJudge::evaluate(*this, repeatLen);
//                return ruleRepetition(repeatLen);
//            }
//        }
//    }

    return result;
}


#ifdef _FELICITY_USE_HASH_
uint64_t zobristWhite = 0, zobristTable[16][BOARD_SZ];

uint64_t rand64() {
    const uint64_t z = 0x9FB21C651E98DF25;
    static uint64_t state = 1;

    auto n = state;
    state++;

    n ^= ((n << 49) | (n >> 15)) ^ ((n << 24) | (n >> 40));
    n *= z;
    n ^= n >> 35;
    n *= z;
    n ^= n >> 28;

    return n;

}

uint64_t BoardData::xorHashKey(int pos) const
{
    assert(isPositionValid(pos));
    auto piece = pieces[pos]; assert(!piece.isEmpty());
    
    auto p = static_cast<int>(piece.type)
    + static_cast<int>(piece.side) * static_cast<int>(PieceType::pawn);
    return zobristTable[p][pos];
}

uint64_t BoardData::initHashKey() const {
    if (zobristWhite == 0) {
        zobristWhite = rand64();
        
        for(auto p = 0; p < 16; p++) {
            for(auto i = 0; i < BOARD_SZ; i++) {
                zobristTable[p][i] = rand64();
            }
        }
    }
    
    uint64_t key = 0;
    for(int i = 0; i < BOARD_SZ; i++) {
        if (!pieces[i].isEmpty()) {
            key ^= xorHashKey(i);
        }
    }

    if (side == Side::white) {
        key ^= zobristWhite;
    }
    return key;
}

bool BoardData::isHashKeyValid() {
    auto hk = initHashKey();
    if (hashKey == hk) {
        return true;
    }
    std::cout << "BoardCore::isHashKeyValid, hashKey:" << hashKey << ", hk: " << hk << std::endl;
    return false;
}

#endif
