//
//  EgtbNewGenFileMng.cpp
//
//  Created by TonyPham on 12/3/17.
//


#include "EgtbNewGenFileMng.h"
#include "TbLookupMng.h"
#include "../egtb/EgtbKey.h"

#include <vector>
#include <set>
#include <thread>


//class NameAndOrder {
//public:
//    const char* name;
//    int order;
//};
//
//extern const NameAndOrder nameAndGoodOrder[];

using namespace egtb;
using namespace bslib;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
i64 totalIdxCnt = 0, normalCnt = 0, winningCnt = 0, undeterminedCnt = 0;

bool EgtbNewGenFileMng::genSingleWinningEgtb_loop(int threadIdx) {
    auto& rcd = threadRecordVec.at(threadIdx);

    ExtBoard board;

    auto ver2 = true; // tbLookup.isVersion2(); assert(ver2);

    for (i64 idx = rcd.fromIdx; idx < rcd.toIdx; idx++) {

        if (!setup(egtbFile, board, idx, FlipMode::none, Side::white)) {
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, B);
            egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, W);
            continue;
        }

        assert(board.isValid());

        for (int sd = 0, contSd = 1; sd < 2 && contSd; sd++) {
            auto side = static_cast<Side>(sd);

            if (board.isIncheck(getXSide(side))) {
                egtbFile->setBufScore(idx, EGTB_SCORE_ILLEGAL, sd);
                continue;
            }

            int e0 = board.pieceList[W][3];
            int e1 = board.pieceList[W][4];
            assert(e0 <= 0 || (e0 == 67 && board.getPiece(e0).isPiece(PieceType::elephant, Side::white)));
            assert(e1 <= 0 || (e1 == 67 && board.getPiece(e1).isPiece(PieceType::elephant, Side::white)));

            int score = traEgtbFileMng->getScore(board, side);
            if (score == EGTB_SCORE_MISSING) {
                board.printOut("FATAL ERROR: missing endgame when probing the board:");
                exit(-1);
            }

            assert(score <= EGTB_SCORE_MATE);

            int scoreArray[32], end = 0;
            scoreArray[end++] = EgtbFileWritting::scoreToCell(score, ver2);
            assert(score == EgtbFileWritting::cellToScore(scoreArray[end - 1], ver2));

            if (score <= EGTB_SCORE_MATE) {
                bool noMidElephant = board.getPiece(67).type != PieceType::elephant;
                // first elephants
                for (int i = 0; i < 6; i++) {
                    int e0 = EgtbBoard::flip(blackElephantPosWOMiddle[i], FlipMode::vertical); // epos[i];
                    if (!board.isEmpty(e0)) {
                        scoreArray[end++] = 0;
                        if (noMidElephant) {
                            for (int j = i + 1; j < 6; j++) {
                                scoreArray[end++] = 0;
                            }
                        }
                        continue;
                    }
                    board.set(e0, PieceType::elephant, Side::white);

                    int score2 = traEgtbFileMng->getScore(board, side); assert(score2 <= EGTB_SCORE_MATE);

                    scoreArray[end++] = EgtbFileWritting::scoreToCell(score2, ver2);
                    if (score2 != EgtbFileWritting::cellToScore(scoreArray[end - 1], ver2)) {
                        board.show();
                        std::cerr << "Error: EgtbNewGenFileMng::createLookupTables, score ranges problem, idx: " << idx << ", sd: " << sd << ", traScore: " << score2 << ", scoreArray[end - 1]: " << scoreArray[end - 1] << std::endl;
                        assert(false);
                        exit(-1);
                    }

                    if (score != score2) {
                        if (score == EGTB_SCORE_DRAW || score2 == EGTB_SCORE_DRAW) {
                            score = EGTB_SCORE_UNKNOWN;
                            //board.setEmpty(e0, PieceType::elephant, Side::white);
                            //break;
                        }

                        if (score != EGTB_SCORE_UNKNOWN) {
                            score = EGTB_SCORE_WINNING;
                        }
                    }

                    // second elephant
                    if (noMidElephant) {
                        for (int j = i + 1; j < 6; j++) {
                            int e1 = EgtbBoard::flip(blackElephantPosWOMiddle[j], FlipMode::vertical); // epos[j];
                            if (!board.isEmpty(e1)) {
                                scoreArray[end++] = 0;
                                continue;
                            }
                            board.set(e1, PieceType::elephant, Side::white);

                            int score2 = traEgtbFileMng->getScore(board, side); assert(score2 <= EGTB_SCORE_MATE);
                            board.setEmpty(e1, PieceType::elephant, Side::white);

                            scoreArray[end++] = EgtbFileWritting::scoreToCell(score2, ver2);
                            if (score2 != EgtbFileWritting::cellToScore(scoreArray[end - 1], ver2)) {
                                board.show();
                                std::cerr << "Error: EgtbNewGenFileMng::createLookupTables, score ranges problem, idx: " << idx << ", sd: " << sd << ", traScore: " << score2 << ", scoreArray[end - 1]: " << scoreArray[end - 1] << std::endl;
                                assert(false);
                                exit(-1);
                            }

                            if (score2 != EGTB_SCORE_ILLEGAL && score != score2) {
                                if (score == EGTB_SCORE_DRAW || score2 == EGTB_SCORE_DRAW) {
                                    score = EGTB_SCORE_UNKNOWN;
                                    //i = 6; // stop continueuing
                                } else if (score != EGTB_SCORE_UNKNOWN) {
                                    score = EGTB_SCORE_WINNING;
                                    // score = MIN(score, score2);
                                }
                            }
                        }
                    }
                    board.setEmpty(e0, PieceType::elephant, Side::white);
                }
            }
            assert(score == EGTB_SCORE_UNKNOWN || score == EGTB_SCORE_WINNING || score <= EGTB_SCORE_MATE);

            if (score == EGTB_SCORE_WINNING) {
                winningCnt++;
            } else if (score == EGTB_SCORE_UNKNOWN) {
                undeterminedCnt++;
            } else {
                normalCnt++;
            }

            if (score == EGTB_SCORE_WINNING || score == EGTB_SCORE_UNKNOWN) {
                assert(end == 7 || end == 22);
                int groupIdx = end < 20 ? 0 : 1;
                i64 key = egtbFile->getKey(board).key;
                tbLookup[sd].add(key, scoreArray, end, groupIdx);
            }

            egtbFile->setBufScore(idx, score, sd);
        }
    }

    return true;
}


