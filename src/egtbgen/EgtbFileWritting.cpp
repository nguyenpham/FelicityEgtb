//
//  EgtbFileWritting.cpp
//
//  Created by Tony Pham on 4/11/17.
//

#include <thread>
#include "../egtb/Egtb.h"
#include "../egtb/EgtbKey.h"

#include "EgtbFileWritting.h"
#include "CompressLib.h"
#include "Lib.h"
#include "Extensions.h"

std::mutex printMutex;

using namespace egtb;

extern bool twoBytes;
extern const int pieceValForOrdering[7];

#define COPYRIGHT       "Copyright 2017-2019 by NHP"
static const int TmpHeaderSize = 16;
static const i16 TmpHeaderSign = 2345;
static const i16 TmpHeaderSign2 = 2346;

struct TmpHeader {
    i16 sign;
    i16 loop;
    u32 checksum;
    char resert[16];
};


EgtbFileWritting::~EgtbFileWritting()
{
    removeFlagBuffer();
}

void EgtbFileWritting::removeBuffers()
{
    EgtbFile::removeBuffers();
    removeFlagBuffer();
}

void EgtbFileWritting::addProperty(uint addprt) {
    header->addProperty(addprt);
}

void EgtbFileWritting::create(const std::string& name, EgtbType _egtbType, EgtbProduct fileFor, u32 order) {
    if (header == nullptr) {
        if (fileFor == EgtbProduct::std) {
            header = new EgtbStdFileHeader();
        } else {
            header = new EgtbBugFileHeader();
        }
    }
    header->reset();

    if (egtbType == EgtbType::newdtm) {
        addProperty(EGTB_PROP_NEW);
    }

    egtbType = _egtbType;

    loadStatus = EgtbLoadStatus::loaded;

    header->setOrder(order);
    
    if (twoBytes) {
        header->addProperty(EGTB_PROP_2BYTES);
        assert(isTwoBytes());
    }

    setName(name);

    i64 sz = setupIdxComputing(name, order); //, getVersion());
    setSize(sz); assert(sz);
}

int EgtbFileWritting::getVersion() const {
    auto signature = header ? header->getSignature() : 0;
    switch (signature) {
        case TB_ID_MAIN_V1:
            return 0;
        case EGTB_ID_MAIN_V2:
            return 1;
        case EGTB_ID_MAIN_V3:
            return 2;
        case EGTB_ID_MAIN:
        case EGTB_ID_BUG:
            return 3;
    }
    assert(false);
    return -1;
}

bool EgtbFileWritting::saveHeader(std::ofstream& outfile) const {
    //            outfile.seekg(0LL, std::ios::beg);
    outfile.write (header->getData(), header->headerSize());
    return true;
}

void EgtbFileWritting::fillBufs(int score)
{
    char cell = scoreToCell(score, getVersion() > 0);
    for(int sd = 0; sd < 2; sd++) {
        if (pBuf[sd])
            std::memset(pBuf[sd], cell, size);
    }
}

bool EgtbFileWritting::setBufScore(i64 idx, int score, int sd)
{
    if (isTwoBytes()) {
        return setBuf2Bytes(idx, score, sd);
    }

    char cell = scoreToCell(score, getVersion() > 0);
    return setBuf(idx, cell, sd);
}


int EgtbFileWritting::cellToScore(char cell) {
    assert(!isTwoBytes());
    return cellToScore(cell, getVersion() > 0);
}


char EgtbFileWritting::scoreToCell(int score, bool ver2) {
    if (ver2) {
        if (score <= EGTB_SCORE_MATE) {
            if (score == EGTB_SCORE_DRAW) return TB_DRAW;
            
            int mi = (EGTB_SCORE_MATE - abs(score)) / 2;

            int k = mi + (score > 0 ? EGTB_START_MATING : EGTB_START_LOSING);
            assert(k >= 0 && k < 255);
            
            if (mi > 125 || k < 0 || k >= 255 || (score > 0 && k >= (EGTB_START_LOSING - 2))) {
                std::lock_guard<std::mutex> thelock(printMutex);
                std::cerr << "FATAL ERROR: overflown score: " << score << ". Please use 2 bytes per item (param: -2)." << std::endl;
                exit(-1);
            }

            return (char)k;
        }

        switch (score) {
            case EGTB_SCORE_MISSING:
                return TB_MISSING;

            case EGTB_SCORE_WINNING:
                return TB_WINING;

            case EGTB_SCORE_UNKNOWN:
                return TB_UNKNOWN;

            case EGTB_SCORE_ILLEGAL:
                return TB_ILLEGAL;
                
//            case EGTB_SCORE_CHECKED:
//                return TB_CHECKED;
//            case EGTB_SCORE_EVASION:
//                return TB_EVASION;
            case EGTB_SCORE_PERPETUAL_CHECKED:
                return (char)TB_PERPETUAL_CHECKED;
            case EGTB_SCORE_PERPETUAL_CHECKED_EVASION:
                return (char)TB_PERPETUAL_CHECKED_EVASION;
            case EGTB_SCORE_PERPETUAL_EVASION:
                return (char)TB_PERPETUAL_EVASION;

            case EGTB_SCORE_UNSET:
                return TB_UNSET;
            default:
                assert(false);
                return TB_UNSET;
        }
    }
    
    std::cerr << "Fatal error: using old version endgames." << std::endl;
    exit(-1);

    assert(false);
    return 0;
//    if (score != 0) {
//        score -= score > 0 ? EGTB_SCORE_BASE : -EGTB_SCORE_BASE;
//    }
//    assert(abs(score) <= 127);
//    return (char)score;
}

int EgtbFileWritting::cellToScore(char cell, bool ver2) {
    if (ver2) {
        u8 s = (u8)cell;
        if (s >= TB_DRAW) {
            //            if (s == TB_DRAW) return EGTB_SCORE_DRAW;
            // Researching
            switch (s) {
                case TB_DRAW:
                    return EGTB_SCORE_DRAW;
//                case TB_CHECKED:
//                    return EGTB_SCORE_CHECKED;
//                case TB_EVASION:
//                    return EGTB_SCORE_EVASION;
                case TB_PERPETUAL_CHECKED:
                    return EGTB_SCORE_PERPETUAL_CHECKED;
                case TB_PERPETUAL_CHECKED_EVASION:
                    return EGTB_SCORE_PERPETUAL_CHECKED_EVASION;
                case TB_PERPETUAL_EVASION:
                    return EGTB_SCORE_PERPETUAL_EVASION;

                default:
                    break;
            }
            
            if (s < EGTB_START_LOSING) {
                int mi = (s - EGTB_START_MATING) * 2 + 1;
                return EGTB_SCORE_MATE - mi;
            }
            
            int mi = (s - EGTB_START_LOSING) * 2;
            return -EGTB_SCORE_MATE + mi;
        }
        
        switch (s) {
            case TB_MISSING:
                return EGTB_SCORE_MISSING;
                
            case TB_WINING:
                return EGTB_SCORE_WINNING;
                
            case TB_UNKNOWN:
                return EGTB_SCORE_UNKNOWN;
                
            case TB_ILLEGAL:
                return EGTB_SCORE_ILLEGAL;
                
            case TB_UNSET:
            default:
                return EGTB_SCORE_UNSET;
        }
    }
    
    std::cerr << "Fatal error: using old version endgames." << std::endl;
    exit(-1);

    assert(false);
    return 0;

//    int score = (int)cell;
//
//    if (score != 0) {
//        score += score > 0 ? EGTB_SCORE_BASE : -EGTB_SCORE_BASE;
//    }
//    return score;
}


