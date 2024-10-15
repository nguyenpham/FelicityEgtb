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

#include "fegtb.h"
#include "fegtbdb.h"
#include "fegtbkey.h"
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
                delete egtbFile;
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
    return getScoreAndIdx(board, side).first;
}

std::pair<int, i64> EgtbDb::getScoreAndIdx(EgtbBoard& board, Side side)
{
    assert(side == Side::white || side == Side::black);

    std::pair<int, i64> p;
    
    auto pEgtbFile = getEgtbFile(board);
    if (pEgtbFile == nullptr || pEgtbFile->getLoadStatus() == EgtbLoadStatus::error) {
        board.printOut("Error: cannot probe endgame for this board");
    } else {
        
        //    pEgtbFile->checkToLoadHeaderAndTables(side);
        pEgtbFile->checkToLoadHeaderAndTables(Side::none);
        
        auto r = pEgtbFile->getIdx(board);
        auto querySide = r.flipSide ? getXSide(side) : side;
        
        if (pEgtbFile->getHeader()->isSide(querySide)
            //#ifdef _FELICITY_CHESS_
            //        && board.enpassant <= 0
            //#endif
            ) {
            p.first = pEgtbFile->getScore(r.key, querySide);
            p.second = r.key;
            return p;
        } else {
            exit(2);
            //    return getScoreOnePly(board, side);
        }
    }
    
    p.first = EGTB_SCORE_MISSING;
    p.second = -1;
    return p;
}

i64 EgtbDb::getKey(EgtbBoard& board) {
    auto pEgtbFile = getEgtbFile(board);
    return pEgtbFile == nullptr ? -1 : pEgtbFile->getIdx(board).key;
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

    auto bestScore = EGTB_SCORE_UNSET, legalMoveCnt = 0, unsetCount = 0;
    auto side = board.side, xside = getXSide(side);
    auto cont = true;
    auto bestMove = MoveFull::illegalMove;

    Hist hist;
    for(auto && move : board.gen(side)) {
        if (!cont) {
            break;
        }
        board.make(move, hist);

        if (!board.isIncheck(side)) {
            int score;
            if (!hist.cap.isEmpty() && board.pieceList_isDraw()) {
                score = EGTB_SCORE_DRAW;
            } else {

                score = getScore(board, xside);

                if (score == EGTB_SCORE_MISSING) {
                    if (egtbVerbose) {
                        std::cerr << "Error: missing or broken data when probing:" << std::endl;
                        board.printOut();
                    }
                    return EGTB_SCORE_MISSING;
                }

                if (score == EGTB_SCORE_UNSET) {
                    unsetCount++;
                    bestScore = EGTB_SCORE_UNSET;
                }
            }

            moveList.push_back(move);

            if (unsetCount == 0 && EgtbFile::pickBestFromRivalScore(bestScore, score)) {
                bestMove = move;

                if (score == EGTB_SCORE_MATE) {
                    cont = false;
                }
            }

            board.printOut("probe after move=" + board.toString(move)
                           + ", score=" + std::to_string(score)
                           + ", bestScore=" + std::to_string(bestScore)
                           );
        }

        board.takeBack(hist);
    }

    if (!legalMoveCnt) {
#ifdef _CHESS_
        bestScore = board.isIncheck(side) ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
        bestScore = -EGTB_SCORE_MATE;
#endif
    }

    return bestScore;
}


