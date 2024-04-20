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

const char* sideStrings[] = {
    "black", "white", "none", nullptr
};
const char* shortSideStrings[] = {
    "b", "w", "n", nullptr
};

const char* variantStrings[] = {
    "standard", "chess960", nullptr
};

bool Funcs::isChessFamily(ChessVariant variant)
{
    return variant == ChessVariant::standard || variant == ChessVariant::chess960;
}

//std::string Funcs::getOriginFen(ChessVariant variant)
//{
//    return originFens[static_cast<int>(variant)];
//}

std::string Funcs::side2String(Side side, bool shortFrom)
{
    auto sd = static_cast<int>(side);
    if (sd < 0 || sd > 1) sd = 2;
    return shortFrom ? shortSideStrings[sd] : sideStrings[sd];
}

Side Funcs::string2Side(std::string s)
{
    toLower(s);
    for(int i = 0; sideStrings[i]; i++) {
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
    for(int i = 0; variantStrings[i]; i++) {
        if (variantStrings[i] == s) {
            return static_cast<ChessVariant>(i);
        }
    }

    if (s.find("960") != std::string::npos || s == "fischerandom" || s == "fische random") {
        return ChessVariant::chess960;
    }
    return s == "orthodox" ? ChessVariant::standard : ChessVariant::none;
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

// trim from right
std::string& Funcs::rtrim(std::string& s)
{
    s.erase(s.find_last_not_of(trimChars) + 1);
    return s;
}

// trim from left & right
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
    int start = 0;
    int end = s.find(del);
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


