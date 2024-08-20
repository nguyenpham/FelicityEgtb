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


#ifndef bs_base_h
#define bs_base_h

#include <stdio.h>
#include <set>
#include <unordered_map>

#include <iomanip> // for setfill, setw

#include "types.h"

namespace bslib {
    
    ///////////////////////////////////

    class BoardData {
    public:
        Side side;
        int quietCnt, fullMoveCnt = 1;
        int pieceList[2][16];

    protected:
        Piece pieces[BOARD_SZ];
        
        void cloneData(const BoardData* oboard) {
            *this = *oboard;
        }
        
    public:
        virtual bool isPositionValid(int pos) const {
            return pos >= 0 && pos < BOARD_SZ;
        }

#ifdef _FELICITY_CHESS_
    public:
        int enpassant = -1;
        int8_t castleRights[2];
#endif
        

#ifdef _FELICITY_USE_HASH_
    public:
        virtual uint64_t key() const {
            return hashKey;
        }
        void setHashKey(uint64_t key) {
            hashKey = key;
        }
        void setupHashKey() {
            hashKey = initHashKey();
        }

        uint64_t xorHashKey(int pos) const;

        virtual uint64_t initHashKey() const;
        bool isHashKeyValid();

    protected:
        uint64_t hashKey;
#endif



    };

    class BoardCore : public BoardData {
    public:
        ChessVariant variant;
        std::string startFen;

//    protected:
        std::vector<Hist> histList;

    public:

        int getHistListSize() const {
            return static_cast<int>(histList.size());
        }

        void histListClear() {
            histList.clear();
        }

        Hist getLastHist() const {
            return getHistAt(static_cast<int>(histList.size()) - 1);
        }

        Hist getHistAt(int idx) const {
            return idx >= 0 && idx < static_cast<int>(histList.size()) ? histList.at(idx) : Hist();
        }

        Hist* getLastHistPointer() {
            return getHistPointerAt(static_cast<int>(histList.size()) - 1);
        }
 
        const Hist* getLastHistPointer() const {
            return getHistPointerAt(static_cast<int>(histList.size()) - 1);
        }

        Hist* getHistPointerAt(int idx) {
            return idx >= 0 && idx < static_cast<int>(histList.size()) ? &histList[idx] : nullptr;
        }
        
        const Hist* getHistPointerAt(int idx) const {
            return idx >= 0 && idx < static_cast<int>(histList.size()) ? &histList[idx] : nullptr;
        }

        MoveFull getMoveAt(int idx) const {
            return idx >= 0 && idx < static_cast<int>(histList.size()) ? histList.at(idx).move : MoveFull();
        }
        
        static int size() {
            return BOARD_SZ;
        }
        static int columnCount() {
            return COLUMN_COUNT;
        }
        static int rankCount() {
            return ROW_COUNT;
        }

        static int getColumn(int pos) {
            assert(pos >= 0 && pos < BOARD_SZ);
            return pos % COLUMN_COUNT;
        }

        static int getRow(int pos) {
            assert(pos >= 0 && pos < BOARD_SZ);
            return pos / COLUMN_COUNT;
        }

        Piece getPiece(int pos) const {
            assert(isPositionValid(pos));
            return pieces[pos];
        }

        bool isEmpty(int pos) const {
            assert(isPositionValid(pos));
            return pieces[size_t(pos)].type == PieceType::empty;
        }
        bool isEmpty() const {
            for(auto && piece : pieces) {
                if (!piece.isEmpty()) return false;
            }
            return true;
        }

        bool isPiece(int pos, PieceType type, Side side) const {
            assert(isPositionValid(pos));
            auto p = pieces[size_t(pos)];
            return p.type == type && p.side == side;
        }

        virtual void clone(const BoardCore* oboard) {
            assert(variant == oboard->variant);
            cloneData(oboard);
            
            histList = oboard->histList;
            startFen = oboard->startFen;
        }

        int getTotalPieceCount() const {
            auto cnt = 0;
            for(auto && p : pieces) {
                if (!p.isEmpty()) cnt++;
            }
            return cnt;
        }
        
    public:
        BoardCore() {}
        virtual ~BoardCore() {}

        virtual void reset() {
            for (auto && p : pieces) {
                p.setEmpty();
            }

            histList.clear();
            quietCnt = 0;
            side = Side::none;
            pieceList_reset((int *)pieceList);
        }

        
        virtual std::string toString() const = 0;
        void printOut(const std::string& = "") const;

        bool isValid(const Move& move) const {
            return move.isValid() && isPositionValid(move.from) && isPositionValid(move.dest);
        }

        bool isValid(const MoveFull& move) const {
            Move m = move;
            return isValid(m);
        }

//        virtual int columnCount() const = 0;
//        virtual int rankCount() const = 0;

//        virtual int getColumn(int pos) const = 0;
//        virtual int getRank(int pos) const = 0;

