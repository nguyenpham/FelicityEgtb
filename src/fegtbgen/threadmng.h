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

#ifndef ThreadMng_h
#define ThreadMng_h

#include <stdio.h>
#include <vector>
#include <map>

#include "defs.h"
#include "genboard.h"
#include "../fegtb/egtb.h"

namespace fegtb {

class EgtbGenThreadRecord {
public:
    int threadIdx;
    i64 fromIdx, toIdx, changes, cnt;
    bool ok;
    
    GenBoard* board = nullptr;
    
    EgtbGenThreadRecord() {}
    EgtbGenThreadRecord(int _threadIdx, i64 _fromIdx, i64 _toIdx) {
        threadIdx = _threadIdx; fromIdx = _fromIdx; toIdx = _toIdx;
    }
    
    ~EgtbGenThreadRecord() {
        deleteBoard();
    }

    void createBoard() {
        if (!board) {
            board = new GenBoard();
        }
    }
    
    void deleteBoard() {
        if (board) {
            delete board;
            board = nullptr;
        }

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

} // namespace fegtb

#endif /* ThreadMng_h */
