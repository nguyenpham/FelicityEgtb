//
//  Perpetuation.cpp
//  EgtbGen
//
//  Created by NguyenPham on 30/11/18.
//

#include "EgtbGenFileMng.h"

using namespace egtb;

i64 debugIdx = -1; //
int debugSd = W; // B;
i64 debugIdx2 = -1;

void simpleDebug(i64 debugIdx, EgtbFileWritting* egtbFile, const char* msg = nullptr) {
    if (debugIdx < 0) {
        return;
    }
    i64 idx = debugIdx;
    int scores[2] = {
        egtbFile->getScore(idx, Side::black), egtbFile->getScore(idx, Side::white)
    };
    if (msg) {
        std::cout << msg << std::endl;
    }
    std::cout << "idx: " << idx << ", scores: " << scores[0] << ", " << scores[1] << std::endl;
}

void simpleDebug(EgtbFileWritting* egtbFile, const char* msg = nullptr) {
    simpleDebug(debugIdx, egtbFile, msg);
    if (debugIdx2 >= 0) {
        simpleDebug(debugIdx2, egtbFile);
    }
}

bool EgtbGenFileMng::perpetuationFix(const std::string& name, bool includeSub)
{
    std::cout << "Find and fix perpetual checks / chases. BEGIN:\n\n";
    NameRecord mainRecord(name);
    if (!mainRecord.isValid() || !mainRecord.isBothArmed()) {
        std::cerr << "Error: name must be valid and has attackers for all sides\n";
        return false;
    }

    std::vector<std::string> vec;

    if (includeSub) {
        vec = parseName(name);
    } else {
        vec.push_back(name);
    }
    
    auto succCnt = 0;
    for(auto && aName : vec) {
        NameRecord record(aName);
        if (!mainRecord.isSameAttackers(record)) {
            continue;
        }

        auto it = nameMap.find(aName);
        if (it == nameMap.end()) {
            std::cerr << "Missing endgame " << aName << std::endl;
            continue;
        }

        egtbFile = (EgtbFileWritting *) it->second;
        assert(egtbFile && egtbFile->isBothArmed());
        
        if (!perpetuationFixSingle()) {
            std::cerr << "Error: cannot fix " << aName << std::endl;
            exit(-1);
            break;
        }
        succCnt++;
        removeAllBuffers();
    }
    
    std::cout << "\nFind and fix perpetual checks / chases for " << name << ", COMPLETED for #endgames: " << succCnt << std::endl;
    return true;
}

static const int EGTB_FLAG_CHECKED0 = 1 << 1;
static const int EGTB_FLAG_ESC0     = 1 << 2;

static const int EGTB_SCORE_TMP_CHECKED0 = EGTB_SCORE_MATE - 1 - EGTB_FLAG_CHECKED0;
//static const int EGTB_SCORE_TMP_ESC0     = EGTB_SCORE_MATE - 1 - EGTB_FLAG_ESC0;


