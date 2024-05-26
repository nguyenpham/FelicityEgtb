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

#include "threadmng.h"

using namespace fegtb;

int MaxGenExtraThreads = 4;

void ThreadMng::setupThreadRecords(i64 size) {
    int extThreadCnt = size > 64 * 1024L ? MaxGenExtraThreads : 0;
    
    threadRecordVec.clear();
    if (extThreadCnt > 0) {
        i64 blockSize = size / (extThreadCnt + 1);
        EgtbGenThreadRecord mainRecord(0, 0, blockSize);
        threadRecordVec.push_back(mainRecord);
        
        for (int i = 1; i <= extThreadCnt; ++i) {
            i64 fromIdx = blockSize * i;
            i64 toIdx = i == extThreadCnt ? size : (fromIdx + blockSize);
            EgtbGenThreadRecord record(i, fromIdx, toIdx);
            threadRecordVec.push_back(record);
        }
        return;
    }
    
    EgtbGenThreadRecord mainRecord(0, 0, size);
    threadRecordVec.push_back(mainRecord);
}

