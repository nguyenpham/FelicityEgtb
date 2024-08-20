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

#ifndef bs_xq_h
#define bs_xq_h

#include <stdio.h>
#include "../base/base.h"

#ifdef _FELICITY_XQ_

namespace bslib {

    class XqBoard : public BoardCore {
    protected:

    public:
        XqBoard(ChessVariant _variant = ChessVariant::xingqi);
        XqBoard(const XqBoard&);
        virtual ~XqBoard() override;

        virtual std::string toString() const override;
        virtual bool isValid() const override;
        virtual bool isLegal() const override;

        virtual void setFen(const std::string& fen) override;

        using BoardCore::getFen;
        virtual std::string getFen(bool enpassantLegal, int halfCount, int fullMoveCount) const override;

        using BoardCore::gen;
        virtual void gen(std::vector<MoveFull>& moveList, Side attackerSide) const override;
        virtual bool isIncheck(Side beingAttackedSide) const override;

        using BoardCore::make;
        using BoardCore::takeBack;

        virtual void make(const MoveFull& move, Hist& hist) override;
        virtual void takeBack(const Hist& hist) override;
        
        using BoardCore::toString;
        
        virtual char pieceType2Char(int pieceType) const override;
        virtual std::string piece2String(const Piece& piece, bool alwayLowerCase) const;

        virtual std::string toString(const Piece&) const override;
        virtual std::string toString(const Move&) const override;
        virtual std::string toString(const MoveFull&) const override;
                
        std::string moveString_coordinate(const Move& move) const;
        
        virtual int coordinateStringToPos(const std::string& str) const override;
        virtual std::string posToCoordinateString(int pos) const override;

        virtual void createFullMoves(std::vector<MoveFull>& moveList, MoveFull m) const override;
        
        using BoardCore::pieceList_isDraw;
        virtual bool pieceList_isDraw(const int *pieceList) const override;

        virtual Result rule() override;
        virtual Result ruleRepetition(int repeatLen);

    protected:
        virtual bool isValidPromotion(int promotion, Side) const override {
            return promotion > KING;
        }


    protected:
        void gen_addMove(std::vector<MoveFull>& moveList, int from, int dest) const;

        virtual bool pieceList_make(const Hist& hist) override;
        virtual bool pieceList_takeback(const Hist& hist) override;

        virtual bool pieceList_setEmpty(int *pieceList, int pos, PieceType type, Side side) override;
        virtual bool pieceList_setPiece(int *pieceList, int pos, PieceType type, Side side) override;
        virtual bool pieceList_isValid() const override;


    private:
        int toPieceCount(int* pieceCnt) const;
        
    };
    
} // namespace bslib

#endif // _FELICITY_XQ_

#endif


