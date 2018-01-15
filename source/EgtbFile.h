
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

#ifndef EgtbFile_hpp
#define EgtbFile_hpp

#include <fstream>
#include <mutex>

#include "Egtb.h"

namespace egtb {

    extern const int blackElephantPosWOMiddle[6];
    extern const int blackElephantCombineWOMiddle[21];

    const int EGTB_HEADER_SIZE      = 128;
    const int EGTB_ID_MAIN          = 556682;
    const int EGTB_PROP_COMPRESSED  = (1 << 2);
    const int EGTB_PROP_NEW         = (1 << 9);

    const int EGTB_SIZE_COMPRESS_BLOCK  = 4 * 1024;
    const int EGTB_SMART_MODE_THRESHOLD = 10L * 1024 * 1024L;

    enum {
        EGTB_IDX_DK,
        EGTB_IDX_DA,
        EGTB_IDX_DE,
        EGTB_IDX_DAA,
        EGTB_IDX_DEE,
        EGTB_IDX_DAE,
        EGTB_IDX_DAAE,
        EGTB_IDX_DAEE,
        EGTB_IDX_DAAEE,

        EGTB_IDX_DM,
        EGTB_IDX_DAM,
        EGTB_IDX_DAAM,

        EGTB_IDX_R = 16,
        EGTB_IDX_C,
        EGTB_IDX_H,
        EGTB_IDX_P,

        EGTB_IDX_R_Full,
        EGTB_IDX_C_Full,
        EGTB_IDX_H_Full,
        EGTB_IDX_P_Full,

        EGTB_IDX_CC,
        EGTB_IDX_CH,
        EGTB_IDX_CP,

        EGTB_IDX_HH,
        EGTB_IDX_HP,

        EGTB_IDX_PP,

        EGTB_IDX_CPP,
        EGTB_IDX_HPP,
        EGTB_IDX_PPP,

        EGTB_IDX_NONE = 254
    };

    class EgtbFileHeader {
    public:
        //*********** HEADER DATA ***************
        u32         signature;
        u32         property;
        u8          dtm_max;
        u8          notused0;
        u8          notused[12];

        u16         order;

        char        name[20], copyright[64];
        i64         checksum;
        char        reserver[80];
        //*********** END OF HEADER DATA **********

        void reset() {
            memset(this, 0, EGTB_HEADER_SIZE);
        };

        bool isValid() const {
            return signature == EGTB_ID_MAIN;
        }

        bool readFile(std::ifstream& file) {
            return file.read((char*)&signature, EGTB_HEADER_SIZE) && isValid();
        }

        bool isSide(Side side) const {
            return property & (1 << static_cast<int>(side));
        }

        void addSide(Side side) {
            property |= 1 << static_cast<int>(side);
        }

        void setOnlySide(Side side) {
            property &= ~((1 << W) | (1 << B));
            property |= 1 << static_cast<int>(side);
        }
    };

    /*
     * EGTB
     */

    class EgtbFile
    {
    public:
        EgtbFileHeader *header;
        i64         size;
        int         idxArr[5];
        i64         idxMult[32];

        u32         materialsignWB, materialsignBW;
        static u64 pieceListToMaterialSign(const int* pieceList);
        EgtbLoadStatus  loadStatus;

    private:
        i64         startpos[2], endpos[2];
        std::mutex  mtx, sdmtx[2];

    public:
        EgtbFile();
        ~EgtbFile();

        static std::pair<EgtbType, bool> getExtensionType(const std::string& path);

        void    removeBuffers();

        i64     getSize() const { return size; }

        int getCompresseBlockCount() const {
            return (int)((getSize() + EGTB_SIZE_COMPRESS_BLOCK - 1) / EGTB_SIZE_COMPRESS_BLOCK);
        }
        bool    isCompressed() const { return header->property & EGTB_PROP_COMPRESSED; }

        void    setPath(const std::string& path, int sd);
        std::string getPath(int sd) const { return path[sd]; }

        std::string getName() const { return egtbName; }

        i64     setupIdxComputing(const std::string& name, int order);

        static i64 parseAttr(const std::string& name, int* idxArr, i64* idxMult, int* pieceCount, EgtbType egtbType, u16 order);
        static i64 parseAttr(const int* idxArr, i64* idxMult, int* pieceCount, int* orderArray);

        static i64 computeSize(const std::string &name, EgtbType egtbType);
        static i64 computeMaterialSigns(const std::string &name, EgtbType egtbType, int* idxArr, i64* idxMult, u16 order);

        EgtbType getEgtbType() const { return egtbType; }

        void    checkToLoadHeaderAndTable(Side side);

    public:
        int     getScore(i64 idx, Side side, bool useLock = true);
        int     getScore(const int* pieceList, Side side, bool useLock = true);
        int     getScore(const EgtbBoard& board, Side side, bool useLock = true);
        int     lookup(const int *pieceList, Side side);

    public:
        bool    preload(const std::string& _path, EgtbMemMode memMode, EgtbLoadMode loadMode);
        bool    loadHeaderAndTable(const std::string& path);
        void    merge(EgtbFile& otherEgtbFile);
        bool    addLookup(EgtbLookup* lookup);

        static int cellToScore(char cell);

        std::pair<i64, bool> getKey(const int* pieceList) const;

    private:
        u32         pieceCount[2][7];
        char*       pBuf[2];
        u32*        compressBlockTables[2];
        char*       pCompressBuf;
        EgtbLookup *lookups[2];
        std::string path[2];
        std::string lupath[2];

        EgtbMemMode memMode;
        EgtbLoadMode loadMode;
        std::string egtbName;
        EgtbType    egtbType;

        std::pair<i64, bool> getKey(const EgtbBoard& board) const;
        void    reset();

        bool    readBuf(i64 startpos, int sd);
        bool    isDataReady(i64 pos, int sd) const { return pos >= startpos[sd] && pos < endpos[sd] && pBuf[sd]; }

        bool    createBuf(i64 len, int sd);
        i64     getBufItemCnt() const;
        i64     getBufSize() const {
            return getBufItemCnt();
        }

        int     getScoreNoLock(i64 idx, Side side);
        int     getScoreNoLock(const int* pieceList, Side side);
        int     getScoreNoLock(const EgtbBoard& board, Side side);

        char    getCell(const int* pieceList, Side side);
        char    getCell(i64 idx, Side side);

        bool    loadAllData(std::ifstream& file, Side side);
        bool    readCompressedBlock(std::ifstream& file, i64 idx, int sd, char* pDest);
    };

} // namespace egtb

#endif /* EgtbFile_hpp */

