//
//  TbLookupMng.h
//
//  Created by TonyPham on 9/4/17.
//

#ifndef TbLookupMng_hpp
#define TbLookupMng_hpp

#include <vector>
#include <map>
#include <string>
#include <mutex>

#include "EgtbLookupWritting.h"

namespace egtb {

class TbLookupMng {
public:
    std::vector<EgtbLookupWritting *> list;
//    std::map<int, TbLookupWritting*> matMap[2];
    std::map<std::string, EgtbLookupWritting *> nameMap[2];

    //std::mutex  mtx;

    TbLookupMng();

    std::string getClassName() const {
        return "TbLookupMng";
    }

    void add(EgtbLookupWritting* tbLookup, int sd) {
        list.push_back(tbLookup);
        nameMap[sd][tbLookup->getName()] = tbLookup;

//        matMap[sd][tbLookup->materialsignWB] = tbLookup;
//        matMap[sd][tbLookup->materialsignBW] = tbLookup;
    }

    bool preload(const std::string& folder, EgtbMemMode mode) {
        auto vec = Lib::listdir(folder);

        for (auto && path : vec) {
            preLoadFile(path, mode);
        }

        return list.size() > 0;
    }

    bool preLoadFile(const std::string& path, EgtbMemMode mode) {
        if (path.find(EGTBLU_FILENAME_EXT) == std::string::npos && path.find(EGTBLU_ZIP_FILENAME_EXT) == std::string::npos) {
            return false;
        }

        auto name = Lib::base_name(path);
        Lib::toUpper(name);
        auto ch = name[name.length() - 5];
        auto sd = ch == 'W' ? W : B;
        name = name.substr(0, name.length() - 5); // substract for w.tbl

        if (nameMap[sd].find(name) == nameMap[sd].end()) {
            EgtbLookupWritting* tbLookup = new EgtbLookupWritting();
            if (tbLookup->preload(path, mode, EgtbLoadMode::loadnow)) {
                add(tbLookup, sd);
                return true;
            } else {
                delete tbLookup;
            }
        }
        return false;
    }

    int lookup(const std::string& name, const int* pieceList, Side side, const EgtbIdxRecord* egtbIdxRecord) {
        auto sd = static_cast<int>(side);
        //if (nameMap[sd].find(name) != nameMap[sd].end()) {
            EgtbLookup* tbLookup = nameMap[sd][name];
            if (tbLookup) {
                return tbLookup->lookup(pieceList, side, egtbIdxRecord);
            }
        //}
        return EGTB_SCORE_MISSING;
    }


public:
    void add(EgtbType egtbType, const std::string& name, i64 key, Side side, const int* scoreArray, int sz) {
        assert(sz == 7 || sz == 22);

        //mtx.lock();
        auto sd = static_cast<int>(side);
        EgtbLookupWritting* tbLookup;
        if (nameMap[sd].find(name) == nameMap[sd].end()) {
            tbLookup = new EgtbLookupWritting(name, 512 * 1024 * 1024);
            add(tbLookup, sd);
        } else {
            tbLookup = nameMap[sd][name];
        }
        assert(tbLookup);
        tbLookup->add(key, scoreArray, sz, sz < 20 ? 0 : 1);
        //mtx.unlock();
    }

    void sort() {
        for(auto && tbLookup : list) {
            tbLookup->sort();
        }
    }

    bool save(const std::string& folder, CompressMode compressMode) {
        bool r = true;
        for(int sd = 0; sd < 2; sd++) {
            for(auto && tbLookup : list) {
                if (!tbLookup->save(folder, sd, compressMode)) {
                    r = false;
                }
            }
        }

        return r;
    }
};

} // namespace egtb

#endif /* TbLookupMng_hpp */
