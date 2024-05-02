//
//  EgtbGenFileMng.h
//
//  Created by TonyPham on 26/2/17.
//

#ifndef EgtbGenFileMng_hpp
#define EgtbGenFileMng_hpp

#include <mutex>
#include <thread>

#include "ThreadMng.h"
#include "EgtbDbWritting.h"
#include "EgtbFileWritting.h"
#include "Obj.h"
#include "Lib.h"
#include "Extensions.h"


extern int MaxGenExtraThreads;

namespace egtb {

    class NameRecord {
    public:
        std::string name;
        int pieceCount[2][7];
        int atkCnt[2] = { 0, 0 }, defCnt[2] = { 0, 0 };
        EgtbType type;
        bool ok;
        
        NameRecord() { ok = false; }
        
        NameRecord(const std::string& name) {
            ok = parse(name);
        }
        
        bool parse(const std::string& _name) {
            name = _name;
            
            memset(pieceCount, 0, sizeof(pieceCount));
            
            type = EgtbType::dtm;

            for(int i = 0, sd = bslib::W; i < name.size(); i++) {
                char ch = name[i];
                const char* p = strchr(pieceTypeName, ch);
                if (p == nullptr) {
                    if (ch == 'm' && type == EgtbType::dtm) { // 1 m only
                        type = EgtbType::newdtm;
                        continue;
                    }
                    return false;
                }
                
                int t = (int)(p - pieceTypeName);
                if (ch == 'k' && i > 0) {
                    sd = B;
                }
                //auto pieceType = static_cast<PieceType>(t);
                pieceCount[sd][t]++;
                
                if (t >= static_cast<int>(bslib::PieceType::rook)) {
                    atkCnt[sd]++;
                } else if (t != static_cast<int>(bslib::PieceType::king)) {
                    defCnt[sd]++;
                }
            }
            
            return true;
        }

        bool isBothArmed() const {
            return atkCnt[0] > 0 && atkCnt[1] > 0 ;
        }

        bool isValid() const {
            if (!ok || pieceCount[0][0] != 1 || pieceCount[1][0] != 1 ||
                atkCnt[1] == 0 || atkCnt[1] < atkCnt[0] ||
                (type == EgtbType::newdtm && pieceCount[1][2] > 0)) // newdtm: white side must have no elephant
                return false;
            int totals[2] = { 1, 1 };

            auto first = atkCnt[1] == atkCnt[0];
            auto sameAtk = first;
            
            for(int t = 1; t < 7; t++) {
                if (t > 2 && first && pieceCount[1][t] != pieceCount[0][t]) {
                    sameAtk = false;
                    if (pieceCount[1][t] < pieceCount[0][t]) {
                        return false;
                    }
                    first = false;
                }
                for(int sd = 0; sd < 2; sd++) {
                    if (pieceCount[sd][t] > 0) {
                        totals[sd] += pieceCount[sd][t];
                        int n = t != static_cast<int>(bslib::PieceType::pawn) ? 2 : 5;
                        if (pieceCount[sd][t] > n) {
                            return false;
                        }
                    }
                }
            }
            
            if (sameAtk) {
                if (defCnt[0] > defCnt[1] || (defCnt[0] == defCnt[1] && pieceCount[1][1] < pieceCount[0][1])) {
                    return false;
                }
            }
            
            return totals[0] >= 1 && totals[0] <= 16 && totals[1] > 1 && totals[1] <= 16;
        }

        bool isMeSmaller(const NameRecord& otherNameRecord) const {
            // attackers
            for(int sd = bslib::W; sd >= 0; sd--) {
                if (atkCnt[sd] != otherNameRecord.atkCnt[sd]) {
                    return atkCnt[sd] < otherNameRecord.atkCnt[sd];
                }
            }

            for(int sd = bslib::W; sd >= 0; sd--) {
                for(int i = 3; i < 7; i++) {
                    if (pieceCount[sd][i] != otherNameRecord.pieceCount[sd][i]) {
                        return pieceCount[sd][i] < otherNameRecord.pieceCount[sd][i];
                    }
                }
            }

            // defenders
            for(int sd = bslib::W; sd >= 0; sd--) {
                if (defCnt[sd] != otherNameRecord.defCnt[sd]) {
                    return defCnt[sd] < otherNameRecord.defCnt[sd];
                }
            }
            
            for(int sd = bslib::W; sd >= 0; sd--) {
                for(int i = 1; i < 3; i++) {
                    if (pieceCount[sd][i] != otherNameRecord.pieceCount[sd][i]) {
                        return pieceCount[sd][i] < otherNameRecord.pieceCount[sd][i];
                    }
                }
            }
            
            return false;
        }
        
        bool isSameAttackers(const NameRecord& other) const {
            if (atkCnt[0] == other.atkCnt[0] && atkCnt[1] == other.atkCnt[1]) {
                for(int sd = 0; sd < 2; sd++) {
                    for(int t = 3; t < 7; t++) {
                        if (pieceCount[sd][t] != other.pieceCount[sd][t]) {
                            return false;
                        }
                    }
                }
                return true;
            }
            return false;
        }
        
    };

    class SubfolderParser : public Obj {
    public:
        int pieceCount[2][7], attackingCnt, wrongSymbolCnt;
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
            for(int i = 0, sd = 1; i < name.size(); ++i) {
                char ch = name[i];
                //king, advisor, elephant, rook, cannon, horse, pawn, empty, offboard
                auto p = strchr(pieceChars, ch);
                if (p) {
                    int pieceType = (int)(p - pieceChars);
                    if (pieceType == static_cast<int>(bslib::PieceType::king)) {
                        sd = i == 0 ? W : B;
                    }
                    pieceCount[sd][pieceType]++;
                } else {
					wrongSymbolCnt++;
                }
            }

