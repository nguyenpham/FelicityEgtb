//
//  Extensions.cpp
//  EgtbGen
//
//  Created by Tony Pham on 20/9/18.
//

#include "../egtb/Egtb.h"
#include "Extensions.h"


using namespace egtb;
using namespace bslib;

static const int8_t legalPosBits [7] = {
    1, 2, 4, 0, 0, 0, 8
};

static const int8_t legalPositions [90] = {
    0, 0, 4, 3, 1, 3, 4, 0, 0,
    0, 0, 0, 1, 3, 1, 0, 0, 0,
    4, 0, 0, 3, 5, 3, 0, 0, 4,
    8, 0, 8, 0, 8, 0, 8, 0, 8,
    8, 0,12, 0, 8, 0,12, 0, 8,
    8, 8,12, 8, 8, 8,12, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8,
    12, 8, 8,11,13,11, 8, 8,12,
    8, 8, 8, 9,11, 9, 8, 8, 8,
    8, 8,12,11, 9,11,12, 8, 8,
};

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


int ExtBoard::flip(int pos, FlipMode flipMode) const {
    switch (flipMode) {
        case FlipMode::none: return pos;
        case FlipMode::horizontal: return flip_h[pos];
        case FlipMode::vertical: return flip_v[pos];
        case FlipMode::rotate: return 89 - pos;    // around
        default:
            assert(false);
    }
    return 0;
}

static const FlipMode flipflip_h[] = { FlipMode::horizontal, FlipMode::none, FlipMode::rotate, FlipMode::vertical };
static const FlipMode flipflip_v[] = { FlipMode::vertical, FlipMode::rotate, FlipMode::none, FlipMode::horizontal };
static const FlipMode flipflip_r[] = { FlipMode::rotate, FlipMode::vertical, FlipMode::horizontal, FlipMode::none };

