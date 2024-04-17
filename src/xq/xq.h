/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifndef bs_chess_h
#define bs_chess_h

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

        virtual int charactorToPieceType(char ch) const override;
        
        using BoardCore::toString;
        
        virtual char pieceType2Char(int pieceType) const override;
        virtual std::string piece2String(const Piece& piece, bool alwayLowerCase);

        static std::string chessPiece2String(const Piece& piece, bool alwayLowerCase);
        virtual std::string toString(const Piece&) const override;
        virtual std::string toString(const Move&) const override;
        virtual std::string toString(const MoveFull&) const override;
                
        static std::string moveString_coordinate(const Move& move);
        static std::string hist2String(const HistBasic&);

        virtual std::string posToCoordinateString(int pos) const override;

        using BoardCore::flip;
        virtual int flip(int, FlipMode) const override {
            assert(0);
            return 0;
        }

        virtual void createFullMoves(std::vector<MoveFull>& moveList, MoveFull m) const override;
        
        virtual void clone(const BoardCore* oboard) override;


        static void staticInit();

    protected:
        virtual bool isValidPromotion(int promotion, Side) const override {
            return promotion > KING;
        }


    protected:
        void gen_addMove(std::vector<MoveFull>& moveList, int from, int dest) const;

        virtual bool pieceList_make(const Hist& hist) override;
        virtual bool pieceList_takeback(const Hist& hist) override;

        virtual bool pieceList_setEmpty(int *pieceList, int pos, int type, Side side) override;
        virtual bool pieceList_setPiece(int *pieceList, int pos, int type, Side side) override;
        virtual bool pieceList_isValid() const override;

    private:
        int toPieceCount(int* pieceCnt) const;
        
    };
    
//    extern const char* pieceTypeFullNames[8];

} // namespace bslib

#endif // _FELICITY_XQ_

#endif /* bs_chess_h */