            int atkCnt[2] = { 0, 0 };
            for(int t = static_cast<int>(PieceType::rook); t <= static_cast<int>(PieceType::pawn); ++t) {
                atkCnt[0] += pieceCount[0][t];
                atkCnt[1] += pieceCount[1][t];
            }
            attackingCnt = atkCnt[0] + atkCnt[1];
            assert(attackingCnt > 0);
            if (atkCnt[0] == 0 || atkCnt[1] == 0) {
                subfolder = Lib::itoa(attackingCnt);
            } else {
                subfolder = Lib::itoa(atkCnt[W]) + "-" + Lib::itoa(atkCnt[B]);
            }
            allSubfolders.push_back(subfolder);
            subfolder += "/";

            std::string atkStrings[2] = { "", "" };
            for(int pieceType = static_cast<int>(PieceType::rook); pieceType <= static_cast<int>(PieceType::pawn); ++pieceType) {
                char ch = pieceChars[pieceType];
                for(int sd = 0; sd < 2; ++sd) {
                    for(int i = 0; i < pieceCount[sd][pieceType]; ++i) {
                        atkStrings[sd] += ch;
                    }
                }
            }
            
            if (atkCnt[0] == 0 || atkCnt[1] == 0) {
                subfolder += atkStrings[atkCnt[0] ? B : W];
            } else {
                subfolder += atkStrings[W] + "-" + atkStrings[B];
            }

            allSubfolders.push_back(subfolder);
        }

        void createAllSubfolders(const std::string& folder) {
            for(auto && s : allSubfolders) {
                auto str = folder + s;
                Lib::createFolder(str);
            }
        }

        virtual bool isValid() const {
            for(int sd = 0; sd < 2; ++sd) {
                if (pieceCount[sd][0] != 1 || pieceCount[sd][static_cast<int>(PieceType::pawn)] > 5) {
                    return false;
                }
                for(int i = 1; i < static_cast<int>(PieceType::pawn); ++i) {
                    if (pieceCount[sd][i] > 2) {
                        return false;
                    }
                }
            }
            return true;
        }
    };
    
class EgtbGenFileMng : public EgtbDbWritting, public ThreadMng {
    
protected:
    EgtbFileWritting* egtbFile = nullptr;

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
    
    bool genEgtb(std::string folder, const std::string& name, EgtbType egtbType, EgtbProduct fileFor, CompressMode compressMode);
    static void showSubTables(const std::string& name, EgtbType egtbType);
    
    static std::vector<std::string> parseName(const std::string& name, bool includeSubs = true);
    
    void createStatsFiles();

    void convert();
    void convertScores();
    void verifyConvertScore();

    std::vector<std::string> showMissing(const std::string& startName) const;
    
    bool compare(EgtbFile* egtbFile0, EgtbFile* egtbFile1) const;
    void compare(EgtbGenFileMng& otherEgtbGenFileMng, std::string endgameName, bool isExact);

    void testSpeed(const std::string& name, bool whole);
    
    void createProduct(EgtbGenFileMng& stdEgtbFileMng, std::string egtbFolder, const std::string& endgameName, EgtbProduct egtbProduct, bool forAllSides);
    
protected:
    virtual void genSingleEgtb_init(int threadIdx);
    virtual void genSingleEgtb_loop(int threadIdx, int sd, int fly);

    void genSingleEgtb_backward_init(int threadIdx);
    void genSingleEgtb_backward_loop(int threadIdx, int sd, int fly, int task);
    void genSingleEgtb_backward_main(const std::string& folder);
    int probe_gen_backward(i64 idx, Side side, int ply);
//    void genSingleEgtb_backward_endProcess();
    
    void verifyCheckAndEvasion();
    void verifyCheckAndEvasion_loop(int threadIdx);
//    void verifyUnset_loop(int threadIdx);

    
    bool createProduct(std::string writtingFolder, EgtbFileWritting& egtbFile, EgtbProduct egtbProduct, bool forAllSides);
protected:
    virtual bool compressEndgame(EgtbFileWritting* egtbFile, std::string writtenfolder, CompressMode compressMode);

    virtual bool genSingleEgtb(const std::string& folder, const std::string& name, EgtbType egtbType, EgtbProduct fileFor, CompressMode compressMode);
    void genSingleEgtb_main(const std::string& folder);
    bool genSingleEgtb_finish(const std::string& folder, CompressMode compressMode, bool needVerify = true);
    
    int probe_gen(i64 idx, Side side, int ply, int oldScore);
    
    bool verifyCheck(i64 idx, Side side);
    bool verifyEvasion(i64 idx, Side side);
//    bool verifyUnset(i64 idx, Side side);
    
    void perpetualPropaganda_backward(int threadIdx, int ply);
    void perpetualPropaganda(int threadIdx, int ply);

    void numberisePerpetuation();
    void numberisePerpetuation_loop(int threadIdx);
    
public:
    bool perpetuationFix(const std::string& name, bool includeSub);
    
protected:
    bool perpetuationFixSingle();
    bool perpetuationFixSingle_main();
    void perpetuationFix_reachableLoop(int threadIdx, int sd, int ply);
    
    void perpetuationFix_initialise(int threadIdx);
//    void perpetuationFix_init(i64 idx, Side side);
    bool perpetuationFix_reachable(i64 idx, Side side, int ply);
//    i64 perpetuationFix_reachable_cleanup();
    void perpetualPropaganda_fromCaptures(int threadIdx);
    
    void perpetuationFix_finish();
    
//    void printStas(EgtbFileWritting* egtbFile);
};

} // namespace egtb

#endif /* EgtbGenFileMng_hpp */
