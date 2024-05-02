//
//  ThreadMng.cpp
//  EgtbGen
//
//  Created by NguyenPham on 26/11/18.
//

#include "ThreadMng.h"

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

