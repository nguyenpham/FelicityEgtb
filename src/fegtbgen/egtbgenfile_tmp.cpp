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

#include <thread>

#include "../fegtb/fegtb.h"
#include "../base/funcs.h"

#include "egtbgenfile.h"
#include "egtbgendb.h"
#include "compresslib.h"
#include "genlib.h"

extern std::mutex printMutex;

using namespace fegtb;
using namespace bslib;

static const int TmpHeaderSize = 16;
static const i16 TmpHeaderSign = 2345;
static const i16 TmpHeaderSign2 = 2346;

struct TmpHeader {
    i16 sign;
    i16 loop;
    u32 checksum;
    char resert[16];
};


std::string EgtbGenFile::getTmpFileName(const std::string& folder, Side side) const {
    return createFileName(folder, getName(), EgtbType::tmp, side, false);
}

std::string EgtbGenFile::getFlagTmpFileName(const std::string& folder) const {
    auto fileName = getTmpFileName(folder, Side::white);
    fileName = fileName.substr(0, fileName.length() - 5) + "f.tmt";
    return fileName;
}


bool EgtbGenFile::readFromTmpFiles(const std::string& folder, int& ply, int& mPly)
{
    ply = readFromTmpFile(folder, Side::black);
    mPly = readFromTmpFile(folder, Side::white);
    return ply >= 0 && mPly >= 0 && (flags == nullptr || readFlagTmpFile(folder));
}

bool EgtbGenFile::writeTmpFiles(const std::string& folder, int ply, int mPly)
{
    return writeTmpFile(folder, Side::black, ply)
    && writeTmpFile(folder, Side::white, mPly)
    && (flags == nullptr || writeFlagTmpFile(folder))
    ;
}

int EgtbGenFile::readFromTmpFile(const std::string& folder, Side side)
{
    auto sz = getSize();
    auto bufsz = sz;
    if (isTwoBytes()) bufsz += bufsz;

    auto sd = static_cast<int>(side);
    if (!pBuf[sd]) {
        createBuf(bufsz, side);
    }
    assert(pBuf[sd]);

    return readFromTmpFile(folder, side, 0, sz, pBuf[sd]);
}

u32 EgtbGenFile::checksum(void* data, i64 len) const
{
    assert(data && len > 0);
    
    u32 checksum = 0;
    unsigned shift = 0;
    
    auto n = len / sizeof(u32);
    u32 *p = (u32*)data, *e = p + n;
    
    for (; p < e; p++) {
        checksum += (*p << shift);
        shift += 8;
        if (shift == 32) {
            shift = 0;
        }
    }
    
    return checksum;
}

int EgtbGenFile::readFromTmpFile(const std::string& folder, Side side, i64 fromIdx, i64 toIdx, char * toBuf)
{
    assert(toBuf);
    auto fromOfs = fromIdx, toOfs = toIdx;
    if (isTwoBytes()) {
        fromOfs += fromOfs;
        toOfs += toOfs;
    }
    i64 bufsz = toOfs - fromOfs;
    if (bufsz <= 0) {
        return -1;
    }

    auto fileName = getTmpFileName(folder, side);

    std::ifstream file(fileName.c_str(), std::ios::binary);

    if (!file) {
        if (egtbVerbose)
            printf("Not found temp file %s\n", fileName.c_str());
        return -1;
    }

    TmpHeader tmpHeader;
    file.seekg(0);
    i16 sign = flags == nullptr ? TmpHeaderSign : TmpHeaderSign2;
    bool ok = file.read((char *)&tmpHeader, TmpHeaderSize) && tmpHeader.sign == sign && tmpHeader.loop >= 0;

    if (ok) {
        file.seekg((i64)(TmpHeaderSize + fromOfs));
        file.read(toBuf, bufsz);
    }

    if (!ok) {
        if (egtbVerbose) {
            std::cerr << "Error: cannot read tmp file: " << fileName << ", sd: " << Funcs::side2String(side, false) << std::endl;
        }
        file.close();
        return -1;
    }

    auto sd = static_cast<int>(side);
    startpos[sd] = fromIdx;
    endpos[sd] = toIdx;

    file.close();
    
    if (fromIdx == 0 && toIdx == getSize()) {
        auto sum = checksum(pBuf[sd], bufsz);
        if (tmpHeader.checksum != sum) {
            std::cerr << "Error: checksum failed for temp file side " << Funcs::side2String(side, false) << ". Ignored that file." << std::endl;
            return -1;
        }
    }
    return tmpHeader.loop;
}

