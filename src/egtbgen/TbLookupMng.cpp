//
//  TbLookupMng.cpp
//
//  Created by TonyPham on 9/4/17.
//

#include <fstream>
#include <iomanip>

#include "TbLookupMng.h"

using namespace egtb;

static bool tbLookupMngInit = false;

TbLookupMng::TbLookupMng() {
    if (tbLookupMngInit) {
        return;
    }
    tbLookupMngInit = true;
}

