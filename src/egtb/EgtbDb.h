/*
 This file is part of NhatMinh Egtb, distributed under MIT license.

 Copyright (c) 2018 Nguyen Hong Pham

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#ifndef EgtbDb_h
#define EgtbDb_h

#include <vector>
#include <map>
#include <string>

#include "Egtb.h"
#include "EgtbFile.h"
//#include "EgtbBoard.h"

namespace egtb {

    class EgtbDb {
    protected:
        std::vector<std::string> folders;
        std::map<std::string, EgtbFile*> nameMap;

    public:
        std::vector<EgtbFile*> egtbFileVec;

    public:
        EgtbDb();
        ~EgtbDb();

        // Call it to release memory
        void removeAllBuffers();

        int getSize() const {
            return (int)egtbFileVec.size();
        }

        // Folders and files
        void setFolders(const std::vector<std::string>& folders);
        void addFolders(const std::string& folderName);

        void preload(EgtbMemMode egtbMemMode = EgtbMemMode::tiny, EgtbLoadMode loadMode = EgtbLoadMode::onrequest);
        void preload(const std::string& folder, EgtbMemMode egtbMemMode, EgtbLoadMode loadMode = EgtbLoadMode::onrequest);

        /// Scores
        int getScore(bslib::BoardCore& board, bslib::Side side);
        int getScore(bslib::BoardCore& board);
        int getScore(const std::vector<bslib::Piece> pieceVec, bslib::Side side);

        /// Probe (for getting the line of moves to win
        int probe(bslib::BoardCore& board, MoveList& moveList);
        int probe(const std::vector<bslib::Piece> pieceVec, bslib::Side side, MoveList& moveList);
        int probe(const char* fenString, MoveList& moveList);

    public:
        EgtbFile* getEgtbFile(const std::string& name);
        virtual EgtbFile* getEgtbFile(const bslib::BoardCore& board) const;

        void closeAll();

    private:
        void addEgtbFile(EgtbFile *egtbFile);

        int getScoreOnePly(bslib::BoardCore& board, bslib::Side side);

    };

} //namespace egtb

#endif /* EgtbFileMng_hpp */