bool EgtbFileWritting::createBuffersForGenerating() {
    memMode = EgtbMemMode::all;
    auto sz = getSize();
    auto bufSz = sz;
    if (isTwoBytes()) {
        bufSz += bufSz;
    }

    bool r = createBuf(bufSz, 0) && createBuf(bufSz, 1);
    startpos[0] = startpos[1] = 0;
    endpos[0] = endpos[1] = sz;
    return r;
}


void EgtbFileWritting::createFlagBuffer() {
    removeFlagBuffer();
    auto flagLen = getSize() / 2 + 16;
    flags = (uint8_t*) malloc(flagLen);
    memset(flags, 0, flagLen);
}


void EgtbFileWritting::removeFlagBuffer()
{
    if (flags) {
        free(flags);
        flags = nullptr;
    }
}

void EgtbFileWritting::clearFlagBuffer() {
    if (flags) {
        auto flagLen = getSize() / 2 + 16;
        memset(flags, 0, flagLen);
    }
}


std::string EgtbFileWritting::getLogFileName() const
{
    std::string fileName = getPath(W);
    Lib::replaceString(fileName, "w.xtb", ".ini");
    return fileName;
}

int EgtbFileWritting::readFromLogFile() const {
    auto fileName = getLogFileName();

    auto vec = Lib::readFileToLineArray(fileName);

    for (auto && str : vec) {
        if (!str.empty()) {
            int n = std::stoi(str);
            return n;
        }
    }

    return -1;
}

void EgtbFileWritting::writeLogFile(int completedPly) const {
    auto fileName = getLogFileName();
    std::ofstream outfile(fileName.c_str());
    outfile << completedPly << "\n\n";
}

std::string EgtbFileWritting::getTmpFileName(const std::string& folder, int sd) const {
    Side side = static_cast<Side>(sd);
    return createFileName(folder, getName(), EgtbType::tmp, side, false);
}

std::string EgtbFileWritting::getFlagTmpFileName(const std::string& folder) const {
    auto fileName = getTmpFileName(folder, W);
    fileName = fileName.substr(0, fileName.length() - 5) + "f.tmt";
    return fileName;
}


bool EgtbFileWritting::readFromTmpFiles(const std::string& folder, int& ply, int& mPly)
{
    ply = readFromTmpFile(folder, 0);
    mPly = readFromTmpFile(folder, 1);
    return ply >= 0 && mPly >= 0 && (flags == nullptr || readFlagTmpFile(folder));
}

bool EgtbFileWritting::writeTmpFiles(const std::string& folder, int ply, int mPly)
{
    return writeTmpFile(folder, 0, ply) && writeTmpFile(folder, 1, mPly) && (flags == nullptr || writeFlagTmpFile(folder));
}

int EgtbFileWritting::readFromTmpFile(const std::string& folder, int sd)
{
    auto sz = getSize();
    auto bufsz = sz;
    if (isTwoBytes()) bufsz += bufsz;

    if (!pBuf[sd]) {
        createBuf(bufsz, sd);
    }
    assert(pBuf[sd]);

    return readFromTmpFile(folder, sd, 0, sz, pBuf[sd]);
}

u32 EgtbFileWritting::checksum(void* data, i64 len) const
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
    
    // Ignore few last bytes for faster
//    auto k = len - n * sizeof(u32);
//    if (k > 0) {
//        assert(k < 4);
//        u32 x = *e & (0xffffffff >> ((4 - k) * 8));
//        checksum += (x << shift);
//    }
    
    return checksum;
}

int EgtbFileWritting::readFromTmpFile(const std::string& folder, int sd, i64 fromIdx, i64 toIdx, char * toBuf)
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

    auto fileName = getTmpFileName(folder, sd);

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
            std::cerr << "Error: cannot read tmp file: " << fileName << ", sd: " << sd << std::endl;
        }
        file.close();
        return -1;
    }

    startpos[sd] = fromIdx;
    endpos[sd] = toIdx;

    file.close();
    
    if (fromIdx == 0 && toIdx == getSize()) {
        auto sum = checksum(pBuf[sd], bufsz);
        if (tmpHeader.checksum != sum) {
            std::cerr << "Error: checksum failed for temp file side " << (sd == B ? "Black" : "White") << ". Ignored that file." << std::endl;
            return -1;
        }
    }
    return tmpHeader.loop;
}

bool EgtbFileWritting::readFlagTmpFile(const std::string& folder)
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

bool EgtbFileWritting::writeFlagTmpFile(const std::string& folder)
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