bool EgtbGenFileMng::perpetuationFixSingle() {
    
    startTime = time(NULL);
    assert(egtbFile != nullptr && egtbFile->getMemMode() == EgtbMemMode::all);
    auto egtbName = egtbFile->getName();
    std::cout << std::endl << "Perpetuation fixing " << egtbName << ", " << Lib::formatString(egtbFile->getSize()) << ", " << Lib::currentTimeDate() << std::endl;

    egtbFile->createFlagBuffer();

    setupThreadRecords(egtbFile->getSize());

    {
        // Load all data into buffers
        i64 idx = 0;
        egtbFile->getScore(idx, Side::black); egtbFile->getScore(idx, Side::white);
        egtbFile->printPerpetuationStats("Begining:");
    }
    
    // clear some values
    i64 cnt = 0;
    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
        for(int sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto score = egtbFile->getScore(idx, side);
            if (score == EGTB_SCORE_DRAW || (score >= EGTB_SCORE_PERPETUAL_CHECKED && score <= EGTB_SCORE_PERPETUAL_EVASION)) {
                egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, sd);
                cnt++;
            }
        }
    }

    // checked
    {
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::perpetuationFix_initialise, this, i));
        }
        perpetuationFix_initialise(0);
        
        for (auto && t : threadVec) {
            t.join();
        }

        if (allThreadChangeCount()) {
            i64 checkCnt = 0, escCnt = 0, escCheckCnt = 0;
            for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
                for (int sd = 0; sd < 2; sd++) {
                    if (egtbFile->flag_is_cap(idx, sd)) {
                        egtbFile->flag_clear_cap(idx, sd);
                        auto flag = EGTB_FLAG_ESC0;
                        if (egtbFile->flag_is_side(idx, sd)) {
                            flag |= EGTB_FLAG_CHECKED0;
                        } else {
                            egtbFile->flag_set_side(idx, sd);
                        }
                        auto score = EGTB_SCORE_MATE - 1 - flag; assert(score & 1);
                        egtbFile->setBufScore(idx, score, sd);
                        auto score2 = egtbFile->getScore(idx, static_cast<Side>(sd));
                        assert(score == score2);
                    }
                    if (egtbFile->flag_is_side(idx, sd)) {
                        auto score = egtbFile->getScore(idx, static_cast<Side>(sd));
                        auto flag = EGTB_SCORE_MATE - 1 - score;
                        if (flag & EGTB_FLAG_CHECKED0) {
                            if (flag & EGTB_FLAG_ESC0) {
                                escCheckCnt++;
                            } else {
                                checkCnt++;
                            }
                        } else if (flag & EGTB_FLAG_ESC0) {
                            escCnt++;
                        }
                    }
                }
            }
            
            if (egtbVerbose) {
                std::cout << "After initiation, checkCnt: " << checkCnt << ", escCnt: " << escCnt << ", escCheckCnt: " << escCheckCnt << std::endl;
            }
        }
    }
    
    bool r = perpetuationFixSingle_main();
    
    if (r) {
        // write down
        auto path = egtbFile->getPath(0);
        auto pos = path.find_last_of("/\\");
        auto writingFolder = pos != std::string::npos ? path.substr(0, pos) : path;
        auto needVerifyData = true;
        genSingleEgtb_finish(writingFolder, egtbFile->isCompressed() ? CompressMode::compress : CompressMode::compress_none, needVerifyData);
    }

    if (r) {
        std::cout << "Completed " << egtbName << std::endl << std::endl;
    }
    return r;
}

void EgtbGenFileMng::perpetuationFix_initialise(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    
    ExtBoard board;

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getScore(idx, Side::black, false),
            egtbFile->getScore(idx, Side::white, false) };
        if (scores[0] != EGTB_SCORE_UNSET && scores[1] != EGTB_SCORE_UNSET) {
            continue;
        }
        
        bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white); assert(ok);
        
        for(int sd = 0; sd < 2; sd++) {
            if (scores[sd] != EGTB_SCORE_UNSET || !board.isIncheck(static_cast<Side>(sd))) {
                continue;
            }
            auto cnt = 0;
            MoveList moveList;
            auto side = static_cast<Side>(sd);
            auto xside = getXSide(side);
            
            board.gen(moveList, side, false); assert(!moveList.isEmpty());
            
            for(int i = 0; i < moveList.end; i++) {
                auto m = moveList.list[i];
                if (!board.isEmpty(m.dest)) {
                    continue;
                }
                
                board.make(m);
                if (!board.isIncheck(side)) {
                    auto tIdx = egtbFile->getKey(board).key;
                    auto score2 = egtbFile->getScore(tIdx, xside, false);
                    if (score2 == EGTB_SCORE_UNSET) {
                        egtbFile->flag_set_cap(tIdx, 1 - sd);
                        cnt++;
                    }
                }
                board.takeBack();
            }
            
            if (cnt > 0) {
                egtbFile->flag_set_side(idx, sd);
                egtbFile->setBufScore(idx, EGTB_SCORE_TMP_CHECKED0, sd);
                rcd.changes++;
            }
        }
    }
}

bool EgtbGenFileMng::perpetuationFixSingle_main() {
    
    assert(egtbFile->isBothArmed());
//    egtbFile->printPerpetuationStats("perpetuationFixSingle_main BEGIN:");

    // Main loops
    i64 perpetualCnt = 0;
    for(int ply = 1; ; ply++) {
        resetAllThreadRecordCounters();

        for(int sd = 0; sd < 2; sd++) {
            std::vector<std::thread> threadVec;
            for (int i = 1; i < threadRecordVec.size(); ++i) {
                threadVec.push_back(std::thread(&EgtbGenFileMng::perpetuationFix_reachableLoop, this, i, sd, ply));
            }
            perpetuationFix_reachableLoop(0, sd, ply);
            
            for (auto && t : threadVec) {
                t.join();
            }
        }
        
        i64 perpetualCnt2 = 0;
        {
            auto isEven = (ply & 1) == 0;
            auto iflag = isEven ? EGTB_FLAG_CHECKED0 : EGTB_FLAG_ESC0;
            for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
                for (int sd = 0; sd < 2; sd++) {
                    if (egtbFile->flag_is_cap(idx, sd)) {
                        egtbFile->flag_clear_cap(idx, sd);
                        auto side = static_cast<Side>(sd);
                        auto score = egtbFile->getScore(idx, side);
                        auto flag = EGTB_SCORE_MATE - 1 - score;
                        assert((iflag & flag) != 0);
                        flag &= ~iflag;
                        auto nScore = EGTB_SCORE_MATE - 1 - flag;
                        if (flag == 0) {
                            egtbFile->flag_clear_side(idx, sd);
                            nScore = EGTB_SCORE_UNSET;
                        }
                        egtbFile->setBufScore(idx, nScore, sd);
                    }
                    if (egtbFile->flag_is_side(idx, sd)) {
                        perpetualCnt2++;
                    }
                }
            }
        }

        if (perpetualCnt == perpetualCnt2) {
            break;
        }
        perpetualCnt = perpetualCnt2;
    }
    
