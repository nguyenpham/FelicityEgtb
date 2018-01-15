
/*
 This file is part of Felicity Egtb, distributed under MIT license.

 Copyright (c) 2018 Nguyen Hong Pham

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


#ifndef EgtbBoard_h
#define EgtbBoard_h

#include "Egtb.h"

namespace egtb {

    extern const char* pieceTypeName;

    class Piece {
    public:
        PieceType type;
        Side side;

        // this variable is used in some purpose such as to setup a board
        int pos;

    public:
        Piece() {}
        Piece(PieceType _type, Side _side, int _pos = -1) {
            set(_type, _side, _pos);
        }

        Piece(PieceType _type, Side _side, Squares square) {
            set(_type, _side, static_cast<int>(square));
        }

        void set(PieceType _type, Side _side, int _pos = -1) {
            type = _type; side = _side; pos = _pos;
        }

        void setEmpty() {
            set(PieceType::empty, Side::none, -1);
        }

        bool isEmpty() const {
            return type == PieceType::empty;
        }

        bool isPiece(PieceType _type, Side _side) const {
            return type == _type && side == _side;
        }

        bool isValid() const {
            return (side == Side::none && type == PieceType::empty) || (side != Side::none && type != PieceType::empty);
        }

        static std::string toString(const PieceType type, const Side side) {
            int k = static_cast<int>(type);
            char ch = pieceTypeName[k];
            if (side == Side::white) {
                ch += 'A' - 'a';
            }
            return std::string(1, ch);
        }

        std::string toString() const {
            return toString(type, side);
        }

    };

    class Move {
    public:
        int8_t  from, dest;
        i32     score;

        PieceType type, capType;
        Side side;

    public:
        Move() {}
        Move(PieceType _type, Side _side, int _from, int _dest) {
            set(_type, _side, _from, _dest);
        }
        Move(int from, int dest) {
            set(from, dest);
        }

        bool operator == (const Move& otherMove) const {
            return from == otherMove.from && dest == otherMove.dest;
        }

        void set(PieceType _type, Side _side, int _from, int _dest) {
            type = _type; side = _side; from = _from; dest = _dest;
        }

        void set(int _from, int _dest) {
            from = _from; dest = _dest;
        }

        bool isValid() const {
            return from != dest && from >= 0 && from < 90 && dest >= 0 && dest < 90;
        }

        std::string toString() const {
            std::ostringstream stringStream;
            stringStream << Piece(type, side).toString()
            << posToCoordinateString(from) << posToCoordinateString(dest);
            return stringStream.str();
        }
    };

    class Hist {
    public:
        Move move;
        Piece cap;

    public:
        void set(const Move& _move) { move = _move; }

        bool isValid() const {
            return move.isValid() && cap.isValid();
        }
    };

    class MoveList {
        const static int MaxMoveNumber = 300;

    public:
        Move list[MaxMoveNumber];
        int end;

    public:
        MoveList() { reset(); }

        void reset() { end = 0; }

        void add(const Move& move) { list[end] = move; end++; }

        void add(PieceType type, Side side, int from, int dest) {
            list[end].set(type, side, from, dest); end++;
        }

        std::string toString() const {
            std::ostringstream stringStream;
            for (int i = 0; i < end; i++) {
                if (i % 2 == 0) {
                    stringStream << i / 2 + 1 << ") ";
                }
                stringStream << list[i].toString() << " ";
            }
            return stringStream.str();
        }
    };


    class EgtbBoard {
    private:
        Piece pieces[90];

    public:
        int pieceList[2][16];
        Side side;

    public:
        void set(int pos, PieceType type, Side side) {
            pieces[pos].set(type, side);
            pieceList_set((int *)pieceList, pos, type, side);
        }

        Piece getPiece(int pos) const {
            return pieces[pos];
        }

        Side getSide(int pos) const {
            return pieces[pos].side;
        }

        bool isEmpty(int pos) const {
            return pieces[pos].type == PieceType::empty;
        }

        void setEmpty(int pos) {
            pieces[pos].setEmpty();
        }

        void gen(MoveList& moveList, Side side, bool capOnly) const;

        bool isIncheck(Side beingAttackedSide) const;

        void make(const Move& move, Hist& hist);
        void takeBack(const Hist& hist);

        void setFen(const std::string& fen);
        bool setup(const std::vector<Piece> pieceVec, Side side);

        void show() const;

        std::string getFen(Side side, int halfCount = 0, int fullMoveCount = 0) const;

        void reset() {
            for (int i = 0; i < 90; i++) {
                setEmpty(i);
            }
        }

        static int flip(int pos, FlipMode flipMode);
        static FlipMode flip(FlipMode oMode, FlipMode flipMode);

        bool pieceList_setupBoard(const int *pieceList = nullptr);
        static void pieceList_reset(int *pieceList);

        bool isThereAttacker() const {
            return pieceList_isThereAttacker((const int *)pieceList);
        }

    private:
        std::string toString() const;

        void gen_addMove(MoveList& moveList, int from, int dest, bool capOnly) const;
        int findKing(Side side) const;

        bool isPiece(int pos, PieceType type, Side side) const {
            auto p = pieces[pos];
            return p.type==type && p.side==side;
        }

        static bool pieceList_set(int *pieceList, int pos, PieceType type, Side side);
        static bool pieceList_setEmpty(int *pieceList, int pos);
        static bool pieceList_setEmpty(int *pieceList, int pos, int sd);
        static bool pieceList_setEmpty(int *pieceList, int pos, PieceType type, Side side);
        static bool pieceList_isThereAttacker(const int *pieceList);

        bool pieceList_make(const Hist& hist);
        bool pieceList_takeback(const Hist& hist);
    };

} // namespace egtb

#endif /* EgtbBoard.h */

