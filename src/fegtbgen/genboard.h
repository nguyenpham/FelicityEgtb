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

#ifndef GenBoard_h
#define GenBoard_h

#include <assert.h>
#include <string.h>
#include <fstream>

#include "defs.h"
#include "../chess/chess.h"
#include "../xq/xq.h"


namespace fegtb {

    class GenBoard : public EgtbBoard {
    public:
        virtual ~GenBoard() {}
        
        /// Generate retro moves without captures and promotions
        std::vector<bslib::MoveFull> gen_backward_quiet(bslib::Side attackerSide) const;
        
        bslib::FlipMode needSymmetryFlip() const;
        
#ifdef _FELICITY_CHESS_
#endif

    private:
        void genPawn_backward_quiet(std::vector<bslib::MoveFull>& moves, bslib::Side side, int pos) const;
        
    };

} /// namespace fegtb

#endif /* GenBoard_h */

