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

DataItemMode EgtbGenDb::dataItemMode = DataItemMode::two;
bool EgtbGenDb::twoBytes = false; // per item
bool EgtbGenDb::useTempFiles = false;
bool EgtbGenDb::useBackward = true;
bool EgtbGenDb::verifyMode = true;
i64 EgtbGenDb::maxEndgameSize = -1;

#ifdef _FELICITY_CHESS_
static const std::string pieceSorting = "0987654321";

static const std::set<std::string> knownTwoBytesEndgameSet {
    "kqpkq", "kqnkp", "kqbkp", "kqrkp", "kqqkp", "knnpk", "kbnpk", "kbbpk",
    "krnpk", "krbpk", "krrpk", "kqnpk", "kqbpk", "kqrpk", "kqqpk", "knkpp",
    "kbkpp", "krkpp", "kqkpp", "knpkp", "kbpkp", "krpkp", "kqpkp", "knppk",
    "kbppk", "krppk", "kqppk", "kppkp", "kpppk"
};

#else

static const std::string pieceSorting = "09218765430";

static const std::set<std::string> knownTwoBytesEndgameSet {
    "",
};

#endif

////////////////////////////////////////////
bool NameRecord::parse(const std::string& _name)
{
    memset(pieceCount, 0, sizeof(pieceCount));
    
    name = _name;
    type = EgtbType::dtm;
    pawnCount[0] = pawnCount[1] = 0; /// for Chess only
    sortingSides[0] = sortingSides[1] = "";
    attackerSides[0] = attackerSides[1] = "";

    auto prevCh = '9';
    for(auto i = 0, sd = W; i < name.size(); i++) {
        char ch = name[i];
        auto type = Funcs::charactorToPieceType(ch);
        if (type == PieceType::empty) {
            std::cerr << "Error: don't know the charactor " << ch << " in name " << name << std::endl;
            return false;
        }
        
        auto t = static_cast<int>(type);

        auto c = pieceSorting[t];

        if (type == PieceType::king) {
            prevCh = c;
            if (i > 0) {
                sd = B;
            }
            pieceCount[sd][t]++;
            continue;
        }

        pieceCount[sd][t]++;
        
        if (prevCh < c) {
            return false;
        }
        sortingSides[sd] += c;
        prevCh = c;

#ifdef _FELICITY_CHESS_
        if (type == PieceType::pawn) {
            pawnCount[sd]++;
        }
        attackerSides[sd] += c;
#else
        if (t >= ROOK) {
            attackerSides[sd] += c;
        }
        allSides[sd] += c;
#endif
    }
    
#ifdef _FELICITY_CHESS_
    /// Check promotion exceed the limit
    if (name.size() > 6) {
        int promotionCnt[2] = { 0, 0 };
        for(auto sd = 0; sd < 2; sd++) {
            for(auto t = QUEEN; t < PAWN; t++) {
                auto m = t == QUEEN ? 1 : 2;
                if (pieceCount[sd][t] > m) {
                    if (pieceCount[sd][t] > m + 8) {
                        return false;
                    }
                    promotionCnt[sd] += pieceCount[sd][t] - m;
                }
            }
        }
        
        if (promotionCnt[0] + pieceCount[0][PAWN] > 8 || promotionCnt[1] + pieceCount[1][PAWN] > 8) {
            return false;
        }
    }
#endif
    
    return pieceCount[0][KING] == 1 && pieceCount[1][KING] == 1
    && !sortingSides[W].empty()
    && sortingSides[W] >= sortingSides[B]
    
#ifdef _FELICITY_CHESS_
#else
    && attackerSides[0].size() + attackerSides[1].size()
#endif
//    && !isLimited()
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
//    return attackerCount[B] > 0 ||
//            (attackerCount[W] > 1 && pieceCount[W][ROOK] > 0);
    return false;
#endif
}

bool NameRecord::isValid() const
{
    return ok;
}

bool NameRecord::isValid(const std::string& name)
{
    NameRecord record(name);
    return record.isValid();
}

bool NameRecord::isBothArmed() const
{
    return !sortingSides[W].empty() && !sortingSides[B].empty();
}

bool NameRecord::hasAttackers() const
{
    return !sortingSides[W].empty();
}