int EgtbDb::probeByChildren(EgtbBoard& board, Side side, EgtbFile* mainEgtbFile, bool rule120, bool debugging)
{
    assert(mainEgtbFile);
    
    auto bestScore = EGTB_SCORE_UNSET, legalCount = 0, unsetCount = 0;
    auto xside = getXSide(side);

    if (debugging) {
        board.printOut("EgtbDb::probeByChildren, debugging, side=" + Funcs::side2String(side, false));
    }
    Hist hist;
    for(auto && move : board.gen(side)) {
        board.make(move, hist);
        
        if (!board.isIncheck(side)) {
            legalCount++;

            int score;
            i64 sIdx = -1;
            
            if (hist.cap.isEmpty() && !move.hasPromotion()) {     /// score from current working buffers
                auto r = mainEgtbFile->getIdx(board);
                auto xs = r.flipSide ? side : xside;
                sIdx = r.key;
                score = mainEgtbFile->getScore(r.key, xs, false);
            } else if (!board.hasAttackers()) {
                score = EGTB_SCORE_DRAW;
            } else {            /// probe from a sub-endgame
                score = getScore(board, xside);
                if (score == EGTB_SCORE_MISSING) {
                    board.printOut("Missing endgame for probling the board:");
                    exit(-1);
                }
            }
            
            if (!EgtbFile::pickBestFromRivalScore(bestScore, score, rule120)) {
                unsetCount++;
                bestScore = EGTB_SCORE_UNSET;
            }
            
            if (debugging) {
                board.printOut("after a move=" + board.toString(move)
                               + ", sIdx=" + std::to_string(sIdx)
                               + ", score=" + std::to_string(score)
                               + ", bestScore=" + std::to_string(bestScore)
                               + ", unsetCount=" + std::to_string(unsetCount));
            }

        }
        board.takeBack(hist);

        if (unsetCount && !debugging) {
            return EGTB_SCORE_UNSET;
        }
    }
    
    if (legalCount == 0) {
#ifdef _FELICITY_CHESS_
        bestScore = board.isIncheck(side) ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
        bestScore = -EGTB_SCORE_MATE;
#endif
    }

    if (debugging) {
        std::cout << "Debugging completed, bestScore=" << bestScore << ", legalCount=" << legalCount << std::endl;
    }
    
    return unsetCount ? EGTB_SCORE_UNSET : bestScore;
}

std::pair<Result, std::vector<Move>> EgtbDb::getBestLine(const std::string& fen)
{
    EgtbBoard board;
    board.setFen(fen);
    return getBestLine(board);
}


static i64 getBestLineCnt = 0;

std::pair<Result, std::vector<Move>> EgtbDb::getBestLine(EgtbBoard& board)
{
    getBestLineCnt = 0;
    
    auto score = getScore(board, board.side);
    board.printOut("getBestLine, root score: " + std::to_string(score));
    
    GameResultType rootResultType = score == EGTB_SCORE_DRAW ? GameResultType::draw :
    (score > EGTB_SCORE_DRAW && board.side == Side::white) || (score < EGTB_SCORE_DRAW && board.side == Side::black) ? GameResultType::win
    : GameResultType::loss;

    std::unordered_map<i64, int> idxPlyMap;
    auto p = getBestLine(board, rootResultType, idxPlyMap, 0);
    auto fen = board.getStartingFen();

    std::cout << "result: " << p.first.toString()
    << ", sz: " << p.second.size()
    << ", getBestLineCnt: " << getBestLineCnt
    << std::endl;
    
    std::string s =
    "\n[Event \"FelicityEgtb\"]\n"
    "[Site \"banksiagui.com\"]\n"
    "[White \"White\"]\n"
    "[Black \"Black\"]\n"
    "[PlyCount \"" + std::to_string(p.second.size()) + "\"]\n";
    
    
#ifdef _FELICITY_XQ_
    s += "[Variant \"xiangqi\"]\n";
#endif
    
    s += "[Result \"" + p.first.toString() + "\"]\n";
    
    if (!fen.empty()) {
        s += "[FEN \"" + fen + "\"]\n"
        "[SetUp \"1\"]\n\n";
    }
    
    auto cnt = 0;
    s += "1.";
    if (board.side == Side::black) {
        s += "..";
        cnt++;
    }

    for (auto && m : p.second) {
        if (cnt > 1 && (cnt & 1) == 0) {
            s += std::to_string(1 + cnt / 2) + ".";
        }
        s += board.toString(m) + " ";
        cnt++;
    }
    s += "\n" + p.first.toString() + "\n";
    std::cout << s << std::endl;
    return p;
}