//    simpleDebug(egtbFile, "perpetuationFixSingle_main before isBothArmed");
//    egtbFile->printPerpetuationStats("before perpetualPropaganda:");
    if (perpetualCnt < 2) {
        std::cout << "There are no perpetual checks. Ignored: " << egtbFile->getName() << std::endl;
        return false;
    }
    
    i64 cnt = 0;
    for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
        for(int sd = 0; sd < 2; sd++) {
            if (!egtbFile->flag_is_side(idx, sd)) {
                continue;
            }
            auto side = static_cast<Side>(sd);
            auto flag = EGTB_SCORE_MATE - 1 - egtbFile->getScore(idx, side);
            assert(flag & (EGTB_FLAG_CHECKED0 | EGTB_FLAG_ESC0));
            auto score2 = 0;
            if (flag & EGTB_FLAG_CHECKED0) {
                score2 = EGTB_SCORE_PERPETUAL_CHECKED;
                if (flag & EGTB_FLAG_ESC0) {
                    score2 = EGTB_SCORE_PERPETUAL_CHECKED_EVASION;
                }
                cnt++;
            } else if (flag & EGTB_FLAG_ESC0) {
                score2 = EGTB_SCORE_PERPETUAL_EVASION;
                cnt++;
            }
            
            egtbFile->setBufScore(idx, score2, sd);
        }
    }
    
    simpleDebug(egtbFile, "before verifyCheckAndEvasion");
    egtbFile->printPerpetuationStats("before verifyCheckAndEvasion:");
    
    egtbFile->clearFlagBuffer();
    
    verifyCheckAndEvasion();
    
    simpleDebug(egtbFile, "before perpetualPropaganda");
    egtbFile->printPerpetuationStats("before perpetualPropaganda:");
    
    {
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::perpetualPropaganda_fromCaptures, this, i));
        }
        perpetualPropaganda_fromCaptures(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
    }
    
    egtbFile->clearFlagBuffer();
    bool cont = true;
    
    for(int ply = 0; cont; ply++) {
        resetAllThreadRecordCounters();
        i64 changeCnt = 0;
        
        {
            std::vector<std::thread> threadVec;
            for (int i = 1; i < threadRecordVec.size(); ++i) {
                threadVec.push_back(std::thread(&EgtbGenFileMng::perpetualPropaganda_backward, this, i, ply));
            }
            
            perpetualPropaganda_backward(0, ply);
            
            for (auto && t : threadVec) {
                t.join();
            }
        }
        
        {
            std::vector<std::thread> threadVec;
            for (int i = 1; i < threadRecordVec.size(); ++i) {
                threadVec.push_back(std::thread(&EgtbGenFileMng::perpetualPropaganda, this, i, ply));
            }
            
            perpetualPropaganda(0, ply);
            
            for (auto && t : threadVec) {
                t.join();
            }
            
            changeCnt += allThreadChangeCount();
        }
        
        //            std::cout << "perpetualPropagandas, changeCnt: " << changeCnt << ", ply: " << ply << std::endl;
        if (changeCnt == 0) {
            break;
        }
    }

//    egtbFile->printPerpetuationStats("after perpetualPropaganda:");
//    simpleDebug(egtbFile, "perpetuationFixSingle_main have checks and esc");

    perpetuationFix_finish();
    simpleDebug(egtbFile, "FINISHED");
    return true;
}

void EgtbGenFileMng::perpetuationFix_reachableLoop(int threadIdx, int sd, int ply) {
    auto& rcd = threadRecordVec.at(threadIdx);
    
    auto side = static_cast<Side>(sd);
    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        if (egtbFile->flag_is_side(idx, sd)) {
            if (perpetuationFix_reachable(idx, side, ply)) {
                rcd.changes++;
            }
        }
    }
}

