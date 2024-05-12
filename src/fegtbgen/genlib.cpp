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
    inFile.close();
    return content;
}

void GenLib::writeTextFile(const std::string& filepath, const std::string& line)
{
    std::ofstream file;
    file.open(filepath, std::ios::out | std::ios::app);

    file.exceptions(file.exceptions() | std::ios::failbit | std::ifstream::badbit);

    file << line;
    file.close();
}

std::vector<std::string> GenLib::readFileToLineArray(const std::string& fileName){
    std::ifstream inFile;
    inFile.open(fileName);

    std::string line;
    std::vector<std::string> vec;
    while (getline(inFile, line)) {
        vec.push_back(line);
    }

    inFile.close();
    return vec;
}


#ifdef _WIN32

void GenLib::createFolder(std::string dirname) {
	if (fegtb::egtbVerbose)
		std::cout << "createFolder: " << dirname << std::endl;
    CreateDirectoryA(dirname.c_str(),NULL);
}


#else
//std::vector<std::string> GenLib::listdir(std::string dirname) {
//    DIR* d_fh;
//    struct dirent* entry;
//
//    std::vector<std::string> vec;
//
//    while( (d_fh = opendir(dirname.c_str())) == NULL) {
//        //        std::cerr << "Couldn't open directory: %s\n", dirname.c_str());
//        return vec;
//    }
//
//    dirname += "/";
//
//    while ((entry=readdir(d_fh)) != NULL) {
//
//        // Don't descend up the tree or include the current directory
//        if(strncmp(entry->d_name, "..", 2) != 0 &&
//           strncmp(entry->d_name, ".", 1) != 0) {
//
//            // If it's a directory print it's name and recurse into it
//            if (entry->d_type == DT_DIR) {
//                auto vec2 = listdir(dirname + entry->d_name);
//                vec.insert(vec.end(), vec2.begin(), vec2.end());
//            }
//            else {
//                auto s = dirname + entry->d_name;
//                vec.push_back(s);
//            }
//        }
//    }
//
//    closedir(d_fh);
//    return vec;
//}

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

void GenLib::appendStringToFile(const std::string& filepath, const std::string& string)
{
    std::ofstream file;
    file.open(filepath, std::ios::out | std::ios::app);
    if (file.fail()) {
        return;
    }

    file << string << std::endl;
}
