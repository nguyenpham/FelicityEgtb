//
//  EgtbNewGenFileMng2.cpp
//
//  Created by Tony Pham on 20/9/18.
//

#include <vector>
#include <thread>

#include "EgtbNewGenFileMng.h"
#include "TbLookupMng.h"
#include "../egtb/EgtbKey.h"

using namespace egtb;

static bool verifyDataOK = true;

bool EgtbNewGenFileMng::verifyData_loop(int threadIdx, EgtbFile* pTraEgtbFile) {
    assert(pTraEgtbFile);
    auto& rcd = threadRecordVec.at(threadIdx);

    {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "EgtbNewGenFileMng::verify BEGIN " << pTraEgtbFile->getName() << ", " << threadIdx << ") " << rcd.fromIdx << " -> " << rcd.toIdx << std::endl;
    }

    ExtBoard board;

    //    fromIdx = 30734648;
    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx && verifyDataOK; idx ++) {

        int curScore[] = {
            pTraEgtbFile->getScore(idx, Side::black, false),
            pTraEgtbFile->getScore(idx, Side::white, false)
        };

        if (egtbVerbose) {
            auto k = idx - rcd.fromIdx;
            if (k % (64L * 1024 * 1024) == 0) {
                auto elapsed = u64(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count()) / 1000.0;
                if (elapsed <= 0) elapsed = 1;
                {
                    std::lock_guard<std::mutex> thelock(printMutex);
                    std::cout << "verifyData threadIdx = " << threadIdx << ", idx = " << Lib::formatString(idx) << " of " << rcd.toIdx << " " << k * 100 / (rcd.toIdx - rcd.fromIdx) << "%, speed: " << Lib::formatSpeed((int)(k / elapsed)) << std::endl;
                }
            }
        }

        if (curScore[0] > EGTB_SCORE_MATE && curScore[1] > EGTB_SCORE_MATE) {
            continue;
        }

        auto b = setup(pTraEgtbFile, board, idx, FlipMode::none, Side::white);
        if (!b) {
            if (curScore[0] != EGTB_SCORE_ILLEGAL || curScore[1] != EGTB_SCORE_ILLEGAL) {
                verifyDataOK = false;
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cerr << "Error cannot create board when there is score not illegal, idx: " << idx << ", score: " << curScore[0] << ", " << curScore[1] << std::endl;
                return false;
            }
            continue;
        }

        for (int sd = 0; sd < 2; sd ++) {
            if (curScore[sd] > EGTB_SCORE_MATE) {
                continue;
            }

            Side side = static_cast<Side>(sd);

            int score1 = EgtbDb::getScore((const int*)board.pieceList, side);
            assert(score1 != EGTB_SCORE_MISSING);
            if (abs(score1) == MATE) {
                score1 = score1 > 0 ? EGTB_SCORE_MATE : -EGTB_SCORE_MATE;
            }

            if (curScore[sd] != score1) {
                verifyDataOK = false;
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cout << "Error: not mached scores for " << pTraEgtbFile->getName() << ", idx: " << idx << ", curScore[" << sd << "]: " << curScore[sd] << ", score1: " << score1 << std::endl;
                board.show();
                score1 = EgtbDb::getScore((const int*)board.pieceList, side);
                return false;
            }
        }
    }

    {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "EgtbGenFileMng::verify DONE " << pTraEgtbFile->getName() << ", " << threadIdx << std::endl;
    }
    return true;
}

bool EgtbNewGenFileMng::verifyDataWithTraditional(EgtbFile* traEgtbFile) {
    assert(traEgtbFile);

    verifyDataOK = true;

    begin = std::chrono::steady_clock::now();

    // Read in advance to avoid threads racing
    i64 idx = 0L;
    traEgtbFile->getScore(idx, Side::black, false);
    traEgtbFile->getScore(idx, Side::white, false);

    setupThreadRecords(traEgtbFile->getSize());
    {
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbNewGenFileMng::verifyData_loop, this, i, traEgtbFile));
        }
        
        verifyData_loop(0, traEgtbFile);
        
        for (auto && t : threadVec) {
            t.join();
        }

        if (!verifyDataOK) {
            return false;
        }
    }

    printf("verify %s done\n", traEgtbFile->getName().c_str());
    return true;
}

bool EgtbNewGenFileMng::verifyDataWithNew(EgtbNewGenFileMng* traEgtbFileMng, const std::string& name)
{
    if (name.find("m") == std::string::npos) {
        std::cerr << "Error: name must have 'm'" << std::endl;
        return false;
    }
    
    std::vector<std::string> nameVec;
    auto str = name;
    Lib::replaceString(str, "m", "");
    nameVec.push_back(str);
    
    str = name;
    Lib::replaceString(str, "m", "e");
    nameVec.push_back(str);

    str = name;
    Lib::replaceString(str, "m", "ee");
    nameVec.push_back(str);
    
    for(auto && str : nameVec) {
        auto traEgtbFile = traEgtbFileMng->getEgtbFile(str);
        if (traEgtbFile == nullptr) {
            std::cerr << "Error: missing endgame " << str << std::endl;
            return false;
        }
        
        if (!verifyDataWithTraditional(traEgtbFile)) {
            return false;
        }

        traEgtbFileMng->removeAllBuffers();
    }
    
    return true;
}

void EgtbNewGenFileMng::verifyData(const std::string& traFolder, const std::string& newFolder, const std::string& name, bool includeSubs) {
    std::cout << "EgtbNewGenFileMng::verifyData BEGIN\n";

    NameRecord record(name);
    if (!record.isValid() || record.type != EgtbType::newdtm) {
        std::cerr << "Error: wrong name: " << name << std::endl;
        return;
    }

    EgtbNewGenFileMng traEgtbFileMng;
    traEgtbFileMng.addFolder(traFolder);
    traEgtbFileMng.preload(EgtbMemMode::all);
    
    preload(newFolder, EgtbMemMode::all);
    std::cout << "egtbFileVec " << newFolder << ", sz: " << egtbFileVec.size() << std::endl;

    auto vec = parseName(name, includeSubs);
    
    for(auto && aName : vec) {
        NameRecord record2(aName);
        assert(record2.isValid());
        
        if (!record.isSameAttackers(record2)) {
            continue;
        }

        if (!verifyDataWithNew(&traEgtbFileMng, aName)) {
            exit(-1);
        }
        traEgtbFileMng.removeAllBuffers();
        removeAllBuffers();
    }
    

//    ExtBoard board;
//
//    NameRecord startRecord(name);
//
//    for (int i = 0; i < traEgtbFileMng.egtbFileVec.size(); ++i) {
//        auto traEgtbFile = traEgtbFileMng.egtbFileVec[i];
//
//        auto theName = traEgtbFile->getName();
//
//        NameRecord record(theName);
//        if (!startRecord.isSameAttackers(record)) {
//            continue;
//        }
//
//        std::cout << "\n" << i << " of " << traEgtbFileMng.egtbFileVec.size() << ") " << traEgtbFile->getName() << " sz: " << Lib::formatString(traEgtbFile->getSize()) << " at: " << Lib::currentTimeDate() << std::endl;
//
//        verifyDataWithTraditional(traEgtbFile);
//        traEgtbFileMng.removeAllBuffers();
//        removeAllBuffers();
//    }

    std::cout << "verifyData END\n";
}

