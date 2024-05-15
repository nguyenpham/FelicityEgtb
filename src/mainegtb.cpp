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
#include <thread>
#include <map>
#include <algorithm>
#include <iomanip>

#include "fegtb/egtbdb.h"
#include "base/funcs.h"

#include "fegtbgen/egtbgendb.h"

#include "fegtbgen/compresslib.h"

using namespace fegtb;
using namespace bslib;

extern bool twoBytes;
extern bool useBackward;
//extern bool useTempFiles;
extern i64 maxEndgameSize;

void chessDbAnalyse();

void quickTest();
void doResearch();
bool probeFen(EgtbDb&, const std::string& fenString, bool allmovescores);

#ifdef _FELICITY_CHESS_
const int maxAttackers = 5;
#else
const int maxAttackers = 3;
#endif

static void show_usage(std::string name)
{
    /// Basic guides
    std::cerr << "Usage: " << name << " <option>\n"
    << "Options:\n"
    << "  -h          Show this help message\n"
    << "  -core N     Number of cores allowed to use\n"
//    << "\t-ram N\t\tTell program RAM in GB allowed to use\n"
    << "  -n NAME     The endgame name"
#ifdef _FELICITY_CHESS_
    << " (kqrbnp)"
#else
    << " (kabrcnp: for king, advisor, bishop/elephant, rook, cannon, knight, pawn)"
#endif
    << "; could be a number of attackers\n"
    << "  -i           Show info of all existent endgames\n"
    << "  -subinfo     Show sub endgames\n"
	<< "  -fen FEN     FEN string to probe\n"
    << "  -d FOLDER    Egtb data folder, default is egtb inside program folder\n"
    << "  -d2 FOLDER   Second egtb data folder, for comparing, converting\n"
    << "  -verbose     Verbose - print more information\n"
    << "\n"
    << "  -g           Generate\n"
    << "  -notempfiles Not using temporary files\n"
//    << "  -c           Compare (need another folder d2)\n"
//    << "  -maxsize     Max index size of endgames in Giga (\"-maxsize 8\" means 8 G indexes) for generating\n"
//    << "  -minset      Min set of sub endgames for generating / showing\n"
//    << "  -zip         Compress endgames (create .ztb files)\n"
//    << "  -unzip       Uncompress endgames (create .xtb files)\n"
    << "  -v           Verify endgames (exact name or attack pieces such as ch, r-h)\n"
    << "  -vkey        Verify keys (boards <-> indeces)\n"
//    << "  -speed       Test speed\n"
//    << "  -2           2 bytes per item\n"
    << "\n"
    << "Example:\n"
#ifdef _FELICITY_CHESS_
    << "  " << name << " -n krpkp -subinfo\n"
    << "  " << name << " -n kbbkp -d d:\\mainegtb -g\n"
    << "  " << name << " -n 3 -d d:\\mainegtb -g -core 8\n"
    << "  " << name << " -d d:\\mainegtb -fen K7/8/7k/8/8/1Rp5/8/8 w - - 0 2\n"
    << "  " << name << " -n kqrkrn -v\n"
    << "  " << name << " -n 2 -vkey\n"
    << "  " << name << " -n rn -v\n"
    << "  " << name << " -n r-n -v\n"
#else
    << "  " << name << " -n knpaabbkaabb -subinfo\n"
    << "  " << name << " -n 2 -vkey\n"
    << "  " << name << " -n kraabbkaabb -d d:\\mainegtb -g\n"
    << "  " << name << " -n 1 -d d:\\mainegtb -g -core 4\n"
    << "  " << name << " -d d:\\mainegtb -fen 3ak4/4a4/9/9/9/9/n8/3AK4/9/3A5 b 0 0\n"
    << "  " << name << " -n kraaeekaaee -v\n"
    << "  " << name << " -n rn -v\n"
    << "  " << name << " -n r-n -v\n"
#endif
//    << "  " << name << " -n krcpakrc -g -minset\n"
	<< std::endl;
}