//bool workingOK = true;

bool EgtbNewGenFileMng::genSingleWinningEgtb(EgtbDb* traEgtbFileMng, const std::string& folder, const std::string& name, bool permutate) {
    egtbFile = new EgtbFileWritting();

    int order = 0;
//    for(int i = 0; nameAndGoodOrder[i].name; ++i) {
//        if (strcmp(name.c_str(), nameAndGoodOrder[i].name) == 0) {
//            order = nameAndGoodOrder[i].order;
//            break;
//        }
//    }

    auto egtbType = EgtbType::newdtm;
    egtbFile->create(name, egtbType, EgtbProduct::std, order);
    egtbFile->createBuffersForGenerating();

    // Main loop
    if (egtbVerbose) {
	    std::cout << "Start generating new-format " << name << ", sz: " << Lib::formatString(egtbFile->getSize()) << ", order: " << order << std::endl;
    }

    tbLookup[0].init(name, 1024LL * 1024 * 512 * 2);
    tbLookup[1].init(name, 1024LL * 1024 * 512 * 2);
    
    totalIdxCnt += egtbFile->getSize() * 2;

    setupThreadRecords(egtbFile->getSize());
    
    {
        std::vector<std::thread> threadVec;
        for (int i = 1; i < threadRecordVec.size(); ++i) {
            threadVec.push_back(std::thread(&EgtbNewGenFileMng::genSingleWinningEgtb_loop, this, i));
        }
        
        genSingleWinningEgtb_loop(0);
        
        for (auto && t : threadVec) {
            t.join();
        }
    }

    if (egtbVerbose) {
        std::cout << "\tsaving...\n";
    }
    
    if (order == 0 && permutate) {
        order = testSizePermutation();
    }
    
    if (order) {
        auto orgEgtbFile = egtbFile;
        egtbFile = new EgtbFileWritting();
        egtbFile->create(name, EgtbType::newdtm, EgtbProduct::std, order);
        egtbFile->createBuffersForGenerating();
        egtbFile->convertPermutations(orgEgtbFile, false);
        
        convertLookupForNewOrder(orgEgtbFile, egtbFile);

        delete orgEgtbFile;
    }

    if (!egtbFile->saveFile(folder, EgtbProduct::std, CompressMode::compress)) {
        std::cerr << "Error: save UNSUCCESSFULLY for " << name << std::endl;
        return false;
    }

    if (egtbVerbose) {
        std::cout << "\tsaving lookup...\n";
    }

    auto luFolder = folder + "/lu";
    Lib::createFolder(luFolder);
    for(int sd = 0; sd < 2; sd++) {
        tbLookup[sd].sort();
        tbLookup[sd].save(luFolder, sd, CompressMode::compress);
    }

    delete egtbFile;

    std::cout << "Generated successfully for " << name << std::endl;
    return true;
}


