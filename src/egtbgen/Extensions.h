//
//  Extensions.h
//  EgtbGen
//
//  Created by Tony Pham on 20/9/18.
//

#ifndef Extensions_h
#define Extensions_h

#include "../chess/chess.h"
#include "../egtb/Egtb.h"
#include "Obj.h"

namespace egtb {

    class ExtBoard : public EgtbBoard, public Obj {
    public:
        virtual int flip(int pos, bslib::FlipMode flipMode) const override;
        virtual bslib::FlipMode flip(bslib::FlipMode oMode, bslib::FlipMode flipMode) const;

        void genBackward(std::vector<bslib::MoveFull>& moves, bslib::Side side) const;
        
        void flip(bslib::FlipMode flipMode) override;

        virtual bool isPositionValid(int pos) const override {
            return pos >= 0 && pos < 90;
        }

        // tricky: not clear pieceList
        virtual void setEmpty(int pos) {
            assert(isPositionValid(pos));
            pieces[pos].setEmpty();
        }

        virtual void setEmpty(int pos, bslib::PieceType type, bslib::Side side) {
            assert(isPositionValid(pos));
            pieces[pos].setEmpty();
            pieceList_setEmpty((int *)pieceList, pos, type, side);
        }

        bool isValid() const override;

        static bool pieceList_isThereAttacker(const int *pieceList) {
            for(int i = 5; i < 16; i ++) {
                if (pieceList[i] >= 0 || pieceList[i + 16] >= 0) return true;
            }
            return false;
        }

        static int pieceList_countStrong(const int *pieceList, bslib::Side side) {
            const int* p = pieceList + (side == bslib::Side::white ? 16 : 0);

            int cnt = 0;
            for(int i = 5; i < 16; i ++) {
                if (p[i] >= 0) cnt++;
            }
            return cnt;
        }

        static int pieceList_countDefence(const int *pieceList, bslib::Side side) {
            const int* p = pieceList + 1 + (side == bslib::Side::white ? 16 : 0);

            int cnt = 0;
            for(int i = 0; i < 4; i ++) {
                if (p[i] >= 0) cnt++;
            }
            return cnt;
        }

        int pieceList_countStrong() const {
            return pieceList_countStrong((const int *)pieceList, bslib::Side::white) + pieceList_countStrong((const int *)pieceList, bslib::Side::black);
        }

//        virtual void printOut(const char* msg = nullptr) const override {
//            if (msg != nullptr) {
//                std::cout << msg << std::endl;
//            }
//            std::cout << EgtbBoard::toString() << std::endl;
//        }
//
//        virtual void printOut(const std::string& msg) const override {
//            printOut(msg.c_str());
//        }

        void genLegalOnly(std::vector<bslib::MoveFull>& moveList, bslib::Side attackerSide);
        bool isLegalMove(int from, int dest);
        bool isLegal(int pos, bslib::PieceType pieceType, bslib::Side side) const;
        
        bool areKingsFacing() const;
    };

};

#endif /* Extensions_h */
