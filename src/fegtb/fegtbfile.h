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

#ifndef fegtb_file_h
#define fegtb_file_h

#include <assert.h>
#include <fstream>
#include <mutex>

#include "fegtb.h"


namespace fegtb {

const int EGTB_HEADER_SIZE                  = 128;
const int EGTB_ID_MAIN                      = 556682;

const int EGTB_PROP_SIDE_BLACK              = (1 << 0);
const int EGTB_PROP_SIDE_WHITE              = (1 << 1);

const int EGTB_PROP_COMPRESSED              = (1 << 2);
const int EGTB_PROP_2BYTES                  = (1 << 3);

const int EGTB_PROP_LARGE_COMPRESSTABLE_B   = (1 << 4);
const int EGTB_PROP_LARGE_COMPRESSTABLE_W   = (1 << 5);

const int EGTB_PROP_COMPRESS_OPTIMIZED      = (1 << 8);
const int EGTB_PROP_NEW                     = (1 << 9);


const int EGTB_SIZE_COMPRESS_BLOCK          = 4 * 1024;
const int EGTB_SMART_MODE_THRESHOLD         = 120L * 1024 * 1024L;


class EgtbIdxRecord
{
public:
    EgtbIdx         idx;
    i64             mult;
    bslib::Side     side;
    i64             factor;
};


class EgtbFileHeader {
private:
    //*********** HEADER DATA, total size should >= EGTB_HEADER_SIZE
    u32         signature;
    u32         property;
    u8          dtm_max;
    u8          notused0;
    u8          notused[12];

    u16         order;

    char        name[20], copyright[COPYRIGHT_BUFSZ];
    i64         checksum;
    char        reserver[80];
    //*********** END OF HEADER DATA **********

public:
    void reset() {
        memset(&signature, 0, headerSize());
        signature = EGTB_ID_MAIN;
    }

    char* getData() { return (char*)&signature; }
    void resetSignature() { signature = EGTB_ID_MAIN; }
    u32 getSignature() const { return signature; }
    void setSignature(u32 sig) { signature = sig; }
    
    std::string getCopyright() { return copyright; }

    bool fromBuffer(const char* buffer) {
        memcpy(&signature, buffer, EGTB_HEADER_SIZE);
        return isValid();
    }

    bool isValid() const { return signature == EGTB_ID_MAIN; }

    bool isSide(bslib::Side side) const {
        auto bit = side == bslib::Side::black ? EGTB_PROP_SIDE_BLACK : EGTB_PROP_SIDE_WHITE;
        return property & bit;
    }

    void addSide(bslib::Side side) {
        auto bit = side == bslib::Side::black ? EGTB_PROP_SIDE_BLACK : EGTB_PROP_SIDE_WHITE;
        property |= bit;
    }

    void setOnlySide(bslib::Side side) {
        auto bit = side == bslib::Side::black ? EGTB_PROP_SIDE_BLACK : EGTB_PROP_SIDE_WHITE;

        property &= ~(EGTB_PROP_SIDE_BLACK | EGTB_PROP_SIDE_WHITE);
        property |= bit;
    }
    
    int headerSize() const { return EGTB_HEADER_SIZE; }
    
    int getProperty() const { return property; }
    void setProperty(int prop) { property = prop; }
    void addProperty(int prop) { property |= prop; }

    virtual int getOrder() const { return order; }
    virtual void setOrder(int ord) { order = ord; }

    std::string getName() const { return name; }
    int getDtm_max() const { return dtm_max; }
    void setDtm_max(int m) { dtm_max = m; }

#ifdef _WIN32
    void setName(const std::string& s)
    {
        strncpy_s(name, sizeof(name), s.c_str(), s.length());
    }
    void setCopyright(const std::string& s)
    {
        strncpy_s(copyright, sizeof(copyright), s.c_str(), s.length());
    }
#else
    void setName(const std::string& s)
    {
        strncpy(name, s.c_str(), sizeof(name));
    }
    void setCopyright(const std::string& s)
    {
        strncpy(copyright, s.c_str(), sizeof(copyright));
    }
#endif

};


/*
 * EgtbFile
 */
class EgtbFile
{
public:
    EgtbFile(const std::string& name = "", int order = 0);
    ~EgtbFile();

    static EgtbType getExtensionType(const std::string& path);
    EgtbType getEgtbType() const { return egtbType; }

    void    setPath(const std::string& path, bslib::Side side);
    std::string getPath(bslib::Side side) const;
    
    std::string getName() const { return egtbName; }

    virtual void removeBuffers();
    static u64 computeSize(const std::string &name);
    
    virtual int getCompressBlockSize() const {
        return EGTB_SIZE_COMPRESS_BLOCK;
    }

    virtual int getCompresseBlockCount() const {
        auto sz = getSize();
        if (isTwoBytes()) sz += sz;
        return (int)((sz + getCompressBlockSize() - 1) / getCompressBlockSize());
    }

    bool    isCompressed() const { return header->getProperty() & EGTB_PROP_COMPRESSED; }

