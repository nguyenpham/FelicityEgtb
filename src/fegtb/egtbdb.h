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

#ifndef fegtb_db_h
#define fegtb_db_h

#include <vector>
#include <map>
#include <string>

#include "egtb.h"
#include "egtbfile.h"

namespace fegtb {

    class EgtbFile;

    class EgtbDb {
    protected:
        std::vector<std::string> folders;
        std::map<std::string, EgtbFile*> nameMap;

    public:
        std::vector<EgtbFile*> egtbFileVec;

    public:
        EgtbDb();
        ~EgtbDb();

        /// Call it to release memory
        void removeAllProbedBuffers();

        int getSize() const {
            return (int)egtbFileVec.size();
        }

        /// Folders and files
        void setFolders(const std::vector<std::string>& folders);
        void addFolders(const std::string& folderName);

        void preload(EgtbMemMode egtbMemMode = EgtbMemMode::tiny, EgtbLoadMode loadMode = EgtbLoadMode::onrequest);
        void preload(const std::string& folder, EgtbMemMode egtbMemMode, EgtbLoadMode loadMode = EgtbLoadMode::onrequest);

        /// Scores
        int getScore(EgtbBoard& board, bslib::Side side);
        int getScore(EgtbBoard& board);
        
        i64 getKey(EgtbBoard& board);

        /// Probe (for getting the line of moves to win
        int probe(EgtbBoard& board, std::vector<bslib::MoveFull>& moveList);
        int probe(const std::string& fenString, std::vector<bslib::MoveFull>& moveList);

    public:
        EgtbFile* getEgtbFile(const std::string& name);
        virtual EgtbFile* getEgtbFile(const bslib::BoardCore& board) const;

        void closeAll();

    protected:
        static std::string getEgtbFileName(const bslib::BoardCore& board);

        void addEgtbFile(EgtbFile *egtbFile);
        bool verifyEgtbFileSides() const;

        int getScoreOnePly(EgtbBoard& board, bslib::Side side);

    };

} //namespace fegtb

#endif