bool EgtbFileWritting::writeTmpFile(const std::string& folder, int sd, int loop)
{
    auto sz = getSize(), bufSz = sz;
    if (isTwoBytes()) bufSz += bufSz;

    assert(pBuf[sd]);

    auto fileName = getTmpFileName(folder, sd);
    std::ofstream outfile(fileName.c_str(), std::ios::binary);

    if (egtbVerbose) {
        std::cout << "writeTmpFile, starting sd: " << sd << ", bufsz: " << Lib::formatString(bufSz) << std::endl;
    }
    
    if (!outfile) {
        printf("EgtbFileWritting::writeTmpFile, Error: Cannot open file %s\n", fileName.c_str());
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

    printf("EgtbFileWritting::writeTmpFile Error: cannot write data, sd=%d, %s\n", sd, fileName.c_str());
    outfile.close();
    return false;
}

void EgtbFileWritting::removeTmpFiles(const std::string& folder) const
{
    for(int sd = 0; sd < 2; sd++) {
        auto fileName = getTmpFileName(folder, sd);
        std::remove(fileName.c_str());
    }
    auto fileName = getFlagTmpFileName(folder);
    std::remove(fileName.c_str());
}

std::string EgtbFileWritting::createStatsString()
{
    std::ostringstream stringStream;
    stringStream << "Name:\t\t\t" << getName() << std::endl;

    i64 validCnt[2] = { 0, 0 };
    int smallestCell = EGTB_SCORE_MATE;
    i64 wdl[2][3] = {{0, 0, 0}, {0, 0, 0}};

    for (int sd = 0; sd < 2; sd++) {
        Side side = static_cast<Side>(sd);
        for(i64 idx = 0; idx < getSize(); idx++) {
            int score = getScore(idx, side);
            if (score == EGTB_SCORE_ILLEGAL) {
                continue;
            }
            validCnt[sd]++;
            if (score == EGTB_SCORE_DRAW) {
                wdl[sd][1]++;
            } else if (score <= EGTB_SCORE_MATE || score >= EGTB_SCORE_PERPETUAL_CHECKED) {
                if (score > 0 || score == EGTB_SCORE_PERPETUAL_CHECKED || score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) wdl[sd][0]++;
                else wdl[sd][2]++;
                if (score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) wdl[sd][2]++;
                int absScore = abs(score);
                smallestCell = std::min(smallestCell, absScore);
            }
        }
    }

    stringStream << "Total positions:\t" << getSize() << std::endl;
    i64 total = validCnt[0] + validCnt[1];
    stringStream << "Legal positions:\t" << total << " (" << (total * 50 / getSize()) << "%) (2 sides)" << std::endl;  // count size for both sides, thus x 50 insteal of 100

    i64 w = wdl[1][0] * 100 / validCnt[1];
    stringStream << "White to move,\t\twin: " << w << "%";
    if (w == 0 &&  wdl[1][0] > 0) {
        stringStream << " (" << wdl[1][0] << ")";
    }
    stringStream << ", draw: " << wdl[1][1] * 100 / validCnt[1] << "%";
    if (wdl[1][2] || isBothArmed()) {
        stringStream << ", loss: " << wdl[1][2] * 100 / validCnt[1] << "%";
    }
    stringStream << std::endl;

    stringStream << "Black to move,\t\t";
    if (wdl[0][0] || isBothArmed()) {
        w = wdl[0][0] * 100 / validCnt[0];
        stringStream << "win: " << w << "%";
        if (wdl[0][0] > 0 && (w == 0 || !isBothArmed())) {
            stringStream << " (" << wdl[0][0] << ")";
        }
        stringStream << ", ";
    }
    stringStream << "draw: " << wdl[0][1] * 100 / validCnt[0] << "%, loss: " << wdl[0][2] * 100 / validCnt[0] << "%" << std::endl;

    stringStream << "Max distance to mate:\t" << abs(EGTB_SCORE_MATE - smallestCell) << std::endl;
    return stringStream.str();
}

void EgtbFileWritting::createStatsFile()
{
    std::string str = createStatsString();
    auto statsFileName = getPath(B);
    statsFileName = statsFileName.substr(0, statsFileName.length() - 5) + ".txt"; // 5 = "b.xtb"

    std::remove(statsFileName.c_str());
    Lib::writeTextFile(statsFileName, str);
}

std::string EgtbFileWritting::createFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, Side side, bool compressed)
{
    auto t = static_cast<int>(egtbType);
    const char* ext = compressed ? EgtbFile::egtbCompressFileExtensions[t] : EgtbFile::egtbFileExtensions[t];
    auto theName = name;
    Lib::toLower(theName);
    return folderName + "/" + theName + (side == Side::black ? "b" : "w") + ext;
}

bool EgtbFileWritting::existFileName(const std::string& folderName, const std::string& name, EgtbType egtbType, Side side, bool compressed)
{
    if (side == Side::none) {
        return Lib::existFile(createFileName(folderName, name, egtbType, Side::white, compressed).c_str()) &&
            Lib::existFile(createFileName(folderName, name, egtbType, Side::black, compressed).c_str());
    }
    return Lib::existFile(createFileName(folderName, name, egtbType, side, compressed).c_str());
}

i64 totalSize = 0, illegalCnt = 0, drawCnt = 0, compressedUndeterminedCnt = 0;

