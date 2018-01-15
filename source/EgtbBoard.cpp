
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

#include "EgtbBoard.h"

namespace egtb {

    extern const char* pieceTypeName;
    extern const int egtbPieceListStartIdxByType[7];
    extern const PieceType egtbPieceListIdxToType[16];

    const int egtbPieceListStartIdxByType[7] = { 0, 1, 3, 5, 7, 9, 11 };

    // king, advisor, elephant, rook, cannon, horse, pawn, empty
    const char* pieceTypeName = "kaerchp.";

    const PieceType egtbPieceListIdxToType[16] = {
        PieceType::king,
        PieceType::advisor, PieceType::advisor,
        PieceType::elephant, PieceType::elephant,
        PieceType::rook, PieceType::rook,
        PieceType::cannon, PieceType::cannon,
        PieceType::horse, PieceType::horse,
        PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn
    };

}

using namespace egtb;

static const std::string originalFen = "rneakaenr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNEAKAENR w - - 0 1";

void EgtbBoard::show() const {
    std::cout << toString() << std::endl;
}

std::string EgtbBoard::toString() const {
    std::ostringstream stringStream;

    stringStream << getFen(side) << std::endl;

    for (int i = 0; i < 90; i++) {
        auto pieceType = getPiece(i).type;
        auto side = getSide(i);

        stringStream << Piece::toString(pieceType, side) << " ";

        if (i > 0 && i % 9 == 8) {
            int row = 9 - i / 9;
            stringStream << " " << row << "\n";
        }
    }

    stringStream << "a b c d e f g h i  ";

    if (side != Side::none) {
        stringStream << (side == Side::white ? "w turns" : "b turns") << "\n";
    }
    return stringStream.str();
}

bool EgtbBoard::setup(const std::vector<Piece> pieceVec, Side _side) {
    pieceList_reset((int *)pieceList);
    for (auto && p : pieces) {
        p.setEmpty();
    }

    side = _side;
    for (auto && p : pieceVec) {
        if (!isEmpty(p.pos)) {
            return false;
        }
        set(p.pos, p.type, p.side);
    }
    return true;
}

void EgtbBoard::setFen(const std::string& fen) {
    pieceList_reset((int *)pieceList);
    for (auto && p : pieces) {
        p.setEmpty();
    }

    std::string thefen = fen;
    if (fen.empty()) {
        thefen = originalFen;
    }

    bool last = false;
    side = Side::white;

    for (int i=0, pos=0; i < (int)thefen.length(); i++) {
        char ch = thefen.at(i);

        if (ch==' ') {
            last = true;
            continue;
        }

        if (last) {
            if (ch=='w' || ch=='W') {
                side = Side::white;
            } else if (ch=='b' || ch=='B') {
                side = Side::black;
            }

            continue;
        }

        if (ch=='/') {
            continue;
        }

        if (ch>='0' && ch <= '9') {
            int num = ch - '0';
            pos += num;
            continue;
        }

        Side side = Side::black;
        if (ch >= 'A' && ch < 'Z') {
            side = Side::white;
            ch += 'a' - 'A';
        }

        auto pieceType = PieceType::empty;
        const char* p = strchr(pieceTypeName, ch);
        if (p==NULL) {
            if (ch=='n') {
                pieceType = PieceType::horse;
            } else if (ch=='b') {
                pieceType = PieceType::elephant;
            }
        } else {
            int k = (int)(p - pieceTypeName);
            pieceType = static_cast<PieceType>(k);

        }
        if (pieceType != PieceType::empty) {
            set(pos, pieceType, side);
        }
        pos ++;
    }
}

std::string EgtbBoard::getFen(Side side, int halfCount, int fullMoveCount) const {
    std::ostringstream stringStream;

    int e=0;
    for (int i=0; i < 90; i++) {
        auto piece = getPiece(i);
        if (piece.isEmpty()) {
            e += 1;
        } else {
            if (e) {
                stringStream << e;
                e = 0;
            }
            stringStream << piece.toString();
        }

        if (i % 9 == 8) {
            if (e) {
                stringStream << e;
            }
            if (i < 89) {
                stringStream << "/";
            }
            e = 0;
        }
    }

    stringStream << (side == Side::white ? " w " : " b ") << halfCount << " " << fullMoveCount;

    return stringStream.str();
}

void EgtbBoard::pieceList_reset(int *pieceList) {
    for(int i = 0; i < 16; i++) {
        pieceList[i] = pieceList[16 + i] = -1;
    }
}