bool EgtbGenFileMng::perpetuationFix_reachable(i64 idx, Side side, int ply) {
    auto sd = static_cast<int>(side);
    auto isEven = (ply & 1) == 0;

    assert(idx >= 0 && ply >= 0);
    assert(egtbFile->flag_is_side(idx, sd));
    auto oScore = egtbFile->getScore(idx, side, false);
    
    auto oflag = EGTB_SCORE_MATE - 1 - oScore;
    auto iflag = isEven ? EGTB_FLAG_CHECKED0 : EGTB_FLAG_ESC0;
    if ((oflag & iflag) == 0) {
        return false;
    }
    
    ExtBoard board;
    bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white); assert(ok);

    auto xside = getXSide(side);
    auto xsd = 1 - sd;

    MoveList moveList;
    board.gen(moveList, side, false); assert(!moveList.isEmpty());
    
    auto db = false; //idx == debugIdx && side == Side::white;
    if (db) {
        board.printOut("idx: " + Lib::itoa(idx));
    }

    auto needflag = isEven ? EGTB_FLAG_ESC0 : EGTB_FLAG_CHECKED0;
    auto reached = false;
    
    for(int i = 0, cont = 1; i < moveList.end && cont; i++) {
        auto m = moveList.list[i];
        auto noCap = board.isEmpty(m.dest);
        if (!isEven && !noCap) {
            continue;
        }

        board.make(m);
        if (!board.isIncheck(side)) {
            if (noCap) {
                auto tIdx = egtbFile->getKey(board).key;
                
                if (db) {
                    board.printOut("after " + m.toString() + ", tIdx: " + Lib::itoa(tIdx));
                }
                
                if (egtbFile->flag_is_side(tIdx, xsd)) {
                    auto score2 = egtbFile->getScore(tIdx, xside, false);
                    auto theFlag = EGTB_SCORE_MATE - 1 - score2;
                    
                    if (!isEven && (theFlag & EGTB_FLAG_ESC0) != 0) {
                        reached = false;
                        cont = false;
                    } else
                    if ((theFlag & needflag) != 0) {
                        reached = true;
                    }
                } else if (isEven) {
                    auto score2 = egtbFile->getScore(tIdx, xside, false);
                    if (score2 <= EGTB_SCORE_DRAW) {
                        cont = 0;
                        reached = false;
                    }
                }
            } else {
                auto score = getScore(board, xside, AcceptScore::winning);
                if (score <= EGTB_SCORE_DRAW) {
                    cont = 0;
                    reached = false;
                }
            }
        }
        board.takeBack();
        
        if (reached && !isEven) {
            return true;
        }
    }

    if (reached) {
        return true;
    }

    egtbFile->flag_set_cap(idx, sd);
    return false;
}


void EgtbGenFileMng::perpetualPropaganda_fromCaptures(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);
    
    ExtBoard board;

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        int scores[2] = {
            egtbFile->getScore(idx, Side::black, false),
            egtbFile->getScore(idx, Side::white, false) };
        
        if (scores[0] != EGTB_SCORE_UNSET && scores[1] != EGTB_SCORE_UNSET) {
            continue;
        }
        
        bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white); assert(ok);
        
        for(int sd = 0; sd < 2; sd++) {
            if (scores[sd] != EGTB_SCORE_UNSET) {
                continue;
            }
            
            auto checkedCnt = 0, escCnt = 0;
            MoveList moveList;
            auto side = static_cast<Side>(sd);
            auto xside = getXSide(side);
            
            board.gen(moveList, side, false); assert(!moveList.isEmpty());
            
            for(int i = 0; i < moveList.end && !escCnt; i++) {
                auto m = moveList.list[i];
                if (board.isEmpty(m.dest)) {
                    continue;
                }
                
                board.make(m);
                if (board.pieceList_doBothHaveAttackers() && !board.isIncheck(side)) {
                    auto score2 = getScore(board, xside, AcceptScore::winning);
                    if (score2 >= EGTB_SCORE_PERPETUAL_CHECKED) {
                        if (score2 == EGTB_SCORE_PERPETUAL_CHECKED) {
                            checkedCnt++;
                        } else {
                            assert(score2 <= EGTB_SCORE_PERPETUAL_EVASION);
                            escCnt++;
                        }
                    }
                }
                board.takeBack();
            }

            if (checkedCnt + escCnt) {
                auto score = escCnt ? EGTB_SCORE_PERPETUAL_CHECKED : EGTB_SCORE_PERPETUAL_EVASION;
                egtbFile->setBufScore(idx, score, sd);
                rcd.changes++;
            }
        }
    }
}