void EgtbNewGenFileMng::genWinningEgtbs(std::string traFolder, std::string folder, const std::string& name, bool includeSubs, bool permutate)
{
    std::cout << "genWinningEgtbs BEGIN" << std::endl;
    
    NameRecord record(name);
    if (!record.isValid() || record.type != EgtbType::newdtm) {
        std::cerr << "Error: wrong name: " << name << std::endl;
        return;
    }

    traEgtbFileMng = new EgtbDb();
    traEgtbFileMng->preload(traFolder, EgtbMemMode::all, EgtbLoadMode::onrequest);
    
    Lib::createFolder(folder);
    folder += "/";

    std::vector<std::string> vec = parseName(name, includeSubs);

    for(auto && aName : vec) {
        NameRecord record2(aName);
        assert(record2.isValid());
        
        if (!record.isSameAttackers(record2)) {
            continue;
        }

        SubfolderParser subfolder(aName);
        subfolder.createAllSubfolders(folder);

        std::string wfolder = folder + "/" + subfolder.subfolder;

        auto fullpath = EgtbFileWritting::createFileName(wfolder, aName, EgtbType::newdtm, Side::white, CompressMode::compress);
        if (Lib::existFile(fullpath.c_str())) {
            continue;
        }

        genSingleWinningEgtb(traEgtbFileMng, wfolder, aName, permutate);

        removeAllBuffers();
        traEgtbFileMng->removeAllBuffers();
    }

    delete traEgtbFileMng;
    
//    std::cout << "EgtbNewGenFileMng::genWinningEgtbs totalIdxCnt: " << totalIdxCnt
//    << ": " << normalCnt << " " << (double)normalCnt / totalIdxCnt << "% "
//    << ": " << winningCnt << " " << (double)winningCnt / totalIdxCnt << "% "
//    << ": " << undeterminedCnt << " " << (double)undeterminedCnt / totalIdxCnt << "% "
//    << std::endl;
    std::cout << "genWinningEgtbs COMPLETED!" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EgtbNewGenFileMng::testSizePermutation() {
    assert(egtbFile);
    auto idxSz = egtbFile->getEgtbIdxArraySize();
    const int *orders = createOrders(idxSz);
    
    i64 bestSize = egtbFile->getSize();
    int bestOrder = 0;
    
    for(int i = 0; orders[i] >= 0; i++) {
        int order = orders[i];
        auto egtbFile2 = new EgtbFileWritting();
        egtbFile2->create(egtbFile->getName(), egtbFile->getEgtbType(), EgtbProduct::std, order);
        egtbFile2->createBuffersForGenerating();
        egtbFile2->convertPermutations(egtbFile, true);
        auto testsz = egtbFile2->threadRecordVec.at(0).cnt;
        std::cout << "testsz: " << testsz << std::endl;
        assert(testsz > 0 && testsz <= egtbFile2->getSize());
        if (testsz < bestSize) {
            bestSize = testsz;
            bestOrder = order;
        }
        
        std::cout << "testSizePermutation i: " << i << ", order: " << order << ", testsz: " << testsz << ", bestOrder: " << bestOrder << std::endl;
        delete egtbFile2;
    }
    return bestOrder;
}

void EgtbNewGenFileMng::convertLookupForNewOrder(EgtbFileWritting* orgEgtbFile, EgtbFileWritting* newEgtbFile)
{
    for(int sd = 0; sd < 2; sd++) {
        tbLookup[sd].sort();
        int groupNum = 2;
        for (int grp = 0; grp < groupNum; grp++) {
            int n = tbLookup[sd].itemCnt[grp];
            if (n == 0) {
                continue;
            }
            
            auto p = tbLookup[sd].getDataPointer(grp);
            auto itemSizeInByte = 4 + EgtbLookup::luGroupSizes[grp];
            for(int i = 0; i < n; i++) {
                auto key = (u32 *)(p);
                u32 newKey = (u32)newEgtbFile->convertPermutations_idx(*key, orgEgtbFile);
                *key = newKey;
                p += itemSizeInByte;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EgtbNewGenFileMng::loadLookupTables()
{
    assert(false);
//    TbLookupMng tbLookupMng;
//    tbLookupMng.load(EgtbFolderBase + "egtb/new-wining/lookup");
//    tbLookupMng.printOut("\n\ntbLookupMng:");
}

void EgtbNewGenFileMng::createLookupTables()
{
//    const std::string winningFolder = EgtbFolderBase + "egtb/winning/unzip/2/hp";
//    const std::string traditionalFolder = EgtbFolderBase + "egtb/dtm/2/hp";
//    const std::string lookupFolder = EgtbFolderBase + "egtb/winning/lookup"; //  + "egtb/permutation/lookup";
//
//    createLookupTables(traditionalFolder, winningFolder, lookupFolder);
}


void EgtbNewGenFileMng::createLookupTables(const std::string& traditionalFolder, const std::string& winningFolder, const std::string& _lookupFolder, const int* orders)
{
    std::cout << "EgtbNewGenFileMng::createLookupTables BEGIN, lookupFolder: " << _lookupFolder << std::endl;

//    std::string folder = EgtbFolderBase + "egtb/permutation/1a"; // + "egtb/winning/1";
    preload(winningFolder, EgtbMemMode::all);

//    std::string tradFolder = EgtbFolderBase + "egtb/dtm/1";
    EgtbDb traEgtbFileMng;
    traEgtbFileMng.preload(traditionalFolder, EgtbMemMode::all);

//    printf("egtbFileVec sz=%lld, winningFolder: %s\n", egtbFileVec.size(), winningFolder.c_str());
    std::cout << "traEgtbFileMng egtbFileVec sz: " << traEgtbFileMng.getEgtbFileVec().size() << ", traditionalFolder: " << traditionalFolder << std::endl;

    for (auto && egtbFile : egtbFileVec) {
        if (egtbFile->isCompressed()) {
            std::cout << "WARNING: winning files should not be compressed in optimized mode (losing INVALID values). Uncompressed files DETECTED!";
            break;
        }
    }

    for (auto && egtbFile : egtbFileVec) {
        auto theName = egtbFile->getName();

        std::cout << theName << std::endl;
        if (theName != "khpamk") {
            continue;
        }
        auto r = std::count(theName.begin(), theName.end(), 'r');
        auto c = std::count(theName.begin(), theName.end(), 'c');
        auto h = std::count(theName.begin(), theName.end(), 'h');
        auto p = std::count(theName.begin(), theName.end(), 'p');
//        auto n = r + c + h + p;
//        if (n != 2) {
//            continue;
//        }

        std::string wFolder = _lookupFolder + "/";
        if (r) {
            wFolder += "r";
        }
        if (c) {
            wFolder += c == 1 ? "c" : "cc";
        }
        if (h) {
            wFolder += h == 1 ? "h" : "hh";
        }
        if (p) {
            wFolder += p == 1 ? "p" : p == 2 ? "pp" : "ppp";
        }

        Lib::createFolder(wFolder);

//        if (Lib::existFile(EgtbLookupWritting::getFileName(wFolder, theName, W, true).c_str()) ||
//            Lib::existFile(EgtbLookupWritting::getFileName(wFolder, theName, W, false).c_str()) ||
//            Lib::existFile(EgtbLookupWritting::getFileName(wFolder, theName, B, true).c_str()) ||
//            Lib::existFile(EgtbLookupWritting::getFileName(wFolder, theName, B, false).c_str())
//            ) {
//            continue;
//        }

        auto compressMode = CompressMode::compress;
        createLookupTables(traEgtbFileMng, egtbFile, wFolder, compressMode, orders);
    }

}

static bool createLookupTablesOK;

void call_createLookupTables(int threadIdx, EgtbNewGenFileMng* egtbNewGenFileMng, EgtbLookupWritting* tbLookup, EgtbDb* traEgtbFileMng, EgtbFile* egtbFile, i64 fromIdx, i64 toIdx, int sd, const int* orders) {

    if (!egtbNewGenFileMng->createLookupTables(threadIdx, *tbLookup, *traEgtbFileMng, egtbFile, fromIdx, toIdx, sd, orders)) {
        createLookupTablesOK = false;
    }
}

void EgtbNewGenFileMng::createLookupTables(EgtbDb& traEgtbFileMng, EgtbFile* egtbFile, std::string& saveFolder, CompressMode compressMode, const int* orders)
{
    auto theName = egtbFile->getName();
    std::cout << "Creating lookup for " << theName << std::endl;

    for(int sd = 0; sd < 2; sd++) {
//        Side side = static_cast<Side>(sd);
        EgtbLookupWritting tbLookup(theName, 1024 * 1024 * 1024);
        egtbFile->checkToLoadHeaderAndTable(Side::none);
        tbLookup.order = egtbFile->getHeader()->getOrder();
//        assert(tbLookup.order);
        assert(tbLookup.itemCnt[0] == 0 && tbLookup.itemCnt[1] == 0);


        auto theSize = egtbFile->getSize();
        int num_threads = theSize > 64 * 1024L ? 6 : 0;

        createLookupTablesOK = true;

        if (num_threads > 0) {
            std::vector<std::thread> threadVec;
            i64 blockSize = theSize / (num_threads + 1);
            for (int i = 0; i < num_threads; ++i) {
                i64 fromIdx = blockSize * (i + 1);
                i64 toIdx = i+1 == num_threads ? theSize : (fromIdx + blockSize);
//                threadVec.push_back(std::thread(call_createLookupTables, i + 1, this, tbLookup, traEgtbFileMng, egtbFile, fromIdx, toIdx, sd, orders));
                threadVec.push_back(std::thread(call_createLookupTables, i + 1, this, &tbLookup, &traEgtbFileMng, egtbFile, fromIdx, toIdx, sd, orders));
            }

            if (!createLookupTables(0, tbLookup, traEgtbFileMng, egtbFile, 0, blockSize, sd, orders)) {
                createLookupTablesOK = false;
            }

            for (auto && t : threadVec) {
                t.join();
            }
            threadVec.clear();
        } else {
            createLookupTablesOK = createLookupTables(0, tbLookup, traEgtbFileMng, egtbFile, 0, theSize, sd, orders);
        }

        if (createLookupTablesOK) {
            tbLookup.sort();
            tbLookup.save(saveFolder, sd, compressMode);
        }
    }

    removeAllBuffers();
    traEgtbFileMng.removeAllBuffers();

    std::cout << "DONE lookup for " << theName << std::endl;

}

bool EgtbNewGenFileMng::createLookupTables(int threadIdx, EgtbLookupWritting& tbLookup, EgtbDb& traEgtbFileMng, EgtbFile* egtbFile, i64 fromIdx, i64 toIdx, int sd, const int* elephantorders)
{
    {
        std::lock_guard<std::mutex> thelock(printMutex);
        std::cout << "createLookupTables threadIdx: " << threadIdx << ", range: " << fromIdx << " - " << toIdx << std::endl;
    }

    ExtBoard board;
    bool ok = true;
    i64 nodeCnt = 0;
    auto theName = egtbFile->getName();

    Side side = static_cast<Side>(sd), xside = getXSide(side);

    auto ver2 = tbLookup.isVersion2(); assert(ver2);

    for(i64 idx = fromIdx; idx < toIdx; idx++) {
//        if (idx == 1227621) {
//            std::cout << "Iamhere\n";
//        } else {
//            continue;
//        }
        int rawScore = egtbFile->getScore(idx, side);
        assert(rawScore != EGTB_SCORE_MISSING);
        if (rawScore != EGTB_SCORE_WINNING && rawScore != EGTB_SCORE_UNKNOWN) {
            continue;
        }

        if (!setup(egtbFile, board, idx, FlipMode::none, Side::white) || board.isIncheck(xside)) {
            board.printOut("Error: board invalid");
            assert(false);
            exit(-1);
        }

//        if (idx == 1227621) {
//            board.printOut("idx == 1227621");
//        }

        nodeCnt++;
        int scoreArray[32], end = 0;

        i64 key = egtbFile->getKey(board).key;

        // Check key, subKey
//#ifdef DEBUG
//        EgtbKeyRec rec;
//        EgtbKey::getKey(rec, (const int *)board.pieceList, true, egtbFile->egtbIdxArray);
//        assert(rec.key == key && rec.subKey == end);
//#endif

//        scoreArray[end] = EgtbFileWritting::scoreToCell(traEgtbFileMng.getScore(board, side), ver2); end++;
        auto traScore = traEgtbFileMng.getScore(board, side);
        scoreArray[end] = EgtbFileWritting::scoreToCell(traScore, ver2); end++;
        if (traScore != EgtbFileWritting::cellToScore(scoreArray[end - 1], ver2)) {
            board.printOut("Error: score range");
            std::cerr << "Error: EgtbNewGenFileMng::createLookupTables, score ranges problem, idx: " << idx << ", sd: " << sd << ", traScore: " << traScore << ", scoreArray[end - 1]: " << scoreArray[end - 1] << std::endl;
            assert(false);
            exit(-1);
        }

        bool noMidElephant = board.getPiece(67).type != PieceType::elephant;
        // first elephants
        for (int i = 0; i < 6; i++) {
            int e0 = EgtbBoard::flip(blackElephantPosWOMiddle[i], FlipMode::vertical);
            assert(e0 > 0);
            if (!board.isEmpty(e0)) {
                scoreArray[end++] = 0;
                if (noMidElephant) {
                    for (int j = i + 1; j < 6; j++) {
                        scoreArray[end++] = 0;
                    }
                }
                continue;
            }
            board.set(e0, PieceType::elephant, Side::white);

//#ifdef DEBUG
//            EgtbKeyRec rec;
//            EgtbKey::getKey(rec, (const int *)board.pieceList, true, egtbFile->egtbIdxArray);
//
//            assert(rec.key == key && rec.subKey == end);
//#endif

            // Check with first added elephant
            auto traScore = traEgtbFileMng.getScore(board, side);
            scoreArray[end] = EgtbFileWritting::scoreToCell(traScore, ver2); end++;
            if (traScore != EgtbFileWritting::cellToScore(scoreArray[end - 1], ver2)) {
                board.printOut("Error: score range");
                std::cerr << "Error: EgtbNewGenFileMng::createLookupTables, score ranges problem, idx: " << idx << ", sd: " << sd << ", traScore: " << traScore << ", scoreArray[end - 1]: " << scoreArray[end - 1] << std::endl;
                assert(false);
                exit(-1);
            }

            // second elephant
            if (noMidElephant) {
                for (int j = i + 1; j < 6; j++) {
                    auto e1 = EgtbBoard::flip(blackElephantPosWOMiddle[j], FlipMode::vertical);
                    assert(e1 > 44);

                    if (!board.isEmpty(e1)) {
                        scoreArray[end++] = 0;
                        continue;
                    }
                    board.set(e1, PieceType::elephant, Side::white);

                    //board.printOut();

//#ifdef DEBUG
//                    EgtbKey::getKey(rec, (const int *)board.pieceList, true, egtbFile->egtbIdxArray);
//                    assert(rec.key == key && rec.subKey == end);
//#endif

                    // Check with second added elephant
//                    scoreArray[end] = EgtbFileWritting::scoreToCell(traEgtbFileMng.getScore(board, side), ver2); end++;
                    auto traScore = traEgtbFileMng.getScore(board, side);
                    scoreArray[end] = EgtbFileWritting::scoreToCell(traScore, ver2); end++;
//                    assert(traScore == EgtbFile::cellToScore(scoreArray[end - 1], ver2));
                    if (traScore != EgtbFileWritting::cellToScore(scoreArray[end - 1], ver2)) {
                        board.printOut("Error: score range");
                        std::cerr << "Error: EgtbNewGenFileMng::createLookupTables, score ranges problem, idx: " << idx << ", sd: " << sd << ", traScore: " << traScore << ", scoreArray[end - 1]: " << scoreArray[end - 1] << std::endl;
                        assert(false);
                        exit(-1);
                    }

                    board.setEmpty(e1, PieceType::elephant, Side::white);
                };
            }
            board.setEmpty(e0, PieceType::elephant, Side::white);
        }

        assert(end == 7 || end == 22);
        int groupIdx = end < 20 ? 0 : 1;

//        for(int i = 0; i < end; i++) {
//            std::cout << " " << i << ") " << scoreArray[i];
//        }
//        std::cout << "\n";

        if (elephantorders) { // NOT purmutation order, but order of elephants
            int scoreArray2[32];
            memset(scoreArray2, 0xff, sizeof(scoreArray2));

            const int* p = elephantorders + (groupIdx == 0 ? 0 : EgtbLookup::luGroupSizes[0]);
            for(int i = 0; i < end; i++) {
                int k = p[i];
                assert(k >= 0 && k < EgtbLookup::luGroupSizes[groupIdx]);
                assert(scoreArray2[i] == -1);
                scoreArray2[i] = scoreArray[k];
            }
            ok = tbLookup.add(key, scoreArray2, end, groupIdx);
        } else {
            ok = tbLookup.add(key, scoreArray, end, groupIdx);
        }

        if (!ok) {
            break;
        }
    }

//    printf("nodeCnt=%lld\n", nodeCnt);
    std::cout << "createLookupTables DONE threadIdx: " << threadIdx << std::endl;
    return ok;
}

//void EgtbNewGenFileMng::testLookupTables()
//{
//    TbLookupMng tbLookupMng;
//    tbLookupMng.preload(EgtbFolderBase + "egtb/winning/lookup", EgtbMemMode::all);
//    //tbLookupMng.printOut("\n\ntbLookupMng:");
//
////    addFolders(EgtbFolderBase + "egtb/winning/lookup");
//    std::string folder = EgtbFolderBase + "egtb/winning/1";
//    preload(folder, EgtbMemMode::all);
//
//    std::string tradFolder = EgtbFolderBase + "egtb/dtm/1";
//    EgtbDb traEgtbFileMng;
//    traEgtbFileMng.preload(tradFolder, EgtbMemMode::all);
//
////    printf("egtbFileVec sz=%ld, folder: %s\n", egtbFileVec.size(), folder.c_str());
////    printOut();
//
//    EgtbBoard board;
//
//    for (auto && egtbFile : egtbFileVec) {
//        printf("Start checking %s\n", egtbFile->getName().c_str());
//        for(int sd = 0; sd < 2; sd++) {
//            Side side = static_cast<Side>(sd), xside = getXSide(side);
//            for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
//                int rawScore = egtbFile->getScore(idx, side);
//                assert(rawScore != EGTB_SCORE_MISSING);
//                if (rawScore != EGTB_SCORE_WINNING && rawScore != EGTB_SCORE_UNKNOWN) {
//                    continue;
//                }
//
////                auto b = egtbFile->setupPieceList((int *)board.pieceList, idx, EGTB_FLIPBOARD_NONE, Side::white) && board.pieceList_setupBoard();
//                if (!egtbFile->setup(board, idx, FlipMode::none, Side::white) || board.isIncheck(xside)) {
//                    assert(false);
//                    continue;
//                }
//
//                // Check origin board
//                //board.printOut();
//                int score0 = tbLookupMng.lookup(egtbFile->getName(), (const int *)board.pieceList, side, egtbFile->idxArr, egtbFile->idxMult, egtbFile->header->order);
//                int score00 = traEgtbFileMng.getScore((const int *)board.pieceList, side);
//                assert(score0 <= EGTB_SCORE_MATE && (rawScore == EGTB_SCORE_UNKNOWN || score0 != 0));
//                assert(score0 == score00);
//
////                const int epos[] = { 47, 51, 63, 71, 83, 87 }; // no middle elephant position 67
//
//                bool noMidElephant = board.getPiece(67).type != PieceType::elephant;
//                // first elephants
//                for (int i = 0; i < 6; i++) {
//                    //int e0 = whiteElephantPosWOMiddle[i];
//                    int e0 = EgtbBoard::flip(blackElephantPosWOMiddle[i], FlipMode::vertical);
//                    if (!board.isEmpty(e0)) {
//                        continue;
//                    }
//                    board.set(e0, PieceType::elephant, Side::white);
////                    board.printOut();
//
//                    // Check with first added elephant
//                    int score11 = traEgtbFileMng.getScore((const int *)board.pieceList, side);
//                    int score1 = tbLookupMng.lookup(egtbFile->getName(), (const int *)board.pieceList, side, egtbFile->idxArr, egtbFile->idxMult, egtbFile->header->order);
//                    assert(score1 <= EGTB_SCORE_MATE && (rawScore == EGTB_SCORE_UNKNOWN || score1 != 0));
//                    assert(score1 == score11);
//
//                    // second elephant
//                    if (noMidElephant) {
//                        for (int j = i + 1; j < 6; j++) {
//                            //int e1 = whiteElephantPosWOMiddle[j];
//                            int e1 = EgtbBoard::flip(blackElephantPosWOMiddle[i], FlipMode::vertical);
//                            if (!board.isEmpty(e1)) {
//                                continue;
//                            }
//                            board.set(e1, PieceType::elephant, Side::white);
//                            // Check with second added elephant
////                            board.printOut();
//                            int score2 = tbLookupMng.lookup(egtbFile->getName(), (const int *)board.pieceList, side, egtbFile->idxArr, egtbFile->idxMult, egtbFile->header->order);
//                            int score22 = traEgtbFileMng.getScore((const int *)board.pieceList, side);
//                            assert(score2 <= EGTB_SCORE_MATE && (rawScore == EGTB_SCORE_UNKNOWN || score2 != 0));
//                            assert(score2 == score22);
//
//                            board.setEmpty(e1, PieceType::elephant, Side::white);
//                        }
//                    }
//                    board.setEmpty(e0, PieceType::elephant, Side::white);
//                }
//            }
//        }
//    }
//}


//const NameAndOrder nameAndGoodOrder[] = {
//    {"kcaamkaae", 66},
//    {"kcaamkaaee", 66},
//    {"kcaamkaa", 17},
//    {"kcaamka", 136},
//    {"kcaamkae", 10},
//    {"kcaamkaee", 129},
//    {"kcaamk", 136},
//    {"kcaamke", 136},
//    {"kcaamkee", 129},
//    {"kcamkaae", 66},
//    {"kcamkaaee", 66},
//    {"kcamkaa", 10},
//    {"kcamka", 136},
//    {"kcamkae", 66},
//    {"kcamkaee", 129},
//    {"kcamk", 129},
//    {"kcamke", 129},
//    {"kcamkee", 129},
//    {"kcmkaa", 10},
//    {"kcmkaae", 66},
//    {"kcmkaaee", 66},
//    {"kcmka", 17},
//    {"kcmkae", 129},
//    {"kcmkaee", 129},
//    {"kcmk", 136},
//    {"kcmke", 136},
//    {"kcmkee", 136},
//    {"khaamkaa", 66},
//    {"khaamkaae", 66},
//    {"khaamkaaee", 66},
//    {"khaamka", 129},
//    {"khaamkae", 66},
//    {"khaamkaee", 129},
//    {"khaamke", 17},
//    {"khaamkee", 129},
//    {"khaamk", 17},
//    {"khamkaa", 66},
//    {"khamkaae", 66},
//    {"khamkaaee", 66},
//    {"khamka", 129},
//    {"khamkae", 66},
//    {"khamkaee", 66},
//    {"khamke", 17},
//    {"khamkee", 136},
//    {"khamk", 17},
//    {"khmkaa", 66},
//    {"khmkaae", 66},
//    {"khmkaaee", 66},
//    {"khmka", 129},
//    {"khmkae", 66},
//    {"khmkaee", 66},
//    {"khmk", 136},
//    {"khmke", 17},
//    {"khmkee", 136},
//    {"kpaamkaa", 66},
//    {"kpaamkaae", 66},
//    {"kpaamkaaee", 66},
//    {"kpaamka", 129},
//    {"kpaamkae", 66},
//    {"kpaamkaee", 136},
//    {"kpaamke", 66},
//    {"kpaamkee", 136},
//    {"kpaamk", 17},
//    {"kpamkaa", 10},
//    {"kpamkaae", 66},
//    {"kpamkaaee", 66},
//    {"kpamka", 129},
//    {"kpamkae", 66},
//    {"kpamkaee", 136},
//    {"kpamke", 66},
//    {"kpamkee", 136},
//    {"kpamk", 17},
//    {"kpmkaa", 66},
//    {"kpmkaae", 66},
//    {"kpmkaaee", 66},
//    {"kpmka", 129},
//    {"kpmkae", 66},
//    {"kpmkaee", 136},
//    {"kpmk", 17},
//    {"kpmke", 66},
//    {"kpmkee", 136},
//    {"kraamkaaee", 10},
//    {"kraamkaae", 17},
//    {"kraamkaa", 17},
//    {"kraamka", 129},
//    {"kraamkaee", 17},
//    {"kraamkae", 17},
//    {"kraamkee", 17},
//    {"kraamke", 129},
//    {"kraamk", 10},
//    {"kramkaaee", 17},
//    {"kramkaae", 17},
//    {"kramkaa", 17},
//    {"kramkaee", 17},
//    {"kramkae", 17},
//    {"kramka", 17},
//    {"kramkee", 17},
//    {"kramke", 129},
//    {"kramk", 136},
//    {"krmkaaee", 10},
//    {"krmkaae", 17},
//    {"krmkaa", 17},
//    {"krmkaee", 17},
//    {"krmkae", 17},
//    {"krmka", 17},
//    {"krmkee", 17},
//    {"krmke", 17},
//    {"krmk", 136},
//    {"khhaamkaa", 129},
//    {"khhaamkaaee", 10},
//    {"khhaamkaae", 10},
//    {"khhaamka", 129},
//    {"khhaamkae", 129},
//    {"khhaamkaee", 66},
//    {"khhaamke", 129},
//    {"khhaamkee", 129},
//    {"khhaamk", 10},
//    {"khhamkaa", 129},
//    {"khhamkaae", 66},
//    {"khhamkaaee", 10},
//    {"khhamka", 129},
//    {"khhamkae", 129},
//    {"khhamkaee", 66},
//    {"khhamke", 129},
//    {"khhamkee", 129},
//    {"khhamk", 10},
//    {"khhmkaa", 129},
//    {"khhmkaae", 66},
//    {"khhmkaaee", 66},
//    {"khhmka", 129},
//    {"khhmkae", 129},
//    {"khhmkaee", 66},
//    {"khhmk", 129},
//    {"khhmke", 129},
//    {"khhmkee", 129},
//    {"khpaamkaa", 1546},
//    {"khpaamkaae", 90},
//    {"khpaamkaaee", 90},
//    {"khpaamka", 1602},
//    {"khpaamkaee", 1035},
//    {"khpaamkae", 90},
//    {"khpaamke", 1602},
//    {"khpaamkee", 1546},
//    {"khpaamk", 202},
//    {"khpamkaa", 1546},
//    {"khpamkaae", 90},
//    {"khpamkaaee", 90},
//    {"khpamka", 1546},
//    {"khpamkaee", 1035},
//    {"khpamkae", 90},
//    {"khpamke", 1602},
//    {"khpamkee", 1546},
//    {"khpamk", 202},
//    {"khpmkaa", 1546},
//    {"khpmkaae", 1091},
//    {"khpmkaaee", 1035},
//    {"khpmka", 1546},
//    {"khpmkae", 1602},
//    {"khpmkaee", 1035},
//    {"khpmke", 1546},
//    {"khpmkee", 1546},
//    {"khpmk", 202},
//    {"kppaamkaa", 129},
//    {"kppaamkaae", 66},
//    {"kppaamkaaee", 66},
//    {"kppaamka", 129},
//    {"kppaamkae", 66},
//    {"kppaamkaee", 66},
//    {"kppaamke", 129},
//    {"kppaamkee", 129},
//    {"kppaamk", 10},
//    {"kppamkaa", 129},
//    {"kppamkaae", 66},
//    {"kppamkaaee", 66},
//    {"kppamka", 129},
//    {"kppamkae", 66},
//    {"kppamkaee", 66},
//    {"kppamke", 129},
//    {"kppamkee", 129},
//    {"kppamk", 10},
//    {"kppmkaa", 129},
//    {"kppmkaae", 66},
//    {"kppmkaaee", 66},
//    {"kppmka", 129},
//    {"kppmkae", 66},
//    {"kppmkaee", 66},
//    {"kppmke", 129},
//    {"kppmkee", 129},
//    {"kppmk", 10},
//    {"kpppmkaae", 66},
//    {"kpppmkaaee", 66},
//    {"kpppmkaa", 129},
//    {"kpppmkae", 66},
//    {"kpppmkaee", 66},
//    {"kpppmka", 129},
//    {"kpppmkee", 129},
//    {"kpppmke", 129},
//    {"kpppmk", 10},
//    { nullptr, -1}
//};