bool EgtbBoard::pieceList_set(int *pieceList, int pos, PieceType type, Side side) {
    int d = side == Side::white ? 16 : 0;
    for (int t = egtbPieceListStartIdxByType[static_cast<int>(type)]; ; t++) {
        if (t >= 16) {
            break;
        }
        if (pieceList[d + t] < 0 || pieceList[d + t] == pos) {
            pieceList[d + t] = pos;
            return true;
        }
    }
    return false;
}

bool EgtbBoard::pieceList_setEmpty(int *pieceList, int pos, PieceType type, Side side) {
    int d = side == Side::white ? 16 : 0;
    for (int t = egtbPieceListStartIdxByType[static_cast<int>(type)]; ; t++) {
        if (t >= 16) {
            break;
        }
        if (pieceList[d + t] == pos) {
            pieceList[d + t] = -1;
            return true;
        }
    }
    return false;
}

bool EgtbBoard::pieceList_setEmpty(int *pieceList, int pos) {
    for (int sd = 0; sd < 2; sd ++) {
        if (pieceList_setEmpty(pieceList, pos, sd)) {
            return true;
        }
    }
    return false;
}

bool EgtbBoard::pieceList_setEmpty(int *pieceList, int pos, int sd) {
    int d = sd == 0 ? 0 : 16;
    for(int i = 0; i < 16; i++) {
        if (pieceList[i + d] == pos) {
            pieceList[i + d] = -1;
            return true;
        }
    }

    return false;
}

bool EgtbBoard::pieceList_isThereAttacker(const int *pieceList) {
    for(int i = 5; i < 16; i ++) {
        if (pieceList[i] >= 0 || pieceList[i + 16] >= 0) return true;
    }
    return false;
}

bool EgtbBoard::pieceList_make(const Hist& hist) {
    if (!hist.cap.isEmpty()) {
        bool ok = false;
        for (int t = egtbPieceListStartIdxByType[static_cast<int>(hist.cap.type)], sd = static_cast<int>(hist.cap.side); ; t++) {
            if (t >= 16) {
                break;
            }
            if (pieceList[sd][t] == hist.move.dest) {
                pieceList[sd][t] = -1;
                ok = true;
                break;
            }
        }
        if (!ok) {
            return false;
        }
    }
    for (int t = egtbPieceListStartIdxByType[static_cast<int>(hist.move.type)], sd = static_cast<int>(hist.move.side); ; t++) {
        if (t >= 16) {
            break;
        }
        if (pieceList[sd][t] == hist.move.from) {
            pieceList[sd][t] = hist.move.dest;
            return true;
        }
    }
    return false;
}

bool EgtbBoard::pieceList_takeback(const Hist& hist) {
    bool ok = false;
    for (int t = egtbPieceListStartIdxByType[static_cast<int>(hist.move.type)], sd = static_cast<int>(hist.move.side); ; t++) {
        if (t >= 16) {
            break;
        }
        if (pieceList[sd][t] == hist.move.dest) {
            pieceList[sd][t] = hist.move.from;
            ok = true;
            break;
        }
    }
    if (!ok) {
        return false;
    }
    if (hist.cap.isEmpty()) {
        return true;
    }
    for (int t = egtbPieceListStartIdxByType[static_cast<int>(hist.cap.type)], sd = static_cast<int>(hist.cap.side); ; t++) {
        if (t >= 16) {
            break;
        }
        if (pieceList[sd][t] < 0) {
            pieceList[sd][t] = hist.move.dest;
            return true;
        }
    }
    return false;
}

bool EgtbBoard::pieceList_setupBoard(const int *thePieceList) {
    reset();

    if (thePieceList == nullptr) {
        thePieceList = (const int *) pieceList;
    } else {
        pieceList_reset((int *)pieceList);
    }

    for (int sd = 0, d = 0; sd < 2; sd++, d = 16) {
        Side side = static_cast<Side>(sd);
        for(int i = 0; i < 16; i++) {
            auto pos = thePieceList[d + i];
            if (pos >= 0) {
                if (!isEmpty(pos)) {
                    return false;
                }
                auto type = egtbPieceListIdxToType[i];
                set(pos, type, side);
            }
        }
    }

    return true;
}

static const int flip_h[90] = {
    8, 7, 6, 5, 4, 3, 2, 1, 0,
    17,16,15,14,13,12,11,10, 9,
    26,25,24,23,22,21,20,19,18,
    35,34,33,32,31,30,29,28,27,
    44,43,42,41,40,39,38,37,36,
    53,52,51,50,49,48,47,46,45,
    62,61,60,59,58,57,56,55,54,
    71,70,69,68,67,66,65,64,63,
    80,79,78,77,76,75,74,73,72,
    89,88,87,86,85,84,83,82,81
};

