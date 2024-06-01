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
    
    static void replaceString(std::string& subject, const std::string& search, const std::string& replace);
    
    static void removeSubstrs(std::string& s, const std::string& p);
        
    /// File
    static int existFile(const char * filename);
    static std::string loadFile(const std::string& fileName);
    static std::vector<std::string> readFileToLineArray(const std::string& fileName);
    static void writeTextFile(const std::string& fileName, const std::string& content);
    static std::string base_name(std::string const & path);
    static std::string getFileName(const std::string & path);
    
    static void copy_file(const std::string& dest_file, const std::string& srce_file);
    
//    static i64 getFileSize(const std::string& fileName);
//    static std::vector<std::string> listdir(std::string dirname);
    static void createFolder(std::string dirname);
    
    
    static std::string formatSpeed(int speed);
    static std::string formatString(uint64_t num, bool format = true);
    
    static std::string formatPeriod(int seconds);
    static std::string currentTimeDate();
    static std::string number_percent(i64 num, i64 sz);
    
    static int fitBitSizeToStoreValue(int value);
    static int encodeRL(char* buf, int size, char* toBuf);
    
    static void appendStringToFile(const std::string& filepath, const std::string& string);

};

extern const std::string originalFen;

} // namespace fegtb


#endif /* Lib_hpp */
