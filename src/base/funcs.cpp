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

#include <sstream>
#include <iomanip>
#include <cstring>
#include <assert.h>
#include <filesystem>

#ifdef _WIN32

#else

#include <sys/stat.h>
#endif

#include "funcs.h"
#include "../chess/chess.h"

using namespace bslib;

static const char* sideStrings[] = {
    "black", "white", "none", nullptr
};

static const char* shortSideStrings[] = {
    "b", "w", "n", nullptr
};

static const char* variantStrings[] = {
    "standard", "chess960", nullptr
};

#ifdef _FELICITY_CHESS_
const std::string Funcs::pieceTypeName = ".kqrbnp";
#endif

#ifdef _FELICITY_XQ_
/// Use symbol compatible to UCCI standard
/// Rank from 0 to 9, B: elephant, N: horse/knight
const std::string Funcs::pieceTypeName = ".kabrcnp";
#endif

bool Funcs::isChessFamily(ChessVariant variant)
{
    return variant == ChessVariant::standard || variant == ChessVariant::chess960;
}

std::string Funcs::side2String(Side side, bool shortFrom)
{
    auto sd = static_cast<int>(side);
    if (sd < 0 || sd > 1) sd = 2;
    return shortFrom ? shortSideStrings[sd] : sideStrings[sd];
}

Side Funcs::string2Side(std::string s)
{
    toLower(s);
    for(auto i = 0; sideStrings[i]; i++) {
        if (sideStrings[i] == s || shortSideStrings[i] == s) {
            return static_cast<Side>(i);
        }
    }
    return Side::none;
}

std::string Funcs::chessVariant2String(ChessVariant variant)
{
    auto t = static_cast<int>(variant);
    if (t < 0 || t >= static_cast<int>(ChessVariant::none)) t = 0;
    return variantStrings[t];
}


ChessVariant Funcs::string2ChessVariant(std::string s)
{
    if (s.empty()) return ChessVariant::standard;

    toLower(s);
    for(auto i = 0; variantStrings[i]; i++) {
        if (variantStrings[i] == s) {
            return static_cast<ChessVariant>(i);
        }
    }

    if (s.find("960") != std::string::npos || s == "fischerandom" || s == "fische random") {
        return ChessVariant::chess960;
    }
    return s == "orthodox" ? ChessVariant::standard : ChessVariant::none;
}

PieceType Funcs::charactorToPieceType(char ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        ch += 'a' - 'A';
    }

    auto p = pieceTypeName.find(ch);
    if (p != std::string::npos) {
        return static_cast<PieceType>(p);
    }

    return PieceType::empty;
}

void Funcs::toLower(std::string& str)
{
    for (size_t i = 0; i < str.size(); ++i) {
        str[i] = tolower(str[i]);
    }
}

void Funcs::toLower(char* str)
{
    for (size_t i = 0; str[i]; ++i) {
        str[i] = tolower(str[i]);
    }
}

static const char* trimChars = " \t\n\r\f\v";

/// trim from left
std::string& ltrim(std::string& s)
{
    s.erase(0, s.find_first_not_of(trimChars));
    return s;
}

/// trim from right
std::string& Funcs::rtrim(std::string& s)
{
    s.erase(s.find_last_not_of(trimChars) + 1);
    return s;
}

/// trim from left & right
std::string& Funcs::trim(std::string& s)
{
    return ltrim(rtrim(s));
}

std::vector<std::string> Funcs::splitString(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        auto s = trim(item);
        if (!s.empty())
            elems.push_back(s);
    }
    return elems;
}

std::vector<std::string> Funcs::splitString(const std::string& s, const std::string& del)
{
    std::vector<std::string> vec;
    size_t start = 0;
    auto end = s.find(del);
    while (end != -1) {
        vec.push_back(s.substr(start, end - start));
        start = end + del.size();
        end = s.find(del, start);
    }
    vec.push_back(s.substr(start, end - start));
    return vec;
}

std::string Funcs::replaceString(std::string subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
        if (pos >= subject.size()) break;
    }
    return subject;
}

std::string Funcs::score2String(int centiscore, bool pawnUnit)
{
    if (pawnUnit) {
        auto score = static_cast<double>(centiscore) / 100;
        return score2String(score, true);
    }
    return std::to_string(centiscore);
}

std::string Funcs::score2String(double score, bool pawnUnit)
{
    if (pawnUnit) {
        std::ostringstream stringStream;
        stringStream << std::fixed << std::setprecision(2) << std::showpos << score;
        return stringStream.str();
    }

    auto centiscore = static_cast<int>(score * 100);
    return score2String(centiscore, false);
}

std::string Funcs::secondToClockString(int second, const std::string& spString)
{
    auto as = std::abs(second);

    auto h = as / (60 * 60), r = as % (60 * 60), m = r / 60, s = r % 60;

    std::string str, mstring, sstring;
    if (m == 0) mstring = "00";
    else {
        if (m < 10) mstring = "0";
        mstring += std::to_string(m);
    }
    if (s == 0) sstring = "00";
    else {
        if (s < 10) sstring = "0";
        sstring += std::to_string(s);
    }

    if (second < 0) str = "-";

    if (h > 0) {
        str += std::to_string(h) + spString;
    }

    str += mstring + spString + sstring;
    return str;
}