void EgtbGenFileMng::perpetualPropaganda_backward(int threadIdx, int ply)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    
    ExtBoard board;
    
    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        auto needSetupBoard = true;
        
        for(int sd = 0; sd < 2; sd++) {
            if (egtbFile->flag_is_side(idx, sd)) {
                continue;
            }
            auto side = static_cast<Side>(sd);
            auto oScore = egtbFile->getScore(idx, side);

            if (oScore != EGTB_SCORE_DRAW && oScore != EGTB_SCORE_UNSET) {
                egtbFile->flag_set_side(idx, sd);
            }

            if (oScore < EGTB_SCORE_PERPETUAL_CHECKED) { //}  oScore != EGTB_SCORE_PERPETUAL_EVASION) {
                continue;
            }
            
            if (needSetupBoard) {
                needSetupBoard = false;
                bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white);
                assert(ok);
            }

            auto xside = getXSide(side);
            auto xsd = 1 - sd;
            
            auto db = false; //idx == debugIdx; // 5763661;
            if (db) {
                board.printOut("perpetualPropaganda_backward, idx = " + Lib::itoa(idx)
                               + ", score = " + std::to_string(oScore)
                               + ", sd = " + std::to_string(sd) + ", ply = " + std::to_string(ply));
            }
            
            Hist hist;
            MoveList moveList;
            board.genBackward(moveList, xside, false);
            
            for(int i = 0; i < moveList.end; i++) {
                auto move = moveList.list[i];
                Hist hist;
                board.make(move, hist);
                
                assert(hist.cap.isEmpty());
                
                if (!board.isIncheck(side)) {
                    assert(board.isValid());
                    
                    auto rIdx = egtbFile->getKey(board).key;
                    auto xscore = egtbFile->getScore(rIdx, xside, false);
                    
                    if (db) {
                        board.printOut("perpetualPropaganda_backward, after move " + move.toString());
                        std::cout << "rIdx: " << rIdx << ", xscore: " << xscore << ", sd: " << sd << std::endl;
                    }
                    if (xscore == EGTB_SCORE_UNSET ||
                        (xscore == EGTB_SCORE_DRAW && oScore == EGTB_SCORE_PERPETUAL_CHECKED)
                        ) {
                        egtbFile->flag_set_cap(rIdx, xsd);
                    }
                }
                board.takeBack(hist);
            }
        }
    }
}


