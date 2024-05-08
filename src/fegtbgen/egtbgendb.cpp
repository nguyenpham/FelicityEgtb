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

#include "egtbgendb.h"
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
    allCount[0] = allCount[1] = 0;
    
    /// For Chess only
    pawnCount[0] = pawnCount[1] = 0;
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
        if (type == PieceType::pawn) {
            pawnCount[sd]++;
        }
#else
        allCount[sd]++;
        if (t >= ROOK) {
            attackerMats[sd] += mat;
            attackerCount[sd]++;
        }
#endif
    }
    
    return pieceCount[0][KING] == 1 && pieceCount[1][KING] == 1
    && attackerMats[W] > 0
    && attackerMats[W] >= attackerMats[B]
    && !isLimited()
    ;
}

bool NameRecord::isLimited() const
{
#ifdef _FELICITY_CHESS_
    /// No limit for chess
    return false;
#else
    /// Don't count endgames:
    /// - B has attackers
    /// - W has two attackers with a Rook
    return attackerCount[B] > 0 ||
            (attackerCount[W] > 1 && pieceCount[W][ROOK] > 0);
#endif
}

bool NameRecord::isValid() const
{
    return ok;
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
    auto attackers = attackerCount[W] + attackerCount[B];
    auto attackers_o = other.attackerCount[W] + other.attackerCount[B];
    
    if (attackers != attackers_o) {
        return attackers < attackers_o;
    }

#ifdef _FELICITY_CHESS_
    auto pawns1 = pawnCount[0] + pawnCount[1];
    auto pawns2 = other.pawnCount[0] + other.pawnCount[1];
    if (pawns1 < pawns2) {
        return true;
    }
    if (pawns1 > pawns2) {
        return false;
    }
#else
    auto all = allCount[0] + allCount[1];
    auto all_o = other.allCount[0] + other.allCount[1];

    if (all != all_o) {
        return all < all_o;
    }
    if (allCount[W] != other.allCount[W]) {
        return allCount[W] < other.allCount[W];
    }
#endif
    
    if (attackerCount[W] != other.attackerCount[W]) {
        return attackerCount[W] < other.attackerCount[W];
    }
    
    if (attackerMats[W] != other.attackerMats[W]) {
        return attackerMats[W] < other.attackerMats[W];
    }

    return attackerMats[W] + attackerMats[B] < other.attackerMats[W] + other.attackerMats[B];
}

std::string NameRecord::getSubfolder() const
{
    if (attackerCount[0] == 0 || attackerCount[1] == 0) {
        return std::to_string(attackerCount[0] + attackerCount[1]);
    }
    return std::to_string(attackerCount[W]) + "-" + std::to_string(attackerCount[B]);
}

////////////////////////////////////////////
void EgtbGenDb::gen_thread_init(int threadIdx) {
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
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::black);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, Side::white);
            continue;
        }
        
//        rcd.board->printOut();
        assert(rcd.board->isValid());
        
        bool inchecks[] = { rcd.board->isIncheck(Side::black), rcd.board->isIncheck(Side::white) };
        
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
        std::cout << "EgtbGenFileMng::genSingleEgtb_init done, threadIdx: " << threadIdx << std::endl;
    }
}


