
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

#include <iostream>

/*
 * Need to add only one .h file
 */
#include "Egtb.h"

/*
 * The EGT should declare only one since it may take time to load data and some memory
 */
egtb::EgtbDb egtbDb;

std::string explainScore(int score);

int main(int argc, const char * argv[]) {
    std::cout << "Welcome to Felicity Xiangqi Endgame databases - version: " << egtb::getVersion() << std::endl;

    /*
     * Allow Egtb to print out more information
     */
    egtb::egtbVerbose = true;

    /*
     * Preload endgames into memory
     * If this process of loading is slow, you may use a background thread to load it and use when data is ready
     */
    // You may give Egtb the main folder and it will scan that folder and all subfolder for endgames
    // You may also locate endgames in several folders and use the function addFolder to add one by one
    const char* egtbDataFolder = "./egtb";

    // Data can be loaded all into memory or tiny or smart (let programm decide between all-tiny)
    egtb::EgtbMemMode egtbMemMode = egtb::EgtbMemMode::all;
    // Data can be loaded right now or don't load anything until the first request
    egtb::EgtbLoadMode loadMode = egtb::EgtbLoadMode::onrequest;

    egtbDb.preload(egtbDataFolder, egtbMemMode, loadMode);

    // The numbers of endgames should not be zero
    // for current all endgames of one attacker (R, C, H, P), the number should be 108
    if (egtbDb.getSize() == 0) {
        std::cerr << "Error: Egtb could not load any endgames from folder " << egtbDataFolder << ". Full one-attacker egtb should have totally 108 endgames. Please check!" << std::endl;
        return -1;
    }

    std::cout << "Egtb database size: " << egtbDb.getSize() << std::endl << std::endl;

    /*
     * Query scores
     * To enter a chess board to query, you may use a fen strings or an array of pieces, each piece has piece type, side and position
     * You may put input (fen string or vector of pieces) directly to egtb or via internal board of egtb
     *
     * WARNING: the chess board must be valid, otherwise the score may have no meaning (just a random number)
     */

    // Query with internal board of egtb
    // The advantages of using that board you may quickly show the board or check the valid
    egtb::EgtbBoard board;

    board.setFen(""); // default starting board
    board.show();

    auto score = egtbDb.getScore(board);
    std::cout << "Query the starting board, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    board.setFen("3a1a2C/5k3/9/9/9/9/9/9/9/2EAKAE2 b 0 0");
    board.show();

    score = egtbDb.getScore(board);
    std::cout << "Query with a fen string, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    board.setFen("3ak4/4a4/9/9/9/9/h8/3AK4/9/3A5 b 0 0");
    board.show();

    score = egtbDb.getScore(board);
    std::cout << "Query with a fen string (black is the strong side), score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    // Use a vector of pieces
    /*
     Squares is defined in Egtb.h as below:
     enum Squares {
     a9, b9, c9, d9, e9, f9, g9, h9, i9,
     a8, b8, c8, d8, e8, f8, g8, h8, i8,
     a7, b7, c7, d7, e7, f7, g7, h7, i7,
     a6, b6, c6, d6, e6, f6, g6, h6, i6,
     a5, b5, c5, d5, e5, f5, g5, h5, i5,
     a4, b4, c4, d4, e4, f4, g4, h4, i4,
     a3, b3, c3, d3, e3, f3, g3, h3, i3,
     a2, b2, c2, d2, e2, f2, g2, h2, i2,
     a1, b1, c1, d1, e1, f1, g1, h1, i1,
     a0, b0, c0, d0, e0, f0, g0, h0, i0
     };
     */

    std::vector<egtb::Piece> pieces;
    pieces.push_back(egtb::Piece(egtb::PieceType::rook, egtb::Side::white, egtb::Squares::d0));
    pieces.push_back(egtb::Piece(egtb::PieceType::king, egtb::Side::white, egtb::Squares::e0));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::white, egtb::Squares::d2));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::white, egtb::Squares::e1));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::white, egtb::Squares::c0));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::white, egtb::Squares::e2));

    pieces.push_back(egtb::Piece(egtb::PieceType::king, egtb::Side::black, egtb::Squares::e7));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::black, egtb::Squares::d7));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::black, egtb::Squares::e8));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::black, egtb::Squares::a7));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::black, egtb::Squares::c9));

    if (board.setup(pieces, egtb::Side::white)) { // vector of pieces and side to move
        board.show();
        auto score = egtbDb.getScore(board);
        std::cout << "Query with a vector of pieces, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;
    } else {
        std::cerr << "Error on board setup" << std::endl;
    }

    // Not use internal board:
    score = egtbDb.getScore(pieces, egtb::Side::white);
    std::cout << "Query directly (not using internal board) with a vector of pieces, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    score = egtbDb.getScore(pieces, egtb::Side::white, egtb::AcceptScore::winning); // accept scores as DTM (normal) or WINNING value
    std::cout << "Query directly with a vector of pieces, accepted winning score, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    /*
     * Probe a position via a vector of pieces
     * Different from getScore, probe will return a list of moves which lead to the mate
     */
    egtb::MoveList moveList;
    score = egtbDb.probe(pieces, egtb::Side::white, moveList);
    std::cout << "Probe directly with a vector of pieces, score: " << score << ", explaination: " << explainScore(score) << std::endl;
    std::cout << "moves to mate: " << moveList.toString() << std::endl << std::endl;

    return 0;
}

std::string explainScore(int score) {
    std::string str = "";

    switch (score) {
        case EGTB_SCORE_DRAW:
            str = "draw";
            break;

        case EGTB_SCORE_MISSING:
            str = "missing (board is incorrect or missing some endgame databases)";
            break;
        case EGTB_SCORE_MATE:
            str = "mate";
            break;

        case EGTB_SCORE_WINNING:
            str = "winning";
            break;

        case EGTB_SCORE_ILLEGAL:
            str = "illegal";
            break;

        case EGTB_SCORE_UNKNOWN:
            str = "unknown";
            break;

        default: {
            char buf[250];
            auto mateInPly = EGTB_SCORE_MATE - abs(score);
            int mateIn = (mateInPly + 1) / 2; // devide 2 for full (not half or ply) moves
            if (score < 0) mateIn = -mateIn;

            snprintf(buf, sizeof(buf), "mate in %d (%d %s)", mateIn, mateInPly, mateInPly <= 1 ? "ply" : "plies");
            str = buf;
            break;
        }
    }
    return str;
}

