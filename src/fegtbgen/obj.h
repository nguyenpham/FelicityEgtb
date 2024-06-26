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

#ifndef Obj_h
#define Obj_h

#include <iostream>
#include <sstream>
#include <assert.h>
#include <cstdarg>
#include <algorithm>

#include "defs.h"

namespace fegtb {

class Obj {
public:
    virtual ~Obj() {}
    
    virtual bool isValid() const {
        return false;
    }
    
    virtual std::string toString() const {
        return "";
    }
    
    virtual void printOut(const std::string& msg = "") const {
        if (!msg.empty()) {
            std::cout << msg << std::endl;
        }
        std::cout << toString() << std::endl;
    }
    
};

} // namespace fegtb


#endif /* Obj_h */