int EgtbGenDb::probe_gen(EgtbBoard& board, i64 idx, Side side) {
    
    auto ok = egtbFile->setupBoard(board, idx, FlipMode::none, Side::white);
    assert(ok);
    
    auto legalCount = 0, unsetCount = 0, bestScore = EGTB_SCORE_UNSET;
    auto xside = getXSide(side);
    Hist hist;
    for(auto move
        : board.gen(side)) {
        board.make(move, hist);
        if (!board.isIncheck(side)) {
            legalCount++;
            
            /// If the move is a capture or a promotion, it should probe from sub-endgames
            auto internal = hist.cap.isEmpty() && move.promotion == PieceType::empty;

            int score;
            if (internal) {
                auto idx2 = egtbFile->getKey(board).key;
                score = egtbFile->getScore(idx2, xside, false);
            } else {
                if (!board.hasAttackers()) {
                    score = EGTB_SCORE_DRAW;
                } else {
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
    }
    
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

/// Generate within a thread
void EgtbGenDb::gen_thread(int threadIdx, int sd, int ply) {
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

        auto bestScore = probe_gen(*rcd.board, idx, side);
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

void EgtbGenDb::gen_forward(const std::string& folder) {
    if (egtbVerbose) {
        std::cout << "\tGenerate forwardly!" << std::endl;
    }

    auto ply = 0;
    auto side = Side::white;
    
    /// Load temp files
    auto wLoop = 0, bLoop = 0;
    if (useTempFiles) {
        wLoop = egtbFile->readFromTmpFile(folder, Side::white);
        bLoop = egtbFile->readFromTmpFile(folder, Side::black);
    }
    
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
            threadVec.push_back(std::thread(&EgtbGenDb::gen_thread_init, this, i));
        }
        
        gen_thread_init(0);
        
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
            threadVec.push_back(std::thread(&EgtbGenDb::gen_thread, this, i, sd, ply));
        }
        gen_thread(0, sd, ply);
        
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
            egtbFile->writeTmpFile(folder, side, ply);
        }
        
        if (callLoopChangeCnt == 0) {
            tryCnt--;
        } else {
            tryCnt = 2;
        }
    }
}

/// Gen a single endgame
bool EgtbGenDb::gen_single(const std::string& folder, const std::string& name, EgtbType egtbType, CompressMode compressMode)
{
    
    startTime = time(NULL);

    assert(egtbFile == nullptr);
    egtbFile = new EgtbGenFile();
    egtbFile->create(name, egtbType);

//    auto writtingFolder = _folder + egtbFile->getSubfolder();

    auto sz = egtbFile->getSize();
    auto bufsz = sz * 2;
    if (egtbFile->isTwoBytes()) bufsz += bufsz;
    if (useBackward) bufsz += sz / 2;

    std::cout << std::endl << "Start generating " << name << ", " << GenLib::formatString(sz) << (egtbFile->isTwoBytes() ? ", 2b/item" : "") << ", main mem sz: " << GenLib::formatString(bufsz) << ", " << GenLib::currentTimeDate() << std::endl;

    egtbFile->createBuffersForGenerating();
    assert(egtbFile->isValidHeader());
    
    setupThreadRecords(egtbFile->getSize());

    /// WARNING: not use backward
    gen_forward(folder);

    auto r = gen_finish(folder, compressMode);
    if (r) {
        std::cout << "Generated successfully " << name << std::endl << std::endl;
    }
    return r;

}

bool EgtbGenDb::gen_finish(const std::string& folder, CompressMode compressMode, bool needVerify)
{
    gen_finish_adjust_scores();

    assert(egtbFile->isValidHeader());
    egtbFile->getHeader()->addSide(Side::white);
    egtbFile->getHeader()->addSide(Side::black);

    auto elapsed_secs = std::max(1, (int)(time(NULL) - startTime));
    total_elapsed += elapsed_secs;
    if (egtbVerbose) {
        std::cout << "Completed generating " << egtbFile->getName() << ", elapse: " << GenLib::formatPeriod(elapsed_secs) << ", speed: " << GenLib::formatSpeed((int)(egtbFile->getSize() / elapsed_secs)) << std::endl;
    }

    addEgtbFile(egtbFile);

    std::cout << "Total generating time (not including verifying): " << GenLib::formatPeriod(total_elapsed) << std::endl;

    /// Verify
    if (needVerify && !verifyData(egtbFile)) {
        std::cerr << "Error: verify FAILED for " << egtbFile->getName() << std::endl;
        exit(1);
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

void EgtbGenDb::gen_finish_adjust_scores()
{
    /// Clean and show some results
    int maxDTM = 0;
    i64 unset = 0;
    
    for (auto sd = 0; sd < 2; sd++) {
        auto side = static_cast<Side>(sd);
        for (i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            auto score = egtbFile->getScore(idx, side);
            if (score == EGTB_SCORE_UNSET) {
                unset++;
                egtbFile->setBufScore(idx, EGTB_SCORE_DRAW, side);

            } else {
                maxDTM = std::max(maxDTM, EGTB_SCORE_MATE - abs(score));
            }
        }
    }

    if (egtbVerbose) {
        auto sz = egtbFile->getSize();
        std::cout << "Size: " << sz
        << std::endl;
        
        std::cout << "Stats:\n" << egtbFile->createStatsString() << std::endl;
    }
    egtbFile->getHeader()->setDtm_max(maxDTM);
}

bool EgtbGenDb::gen_all(std::string folder, const std::string& name, EgtbType egtbType, CompressMode compressMode)
{
    std::cout << "Missing endgames, they will be generated:\n";

    std::vector<std::string> vec = showMissing(name);

    if (!folder.empty()) {
        GenLib::createFolder(folder);
        folder += "/";
    }

    for(auto && aName : vec) {
        auto p = nameMap.find(aName);
        if (p != nameMap.end()) {
            continue;
        }

//        SubfolderParser subfolder(aName);
//        subfolder.createAllSubfolders(folder);
//        auto writtingFolder = folder + subfolder.subfolder;
//        
//        auto sz = EgtbFile::computeSize(aName);
//
//        if (maxEndgameSize > 0 && sz > maxEndgameSize) {
//            std::cout << aName << " is larger than maxsize limit (" << (maxEndgameSize >> 30) << " G). Ignored!" << std::endl;
//            continue;
//        }
        
        NameRecord nameRecord(aName);
        
        auto writtingFolder = folder + nameRecord.getSubfolder();
        GenLib::createFolder(writtingFolder);


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


