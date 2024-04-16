/**
 * This file is part of Open Chess Game Database Standard.
 *
 * Copyright (c) 2021-2022 Nguyen Pham (github@nguyenpham)
 * Copyright (c) 2021-2022 developers
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
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
    static std::string getOriginFen(ChessVariant variant);

    static std::string side2String(Side side, bool shortFrom);
    static Side string2Side(std::string s);

    static std::string chessVariant2String(ChessVariant variant);
    static char chessPieceType2Char(int pieceType);
    static int chessCoordinateStringToPos(const std::string& str);
    static std::string chessPosToCoordinateString(int pos);
    static int chessCharactorToPieceType(char ch);

    static ChessVariant string2ChessVariant(std::string s);

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

};

}

#endif /* bs_funcs_h */
