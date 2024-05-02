//
//  Obj.h
//
//  Created by Nguyen Hong Pham on 1/12/16.
//

#ifndef Obj_h
#define Obj_h

#include <iostream>
#include <sstream>
#include <assert.h>
#include <cstdarg>
#include <algorithm>

#include "Defs.h"


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


#endif /* Obj_h */
