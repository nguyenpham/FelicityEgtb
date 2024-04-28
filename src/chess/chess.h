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

#ifndef bs_chess_h
#define bs_chess_h

#ifdef _FELICITY_CHESS_

#include <stdio.h>
#include "../base/base.h"

namespace bslib {

    const int CastleRight_long  = (1<<0);
    const int CastleRight_short = (1<<1);
    const int CastleRight_mask  = (CastleRight_long|CastleRight_short);


    class ChessBoard : public BoardCore {
    protected:
        int enpassant = 0;
        int8_t castleRights[2];
        int castleRights_column_king = 4, castleRights_column_rook_left = 0, castleRights_column_rook_right = 7;

    public:
        static const std::string pieceTypeName; // = ".kqrbnp";
        
        ChessBoard(ChessVariant _variant = ChessVariant::standard);
        ChessBoard(const ChessBoard&);
        virtual ~ChessBoard() override;

        virtual std::string toString() const override;
        virtual bool isValid() const override;

        virtual int columnCount() const override;
        virtual int getColumn(int pos) const override;
        virtual int getRank(int pos) const override;

        virtual void setFen(const std::string& fen) override;

        using BoardCore::getFen;
        virtual std::string getFen(bool enpassantLegal, int halfCount, int fullMoveCount) const override;

        virtual void gen(std::vector<MoveFull>& moveList, Side attackerSide) const override;
        virtual bool isIncheck(Side beingAttackedSide) const override;

        using BoardCore::make;
        using BoardCore::takeBack;

        virtual void make(const MoveFull& move, Hist& hist) override;
        virtual void takeBack(const Hist& hist) override;

        virtual PieceType charactorToPieceType(char ch) const override;
        
        using BoardCore::toString;
        
        virtual char pieceType2Char(int pieceType) const override;
        virtual std::string piece2String(const Piece& piece, bool alwayLowerCase) const;

        virtual std::string toString(const Piece&) const override;
        virtual std::string toString(const Move&) const override;
        virtual std::string toString(const MoveFull&) const override;
                
        std::string moveString_coordinate(const Move& move) const;

        virtual int coordinateStringToPos(const std::string& str) const override;

        virtual std::string posToCoordinateString(int pos) const override;

        using BoardCore::flip;
        virtual int flip(int, FlipMode) const override {
            assert(0);
            return 0;
        }

        virtual void createFullMoves(std::vector<MoveFull>& moveList, MoveFull m) const override;
        
        virtual void clone(const BoardCore* oboard) override;

        int8_t getCastleRights(int sd) const {
            return castleRights[sd];
        }

        void setEnpassant(int _enpassant) {
            enpassant = _enpassant;
        }

        void setCastleRights(int sd, int8_t rights) {
            castleRights[sd] = rights;
        }

        int getEnpassant() const {
            return enpassant;
        }

        virtual Move moveFromString_castling(const std::string& str, Side side) const;


    protected:
        bool canRivalCaptureEnpassant() const;
        
        virtual bool isValidPromotion(int promotion, Side) const override {
            return promotion > KING && promotion < static_cast<int>(PieceType::pawn);
        }

        virtual bool beAttacked(int pos, Side attackerSide) const;

        virtual void genPawn(std::vector<MoveFull>& moves, Side side, int pos) const;
        virtual void genKnight(std::vector<MoveFull>& moves, Side side, int pos) const;
        virtual void genRook(std::vector<MoveFull>& moves, Side side, int pos, bool oneStep) const;
        virtual void genBishop(std::vector<MoveFull>& moves, Side side, int pos, bool oneStep) const;


        virtual void gen_castling(std::vector<MoveFull>& moveList, int kingPos) const;

        virtual bool isValidCastleRights() const;
        virtual void setFenCastleRights_clear();
        virtual void setFenCastleRights(const std::string& string);
        virtual std::string getFenCastleRights() const;
        virtual void clearCastleRights(int rookPos, Side rookSide);

    protected:
        void gen_addMove(std::vector<MoveFull>& moveList, int from, int dest) const;

        virtual bool pieceList_make(const Hist& hist) override;
        virtual bool pieceList_takeback(const Hist& hist) override;

        virtual bool pieceList_setEmpty(int *pieceList, int pos, PieceType type, Side side) override;
        virtual bool pieceList_setPiece(int *pieceList, int pos, PieceType type, Side side) override;
        virtual bool pieceList_isValid() const override;

    private:
        void checkEnpassant();
        
        int toPieceCount(int* pieceCnt) const;
        
    private:
        void gen_addPawnMove(std::vector<MoveFull>& moveList, int from, int dest) const;
    };
    
    extern const char* pieceTypeFullNames[8];

} // namespace bslib

#endif // #ifdef _FELICITY_CHESS_

#endif /* bs_chess_h */


