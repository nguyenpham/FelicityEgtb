//
//  Lib.hpp
//
//  Created by Nguyen Hong Pham on 1/12/16.
//

#ifndef Lib_hpp
#define Lib_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <assert.h>

#include "Defs.h"


class Lib {
public:

    static std::vector<std::string> splitString(const std::string& string, const std::string& regexString);
    static std::vector<std::string> split(const std::string &s, char delim);
    static void split(const std::string &s, char delim, std::vector<std::string> &elems);

    static void replaceString(std::string& subject, const std::string& search, const std::string& replace);

    static void removeSubstrs(std::string& s, const std::string& p);

    static void toLower(char* str);
    static void toLower(std::string& str);
    static void toUpper(std::string& str);

    static std::string& ltrim(std::string& s);
    static std::string& rtrim(std::string& s);
    static std::string& trim(std::string& s);

    // File
    static int existFile(const char * filename);
    static std::string loadFile(const std::string& fileName);
    static std::vector<std::string> readFileToLineArray(const std::string& fileName);
    static void writeTextFile(const std::string& fileName, const std::string& content);
    static std::string base_name(std::string const & path);
    static std::string getFileName(const std::string & path);

    static void copy_file(const std::string& dest_file, const std::string& srce_file);
    static i64 getFileSize(const std::string& fileName);

//    static std::vector<std::string> getAllFileNames(const std::string& pattern);
    static std::vector<std::string> listdir(std::string dirname);
    static void createFolder(std::string dirname);


    static std::string itoa(int n);
    static std::string itoa(i64 n);
    static std::string formatSpeed(int speed);
    static std::string formatString(i64 num, bool format = true);
    static int coordinateStringToPos(const char* s);
    static std::string posToCoordinateString(int pos);
    static std::string formatPeriod(int seconds);
    static std::string currentTimeDate();
    static std::string number_percent(i64 num, i64 sz);
        

    static int fitBitSizeToStoreValue(int value);
    static int encodeRL(char* buf, int size, char* toBuf);
};

extern const std::string originalFen;

#endif /* Lib_hpp */
