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

#ifndef EgtbGenDb_h
#define EgtbGenDb_h

#include <mutex>
#include <thread>

#include "../fegtb/egtbdb.h"

#include "threadmng.h"
#include "egtbgenfile.h"
#include "obj.h"
#include "genlib.h"


extern int MaxGenExtraThreads;

namespace fegtb {

class NameRecord {
public:
    std::string name;
    std::string sortingSides[2];
    
    int pieceCount[2][10], pawnCount[2];
    
    EgtbType type = EgtbType::dtm;
    bool ok = false;
    
    NameRecord() { }
    
    NameRecord(const std::string& name) {
        ok = parse(name);
    }
    
    bool isValid() const;
    
    bool isBothArmed() const;
    bool hasAttackers() const;

    /// for sorting
    bool isMeSmaller(const NameRecord& other) const;
    std::string getSubfolder() const;
    
    bool isLimited() const;
    
private:
    bool parse(const std::string& _name);
};


class EgtbGenDb : public EgtbDb, public ThreadMng {
    
protected:
    EgtbGenFile* egtbFile = nullptr;
    
public:
    std::chrono::steady_clock::time_point begin;
    time_t startTime;
    int total_elapsed = 0;
    
public:
    virtual bool compress(std::string folder, const std::string& endgameName, bool includingSubEndgames, bool compress);
    
//    void verifyData(const std::string& endgameName, bool includingSubEndgames);
    void verifyData(const std::vector<std::string>& nameVec);

    
    bool verifyData_chunk(int threadIdx, EgtbFile* pEgtbFile);
    bool verifyData(EgtbFile* pEgtbFile);
    
    virtual bool verifyKeys(const std::string& name, EgtbType egtbType) const;
    
    void verifyKeys(const std::vector<std::string>& endgameNames) const;

    bool gen_all(std::string folder, const std::string& name, EgtbType egtbType, CompressMode compressMode);
    
    static void showSubTables(const std::string& name, EgtbType egtbType);
    static void showSubTables(const std::vector<std::string>& egNames, EgtbType egtbType);


    static void showIntestingSubTables(EgtbType egtbType);

    static std::vector<std::string> parseName(const std::string& name, bool includeSubs = true);

    static std::vector<std::string> parseNames(const std::vector<std::string>& names);

    void createStatsFiles();
    
    std::vector<std::string> showMissing(const std::string& startName) const;
    
    bool compare(EgtbFile* egtbFile0, EgtbFile* egtbFile1) const;
    void compare(EgtbGenDb& otherEgtbGenFileMng, std::string endgameName, bool isExact);
    
protected:

    virtual void gen_thread_init(int threadIdx);
    virtual void gen_thread(int threadIdx, int sd, int fly);
    
protected:
    virtual bool compressEndgame(EgtbGenFile* egtbFile, std::string writtenfolder, CompressMode compressMode);
    
    virtual bool gen_single(const std::string& folder, const std::string& name, EgtbType egtbType, CompressMode compressMode);
    void gen_forward(const std::string& folder);
    bool gen_finish(const std::string& folder, CompressMode compressMode, bool needVerify = true);
    
    void gen_finish_adjust_scores();
    
    int probe_gen(EgtbBoard& board, i64 idx, bslib::Side side);
    
private:
    bool verifyDataOK = true;

};

} // namespace fegtb

#endif /* EgtbGenDb_h */