char* Funcs::trim(char* s)
{
    if (s) {
        // trim left
        while(*s <= ' ' && *s > 0) s++;

        // trim right
        for(auto len = strlen(s); len > 0; --len) {
            auto ch = s[len - 1];
            if (ch <= ' ' && ch > 0) s[len - 1] = 0;
            else break;
        }
    }

    return s;
}

bool Funcs::endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

bool Funcs::startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

size_t Funcs::getFileSize(FILE * file)
{
    assert(file);
    
#ifdef _MSC_VER
    _fseeki64(file, 0, SEEK_END);
    size_t size = _ftelli64(file);
    _fseeki64(file, 0, SEEK_SET);
#else
    //probably should be fseeko/ftello with #define _FILE_OFFSET_BITS 64
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
#endif
    return size;
}


#ifdef _WIN32
size_t Funcs::getFileSize(const std::string& path)
{
    // Good to compile with Visual C++
    struct __stat64 fileStat;
    int err = _stat64(path.c_str(), &fileStat);
    if (0 != err) return 0;
    return fileStat.st_size;
}

#else
size_t Funcs::getFileSize(const std::string& fileName)
{
    struct stat st;
    if (stat(fileName.c_str(), &st) != 0) {
        return 0;
    }
    return st.st_size;
}

#endif


BoardCore* Funcs::createBoard(ChessVariant variant)
{
    assert(false);
    return nullptr;
//    return variant == ChessVariant::standard ? new ChessBoard() : nullptr;
}


std::ofstream Funcs::openOfstream2write(const std::string& path)
{
#ifdef _WIN32
    return std::ofstream(std::filesystem::u8path(path.c_str()), std::ios_base::out | std::ios_base::app);
#else
    return std::ofstream(path, std::ios_base::out | std::ios_base::app);
#endif
}


std::chrono::milliseconds::rep Funcs::now()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now().time_since_epoch()).count();
}


int Funcs::bSearch(const int* array, int sz, int key)
{
    auto i = 0, j = sz - 1;

    while (i <= j) {
        int idx = (i + j) / 2;
        if (key == array[idx]) {
            return idx;
        }
        if (key < array[idx]) j = idx - 1;
        else i = idx + 1;
    }

    return -1;
}

void Funcs::sort_tbkeys(int* tbkeys, int sz)
{
    std::qsort(tbkeys, sz, sizeof(int), [](const void* a, const void* b) {
        const int* x = static_cast<const int*>(a);
        const int* y = static_cast<const int*>(b);
        return (int)(*x - *y);
    });
}

#ifdef _FELICITY_CHESS_

/// for chess only
static const int flip_r90[64] = {
    7,15,23,31,39,47,55,63,
    6,14,22,30,38,46,54,62,
    5,13,21,29,37,45,53,61,
    4,12,20,28,36,44,52,60,
    3,11,19,27,35,43,51,59,
    2,10,18,26,34,42,50,58,
    1, 9,17,25,33,41,49,57,
    0, 8,16,24,32,40,48,56
};

static const int flip_r270[64] = {
    56,48,40,32,24,16, 8, 0,
    57,49,41,33,25,17, 9, 1,
    58,50,42,34,26,18,10, 2,
    59,51,43,35,27,19,11, 3,
    60,52,44,36,28,20,12, 4,
    61,53,45,37,29,21,13, 5,
    62,54,46,38,30,22,14, 6,
    63,55,47,39,31,23,15, 7
};

static const int flip_vh[64] = { // a8-h1
    0, 8,16,24,32,40,48,56,
    1, 9,17,25,33,41,49,57,
    2,10,18,26,34,42,50,58,
    3,11,19,27,35,43,51,59,
    4,12,20,28,36,44,52,60,
    5,13,21,29,37,45,53,61,
    6,14,22,30,38,46,54,62,
    7,15,23,31,39,47,55,63
};

static const int flip_hv[64] = { // a1-h8
    63,55,47,39,31,23,15, 7,
    62,54,46,38,30,22,14, 6,
    61,53,45,37,29,21,13, 5,
    60,52,44,36,28,20,12, 4,
    59,51,43,35,27,19,11, 3,
    58,50,42,34,26,18,10, 2,
    57,49,41,33,25,17, 9, 1,
    56,48,40,32,24,16, 8, 0
};
#endif

#ifdef _FELICITY_CHESS_
static auto rankCount = 8;
static auto columnCount = 8;
static auto boardSz = 64;

