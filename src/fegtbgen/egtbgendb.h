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
    std::string attackerSides[2];

    int pieceCount[2][10], pawnCount[2];
    
    EgtbType type = EgtbType::dtm;
    bool ok = false;
    
    NameRecord() { }
    
    NameRecord(const std::string& name) {
        ok = parse(name);
    }
    
    bool isValid() const;
    static bool isValid(const std::string& name);
    
    bool isBothArmed() const;
    bool hasAttackers() const;

    /// for sorting
    bool isSmaller(const NameRecord& other) const;
    
    bool isLimited() const;
    
    std::string getSubfolder() const;
    
private:
    bool parse(const std::string& _name);
};


class EgtbGenDb : public EgtbDb, public ThreadMng {

public:
    static DataItemMode dataItemMode;
    static bool twoBytes; // per item
    static  bool useTempFiles;
    static bool useBackward;
    static bool verifyMode;
    static i64 maxEndgameSize;

protected:
    EgtbGenFile* egtbFile = nullptr;
    
    
public:
    virtual bool compress(std::string folder, const std::string& endgameName, bool includingSubEndgames, bool compress);
    
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

    void writeLog();
    
protected:
    virtual bool compressEndgame(EgtbGenFile* egtbFile, std::string writtenfolder, CompressMode compressMode);
    
    virtual bool gen_single(int egtbidx, const std::string& folder, const std::string& name, EgtbType egtbType, CompressMode compressMode);
    
    
    bool gen_finish(const std::string& folder, CompressMode compressMode, bool needVerify = true);
    
    void gen_finish_adjust_scores();
    
    
public:
    void gen_forward(const std::string& folder);
        
protected:
    void gen_forward_thread_init(int threadIdx);
    void gen_forward_thread(int threadIdx, int sd, int fly);
    int  gen_forward_probe(GenBoard& board, i64 idx, bslib::Side side, bool setup = true);


public:
    void gen_backward(const std::string& folder);
    void gen_backward_thread(int threadIdx, int ply, int sd, int phase);
    
protected:
    void gen_backward_thread_init(int threadIdx);
    int  gen_backward_probe(GenBoard& board, i64 idx, bslib::Side side);

    
private:
    std::string gen_folder;
    bool verifyDataOK = true;
    
    /// for report/log purposes
    std::string startTimeString;
    std::chrono::milliseconds::rep time_start, time_completed, time_start_verify, total_elapsed_gen = 0, total_elapsed_verify = 0, elapsed_gen = 0, elapsed_verify = 0;

};


} // namespace fegtb

#endif /* EgtbGenDb_h */
