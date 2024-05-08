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

#ifndef bs_funcs_h
#define bs_funcs_h

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>

#include "types.h"

namespace bslib {

class BoardCore;

class Funcs
{
public:
    static bool isChessFamily(ChessVariant variant);

    static std::string side2String(Side side, bool shortFrom);
    static Side string2Side(std::string s);

    static std::string chessVariant2String(ChessVariant variant);
    static ChessVariant string2ChessVariant(std::string s);

    static const std::string pieceTypeName;

    static PieceType charactorToPieceType(char ch);


    static void toLower(std::string& str);
    static void toLower(char* str);
    static std::string& trim(std::string& s);
    static std::string& rtrim(std::string& s);
    static char* trim(char*);
    static bool endsWith(const std::string& str, const std::string& suffix);
    static bool startsWith(const std::string& str, const std::string& prefix);


    static std::vector<std::string> splitString(const std::string &s, char delim);
    static std::vector<std::string> splitString(const std::string& s, const std::string& del);

    static std::string replaceString(std::string subject, const std::string& search, const std::string& replace);

    static std::string score2String(int centiscore, bool pawnUnit);
    static std::string score2String(double score, bool pawnUnit);

    static std::string secondToClockString(int second, const std::string& spString);
    

    static size_t getFileSize(FILE *);
    static size_t getFileSize(const std::string& path);

    static BoardCore* createBoard(ChessVariant variant);
    static std::ofstream openOfstream2write(const std::string& path);

    static std::chrono::milliseconds::rep now();

    static int bSearch(const int* array, int sz, int key);
    static void sort_tbkeys(int* tbkeys, int sz);

    static int flip(int pos, FlipMode flipMode);
    static FlipMode flip(FlipMode oMode, FlipMode flipMode);
    static bool is_integer(const std::string &str);
};

}

#endif /* bs_funcs_h */
