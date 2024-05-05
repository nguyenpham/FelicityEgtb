/**
 This file is part of Felicity Egtb, distributed under MIT license.

 * Copyright (c) 2024 Nguyen Pham (github@nguyenpham)
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

#include "egtbgenfilemng.h"
#include "../base/funcs.h"

using namespace fegtb;
using namespace bslib;

bool twoBytes = false; // per item
bool useTempFiles = false;
bool useBackward = false;

i64 maxEndgameSize = -1;

#ifdef _FELICITY_CHESS_
static const int pieceValueForSortingNames[8] = { -1, 10000, 1000, 400, 300, 200, 100, 0 };
#else

/// Pawn before A, B
static const int pieceValueForSortingNames[8] = { -1, 10000, 150, 100, 1000, 500, 400, 300 };
#endif

////////////////////////////////////////////
bool NameRecord::parse(const std::string& _name)
{
    memset(pieceCount, 0, sizeof(pieceCount));
    attackerCount[0] = attackerCount[1] = 0;
    name = _name;
    type = EgtbType::dtm;
    
    for(auto i = 0, sd = W, preMat = -1; i < name.size(); i++) {
        char ch = name[i];
        auto type = Funcs::charactorToPieceType(ch);
        if (type == PieceType::empty) {
            std::cerr << "Error: don't know the charactor " << ch << " in name " << name << std::endl;
            return false;
        }
        
        auto t = static_cast<int>(type);

        auto mat = pieceValueForSortingNames[t];

        if (type == PieceType::king) {
            preMat = mat;
            if (i > 0) {
                sd = B;
            }
            pieceCount[sd][t]++;
            continue;
        }

        pieceCount[sd][t]++;

        if (mat > preMat) {
            return false;
        }
        preMat = mat;

        mats[sd] += mat;

#ifdef _FELICITY_CHESS_
        attackerMats[sd] += mat;
        attackerCount[sd]++;
#else
        if (t >= ROOK) {
            attackerMats[sd] += mat;
            attackerCount[sd]++;
        }
#endif
    }
    
    return true;
}

bool NameRecord::isValid() const
{
    return ok
    && pieceCount[0][KING] == 1 && pieceCount[1][KING] == 1
    && attackerMats[W] > 0
    && attackerMats[W] >= attackerMats[B];
}

bool NameRecord::isBothArmed() const
{
    return attackerMats[W] > 0 && attackerMats[B] > 0 ;
}

bool NameRecord::hasAttackers() const
{
    return attackerMats[W] + attackerMats[B] > 0 ;
}

bool NameRecord::isMeSmaller(const NameRecord& other) const
{
    return attackerCount[W] < other.attackerCount[W]
    ||
(attackerCount[W] == other.attackerCount[W] && attackerMats[W] < other.attackerMats[W])
    ||
    (attackerCount[W] == other.attackerCount[W] && attackerMats[W] == other.attackerMats[W] && mats[W] < other.mats[W])
    ||
    (attackerCount[W] == other.attackerCount[W] && attackerMats[W] == other.attackerMats[W] && mats[W] == other.mats[W] && mats[B] < other.mats[B])
    ;
}

////////////////////////////////////////////
void EgtbGenFileMng::gen_single_init(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    assert(rcd.fromIdx < rcd.toIdx);
    assert(!rcd.board);
    rcd.board = new EgtbBoard();
    
    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        
        if (egtbVerbose && (idx - rcd.fromIdx) % (16 * 1024 * 1024) == 0) {
            std::lock_guard<std::mutex> thelock(printMutex);
            std::cout << "init, threadIdx = " << threadIdx << ", idx = " << idx << ", toIdx = " << rcd.toIdx << ", " << (idx - rcd.fromIdx) * 100 / (rcd.toIdx - rcd.fromIdx) << "%" << std::endl;
        }
        
        
        if (!egtbFile->setupBoard(*rcd.board, idx, FlipMode::none, Side::white)
#ifdef _FELICITY_XQ_
            || !rcd.board->isLegal() /// don't need to check legal for chess variant
#endif
            ) {
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, 0);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, 1);
            continue;
        }
        
        assert(rcd.board->isValid());

        bool inchecks[] = { rcd.board->isIncheck(Side::black), rcd.board->isIncheck(Side::white) };
        int scores[2];
        
        for (auto sd = 0; sd < 2; sd++) {
            auto xsd = 1 - sd;
            if (inchecks[xsd]) {
                scores[sd] = EGTB_SCORE_ILLEGAL;
            } else {
                std::vector<MoveFull> moveList;
                rcd.board->genLegalOnly(moveList, static_cast<Side>(sd));
                auto s = EGTB_SCORE_UNSET;
                if (moveList.empty()) {
#ifdef _FELICITY_CHESS_
                    s = inchecks[sd] ? -EGTB_SCORE_MATE : EGTB_SCORE_DRAW;
#else
                    s = -EGTB_SCORE_MATE;
#endif
                }
                scores[sd] = s;
            }
        }
        
        for (auto sd = 0; sd < 2; sd++) {
            auto r = egtbFile->setBufScore(idx, scores[sd], sd); assert(r);
            assert(scores[sd] == egtbFile->getScore(idx, static_cast<Side>(sd)));
        }
    }
    
    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "EgtbGenFileMng::genSingleEgtb_init done, threadIdx: " << threadIdx << std::endl;
    }
}


int EgtbGenFileMng::probe_gen(EgtbBoard& board, i64 idx, Side side, int ply, int oldScore) {
    
    auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white);
    assert(ok);
    
    auto legalCount = 0, unsetCount = 0;
    auto bestScore = EGTB_SCORE_UNSET;
    auto xside = getXSide(side);

    Hist hist;
    std::vector<MoveFull> moveList;
    board.gen(moveList, side); assert(!moveList.empty());
    
    for(auto i = 0; i < moveList.size(); i++) {
        auto m = moveList[i];

        board.make(m, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            auto internal = hist.cap.isEmpty();

            int score;
            if (internal) {
                auto idx = egtbFile->getKey(board).key;
                score = egtbFile->getScore(idx, xside, false);

            } else {
                if (!board.hasAttackers()) {
                    score = EGTB_SCORE_DRAW;
                } else {
                    score = getScore(board, xside);

                    if (score == EGTB_SCORE_MISSING) {
                        board.printOut("Error: missing sub data for probing the board:");
                        exit(-1);
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
    }

    assert(legalCount > 0); assert(bestScore != EGTB_SCORE_ILLEGAL);
    return unsetCount == 0 || bestScore > EGTB_SCORE_DRAW || (bestScore == EGTB_SCORE_DRAW && side == Side::black && !egtbFile->isBothArmed()) ? bestScore : EGTB_SCORE_UNSET;
}

void EgtbGenFileMng::gen_single_thread(int threadIdx, int sd, int ply) {
    auto& rcd = threadRecordVec.at(threadIdx);

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tLoop, threadIdx: " << threadIdx << ", start ply: " << ply << std::endl;
    }

    auto side = static_cast<Side>(sd);

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        auto oldScore = egtbFile->getScore(idx, side, false);
        
        if (abs(oldScore) >= EGTB_SCORE_MATE - 1 - ply && oldScore < EGTB_SCORE_UNSET) {
            continue;
        }

        auto bestScore = probe_gen(*rcd.board, idx, side, ply, oldScore);
        assert(bestScore >= -EGTB_SCORE_MATE);

        if (bestScore != oldScore && bestScore <= EGTB_SCORE_MATE) {
            assert(bestScore != EGTB_SCORE_WINNING && bestScore != EGTB_SCORE_UNKNOWN);
            auto saveok = egtbFile->setBufScore(idx, bestScore, sd);
            assert(saveok);
            rcd.changes++;
        }
    }

    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "\tLoop, threadIdx: " << threadIdx << ", completed ply: " << ply << std::endl;
    }
}

void EgtbGenFileMng::gen_single_forward(const std::string& folder) {
    if (egtbVerbose) {
        std::cout << "\tGenerate forwardly!" << std::endl;
    }

    auto ply = 0, sd = W;
    
    /// Load temp files
    auto wLoop = 0, bLoop = 0;
    if (useTempFiles) {
        wLoop = egtbFile->readFromTmpFile(folder, W);
        bLoop = egtbFile->readFromTmpFile(folder, B);
    }
    
    if (wLoop > 0 && bLoop > 0) {
        ply = std::max(wLoop, bLoop);
        sd = wLoop > bLoop ? W : B;
        
        ply++; // next loop
        sd = 1 - sd;
        
        if (egtbVerbose) {
            std::cout << "\tLoaded temp file, start from ply: " << ply << ", sd: " << sd << std::endl;
        }
    }
    
    /// Init loop
    if (ply == 0) {
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::gen_single_init, this, i));
        }
        
        gen_single_init(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        threadVec.clear();
    }

    /// Main loops
    i64 totalChangeCnt = 0;
    
    for(auto tryCnt = 2; tryCnt > 0; ply++, sd = 1 - sd) {
        auto startLoop = time(NULL);

        resetAllThreadRecordCounters();
        
        std::vector<std::thread> threadVec;
        for (auto i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::gen_single_thread, this, i, sd, ply));
        }
        gen_single_thread(0, sd, ply);
        
        for (auto && t : threadVec) {
            t.join();
        }

        auto callLoopChangeCnt = allThreadChangeCount();
        
        totalChangeCnt += callLoopChangeCnt;
        
        if (egtbVerbose) {
            int elapsed_secs = std::max(1, (int)(time(NULL) - startLoop));
            std::cout << "\tChanged: " << GenLib::formatString(callLoopChangeCnt) << ", elapsed: " << GenLib::formatPeriod(elapsed_secs) << " (" << elapsed_secs << " s), speed: " << GenLib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
        }
        
        if (useTempFiles && ply > 0 && callLoopChangeCnt > 0) {
            egtbFile->writeTmpFile(folder, sd, ply);
        }
        
        if (callLoopChangeCnt == 0) {
            tryCnt--;
        } else {
            tryCnt = 2;
        }
    }
}

bool EgtbGenFileMng::gen_single(const std::string& folder, const std::string& name, EgtbType egtbType, CompressMode compressMode) {
    
    startTime = time(NULL);

    assert(egtbFile == nullptr);
    egtbFile = new EgtbFileWriter();
    egtbFile->create(name, egtbType);

    auto sz = egtbFile->getSize();
    auto bufsz = sz * 2;
    if (egtbFile->isTwoBytes()) bufsz += bufsz;
    if (useBackward) bufsz += sz / 2;

    std::cout << std::endl << "Start generating " << name << ", " << GenLib::formatString(sz) << (egtbFile->isTwoBytes() ? ", 2b/item" : "") << ", main mem sz: " << GenLib::formatString(bufsz) << ", " << GenLib::currentTimeDate() << std::endl;

    egtbFile->createBuffersForGenerating();
    assert(egtbFile->isValidHeader());
    
    setupThreadRecords(egtbFile->getSize());

    /// WARNING: not useBackward
    gen_single_forward(folder);

    auto r = gen_single_finish(folder, compressMode);
    if (r) {
        std::cout << "Generated successfully " << name << std::endl << std::endl;
    }
    return r;

}

bool EgtbGenFileMng::gen_single_finish(const std::string& folder, CompressMode compressMode, bool needVerify)
{
//    perpetuationFix_finish();

    assert(egtbFile->isValidHeader());
    egtbFile->getHeader()->addSide(Side::white);
    egtbFile->getHeader()->addSide(Side::black);

    if (egtbVerbose) {
        int elapsed_secs = std::max(1, (int)(time(NULL) - startTime));
        std::cout << "Completed generating " << egtbFile->getName() << ", elapse: " << GenLib::formatPeriod(elapsed_secs) << ", speed: " << GenLib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
    }

    addEgtbFile(egtbFile);


    /// Verify
    if (needVerify && !verifyData(egtbFile)) {
        std::cerr << "Error: verify FAILED for " << egtbFile->getName() << std::endl;
        exit(-1);
        return false;
    }
    
//    egtbFile->checkAndConvert2bytesTo1();

    if (egtbFile->saveFile(folder, compressMode)) {
        egtbFile->createStatsFile();

        egtbFile->removeTmpFiles(folder);
        egtbFile = nullptr;
        return true;
    }

    std::cerr << "Error: Generator ended UNSUCCESSFULLY " << egtbFile->getName() << std::endl;
    exit(-1);
    return false;
}

bool EgtbGenFileMng::gen_all(std::string folder, const std::string& name, EgtbType egtbType, CompressMode compressMode) {
    std::cout << "Missing endgames, they will be generated:\n";

    std::vector<std::string> vec = showMissing(name);

    if (!folder.empty()) {
        GenLib::createFolder(folder);
        folder += "/";
    }

    for(auto && aName : vec) {
        if (nameMap.find(aName) != nameMap.end()) {
//            if (nameMap[aName]->getSubPawnRank() < 0) {
                continue;
//            }
        }

        SubfolderParser subfolder(aName);
        subfolder.createAllSubfolders(folder);
        auto writtingFolder = folder + subfolder.subfolder;
        
        auto sz = EgtbFile::computeSize(aName);

//        if (maxEndgameSize > 0 && sz > maxEndgameSize) {
//            std::cout << aName << " is larger than maxsize limit (" << (maxEndgameSize >> 30) << " G). Ignored!" << std::endl;
//            continue;
//        }

        if (!gen_single(writtingFolder, aName, egtbType, compressMode)) {
            std::cerr << "ERROR: generating UNSUCCESSFULLY " << aName << std::endl;
            exit(-1);
            break;
        }
        removeAllBuffers();
    }

    std::cout << "Generating ENDED for " << name << std::endl;
    return true;
}