std::string explainScore(int score) {
    std::string str;
    
    switch (score) {
        case EGTB_SCORE_DRAW:
            str = "draw";
            break;
            
        case EGTB_SCORE_MISSING:
            str = "missing (board is incorrect or missing some endgame databases)";
            break;
        case EGTB_SCORE_MATE:
            str = "mate";
            break;
            
        case EGTB_SCORE_ILLEGAL:
            str = "illegal";
            break;
            
        case EGTB_SCORE_UNKNOWN:
            str = "unknown";
            break;
            
        default: {
            auto mateInPly = EGTB_SCORE_MATE - abs(score);
            auto mateIn = (mateInPly + 1) / 2; // devide 2 for full (not half or ply) moves
            if (score < 0) mateIn = -mateIn;
            str = "mate in " + std::to_string(mateIn) + " (" + std::to_string(mateInPly) + " " + (mateInPly <= 1 ? "ply" : "plies") + ")";
            break;
        }
    }
    return str;
}

static void processName(std::string& endgameName, bool& isExactName)
{
    if (endgameName.empty()) {
        return;
    }
    
    auto k = std::count(endgameName.begin(), endgameName.end(), 'k');
    if (k != 0 && k != 2) {
        endgameName = "";
        return;
    }

    isExactName = k == 2;

#ifdef _FELICITY_CHESS_
    if (k == 0) {
        auto p = endgameName.find('-');
        if (p == std::string::npos) {
            endgameName = "k" + endgameName + "k";
        } else {
            auto s0 = endgameName.substr(0, p);
            auto s1 = endgameName.substr(p + 1);
            endgameName = "k" + s0 + "k" + s1;
        }
    }
#else
    if (k == 0) {
        auto p = endgameName.find('-');
        if (p == std::string::npos) {
            endgameName = "k" + endgameName
            + "aabbkaabb";
        } else {
            auto s0 = endgameName.substr(0, p);
            auto s1 = endgameName.substr(p + 1);
            endgameName = "k" + s0 + "aabbk" + s1 + "aabb";
        }
    }

    if (endgameName.find('*') != std::string::npos) {
        isExactName = false;
        GenLib::replaceString(endgameName, "*", "aabb");
    }
#endif

}

int main(int argc, char* argv[])
{    
    /// Quick test
 /*   {
        EgtbBoard board;
        board.setFen("1Nk5/K1n5/8/8/8/8/8/8 b - - 0 1");
        board.printOut();

        EgtbGenFile egtbFile;
        egtbFile.create("knkn");
        auto r = egtbFile.getKey(board);
        std::cout << "Key = " << r.key << std::endl;
        board.reset();
        auto ok = egtbFile.setupBoard(board, r.key, FlipMode::none, Side::white);
        board.printOut();
        std::cout << "OK = " << ok << std::endl;
    }*/

#if defined(_MSC_VER)
	setvbuf(stdout, 0, _IOLBF, 4096);
#endif
    
    static const auto programName = "egtbgen";
    std::cout << "Felicity EGTB generator for " << EGTB_MAJOR_VARIANT
    << ", by Nguyen Pham 2024, version: " << EGTB_VERSION_STRING
    << "\n" << std::endl;

    if (argc < 2) {
        show_usage(programName);
        return 1;
    }
    
    std::map <std::string, std::string> argmap;

    for (auto i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.empty() || arg.at(0) != '-' || arg == "-h" || arg == "--help") {
            show_usage(programName);
            return 0;
        }
        std::string str = arg;
        auto ok = true;

        if (arg == "-core" || arg == "-ram" || arg == "-n" || arg == "-fen" || arg == "-fenfile" || arg == "-d" || arg == "-d2" || arg == "-maxsize") {
            if (i + 1 < argc) {
                i++;
                str = argv[i];

				if (arg == "-fen") {
					while (i + 1 < argc) {
						i++;
						str += " ";
						str += argv[i];
					}
				}
            } else {
                ok = false;
            }
        }

        if (!ok || str.empty()) {
            std::cerr << arg << " requires one argument." << std::endl;
            return 1;
        }
        argmap[arg] = str;
    }

    const auto separator = CHAR_PATH_SLASH;
    std::string egtbFolder, egtbFolder2;

    if (argmap.find("-d") != argmap.end()) {
        egtbFolder = argmap["-d"];
    } else {
        std::string base, s = argv[0];
        ///std::replace(s.begin(), s.end(), '\\', separator);

        if (s.find(separator) == std::string::npos) {
            base = ".";
        }
        else {
            base = s.substr(0, s.find_last_of(separator));
        }
        egtbFolder = base + separator + "db";
    }

    if (argmap.find("-d2") != argmap.end()) {
        egtbFolder2 = argmap["-d2"];
    }
    
    egtbVerbose = argmap.find("-verbose") != argmap.end();
    
    if (argmap.find("-genforward") != argmap.end()) {
        useBackward = false;
    }

    if (argmap.find("-maxsize") != argmap.end()) {
        maxEndgameSize = std::atoi(argmap["-maxsize"].c_str()) * 1024LL * 1024LL * 1024LL;
    }
    if (argmap.find("-2") != argmap.end()) {
        twoBytes = true;
        std::cout << "generating with 2 bytes per item.\n";
    }

