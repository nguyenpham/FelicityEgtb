//
//  EgtbFileWritting.h
//
//  Created by Tony Pham on 4/11/17.
//

#ifndef EgtbFileWritting_hpp
#define EgtbFileWritting_hpp

#include <assert.h>
#include <string.h>
#include <fstream>

#include "../egtb/Egtb.h"
#include "../egtb/EgtbFile.h"
#include "Defs.h"
#include "ThreadMng.h"
#include "ExtEgtbDb.h"

extern std::mutex printMutex;

namespace egtb {

    enum EgtbProduct {
        std, bug
    };
    
    class EgtbFileWritting : public ExtEgtbFile, public ThreadMng {
    public:
        ~EgtbFileWritting();
        virtual void removeBuffers();

        std::string createStatsString();
        void createStatsFile();

        bool saveFile(const std::string& folder, EgtbProduct egtbProduct, CompressMode compressMode) {
            return saveFile(folder, bslib::B, egtbProduct, compressMode) && saveFile(folder, bslib::W, egtbProduct, compressMode);
        }

        void setName(const std::string& s) {
            egtbName = s;
#ifdef _WIN32
            strncpy_s(header->getName(), sizeof(header->getName()), s.c_str(), s.length());
#else
            strncpy(header->getName(), s.c_str(), sizeof(header->getName()));
#endif
        }

        void    setSize(i64 sz) { size = sz; }

        virtual int cellToScore(char cell);

        static int cellToScore(char cell, bool ver2);
        static char scoreToCell(int score, bool ver2);

        void    checkAndConvert2bytesTo1();
        void    convert1byteTo2();

        void    convertPermutations(EgtbFileWritting* fromStandardEgtbFile, bool testingSize);
        void    convertPermutations_loop(int threadIdx, EgtbFileWritting* fromStandardEgtbFile);
        int     getEgtbIdxArraySize() const;
        
        i64     convertPermutations_idx(i64 idx, EgtbFileWritting* fromStandardEgtbFile) const;
        
        void    convertScores(EgtbFile* standardEgtbFile);

        virtual bool isValidHeader() const {
            return header && header->isValid();
        }

    public:
        bool    saveFile(const std::string& folder, int sd, EgtbProduct egtbProduct, CompressMode compressMode);
        bool    saveFileTestIdea(const std::string& folder, int sd, CompressMode compressMode);
        i64     convert(const std::string& newFolder);

        void    printPerpetuationStats(const char* msg = nullptr);

    public:
        u32     checksum(void* data, i64 len) const;

        bool    readFromTmpFiles(const std::string& folder, int& ply, int& mPly);
        bool    writeTmpFiles(const std::string& folder, int ply, int mPly);
        void    removeTmpFiles(const std::string& folder) const;

        int     readFromTmpFile(const std::string& folder, int sd);
        int     readFromTmpFile(const std::string& folder, int sd, i64 fromIdx, i64 toIdx, char * toBuf);
        bool    writeTmpFile(const std::string& folder, int sd, int loop);

    private:
        bool readFlagTmpFile(const std::string& folder);
        bool writeFlagTmpFile(const std::string& folder);

        std::string getTmpFileName(const std::string& folder, int sd) const;
        std::string getFlagTmpFileName(const std::string& folder) const;

    public:
        std::string getLogFileName() const;
        int readFromLogFile() const;
        void writeLogFile(int completedPly) const;

        bool    verifyKeys();
        bool    verifyKey(int threadIdx, i64 idx);
        bool    verifyKeys_loop(int threadIdx);

        bool    createBuffersForGenerating();
        void    createFlagBuffer();
        void    removeFlagBuffer();
        void    clearFlagBuffer();

        bool    setBufScore(i64 idx, int score, int sd);
        void    fillBufs(int score);

        bool    setBuf(i64 idx, char cell, int sd) {
            if (pBuf[sd] && idx >= startpos[sd] && idx < endpos[sd]) {
                pBuf[sd][idx - startpos[sd]] = cell;
                return true;
            }
            return false;
        }

        bool    setBuf2Bytes(i64 idx, int score, int sd) {
            if (pBuf[sd] && idx >= startpos[sd] && idx < endpos[sd]) {
                i16* p = (i16*)pBuf[sd];
                p[idx - startpos[sd]] = i16(score);
                return true;
            }
            return false;
        }

        static std::string createFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, bslib::Side side, bool compressed);
        static bool existFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, bslib::Side side, bool compressed);

        bool    setupPieceList(int *pieceList, i64 idx, bslib::FlipMode flip, bslib::Side strongsider) const;
        bool    setupPieceList(EgtbBoard& board, i64 idx, bslib::FlipMode flip, bslib::Side strongsider) const {
            return setupPieceList((int *)board.pieceList, idx, flip, strongsider);
        }

        void    addProperty(uint addprt);

        void create(const std::string& name, EgtbType _egtbType, EgtbProduct fileFor, u32 order = 0);
        int getVersion() const;
        bool saveHeader(std::ofstream& outfile) const;

        virtual std::string toString() const {
            std::ostringstream stringStream;
            stringStream << getName() << ", " << getSize();
            if (header) {
                stringStream << ", ver: " << getVersion() << ", order: " << header->getOrder() << (isCompressed() ? " compressed" : " uncompressed");
            }
            return stringStream.str();
        }
        
        void flag_clear_side(i64 idx, int sd) {
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 4 : 0));
            flags[idx >> 1] &= ~f;
        }
        void flag_set_side(i64 idx, int sd) {
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 4 : 0));
            flags[idx >> 1] |= f;
        }
        bool flag_is_side(i64 idx, int sd) const {
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 4 : 0));
            return (flags[idx >> 1] & f) != 0;
        }

        void flag_clear_cap(i64 idx, int sd) {
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 6 : 2));
            flags[idx >> 1] &= ~f;
        }
        
        void flag_set_cap(i64 idx, int sd) {
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 6 : 2));
            flags[idx >> 1] |= f;
        }
        bool flag_is_cap(i64 idx, int sd) const {
            assert(idx >= 0 && idx < getSize() && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 6 : 2));
            return (flags[idx >> 1] & f) != 0;
        }

        uint8_t* flags = nullptr;
    };

} // namespace egtb

#endif /* EgtbFileWritting_hpp */

