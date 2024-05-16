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

#ifndef EgtbFileWriter_h
#define EgtbFileWriter_h

#include <assert.h>
#include <string.h>
#include <fstream>

#include "../fegtb/egtb.h"
#include "../fegtb/egtbfile.h"
#include "defs.h"
#include "threadmng.h"

extern std::mutex printMutex;

namespace fegtb {

    class EgtbGenFile : public EgtbFile, public ThreadMng {
    public:
        ~EgtbGenFile();
        virtual void removeBuffers();

        std::string createStatsString();
        void createStatsFile();

        bool saveFile(const std::string& folder, CompressMode compressMode) {
            return saveFile(folder, bslib::Side::black, compressMode) && saveFile(folder, bslib::Side::white, compressMode);
        }

        void setName(const std::string& s);

        void    setSize(i64 sz) { size = sz; }

        static char scoreToCell(int score);

        virtual bool isValidHeader() const {
            return header && header->isValid();
        }

    public:
        bool    saveFile(const std::string& folder, bslib::Side side, CompressMode compressMode);

        void    checkAndConvert2bytesTo1();
        void    convert1byteTo2();
        
        bool    verifyKeys();
        bool    verifyKey(int threadIdx, i64 idx);
        bool    verifyKeys_loop(int threadIdx);

        bool    createBuffersForGenerating();
        bool    setBufScore(i64 idx, int score, bslib::Side side);
        void    fillBufs(int score);

        bool    setBuf(i64 idx, char cell, bslib::Side side) {
            auto sd = static_cast<int>(side);
            if (pBuf[sd] && idx >= startpos[sd] && idx < endpos[sd]) {
                pBuf[sd][idx - startpos[sd]] = cell;
                return true;
            }
            return false;
        }

        bool    setBuf2Bytes(i64 idx, int score, bslib::Side side) {
            auto sd = static_cast<int>(side);
            if (pBuf[sd] && idx >= startpos[sd] && idx < endpos[sd]) {
                i16* p = (i16*)pBuf[sd];
                p[idx - startpos[sd]] = i16(score);
                return true;
            }
            return false;
        }

        static std::string createFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, bslib::Side side, bool compressed);
        static bool existFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, bslib::Side side, bool compressed);

        void    addProperty(uint addprt);

        void    create(const std::string& name, EgtbType = EgtbType::dtm, u32 order = 0);
        bool    saveHeader(std::ofstream& outfile) const;

        virtual std::string toString() const {
            std::ostringstream stringStream;
            stringStream << getName() << ", " << getSize();
            if (header) {
                stringStream << ", ver: " << getVersion() << ", order: " << header->getOrder() << (isCompressed() ? " compressed" : " uncompressed");
            }
            return stringStream.str();
        }
   
    public:
//        u32     checksum(void* data, i64 len) const;
//
//        bool    readFromTmpFiles(const std::string& folder, int& ply, int& mPly);
//        bool    writeTmpFiles(const std::string& folder, int ply, int mPly);
//        void    removeTmpFiles(const std::string& folder) const;
//
//        int     readFromTmpFile(const std::string& folder, bslib::Side side);
//        int     readFromTmpFile(const std::string& folder, bslib::Side side, i64 fromIdx, i64 toIdx, char * toBuf);
//        bool    writeTmpFile(const std::string& folder, bslib::Side side, int loop);
//
//    private:
//        bool readFlagTmpFile(const std::string& folder);
//        bool writeFlagTmpFile(const std::string& folder);
//
//        std::string getTmpFileName(const std::string& folder, bslib::Side side) const;
//        std::string getFlagTmpFileName(const std::string& folder) const;

    public:
//        std::string getLogFileName() const;
//        int     readFromLogFile() const;
//        void    writeLogFile(int completedPly) const;

        void    createFlagBuffer();
        void    removeFlagBuffer();
        void    clearFlagBuffer();

     
        void flag_clear_side(i64 idx, bslib::Side side) {
            auto sd = static_cast<int>(side);
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 4 : 0));
            flags[idx >> 1] &= ~f;
        }
        void flag_set_side(i64 idx, bslib::Side side) {
            auto sd = static_cast<int>(side);
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 4 : 0));
            flags[idx >> 1] |= f;
        }
        bool flag_is_side(i64 idx, bslib::Side side) const {
            auto sd = static_cast<int>(side);
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 4 : 0));
            return (flags[idx >> 1] & f) != 0;
        }

        void flag_clear_cap(i64 idx, bslib::Side side) {
            auto sd = static_cast<int>(side);
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 6 : 2));
            flags[idx >> 1] &= ~f;
        }
        
        void flag_set_cap(i64 idx, bslib::Side side) {
            auto sd = static_cast<int>(side);
            assert(idx >= 0 && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 6 : 2));
            flags[idx >> 1] |= f;
        }
        bool flag_is_cap(i64 idx, bslib::Side side) const {
            auto sd = static_cast<int>(side);
            assert(idx >= 0 && idx < getSize() && sd >= 0 && sd <= 1);
            auto f = 1 << (sd + ((idx & 1) ? 6 : 2));
            return (flags[idx >> 1] & f) != 0;
        }

        uint8_t* flags = nullptr;
    };

} // namespace fegtb

#endif /* EgtbFileWriter_h */