bool EgtbFileWritting::saveFile(const std::string& folder, int sd, EgtbProduct egtbProduct, CompressMode compressMode) {
    assert(sd == 0 || sd == 1);
    auto side = static_cast<Side>(sd);
    auto compress = compressMode != CompressMode::compress_none;
    const std::string thePath = createFileName(folder, getName(), getEgtbType(), side, compress);

    setPath(thePath, sd);
    std::ofstream outfile (thePath, std::ofstream::binary);
    outfile.seekp(0);

    auto oldProperty = header->getProperty();

    if (compress) {
        header->addProperty(EGTB_PROP_COMPRESSED);
    } else {
        header->setProperty(header->getProperty() & ~EGTB_PROP_COMPRESSED);
    }

    if (getEgtbType() == EgtbType::newdtm) {
        header->addProperty(EGTB_PROP_NEW);
    } else {
        header->setProperty(header->getProperty() & ~EGTB_PROP_NEW);
    }
    header->setProperty(header->getProperty() & ~(EGTB_PROP_LARGE_COMPRESSTABLE_B | EGTB_PROP_LARGE_COMPRESSTABLE_W));
    
    if (compressMode == CompressMode::compress_optimizing) {
        header->addProperty(EGTB_PROP_COMPRESS_OPTIMIZED);
    } else {
        header->setProperty(header->getProperty() & ~EGTB_PROP_COMPRESS_OPTIMIZED);
    }

//    if(!isValidExtension(thePath, getEgtbType(), compress) ||
//        (getName().find("m") == std::string::npos && getEgtbType() == EgtbType::newdtm) ||
//        (getName().find("m") != std::string::npos && getEgtbType() != EgtbType::newdtm) ||
//       isCompressed() != (compressMode != CompressMode::compress_none)) {
//        std::cerr << "Error: EgtbFileWritting::saveFile, incorrect name/property for " << getName() << std::endl;
//        exit(-1);
//    }

    header->setOnlySide(side);

#ifdef _WIN32
    strncpy_s(header->getCopyright(), COPYRIGHT_BUFSZ, COPYRIGHT, strlen(COPYRIGHT));
#else
    strcpy(header->getCopyright(), COPYRIGHT);
#endif

//    header->setSignature(EGTB_ID_MAIN);
    header->resetSignature();

    bool r = true;

    if (outfile) {
        auto size = getSize();
        auto bufSz = size;
        if (isTwoBytes()) bufSz += bufSz;

        if (compress) {
            totalSize += size;
            // TODO
            int blocksize = getCompressBlockSize();
            int blockNum = (int)((bufSz + blocksize - 1) / blocksize);
            assert(blockNum > 0);

            // 5 bytes per item
            u8* blocktable = (u8*)malloc(blockNum * 5 + 64);
            i64 compBufSz = bufSz + 2 * blockNum + 2 * blocksize;
            char *compBuf = (char *)malloc(compBufSz);

            /*
             * Convert all illegal to previous one to improve compress ratio
             */
            if (compressMode == CompressMode::compress_optimizing) {
//                assert(false);
//                int sameLastCellCnt = 0;
//                u8 lastCell = 0;
//                for (i64 i = 0; i < size; i++) {
//                    u8 cell = (u8)pBuf[sd][i];
//                    if (cell == TB_ILLEGAL) {
//                        illegalCnt++;
//                        char ch = (char)lastCell;
//                        char ch2 = pBuf[sd][i + 1];
//                        if (sameLastCellCnt == 0 && ch != ch2 && ch2 != TB_ILLEGAL) {
//                            ch = ch2;
//                            sameLastCellCnt = 0;
//                        } else {
//                            sameLastCellCnt++;
//                        }
//                        pBuf[sd][i] = ch;
//                    } else {
//                        if (lastCell == cell) {
//                            sameLastCellCnt++;
//                        } else {
//                            sameLastCellCnt = 0;
//                            lastCell = cell;
//                        }
//
//                        if (cell == TB_DRAW) {
//                            drawCnt++;
//                        } else if (cell == TB_WINING || cell == TB_UNKNOWN) {
//                            compressedUndeterminedCnt++;
//                        }
//                    }
//                }
                
                auto sameLastCell = false;
                int lastScore = 0;
                for (i64 i = 0; i < size; i++) {
                    auto score = getScore(i, side);
                    if (score == EGTB_SCORE_ILLEGAL) {
                        auto b = true;
                        if (!sameLastCell && i + 1 < size) {
                            auto score2 = getScore(i + 1, side);
                            if (score2 != EGTB_SCORE_ILLEGAL) {
                                score = score2;
                                b = false;
                            }
                        }
                        if (b) {
                            sameLastCell = true;
                            score = lastScore;
                        }
                        setBufScore(i, score, sd);
                    } else {
                        sameLastCell = lastScore == score;
                    }
                    
                    lastScore = score;
                }
            }

            int64_t compSz = CompressLib::compressAllBlocks(blocksize, blocktable, compBuf, (char*)pBuf[sd], bufSz);
            assert(compSz < bufSz);

            if (compSz > bufSz || compSz > EGTB_LARGE_COMPRESS_SIZE) {
                std::cerr << "\nError: cannot compress compSz (" << compSz << " > size (" << size << ")\n";
                exit(-1);
            }

            int bytePerItem = 4;

            if (compSz > EGTB_SMALL_COMPRESS_SIZE) {
                bytePerItem = 5;
                assert( (*((i64*)(blocktable + 5 * (blockNum - 1))) & EGTB_LARGE_COMPRESS_SIZE) == compSz);

                header->addProperty(EGTB_PROP_LARGE_COMPRESSTABLE_B << sd);
                std::cout << "NOTE: Using 5 bytes per item for compress table\n\n";
            } else {
                assert((*((u32*)blocktable + blockNum - 1) & EGTB_SMALL_COMPRESS_SIZE) == compSz);
            }

            auto blockTableSize = blockNum * bytePerItem;

            if (r && !saveHeader(outfile)) {
                r = false;
            }
            
            if (egtbProduct == EgtbProduct::bug) {
                flipCode((char*)blocktable, blockTableSize);
            }

            if (r && !outfile.write ((char*)blocktable, blockTableSize)) {
                r = false;
            }

            if (r && compBuf && !outfile.write((char*)compBuf, compSz)) {
                r = false;
            }

            if (blocktable) {
                free(blocktable);
            }
            if (compBuf) {
                free(compBuf);
            }

        } else
        if (!saveHeader(outfile) || !outfile.write (pBuf[sd], bufSz)) {
            r = false;
        }
    }

    header->setProperty(oldProperty);

    return r;
}


bool EgtbFileWritting::saveFileTestIdea(const std::string& folder, int sd, CompressMode compressMode) {
    std::cout << "EgtbFileWritting::saveFileTestIdea BEGIN folder: " << folder << std::endl;

//    assert(sd == 0 || sd == 1);
//    auto side = static_cast<Side>(sd);
//    const std::string thePath = createFileName(folder, getName(), egtbType, side, compressMode != CompressMode::compress_none);
//    setPath(thePath, sd);
//    std::ofstream outfile (thePath, std::ofstream::binary);
//    outfile.seekp(0);
//
//    if (compressMode != CompressMode::compress_none) {
//        header->property |= EGTB_PROP_COMPRESSED;
//    } else {
//        header->property &= ~EGTB_PROP_COMPRESSED;
//    }
//
//    bool oW = header->isSide(Side::white);
//    bool oB = header->isSide(Side::black);
//
//    header->setOnlySide(side);
//
//#ifdef _WIN32
//    strncpy_s(header->copyright, sizeof(header->copyright), COPYRIGHT, strlen(COPYRIGHT));
//#else
//    strcpy(header->copyright, COPYRIGHT);
//#endif
//
//
////    header.signature = EGTB_ID_MAIN_V3;
//
//    bool r = true;
//
//    if (outfile && header->saveFile(outfile)) {
//        auto size = getSize();
//        if (compressMode != CompressMode::compress_none) {
//            int blocksize = EGTB_SIZE_COMPRESS_BLOCK;
//            int blockNum = (int)((size + blocksize - 1) / blocksize);
//            assert(blockNum > 0);
//
//            u32  *wdlblocktable = (u32*)malloc(blockNum * sizeof(u32) + 64);
//            char *wdlBuf = (char *)malloc(size * 2);
//            char *wdlCompBuf = (char *)malloc(size * 2);
//
//            u32* blocktable = (u32*)malloc(blockNum * sizeof(u32) + 64);
//            char *compBuf = (char *)malloc(size * 2);
//
//            /*
//             * Convert all illegal to previous one to improve compress ratio
//             */
//            u8 wdllastCell = 0, lastCell = 0;
//            for (i64 i = 0; i < size; i++) {
//                u8 cell = (u8)pBuf[sd][i];
//
//                {
//                    char ch = 0;
//                    if (cell == TB_ILLEGAL) {
//                        ch = wdllastCell;
//                    } else if (cell == TB_WINING || cell == TB_UNKNOWN) {
//                        cell = 2;
//                    } else if (cell > TB_DRAW) {
//                        ch = 1;
//                    }
//                    wdlBuf[i] = ch;
//                    wdllastCell = ch;
//                }
//
//
//                if (cell == TB_DRAW || cell == TB_ILLEGAL || cell == TB_WINING || cell == TB_UNKNOWN) {
//                    pBuf[sd][i] = lastCell;
//                } else {
//                    lastCell = cell;
//                }
//            }
//
//            int wdlCompSz = CompressLib::compressAllBlocks(blocksize, wdlblocktable, wdlCompBuf, (char*)wdlBuf, size);
//            assert(wdlCompSz < size);
//            assert((wdlblocktable[blockNum - 1] & ~EGTB_UNCOMPRESS_BIT) == wdlCompSz);
//
//            int compSz = CompressLib::compressAllBlocks(blocksize, blocktable, compBuf, (char*)pBuf[sd], size);
//            assert(compSz < size);
//            assert((blocktable[blockNum - 1] & ~EGTB_UNCOMPRESS_BIT) == compSz);
//
//
//            auto blockTableSize = blockNum * sizeof(u32);
//
//            if (r && !outfile.write ((char*)wdlblocktable, blockTableSize)) {
//                r = false;
//            }
//
//            if (r && !outfile.write ((char*)blocktable, blockTableSize)) {
//                r = false;
//            }
//
//            if (r && wdlCompBuf && !outfile.write ((char*)wdlCompBuf, wdlCompSz)) {
//                r = false;
//            }
//
//            if (r && compBuf && !outfile.write ((char*)compBuf, compSz)) {
//                r = false;
//            }
//
//            if (wdlblocktable) {
//                free(wdlblocktable);
//            }
//            if (wdlCompBuf) {
//                free(wdlCompBuf);
//            }
//
//            if (blocktable) {
//                free(blocktable);
//            }
//            if (compBuf) {
//                free(compBuf);
//            }
//
//        } else { // compressed
//            if (!outfile.write (pBuf[sd], size)) {
//                r = false;
//            }
//        }
//    }
//
//    if (oW) {
//        header->addSide(Side::white);
//    }
//    if (oB) {
//        header->addSide(Side::black);
//    }

    std::cout << "EgtbFileWritting::saveFileTestIdea END" << std::endl;
    return true;
}

