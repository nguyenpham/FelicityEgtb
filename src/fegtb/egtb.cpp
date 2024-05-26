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

#include <string>
#include <vector>
#include <algorithm>

#include "egtb.h"

//f for scaning files from a given path
#ifdef _WIN32

#include <windows.h>

#else

#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>

#endif

/// for compression
#include "../lzma/7zTypes.h"
#include "../lzma/LzmaDec.h"


/*
 * Library functions
 */

namespace fegtb {

    bool egtbVerbose = false;

    std::string getFileName(const std::string& path) {
        auto pos = path.find_last_of("/\\");
        std::string str = pos != std::string::npos ? path.substr(pos + 1) : path;
        pos = str.find_last_of(".");
        if (pos != std::string::npos) {
            str = str.substr(0, pos);
        }
        return str;
    }

    std::string getVersion() {
        char buf[10];
        snprintf(buf, sizeof(buf), "%d.%02d", EGTB_VERSION >> 8, EGTB_VERSION & 0xff);
        return buf;
    }


#ifdef _WIN32
    static void findFiles(std::vector<std::string>& names, const std::string& dirname) {
        std::string search_path = dirname + "/*.*";

        WIN32_FIND_DATAA file;
        HANDLE search_handle = FindFirstFileA(search_path.c_str(), &file);
        if (search_handle) {
            do {
                std::string fullpath = dirname + std::string("/") + std::string(file.cFileName);
                if ((file.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && (file.cFileName[0] != '.')) {
                    findFiles(names, fullpath);
                } else {
                    names.push_back(fullpath);
                }
            } while (FindNextFileA(search_handle, &file));
            ::FindClose(search_handle);
        }
    }

    std::vector<std::string> listdir(std::string dirname) {
        std::vector<std::string> names;
        findFiles(names, dirname);
        return names;
    }

#else

    std::vector<std::string> listdir(std::string dirname) {
        DIR* d_fh;
        struct dirent* entry;

        std::vector<std::string> vec;

        while( (d_fh = opendir(dirname.c_str())) == NULL) {
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

#endif

    static void * _allocForLzma(ISzAllocPtr, size_t size) { return malloc(size); }
    static void _freeForLzma(ISzAllocPtr, void *addr) { free(addr); }
    static ISzAlloc _szAllocForLzma = { _allocForLzma, _freeForLzma };

    static const Byte lzmaPropData[5] = { 93, 0, 0, 0, 1 };

    int decompress(char *dst, int uncompresslen, const char *src, int slen) {
        SizeT srcLen = slen, dstLen = uncompresslen;
        ELzmaStatus lzmaStatus;

        SRes res = LzmaDecode(
                              (Byte *)dst, &dstLen,
                              (const Byte *)src, &srcLen,
                              lzmaPropData, LZMA_PROPS_SIZE,
                              LZMA_FINISH_ANY,
                              &lzmaStatus, &_szAllocForLzma
                              );
        return res == SZ_OK ? (int)dstLen : -1;
    }

    i64 decompressAllBlocks(int blocksize, int blocknum, u32* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen) {
        auto *s = src;
        auto p = dest;

        for(auto i = 0; i < blocknum; i++) {
            auto blocksz = (blocktable[i] & ~EGTB_UNCOMPRESS_BIT) - (i == 0 ? 0 : (blocktable[i - 1] & ~EGTB_UNCOMPRESS_BIT));
            auto uncompressed = blocktable[i] & EGTB_UNCOMPRESS_BIT;

            if (uncompressed) {
                memcpy(p, s, blocksz);
                p += blocksz;
            } else {
                auto left = uncompressedlen - (i64)(p - dest);
                auto curBlockSize = (int)std::min<i64>(left, (i64)blocksize);

                auto originSz = decompress((char*)p, curBlockSize, s, blocksz);
                p += originSz;
            }
            s += blocksz;
        }

        return (i64)(p - dest);
    }
}