void EgtbGenFileMng::perpetualPropaganda(int threadIdx, int ply)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    
    ExtBoard board;
    
    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        auto needBoard = true;
        for(int sd = 0; sd < 2; sd++) {
            if (!egtbFile->flag_is_cap(idx, sd)) {
                continue;
            }

            if (needBoard) {
                needBoard = false;
                bool ok = setup(egtbFile, board, idx, FlipMode::none, Side::white);
                assert(ok);
            }

            auto db = idx == debugIdx && (debugSd < 0 || debugSd == sd);
            if (db) {
                board.printOut("\n***** perpetualPropaganda, idx = " + Lib::itoa(idx) + ", sd = " + std::to_string(sd) + ", ply = " + std::to_string(ply));
            }

            auto side = static_cast<Side>(sd), xside = getXSide(side);
            assert(egtbFile->getScore(idx, static_cast<Side>(sd), false) == EGTB_SCORE_UNSET || egtbFile->getScore(idx, static_cast<Side>(sd), false) == EGTB_SCORE_DRAW);

            Hist hist;
            MoveList moveList;
            board.gen(moveList, side, false);
            
            auto drawCnt = 0, eCnt = 0, cCnt = 0, uCnt = 0;
            
            for(int i = 0; i < moveList.end; i++) {
                auto move = moveList.list[i];

                board.make(move, hist);
                
                if (!board.isIncheck(side)) {
                    assert(board.isValid());
                    
                    int score;
                    i64 rIdx = -1;
                    if (hist.cap.isEmpty()) {
                        rIdx = egtbFile->getKey(board).key;
                        score = egtbFile->getScore(rIdx, xside, false);
                    } else {
                        score = getScore(board, xside, AcceptScore::winning);
                    }
                    
                    if (db) {
                        board.printOut("perpetualPropaganda, after move " + move.toString() + ", score: " + std::to_string(score) + ", sd: " + std::to_string(sd) + ", ply: " + std::to_string(ply) + ", rIdx: " + Lib::itoa(rIdx));
                    }

                    assert(score >= EGTB_SCORE_DRAW);
                    
                    if (score == EGTB_SCORE_DRAW) {
                        drawCnt++;
                    } else if (score == EGTB_SCORE_PERPETUAL_EVASION) {
                        eCnt++;
                    } else if (score == EGTB_SCORE_PERPETUAL_CHECKED) {
                        cCnt++;
                    } else if (score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) {
                        eCnt++; cCnt++;
                    } else if (score == EGTB_SCORE_UNSET) {
                        if (uCnt || !board.isIncheck(xside)) {
                            uCnt++;
                        }
                    }
                }
                board.takeBack(hist);
                
                if (eCnt) {
                    break;
                }
            }
            
            auto newScore = eCnt ? EGTB_SCORE_PERPETUAL_CHECKED : (cCnt && drawCnt + uCnt == 0) ? EGTB_SCORE_PERPETUAL_EVASION : EGTB_SCORE_UNSET;

            if (newScore != EGTB_SCORE_UNSET) {
                bool r = egtbFile->setBufScore(idx, newScore, sd); assert(r);
                egtbFile->flag_clear_cap(idx, sd);
                rcd.changes++;
                if (db) {
                    board.printOut("perpetualPropaganda, set evasion for idx = " + Lib::itoa(idx) + ", sd = " + std::to_string(sd));
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EgtbGenFileMng::perpetuationFix_finish() {
    
    // Clean and show some results
    int maxDTM = 0;
    i64 pepertualChecks = 0, pepertualEvasions = 0, unset = 0, pepertualChecksEvasions = 0, pepertualWins = 0, pepertualLoses = 0;
    for (int sd = 0; sd < 2; sd++) {
        auto side = static_cast<Side>(sd);
        for (i64 idx = 0; idx < egtbFile->getSize(); idx++) {
            int score = egtbFile->getScore(idx, side);
            if (score > 0) pepertualWins++; else pepertualLoses++;
            if (abs(score) > EGTB_SCORE_PERPETUAL_BEGIN) {
                
            } else if (score > EGTB_SCORE_MATE) {
                switch (score) {
                    case EGTB_SCORE_UNSET:
                        unset++;
                        egtbFile->setBufScore(idx, EGTB_SCORE_DRAW, sd);
                        break;
                    case EGTB_SCORE_PERPETUAL_CHECKED:
                        pepertualChecks++;
                        if (egtbVerbose && pepertualChecks < 10) {
                            ExtBoard board;
                            setup(egtbFile, board, idx, FlipMode::none, Side::white);
                            board.printOut("Perpetual check idx: " + Lib::itoa(idx) + ", sd: " + Lib::itoa(sd));
                        }
                        break;
                    case EGTB_SCORE_PERPETUAL_CHECKED_EVASION:
                        pepertualChecksEvasions++;
                        if (egtbVerbose && pepertualChecksEvasions < 10) {
                            ExtBoard board;
                            setup(egtbFile, board, idx, FlipMode::none, Side::white);
                            board.printOut("Perpetual evasion idx: " + Lib::itoa(idx) + ", sd: " + Lib::itoa(sd));
                        }
                        break;
                    case EGTB_SCORE_PERPETUAL_EVASION:
                        pepertualEvasions++;
                        if (egtbVerbose && pepertualEvasions < 10) {
                            ExtBoard board;
                            setup(egtbFile, board, idx, FlipMode::none, Side::white);
                            board.printOut("Perpetual evasion idx: " + Lib::itoa(idx) + ", sd: " + Lib::itoa(sd));
                        }
                        break;
                        
                    default:
                        break;
                }
            } else if (score != EGTB_SCORE_DRAW) {
                maxDTM = std::max(maxDTM, EGTB_SCORE_MATE - abs(score));
            }
        }
    }

    if (egtbVerbose) {
        auto sz = egtbFile->getSize(), sz2 = 2 * sz;
        std::cout << "Size: " << sz
        << ", perpetual checks: " <<  Lib::number_percent(pepertualChecks, sz2)
        << ", evasions: " <<  Lib::number_percent(pepertualEvasions, sz2)
        << ", checks-evasions: " <<  Lib::number_percent(pepertualChecksEvasions, sz2)
        << ", pepertualWins: " << Lib::number_percent(pepertualWins, sz2)
        << ", pepertualLoses: " << Lib::number_percent(pepertualLoses, sz2)

        // << ", startCheckingCnt: " <<  Lib::number_percent(startCheckingCnt, sz2)
        << std::endl;
        
        std::cout << "Stats:\n" << egtbFile->createStatsString() << std::endl;
    }
    egtbFile->getHeader()->setDtm_max(maxDTM);

}

void EgtbGenFileMng::verifyCheckAndEvasion()
{
    for (int ply = 0; ; ply++) {
        std::cout << "verifyCheckAndEvasion, ply: " << ply << std::endl;
        
        resetAllThreadRecordCounters();
        
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::verifyCheckAndEvasion_loop, this, i));
        }
        
        verifyCheckAndEvasion_loop(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        
        simpleDebug(egtbFile, ("in verifyCheckAndEvasion_loop, ply: " + std::to_string(ply)).c_str());
        egtbFile->printPerpetuationStats(("in verifyCheckAndEvasion_loop, ply: " + std::to_string(ply)).c_str());
        
        if (allThreadChangeCount() == 0) {
            break;
        }
    }
    
}

void EgtbGenFileMng::verifyCheckAndEvasion_loop(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    
    for (int sd = 0; sd < 2; sd++) {
        auto side = static_cast<Side>(sd);
        for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
            int score = egtbFile->getScore(idx, side);
            if (score == EGTB_SCORE_PERPETUAL_EVASION || score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) {
                if (!verifyEvasion(idx, side)) {
                    score = score == EGTB_SCORE_PERPETUAL_EVASION ? EGTB_SCORE_UNSET : EGTB_SCORE_PERPETUAL_CHECKED;
                    egtbFile->setBufScore(idx, score, sd);
                    rcd.changes++;
                }
            }
            if (score == EGTB_SCORE_PERPETUAL_CHECKED || score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) {
                if (!verifyCheck(idx, side)) {
                    score = score == EGTB_SCORE_PERPETUAL_CHECKED ? EGTB_SCORE_UNSET : EGTB_SCORE_PERPETUAL_EVASION;
                    egtbFile->setBufScore(idx, EGTB_SCORE_UNSET, sd);
                    rcd.changes++;
                }
            }
        }
    }
}