int Funcs::flip(int pos, FlipMode flipMode)
{
    
    switch (flipMode) {
        case FlipMode::none:
            return pos;
        case FlipMode::horizontal: {
            auto f = pos % columnCount;
            return pos - 2 * f + columnCount - 1;
        }
            
        case FlipMode::vertical: {
            auto r = pos / columnCount, c = pos % columnCount;
            return (rankCount - r - 1) * columnCount + c;
        }
        case FlipMode::rotate: {
            return boardSz - pos - 1;
        }

            /// For chess only since it has a square board
        case FlipMode::flipVH: return flip_vh[pos];
        case FlipMode::flipHV: return flip_hv[pos];
        case FlipMode::rotate90: return flip_r90[pos];
        case FlipMode::rotate270: return flip_r270[pos];

        default:
            assert(false);
            return pos;
    }
    
//    return pos;
}

static const FlipMode flipflip_h[] = { FlipMode::horizontal, FlipMode::none, FlipMode::rotate, FlipMode::rotate90, FlipMode::rotate270, FlipMode::flipHV, FlipMode::vertical, FlipMode::flipVH };
static const FlipMode flipflip_v[] = { FlipMode::vertical, FlipMode::rotate, FlipMode::none, FlipMode::rotate90, FlipMode::rotate270, FlipMode::flipVH, FlipMode::horizontal, FlipMode::flipHV };
static const FlipMode flipflip_vh[] = { FlipMode::flipVH, FlipMode::rotate270, FlipMode::rotate90, FlipMode::none, FlipMode::rotate, FlipMode::vertical, FlipMode::flipHV, FlipMode::horizontal };
static const FlipMode flipflip_r180[] = { FlipMode::rotate, FlipMode::vertical, FlipMode::horizontal, FlipMode::flipHV, FlipMode::flipVH, FlipMode::rotate270, FlipMode::none, FlipMode::rotate90 };

static const FlipMode flipflip_hv[] = { FlipMode::flipHV, FlipMode::rotate90, FlipMode::rotate270, FlipMode::rotate, FlipMode::none, FlipMode::horizontal, FlipMode::flipVH, FlipMode::vertical};
static const FlipMode flipflip_r90[] = { FlipMode::rotate90, FlipMode::flipHV, FlipMode::flipVH, FlipMode::horizontal, FlipMode::vertical, FlipMode::rotate, FlipMode::rotate270, FlipMode::none };
static const FlipMode flipflip_r270[] = { FlipMode::rotate270, FlipMode::flipVH, FlipMode::flipHV, FlipMode::vertical, FlipMode::horizontal, FlipMode::none, FlipMode::rotate90, FlipMode::rotate };

FlipMode Funcs::flip(FlipMode oMode, FlipMode flipMode) {
    switch (flipMode) {
        case FlipMode::none:
            break;

        case FlipMode::horizontal:
            return flipflip_h[static_cast<int>(oMode)];

        case FlipMode::vertical:
            return flipflip_v[static_cast<int>(oMode)];

        case FlipMode::rotate:
            return flipflip_r180[static_cast<int>(oMode)];

        /// for chess only
        case FlipMode::flipVH:
            return flipflip_vh[static_cast<int>(oMode)];

        case FlipMode::flipHV:
            return flipflip_hv[static_cast<int>(oMode)];

        case FlipMode::rotate90:
            return flipflip_r90[static_cast<int>(oMode)];

        case FlipMode::rotate270:
            return flipflip_r270[static_cast<int>(oMode)];
    }
    return oMode;
}

#else
static auto rankCount = 10;
static auto columnCount = 9;
static auto boardSz = 90;

int Funcs::flip(int pos, FlipMode flipMode)
{
    
    switch (flipMode) {
        case FlipMode::none:
            return pos;
        case FlipMode::horizontal: {
            auto f = pos % columnCount;
            return pos - 2 * f + columnCount - 1;
        }
            
        case FlipMode::vertical: {
            auto r = pos / columnCount, c = pos % columnCount;
            return (rankCount - r - 1) * columnCount + c;
        }
        case FlipMode::rotate: {
            return boardSz - pos - 1;
        }

        default:
            assert(false);
            return pos;
    }
    
//    return pos;
}

static const FlipMode flipflip_h[] = { FlipMode::horizontal, FlipMode::none, FlipMode::rotate, FlipMode::vertical };
static const FlipMode flipflip_v[] = { FlipMode::vertical, FlipMode::rotate, FlipMode::none, FlipMode::horizontal };
static const FlipMode flipflip_r[] = { FlipMode::rotate, FlipMode::vertical, FlipMode::horizontal, FlipMode::none };

FlipMode Funcs::flip(FlipMode oMode, FlipMode flipMode)
{
    switch (flipMode) {
        case FlipMode::none:
            break;
        case FlipMode::horizontal:
            return flipflip_h[static_cast<int>(oMode)];

        case FlipMode::vertical:
            return flipflip_v[static_cast<int>(oMode)];
        case FlipMode::rotate:
            return flipflip_r[static_cast<int>(oMode)];
    }
    return oMode;
}

#endif


bool Funcs::is_integer(const std::string &str)
{
    return !str.empty() && str.find_first_not_of("0123456789") == std::string::npos;
}
