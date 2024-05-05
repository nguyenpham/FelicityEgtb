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

#ifndef EgtbGenFileMng_h
#define EgtbGenFileMng_h

#include <mutex>
#include <thread>

#include "../egtb/egtbdb.h"

#include "threadmng.h"
#include "egtbfilewriter.h"
#include "obj.h"
#include "genlib.h"


extern int MaxGenExtraThreads;

namespace fegtb {

class NameRecord {
public:
    std::string name;
    int pieceCount[2][10], attackerCount[2];
    EgtbType type = EgtbType::dtm;
    bool ok = false;
    
    NameRecord() { }
    
    NameRecord(const std::string& name) {
        ok = parse(name);
    }
    
    bool isValid() const;
    
    
    bool isBothArmed() const;
    bool hasAttackers() const;

    bool isMeSmaller(const NameRecord& other) const;
    
private:
    bool parse(const std::string& _name);
    
    int mats[2] = { 0, 0 };
    int attackerMats[2] = { 0, 0 };
};

class SubfolderParser : public Obj {
public:
    int pieceCount[2][10], attackingCnt, wrongSymbolCnt;
    std::string subfolder;
    std::vector<std::string> allSubfolders;
    
public:
    SubfolderParser(const std::string& name) {
        parse(name);
        assert(name.size() > 2 && subfolder.size() > 2);
    }
    
    void parse(const std::string& name) {
        static const char *pieceChars = "kaerchp";
        memset(pieceCount, 0, sizeof(pieceCount));
        wrongSymbolCnt = attackingCnt = 0;
        for(auto i = 0, sd = 1; i < name.size(); ++i) {
            char ch = name[i];
            //king, advisor, elephant, rook, cannon, horse, pawn, empty, offboard
            auto p = strchr(pieceChars, ch);
            if (p) {
                auto pieceType = (int)(p - pieceChars);
                if (pieceType == static_cast<int>(bslib::PieceType::king)) {
                    sd = i == 0 ? bslib::W : bslib::B;
                }
                pieceCount[sd][pieceType]++;
            } else {
                wrongSymbolCnt++;
            }
        }
        
        int atkCnt[2] = { 0, 0 };
        for(auto t = bslib::ROOK; t <= bslib::PAWN; ++t) {
            atkCnt[0] += pieceCount[0][t];
            atkCnt[1] += pieceCount[1][t];
        }
        attackingCnt = atkCnt[0] + atkCnt[1];
        assert(attackingCnt > 0);
        if (atkCnt[0] == 0 || atkCnt[1] == 0) {
            subfolder = std::to_string(attackingCnt);
        } else {
            subfolder = std::to_string(atkCnt[bslib::W]) + "-" + std::to_string(atkCnt[bslib::B]);
        }
        allSubfolders.push_back(subfolder);
        subfolder += "/";
        
        std::string atkStrings[2] = { "", "" };
        for(auto pieceType = bslib::ROOK; pieceType <= bslib::PAWN; ++pieceType) {
            char ch = pieceChars[pieceType];
            for(auto sd = 0; sd < 2; ++sd) {
                for(auto i = 0; i < pieceCount[sd][pieceType]; ++i) {
                    atkStrings[sd] += ch;
                }
            }
        }
        
        if (atkCnt[0] == 0 || atkCnt[1] == 0) {
            subfolder += atkStrings[atkCnt[0] ? bslib::B : bslib::W];
        } else {
            subfolder += atkStrings[bslib::W] + "-" + atkStrings[bslib::B];
        }
        
        allSubfolders.push_back(subfolder);
    }
    
    void createAllSubfolders(const std::string& folder) {
        for(auto && s : allSubfolders) {
            auto str = folder + s;
            GenLib::createFolder(str);
        }
    }
    
    virtual bool isValid() const {
        for(auto sd = 0; sd < 2; ++sd) {
            if (pieceCount[sd][0] != 1 || pieceCount[sd][bslib::PAWN] > 5) {
                return false;
            }
            for(auto i = 1; i < bslib::PAWN; ++i) {
                if (pieceCount[sd][i] > 2) {
                    return false;
                }
            }
        }
        return true;
    }
};

class EgtbGenFileMng : public EgtbDb, public ThreadMng {
    
protected:
    EgtbFileWriter* egtbFile = nullptr;
    
public:
    std::chrono::steady_clock::time_point begin;
    time_t startTime;
    
public:
    virtual bool compress(std::string folder, const std::string& endgameName, bool includingSubEndgames, bool compress);
    
    void verifyData(const std::string& endgameName, bool includingSubEndgames);
    bool verifyData_chunk(int threadIdx, EgtbFile* pEgtbFile);
    bool verifyData(EgtbFile* pEgtbFile);
    
    virtual bool verifyKeys(const std::string& name, EgtbType egtbType) const;
    virtual void verifyKeys(const std::string& endgameName, bool includingSubEndgames) const;
    
    bool gen_all(std::string folder, const std::string& name, EgtbType egtbType, CompressMode compressMode);
    
    static void showSubTables(const std::string& name, EgtbType egtbType);
    

    static void showIntestingSubTables(EgtbType egtbType);

    static std::vector<std::string> parseName(const std::string& name, bool includeSubs = true);

    static std::vector<std::string> parseNames(const std::vector<std::string>& names);

    void createStatsFiles();
    
    std::vector<std::string> showMissing(const std::string& startName) const;
    
    bool compare(EgtbFile* egtbFile0, EgtbFile* egtbFile1) const;
    void compare(EgtbGenFileMng& otherEgtbGenFileMng, std::string endgameName, bool isExact);
    
    void createProduct(EgtbGenFileMng& stdEgtbFileMng, std::string egtbFolder, const std::string& endgameName, bool forAllSides);
    
protected:
    static void showSubTables(const std::vector<std::string>& egNames, EgtbType egtbType);

    virtual void gen_single_init(int threadIdx);
    virtual void gen_single_thread(int threadIdx, int sd, int fly);
    
protected:
    virtual bool compressEndgame(EgtbFileWriter* egtbFile, std::string writtenfolder, CompressMode compressMode);
    
    virtual bool gen_single(const std::string& folder, const std::string& name, EgtbType egtbType, CompressMode compressMode);
    void gen_single_forward(const std::string& folder);
    bool gen_single_finish(const std::string& folder, CompressMode compressMode, bool needVerify = true);
    
    int probe_gen(EgtbBoard& board, i64 idx, bslib::Side side, int ply, int oldScore);
    
};

} // namespace fegtb

#endif /* EgtbGenFileMng_h */