FlipMode ExtBoard::flip(FlipMode oMode, FlipMode flipMode) const {
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

void ExtBoard::flip(FlipMode flipMode) {
    if (flipMode == FlipMode::none) {
        return;
    }
    int bkPieceList[2][16];
    memcpy(bkPieceList, pieceList, sizeof(bkPieceList));

    reset();
    pieceList_reset((int *)pieceList);

    for(int sd = 0; sd < 2; sd++) {
        Side side = static_cast<Side>(sd);

        if (flipMode == FlipMode::vertical || flipMode == FlipMode::rotate) {
            side = getXSide(side);
        }
        for(int i = 0; i < 16; i++) {
            int pos = bkPieceList[sd][i];
            if (pos < 0) {
                continue;
            }
            int newpos = flip(pos, flipMode);
            PieceType type = egtbPieceListIdxToType[i];
            set(newpos, type, side);
        }
    }
}

bool ExtBoard::isValid() const
{
    int pieceCout[2][7] = {{0,0,0,0,0,0,0}, {0,0,0,0,0,0,0}};

    for (int i = 0; i < 90; i++) {
        auto piece = getPiece(i);
        if (piece.type >= PieceType::empty) {
            continue;
        }

        pieceCout[static_cast<int>(piece.side)][static_cast<int>(piece.type)]++;

        auto flag = legalPosBits [static_cast<int>(piece.type)];
        if (flag) {
            auto k = piece.side == Side::white ? (89 - i) : i;
            if ((flag & legalPositions [k]) == 0) {
                printf("flag=%d, k=%d, legalPositions[k]=%d\n", flag, k, legalPositions [k]);
                printOut("Wrong board");
                //                auto piece2 = getPiece90(i);
                return false;
            }
        }
    }

    bool b =
    pieceCout[0][0] == 1 && pieceCout[1][0] == 1 &&     // king
    pieceCout[0][1] <= 2 && pieceCout[1][1] <= 2 &&     // advisor
    pieceCout[0][2] <= 2 && pieceCout[1][2] <= 2 &&     // elephant
    pieceCout[0][3] <= 2 && pieceCout[1][3] <= 2 &&     // rook
    pieceCout[0][5] <= 2 && pieceCout[1][4] <= 2 &&     // cannon
    pieceCout[0][5] <= 2 && pieceCout[1][5] <= 2 &&     // horse
    pieceCout[0][6] <= 5 && pieceCout[1][6] <= 5;       // pawn
    return b;
}

void ExtBoard::genLegalOnly(std::vector<bslib::MoveFull>& moveList, Side attackerSide) {
    gen(moveList, attackerSide);

    Hist hist;
    int x = 0;
    for (int i = 0; i < moveList.size(); i++) {
        make(moveList[i], hist);
        if (!isIncheck(attackerSide)) {
            moveList[x] = moveList[i]; assert(x <= i);
            x++;
        }
        takeBack(hist);
    }
    moveList.end = x;
}


bool ExtBoard::isLegalMove(int from, int dest)
{
    if (from == dest || !isPositionValid(from) || !isPositionValid(dest) || isEmpty(from)) {
        return false;
    }
    
    auto moveSide = getSide(from);
    if (moveSide == getSide(dest)) {
        return false;
    }
    
    std::vector<bslib::MoveFull> moveList;
    gen(moveList, moveSide);
    
    for (int i = 0; i < moveList.size(); i++) {
        auto move = moveList[i];
        if (move.from == from && move.dest == dest) {
            Hist hist;
            make(move, hist);
            bool r = !isIncheck(moveSide);
            takeBack(hist);
            return r;
        }
    }

    return false;
}

bool ExtBoard::isLegal(int pos, PieceType pieceType, Side side) const
{
//    return isPositionValid(pos) && getPiece(pos).isPiece(pieceType, side);

    if (!isPositionValid(pos)) {
        return false;
    }
    
    auto flag = legalPosBits [static_cast<int>(pieceType)];
    if (flag) {
        auto k = side == Side::white ? (89 - pos) : pos;
        if ((flag & legalPositions [k]) == 0) {
            return false;
        }
    }
    
    return true;
}

bool ExtBoard::areKingsFacing() const
{
    auto bK = pieceList[0][0]; assert(isPositionValid(bK));
    auto wK = pieceList[1][0]; assert(isPositionValid(wK));
    
    auto f = bK % 9;
    if (f != wK % 9) {
        return false;
    }
    
    for(int x = bK + 9; x < wK; x += 9) {
        if (!isEmpty(x)) {
            return false;
        }
    }
    
    return true;
}

void ExtBoard::genBackward(std::vector<bslib::MoveFull>& moves, Side side) const
{
    moves.clear();
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
                if (col != 3 && isEmpty(pos - 1)) { // go left
                    gen_addMove(moves, pos, pos - 1);
                }
                if (col != 5 && isEmpty(pos + 1)) { // right
                    gen_addMove(moves, pos, pos + 1);
                }
                if ((pos > 72 || (pos > 8 && pos < 27)) && isEmpty(pos - 9)) { // up
                    gen_addMove(moves, pos, pos - 9);
                }
                if ((pos < 18 || (pos > 63 && pos < 81)) && isEmpty(pos + 9)) { // down
                    gen_addMove(moves, pos, pos + 9);
                }
                break;
            }
                
            case PieceType::advisor:
            {
                int y = pos - 10;   /* go left up */
                if ((y == 3 || y == 13 || y == 66 || y == 76) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 8;        /* go right up */
                if ((y == 5 || y == 13 || y == 68 || y == 76) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 8;        /* go left down */
                if ((y == 13 || y == 21 || y == 84 || y == 76) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 10;       /* go right down */
                if ((y == 13 || y == 23 || y == 76 || y == 86) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }
                
            case PieceType::elephant:
            {
                int y = pos - 20; /* go left up */
                if ((y == 2 || y == 6 || y == 18 || y == 22 || y == 47 || y == 51 || y == 63 || y == 67) && isEmpty(pos - 10) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 16; /* go right up */
                if ((y == 2 || y == 6 || y == 22 || y == 26 || y == 47 || y == 51 || y == 67 || y == 71) && isEmpty(pos - 8) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 16; /* go left down */
                if ((y == 18 || y == 22 || y == 38 || y == 42 || y == 63 || y == 67 || y == 83 || y == 87) && isEmpty(pos + 8) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 20; /* go right up */
                if ((y == 22 || y == 26 || y == 38 || y == 42 || y == 67 || y == 71 || y == 83 || y == 87) && isEmpty(pos + 10) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }
                
            case PieceType::cannon:
            case PieceType::rook:
            {
                int col = pos % 9;
                for (int y=pos - 1; y >= pos - col; y--) { /* go left */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                
                for (int y=pos + 1; y < pos - col + 9; y++) { /* go right */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                
                for (int y=pos - 9; y >= 0; y -= 9) { /* go up */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                
                for (int y=pos + 9; y < 90; y += 9) { /* go down */
                    if (!isEmpty(y)) {
                        break;
                    }
                    gen_addMove(moves, pos, y);
                }
                break;
            }
                
            case PieceType::horse:
            {
                int col = pos % 9;
                int y = pos - 11;
                if (y >= 0 && col > 1 && isEmpty(y + 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 19;
                if (y >= 0 && col > 0 && isEmpty(y + 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 17;
                if (y >= 0 && col < 8 && isEmpty(y + 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos - 7;
                if (y >= 0 && col < 7 && isEmpty(y - 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                
                y = pos + 7;
                if (y < 90 && col > 1 && isEmpty(y + 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 17;
                if (y < 90 && col > 0 && isEmpty(y - 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 19;
                if (y < 90 && col < 8 && isEmpty(y - 9) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                y = pos + 11;
                if (y < 90 && col < 7 && isEmpty(y - 1) && isEmpty(y)) {
                    gen_addMove(moves, pos, y);
                }
                break;
            }
                
            case PieceType::pawn:
            {
                if ((side == Side::black && pos > 44) || (side == Side::white && pos < 45)) {
                    int col = pos % 9;
                    /* go left */
                    if (col > 0 && isEmpty(pos - 1)) {
                        gen_addMove(moves, pos, pos - 1);
                    }
                    /* go right */
                    if (col < 8 && isEmpty(pos + 1)) {
                        gen_addMove(moves, pos, pos + 1);
                    }
                }
                
                if (side == Side::black) {
                    /* go up */
                    if ((pos >= 54 || (pos >= 36 && ((pos % 9) & 1) == 0)) && isEmpty(pos - 9)) {
                        gen_addMove(moves, pos, pos - 9);
                    }
                } else {
                    /* go down */
                    if ((pos < 36 || (pos < 54 && ((pos % 9) & 1) == 0)) && isEmpty(pos + 9)) {
                        gen_addMove(moves, pos, pos + 9);
                    }
                }
                break;
            }
                
            default:
                break;
        }
    }
}