void EgtbFileWritting::checkAndConvert2bytesTo1() {
    if (!isTwoBytes()) {
        return;
    }
    
    for(i64 idx = 0; idx < getSize(); ++idx) {
        auto score = getScore(idx, Side::white);
        bool confirm = false;
        if (score < EGTB_SCORE_MATE && score != EGTB_SCORE_DRAW) {
            auto dtm = EGTB_SCORE_MATE - std::abs(score);
            confirm = dtm > 248;
        } else {
            confirm = std::abs(score) >= EGTB_SCORE_PERPETUAL_BEGIN;
        }
        if (confirm) {
            std::cout << "\t\tconfirmed: 2 bytes per item." << std::endl;
            return;
        }
    }
    
    header->setProperty(header->getProperty() & ~EGTB_PROP_2BYTES);
    assert(!isTwoBytes());
    
    for(i64 idx = 0; idx < getSize(); ++idx) {
        for(int sd = 0; sd < 2; sd++) {
            i16* p = (i16*)pBuf[sd];
            i16 score = p[idx];
            setBufScore(idx, score, sd);
        }
    }
    
    std::cout << "\t\tredundant. Converted into 1 byte per item." << std::endl;
}

void EgtbFileWritting::convert1byteTo2() {
    if (isTwoBytes()) {
        return;
    }
    
    for(int sd = 0; sd < 2; sd++) {
        auto side = static_cast<Side>(sd);
        auto p = (i16*)malloc(getSize() * 2 + 64);
        for(i64 idx = 0; idx < getSize(); ++idx) {
            auto score = getScore(idx, side);
            p[idx] = score;
        }
        
        free(pBuf[sd]);
        pBuf[sd] = (char*)p;
    }

    header->setProperty(header->getProperty() | EGTB_PROP_2BYTES);
    assert(isTwoBytes());

    std::cout << "\t\tConverted into 2 bytes per item." << std::endl;
}

i64 EgtbFileWritting::convert(const std::string& newFolder) {

    int maxDTM = 0;
    int sd = W;
    //    for (int sd = 0; sd < 2; sd++) {
    i64 winCnt = 0;
    Side side = static_cast<Side>(sd);
    for(i64 idx = 0; idx < getSize(); idx++) {
        u8 cell = (u8)getCell(idx, side);
        maxDTM = std::max(maxDTM, (int)cell);

        if (cell < TB_DRAW || (cell >= EGTB_START_MATING && cell < EGTB_START_LOSING)) {
            winCnt++;
        }

        //            if (cell == TB_START_MATING + 1 && getName() == "KCMKAA") {
        //                EgtbBoard board;
        //                setup(board, idx);
        //                board.printOut("KCMKAA mate in 3");
        //            }

        //            auto cell = getScore(idx, side);
        //            if (cell > TB_SCORE_MATE || cell == TB_SCORE_DRAW) {
        //                continue;
        //            }
        //
        //            int dtm = TB_SCORE_MATE - abs(cell);
        //            maxDTM = MAX(maxDTM, dtm);
        //
        //            if ((sd == 0 && (dtm & 1) != 0) || (sd == 1 && (dtm & 1) == 0)) {
        //                err++;
        //                printf("ERROR %s, cell: %d, dtm: %d\n", getName().c_str(), cell, dtm);
        //            }
    }
    //    }

    //    return maxDTM > 3 ? 2 * (int)getSize() : 0;
    if (maxDTM <= EGTB_START_MATING + 1) {// || winCnt >= getSize() * 99 / 100) {
        //    if (winCnt >= getSize() * 99 / 100) {
        std::cout << "Ignore " << getName() << ", " << maxDTM - EGTB_START_MATING << ", " << getSize() << std::endl;
        return 0;
    }

    // draw, unknown, winning,
    //    int fitBitSz = Lib::fitBitSizeToStoreValue(maxDTM + 4);
    int fitBitSz = Lib::fitBitSizeToStoreValue(maxDTM);

    return fitBitSz * getSize() / 8 + 1;

    //    i64 newSize = getSize();

    //    char *newBuf[2];
    //
    //    for (int sd = 0; sd < 2; sd++) {
    //        newBuf[sd] = (char *)malloc(newSize + 16);
    //        memset(newBuf[sd], 0, newSize + 16);
    //        Side side = static_cast<Side>(sd);
    //        for(i64 idx = 0; idx < getSize(); idx++) {
    //            int newScore = TB_FITSCORE_UNKNOWN;
    //            auto cell = getScore(idx, side);
    //            if (cell == TB_SCORE_DRAW) {
    //                newScore = TB_FITSCORE_DRAW;
    //            } else if (cell <= TB_SCORE_MATE) {
    //                int dtm = TB_SCORE_MATE - abs(cell);
    //                newScore = TB_FITSCORE_MATING + dtm / 2;
    //            }
    //
    //            assert(newScore <= bitMasks[fitBitSz]);
    //
    //            newBuf[sd][idx] = newScore;
    //        }
    //    }
    //
    //    char* compressBuf = (char *)malloc(getSize() * 22 / 10);
    //    auto compressSize = encodeRL(pBuf[0], getSize(), compressBuf);
    //
    //    delete newBuf[0];
    //    delete newBuf[1];
    //    delete compressBuf;
    //
    //    return compressSize;


    //    delete pBuf[0];
    //    delete pBuf[1];
    //
    //    pBuf[0] = newBuf[0];
    //    pBuf[1] = newBuf[1];
    //
    //
    //    header.dtm_max = (u8)maxDTM;
    //    header.property |= TB_PROP_FIT;
    //    header.itemBitSize = (u8)fitBitSz;
    //
    //    createFileName(newFolder, EgtbType::New);
    //    saveFile();


    //    printf("EgtbFileWritting::convert, name: %s, err: %d, sz: %lld, newSz: %lld, fitBitSz: %d, maxDTM: %d\n", getName().c_str(), err, getSize(), newSize, fitBitSz, maxDTM);
    //    return (int)newSize;


    //    // Compress RL
    //    i64 idx = 0;
    //    auto cell0 = getScore(idx, Side::white);
    //    auto cell1 = getScore(idx, Side::black);
    //
    //    char* compressBuf = (char *)malloc(getSize() * 22 / 10);
    //    int newSize = Lib::encodeRL(pBuf[0], (int)getSize(), compressBuf) + Lib::encodeRL(pBuf[1], (int)getSize(), compressBuf);
    //
    //    newSize = MIN((int)getSize() * 2, newSize);
    //    return newSize;
}