bool EgtbGenFileMng::verifyEvasion(i64 idx, Side side)
{
    assert(egtbFile->getScore(idx, side, false) == EGTB_SCORE_PERPETUAL_EVASION || egtbFile->getScore(idx, side, false) == EGTB_SCORE_PERPETUAL_CHECKED_EVASION);
    
    ExtBoard board;
    setup(egtbFile, board, idx, FlipMode::none, Side::white);
    
    auto xside = getXSide(side);
    Hist hist;
    MoveList moveList;
    board.gen(moveList, side, false); assert(!moveList.isEmpty());
    auto ok = true;

    auto db = idx == debugIdx;
    if (db) {
        board.printOut("verifyEvasion, idx: " + Lib::itoa(idx) + ", sd: " + Lib::itoa(static_cast<int>(side)));
    }

    for(int i = 0; i < moveList.end && ok; i++) {
        auto m = moveList.list[i];
        
        board.make(m, hist);
        if (!board.isIncheck(side)) {
            int score;
            if (hist.cap.isEmpty()) {
                auto idx2 = egtbFile->getKey(board).key;
                score = egtbFile->getScore(idx2, xside, false);
            } else {
                assert(board.pieceList_countStrong() > 0);
                score = getScore(board, xside, AcceptScore::winning);
                assert(score != EGTB_SCORE_MISSING);
            }
            
            if (score == EGTB_SCORE_UNSET || (score >= -EGTB_SCORE_MATE && score <= EGTB_SCORE_DRAW)) {
                ok = false;
            }
            if (db) {
                board.printOut("verifyEvasion, after move " + m.toString() + ", score: " + std::to_string(score) + ", sd: " + std::to_string(static_cast<int>(side)) + ", ok: " + std::to_string(ok));
            }
        }
        board.takeBack(hist);
    }
    
    return ok;
}


bool EgtbGenFileMng::verifyCheck(i64 idx, Side side)
{
    assert(egtbFile->getScore(idx, side, false) == EGTB_SCORE_PERPETUAL_CHECKED || egtbFile->getScore(idx, side, false) == EGTB_SCORE_PERPETUAL_CHECKED_EVASION);
    
    ExtBoard board;
    setup(egtbFile, board, idx, FlipMode::none, Side::white);
    
    Side xside = getXSide(side);
    MoveList moveList;
    board.gen(moveList, side, false); assert(!moveList.isEmpty());
    
    auto db = idx == debugIdx || idx == debugIdx2;
    if (db) {
        board.printOut("verifyCheck, idx: " + Lib::itoa(idx) + ", sd: " + Lib::itoa(static_cast<int>(side)));
    }
    

    auto ok = false;
    for(int i = 0; i < moveList.end && !ok; i++) {
        auto m = moveList.list[i];
        
        if (!board.isEmpty(m.dest)) {
            continue;
        }
        
        board.make(m);
        if (!board.isIncheck(side)) {
            auto idx2 = egtbFile->getKey(board).key;
            int score = egtbFile->getScore(idx2, xside, false);
            
            if (score == EGTB_SCORE_PERPETUAL_EVASION) {
                ok = true;
            }
            if (db) {
                board.printOut("verifyCheck, after move " + m.toString() + ", score: " + std::to_string(score) + ", sd: " + std::to_string(static_cast<int>(side)) + ", ok: " + std::to_string(ok));
            }
        }
        board.takeBack();
    }

    return ok;
}


