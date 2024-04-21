/**
 This file is part of Felicity Egtb, distributed under MIT license.

 * Copyright (c) 2024 Nguyen Pham (github@nguyenpham)
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

#include <iostream>
#ifdef _FELICITY_CHESS_
#include "chess/chess.h"
#else
#include "xq/xq.h"
#endif

#include "base/funcs.h"
#include <random>
#include <cmath>

int main(int argc, const char * argv[])
{
    std::cout << "Welcome to Felicity Endgame databases - version: 0.00001" << std::endl;

    std::random_device rd;
    std::mt19937_64 e2(rd());
    std::uniform_int_distribution<long long int> dist(std::llround(std::pow(2,61)), std::llround(std::pow(2,62)));

    
    const int64_t sz = 1024UL * 1024 * 1024 * 7;
    const int64_t n = 1024UL * 1024 * 1024;

    {
        char* p = (char *) malloc(sz);
        assert(p);
        
        auto start = bslib::Funcs::now();
        
        
        for (auto i = n; i > 0; i--) {
            
            int64_t k = dist(e2) % sz;
            p[k] = rand() & 0xff;
        }
        
        auto elapsed = bslib::Funcs::now() - start;
        std::cout << "Test 1, malloc, elapsed (ms)  : " << elapsed
        << std::endl;
        
        free(p);
    }
    
    {
        std::vector<char> p;
        p.reserve(sz);
                
        auto start = bslib::Funcs::now();
                
        for (auto i = n; i > 0; i--) {

            int64_t k = dist(e2) % sz;
            p[k] = rand() & 0xff;
        }
        
        
        auto elapsed = bslib::Funcs::now() - start;
        std::cout << "Test 2, vector, elapsed (ms)  : " << elapsed
        << std::endl;
    }

    {
        std::array<char, sz> p;
        
        auto start = bslib::Funcs::now();
                
        for (auto i = n; i > 0; i--) {
            int64_t k = dist(e2) % sz;
            p[k] = rand() & 0xff;
        }
        
        
        auto elapsed = bslib::Funcs::now() - start;
        std::cout << "Test 3, array, elapsed (ms)  : " << elapsed
        << std::endl;
    }

    return 0;
}