bool EgtbFileWritting::verifyKey(int threadIdx, i64 idx) {
    ExtBoard board;
    if (!setupPieceList(board, idx, FlipMode::none, Side::white) ||
        !board.pieceList_setupBoard()) {
        threadRecordVec.at(threadIdx).cnt++;
        return true;
    }
    
    if (board.areKingsFacing()) {
        threadRecordVec.at(threadIdx).changes++;
    }

    egtb::EgtbKeyRec rec = getKey(board);
    i64 idx2 = rec.key, idx3;

    bool r = idx == idx2;

//    if (r) {
//        board.flip(FlipMode::vertical);
//        egtb::EgtbKeyRec rec3 = getKey(board);
//        idx3 = rec3.key;
//        if (idx != idx3) {
//            // OK for w pieces = b pieces
//            for(int t = 1; t < 7; t++) {
//                auto k = egtbPieceListStartIdxByType[t];
//                auto n = t != 6 ? 2 : 5;
//                int cnt[] = { 0, 0 };
//                for(int sd = 0; sd < 2; sd++) {
//                    for(int j = 0; j < n; j++) {
//                        if (board.pieceList[sd][k + j] >= 0) {
//                            cnt[sd]++;
//                        }
//                    }
//                }
//                if (cnt[0] != cnt[1]) {
//                    r = false;
//                    break;
//                }
//            }
//        }
//    }
//
    if (!r) {
        std::lock_guard<std::mutex> thelock(printMutex);
        board.printOut("FAILED verifyKey, idx: " + Lib::itoa(idx));
        idx2 = getKey(board).key;
        if (!setupPieceList(board, idx, FlipMode::none, Side::white) ||
            !board.pieceList_setupBoard()) {
            return true;
        }
    }
    assert(r);
    return r;
}


bool EgtbFileWritting::verifyKeys_loop(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);

    for(i64 idx = rcd.fromIdx; idx < rcd.toIdx && rcd.ok; idx++) {
        if (!verifyKey(threadIdx, idx)) {
            assert(false);
            setAllThreadNotOK();
            return false;
        }
    }
    return true;
}

i64 totalConflictLocCnt = 0, totalFacingCnt = 0, allSize = 0;

bool EgtbFileWritting::verifyKeys() {
//    std::cout << "verifyKeys: " << getName() << ", sz: " << Lib::formatString(getSize()) << std::endl;

    setupThreadRecords(getSize());
    resetAllThreadRecordCounters();

    std::vector<std::thread> threadVec;
    for (int i = 1; i < threadRecordVec.size(); ++i) {
        threadVec.push_back(std::thread(&EgtbFileWritting::verifyKeys_loop, this, i));
    }
    verifyKeys_loop(0);
    
    for (auto && t : threadVec) {
        t.join();
    }
    
    i64 conflictLocCnt = 0, facingCnt = 0;
    for(auto && rcd : threadRecordVec) {
        conflictLocCnt += rcd.cnt;
        facingCnt += rcd.changes;
    }
    totalConflictLocCnt += conflictLocCnt;
    totalFacingCnt += facingCnt;
    allSize += getSize();
    
    std::cout << getName() << "," << getSize()
        << "," << conflictLocCnt << "," << conflictLocCnt * 100 / getSize()
        << "," << facingCnt << "," << facingCnt * 100 / getSize()
        << "," << totalConflictLocCnt << "," << totalFacingCnt
        << "," << totalConflictLocCnt * 100 / allSize << "," << totalFacingCnt * 100 / allSize
        << "," << (totalConflictLocCnt + totalFacingCnt) * 100 / allSize
        << std::endl;

    auto r = allThreadOK();
    if (r) {
//        std::cout << "    passed: " << getName() << std::endl;
    } else {
        std::cerr << "    FAILED: " << getName() << std::endl;
    }
    return r;
}

