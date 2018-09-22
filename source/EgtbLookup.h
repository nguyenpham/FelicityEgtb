
/*
 This file is part of Felicity Egtb, distributed under MIT license.

 Copyright (c) 2018 Nguyen Hong Pham

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#ifndef EgtbLookup_h
#define EgtbLookup_h

#include "Egtb.h"
#include <mutex>

namespace egtb {


    class EgtbLookup {
    protected:
        const int EGTBLU_HEADER_SIZE              = 48;
        const int EGTBLU_HEADER_SIGN              = 1765;
        const int EGTBLU_PROPERTY_COMPRESS        = (1 << 4);
        const int EGTBLU_PROPERTY_VERSION_2       = (1 << 5);

        const int EGTBLU_COMPRESS_BLOCK_SIZE      = (7 * 22 * 32);
        const int EGTBLU_SMART_MODE_THRESHOLD     = 10L * 1024 * 1024L;

        // Header
    public:
        int16_t sign;
        int8_t  property, info;
        int32_t itemCnt[4];
        int16_t order;
        int16_t notused;

        int32_t keyCompressedSize;
        char    theName[16];
        int8_t  reserves[32];
        // End header data

    public:
        EgtbLookup();
        EgtbLookup(const std::string& path, EgtbMemMode memMode, EgtbLoadMode loadMode);
        ~EgtbLookup();

        std::string getName() const;
        Side getSide() const;

        bool preload(const std::string& path, EgtbMemMode memMode, EgtbLoadMode loadMode);
        int lookup(const int* pieceList, Side side, const int* idxArr, const i64* idxMult, u32 order);

        void removeBuffers();

        bool isVersion2() const {
            return property & EGTBLU_PROPERTY_VERSION_2;
        }

        std::string getPath() const {
            return path;
        }

        static const int luGroupSizes[2];
        
    protected:
        u32*        keyTable;
        u32*        blockTable;

        int8_t*     data[2];
        i64         startpos[2], endpos[2];
        int         bufsz[2];
        char*       pCompressBuf;

        std::string path;
        std::string luName;
        EgtbMemMode memMode;
        EgtbLoadMode loadMode;
        EgtbLoadStatus loadStatus;

        std::mutex  mtx, loadMutex;

        int  getScore(i64 key, int groupIdx, int subIdx);
        int  getCell(int groupIdx, int itemidx, int subIdx);

        void reset();
        bool _preload();
        bool preload_tables(std::ifstream& file);
    };

} // namespace egtb

#endif /* EgtbLookup_h */