static const int flip_v[90] = {
    81, 82,83,84,85,86,87,88,89,
    72, 73,74,75,76,77,78,79,80,
    63, 64,65,66,67,68,69,70,71,
    54, 55,56,57,58,59,60,61,62,
    45, 46,47,48,49,50,51,52,53,

    36, 37,38,39,40,41,42,43,44,
    27, 28,29,30,31,32,33,34,35,
    18, 19,20,21,22,23,24,25,26,
    9, 10,11,12,13,14,15,16,17,
    0,  1, 2, 3, 4, 5, 6, 7, 8
};


int EgtbBoard::flip(int pos, FlipMode flipMode) {
    switch (flipMode) {
        case FlipMode::none: return pos;
        case FlipMode::horizontal: return flip_h[pos];
        case FlipMode::vertical: return flip_v[pos];
        case FlipMode::rotate: return 89 - pos;    // around
        default:
            break;
    }
    return 0;
}

static const FlipMode flipflip_h[] = { FlipMode::horizontal, FlipMode::none, FlipMode::rotate, FlipMode::vertical };
static const FlipMode flipflip_v[] = { FlipMode::vertical, FlipMode::rotate, FlipMode::none, FlipMode::horizontal };
static const FlipMode flipflip_r[] = { FlipMode::rotate, FlipMode::vertical, FlipMode::horizontal, FlipMode::none };