void EgtbFileWritting::convertPermutations(EgtbFileWritting* fromStandardEgtbFile, bool testingSize) {
//    convertPermutations(0, getSize(), standardEgtbFile);
    assert(fromStandardEgtbFile->getHeader()->getOrder() == 0);
    if (getSize() != fromStandardEgtbFile->getSize()) {
        std::lock_guard<std::mutex> thelock(printMutex);
        auto str = getName();
        
        std::cerr << "Error: EgtbFileWritting::convertPermutations " << getName() << ", getSize() = " << getSize()
        << ", standardEgtbFile->getSize()=" << fromStandardEgtbFile->getSize() << ", name = " << fromStandardEgtbFile->getName() << std::endl;
        assert(false);
        exit(-1);
    }
    assert(getSize() == fromStandardEgtbFile->getSize());

    auto sz = getSize();
    for (i64 idx = 0; idx < sz; idx++) {
        setBufScore(idx, EGTB_SCORE_UNSET, B);
        setBufScore(idx, EGTB_SCORE_UNSET, W);
    }

    i64 testsz = 0;
    
    if (testingSize) {
        testsz = getCompressBlockSize();
        testsz *= 100;
        
        sz = std::min(sz, testsz);
    }
    
    setupThreadRecords(sz);
    
    {
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbFileWritting::convertPermutations_loop, this, i, fromStandardEgtbFile));
        }
        
        convertPermutations_loop(0, fromStandardEgtbFile);

        for (auto && t : threadVec) {
            t.join();
        }
    }
    

    if (testingSize) {
        int blocksize = getCompressBlockSize();
        int blockNum = (int)((size + blocksize - 1) / blocksize);
        assert(blockNum > 0);
        
        // 5 bytes per item
        u8* blocktable = (u8*)malloc(blockNum * 5 + 64);
        i64 compBufSz = size + 2 * blockNum + 2 * blocksize;
        char *compBuf = (char *)malloc(compBufSz);

        auto compSz0 = CompressLib::compressAllBlocks(blocksize, blocktable, compBuf, (char*)pBuf[0], size);
        auto compSz1 = CompressLib::compressAllBlocks(blocksize, blocktable, compBuf, (char*)pBuf[1], size);

        auto& rcd = threadRecordVec[0];
        rcd.cnt = std::min(compSz0, compSz1);
        
        delete blocktable;
        delete compBuf;
    }
}

int EgtbFileWritting::getEgtbIdxArraySize() const
{
    int i = 0;
    for(; egtbIdxArray[i].idx != EGTB_IDX_NONE; i++)
        ;
    return i;
}

void EgtbFileWritting::convertPermutations_loop(int threadIdx, EgtbFileWritting* fromStandardEgtbFile) {
    auto& rcd = threadRecordVec.at(threadIdx);
    
    if (egtbVerbose) {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "convertPermutations BEGIN, threadIdx: " << threadIdx << ", range: " << rcd.fromIdx << " -> " << rcd.toIdx << std::endl;
    }

    auto order = (int)header->getOrder();
    assert(order != 0);
    const int orderArray[] = { order & 0x7, order >> 3 & 0x7, order >> 6 & 0x7, order >> 9 & 0x7, order >> 12 & 0x7, order >> 15 & 0x7, order >> 18 & 0x7, order >> 21 & 0x7 };
    
    auto proCnt = getEgtbIdxArraySize();
    auto stdProCnt = fromStandardEgtbFile->getEgtbIdxArraySize();
    
    assert(proCnt == stdProCnt);
    assert(proCnt <= 8);
    
    if (proCnt == stdProCnt) {
        for(int sd = 0; sd < 2; sd++) {
            Side side = static_cast<Side>(sd);
            for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
                int score = fromStandardEgtbFile->getScore(idx, side);

                i64 subIdx[10];
                i64 rest = idx;
                for(int i = 0; i < proCnt; i++) {
                    assert(i < 12);
                    auto mul = fromStandardEgtbFile->egtbIdxArray[i].mult;
                    i64 x = rest / mul;
                    rest = rest % mul;
                    
                    auto o = orderArray[i];
                    subIdx[o] = x;
                }
                
                i64 idx2 = 0;
                for(int i = 0; i < proCnt; i++) {
                    assert(i < 12);
                    auto mul = egtbIdxArray[i].mult;
                    idx2 += subIdx[i] * mul;
                }
                
                assert(idx2 >= startpos[sd] && idx2 < endpos[sd]);
                assert(getScore(idx2, side) == EGTB_SCORE_UNSET);
                setBufScore(idx2, score, sd);
                assert(getScore(idx2, side) == score);
            }
        }
        return;
    }

    std::cerr << "Error: something wrong in EgtbFileWritting::convertPermutations" << std::endl;
    assert(false);
    exit(-1);
    
    assert(proCnt == 4 && stdProCnt == 3);
    
    i64 k2;
    auto o1 = orderArray[2];
    if (o1 == 0) k2 = getSize() / egtbIdxArray[0].mult;
    else {
        k2 = egtbIdxArray[o1 - 1].mult / egtbIdxArray[o1].mult; //idxMult[o1];
    }
    assert(k2 >= 0 && k2 < 90);
    
    for(int sd = 0; sd < 2; sd++) {
        Side side = static_cast<Side>(sd);
        for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {
            int score = fromStandardEgtbFile->getScore(idx, side);

            i64 subIdx[16];
            i64 rest = idx;
            for(int i = 0, j = 0; i < stdProCnt; i++) {
                assert(i < 4);
                auto mul = fromStandardEgtbFile->egtbIdxArray[i].mult; // idxMult[i];
                i64 x = rest / mul;
                rest = rest % mul;

                auto o = orderArray[j]; j++;
                if (i == 1) {
                    assert(x >= 0 && x < 90 * 90);
                    subIdx[o] = x / k2;
                    o = orderArray[j]; j++;
                    subIdx[o] = x % k2;
                } else {
                    subIdx[o] = x;
                }
            }
            
            i64 idx2 = 0;
            for(int i = 0; i < proCnt; i++) {
                auto mul = egtbIdxArray[i].mult; // idxMult[i];
                idx2 += subIdx[i] * mul;
                assert(mul > 0);
            }
            
            if (idx2 < 0 || idx2 >= getSize() || getScore(idx2, side) != EGTB_SCORE_UNSET) {
                std::cerr << "Error EgtbFileWritting::convertPermutations, idx2 = " << idx2 << ", getScore(idx2, side) = " << getScore(idx2, side) << " != EGTB_SCORE_UNSET" << std::endl;
                assert(false);
                exit(-1);
            }
            setBufScore(idx2, score, sd);
            assert(getScore(idx2, side) == score);
        }
    }
    
    std::cout << "EgtbFileWritting::convertPermutations DONE, threadIdx: " << threadIdx << std::endl;
}

i64 EgtbFileWritting::convertPermutations_idx(i64 idx, EgtbFileWritting* fromStandardEgtbFile) const {
    assert(idx >= 0 && idx < getSize());

    auto order = (int)header->getOrder();
    assert(order != 0);
    const int orderArray[] = { order & 0x7, order >> 3 & 0x7, order >> 6 & 0x7, order >> 9 & 0x7, order >> 12 & 0x7, order >> 15 & 0x7, order >> 18 & 0x7, order >> 21 & 0x7 };

    i64 subIdx[16];
    i64 rest = idx;
    int n = 0;
    
    for(int i = 0; egtbIdxArray[i].idx != EGTB_IDX_NONE; i++, n++) {
        assert(i < 12);
        auto mul = fromStandardEgtbFile->egtbIdxArray[i].mult;
        auto o = orderArray[i];
        subIdx[o] = rest / mul;
        rest = rest % mul;
    }
    
    i64 idx2 = 0;
    for(int i = 0; i < n; i++) {
        idx2 += subIdx[i] * egtbIdxArray[i].mult;
    }
    assert(idx2 >= 0 && idx2 < getSize());
    return idx2;
}

