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

#include "../fegtb/fegtbdb.h"

#include "../xq/xqchasejudge.h"

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

#ifdef _FELICITY_XQ_
    std::string allSides[2];
#endif
    
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
    
    void showData(const std::string& msg, i64 idx, bslib::Side side = bslib::Side::none, bool withChildren = true);
    void showData(const std::string& msg);

    std::vector<i64> setBufScore(EgtbGenThreadRecord&, i64 idx, int score, bslib::Side side);
    
public:
    virtual bool compress(std::string folder, const std::string& endgameName, bool includingSubEndgames, bool compress);
    
    void verifyData(const std::vector<std::string>& nameVec);
    
    
    bool verifyData_thread(int threadIdx, EgtbFile* pEgtbFile);
    bool verifyData(EgtbFile* pEgtbFile);
    
    virtual bool verifyKeys(const std::string& name, EgtbType egtbType) const;
    
    void verifyIndexes(const std::vector<std::string>& endgameNames) const;
    
    bool gen_all(std::string folder, const std::string& name, EgtbType egtbType, CompressMode compressMode);
    
    static void showSubTables(const std::string& name, EgtbType egtbType);
    static void showSubTables(const std::vector<std::string>& egNames, EgtbType egtbType);
    
    void showStats(const std::vector<std::string>& egNames);

    static void showIntestingSubTables(EgtbType egtbType);
    
    static void showStringWithCurrentTime(const std::string& msg);
    
    
    static std::vector<std::string> parseName(const std::string& name, bool includeSubs = true);
    
    static std::vector<std::string> parseNames(const std::vector<std::string>& names);
    
    void createStatsFiles();
    
    std::vector<std::string> showMissing(const std::string& startName) const;
    
    bool compare(EgtbFile* egtbFile0, EgtbFile* egtbFile1) const;
    void compare(EgtbGenDb& otherEgtbGenFileMng, std::string endgameName, bool isExact);
    
    void createTestEPD(const std::string& path, int countPerEndgame = 10);
    void testEPD(const std::string& path);
    
protected:
    
    /// For debugging purposes
    void changeOneThread();

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
    
    void gen_fillCapturesAfterGenerating();
    int gen_probeByChildren(EgtbBoard& board, bslib::Side side, bool debugging = false);
protected:
    void gen_backward_thread_init(int threadIdx);
    int  gen_backward_probe(GenBoard& board, i64 idx, bslib::Side side);
    
#ifdef _FELICITY_FLIP_MAP_
private:
    mutable std::mutex flipIdxMapMutex;
    std::unordered_map<i64, i64> flipIdxMap;
    void gen_backward_thread_init_flipMap(int threadIdx);
#endif

    i64 getFlipIdx(EgtbGenThreadRecord&, i64 idx) const;

#ifdef _FELICITY_XQ_
private:
    
    void perpetuation_debug();

    void perpetuation_process();
    i64 perpetuation_detect();
    void perpetuation_propaganda(int lap);
    
    void perpetuation_thread_detect_checkchase(int threadIdx);
    void perpetuation_fill(EgtbGenThreadRecord& rcd, std::vector<std::map<i64, bslib::Side>>& v, const bslib::Side side, bool check);
    
    void perpetuation_thread_detect_check(int threadIdx);
    void perpetuation_thread_detect_chase(int threadIdx);
    void perpetuation_thread_propaganda(int threadIdx, int ply, int sd, int phase);
    void perpetuation_thread_propaganda_init(int threadIdx, int lap);
    
    std::pair<int, i64> perpetual_check_probe(EgtbGenThreadRecord&, const bslib::Hist&, bool drawIfNotIncheck);
    
//    static bool perpetuation_score_valid(int score);
    std::vector<std::map<i64, bslib::Side>> perpetual_check_evasion(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side, const i64 startIdx, std::unordered_map<i64, int>&, int ply, bool evasion_checking);

    std::vector<std::map<i64, bslib::Side>> perpetual_check_attack(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side, const i64 startIdx, std::unordered_map<i64, int>&, int ply, bool evasion_checking);

    
    
    std::pair<int, i64> perpetual_chase_probe(EgtbGenThreadRecord&, const bslib::Hist&);

//    std::vector<std::map<i64, bslib::Side>> perpetual_chase(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side);

    std::vector<std::map<i64, bslib::Side>> perpetual_chase_evasion(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side, const i64 startIdx, std::set<i64>&, bslib::XqChaseJudge&);

    std::vector<std::map<i64, bslib::Side>> perpetual_chase_attack(EgtbGenThreadRecord& rcd, const i64 idx, const bslib::Side side, const i64 startIdx, std::set<i64>&, bslib::XqChaseJudge&);

    int perpetuation_propaganda_probe(GenBoard& board, bslib::Side side);
    
    
//    void perpetuation_propaganda0();
//
//    void perpetuation_thread_propaganda_init0(int threadIdx);
//    void perpetuation_thread_propaganda0(int threadIdx, int ply);
//    int perpetuation_thread_propaganda_set_parent_flag(EgtbGenThreadRecord&, i64 idx, GenBoard* board, bslib::Side side, int oscore);

#endif
    
private:
    std::string gen_folder;
    bool verifyDataOK = true;
    
    /// for report/log purposes
    std::string startTimeString;
    std::chrono::milliseconds::rep time_start, time_completed, time_start_verify, total_elapsed_gen = 0, total_elapsed_verify = 0, elapsed_gen = 0, elapsed_verify = 0;

#ifdef _FELICITY_XQ_
    std::chrono::milliseconds::rep time_perpetuation = 0, elapsed_perpetuation = 0, total_elapsed_perpetuation = 0;
    
    i64 chase_atk_cnt, chase_evasion_cnt, chase_len_max;
    void printChaseStats();


#endif
};


} // namespace fegtb

#endif /* EgtbGenDb_h */