FlipMode EgtbBoard::flip(FlipMode oMode, FlipMode flipMode) {
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

void EgtbBoard::make(const Move& move, Hist& hist) {
    hist.move = move;
    hist.cap = pieces[move.dest];
    pieces[move.dest] = pieces[move.from];
    pieces[move.from].setEmpty();

    pieceList_make(hist);
}

void EgtbBoard::takeBack(const Hist& hist) {
    pieces[hist.move.from] = pieces[hist.move.dest];
    pieces[hist.move.dest] = hist.cap;

    pieceList_takeback(hist);
}

int EgtbBoard::findKing(Side side) const
{
    auto sd = static_cast<int>(side);
    auto kingpos = pieceList[sd][0];
    return kingpos;
}

void EgtbBoard::gen_addMove(MoveList& moves, int from, int dest, bool captureOnly) const {
    auto toSide = pieces[dest].side;

    if (pieces[from].side != toSide && (!captureOnly || toSide != Side::none)) {
        moves.add(pieces[from].type, pieces[from].side, from, dest);
    }
}

void EgtbBoard::gen(MoveList& moves, Side side, bool captureOnly) const {
    moves.reset();
    for (int l = 0; l < 16; l++) {
        auto pos = pieceList[static_cast<int>(side)][l];
        if (pos < 0) {
            continue;
        }
        auto piece = pieces[pos];

        switch (piece.type) {
            case PieceType::king:
            {
                int col = pos % 9;
                if (col != 3) { // go left
                    gen_addMove(moves, pos, pos - 1, captureOnly);
                }
                if (col != 5) { // right
                    gen_addMove(moves, pos, pos + 1, captureOnly);
                }
                if (pos > 72 || (pos > 8 && pos < 27)) { // up
                    gen_addMove(moves, pos, pos - 9, captureOnly);
                }
                if (pos < 18 || (pos > 63 && pos < 81)) { // down
                    gen_addMove(moves, pos, pos + 9, captureOnly);
                }
                break;
            }

            case PieceType::advisor:
            {
                int y = pos - 10;   /* go left up */
                if (y == 3 || y == 13 || y == 66 || y == 76) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos - 8;        /* go right up */
                if (y == 5 || y == 13 || y == 68 || y == 76) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos + 8;        /* go left down */
                if (y == 13 || y == 21 || y == 84 || y == 76) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos + 10;       /* go right down */
                if (y == 13 || y == 23 || y == 76 || y == 86) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                break;
            }

            case PieceType::elephant:
            {
                int y = pos - 20; /* go left up */
                if ((y == 2 || y == 6 || y == 18 || y == 22 || y == 47 || y == 51 || y == 63 || y == 67) && isEmpty(pos - 10)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos - 16; /* go right up */
                if ((y == 2 || y == 6 || y == 22 || y == 26 || y == 47 || y == 51 || y == 67 || y == 71) && isEmpty(pos - 8)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos + 16; /* go left down */
                if ((y == 18 || y == 22 || y == 38 || y == 42 || y == 63 || y == 67 || y == 83 || y == 87) && isEmpty(pos + 8)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos + 20; /* go right up */
                if ((y == 22 || y == 26 || y == 38 || y == 42 || y == 67 || y == 71 || y == 83 || y == 87) && isEmpty(pos + 10)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                break;
            }

            case PieceType::cannon: {
                int col = pos % 9;
                /*
                 * go left
                 */
                int f = 0;

                for (int y=pos - 1; y >= pos - col; y--) {
                    if (isEmpty(y)) {
                        if (f == 0 && !captureOnly) {
                            gen_addMove(moves, pos, y, captureOnly);
                        }
                        continue;
                    }
                    f++;
                    if (f == 2) {
                        gen_addMove(moves, pos, y, captureOnly);
                        break;
                    }
                }
                /*
                 * go right
                 */
                f = 0;
                for (int y=pos + 1; y < pos - col + 9; y++) {
                    if (isEmpty(y)) {
                        if (f == 0 && !captureOnly) {
                            gen_addMove(moves, pos, y, captureOnly);
                        }
                        continue;
                    }
                    f++;
                    if (f == 2) {
                        gen_addMove(moves, pos, y, captureOnly);
                        break;
                    }
                }

                f = 0;
                for (int y=pos - 9; y >= 0; y -= 9) { /* go up */
                    if (isEmpty(y)) {
                        if (f == 0 && !captureOnly) {
                            gen_addMove(moves, pos, y, captureOnly);
                        }
                        continue;
                    }
                    f += 1 ;
                    if (f == 2) {
                        gen_addMove(moves, pos, y, captureOnly);
                        break;
                    }
                }

                f = 0;
                for (int y=pos + 9; y < 90; y += 9) { /* go down */
                    if (isEmpty(y)) {
                        if (f == 0 && !captureOnly) {
                            gen_addMove(moves, pos, y, captureOnly);
                        }
                        continue;
                    }
                    f += 1 ;
                    if (f == 2) {
                        gen_addMove(moves, pos, y, captureOnly);
                        break;
                    }
                }

                break;
            }

            case PieceType::rook:
            {
                int col = pos % 9;
                for (int y=pos - 1; y >= pos - col; y--) { /* go left */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }

                for (int y=pos + 1; y < pos - col + 9; y++) { /* go right */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }
                }

                for (int y=pos - 9; y >= 0; y -= 9) { /* go up */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }

                }

                for (int y=pos + 9; y < 90; y += 9) { /* go down */
                    gen_addMove(moves, pos, y, captureOnly);
                    if (!isEmpty(y)) {
                        break;
                    }

                }
                break;
            }

            case PieceType::horse:
            {
                int col = pos % 9;
                int y = pos - 11;
                int z = pos - 1;
                if (y >= 0 && col > 1 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos - 19;
                z = pos - 9;
                if (y >= 0 && col > 0 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos - 17;
                z = pos - 9;
                if (y >= 0 && col < 8 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos - 7;
                z = pos + 1;
                if (y >= 0 && col < 7 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }

                y = pos + 7;
                z = pos - 1;
                if (y < 90 && col > 1 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos + 17;
                z = pos + 9;
                if (y < 90 && col > 0 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos + 19;
                z = pos + 9;
                if (y < 90 && col < 8 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                y = pos + 11;
                z = pos + 1;
                if (y < 90 && col < 7 && isEmpty(z)) {
                    gen_addMove(moves, pos, y, captureOnly);
                }
                break;
            }

            case PieceType::pawn:
            {
                if ((side == Side::black && pos > 44) || (side == Side::white && pos < 45)) {
                    int col = pos % 9;
                    /* go left */
                    if (col > 0) {
                        gen_addMove(moves, pos, pos - 1, captureOnly);
                    }
                    /* go right */
                    if (col < 8) {
                        gen_addMove(moves, pos, pos + 1, captureOnly);
                    }
                }

                if (side == Side::black) {
                    /* go down */
                    if (pos < 81) {
                        gen_addMove(moves, pos, pos + 9, captureOnly);
                    }
                } else {
                    /* go up */
                    if (pos > 8) {
                        gen_addMove(moves, pos, pos - 9, captureOnly);
                    }
                }
                break;
            }

            default:
                break;
        }
    }
}

bool EgtbBoard::isIncheck(Side beingAttackedSide) const
{
    int kingPos = findKing(beingAttackedSide);

    Side attackerSide = getXSide(beingAttackedSide);

    /*
     * Check horizontal and vertical lines for attacking of Rook, Cannon and
     * King face
     */

    /* go down */
    int y = kingPos + 9;
    if (y < 90) {
        int f = 0;
        auto p = pieces[y];
        if (!p.isEmpty()) {
            f = 1;
            if (p.side == attackerSide && (p.type == PieceType::rook || (p.type == PieceType::pawn && attackerSide == Side::white))) {
                return true;
            }
        }

        for (int yy = y+9; yy < 90; yy+=9) {
            auto p = pieces[yy];
            if (p.isEmpty()) {
                continue;
            }
            f++;
            if (p.side == attackerSide) {
                if ((f == 1 && (p.type == PieceType::rook || p.type == PieceType::king)) || (f == 2 && p.type == PieceType::cannon)) {
                    return true;
                }
            }
            if (f == 2) {
                break;
            }
        }
    }

    /* go left */
    y = kingPos - 1;
    int f = 0;

    auto p = pieces[y];
    if (!p.isEmpty()) {
        f = 1;
        if (p.side == attackerSide && (p.type == PieceType::rook || p.type == PieceType::pawn)) {
            return true;
        }
    }

    int col = kingPos % 9;

    for (int yy = y-1; yy >= kingPos - col; yy--) {
        auto p = pieces[yy];
        if (p.isEmpty()) {
            continue;
        }
        f++;
        if (p.side == attackerSide) {
            if ((f == 1 && p.type == PieceType::rook) || (f == 2 && p.type == PieceType::cannon)) {
                return true;
            }
        }
        if (f == 2) {
            break;
        }
    }

    /* go right */
    y = kingPos + 1;
    f = 0;
    p = pieces[y];
    if (!p.isEmpty()) {
        f = 1;
        if (p.side == attackerSide && (p.type == PieceType::rook || p.type == PieceType::pawn)) {
            return true;
        }
    }

    for (int yy = y+1; yy < kingPos - col + 9; yy++) {
        auto p = pieces[yy];
        if (p.isEmpty()) {
            continue;
        }
        f++;
        if (p.side == attackerSide) {
            if ((f == 1 && p.type == PieceType::rook) || (f == 2 && p.type == PieceType::cannon)) {
                return true;
            }
        }
        if (f == 2) {
            break;
        }
    }

    /* go up */
    y = kingPos - 9;
    if (y >= 0) {
        f = 0;
        p = pieces[y];
        if (!p.isEmpty()) {
            f = 1;
            if (p.side == attackerSide && (p.type == PieceType::rook || (p.type == PieceType::pawn && attackerSide == Side::black))) {
                return true;
            }
        }

        for (int yy = y-9; yy >= 0; yy-=9) {
            auto p = pieces[yy];
            if (p.isEmpty()) {
                continue;
            }
            f++;
            if (p.side == attackerSide) {
                if ((f == 1 && (p.type == PieceType::rook || p.type == PieceType::king)) || (f == 2 && p.type == PieceType::cannon)) {
                    return true;
                }
            }
            if (f == 2) {
                break;
            }
        }
    }

    /* Check attacking of Knight */
    if (kingPos > 9 && isPiece(kingPos - 11, PieceType::horse, attackerSide) && isEmpty(kingPos - 10)) {
        return true;
    }
    if (kingPos > 18 && isPiece(kingPos - 19, PieceType::horse, attackerSide) && isEmpty(kingPos - 10)) {
        return true;
    }
    if (kingPos > 18 && isPiece(kingPos - 17, PieceType::horse, attackerSide) && isEmpty(kingPos - 8)) {
        return true;
    }
    if (kingPos > 9 && isPiece(kingPos - 7, PieceType::horse, attackerSide) && isEmpty(kingPos - 8)) {
        return true;
    }
    if (kingPos < 81 && isPiece(kingPos + 7, PieceType::horse, attackerSide) && isEmpty(kingPos + 8)) {
        return true;
    }
    if (kingPos < 72 && isPiece(kingPos + 17, PieceType::horse, attackerSide) && isEmpty(kingPos + 8)) {
        return true;
    }
    if (kingPos < 72 && isPiece(kingPos + 19, PieceType::horse, attackerSide) && isEmpty(kingPos + 10)) {
        return true;
    }
    if (kingPos < 81 && isPiece(kingPos + 11, PieceType::horse, attackerSide) && isEmpty(kingPos + 10)) {
        return true;
    }

    return false;
}