bool EgtbGenFile::readFlagTmpFile(const std::string& folder)
{
    assert(flags);

    auto fileName = getFlagTmpFileName(folder);

    auto bufsz = (getSize() + 1) / 2;
    std::ifstream file(fileName.c_str(), std::ios::binary);

    if (!file) {
        if (egtbVerbose)
            std::cerr << "Not found temp file " << fileName << std::endl;
        return false;
    }

    TmpHeader tmpHeader;
    file.seekg(0);
    bool ok = file.read((char *)&tmpHeader, TmpHeaderSize) && tmpHeader.sign == TmpHeaderSign2;

    if (ok) {
        file.seekg((i64)(TmpHeaderSize));
        file.read((char*)flags, bufsz);
    }

    if (!ok) {
        if (egtbVerbose)
            std::cerr << "Error: cannot read data for flags with temp file " << fileName << std::endl;
        file.close();
        return false;
    }

    file.close();

    auto sum = checksum(flags, bufsz);
    if (tmpHeader.checksum != sum) {
        if (egtbVerbose)
            std::cerr << "Error: cannot read data for flags (checksum failed) with temp file " << fileName << std::endl;
        return false;
    }

    return true;
}

bool EgtbGenFile::writeFlagTmpFile(const std::string& folder)
{
    auto bufsz = (getSize() + 1) / 2;

    auto fileName = getFlagTmpFileName(folder);

    std::ofstream outfile(fileName.c_str(), std::ios::binary);

    if (!outfile) {
        std::cerr << "Error: cannot open to write temp file " << fileName << std::endl;
        return false;
    }

    TmpHeader tmpHeader;
    tmpHeader.sign = TmpHeaderSign2;
    tmpHeader.checksum = checksum(flags, bufsz);
    outfile.seekp(0);

    if (outfile.write((char *)&tmpHeader, TmpHeaderSize) && outfile.write((char*)flags, bufsz)) {
        outfile.close();
        if (egtbVerbose)
            std::cout << "Successfully save to flag tmp, file: " << fileName << std::endl;
        return true;
    }

    std::cerr << "Error: cannot write flag tmp file " << fileName << std::endl;
    outfile.close();
    return false;
}



bool EgtbGenFile::writeTmpFile(const std::string& folder, Side side, int loop)
{
    auto sd = static_cast<int>(side);
    auto sz = getSize(), bufSz = sz;
    if (isTwoBytes()) bufSz += bufSz;

    assert(pBuf[sd]);

    auto fileName = getTmpFileName(folder, side);
    std::ofstream outfile(fileName.c_str(), std::ios::binary);

    if (egtbVerbose) {
        std::cout << "writeTmpFile, starting sd: " << sd << ", bufsz: " << GenLib::formatString(bufSz) << std::endl;
    }
    
    if (!outfile) {
        printf("EgtbFileWriter::writeTmpFile, Error: Cannot open file %s\n", fileName.c_str());
        return false;
    }

    TmpHeader tmpHeader;
    i16 sign = flags == nullptr ? TmpHeaderSign : TmpHeaderSign2;
    tmpHeader.sign = sign;
    tmpHeader.loop = loop;
    tmpHeader.checksum = checksum(pBuf[sd], bufSz);

    outfile.seekp(0);

    if (outfile.write((char *)&tmpHeader, TmpHeaderSize) &&
        outfile.write(pBuf[sd], bufSz)) {
        outfile.close();
        if (egtbVerbose)
            std::cout << "Successfully save to tmp sd " << sd << ", at " << loop << ", file: " << fileName << std::endl;
        return true;
    }

    printf("EgtbFileWriter::writeTmpFile Error: cannot write data, sd=%d, %s\n", sd, fileName.c_str());
    outfile.close();
    return false;
}

void EgtbGenFile::removeTmpFiles(const std::string& folder) const
{
    for(int sd = 0; sd < 2; sd++) {
        auto fileName = getTmpFileName(folder, static_cast<Side>(sd));
        std::remove(fileName.c_str());
    }
    auto fileName = getFlagTmpFileName(folder);
    std::remove(fileName.c_str());
}
