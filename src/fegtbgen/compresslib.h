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

#ifndef CompressLib_hpp
#define CompressLib_hpp

#include "defs.h"
#include "../fegtb/egtb.h"

namespace fegtb {


class CompressLib {
public:

    static inline int compress(char *dest, const char *src, int slen) {
        return compressLzma(dest, src, slen);
    }
    static inline int decompress(char *dest, int uncompresslen, const char *src, int slen) {
        return decompressLzma(dest, uncompresslen, src, slen);
    }

    static i64 compressAllBlocks(int blocksize, u8* blocktable, char *dest, const char *src, i64 slen);


    static i64 decompressAllBlocks(int blocksize, int blocknum, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);
    static i64 decompressAllBlocks(int blocksize, int blocknum, int fromBlockIdx, int toBlockIdx, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);
    static i64 decompressAllBlocks(int numExtraThreads, int blocksize, int blocknum, u8* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);

private:
    static int compressLzma(char *dest, const char *src, int slen);
    static int decompressLzma(char *dest, int uncompresslen, const char *src, int slen);

    static i64 compressAllBlocksSingleThread(int blocksize, u8* blocktable, char *dest, const char *src, i64 slen);

};

} // namespace fegtb

#endif /* CompressLib_hpp */