//    if (argmap.find("-notempfiles") != argmap.end()) {
//        useTempFiles = false;
//    }

	EgtbBoard board;

    auto showInfo = false;
    if (argmap.find("-i") != argmap.end() || argmap.find("-fen") != argmap.end() || argmap.find("-fenfile") != argmap.end()) {
        showInfo = true;

        EgtbDb egtbDb;
        egtbDb.preload(egtbFolder, EgtbMemMode::tiny);

		if (argmap.find("-i") != argmap.end()) {
            auto cnt = 0;
            i64 sz = 0;
            for(auto && egtb : egtbDb.egtbFileVec) {
                cnt++;
                sz += egtb->getSize();
                std::cout << cnt << ") " << egtb->getName() << ", " << GenLib::formatString(egtb->getSize()) << std::endl;
            }

            std::cout  << "Total: #" << cnt << ", sz: " << GenLib::formatString(sz) << std::endl;
		}
		else if (argmap.find("-fen") != argmap.end()) {
			auto fenString = argmap["-fen"];
            auto allMoveScores = argmap.find("-allmovescores") != argmap.end();
            probeFen(egtbDb, fenString, allMoveScores);
			return 1;
        } else {
            auto fileName = argmap["-fenfile"];
            auto array = GenLib::readFileToLineArray(fileName);
            for(auto && str : array) {
                auto fenString = Funcs::trim(str);
                if (!fenString.empty()) {
                    probeFen(egtbDb, fenString, argmap.find("-allmovescores") != argmap.end());
                }
            }
            return 1;
		}
    }

    std::string orgName = "";
    if (argmap.find("-n") != argmap.end()) {
        orgName = argmap["-n"];
    }
    
    auto isExactName = true;
    std::string endgameName = orgName;
    
    if (Funcs::is_integer(endgameName)) {
        isExactName = false;
        auto n = std::stoi(endgameName);
        if (n <= 0 && n > maxAttackers) {
            std::cerr << "Error: the number of attackers in para -n should be > 0 and < " << maxAttackers << std::endl;
            return 1;
        }
    } else {
        processName(endgameName, isExactName);
    }
        
    auto nameVec = EgtbGenDb::parseName(endgameName, !isExactName);

    if (nameVec.empty()) {
        if (!showInfo) {
            std::cerr << "Error: -n must be an endgame name or a number (of attackers)" << std::endl;
        }
        
        if (isExactName) {
            NameRecord record(endgameName);
            if (!record.isValid()) {
                std::cerr 
                << "Error: name " << endgameName << " is INVALID. Order for left-right sides:\n\t1) attacker numbers (more on left)\n\t2) stronger attacker (stronger on left when attackers are the same)\n"
#ifdef _FELICITY_CHESS_
                << "\t3) Attackers must be in order q, r, b, n, p\nE.g: kqrkr, krbbkp, r-b, r-p, kqpkr, rn-r\n"
#else
                << "\t3) defender number (more on left)\n\t4) advisor > elephant.\nAttackers must be in order r, c, n, p\nE.g: krakr, krbbkra, r-c, r-p, kppkr, pp-r\n"
#endif
                << std::endl;
            }
        }
        return 1;
    }
        
    if (argmap.find("-core") != argmap.end()) {
        int core = std::atoi(argmap["-core"].c_str());
        if (core > 0) {
            MaxGenExtraThreads = core - 1;
        }
    }

    /////////////////////////////////////////////////
    // Display info
    /////////////////////////////////////////////////
    if (argmap.find("-subinfo") != argmap.end()) {
        EgtbGenDb::showSubTables(nameVec, EgtbType::dtm);
    }
    
    /////////////////////////////////////////////////
    // Generate & modify data
    /////////////////////////////////////////////////
    if (argmap.find("-g") != argmap.end()) {
        EgtbGenDb egtbGenFileMng;

        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);

        egtbGenFileMng.gen_all(egtbFolder, endgameName, EgtbType::dtm, CompressMode::compress);
        return 1;
    }

    
