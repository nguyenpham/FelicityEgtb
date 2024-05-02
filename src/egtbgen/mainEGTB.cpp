//
//  mainEGTB.cpp
//
//  Created by TonyPham on 11/10/17.
//

#include <iostream>
#include <thread>
#include <map>
#include <algorithm>
#include <iomanip>

#include "../egtb/EgtbDb.h"
#include "EgtbGenFileMng.h"
#include "EgtbNewGenFileMng.h"
#include "TbLookupMng.h"

#include "CompressLib.h"

using namespace egtb;

extern bool twoBytes;
extern bool useBackward;
extern bool useTempFiles;
extern i64 maxEndgameSize;

void chessDbAnalyse();

void quickTest();
void doResearch();
bool probeFen(EgtbDbWritting& egtbDb, const std::string& fenString, bool allmovescores);

static void show_usage(std::string name)
{
    std::cerr << "Usage: " << name << " <option>\n"
    << "Options:\n"
    << "  -h          Show this help message\n"
    << "  -core N     Number of cores allowed to use\n"
//    << "\t-ram N\t\tTell program RAM in GB allowed to use\n"
    << "  -n NAME     Specify the endgame name (k: king, a: advisor, e: elephant, r: rook, c: cannon, h: horse, p: pawn)\n"
    << "  -i          Show info of all existent endgames\n"
    << "  -subinfo    Show sub endgames\n"
	<< "  -fen FEN    FEN string to probe\n"
    << "  -d FOLDER   Egtb data folder, default is egtb inside program folder\n"
    << "  -d2 FOLDER  Second egtb data folder, for comparing, converting\n"
    << "  -verbose    Verbose - print more information\n"
    << "\n"
    << "  -g          Generate\n"
//    << "  -fixcc      Find and fix perpetual checks / chases\n"
    << "  -c          Compare (need another folder d2)\n"
    << "  -maxsize    Max index size of endgames in Giga (\"-maxsize 8\" means 8 G indexes) for generating\n"
//    << "  -minset     Min set of sub endgames for generating / showing\n"
    << "  -zip        Compress endgames (create .ztb files)\n"
    << "  -unzip      Uncompress endgames (create .xtb files)\n"
    << "  -v          Verify endgames (exact name or attack pieces such as ch, r-h)\n"
    << "  -vkey       Verify keys (boards <-> indeces)\n"
    << "  -speed      Test speed\n"
    << "  -2          2 bytes per item\n"
    << "  -bug N      Create optimized files for bugchess, N = 1: one-side-smallest-size files only, N = 2: both side files\n"
    << "\n"
    << "Example:\n"
    << "  " << name << " -n khpaaeekaaee -subinfo\n"
    << "  " << name << " -d d:\\mainegtb -n kraaeekaaee -g -core 4\n"
	<< "  " << name << " -fen 3ak4/4a4/9/9/9/9/h8/3AK4/9/3A5 b 0 0\n"
    << "  " << name << " -n kraaeekaaee -v\n"
    << "  " << name << " -n rh -v\n"
    << "  " << name << " -n r-h -v\n"
//    << "  " << name << " -n krcpakrc -g -minset\n"
	<< std::endl;
}

std::string explainScore(int score) {
    std::string str = "";
    
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
            
        case EGTB_SCORE_WINNING:
            str = "winning";
            break;
            
        case EGTB_SCORE_ILLEGAL:
            str = "illegal";
            break;
            
//        case EGTB_SCORE_START_CHECKING:
        case EGTB_SCORE_PERPETUAL_CHECKED:
            str = "The active side may be being checked permanently (winning)";
            break;
            
//        case EGTB_SCORE_START_EVASION:
        case EGTB_SCORE_PERPETUAL_EVASION:
            str = "The active side may be checking permanently (illegal / losing)";
            break;
            
        case EGTB_SCORE_UNKNOWN:
            str = "unknown";
            break;
            
        default: {
            char buf[250];
            auto mateInPly = EGTB_SCORE_MATE - abs(score);
            int mateIn = (mateInPly + 1) / 2; // devide 2 for full (not half or ply) moves
            if (score < 0) mateIn = -mateIn;
            
            snprintf(buf, sizeof(buf), "mate in %d (%d %s)", mateIn, mateInPly, mateInPly <= 1 ? "ply" : "plies");
            str = buf;
            break;
        }
    }
    return str;
}

static void processName(std::string& endgameName, bool& isExactName) {
    if (endgameName.empty()) {
        return;
    }

    auto k = std::count(endgameName.begin(), endgameName.end(), 'k');
    if (k != 0 && k != 2) {
        endgameName = "";
        return;
    }

    isExactName = k == 2;

    if (k == 0) {
        auto p = endgameName.find('-');
        if (p == std::string::npos) {
            endgameName = "k" + endgameName + "aaeekaaee";
        } else {
            auto s0 = endgameName.substr(0, p);
            auto s1 = endgameName.substr(p + 1);
            endgameName = "k" + s0 + "aaeek" + s1 + "aaee";
        }
    }

    if (endgameName.find('*') != std::string::npos) {
        isExactName = false;
        Lib::replaceString(endgameName, "*", "aaee");
    }
}

