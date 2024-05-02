//
//  ExtEgtbDb.hpp
//  EgtbGen
//
//  Created by NguyenPham on 11/3/19.
//  Copyright © 2019 Softgaroo. All rights reserved.
//

#ifndef ExtEgtbDb_hpp
#define ExtEgtbDb_hpp

#include <assert.h>

#include "../egtb/EgtbFile.h"

namespace egtb {
    const int EGTB_ID_BUG               = 556688;
    const int EGTB_HEADER_BUG_SIZE      = 126;

    class EgtbBugFileHeader : public EgtbFileHeader {
    public:
        //*********** HEADER DATA ***************
        u32         signature;
        u16         order;
        u8          dtm_max;
        u8          notused0;
        u32         property;
        u8          notused[12];
        i64         checksum;

        char        name[20], copyright[COPYRIGHT_BUFSZ];
        char        reserver[80];
        //*********** END OF HEADER DATA **********
        
        void reset() {
            memset(&signature, 0, headerSize());
            signature = EGTB_ID_BUG;
        }
        
        char* getData() { return (char*)&signature; }
        void resetSignature() { signature = EGTB_ID_BUG; }
        u32 getSignature() const { return signature; }
        void setSignature(u32 sig) { signature = sig; }
        
        char* getCopyright() { return copyright; }
        
        bool fromBuffer(const char* buffer) {
            memcpy(&signature, buffer, EGTB_HEADER_SIZE);
            return isValid();
        }

        bool isValid() const { return signature == EGTB_ID_BUG; }

        bool isSide(bslib::Side side) const {
            return property & (1 << static_cast<int>(side));
        }
        
        void addSide(bslib::Side side) {
            property |= 1 << static_cast<int>(side);
        }
        
        void setOnlySide(bslib::Side side) {
            property &= ~((1 << bslib::W) | (1 << bslib::B));
            property |= 1 << static_cast<int>(side);
        }
        
        int headerSize() const { return EGTB_HEADER_BUG_SIZE; }
        
        int getProperty() const { return property; }
        void setProperty(int prop) { property = prop; }
        void addProperty(int prop) { property |= prop; }
        
        virtual int getOrder() const { return order; }
        virtual void setOrder(int ord) { order = ord; }
        
        const char* getName() const { return name; }
        char* getName() { return name; }
        int getDtm_max() const { return dtm_max; }
        void setDtm_max(int m) { dtm_max = m; }
    };

    class ExtEgtbFile: public EgtbFile {
    public:
        virtual bool readHeader(std::ifstream& file) {
            char buffer[200];
            
            if (file.read(buffer, 128)) {
                auto b = false;
                if (header == nullptr) {
                    b = true;
                    header = new EgtbBugFileHeader();
                    if (header->fromBuffer(buffer)) {
                        return true;
                    }
                    delete header;
                    header = new EgtbFileHeader();
                }
                if (header->fromBuffer(buffer)) {
                    return true;
                }
                
                if (b) {
                    delete header;
                }
            }
            return false;
        }
        
        virtual int getCompressBlockSize() const {
            if (header && header->getSignature() == EGTB_ID_BUG) {
                return 4000;
            }
            return EgtbFile::getCompressBlockSize();
        }

        void flipCode(char* buf, i64 len) {
            for(i64 i = 0; i < len; i++) {
                buf[i] ^= 0x18;
            }
            
            //std::cout << "flipCode for " << getName() << ", len: " << len << std::endl;
        }
        
        virtual bool readCompressTable(std::ifstream& file, bslib::Side loadingSide) {
            auto sd = static_cast<int>(loadingSide);
            auto r = EgtbFile::readCompressTable(file, loadingSide);
            if (r && header->getSignature() == EGTB_ID_BUG) {
                assert(compressBlockTables[sd]);
                auto blockTableSz = getBlockTableSize(sd);
                flipCode((char *)compressBlockTables[sd], blockTableSz);
            }
            return r;
        }
    };
    
    class ExtEgtbDb : public EgtbDb {
    public:
        virtual EgtbFile* createEgtbFile() const;

    protected:
    };
}

#endif /* ExtEgtbDb_hpp */