//    if (argmap.find("-c") != argmap.end()) {
//        if (egtbFolder2.empty()) {
//            std::cerr << "Missing second folder -d2" << std::endl;
//            return -1;
//        }
//        
//        EgtbGenFileMng egtbGenFileMng;
//        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);
//        
//        EgtbGenFileMng egtbGenFileMng2;
//        egtbGenFileMng2.preload(egtbFolder2, EgtbMemMode::all);
//        
//        egtbGenFileMng.compare(egtbGenFileMng2, endgameName, !isExactName);
//        return 1;
//    }
//    
//    if (argmap.find("-zip") != argmap.end() || argmap.find("-unzip") != argmap.end()) {
//        
//        EgtbGenFileMng egtbGenFileMng;
//        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);
//        
//        egtbGenFileMng.compress(egtbFolder, endgameName, !isExactName, argmap.find("-zip") != argmap.end());
//        return 1;
//    }
    
    
    /////////////////////////////////////////////////
    // Verify functions
    /////////////////////////////////////////////////

    if (argmap.find("-v") != argmap.end()) {
        EgtbGenDb egtbGenFileMng;
        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);
        egtbGenFileMng.verifyData(nameVec);
        return 1;
    }
    
    if (argmap.find("-vkey") != argmap.end()) {
        EgtbGenDb egtbGenFileMng;
        egtbGenFileMng.verifyKeys(nameVec);
        return 1;
    }
    
    return 0;
}

bool probeFen(EgtbDb& egtbDb, const std::string& fenString, bool allMoveScores)
{
    EgtbBoard board;
    board.setFen(fenString);
    if (board.isValid()) {
        board.printOut("Board to probe");
        
        auto score = egtbDb.getScore(board);
        auto idx = egtbDb.getKey(board);
        std::cout << "score: " << score << ", explaination: " << explainScore(score) << ", idx: " << idx << std::endl;
        
        if (allMoveScores) {
            auto side = board.side, xside = getXSide(side);
            auto moveList = board.gen(side); assert(!moveList.empty());
            for(auto && move : moveList) {
                board.make(move);
                if (!board.isIncheck(side)) {
                    auto score = egtbDb.getScore(board, xside);
                    auto idx = egtbDb.getKey(board);
                    board.printOut("after move " +
                                   board.moveString_coordinate(move) +
                                   ", score: " + std::to_string(score) + ", idx: " + std::to_string(idx));
                }
                board.takeBack();
            }
            
        }
        return true;
    }
    
    std::cerr << "Error: fen is invalid\n";
    return false;
}

