/*
 This file is part of NhatMinh Egtb, distributed under MIT license.

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

#include "Egtb.h"
#include "EgtbDb.h"
#include "EgtbKey.h"

using namespace egtb;

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

void EgtbDb::removeAllBuffers() {
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

void EgtbDb::preload(EgtbMemMode egtbMemMode, EgtbLoadMode loadMode) {
    for (auto && folderName : folders) {
        auto vec = listdir(folderName);

        for (auto && path : vec) {
            if (EgtbFile::knownExtension(path)) {
                EgtbFile *egtbFile = new EgtbFile();
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

void EgtbDb::addEgtbFile(EgtbFile *egtbFile) {
    egtbFileVec.push_back(egtbFile);

    auto s = egtbFile->getName();
    nameMap[s] = egtbFile;
    auto p = s.find_last_of("k");
    auto s0 = s.substr(0, p);
    auto s1 = s.substr(p);
    s = s1 + s0;
    nameMap[s] = egtbFile;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

int EgtbDb::getScore(const std::vector<bslib::Piece> pieceVec, bslib::Side side) {
    EgtbBoard board;
    board.setup(pieceVec, side);
    return getScore(board, board.side);
}

int EgtbDb::getScore(bslib::BoardCore& board) {
    return getScore(board, board.side);
}

int EgtbDb::getScore(bslib::BoardCore& board, bslib::Side side) {
    assert(side == bslib::Side::white || side == bslib::Side::black);

    EgtbFile* pEgtbFile = getEgtbFile(board);
    if (pEgtbFile == nullptr || pEgtbFile->loadStatus == EgtbLoadStatus::error) {
        return EGTB_SCORE_MISSING;
    }

    pEgtbFile->checkToLoadHeaderAndTable();
    auto r = pEgtbFile->getKey(board);
    auto querySide = r.flipSide ? getXSide(side) : side;

    if (pEgtbFile->header->isSide(querySide) && board.enpassant <= 0) {
        int score = pEgtbFile->getScore(r.key, querySide);
        return score;
    }

    return getScoreOnePly(board, side);
}

int EgtbDb::getScoreOnePly(bslib::BoardCore& board, bslib::Side side) {

    auto xside = getXSide(side);

    bslib::MoveList moveList;
    bslib::Hist hist;
    board.gen(moveList, side, false);
    int bestscore = -EGTB_SCORE_MATE, legalCnt = 0;

    for(int i = 0; i < moveList.end; i++) {
        auto move = moveList.list[i];
        board.make(move, hist);

        if (!board.isIncheck(side)) {
            legalCnt++;
            auto score = getScore(board, xside);

            if (score == EGTB_SCORE_MISSING && !hist.cap.isEmpty() && board.pieceList_isDraw()) {
                score = EGTB_SCORE_DRAW;
            }

            if (abs(score) <= EGTB_SCORE_MATE) {
                bestscore = MAX(bestscore, -score);
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

EgtbFile* EgtbDb::getEgtbFile(const bslib::BoardCore& board) const {
    auto name = EgtbFile::pieceListToName((const bslib::Piece* )board.pieceList);
    return nameMap.find(name) != nameMap.end() ? nameMap.at(name) : nullptr;
}

int EgtbDb::probe(const std::vector<bslib::Piece> pieceVec, bslib::Side side, MoveList& moveList) {
    EgtbBoard board;
    board.setup(pieceVec, side);
    return probe(board, moveList);
}

int EgtbDb::probe(const char* fenString, MoveList& moveList) {
    EgtbBoard board;
    board.setFen(fenString);
    return probe(board, moveList);
}

int EgtbDb::probe(bslib::BoardCore& board, MoveList& moveList) {
    auto side = board.side;
    auto xside = getXSide(board.side);
    int bestScore = -EGTB_SCORE_MATE, legalMoveCnt = 0;
    bool cont = true;
    bslib::Move bestMove(bslib::Side::none, -1, -1);

    bslib::MoveList mList;
    board.gen(mList, side, false);

    for(int i = 0; i < mList.end && cont; i++) {
        auto move = mList.list[i];
        bslib::Hist hist;
        board.make(move, hist);
        board.side = xside;

        if (!board.isIncheck(side)) {

            int score = getScore(board);

            if (score == EGTB_SCORE_MISSING) {
                if (!hist.cap.isEmpty() && board.pieceList_isDraw()) {
                    score = EGTB_SCORE_DRAW;
                } else {
                    if (egtbVerbose) {
                        std::cerr << "Error: missing or broken data when probing:" << std::endl;
                        board.show();
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
        moveList.add(bestMove);

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