int main(int argc, char* argv[])
{
#if defined(_MSC_VER)
	setvbuf(stdout, 0, _IOLBF, 4096);
#endif

    static const auto programName = "egtbgen";
    std::cout << "Xiangqi EGTB generator, by Nguyen Pham 2018, version: " << (EGTB_GENERATOR_VERSION >> 8) << "."
        << std::setfill('0') << std::setw(2) << (EGTB_GENERATOR_VERSION & 0xff) << std::endl << std::setfill(' ') << std::endl;

    if (argc < 2) {
        show_usage(programName);
        return 1;
    }
    
    std::map <std::string, std::string> argmap;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.empty() || arg.at(0) != '-' || arg == "-h" || arg == "--help") {
            show_usage(programName);
            return 0;
        }
        std::string str = arg;
        auto ok = true;

        if (arg == "-core" || arg == "-ram" || arg == "-n" || arg == "-fen" || arg == "-fenfile" || arg == "-d" || arg == "-d2" || arg == "-maxsize" || arg == "-bug") {
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

    const auto separator = '/';
    std::string egtbFolder, egtbFolder2;

    if (argmap.find("-d") != argmap.end()) {
        egtbFolder = argmap["-d"];
    } else {
        std::string base, s = argv[0];
        std::replace(s.begin(), s.end(), '\\', separator);

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

    if (argmap.find("-notempfiles") != argmap.end()) {
        useTempFiles = false;
    }

	ExtBoard board;

    auto showInfo = false;
    if (argmap.find("-i") != argmap.end() || argmap.find("-fen") != argmap.end() || argmap.find("-fenfile") != argmap.end()) {
        showInfo = true;

		EgtbDbWritting egtbDb;
		egtbDb.preload(egtbFolder, EgtbMemMode::tiny);

		if (argmap.find("-i") != argmap.end()) {
			egtbDb.printOut();
		}
		else if (argmap.find("-fen") != argmap.end()) {
			auto fenString = argmap["-fen"];
            probeFen(egtbDb, fenString, argmap.find("-allmovescores") != argmap.end());
			return 1;
        } else {
            auto fileName = argmap["-fenfile"];
            auto array = Lib::readFileToLineArray(fileName);
            for(auto && str : array) {
                auto fenString =  Lib::trim(str);
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

    if (argmap.find("-gnew") != argmap.end() || argmap.find("-vnew") != argmap.end()) {
        if (egtbFolder2.empty()) {
            std::cerr << "Missing second folder -d2 (for newdtm folder) when working with -gnew and -vnew" << std::endl;
            return -1;
        }
        if (orgName.find("m") == std::string::npos) {
            std::cerr << "Name musts contain 'm' (such as krcamkae, rcam-r when working with -gnew and -vnew" << std::endl;
            return -1;
        }
    }
    
    
    if (argmap.find("-speed") != argmap.end()) {
        {
            std::cout << "Test speed, load all (whole files) into memory, onrequest:" << std::endl;
            EgtbGenFileMng egtbGenFileMng;
            egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all, EgtbLoadMode::onrequest);
            egtbGenFileMng.testSpeed(orgName, true);
            egtbGenFileMng.removeAllBuffers();
        }
        {
            std::cout << std::endl << "Test speed, load tiny (a 4 KB data block for each file) into memory, onrequest:" << std::endl;
            EgtbGenFileMng egtbGenFileMng;
            egtbGenFileMng.preload(egtbFolder, EgtbMemMode::tiny, EgtbLoadMode::onrequest);
            egtbGenFileMng.testSpeed(orgName, true);
            egtbGenFileMng.removeAllBuffers();
        }
        
        return 1;
    }

    
    bool isExactName = true;
    std::string endgameName = orgName;
    processName(endgameName, isExactName);

    if (endgameName.empty()) {
        if (!showInfo) {
            std::cerr << "Error: -n must be set by a name" << std::endl;
        }
        return 1;
    }
    
//    auto minset = argmap.find("-minset") != argmap.end();
//    if (minset && !isExactName) {
//        std::cerr << "Error: -minset must work with a specific name only (no '-' in middle) such as -n krcaekrce" << std::endl;
//        return 1;
//    }

    NameRecord record(endgameName);
    if (!record.isValid()) {
        std::cerr << "Error: name " << endgameName << " is INVALID. Order for left-right sides:\n\t1) attacker numbers (more on left)\n\t2) stronger attacker (stronger on left when attackers are the same)\n\t3) defender number (more on left)\n\t4) advisor > elephant.\nAttackers must be in order r, c, h, p\nE.g: krakr, kreekra, r-c, r-p, kppkr, pp-r, kramkcae, caam\n" << std::endl;
        return -1;
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
        EgtbGenFileMng::showSubTables(endgameName, EgtbType::dtm);
    }
    
    /////////////////////////////////////////////////
    // Generate & modify data
    /////////////////////////////////////////////////
    if (argmap.find("-g") != argmap.end()) {
        EgtbGenFileMng egtbGenFileMng;

        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);

        egtbGenFileMng.genEgtb(egtbFolder, endgameName, EgtbType::dtm, EgtbProduct::std, CompressMode::compress);
        return 1;
    }

    if (argmap.find("-gnew") != argmap.end()) {
        assert(!egtbFolder2.empty());

        bool permutate = true;

        EgtbNewGenFileMng egtbGenFileMng;
        egtbGenFileMng.genWinningEgtbs(egtbFolder, egtbFolder2, endgameName, !isExactName, permutate);
        return 1;
    }
    
    if (argmap.find("-fixcc") != argmap.end()) {
        EgtbGenFileMng egtbGenFileMng;
        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);
        egtbGenFileMng.perpetuationFix(endgameName, !isExactName);
        return 1;
    }
    
    if (argmap.find("-bug") != argmap.end()) {
        if (egtbFolder2.empty()) {
            std::cerr << "Missing second folder -d2" << std::endl;
            return -1;
        }
        int n = std::atoi(argmap["-bug"].c_str());
        if (n != 1 && n != 2) {
            std::cerr << "Wrong value for para -bug, must be 1 (take the smallest egtbFile file) or 2 (both side files)" << std::endl;
            return -1;
        }

        EgtbGenFileMng egtbGenFileMng;
        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);

        EgtbGenFileMng egtbGenFileMng2;
        egtbGenFileMng2.preload(egtbFolder2, EgtbMemMode::all);
        
        auto forAllSides = n == 2;
        egtbGenFileMng2.createProduct(egtbGenFileMng, egtbFolder2, endgameName, EgtbProduct::bug, forAllSides);

        return 1;
    }
    
    if (argmap.find("-c") != argmap.end()) {
        if (egtbFolder2.empty()) {
            std::cerr << "Missing second folder -d2" << std::endl;
            return -1;
        }
        
        EgtbGenFileMng egtbGenFileMng;
        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);
        
        EgtbGenFileMng egtbGenFileMng2;
        egtbGenFileMng2.preload(egtbFolder2, EgtbMemMode::all);
        
        egtbGenFileMng.compare(egtbGenFileMng2, endgameName, !isExactName);
        return 1;
    }
    
    if (argmap.find("-zip") != argmap.end() || argmap.find("-unzip") != argmap.end()) {
        
        EgtbGenFileMng egtbGenFileMng;
        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);
        
        egtbGenFileMng.compress(egtbFolder, endgameName, !isExactName, argmap.find("-zip") != argmap.end());
        return 1;
    }
    
    
    /////////////////////////////////////////////////
    // Verify functions
    /////////////////////////////////////////////////

    if (argmap.find("-v") != argmap.end()) {
        EgtbGenFileMng egtbGenFileMng;
        egtbGenFileMng.preload(egtbFolder, EgtbMemMode::all);
        egtbGenFileMng.verifyData(endgameName, !isExactName);
        return 1;
    }
    
    if (argmap.find("-vnew") != argmap.end()) {
        EgtbNewGenFileMng egtbGenFileMng;
        egtbGenFileMng.verifyData(egtbFolder, egtbFolder2, endgameName, !isExactName);
        return 1;
    }
    
    if (argmap.find("-vkey") != argmap.end()) {
        EgtbGenFileMng egtbGenFileMng;
        egtbGenFileMng.verifyKeys(endgameName, !isExactName);
        return 1;
    }
    
    return 0;
}

bool probeFen(EgtbDbWritting& egtbDb, const std::string& fenString, bool allMoveScores)
{
    ExtBoard board;
    board.setFen(fenString);
    if (board.isValid()) {
        board.printOut("Board to check");
        
        auto acceptScore = AcceptScore::real;  // AcceptScore::winning);
        MoveList moveList;
        auto score = egtbDb.getScore(board, acceptScore);
        auto idx = egtbDb.getIdx(board);
        std::cout << "score: " << score << ", explaination: " << explainScore(score) << ", idx: " << idx << std::endl;
        
        if (allMoveScores) {
            auto side = board.side, xside = getXSide(side);
            board.gen(moveList, side, false); assert(!moveList.isEmpty());
            for(int i = 0; i < moveList.end; i++) {
                auto m = moveList.list[i];

                board.make(m);
                if (!board.isIncheck(side)) {
                    auto score = egtbDb.getScore(board, xside, acceptScore);
                    auto idx = egtbDb.getIdx(board);
                    board.printOut("after move " + m.toString() + ", score: " + std::to_string(score) + ", idx: " + Lib::itoa(idx));
                }
                board.takeBack();
            }
            
        }
        return true;
    }
    
    std::cerr << "Error: fen is invalid\n";
    return false;
}