bool NameRecord::isSmaller(const NameRecord& other) const
{
#ifdef _FELICITY_XQ_
    auto all = allSides[W].size() + allSides[B].size();
    auto all_o = other.allSides[W].size() + other.allSides[B].size();
    
    if (all != all_o) {
        return all < all_o;
    }
#endif
    
    auto attackers = attackerSides[W].size() + attackerSides[B].size();
    auto attackers_o = other.attackerSides[W].size() + other.attackerSides[B].size();
    
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
    
    if (sortingSides[W].size() != other.sortingSides[W].size()) {
        return sortingSides[W].size() < other.sortingSides[W].size();
    }

    if (sortingSides[W] != other.sortingSides[W]) {
        return sortingSides[W] < other.sortingSides[W];
    }
    
    return sortingSides[B] < other.sortingSides[B];

#else
    if (attackerSides[W] != other.attackerSides[W]) {
        return attackerSides[W] < other.attackerSides[W];
    }

    if (attackerSides[B] != other.attackerSides[B]) {
        return attackerSides[B] < other.attackerSides[B];
    }
    
    if (sortingSides[W].size() != other.sortingSides[W].size()) {
        return sortingSides[W].size() < other.sortingSides[W].size();
    }

    if (sortingSides[B].size() != other.sortingSides[B].size()) {
        return sortingSides[B].size() < other.sortingSides[B].size();
    }

    if (sortingSides[W] != other.sortingSides[W]) {
        return sortingSides[W] < other.sortingSides[W];
    }
    
    return sortingSides[B] < other.sortingSides[B];

#endif
}

std::string NameRecord::getSubfolder() const
{
    if (attackerSides[0].empty() || attackerSides[1].empty()) {
        return std::to_string(attackerSides[0].size() + attackerSides[1].size());
    }
    return std::to_string(attackerSides[W].size()) + "-" + std::to_string(attackerSides[B].size());
}

/// Gen a single endgame
bool EgtbGenDb::gen_single(int egtbidx, const std::string& folder, const std::string& name, EgtbType egtbType, CompressMode compressMode)
{
    time_start = Funcs::now();

    /// Detect size of each item
    switch (EgtbGenDb::dataItemMode) {
        case DataItemMode::one:
            EgtbGenDb::twoBytes = false;
            break;
        case DataItemMode::two:
            EgtbGenDb::twoBytes = true;
            break;

        default:
        {
            NameRecord nameRecord(name);
            auto attackerCnt = nameRecord.attackerSides[0].size() + nameRecord.attackerSides[1].size();
            
            EgtbGenDb::twoBytes = false;
            if (attackerCnt > 4
                || knownTwoBytesEndgameSet.find(name) != knownTwoBytesEndgameSet.end()) {
                EgtbGenDb::twoBytes = true;
            }
            break;
        }
    }

    assert(egtbFile == nullptr);
    egtbFile = new EgtbGenFile();
    egtbFile->create(name, egtbType);

    auto sz = egtbFile->getSize();
    auto bufsz = sz * 2;
    if (egtbFile->isTwoBytes()) bufsz += bufsz;
    if (useBackward) bufsz += sz / 2;

    startTimeString = GenLib::currentTimeDate();
    std::cout << std::endl << egtbidx + 1 << ") Start generating " << name << ", " << GenLib::formatString(sz) << (egtbFile->isTwoBytes() ? ", 2 bytes/item" : "") << ", main mem sz: " << GenLib::formatString(bufsz) << ", " << startTimeString << std::endl;

    egtbFile->createBuffersForGenerating();
    assert(egtbFile->isValidHeader());
    
    setupThreadRecords(egtbFile->getSize());

    if (useBackward) {
        gen_backward(folder);
    } else {
        gen_forward(folder);
    }

    auto r = gen_finish(folder, compressMode, verifyMode);
    if (r) {
        std::cout << "Generated successfully " << name << std::endl << std::endl;
    }
    return r;
}

