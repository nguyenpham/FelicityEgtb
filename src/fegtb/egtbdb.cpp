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

#include "egtb.h"
#include "egtbdb.h"
#include "egtbkey.h"
#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;

extern const int egtbOrderPieceValue[8];

EgtbDb::EgtbDb() {
}

EgtbDb::~EgtbDb() {
    closeAll();
}

void EgtbDb::closeAll() {
    for (auto && egtbFile : egtbFileVec) {
        delete egtbFile;
    }
    folders.clear();
    egtbFileVec.clear();
    nameMap.clear();
}

void EgtbDb::removeAllProbedBuffers() {
    for (auto && egtbFile : egtbFileVec) {
        egtbFile->removeBuffers();
    }
}

void EgtbDb::setFolders(const std::vector<std::string>& folders_) {
    folders.clear();
    folders.insert(folders_.end(), folders_.begin(), folders_.end());
}

void EgtbDb::addFolders(const std::string& folderName) {
    folders.push_back(folderName);
}

EgtbFile* EgtbDb::getEgtbFile(const std::string& name) {
    return nameMap[name];
}

void EgtbDb::preload(const std::string& folder, EgtbMemMode egtbMemMode, EgtbLoadMode loadMode) {
    addFolders(folder);
    preload(egtbMemMode, loadMode);
}

void EgtbDb::preload(EgtbMemMode egtbMemMode, EgtbLoadMode loadMode)
{
    for (auto && folderName : folders) {
        auto vec = listdir(folderName);

        for (auto && path : vec) {
            if (EgtbFile::getExtensionType(path) == EgtbType::dtm) {
                auto egtbFile = new EgtbFile();
                if (egtbFile->preload(path, egtbMemMode, loadMode)) {
                    auto pos = nameMap.find(egtbFile->getName());
                    if (pos == nameMap.end()) {
                        addEgtbFile(egtbFile);
                        continue;
                    }
                    pos->second->merge(*egtbFile);
                } else {
                    std::cout << "Error: not loaded: " << path << std::endl;
                }
                free(egtbFile);
            }
        }
    }
}


bool EgtbDb::verifyEgtbFileSides() const
{
    auto r = true;
    for (auto&& egtbFile : egtbFileVec) {
        auto header = egtbFile->getHeader();
        if (header && (!header->isSide(Side::white) || !header->isSide(Side::black))) {
            std::cerr << "Error, missing sides for endgame " << egtbFile->getName() << std::endl;
            r = false;
        }
    }
    return r;
}

void EgtbDb::addEgtbFile(EgtbFile *egtbFile) {
    egtbFileVec.push_back(egtbFile);

    auto s = egtbFile->getName();
    nameMap[s] = egtbFile;
    
    /// swap strong-weak sides
    auto p = s.find_last_of("k");
    auto s0 = s.substr(0, p);
    auto s1 = s.substr(p);
    s = s1 + s0;
    nameMap[s] = egtbFile;
}

////////////////////////////////////////////////////////////////////////

int EgtbDb::getScore(EgtbBoard& board) {
    return getScore(board, board.side);
}

int EgtbDb::getScore(EgtbBoard& board, Side side) {
    assert(side == Side::white || side == Side::black);

    auto pEgtbFile = getEgtbFile(board);
    if (pEgtbFile == nullptr || pEgtbFile->getLoadStatus() == EgtbLoadStatus::error) {
        board.printOut("Error: cannot probe endgame for this board");
        return EGTB_SCORE_MISSING;
    }

//    pEgtbFile->checkToLoadHeaderAndTables(side);
    pEgtbFile->checkToLoadHeaderAndTables(Side::none);
    
    auto r = pEgtbFile->getKey(board);
    auto querySide = r.flipSide ? getXSide(side) : side;

    if (pEgtbFile->getHeader()->isSide(querySide)
//#ifdef _FELICITY_CHESS_
//        && board.enpassant <= 0
//#endif
        ) {
        return pEgtbFile->getScore(r.key, querySide);
    }

    {
        board.printOut(std::string("Error: missing endgame for this board with side ") + Funcs::side2String(querySide, false));
        std::cout << "path: " << pEgtbFile->getPath(querySide) << ", getProperty" << pEgtbFile->getHeader()->getProperty() << std::endl;
    }


    exit(2);
//    return getScoreOnePly(board, side);
    return EGTB_SCORE_MISSING;
}

i64 EgtbDb::getKey(EgtbBoard& board) {
    auto pEgtbFile = getEgtbFile(board);
    return pEgtbFile == nullptr ? -1 : pEgtbFile->getKey(board).key;
}

