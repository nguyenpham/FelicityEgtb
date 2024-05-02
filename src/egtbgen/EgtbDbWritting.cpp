//
//  EgtbDbWritting.cpp
//  EgtbGen
//
//  Created by Tony Pham on 20/9/18.
//

#include "EgtbDbWritting.h"
#include "EgtbFileWritting.h"

using namespace egtb;

EgtbFile* EgtbDbWritting::createEgtbFile() const
{
    return new EgtbFileWritting();
}


bool EgtbDbWritting::setupPieceList(EgtbFile* egtbFile, EgtbBoard& board, i64 idx, FlipMode flip, Side strongsider)
{
    return ((EgtbFileWritting*)egtbFile)->setupPieceList((int *)board.pieceList, idx, flip, strongsider);
}

bool EgtbDbWritting::setup(EgtbFile* egtbFile, EgtbBoard& board, i64 idx, FlipMode flip, Side strongsider) const
{
    return ((EgtbFileWritting*)egtbFile)->setupPieceList((int *)board.pieceList, idx, flip, strongsider) && board.pieceList_setupBoard();
}

i64 EgtbDbWritting::getIdx(const EgtbBoard& board)
{
    auto pieceList = (const int*)board.pieceList;
    EgtbFile* pEgtbFile = getEgtbFile(pieceList);
    if (pEgtbFile == nullptr) {
        return -1;
    }
    
    auto r = pEgtbFile->getKey(pieceList);
    return r.key;
}