bool EgtbGenDb::gen_finish(const std::string& folder, CompressMode compressMode, bool needVerify)
{
#ifdef _FELICITY_XQ_
    perpetuation_process();
#endif
    
    gen_finish_adjust_scores();

    assert(egtbFile->isValidHeader());
    egtbFile->getHeader()->addSide(Side::white);
    egtbFile->getHeader()->addSide(Side::black);

    time_completed = Funcs::now();
    elapsed_gen = time_completed - time_start;
    total_elapsed_gen += elapsed_gen;
    
    if (egtbVerbose) {
        std::cout << "Completed generating " << egtbFile->getName() << ", elapse: " << GenLib::formatPeriod(int(elapsed_gen / 1000)) << ", speed: " << GenLib::formatSpeed((int)(egtbFile->getSize() * 1000 / std::max(1, int(elapsed_gen)))) << std::endl;
    }

    addEgtbFile(egtbFile);

    /// Verify
    elapsed_verify = 0;
    if (needVerify) {
        if (!verifyData(egtbFile)) {
            std::cerr << "Error: verify FAILED for " << egtbFile->getName() << std::endl;
            exit(1);
            return false;
        }
        
        elapsed_verify = Funcs::now() - time_completed;
        total_elapsed_verify += elapsed_verify;
    }
    
    egtbFile->checkAndConvert2bytesTo1();

    std::cout << "Total time, generating: " << GenLib::formatPeriod(int(total_elapsed_gen / 1000)) << ", verifying: " << GenLib::formatPeriod(int(total_elapsed_verify / 1000)) << std::endl;

    if (egtbFile->saveFile(folder, compressMode)) {
        egtbFile->createStatsFile();
        writeLog();

//        egtbFile->removeTmpFiles(folder);
        egtbFile = nullptr; /// not delete it because it has been added to the system
        return true;
    }

    std::cerr << "Error: Can't save files. UNSUCCESSFULLY " << egtbFile->getName() << std::endl;
    exit(-1);
    return false;
}

void EgtbGenDb::writeLog()
{
    static bool newSection = true;
    std::string str;
    
    if (newSection) {
        newSection = false;
        str += "\n====== New session, cores: " + std::to_string(MaxGenExtraThreads + 1) + " ======\n";
    }
    str += "\n" + egtbFile->getName()
    + "\n\tsize: " + std::to_string(egtbFile->getSize())
    + "\n\tstart: " + startTimeString
    + "\n\tgen elapsed: " + GenLib::formatPeriod(int(elapsed_gen / 1000))
    + "\n\tverify elapsed: " + GenLib::formatPeriod(int(elapsed_verify / 1000))
    + "\n\ttotal gen elapsed: " + GenLib::formatPeriod(int(total_elapsed_gen / 1000))
    + "\n\ttotal verify elapsed: " + GenLib::formatPeriod(int(total_elapsed_verify / 1000))
    ;
    
    auto logPath = gen_folder + "gen.log";
    GenLib::appendStringToFile(logPath, str);
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
        folder += STRING_PATH_SLASH;
    }
    gen_folder = folder;

    if (vec.empty()) {
        std::cout << "No missing endgames for " << name << std::endl;
        return true;
    }
    
    std::cout << "New session, #cores: " << (MaxGenExtraThreads + 1) << std::endl;

    for(auto egtbidx = 0; egtbidx < vec.size(); egtbidx++) {
        auto aName = vec.at(egtbidx);
        
        auto p = nameMap.find(aName);
        if (p != nameMap.end()) {
            continue;
        }

        NameRecord nameRecord(aName);
        
        auto writtingFolder = folder + nameRecord.getSubfolder();
        GenLib::createFolder(writtingFolder);


        if (!gen_single(egtbidx, writtingFolder, aName, egtbType, compressMode)) {
            std::cerr << "ERROR: generating UNSUCCESSFULLY " << aName << std::endl;
            exit(-1);
            break;
        }
        removeAllProbedBuffers();
    }
        

    std::cout << "Generating ENDED for " << name << std::endl;
    return true;
}