void EgtbFileWritting::convertScores(EgtbFile* standardEgtbFile) {
    assert(standardEgtbFile->getHeader()->getOrder() == 0);
    assert(getSize() == standardEgtbFile->getSize());

    for (i64 idx = 0; idx < getSize(); idx++) {
        setBufScore(idx, EGTB_SCORE_UNSET, B);
        setBufScore(idx, EGTB_SCORE_UNSET, W);
    }

    for(int sd = 0; sd < 2; sd++) {
        Side side = static_cast<Side>(sd);
        for (i64 idx = 0; idx < standardEgtbFile->getSize(); idx++) {
            int score = standardEgtbFile->getScore(idx, side);

            assert(getScore(idx, side) == EGTB_SCORE_UNSET);
            setBufScore(idx, score, sd);
            assert(getScore(idx, side) == score);
        }
    }
}

// From idx to board
bool EgtbFileWritting::setupPieceList(int *pieceList, i64 idx, FlipMode flip, Side strongsider) const
{
    FlipMode flipKB, flipK, flipB;
    if (strongsider == Side::white) {
        if (flip == FlipMode::none) {
            flipKB = FlipMode::none; flipK = FlipMode::vertical;
        } else {
            assert(flip == FlipMode::horizontal);
            flipKB = FlipMode::horizontal; flipK = FlipMode::rotate;
        }
    } else {
        if (flip == FlipMode::horizontal) {
            flipKB = FlipMode::none; flipK = FlipMode::vertical;
        } else {
            assert(flip == FlipMode::rotate);
            flipKB = FlipMode::horizontal; flipK = FlipMode::rotate;
        }
    }
    
    flipB = EgtbBoard::flip(flip, FlipMode::vertical);

    EgtbBoard::pieceList_reset(pieceList);

    i64 rest = idx;

    int strongSd = static_cast<int>(strongsider);
    int wsd = strongSd;
    
    auto wFlip = flip;

    for(int i = 0; ; i++) {
        auto arr = egtbIdxArray[i].idx;
        if (arr == EGTB_IDX_NONE) {
            break;
        }

        auto mult = egtbIdxArray[i].mult;
        auto key = (int)(rest / mult);
        rest = rest % mult;

        switch (arr) {
            case EGTB_IDX_DK:
            case EGTB_IDX_DA:
            case EGTB_IDX_DE:
            case EGTB_IDX_DAA:
            case EGTB_IDX_DEE:
            case EGTB_IDX_DAE:
            case EGTB_IDX_DAAE:
            case EGTB_IDX_DAEE:
            case EGTB_IDX_DAAEE:

            case EGTB_IDX_DM:
            case EGTB_IDX_DAM:
            case EGTB_IDX_DAAM:
            {
                auto kFlip = flipK;
                if (arr != EGTB_IDX_DM && arr != EGTB_IDX_DAM && arr != EGTB_IDX_DAAM) {
                    int strongD = header == nullptr || header->getOrder() == 0 ? 0 : (header->getOrder() & 0x7);
                    if (i != strongD) {
                        wsd = 1 - wsd; kFlip = flipKB;
                        wFlip = flipB;
                    }
                }

                if (!egtbKey.parseKey_defence(arr, key, pieceList, wsd, kFlip)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_R:
            case EGTB_IDX_C:
            case EGTB_IDX_H:
            case EGTB_IDX_P:
            case EGTB_IDX_R_Full:
            case EGTB_IDX_C_Full:
            case EGTB_IDX_H_Full:
            case EGTB_IDX_P_Full:
            {
                if (!egtbKey.parseKey_oneStrongPiece(key, pieceList, wsd, wFlip, arr)) {
                    return false;
                }
                break;
            }

            case EGTB_IDX_RR:
            case EGTB_IDX_RC:
            case EGTB_IDX_RH:
            case EGTB_IDX_RP:

            case EGTB_IDX_CC:
            case EGTB_IDX_HH:
            case EGTB_IDX_PP:

            case EGTB_IDX_CH:
            case EGTB_IDX_CP:
            case EGTB_IDX_HP:

            case EGTB_IDX_PPP:
            case EGTB_IDX_PPP0:
            case EGTB_IDX_PPP1:
            case EGTB_IDX_PPP2:
            case EGTB_IDX_PPP3:
            case EGTB_IDX_PPP4:
            case EGTB_IDX_PPP5:
            case EGTB_IDX_PPP6:
                
            case EGTB_IDX_RR_Full:
            case EGTB_IDX_CC_Full:
            case EGTB_IDX_HH_Full:
            case EGTB_IDX_PP_Full:
            {
                if (!egtbKey.parseKey_twoStrongPieces(key, pieceList, wsd, wFlip, arr)) {
                    return false;
                }
                break;
            }
            default:
                assert(false);
                break;
        }
    }

    return true;
}


void EgtbFileWritting::printPerpetuationStats(const char* msg) {
    
    if (msg) {
        std::cout << msg << std::endl;
    }
    
    i64 drawCnt = 0, unsetCnt = 0, pcheckCnt = 0, pcheckevasionCnt = 0, pevasionCnt = 0, pMattingCnt = 0, pLosingCnt = 0; // checkCnt = 0, evasionCnt = 0,
    for(i64 idx = 0; idx < getSize(); idx++) {
        for(int sd = 0; sd < 2; sd++) {
            auto side = static_cast<Side>(sd);
            auto score = getScore(idx, side);
            if (score == EGTB_SCORE_PERPETUAL_CHECKED) {
                pcheckCnt++;
            } else if (score == EGTB_SCORE_PERPETUAL_CHECKED_EVASION) {
                pcheckevasionCnt++;
            } else if (score == EGTB_SCORE_PERPETUAL_EVASION) {
                pevasionCnt++;
            } else if (score == EGTB_SCORE_DRAW) {
                drawCnt++;
            } else if (score == EGTB_SCORE_UNSET) {
                unsetCnt++;
            } else if (abs(score) > EGTB_SCORE_PERPETUAL_BEGIN) {
                if (score > 0) {
                    pMattingCnt++;
                } else {
                    pLosingCnt++;
                }
            }
        }
    }
    
    std::cout << "pcheckevasionCnt: " << pcheckevasionCnt << ", pcheck: " << pcheckCnt
    << ", pevasion: " << pevasionCnt << ", draw: " << drawCnt << ", unset: " << unsetCnt << ", pMattingCnt: " << pMattingCnt << ", pLosingCnt: " << pLosingCnt << std::endl;
}

