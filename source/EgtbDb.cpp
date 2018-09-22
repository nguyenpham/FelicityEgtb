
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

#include "EgtbDb.h"


using namespace egtb;

EgtbDb::EgtbDb() {
}

EgtbDb::EgtbDb(const std::string& folder, EgtbMemMode egtbMemMode){
    preload(folder, egtbMemMode);
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
    map.clear();
    nameMap.clear();
}

void EgtbDb::removeAllBuffers() {
    for (auto && egtbFile : egtbFileVec) {
        egtbFile->removeBuffers();
    }
}

void EgtbDb::setFolder(const std::vector<std::string>& folders_) {
    folders.clear();
    folders.insert(folders_.end(), folders_.begin(), folders_.end());
}

void EgtbDb::addFolder(const std::string& folderName) {
    folders.push_back(folderName);
}

EgtbFile* EgtbDb::createEgtbFile() const {
    return new EgtbFile();
}

EgtbFile* EgtbDb::getEgtbFile(const std::string& name) {
    return nameMap[name];
}

void EgtbDb::preload(const std::string& folder, EgtbMemMode egtbMemMode, EgtbLoadMode loadMode) {
    addFolder(folder);
    preload(egtbMemMode, loadMode);
}

void EgtbDb::preload(EgtbMemMode egtbMemMode, EgtbLoadMode loadMode) {
    std::vector<EgtbLookup *> lookupVec;

    for (auto && folderName : folders) {
        auto vec = listdir(folderName);

        for (auto && path : vec) {
            auto p = EgtbFile::getExtensionType(path);
            if (p.first == EgtbType::dtm || p.first == EgtbType::newdtm) {
                EgtbFile *egtbFile = createEgtbFile(); //new EgtbFile();
                if (egtbFile->preload(path, egtbMemMode, loadMode)) {
                    auto pos = map.find(egtbFile->materialsignWB);
                    if (pos == map.end()) {
                        addEgtbFile(egtbFile);
                        continue;
                    }
                    pos->second->merge(*egtbFile);
                }
                free(egtbFile);
            } else if (p.first == EgtbType::lookup) {
                EgtbLookup* lookup = new EgtbLookup();
                if (lookup->preload(path, egtbMemMode, loadMode)) {
                    lookupVec.push_back(lookup);
                } else {
                    delete lookup;
                }
            } else {
                //std::cout << "Unrecorgnized file " << path << std::endl;
            }
        }
    }

    for (auto * lookup : lookupVec) {
        auto egtbFile = nameMap[lookup->getName()];
        if (egtbFile) {
            egtbFile->addLookup(lookup);
        } else {
            delete lookup;
        }
    }
}

void EgtbDb::addEgtbFile(EgtbFile *egtbFile) {
    egtbFileVec.push_back(egtbFile);
    map[egtbFile->materialsignWB] = egtbFile;
    map[egtbFile->materialsignBW] = egtbFile;
    nameMap[egtbFile->getName()] = egtbFile;
}

int EgtbDb::getSize() const
{
    return (int)egtbFileVec.size();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
EgtbFile* EgtbDb::getEgtbFile(const int* pieceList) const {
    u64 mat = EgtbFile::pieceListToMaterialSign(pieceList);
    auto mat1 = (int)mat;
    auto pos = map.find(mat1);
    if (pos == map.end()) {
        int mat2 = (int)(mat >> 32);
        pos = map.find(mat2);
    }
    return pos != map.end() ? pos->second : nullptr;
}

int EgtbDb::getScore(const int* pieceList, Side side, AcceptScore accept) {
    EgtbFile* pEgtbFile = getEgtbFile(pieceList);
    if (pEgtbFile == nullptr || pEgtbFile->loadStatus == EgtbLoadStatus::error) {
        return EGTB_SCORE_MISSING;
    }

    pEgtbFile->checkToLoadHeaderAndTable(Side::none); // load all sides
    auto r = pEgtbFile->getKey(pieceList);
    auto querySide = r.flipSide ? getXSide(side) : side;

    if (pEgtbFile->header->isSide(querySide)) {
        auto score = pEgtbFile->getScore(r.key, querySide);
        if ((score == EGTB_SCORE_WINNING && accept == AcceptScore::real) || score == EGTB_SCORE_UNKNOWN) {
            score = pEgtbFile->lookup((const int *)pieceList, querySide);
        }
        return score;
    }

    auto xside = getXSide(side);
    EgtbBoard board;
    board.pieceList_setupBoard(pieceList);

    MoveList moveList;
    Hist hist;
    board.gen(moveList, side, false);
    int bestscore = -EGTB_SCORE_MATE, legalCnt = 0;

    for(int i = 0; i < moveList.end; i++) {
        auto move = moveList.list[i];
        board.make(move, hist);

        if (!board.isIncheck(side)) {
            legalCnt++;
            auto score = getScore((const int*)board.pieceList, xside, accept);

            if (score == EGTB_SCORE_MISSING && !hist.cap.isEmpty() && !board.isThereAttacker()) {
                score = EGTB_SCORE_DRAW;
            }

            if (abs(score) <= EGTB_SCORE_MATE) {
                bestscore = std::max(bestscore, -score);
            }
        }
        board.takeBack(hist);
    }

    if (legalCnt && abs(bestscore) <= EGTB_SCORE_MATE && bestscore != EGTB_SCORE_DRAW) {
        bestscore += bestscore > 0 ? -1 : +1;
    }
    return bestscore;
}

int EgtbDb::getScore(const EgtbBoard& board, AcceptScore accept) {
    return getScore((const int*)board.pieceList, board.side, accept);
}

int EgtbDb::getScore(const EgtbBoard& board, Side side, AcceptScore accept) {
    return getScore((const int*)board.pieceList, side, accept);
}

int EgtbDb::getScore(const char* fenString, AcceptScore accept) {
    EgtbBoard board;
    board.setFen(fenString);
    return getScore(board, accept);
}

int EgtbDb::getScore(const std::vector<Piece> pieceVec, Side side, AcceptScore accept) {
    EgtbBoard board;
    board.setup(pieceVec, side);
    return getScore(board, accept);
}

int EgtbDb::probe(const std::vector<Piece> pieceVec, Side side, MoveList& moveList) {
    EgtbBoard board;
    board.setup(pieceVec, side);
    return probe(board, moveList);
}

int EgtbDb::probe(const char* fenString, MoveList& moveList) {
    EgtbBoard board;
    board.setFen(fenString);
    return probe(board, moveList);
}

int EgtbDb::probe(EgtbBoard& board, MoveList& moveList) {
    auto side = board.side;
    auto xside = getXSide(board.side);
    int bestScore = -EGTB_SCORE_MATE, legalMoveCnt = 0;
    bool cont = true;
    Move bestMove(-1, -1);

    MoveList mList;
    board.gen(mList, side, false);


    for(int i = 0; i < mList.end && cont; i++) {
        auto move = mList.list[i];
        Hist hist;
        board.make(move, hist);
        board.side = xside;

        if (!board.isIncheck(side)) {

            int score = getScore(board);

            if (score == EGTB_SCORE_MISSING) {
                if (!hist.cap.isEmpty() && !board.isThereAttacker()) {
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
        return -EGTB_SCORE_MATE;
    }

    if (bestScore != EGTB_SCORE_DRAW && bestScore < abs(EGTB_SCORE_MATE)) {
        bestScore += bestScore > 0 ? -1 : 1;
    }

    if (bestMove.isValid()) {
        bestMove.score = bestScore;
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

