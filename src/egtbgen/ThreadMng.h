//
//  ThreadMng.hpp
//  EgtbGen
//
//  Created by NguyenPham on 26/11/18.
//

#ifndef ThreadMng_hpp
#define ThreadMng_hpp

#include <stdio.h>
#include <vector>
#include <map>

#include "Defs.h"

class EgtbGenThreadRecord {
public:
    int threadIdx;
    i64 fromIdx, toIdx, changes, cnt;
    bool ok;

    EgtbGenThreadRecord() {}
    EgtbGenThreadRecord(int _threadIdx, i64 _fromIdx, i64 _toIdx) {
        threadIdx = _threadIdx; fromIdx = _fromIdx; toIdx = _toIdx;
    }
    void resetCounters() {
        changes = 0; cnt = 0; ok = true;
    }
};


class ThreadMng {
public:
    std::vector<EgtbGenThreadRecord> threadRecordVec;

    void setupThreadRecords(i64 size);
    
    i64 allThreadChangeCount() const {
		i64 changes = 0;
        for(auto && rcd : threadRecordVec) {
            changes += rcd.changes;
        }
        return changes;
    }
    
    void setAllThreadNotOK() {
        for(auto && rcd : threadRecordVec) {
            rcd.ok = false;
        }
    }
    
    bool allThreadOK() const {
        for(auto && rcd : threadRecordVec) {
            if (!rcd.ok) {
                return false;
            }
        }
        return true;
    }

    void resetAllThreadRecordCounters() {
        for(auto && rcd : threadRecordVec) {
            rcd.resetCounters();
        }
    }

};

#endif /* ThreadMng_hpp */
