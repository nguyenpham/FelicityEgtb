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

#ifndef Lib_hpp
#define Lib_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <assert.h>

#include "defs.h"

namespace fegtb {

class GenLib {
public:
    
//    static std::vector<std::string> splitString(const std::string& string, const std::string& regexString);
//    static std::vector<std::string> split(const std::string &s, char delim);
//    static void split(const std::string &s, char delim, std::vector<std::string> &elems);
//    
    static void replaceString(std::string& subject, const std::string& search, const std::string& replace);
    
    static void removeSubstrs(std::string& s, const std::string& p);
    
//    static void toLower(char* str);
//    static void toLower(std::string& str);
//    static void toUpper(std::string& str);
//    
//    static std::string& ltrim(std::string& s);
//    static std::string& rtrim(std::string& s);
//    static std::string& trim(std::string& s);
    
    // File
    static int existFile(const char * filename);
    static std::string loadFile(const std::string& fileName);
    static std::vector<std::string> readFileToLineArray(const std::string& fileName);
    static void writeTextFile(const std::string& fileName, const std::string& content);
    static std::string base_name(std::string const & path);
    static std::string getFileName(const std::string & path);
    
    static void copy_file(const std::string& dest_file, const std::string& srce_file);
//    static i64 getFileSize(const std::string& fileName);
    
    static std::vector<std::string> listdir(std::string dirname);
    static void createFolder(std::string dirname);
    
    
//    static std::string itoa(int n);
//    static std::string itoa(i64 n);
    static std::string formatSpeed(int speed);
    static std::string formatString(i64 num, bool format = true);
    
//    static int coordinateStringToPos(const char* s);
//    static std::string posToCoordinateString(int pos);

    static std::string formatPeriod(int seconds);
    static std::string currentTimeDate();
    static std::string number_percent(i64 num, i64 sz);
    
    static int fitBitSizeToStoreValue(int value);
    static int encodeRL(char* buf, int size, char* toBuf);
};

extern const std::string originalFen;

} // namespace fegtb


#endif /* Lib_hpp */