    i64     setupIdxComputing(const std::string& name, int order);

    i64     getSize() const { return size; }
    EgtbLoadStatus getLoadStatus() const { return loadStatus; }
    EgtbFileHeader* getHeader() { return  header; }
    const EgtbFileHeader* getHeader() const { return  header; }
    bool isBothArmed() const { return bothArmed; }
    u32 getMaterialsign() const { return materialsign; }
    
    static u64 parseAttr(const std::string& name, EgtbIdxRecord* egtbIdxRecordArray, int* pieceCount, u16 order);


    virtual void checkToLoadHeaderAndTables(bslib::Side side);
    virtual bool forceLoadHeaderAndTable(bslib::Side side);

    static const char* egtbFileExtensions[];

    int     getScore(i64 idx, bslib::Side side, bool useLock = true);
    int     getScore(const EgtbBoard& board, bslib::Side side, bool useLock = true);
    
    bool    preload(const std::string& _path, EgtbMemMode memMode, EgtbLoadMode loadMode);
    bool    loadHeaderAndTable(const std::string& path);
    void    merge(EgtbFile& otherEgtbFile);

    
    bool    isDataReady(i64 pos, bslib::Side side) const {
        int sd = static_cast<int>(side);
        return pos >= startpos[sd] && pos < endpos[sd] && pBuf[sd]; }

    bool setupBoard(EgtbBoard& board, i64 idx, bslib::FlipMode flip, bslib::Side firstSide) const;

    bool showBoard(const std::string& msg, i64 idx, bslib::Side side = bslib::Side::white, bslib::FlipMode flipMode = bslib::FlipMode::none) const;

    virtual EgtbKeyRec getIdx(const EgtbBoard& board) const;

    int     getAttackerCount() const {
        return attackerCount;
    }

    EgtbMemMode getMemMode() const {
        return memMode;
    }
    
    bool isTwoBytes() const {
        return (header->getProperty() & EGTB_PROP_2BYTES) != 0;
    }

    virtual int cellToScore(char cell);
    static int _cellToScore(char cell);

    static bool pickBestFromRivalScore(int& bestScore, int score, bool rule120 = true);
    static bool isSmallerScore(int score0, int score1);
    static int revertScore(int score, int inc = 1, bool rule120 = true);
    static std::string explainScore(int score);
    static bool matchChilrenScore(int score, int chilrenScore);

#ifdef _FELICITY_XQ_
    static bool isPerpetualScore(int score);
    static bool isPerpetualScoreOver120(int score);

#endif

protected:
    EgtbFileHeader  *header = nullptr;
    bool            bothArmed;
    u32             materialsign;
    i64             size;
    i64             startpos[2], endpos[2];
    std::mutex      mtx, sdmtx[2];
    u32             pieceCount[2][10];
    int             attackerCount;
    char*           pBuf[2];
    u8*             compressBlockTables[2];
    char*           pCompressBuf;
    
    
    std::string     path[2];
    std::string     lupath[2];

    EgtbLoadStatus  loadStatus;
    EgtbIdxRecord   egtbIdxArray[16];
    EgtbMemMode     memMode;
    EgtbLoadMode    loadMode;
    std::string     egtbName;
    EgtbType        egtbType;

    void    reset();

    virtual bool readHeader(std::ifstream& file) {
        char buffer[200];

        if (file.read(buffer, 128)) {
            auto b = false;
            if (header == nullptr) {
                header = new EgtbFileHeader();
                b = true;
            }
            if (header->fromBuffer(buffer)) {
                return true;
            }
            
            if (b) {
                delete header;
            }
        }
        return false;
    }

    virtual bool readCompressTable(std::ifstream& file, bslib::Side side);
    
    int getBlockTableSize(bslib::Side side) const {
        auto blockCnt = getCompresseBlockCount();
        auto sd = static_cast<int>(side);
        int blockTableItemSize = (header->getProperty() & (EGTB_PROP_LARGE_COMPRESSTABLE_B << sd)) != 0 ? 5 : 4;
        int blockTableSz = blockCnt * blockTableItemSize; //sizeof(u32);
        return blockTableSz;
    }
    
    virtual bool isValidHeader() const {
        return header && header->isValid();
    }
    
    bool    readBuf(i64 startpos, bslib::Side side);

    bool    createBuf(i64 len, bslib::Side side);
    i64     getBufItemCnt() const;
    i64     getBufSize() const {
        auto sz = getBufItemCnt();
        if (isTwoBytes()) sz += sz;
        return sz;
    }

    int     getScoreNoLock(i64 idx, bslib::Side side);
    int     getScoreNoLock(const EgtbBoard& board, bslib::Side side);

    char    getCell(i64 idx, bslib::Side side);

    bool    loadAllData(std::ifstream& file, bslib::Side side);
    bool    readCompressedBlock(std::ifstream& file, i64 idx, bslib::Side side, char* pDest);
    
};


} // namespace fegtb

#endif

