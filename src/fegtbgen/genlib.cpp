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


#include <iostream>
#include <regex>
#include <fstream>
#include <sstream>

#include <iomanip> // put_time
#include <ctime>
#include <chrono>

// for scaning files from a given path
#ifdef _WIN32

#include <windows.h>

#else

#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>

#endif

#include "genlib.h"


namespace fegtb {
    extern bool egtbVerbose;
}

using namespace fegtb;

//const std::string originalFen = "rneakaenr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNEAKAENR w - - 0 1";

//std::vector<std::string> GenLib::splitString(const std::string& string, const std::string& regexString) {
//    std::regex re(regexString);
//    std::sregex_token_iterator first {string.begin(), string.end(), re}, last;
//    return {first, last};
//}
//
//void GenLib::split(const std::string &s, char delim, std::vector<std::string> &elems) {
//    std::stringstream ss;
//    ss.str(s);
//    std::string item;
//    while (std::getline(ss, item, delim)) {
//        elems.push_back(trim(item));
//    }
//}
//
//std::vector<std::string> GenLib::split(const std::string &s, char delim) {
//    std::vector<std::string> elems;
//    split(s, delim, elems);
//    return elems;
//}

void GenLib::replaceString(std::string& subject, const std::string& search, const std::string& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

void GenLib::removeSubstrs(std::string& s, const std::string& p) {
    for (std::string::size_type n = p.length(), i = s.find(p); i != std::string::npos; i = s.find(p)) {
        s.erase(i, n);
    }
}

//void GenLib::toLower(char* str) {
//    for(int i = 0; str[i]; ++i) {
//        str[i] = tolower(str[i]);
//    }
//}
//
//void GenLib::toLower(std::string& str) {
//    for(int i = 0; i < str.size(); ++i) {
//        str[i] = tolower(str[i]);
//    }
//}
//
//void GenLib::toUpper(std::string& str) {
//    for(int i = 0; i < str.size(); ++i) {
//        str[i] = toupper(str[i]);
//    }
//}
//
//
//static const char* trimChars = " \t\n\r\f\v";
//
//// trim from left
//std::string& GenLib::ltrim(std::string& s)
//{
//    s.erase(0, s.find_first_not_of(trimChars));
//    return s;
//}
//
//// trim from right
//std::string& GenLib::rtrim(std::string& s)
//{
//    s.erase(s.find_last_not_of(trimChars) + 1);
//    return s;
//}
//
//// trim from left & right
//std::string& GenLib::trim(std::string& s)
//{
//    return ltrim(rtrim(s));
//}
//
//std::string GenLib::posToCoordinateString(int pos) {
//    int row = pos / 9, col = pos % 9;
//    std::ostringstream stringStream;
//    stringStream << char('a' + col) << 9 - row;
//    return stringStream.str();
//}
//
//int GenLib::coordinateStringToPos(const char* s) {
//    if (*s >= 'a' && *s <= 'i' && s[1] >= '0' && s[1] <= '9') {
//        int col = *s - 'a';
//        int row = s[1] - '0';
//        return (9 - row) * 9 + col;
//    }
//    return -1;
//}


//////////////////////////////////////////////////////////
int GenLib::existFile(const char * filename)
{
    struct stat   buffer;
    return (stat (filename, &buffer) == 0);
}

std::string GenLib::loadFile(const std::string& fileName) {
    std::ifstream inFile;
    inFile.open(fileName);

    std::stringstream strStream;
    strStream << inFile.rdbuf();//read the file
    std::string content = strStream.str();//str holds the content of the file
    return content;
}

void GenLib::writeTextFile(const std::string& filepath, const std::string& line)
{
    std::ofstream file;
    //can't enable exception now because of gcc bug that raises ios_base::failure with useless message
    //file.exceptions(file.exceptions() | std::ios::failbit);
    file.open(filepath, std::ios::out | std::ios::app);
    //if (file.fail()) throw std::ios_base::failure(std::strerror(errno));

    //make sure write fails with exception if something is wrong
    file.exceptions(file.exceptions() | std::ios::failbit | std::ifstream::badbit);

    file << line;
}

std::vector<std::string> GenLib::readFileToLineArray(const std::string& fileName){
    std::ifstream inFile;
    inFile.open(fileName);

    std::string line;
    std::vector<std::string> vec;
    while (getline(inFile, line)) {
        vec.push_back(line);
    }
    return vec;
}

//std::vector<std::string> GenLib::getAllFileNames(const std::string& pattern) {
//    glob_t glob_result;
//    glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
//    std::vector<std::string> files;
//    for(unsigned int i=0; i<glob_result.gl_pathc; ++i){
//        files.push_back(std::string(glob_result.gl_pathv[i]));
//    }
//    globfree(&glob_result);
//    return files;
//}


#ifdef _WIN32

static void findFiles(std::vector<std::string>& names, const std::string& dirname) {
    std::string search_path = dirname + "/*.*";

    WIN32_FIND_DATA file;
    HANDLE search_handle = FindFirstFile(search_path.c_str(), &file);
    if (search_handle) {
        do {
            std::string fullpath = dirname + "/" + file.cFileName;
            if ((file.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && (file.cFileName[0] != '.')) {
                findFiles(names, fullpath);
            } else {
                names.push_back(fullpath);
            }
        } while (FindNextFile(search_handle, &file));
        ::FindClose(search_handle);
    }
}

std::vector<std::string> GenLib::listdir(std::string dirname) {
    std::vector<std::string> names;
    findFiles(names, dirname);
    return names;
}

void GenLib::createFolder(std::string dirname) {
	if (egtb::egtbVerbose)
		std::cout << "createFolder: " << dirname << std::endl;
    CreateDirectory(dirname.c_str(),NULL);
}

i64 GenLib::getFileSize(const std::string& fileName)
{
	std::ifstream file(fileName.c_str(), std::ifstream::in | std::ifstream::binary);

	if (!file.is_open()) {
		return 0;
	}
	//file.ignore(std::numeric_limits<std::streamsize>::max());
	std::streamsize length = file.gcount();
	file.clear();   //  Since ignore will have set eof.
	file.seekg(0, std::ios_base::beg);
	return (i64)length;
}

#else
std::vector<std::string> GenLib::listdir(std::string dirname) {
    DIR* d_fh;
    struct dirent* entry;

    std::vector<std::string> vec;

    while( (d_fh = opendir(dirname.c_str())) == NULL) {
        //        std::cerr << "Couldn't open directory: %s\n", dirname.c_str());
        return vec;
    }

    dirname += "/";

    while ((entry=readdir(d_fh)) != NULL) {

        // Don't descend up the tree or include the current directory
        if(strncmp(entry->d_name, "..", 2) != 0 &&
           strncmp(entry->d_name, ".", 1) != 0) {

            // If it's a directory print it's name and recurse into it
            if (entry->d_type == DT_DIR) {
                auto vec2 = listdir(dirname + entry->d_name);
                vec.insert(vec.end(), vec2.begin(), vec2.end());
            }
            else {
                auto s = dirname + entry->d_name;
                vec.push_back(s);
            }
        }
    }

    closedir(d_fh);
    return vec;
}

void GenLib::createFolder(std::string dirname) {
    mkdir(dirname.c_str(), 0777);
}

//i64 GenLib::getFileSize(const std::string& fileName)
//{
//    std::ifstream file(fileName.c_str(), std::ifstream::in | std::ifstream::binary);
//
//    if(!file.is_open()) {
//        return -1;
//    }
//    file.ignore( std::numeric_limits<std::streamsize>::max() );
//    std::streamsize length = file.gcount();
//    file.clear();   //  Since ignore will have set eof.
//    file.seekg( 0, std::ios_base::beg );
//    return (int)length;
//}
//
//i64 GenLib::getFileSize(const std::string& fileName)
//{
//    struct stat st;
//    if(stat(fileName.c_str(), &st) != 0) {
//        return 0;
//    }
//    return st.st_size;
//}

#endif

std::string GenLib::base_name(std::string const & path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

std::string GenLib::getFileName(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    std::string str = pos != std::string::npos ? path.substr(pos + 1) : path;
    pos = str.find_last_of(".");
    if (pos != std::string::npos) {
        str = str.substr(0, pos);
    }
    return str;
}

void GenLib::copy_file(const std::string& dest_file, const std::string& srce_file)
{
    std::ifstream srce(srce_file, std::ios::binary);
    std::ofstream dest(dest_file, std::ios::binary);
    dest << srce.rdbuf();
}

std::string GenLib::currentTimeDate() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;

#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &in_time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %X");
#else
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
#endif

    return ss.str();
}

std::string GenLib::formatString(i64 num, bool format) {
    if (num < 0) {
        return "*** overflow ***";
    }
    char buf[64];

    if (format) {
        char *p = buf;
        const char* wzero = "'%03lld", *wozr = "%lld";

        const i64 factor = 1000L;

        auto k = num;
        *p = 0;
        for (i64 i=6; i>=1; i--) {
            auto f = factor;
            for (int j = 1; j < i; j++) {
                f *= factor;
            }
            auto d = k / f;
            k = k % f;
            if (d != 0 || p != buf) {
                snprintf(p, 6, p != buf ? wzero : wozr, d);
                p += strlen(p);
            }
        }
        snprintf(p, 32, p != buf ? wzero : wozr, k);
    } else {
        snprintf(buf, sizeof(buf), "%lld", num);
    }

    return buf;
}

std::string GenLib::formatSpeed(int speed) {
    const char* unit;
    if (speed > 10000000) {
        speed = (speed + 500000) / 1000000;
        unit = "mps";
    } else
    if (speed > 10000) {
        speed = (speed + 500) / 1000;
        unit = "kps";
    } else {
        unit = "nps";
    }
    return GenLib::formatString(speed) + unit;
}

std::string GenLib::formatPeriod(int seconds) {
    int s = seconds % 60, minutes = seconds / 60, m = minutes % 60, hours = minutes / 60, h = hours % 24, d = hours / 24;
    char buf[128], *str = buf;

    if (d > 0) {
        snprintf(str, 64, "%d d ", d);
        str += strlen(str);
    }

    if (h > 0) {
        if (str == buf) {
            snprintf(str, 64, "%d:", h);
        } else {
            snprintf(str, 64, "%02d:", h);
        }
        str += strlen(str);
    }

    snprintf(str, 16, "%02d:%02d", m, s);
    return buf;
}


std::string GenLib::number_percent(i64 num, i64 sz) {
    return formatString(num) + " (" + std::to_string(num * 100 / sz) + "%)";
}

//std::string GenLib::itoa(int n) {
//    char buf[120];
//    snprintf(buf, sizeof(buf), "%d", n);
//    return buf;
//}
//
//std::string GenLib::itoa(i64 n) {
//    char buf[120];
//    snprintf(buf, sizeof(buf), "%lld", n);
//    return buf;
//}

int GenLib::fitBitSizeToStoreValue(int value) {
    for (int i = 1; i < 64; i++) {
        int k = 1 << i;
        if (k > value) {
            return i;
        }
    }

    return 0;
}

int GenLib::encodeRL(char* buf, int size, char* toBuf) {
    char *p = buf, *e = p + size, *q = toBuf, ch = *p;
    int cnt = 1;
    for(p++; p < e; p++) {
        if (ch == *p && cnt < 254) {
            cnt++;
            continue;
        }
        *q = (unsigned char)cnt; q++;
        *q = ch; q++;
        ch = *p;
        cnt = 1;
    }
    *q = (unsigned char)cnt; q++;
    *q = ch; q++;

    return (int)(q - toBuf);
}
