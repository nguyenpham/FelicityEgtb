//
//  TbLookupWritting.h
//
//  Created by Tony Pham on 2/11/17.
//

#ifndef TbLookupWritting_hpp
#define TbLookupWritting_hpp

#include <assert.h>
#include "../egtb/EgtbLookup.h"
#include "ThreadMng.h"
#include "Lib.h"


namespace egtb {

class EgtbLookupWritting : public EgtbLookup, public ThreadMng {
public:
    EgtbLookupWritting() : EgtbLookup()
    {}

    ~EgtbLookupWritting() {}

    EgtbLookupWritting(const std::string& aName, i64 mSz = 512 * 1024 * 1024) {
        init(aName, mSz);
    }

    std::mutex  writtingMtx;

    bool save(const std::string& folder, int sd, CompressMode compressMode);
    
    int8_t* getDataPointer(int sd) {
        return data[sd];
    }

    bool add(i64 key, const int* scoreArray, int arraySz, int groupIdx) {
        assert(key >= 0 && scoreArray && groupIdx >= 0 && groupIdx < 2);
        std::lock_guard<std::mutex> thelock(writtingMtx);

        int sz = EgtbLookup::luGroupSizes[groupIdx];
        assert(sz == arraySz);
        int8_t* p = (int8_t*)(data[groupIdx] + itemCnt[groupIdx] * (4 + sz));
        itemCnt[groupIdx] += 1;

        // 4 bytes for key
        *(u32 *)p = (u32)key;
        p += 4;

        // Should not use memcpy since scoreArray used int, buffer use int8
        for (int i = 0; i < sz; i++) {
            *p = (int8_t)scoreArray[i];
            assert(abs(*p) < 128);
            p++;
        }

        if (p - data[groupIdx] >= bufSz) {
            std::cerr << "Error: EgtbLookupWritting " << getName() << " overflow bufSz: " << bufSz << std::endl;
            exit(-1);
            return false;
        }

        return true;
    }


    void sort() {
        int groupNum = 2;
        for (int grp = 0; grp < groupNum; grp++) {
            int c = itemCnt[grp];
            if (c == 0) {
                continue;
            }
            auto itemSizeInByte = 4 + luGroupSizes[grp];
            assert(c * itemSizeInByte < bufSz);
            std::qsort(data[grp], c, itemSizeInByte, [](const void* a, const void* b) {
                const u32* x = static_cast<const u32*>(a);
                const u32* y = static_cast<const u32*>(b);
                return (int32_t)(*x - *y);
            });
        }
    }

    virtual std::string toString() const {
        std::ostringstream stringStream;
        return stringStream.str();
    }

    static std::string getFileName(const std::string& folder, const std::string& name, int sd, bool compress) {
        std::string s = name;
        Lib::toLower(s);
        return folder + "/" + s + (sd == bslib::B ? "b" : "w") + (compress ? EGTBLU_ZIP_FILENAME_EXT : EGTBLU_FILENAME_EXT);
    }

    i64         bufSz;

    void init(const std::string& aName, i64 _bufSz = 512 * 1024 * 1024) {
        removeBuffers();
        
        memset(&sign, 0, EGTBLU_HEADER_SIZE);

        reset();
        bufSz = _bufSz;

        int groupNum = 2;
        for(int i = 0; i < groupNum; i++) {
            data[i] = (int8_t*) malloc(_bufSz + 64);
            assert(data[i]);
        }

        property |= EGTBLU_PROPERTY_VERSION_2;
        assert(aName.length() < 16);
        assert(isVersion2());

#ifdef _WIN32
        strncpy_s(theName, sizeof(theName), aName.c_str(), aName.length());
#else
        strcpy(theName, aName.c_str());
#endif

	}
};

} // namespace egtb

#endif /* TbLookupWritting_hpp */