        virtual bool isValid() const {
            return false;
        }

        void setPiece_(int pos, Piece piece) {
            assert(isPositionValid(pos));
            pieces[size_t(pos)] = piece;
        }

        void setPiece(int pos, Piece piece) {
            assert(isPositionValid(pos));
            pieces[size_t(pos)] = piece;
            pieceList_setPiece((int *)pieceList, pos, piece.type, piece.side);
        }

        void setEmpty_(int pos) {
            assert(isPositionValid(pos));
            pieces[size_t(pos)].setEmpty();
        }
        void setEmpty(int pos) {
            setPiece(pos, Piece::emptyPiece);
        }

        static Side xSide(Side side) {
            return side == Side::white ? Side::black : Side::white;
        }
        
        virtual std::string toString(const Piece&) const = 0;

        virtual std::string toString(const MoveFull&) const = 0;
        virtual std::string toString(const Move&) const = 0;
        virtual std::string toString(const Hist& hist) const;
        virtual std::string toString_coordinate(const MoveFull&) const;

        Move flip(const Move& move, FlipMode flipMode) const;
        MoveFull flip(const MoveFull& move, FlipMode flipMode) const;

//        virtual int flip(int pos, FlipMode flipMode) const;
        virtual void flip(FlipMode flipMode);
        virtual void flipPieceColors();

        virtual void createFullMoves(std::vector<MoveFull>& moveList, MoveFull m) const = 0;

        virtual int attackerCnt() const;
        virtual bool hasAttackers() const;
        virtual void setupPieceCount() {}

#ifdef _FELICITY_XQ_
        void setupPieceIndexes();
#endif
        

        virtual bool isValidPromotion(int promotion, Side side) const = 0;

        int getQuietCnt() const { return quietCnt; }
        void setQuietCnt(int k) { quietCnt = k; }

        int getFullMoveCnt() const { return fullMoveCnt; }
        void setFullMoveCnt(int k) { fullMoveCnt = k; }

        virtual bool sameContent(BoardCore*) const;

        uint64_t perft(int depth, int ply = 0);

    public:
        virtual void setFen(const std::string& fen) = 0;
        virtual void setFenComplete();

        virtual std::string getFen(bool enpassantLegal = false) const;
        virtual std::string getFen(bool enpassantLegal, int halfCount, int fullMoveCount) const = 0;
        
    public:
        bool fromOriginPosition() const;
        virtual std::string getStartingFen() const;
        
        void newGame(std::string fen = "");

        MoveFull createFullMove(int from, int dest, PieceType promote) const;
        virtual bool isLegalMove(int from, int dest, PieceType promotion = PieceType::empty);
        
        virtual bool isLegal() const = 0;

        void genLegalOnly(std::vector<MoveFull>& moveList, Side attackerSide);
        std::vector<MoveFull> genLegalOnly(Side attackerSide) {
            std::vector<MoveFull> moveList;
            genLegalOnly(moveList, attackerSide);
            return moveList;
        }

        
        void genLegal(std::vector<MoveFull>& moves, Side side, int from = -1, int dest = -1, PieceType promotion = PieceType::empty);
        virtual bool isIncheck(Side beingAttackedSide) const = 0;
        virtual void gen(std::vector<MoveFull>& moveList, Side attackerSide) const = 0;

        std::vector<MoveFull> gen(Side attackerSide) const {
            std::vector<MoveFull> moveList;
            gen(moveList, attackerSide);
            return moveList;
        }
        
        virtual char pieceType2Char(int pieceType) const = 0;
        virtual int coordinateStringToPos(const std::string& str) const = 0;

        virtual std::string posToCoordinateString(int pos) const = 0;

        virtual void make(const MoveFull& move);
        virtual void takeBack();

        virtual void make(const MoveFull& move, Hist& hist) = 0;
        virtual void takeBack(const Hist& hist) = 0;

        virtual int findKing(Side side) const;

        virtual Result rule();

    protected:
        mutable int incheckCnt = 0;

    public:
        static void pieceList_reset(int *pieceList);
        bool pieceList_isDraw() const;
        bool pieceList_isThereAttacker() const;

    protected:
            
        static bool pieceList_setEmpty(int *pieceList, int pos);
        static bool pieceList_setEmpty(int *pieceList, int pos, int sd);
        static bool pieceList_isThereAttacker(const int *pieceList);

        virtual bool pieceList_make(const Hist& hist) = 0;
        virtual bool pieceList_takeback(const Hist& hist) = 0;

        virtual bool pieceList_setEmpty(int *pieceList, int pos, PieceType type, Side side) = 0;
        virtual bool pieceList_setPiece(int *pieceList, int pos, PieceType type, Side side) = 0;
        virtual bool pieceList_isValid() const = 0;
        virtual bool pieceList_isDraw(const int *pieceList) const = 0;

    };
    

} // namespace bslib

#endif /* bs_base_h */


