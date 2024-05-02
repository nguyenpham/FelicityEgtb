//
//  ExtEgtbDb.cpp
//  EgtbGen
//
//  Created by NguyenPham on 11/3/19.
//  Copyright © 2019 Softgaroo. All rights reserved.
//

#include "ExtEgtbDb.h"

using namespace egtb;

EgtbFile* ExtEgtbDb::createEgtbFile() const {
    return new ExtEgtbFile();
}