std::pair<Result, std::vector<Move>> EgtbDb::getBestLine(EgtbBoard& board, GameResultType rootResultType, std::unordered_map<i64, int>& idxPlyMap, int quietCnt)
{
    getBestLineCnt++;
    
    std::pair<Result, std::vector<Move>> p;

    if (quietCnt >= 120) {
        p.first.result = GameResultType::draw;
        return p;
    }
    
    auto side = board.side;
    auto xside = getXSide(side);
    auto legalCount = 0;
    auto bestScore = EGTB_SCORE_UNSET;
    std::vector<std::pair<MoveFull, i64>> bestMoves;

#ifdef _FELICITY_XQ_
    auto bestPerpetualScore = EGTB_SCORE_UNSET;
    std::vector<std::pair<MoveFull, i64>> bestPerpetualMoves;
#endif

    for(auto && move : board.gen(side)) {
        board.make(move);

        if (!board.isIncheck(side)) {
            legalCount++;
            
            int score;
            i64 idx;
            auto lastHist = board.getLastHistPointer();
            if (!lastHist->cap.isEmpty() && !board.hasAttackers()) {
                score = EGTB_SCORE_DRAW;
                idx = -1;
            } else {
                auto r = getScoreAndIdx(board, xside);
                score = EgtbFile::revertScore(r.first);
                idx = r.second;
            }
            
#ifdef _FELICITY_CHESS_
            if (EgtbFile::isSmallerScore(bestScore, score)) {
                bestScore = score;
                bestMoves.clear();
            }
            
            if (bestScore == score) {
                std::pair<MoveFull, i64> q;
                q.first = move;
                q.second = r.second;
                bestMoves.push_back(q);
            }
#else
            if (EgtbFile::isPerpetualScore(score)) {
                if (EgtbFile::isSmallerScore(bestPerpetualScore, score)) {
                    bestPerpetualScore = score;
                    bestPerpetualMoves.clear();
                }
                
                if (bestPerpetualScore == score) {
                    std::pair<MoveFull, i64> q;
                    q.first = move;
                    q.second = idx; //r.second;
                    bestPerpetualMoves.push_back(q);
                }
            } else {
                if (EgtbFile::isSmallerScore(bestScore, score)) {
                    bestScore = score;
                    bestMoves.clear();
                }
                
                if (bestScore == score) {
                    std::pair<MoveFull, i64> q;
                    q.first = move;
                    q.second = idx; //r.second;
                    bestMoves.push_back(q);
                }
            }
#endif
        }
        
        board.takeBack();
    }
    

    if (!legalCount) {
        p.first.result = side == Side::white ? GameResultType::loss : GameResultType::win;
        return p;
    }
    
    if (bestScore == EGTB_SCORE_DRAW) {
        if (rootResultType == GameResultType::draw) {
            p.first.result = GameResultType::draw;
            p.second.push_back((*bestMoves.begin()).first);
            return p;
        } else {
            
        }
    }
    
    if (!bestPerpetualMoves.empty()) {
        bestMoves.insert(bestMoves.end(), bestPerpetualMoves.begin(), bestPerpetualMoves.end());
//        bestMoves = bestPerpetualMoves;
    }

    for(auto && b : bestMoves) {
        if (b.second < 0) {
            continue;
        }
        
        board.make(b.first);
        assert(!board.isIncheck(side));
        Result result;

        auto idxx = b.second;
        if (side == Side::white) {
            idxx = -idxx;
        }
        auto x = idxPlyMap.find(idxx);
        if (x != idxPlyMap.end()) {
#ifdef _FELICITY_CHESS_
            result.result = GameResultType::draw;
            result.reason = ReasonType::repetition;
#else
            auto repeatLen = int(idxPlyMap.size()) - x->second + 1;
            if (repeatLen >= 4) {
                result = board.ruleRepetition(repeatLen);

                if (result.result != rootResultType) {
                    result.result = GameResultType::unknown;
                }
            }
#endif
//            if (!result.isNone()) {
//                /// opposite result -> revert twice for white side
////                auto resultType = result.result;
////                if (resultType != GameResultType::draw &&
////                    side == Side::white) {
////                    resultType = resultType != GameResultType::win ? GameResultType::loss : GameResultType::win;
////                }
//                
//                if (result.result != rootResultType) {
//                    result.result = GameResultType::unknown;
//                }
//            }
        }
        
        if (!result.isNone()) {
            p.first = result;
            p.second.push_back(b.first);
        } else {
            auto qCnt = board.getLastHistPointer()->cap.isEmpty() ? quietCnt + 1 : 1;

            idxPlyMap[idxx] = int(idxPlyMap.size());
            auto v = getBestLine(board, rootResultType, idxPlyMap, qCnt);
            idxPlyMap.erase(idxx);
            
            if (v.first == rootResultType
//                !v.first.isNone()
                && (p.first.isNone() || v.first >= p.first)) {
                
                p.first = v.first;
                p.second.push_back(b.first);
                p.second.insert(p.second.end(), v.second.begin(), v.second.end());
            }
        }
        board.takeBack();
        if (!p.first.isNone()) { //} && p.second.size() <= 1) {
            break;
        }
    }

    return p;
}
