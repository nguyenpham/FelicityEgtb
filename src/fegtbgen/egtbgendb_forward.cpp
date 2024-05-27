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

#include "egtbgendb.h"
#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;


int EgtbGenDb::gen_forward_probe(GenBoard& board, i64 idx, Side side, bool setupBoard) {
    
    if (setupBoard) {
        auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white);
        assert(ok);
    }
    
    assert(side == Side::black || board.enpassant <= 0);

    auto legalCount = 0, unsetCount = 0, bestScore = EGTB_SCORE_UNSET;
    auto xside = getXSide(side);
    for(auto && move
        : board.gen(side)) {
        Hist hist;
        board.make(move, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            auto internal = hist.cap.isEmpty() && !move.hasPromotion();

            int score;
            if (internal) {
                auto r = egtbFile->getKey(board);
                auto xs = r.flipSide ? side : xside;
                score = egtbFile->getScore(r.key, xs, false);
            } else {
                if (!board.hasAttackers()) {
                    score = EGTB_SCORE_DRAW;
                } else {
                    /// If the move is a capture or a promotion, it should probe from outside/a sub-endgames
                    score = getScore(board, xside);

                    if (score == EGTB_SCORE_MISSING) {
                        board.printOut("Error: missing sub endgames for probing the board:");
                        exit(2);
                    }
                    assert(score != EGTB_SCORE_MISSING);
                }
            }
            if (score <= EGTB_SCORE_MATE) {
                score = -score;
                if (score != EGTB_SCORE_DRAW) {
                    if (score > 0) score--;
                    else if (score < 0) score++;
                }

                bestScore = bestScore > EGTB_SCORE_MATE ? score : std::max(bestScore, score);
            } else {
                assert(score != EGTB_SCORE_MISSING);
                unsetCount++;
            }
        }
        board.takeBack(hist);

        assert(side == Side::black || board.enpassant <= 0);
    }
    
    /// Something wrong since it should always have legal moves
    if (legalCount == 0) {
#ifdef _FELICITY_CHESS_
        return board.isIncheck(side) ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
        return -EGTB_SCORE_MATE;
#endif
    }
    
    assert(legalCount > 0); assert(bestScore != EGTB_SCORE_ILLEGAL);
    return unsetCount == 0 || bestScore > EGTB_SCORE_DRAW || (bestScore == EGTB_SCORE_DRAW && side == Side::black && !egtbFile->isBothArmed()) ? bestScore : EGTB_SCORE_UNSET;
}

void EgtbGenDb::gen_forward_thread_init(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx && !rcd.board);
    rcd.createBoard();
    
    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        
        if (egtbVerbose && (idx - rcd.fromIdx) % (16 * 1024 * 1024) == 0) {
            std::lock_guard<std::mutex> thelock(printMutex);
            std::cout << "init, threadIdx = " << threadIdx << ", idx = " << idx << ", toIdx = " << rcd.toIdx << ", " << (idx - rcd.fromIdx) * 100 / (rcd.toIdx - rcd.fromIdx) << "%" << std::endl;
        }

        if (!egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white)
#ifdef _FELICITY_XQ_
            || !rcd.board->isLegal() /// don't need to check legal for chess variant
#endif
            ) {
            
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::black);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::white);
            continue;
        }
        
        assert(rcd.board->isValid());

        bool inchecks[] = {
            rcd.board->isIncheck(Side::black),
            rcd.board->isIncheck(Side::white)
        };
        
        for (auto sd = 0; sd < 2; sd++) {
            auto s = EGTB_SCORE_UNSET;
            auto xsd = 1 - sd;
            if (inchecks[xsd]) {
                s = EGTB_SCORE_ILLEGAL;
            } else {
                auto moveList = rcd.board->genLegalOnly(static_cast<Side>(sd));
                if (moveList.empty()) {
#ifdef _FELICITY_CHESS_
                    s = inchecks[sd] ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
                    s = -EGTB_SCORE_MATE;
#endif
                }
            }

            auto side = static_cast<Side>(sd);
            auto r = egtbFile->setBufScore(idx, s, side); assert(r);
            assert(s == egtbFile->getScore(idx, side));
        }
        
    }
    
    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "gen_thread_init done, threadIdx: " << threadIdx << std::endl;
    }
}


/// Generate within a thread
void EgtbGenDb::gen_forward_thread(int threadIdx, int sd, int ply) {
    auto& rcd = threadRecordVec.at(threadIdx);

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tLoop, threadIdx: " << threadIdx << ", start ply: " << ply << std::endl;
    }

    auto side = static_cast<Side>(sd);

    for (auto idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        auto oldScore = egtbFile->getScore(idx, side, false);
        
        if (abs(oldScore) >= EGTB_SCORE_MATE - 1 - ply && oldScore < EGTB_SCORE_UNSET) {
            continue;
        }

        auto bestScore = gen_forward_probe(*rcd.board, idx, side);
        assert(bestScore >= -EGTB_SCORE_MATE);

        if (bestScore != oldScore && bestScore <= EGTB_SCORE_MATE) {
            auto saveok = egtbFile->setBufScore(idx, bestScore, side);
            assert(saveok);
            rcd.changes++;
        }
    }

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tLoop, threadIdx: " << threadIdx << ", completed ply: " << ply << std::endl;
    }
}

/// Using forward move-generator
void EgtbGenDb::gen_forward(const std::string& folder) {
    if (egtbVerbose) {
        std::cout << "\tGenerate forwardly!" << std::endl;
    }

    auto ply = 0;
    auto side = Side::white;
    
    /// Load temp files
    auto wLoop = 0, bLoop = 0;
//    if (useTempFiles) {
//        wLoop = egtbFile->readFromTmpFile(folder, Side::white);
//        bLoop = egtbFile->readFromTmpFile(folder, Side::black);
//    }
    
    if (wLoop > 0 && bLoop > 0) {
        ply = std::max(wLoop, bLoop);
        side = wLoop > bLoop ? Side::white : Side::black;
        
        ply++; /// next loop
        side = getXSide(side);
        
        if (egtbVerbose) {
            std::cout << "\tLoaded temp file, start from ply: " << ply << ", sd: " << Funcs::side2String(side, false) << std::endl;
        }
    }
    
    /// Init loop
    if (ply == 0) {
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::gen_forward_thread_init, this, i));
        }
        
        gen_forward_thread_init(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }

    /// Main loops
    i64 totalChangeCnt = 0;
    
    for(auto tryCnt = 2; tryCnt > 0; ply++, side = getXSide(side)) {
        auto startLoop = time(NULL);

        resetAllThreadRecordCounters();
        
        std::vector<std::thread> threadVec;
        auto sd = static_cast<int>(side);
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenDb::gen_forward_thread, this, i, sd, ply));
        }
        gen_forward_thread(0, sd, ply);
        
        for (auto && t : threadVec) {
            t.join();
        }

        auto callLoopChangeCnt = allThreadChangeCount();
        
        totalChangeCnt += callLoopChangeCnt;
        
        if (egtbVerbose) {
            int elapsed_secs = std::max(1, (int)(time(NULL) - startLoop));
            std::cout << "\tChanged: " << GenLib::formatString(callLoopChangeCnt) << ", elapsed: " << GenLib::formatPeriod(elapsed_secs) << " (" << elapsed_secs << " s), speed: " << GenLib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
        }
        
//        if (useTempFiles && ply > 0 && callLoopChangeCnt > 0) {
//            egtbFile->writeTmpFile(folder, side, ply);
//        }
        
        if (callLoopChangeCnt == 0) {
            tryCnt--;
        } else {
            tryCnt = 2;
        }
    }
}