int EgtbDb::getScoreOnePly(EgtbBoard& board, Side side) {

    auto xside = getXSide(side);

    Hist hist;
    auto bestscore = -EGTB_SCORE_MATE, legalCnt = 0;

    for(auto move : board.gen(side)) {
        board.make(move, hist);

        if (!board.isIncheck(side)) {
            legalCnt++;
            auto score = getScore(board, xside);

            if (score == EGTB_SCORE_MISSING && !hist.cap.isEmpty() && board.pieceList_isDraw()) {
                score = EGTB_SCORE_DRAW;
            }

            if (abs(score) <= EGTB_SCORE_MATE) {
                bestscore = std::max(bestscore, -score);
            }
        }
        board.takeBack(hist);
    }

    if (legalCnt) {
        if (abs(bestscore) <= EGTB_SCORE_MATE && bestscore != EGTB_SCORE_DRAW) {
            bestscore += bestscore > 0 ? -1 : +1;
        }
        return bestscore;
    }

    return board.isIncheck(side) ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
}


std::string EgtbDb::getEgtbFileName(const BoardCore& board)
{
#ifdef _FELICITY_CHESS_
    std::string names[2][8], wname, bname;
    
    for(auto sd = 0; sd < 2; sd++) {
        for(auto i = 0; i < 16; i++) {
            auto pos = board.pieceList[sd][i];
            if (pos >= 0) {
                auto piece = board.getPiece(pos);
                assert(piece.isValid() && !piece.isEmpty());
                auto t = static_cast<int>(piece.type);
                names[sd][t] += Funcs::pieceTypeName[t];
            }
        }
    }
    
    for(auto i = 0; i < 8; i++) {
        wname += names[W][i];
        bname += names[B][i];
    }
    
    assert(wname.find('k') != std::string::npos);
    assert(bname.find('k') != std::string::npos);
    return wname + bname;

#else
    std::string names[2];
    int mats[2] = { 0, 0 };
    
    for(auto sd = 0; sd < 2; sd++) {
        for(auto i = 0; i < 16; i++) {
            /// king, rook, cannon, horse, pawn, advisor, elephant
            static int nameOrder[] = { 0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4 };
            auto j = nameOrder[i];
            auto pos = board.pieceList[sd][j];

            if (pos >= 0) {
                auto piece = board.getPiece(pos);
                assert(piece.isValid() && !piece.isEmpty());
                auto t = static_cast<int>(piece.type);
                names[sd] += Funcs::pieceTypeName[t];
                
                mats[sd] += t * egtbOrderPieceValue[t];
            }
        }
    }
    
    return mats[W] >= mats[B] ? names[W] + names[B] : names[B] + names[W];

#endif

}

EgtbFile* EgtbDb::getEgtbFile(const BoardCore& board) const {
    auto name = EgtbDb::getEgtbFileName(board);
    return nameMap.find(name) != nameMap.end() ? nameMap.at(name) : nullptr;
}

int EgtbDb::probe(const std::string& fenString, std::vector<MoveFull>& moveList) {
    EgtbBoard board;
    board.setFen(fenString);
    return probe(board, moveList);
}

int EgtbDb::probe(EgtbBoard& board, std::vector<MoveFull>& moveList) {
    auto side = board.side;
    auto xside = getXSide(board.side);
    auto bestScore = -EGTB_SCORE_MATE, legalMoveCnt = 0;
    auto cont = true;
    auto bestMove = MoveFull::illegalMove;

    for(auto && move : board.gen(side)) {
        if (!cont) {
            break;
        }
        Hist hist;
        board.make(move, hist);
        board.side = xside;

        if (!board.isIncheck(side)) {

            auto score = getScore(board);

            if (score == EGTB_SCORE_MISSING) {
                if (!hist.cap.isEmpty() && board.pieceList_isDraw()) {
                    score = EGTB_SCORE_DRAW;
                } else {
                    if (egtbVerbose) {
                        std::cerr << "Error: missing or broken data when probing:" << std::endl;
                        board.printOut();
                    }
                    return EGTB_SCORE_MISSING;
                }
            }
            if (score <= EGTB_SCORE_MATE) {
                legalMoveCnt++;
                score = -score;

                if (score > bestScore) {
                    bestMove = move;
                    bestScore = score;

                    if (score == EGTB_SCORE_MATE) {
                        cont = false;
                    }
                }
            }
        }

        board.takeBack(hist);
        board.side = side;
    }

    if (!legalMoveCnt) {
        return board.isIncheck(side) ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
    }

    if (bestScore != EGTB_SCORE_DRAW && bestScore < abs(EGTB_SCORE_MATE)) {
        bestScore += bestScore > 0 ? -1 : 1;
    }

    if (bestMove.isValid()) {
        moveList.push_back(bestMove);

        if (abs(bestScore) != EGTB_SCORE_MATE && bestScore != EGTB_SCORE_DRAW) {
            Hist hist;
            board.make(bestMove, hist);
            auto side = board.side;
            board.side = getXSide(side);

            probe(board, moveList);
            board.takeBack(hist);
            board.side = side;
        }
    }
    return bestScore;
}