std::vector<std::string> createTestEPDVec(EgtbFile* egtbFile, int countPerEndgame)
{
    std::vector<std::string> epdVec;
    std::set<u64> idSet;
    
    /// for loading data
    egtbFile->getScore(1, Side::white, false);
    
    countPerEndgame = (int)std::min<i64>((i64)countPerEndgame, egtbFile->getSize() / 10);
    
    std::cout << "name: " << egtbFile->getName() << ", sz: " << egtbFile->getSize() << std::endl;
    
    EgtbBoard board;
    
    while(epdVec.size() < countPerEndgame) {

        /// generate idx randomly
        i64 idx = -1;
        for (auto i = 0; i < 1000; i++) {
            auto idx2 = GenLib::rand64() % egtbFile->getSize();
            if (idSet.find(idx2) == idSet.end()) {
                idSet.insert(idx2);
                idx = idx2;
                break;
            }
        }
        
        if (idx < 0) {
            break;
        }

        if (egtbFile->setupBoard(board, idx, FlipMode::none, Side::white)) {
            
            auto r = egtbFile->getKey(board);
            auto scoreW = egtbFile->getScore(r.key, Side::white, false);
            auto scoreB = egtbFile->getScore(r.key, Side::black, false);
            
            if (scoreW > EGTB_SCORE_MATE || scoreB > EGTB_SCORE_MATE) {
                continue;
            }

            
            auto epdString = board.getFen(false, -1, -1)
            + " id " + std::to_string(r.key)
            + "; c0 \"" + egtbFile->getName()
            + "\"; c1 \"scores=" + std::to_string(scoreW)
            + "," + std::to_string(scoreB)
            + "\""
            ;
            epdVec.push_back(epdString);
        }
        
    }
    
    egtbFile->removeBuffers();

    if (epdVec.size() < countPerEndgame) {
        std::cout << " -> smaller than the request: " << epdVec.size() << std::endl;
    }
    
    return epdVec;
}

void EgtbGenDb::createTestEPD(const std::string& path, int countPerEndgame)
{
    std::cout << "Create samples and save to epd file " << path << std::endl;

    std::ofstream file;
    file.open(path, std::ios::out | std::ios::app);

    for(auto && egtbFile : egtbFileVec) {
        auto vec = createTestEPDVec(egtbFile, countPerEndgame);
        for(auto && s : vec) {
            file << s << "\n";
        }
    }

    file.close();
}

void EgtbGenDb::testEPD(const std::string& path)
{
    std::ifstream file (path); //file just has some sentences
    if (!file) {
        std::cout << "unable to open file" << std::endl;
        return;
    }
    
    std::cout << "Load and test with epd file " << path << std::endl;
    
    EgtbBoard board;
    std::string line;
    int sucCnt = 0, wrongCnt = 0;
    while (getline (file, line))
    {
        auto vec = Funcs::splitString(line, ';');
        if (vec.size() < 3 || vec[2].find("c1") == std::string::npos) {
            continue;
        }
//        auto ss = Funcs::splitString(vec[0], ' ');
//        if (ss.size() < 3 || ss[ss.size() - 2] != "id") {
//            continue;
//        }
//        auto egtbName = ss[ss.size() - 1];
//        
//        auto it = nameMap.find(egtbName);
//        if (it == nameMap.end()) {
//            continue;
//        }
//        auto egtbFile = it->second; assert(egtbFile);
        
        board.setFen(vec[0]);
        if (!board.isValid()) {
            continue;
        }
        
        auto egtbFile = getEgtbFile(board);
        if (!egtbFile) {
            continue;
        }

        egtbFile->checkToLoadHeaderAndTables(Side::none);

        auto r = egtbFile->getKey(board);

        auto scoreW = egtbFile->getScore(r.key, Side::white, false);
        auto scoreB = egtbFile->getScore(r.key, Side::black, false);
        
        auto name = GenLib::extractBetweenString(vec[1]); /// name of endgame
        auto scores = GenLib::extractBetweenString(vec[2]); /// scores=21,0 (white, black scores)
        
        auto sss = Funcs::splitString(scores, '=');
        if (sss.size() != 2) {
            continue;
        }
        
        sss = Funcs::splitString(sss[1], ',');
        if (sss.size() != 2) {
            continue;
        }
        
        auto scoreW_ = std::stoi(sss[0]);
        auto scoreB_ = std::stoi(sss[1]);

        if (scoreW == scoreW_ && scoreB == scoreB_) {
            sucCnt++;
        } else {
            wrongCnt++;
            std::cerr << "Wrong " << line << std::endl;
            
            if (wrongCnt > 5000) {
                std::cerr << "Too many error, stopped!" << std::endl;
                return;
            }
        }
    }
    std::cout << "Test DONE! successful count: " << sucCnt << ", fail Count: " << wrongCnt << std::endl;
}
