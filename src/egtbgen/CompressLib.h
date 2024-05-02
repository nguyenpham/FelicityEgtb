//
//  CompressLib.hpp
//
//  Created by TonyPham on 5/6/17.
//

#ifndef CompressLib_hpp
#define CompressLib_hpp

#include "Defs.h"
#include "../egtb/Egtb.h"

namespace egtb {


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

} // namespace egtb

#endif /* CompressLib_hpp */