void EgtbGenFileMng::numberisePerpetuation()
{
    for (int ply = 0; ; ply++) {
        std::cout << "numberisePerpetuation, ply: " << ply << std::endl;
        
        resetAllThreadRecordCounters();
        
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbGenFileMng::numberisePerpetuation_loop, this, i));
        }
        
        numberisePerpetuation_loop(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
        
        simpleDebug(egtbFile, ("in numberisePerpetuation, ply: " + std::to_string(ply)).c_str());
        egtbFile->printPerpetuationStats(("in numberisePerpetuation, ply: " + std::to_string(ply)).c_str());
        
        if (allThreadChangeCount() == 0) {
            break;
        }
    }
    
}

void EgtbGenFileMng::numberisePerpetuation_loop(int threadIdx)
{
    auto& rcd = threadRecordVec.at(threadIdx);
    
    ExtBoard board;

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
        auto needBoard = true;
        for (int sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto oScore = egtbFile->getScore(idx, side);

            if (oScore < EGTB_SCORE_PERPETUAL_CHECKED || oScore > EGTB_SCORE_PERPETUAL_EVASION) {
                continue;
            }
            
            if (needBoard) {
                needBoard = false;
                setup(egtbFile, board, idx, FlipMode::none, Side::white);
            }

            MoveList moveList;
            board.gen(moveList, side, false); assert(!moveList.isEmpty());
            
            auto xside = getXSide(side);
            auto bestScore = -EGTB_SCORE_MATE, bestPerpetualScore = -EGTB_SCORE_PERPETUAL_MATE;
            auto checkedCnt = 0, escCnt = 0;
            auto cont = true;
            for(int i = 0; i < moveList.end && cont; i++) {
                auto m = moveList.list[i];
                
                board.make(m);
                if (!board.isIncheck(side)) {
                    int score2;
                    
                    auto noCap = board.histVec.back().cap.isEmpty();
                    if (noCap) {
                        auto idx2 = egtbFile->getKey(board).key;
                        score2 = egtbFile->getScore(idx2, xside, false);
                    } else {
                        score2 = getScore(board, xside, AcceptScore::winning);
                    }
                    
                    if (score2 >= EGTB_SCORE_PERPETUAL_CHECKED && score2 <= EGTB_SCORE_PERPETUAL_EVASION) {
                        if (!noCap) {
                            std::cerr << "FATAL ERROR: the sub endgame has not been processed propertly by fixcc." << std::endl;
                            board.printOut("Board of that subendgame:");
                            exit(0);
                        }
                        if (oScore == EGTB_SCORE_PERPETUAL_CHECKED) {
                            checkedCnt++;
                        } else {
                            escCnt++;
                        }
                        cont = false;
                    } else {
                        if (abs(score2) >= EGTB_SCORE_PERPETUAL_BEGIN) {
                            bestPerpetualScore = std::max(bestScore, -score2);
                        } else if (score2 < EGTB_SCORE_MATE) {
                            bestScore = std::max(bestScore, -score2);
                        }
                    }
                }
                board.takeBack();
            }
            
            if (!cont) {
                continue;
            }
            
            assert(checkedCnt + escCnt == 0);
            
            if (oScore == EGTB_SCORE_PERPETUAL_CHECKED || oScore == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) {
                assert(bestScore <= EGTB_SCORE_DRAW);
                assert(bestPerpetualScore > 0);
                bestScore = bestPerpetualScore;
            } else {
                assert(bestScore < EGTB_SCORE_DRAW);
                assert(bestPerpetualScore < 0);
                if (bestPerpetualScore != -EGTB_SCORE_PERPETUAL_MATE) {
                    bestScore = bestPerpetualScore;
                } else {
                    bestScore = -EGTB_SCORE_PERPETUAL_MATE - bestScore - EGTB_SCORE_MATE;
                    assert(bestScore > -EGTB_SCORE_PERPETUAL_BEGIN && bestScore <= -EGTB_SCORE_PERPETUAL_MATE);
                }
            }
            
            if (bestScore < 0) bestScore++;
            else if (bestScore > 0) bestScore--;
            
            egtbFile->setBufScore(idx, bestScore, sd);
            rcd.changes++;
        }
    }
}
