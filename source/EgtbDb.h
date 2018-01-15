
/*
 This file is part of Felicity Egtb, distributed under MIT license.

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
#include "EgtbBoard.h"

namespace egtb {

    class EgtbDb {
    public:
        EgtbDb();
        EgtbDb(const std::string& folder, EgtbMemMode egtbMemMode = EgtbMemMode::smart);
        ~EgtbDb();

        // Call it to release memory
        void removeAllBuffers();

        /// folders and files
        // set new folder as one only
        void setFolder(const std::vector<std::string>& folders);
        // add a folder
        void addFolder(const std::string& folderName);

        void preload(EgtbMemMode egtbMemMode = EgtbMemMode::smart, EgtbLoadMode loadMode = EgtbLoadMode::onrequest);
        void preload(const std::string& folder, EgtbMemMode egtbMemMode = EgtbMemMode::smart, EgtbLoadMode loadMode = EgtbLoadMode::onrequest);

        int getSize() const;

        // Scores
        int getScore(const EgtbBoard& board, AcceptScore accept = AcceptScore::real);
        int getScore(const char* fenString, AcceptScore accept=AcceptScore::real);
        int getScore(const std::vector<Piece> pieceVec, Side side, AcceptScore accept=AcceptScore::real);

        // Probe (for getting the line of moves to win
        int probe(EgtbBoard& board, MoveList& moveList);
        int probe(const std::vector<Piece> pieceVec, Side side, MoveList& moveList);
        int probe(const char* fenString, MoveList& moveList);

    private:
        int getScore(const int* pieceList, Side side, AcceptScore accept = AcceptScore::real);

        EgtbFile* getEgtbFile(const std::string& name);
        virtual EgtbFile* getEgtbFile(const int* pieceList) const;

        void closeAll();

        void addEgtbFile(EgtbFile *egtbFile);

    private:
        std::vector<std::string> folders;
        std::map<int, EgtbFile*> map;
        std::map<std::string, EgtbFile*> nameMap;

        std::vector<EgtbFile*> egtbFileVec;
    };

} //namespace egtb

#endif /* EgtbDb_h */

