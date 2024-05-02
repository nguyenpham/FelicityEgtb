//
//  EgtbNewGenFileMng3.cpp
//
//  Created by TonyPham on 14/4/17.
//

#include <algorithm>

#include "EgtbNewGenFileMng.h"
#include "CompressLib.h"

using namespace egtb;


const int PermutationOrderSize = 3;

//const int chunkSizes[] = {512, 1024, 2 * 1024, 4 * 1024, 8 * 1024, 16 * 1024, 32 * 1024, 2 * 1024 * 1024, 4 * 1024 * 1024 , 8 * 1024 * 1024  };


void EgtbNewGenFileMng::convert() {
    std::cout << "EgtbNewGenFileMng::convert BEGIN\n";

//    std::string folder =  EgtbFolderBase + "egtb/winning/1";
//    openEgtbFiles(folder, EgtbMemMode::all);
//
////    std::string newfolder = EgtbFolderBase + "egtb/fit/r";
//
////    const char * aName = "KRAMKAAE";
////    auto egtbFile0 = nameMap[aName];
////    egtbFile0->convert(newfolder);
//
//    for(int i = 0; i < sizeof(chunkSizes) / sizeof(int); i++) {
//        int chunkSize = chunkSizes[i];
//        i64 totalOriginalSize = 0;
////        int ignoreCnt = 0;
////        i64 ignoreSz = 0;
////        i64 totalNewSize = 0;
//
//        i64 zipSizes[] = { 0, 0 }, zipMinSz = 0;
//
//        for (auto && egtbFile : egtbFileVec) {
//            //        printf("Start checking %s, sz: %lld\n", egtbFile->getName().c_str(), egtbFile->getSize());
//            totalOriginalSize += egtbFile->getSize();
//            //        auto sz = egtbFile->convert(newfolder);
//            //        if (sz == 0) {
//            //            ignoreCnt++;
//            //            ignoreSz += egtbFile->getSize();
//            //            continue;
//            //        }
//            //        totalNewSize += sz;
//
//            auto szPair = CompressLib::compressEgtb(egtbFile, chunkSize);
//            zipSizes[0] += szPair.first;
//            zipSizes[1] += szPair.second;
//            zipMinSz += MIN(szPair.first, szPair.second);
//        }
//
////        std::cout << "ignoreCnt: " << ignoreCnt << " of " << egtbFileVec.size() << ", ignoreSz: " << ignoreSz << " (" << ignoreSz * 100 / totalOriginalSize << "%)" << std::endl;
//
//        auto zipTotal = zipSizes[0] + zipSizes[1];
//        std::cout << "chunkSize: " << chunkSize << ", zipMinSz: " << zipMinSz << " (" << zipMinSz * 100 / totalOriginalSize << "%)" << ", zipSizes = " << zipSizes[0] << ", " << zipSizes[1] << ", zip total: " << Lib::formatString(zipTotal) << " (" << zipTotal * 100 / (2 * totalOriginalSize) << "%) of " << Lib::formatString(totalOriginalSize) <<  std::endl;
//        
////        std::cout << "totalOriginalSize: " << totalOriginalSize << ", totalNewSize: " << totalNewSize << " (" << totalNewSize * 100 / totalOriginalSize << "%)\n\n";
//    }

    std::cout << "EgtbNewGenFileMng::convert END\n";
}


void EgtbNewGenFileMng::convertVersion() {
//    std::cout << "EgtbNewGenFileMng::convertVersion BEGIN\n";
//
//    std::string folder = EgtbFolderBase + "egtb/dtm/1old";
//    preload(folder, EgtbMemMode::all);
//
//    const CompressMode compressMode = CompressMode::compress_none;
//
//    for (auto && egtbFile : egtbFileVec) {
//
//        auto theName = egtbFile->getName();
//        auto r = std::count(theName.begin(), theName.end(), 'r');
//        auto c = std::count(theName.begin(), theName.end(), 'c');
//        auto h = std::count(theName.begin(), theName.end(), 'h');
//        auto p = std::count(theName.begin(), theName.end(), 'p');
//        auto n = (int)(r + c + h + p);
//        if (n != 1) {
//            continue;
//        }
//
//        std::cout << theName << std::endl;
//        std::string wFolder = EgtbFolderBase + "egtb/dtm/" + Lib::itoa(n) + "/"; // (n > 1 ? "egtb/dtm/2/" : "egtb/dtm/1/");
//        Lib::createFolder(wFolder);
//
//        if (r) {
//            wFolder += "r";
//        }
//        if (c) {
//            wFolder += c == 1 ? "c" : "cc";
//        }
//        if (h) {
//            wFolder += h == 1 ? "h" : "hh";
//        }
//        if (p) {
//            wFolder += p == 1 ? "p" : "pp";
//        }
//
//        i64 idx = 0;
//        egtbFile->getScore(idx, Side::white, false);
//        egtbFile->getScore(idx, Side::black, false);
//
//        Lib::createFolder(wFolder);
//
////        egtbFile->saveFile(wFolder, compressed);
//
//        EgtbFileWritting newEgtbFile;
//
//        newEgtbFile.create(egtbFile->getName(), egtbFile->getEgtbType());
//        newEgtbFile.createBuffersForGenerating();
//
//        newEgtbFile.convertScores(egtbFile);
//
//        assert(egtbFile->getSize() == newEgtbFile.getSize());
//
//        newEgtbFile.saveFile(wFolder, compressMode);
//    }
//
//    std::cout << "EgtbNewGenFileMng::convertVersion END\n";
}

extern i64 totalSize, illegalCnt, drawCnt, compressedUndeterminedCnt;

void EgtbNewGenFileMng::compress() {
    std::cout << "EgtbNewGenFileMng::compress BEGIN\n";

    const CompressMode compressMode = CompressMode::compress;

    std::string folder = "/Volumes/Seagate Expansion Drive/Nguyen/egtb/working/ch"; // EgtbFolderBase + "egtb/dtm/2/pp"; //
//    std::string folder = "/Volumes/Seagate Expansion Drive/Nguyen/egtb/zip-uncompressedcontents/hh"; // EgtbFolderBase + "egtb/dtm/2/pp"; //
//    std::string folder = EgtbFolderBase + "egtb/winning/2/hp";
    std::string writtenfolder = "/Volumes/Seagate Expansion Drive/Nguyen/egtb/compressed"; // EgtbFolderBase + "egtb/dtm/zip";
    Lib::createFolder(writtenfolder);
    writtenfolder += "/";

    preload(folder, EgtbMemMode::all);

    std::cout << "egtbFileVec sz = " << egtbFileVec.size() << std::endl;
    int cnt = 0;
    for (auto && p : egtbFileVec) {
        auto egtbFile = (EgtbFileWritting*)p;
        auto theName = egtbFile->getName();
        SubfolderParser parser(theName);
        if (parser.attackingCnt != 2) { // theName == "kchaaeekaaee" || 
            continue;
        }
        cnt++;
//        if (cnt < 57) {
//            continue;
//        }

        auto m = std::count(theName.begin(), theName.end(), 'm');
        assert((m == 0 && egtbFile->getEgtbType() != EgtbType::newdtm) || (m == 1 && egtbFile->getEgtbType() == EgtbType::newdtm));

        parser.createAllSubfolders(writtenfolder);
        std::string wFolder = writtenfolder + parser.subfolder;

        std::cout << "\nCompressing " << theName << std::endl;

//        if (egtbFile->getSubPawnRank() >= 0) {
//            EgtbPawnFiles* egtbPawnFiles = (EgtbPawnFiles*)egtbFile;
//            for(int i = 0; i< 7; i++) {
//                if (egtbPawnFiles->subFiles[i]) {
//                    auto p = (EgtbFileWritting*)egtbPawnFiles->subFiles[i];
//                    if (!EgtbFileWritting::existFileName(wFolder, p->getName(), p->getEgtbType(), Side::none, compressMode != CompressMode::compress_none)) {
//                        // Load data to buf
//                        for(int sd = 0; sd < 2; ++sd) {
////                            i64 idx = 0;
//                            auto side = static_cast<Side>(sd);
//                            if (p->forceLoadHeaderAndTable(side)) {
//                                p->saveFile(wFolder, sd, compressMode);
//                                p->removeBuffers();
//                                if (egtbVerbose) {
//                                    std::cout << "Successfully create " << p->getPath(sd) << std::endl;
//                                }
//                            } else {
//                                std::cerr << "\n******Error: Cannot read " << p->getPath(sd) << std::endl << std::endl << std::endl;
//                            }
////                            p->getScore(idx, Side::black, false);
//                        }
//                    }
//                }
//            }
//        } else {
            if (!EgtbFileWritting::existFileName(wFolder, theName, egtbFile->getEgtbType(), Side::none, compressMode != CompressMode::compress_none)) {
                // Load data to buf
//                i64 idx = 0;
//                egtbFile->getScore(idx, Side::white, false);
//                egtbFile->getScore(idx, Side::black, false);
//                egtbFile->saveFile(wFolder, compressMode);
//                egtbFile->removeBuffers();

                for(int sd = 1; sd < 2; ++sd) {
                    auto side = static_cast<Side>(sd);
                    if (egtbFile->forceLoadHeaderAndTable(side)) {
                        egtbFile->saveFile(wFolder, sd, EgtbProduct::std, compressMode);
                        egtbFile->removeBuffers();
                        if (egtbVerbose) {
                            std::cout << "Successfully create " << egtbFile->getPath(sd) << std::endl;
                        }
                    } else {
                        std::cerr << "\n******Error: Cannot read " << egtbFile->getPath(sd) << std::endl << std::endl << std::endl;
                    }
                }

            }
//        }
    }

    std::cout << "totalSize: " << totalSize
    << ", illegalCnt: " << illegalCnt << " " << (double)illegalCnt * 100 / (totalSize + 1)
    << ", drawCnt: " << drawCnt << " " << (double)drawCnt * 100 / (totalSize + 1)
    << ", compressedUndeterminedCnt: " << compressedUndeterminedCnt << " " << (double)compressedUndeterminedCnt * 100 / (totalSize + 1)
    << std::endl;

    std::cout << "EgtbNewGenFileMng::compress END\n";
}

void EgtbNewGenFileMng::copyEgtb() {
//    std::cout << "EgtbNewGenFileMng::copyEgtb BEGIN\n";
//
//    std::string folder = EgtbFolderBase + "egtb/zip/1";
//    preload(folder, EgtbMemMode::all);
//
//    TbLookupMng tbLookupMng;
//    tbLookupMng.preload(EgtbFolderBase + "egtb/winning/lookup", EgtbMemMode::all);
//
//    std::cout << "#egtb: " << egtbFileVec.size() << std::endl;
//
//    std::string selectfolderW = EgtbFolderBase + "egtb/last/1/zipw/";
//    std::string selectfolderB = EgtbFolderBase + "egtb/last/1/zipb/";
//
//    Lib::createFolder(selectfolderW);
//    Lib::createFolder(selectfolderB);
//
//    for (auto && egtbFile : egtbFileVec) {
//        auto theName = egtbFile->getName();
//        auto r = std::count(theName.begin(), theName.end(), 'r');
//        auto c = std::count(theName.begin(), theName.end(), 'c');
//        auto h = std::count(theName.begin(), theName.end(), 'h');
//        auto p = std::count(theName.begin(), theName.end(), 'p');
//        auto n = r + c + h + p;
//        if (n != 1) {
//            continue;
//        }
//
//        std::string subFolder = "";
//        if (r) {
//            subFolder += "r";
//        }
//        if (c) {
//            subFolder += c == 1 ? "c" : "cc";
//        }
//        if (h) {
//            subFolder += h == 1 ? "h" : "hh";
//        }
//        if (p) {
//            subFolder += p == 1 ? "p" : "pp";
//        }
//
//        //auto wPath = selectfolderW + subFolder;
//
//        for(int sd = 0; sd < 2; sd ++) {
//            auto path = egtbFile->getPath(sd);
//            auto baseName = Lib::base_name(path);
//
//            auto newPath = (sd == B ? selectfolderB : selectfolderW) + subFolder;
//            auto newFileName = newPath + "/" + baseName;
//
//            Lib::createFolder(newPath);
//
//            Lib::copy_file(newFileName, path);
//        }
//    }
//
//    std::string lookupFolderW = EgtbFolderBase + "egtb/last/1/zipw/lookup/";
//    std::string lookupFolderB = EgtbFolderBase + "egtb/last/1/zipb/lookup/";
//
//    Lib::createFolder(lookupFolderW);
//    Lib::createFolder(lookupFolderB);
//
//    for(int sd = 0; sd < 2; sd ++) {
//        for (auto && thepair : tbLookupMng.nameMap[sd]) {
//            auto tbLookup = thepair.second;
//            auto theName = tbLookup->getName();
//            auto r = std::count(theName.begin(), theName.end(), 'r');
//            auto c = std::count(theName.begin(), theName.end(), 'c');
//            auto h = std::count(theName.begin(), theName.end(), 'h');
//            auto p = std::count(theName.begin(), theName.end(), 'p');
//            auto n = r + c + h + p;
//            if (n != 1) {
//                continue;
//            }
//
//            std::string subFolder = "";
//            if (r) {
//                subFolder += "r";
//            }
//            if (c) {
//                subFolder += c == 1 ? "c" : "cc";
//            }
//            if (h) {
//                subFolder += h == 1 ? "h" : "hh";
//            }
//            if (p) {
//                subFolder += p == 1 ? "p" : "pp";
//            }
//
//            auto path = tbLookup->getPath();
//            auto baseName = Lib::base_name(path);
//
//            auto newPath = (sd == 0 ? lookupFolderB : lookupFolderW) + subFolder;
//            auto newFileName = newPath + "/" + baseName;
//
//            Lib::createFolder(newPath);
//
//            Lib::copy_file(newFileName, path);
//        }
//    }
//
//    // Smallest between two
//    std::string smallestFolder = EgtbFolderBase + "egtb/last/smallest/";
//
//    Lib::createFolder(smallestFolder);
//
//    for (auto && egtbFile : egtbFileVec) {
//        auto theName = egtbFile->getName();
//        auto r = std::count(theName.begin(), theName.end(), 'r');
//        auto c = std::count(theName.begin(), theName.end(), 'c');
//        auto h = std::count(theName.begin(), theName.end(), 'h');
//        auto p = std::count(theName.begin(), theName.end(), 'p');
//        auto n = r + c + h + p;
//        if (n != 2) {
//            continue;
//        }
//
//        std::string subFolder = "";
//        if (r) {
//            subFolder += "r";
//        }
//        if (c) {
//            subFolder += c == 1 ? "c" : "cc";
//        }
//        if (h) {
//            subFolder += h == 1 ? "h" : "hh";
//        }
//        if (p) {
//            subFolder += p == 1 ? "p" : "pp";
//        }
//
//        //auto wPath = selectfolderW + subFolder;
//        i64 bsz = 0;
//        int theSd = B;
//        for(int sd = 0; sd < 2; sd ++) {
//            auto path = egtbFile->getPath(sd);
//            auto sz = Lib::getFileSize(path);
//
//            auto name = egtbFile->getName();
//
//            if (tbLookupMng.nameMap[sd].find(name) != tbLookupMng.nameMap[sd].end()) {
//                auto tbLookup = tbLookupMng.nameMap[sd][name]; assert(tbLookup);
//                auto lookuppath = tbLookup->getPath();
//                sz += Lib::getFileSize(lookuppath);
//            }
//
//            if (sd == 0) {
//                bsz = sz;
//            } else if (sz < bsz) {
//                theSd = W;
//            }
//        }
//
//        auto path = egtbFile->getPath(theSd);
//        auto baseName = Lib::base_name(path);
//
//        auto newPath = smallestFolder + subFolder;
//        auto newFileName = newPath + "/" + baseName;
//
//        Lib::createFolder(newPath);
//        Lib::copy_file(newFileName, path);
//
//        auto tbLookup = tbLookupMng.nameMap[theSd][egtbFile->getName()];
//        if (tbLookup) {
//            auto lookuppath = tbLookup->getPath();
//            baseName = Lib::base_name(lookuppath);
//
//            auto newLookupFileName = newPath + "/lookups/" + baseName;
//
//            Lib::createFolder(newPath + "/lookups");
//            Lib::copy_file(newLookupFileName, tbLookup->getPath());
//        }
//    }
//
//    std::cout << "EgtbNewGenFileMng::copyEgtb END\n";
}


//static const int orders3[] = {
//    0 | 1 << 3 | 2 << 6, // standard
//    0 | 2 << 3 | 1 << 6,
//    1 | 0 << 3 | 2 << 6,
//    1 | 2 << 3 | 0 << 6,
//    2 | 0 << 3 | 1 << 6,
//    2 | 1 << 3 | 0 << 6,
//    -1
//};
//
//static const int orders4[] = {
//    0 | 1 << 3 | 2 << 6 | 3 << 9, // standard
//    0 | 2 << 3 | 1 << 6 | 3 << 9,
//    0 | 2 << 3 | 3 << 6 | 1 << 9,
//
//
//    1 | 0 << 3 | 2 << 6 | 3 << 9,
//    1 | 2 << 3 | 0 << 6 | 3 << 9,
//    2 | 0 << 3 | 1 << 6 | 3 << 9,
//    2 | 1 << 3 | 0 << 6 | 3 << 9,
//    -1
//};

int* EgtbNewGenFileMng::createOrders(int n) {
    assert(n == 3 || n == 4);
    static int orders[40];

    int k = 0;
    for (int i0 = 0; i0 < n; i0++) {
        for (int i1 = 0; i1 < n; i1++) {
            if (i0 == i1) {
                continue;
            }
            for (int i2 = 0; i2 < n; i2++) {
                if (i0 == i2 || i1 == i2) {
                    continue;
                }
                if (n == 3) {
                    auto o = i0 | i1 << 3 | i2 << 6;
                    //std::cout << k << ") " << i0 << " | " << i1 << " << 3 | "  << i2 << " << 6 = " << o << std::endl;
                    orders[k++] = o;
                } else {
                    for (int i3 = 0; i3 < n; i3++) {
                        if (i0 == i3 || i1 == i3 || i2 == i3) {
                            continue;
                        }
                        auto o = i0 | i1 << 3 | i2 << 6 | i3 << 9;
                        //std::cout << k << ") " << i0 << " | " << i1 << " << 3 | "  << i2 << " << 6 | "  << i3 << " << 9 = " << o << std::endl;
                        orders[k++] = o;
                    }
                }
            }
        }
    }
//    orders[0] = 0;
    orders[k++] = -1;

    return orders;
}

void EgtbNewGenFileMng::convertPermutations(CompressMode compressMode) {
//    std::cout << "EgtbNewGenFileMng::convertPermutations BEGIN, compressMode: " << compressMode << " at: " << Lib::currentTimeDate() << std::endl;
//
//    std::string folder = EgtbFolderBase + "egtb/winning/2/hp";
//    preload(folder, EgtbMemMode::tiny);
//
//    for (auto && egtbFile : egtbFileVec) {
//        auto theName = egtbFile->getName();
//
////        if (theName != "KCPAAKAE") {
////            continue;
////        }
//
//        auto r = (int)std::count(theName.begin(), theName.end(), 'r');
//        auto c = (int)std::count(theName.begin(), theName.end(), 'c');
//        auto h = (int)std::count(theName.begin(), theName.end(), 'h');
//        auto p = (int)std::count(theName.begin(), theName.end(), 'p');
//        auto n = r + c + h + p;
//        if (n != 2) {
//            continue;
//        }
//
//        std::cout << theName << std::endl;
//
//        i64 idx = 0;
//        egtbFile->getScore(idx, Side::white, false);
//        egtbFile->getScore(idx, Side::black, false);
//
////        const int *orders = createOrders(PermutationOrderSize);
//        // 2 for two dependers of 2 sides, n - for strong count
//        const int orderSize = 2 + std::min(1, r) + std::min(1, c) + std::min(1, h) + std::min(1, p);
//        if (orderSize != 3 && orderSize != 4) {
//            std::cout << "Error: EgtbNewGenFileMng::convertPermutations, wrong orderSize " << orderSize << std::endl;
//            assert(false);
//            exit(-1);
//        }
//
//        const int *orders = createOrders(orderSize);
//        for (int i = 0; orders[i] >= 0; i++) {
//            int order = orders[i];
//            assert(i == 0 || order > 0);
//
//            std::string wFolder = EgtbFolderBase + "egtb/permutation/" + Lib::itoa(n);
//            Lib::createFolder(wFolder);
//            wFolder += (compressMode != CompressMode::compress_none ? "/zip/" : "/unzip/");
//            Lib::createFolder(wFolder);
//
//            wFolder += Lib::itoa(n);
//
//            char ch = 'a'; ch += i;
//            wFolder += ch;
//            wFolder += "/";
//            Lib::createFolder(wFolder);
//
//            if (r) {
//                wFolder += "r";
//            }
//            if (c) {
//                wFolder += c == 1 ? "c" : "cc";
//            }
//            if (h) {
//                wFolder += h == 1 ? "h" : "hh";
//            }
//            if (p) {
//                wFolder += p == 1 ? "p" : p == 2 ? "pp" : "ppp";
//            }
//
//            Lib::createFolder(wFolder);
//
//            const std::string whiteFile = EgtbFileWritting::createFileName(wFolder, egtbFile->getName(), egtbFile->getEgtbType(), Side::white, compressMode);
//            if (Lib::existFile(whiteFile.c_str())) {
//                const std::string blackFile = EgtbFileWritting::createFileName(wFolder, egtbFile->getName(), egtbFile->getEgtbType(), Side::black, compressMode);
//                if (Lib::existFile(blackFile.c_str())) {
//                    std::cout << "\tFile existent for " << egtbFile->getName() << " in folder: " << wFolder << ", order: " << order << ", compressed: " << compressMode << std::endl;
//                    continue;
//                }
//            }
//
//            std::cout << "\tsaving in folder: " << wFolder << ", order: " << order << ", compressed: " << compressMode << "\n\twhiteFile: " << whiteFile << std::endl;
//
//            EgtbFileWritting newEgtbFile;
//
//            newEgtbFile.create(egtbFile->getName(), egtbFile->getEgtbType(), order);
//            newEgtbFile.createBuffersForGenerating();
//
//            newEgtbFile.convertPermutations(egtbFile);
//
//            assert(egtbFile->getSize() == newEgtbFile.getSize());
//
//            newEgtbFile.saveFile(wFolder, compressMode);
//        }
//
//        egtbFile->removeBuffers();
//    }
//
//    std::cout << "EgtbNewGenFileMng::convertPermutations END\n";
}


void EgtbNewGenFileMng::testConvertPermutations() {
//    std::cout << "EgtbNewGenFileMng::testConvertPermutations BEGIN at " << Lib::currentTimeDate() << std::endl;
//
//    EgtbNewGenFileMng otherEgtbNewGenFileMng;
//    otherEgtbNewGenFileMng.preload(EgtbFolderBase + "egtb/permutation/1a", EgtbMemMode::all);
//
//    std::string folder = EgtbFolderBase + "egtb/winning/2";
//    //    std::string folder = EgtbFolderBase + "egtb/dtm/1";
//    preload(folder, EgtbMemMode::all);
//
//    EgtbBoard board;
//    for (auto && egtbFile : egtbFileVec) {
//        auto theName = egtbFile->getName();
//
//        auto r = std::count(theName.begin(), theName.end(), 'r');
//        auto c = std::count(theName.begin(), theName.end(), 'c');
//        auto h = std::count(theName.begin(), theName.end(), 'h');
//        auto p = std::count(theName.begin(), theName.end(), 'p');
//        auto n = r + c + h + p;
//        if (n != 1) {
//            continue;
//        }
//
//        auto otherEgtbFile = otherEgtbNewGenFileMng.getEgtbFile(theName);
//        assert(otherEgtbFile);
//
////        i64 idx = 45500;
////        int score = otherEgtbFile->getScore(idx, Side::white);
////        assert(score == 504);
////        std::cout << score << std::endl;
//
//        for(i64 idx = 0; idx < egtbFile->getSize(); idx++) {
//            int scores[] = {
//                egtbFile->getScore(idx, Side::black, false),
//                egtbFile->getScore(idx, Side::white, false)
//            };
//            if (scores[B] == EGTB_SCORE_ILLEGAL && scores[W] == EGTB_SCORE_ILLEGAL) {
//                continue;
//            }
//
//            setup(egtbFile, board, idx);
//
//            for (int sd = 0; sd < 2; sd++) {
//                auto side = static_cast<Side>(sd);
//                int score = otherEgtbFile->getScore((const int *)board.pieceList, side);
//                assert(scores[sd] == EGTB_SCORE_ILLEGAL || score == scores[sd]);
//            }
//        }
//
//        egtbFile->removeBuffers();
//        otherEgtbFile->removeBuffers();
//    }
//
//    std::cout << "EgtbNewGenFileMng::testConvertPermutations END\n";
}

void EgtbNewGenFileMng::createLookupTablesWithPermutations() {
//    std::cout << "EgtbNewGenFileMng::createLookupTablesWithPermutations BEGIN at " << Lib::currentTimeDate() << std::endl;
//
//    const std::string traditionalFolder = EgtbFolderBase + "egtb/dtm/2/hp";
////    const std::string traditionalFolder = EgtbFolderBase + "egtb/dtm/1";
//
//    const int *orders = createOrders(PermutationOrderSize);
//
//    for (int i = 0; orders[i] >= 0; i++) {
//        char ch = 'a' + i;
//
//        closeAll();
//
//        const std::string winningFolder = EgtbFolderBase + "egtb/permutation/2/unzip/2" + ch;
//        std::string lookupFolder        = EgtbFolderBase + "egtb/permutation/2/zip/lookup";
////        const std::string winningFolder = EgtbFolderBase + "egtb/permutation/1/unzip/1" + ch;
////        std::string lookupFolder = EgtbFolderBase + "egtb/permutation/1/zip/lookup";
//
//        Lib::createFolder(lookupFolder);
//        lookupFolder += "/2";
////        lookupFolder += "/1";
//        lookupFolder += ch;
//
////        const std::string winningFolder = EgtbFolderBase + "egtb/permutation/3/unzip/3" + ch;
////        const std::string lookupFolder = EgtbFolderBase + "egtb/permutation/3/zip/lookup/3" + ch;
//
//        Lib::createFolder(winningFolder);
//        Lib::createFolder(lookupFolder);
//
//        EgtbNewGenFileMng egtbNewGenFileMng;
//        egtbNewGenFileMng.createLookupTables(traditionalFolder, winningFolder, lookupFolder);
//    }
//
//    std::cout << "EgtbNewGenFileMng::createLookupTablesWithPermutations DONE\n";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FileInfo {
public:
    std::string name, path, fileName;
    i64 size;
    Side side;
    int groupIdx;
    bool isCompressed;

    EgtbType type;

    FileInfo() {
        size = 0;
    }

    FileInfo(const std::string& _path) {
        parse(_path);
    }

    bool isValid() const {
        return !name.empty() && !path.empty() && !fileName.empty() && size > 0 && type != EgtbType::none && side != Side::none; // groupIdx >= 0 &&
    }

private:

    bool parse(const std::string& _path) {
        path = "";
        auto p = EgtbFile::getExtensionType(_path);
        if (p.first != EgtbType::dtm && p.first != EgtbType::newdtm && p.first != EgtbType::lookup) {
            return false;
        }
        type = p.first;
        path = _path;
        isCompressed = path.find(".z") != std::string::npos;

        size = Lib::getFileSize(_path);

        fileName = Lib::base_name(_path);
        side = fileName.find("w.") != std::string::npos ? Side::white : Side::black;
        name = fileName.substr(0, fileName.length() - 5); // "w.tbl"

        auto r = std::count(name.begin(), name.end(), 'r');
        auto c = std::count(name.begin(), name.end(), 'c');
        auto h = std::count(name.begin(), name.end(), 'h');
        auto b = std::count(name.begin(), name.end(), 'p');
        auto n = (int)(r + c + h + b);

        groupIdx = -1;

        const int *orders = EgtbNewGenFileMng::createOrders(4); //n == 1 ? 3 : 4);

        for (int i = 0; orders[i] >= 0; i++) {
            char ch = 'a' + i;
            std::string s = "/" + Lib::itoa(n); // n == 1 ? "/1" : "/2";
            s += ch;
            if (path.find(s) != std::string::npos) {
                groupIdx = i;
                break;
            }
        }
        return groupIdx >= 0;
    }
};


class FileSet {
public:
    FileInfo* egtbFileInfo[2];
    FileInfo* loopupFileInfo[2];

    i64 size[2];
    int fileCnt;

    FileSet() {
        egtbFileInfo[0] = egtbFileInfo[1] = nullptr;
        loopupFileInfo[0] = loopupFileInfo[1] = nullptr;
        size[0] = size[1] = 0;
        fileCnt = 0;
    }

    ~FileSet() {
        for(int i = 0; i < 2; i++) {
            if (egtbFileInfo[i]) delete egtbFileInfo[i];
            if (loopupFileInfo[i]) delete loopupFileInfo[i];
        }
    }

    bool isValid() const {
        for(int sd = 0; sd < 2; ++sd) {
            if ((egtbFileInfo[sd] && !egtbFileInfo[sd]->isValid()) || (loopupFileInfo[sd] && !loopupFileInfo[sd]->isValid())) {
                return false;
            }
        }

        auto haveFiles = egtbFileInfo[0] || egtbFileInfo[1] || loopupFileInfo[0] || loopupFileInfo[1];
        if ((haveFiles && (fileCnt == 0 || size[0] + size[1] == 0)) || (!haveFiles && fileCnt + size[0] + size[1])) {
            return false;
        }
        return true;
    }

    void add(FileInfo* fileInfo) {
        assert(fileInfo && fileInfo->isValid() && isValid());
        int sd = static_cast<int>(fileInfo->side);
        if (fileInfo->type == EgtbType::lookup) {
            assert(loopupFileInfo[sd] == nullptr);
            loopupFileInfo[sd] = fileInfo;
        } else {
            assert(egtbFileInfo[sd] == nullptr);
            egtbFileInfo[sd] = fileInfo;
        }
        size[sd] += fileInfo->size;
        fileCnt ++;

        assert(isValid());
    }
};

class PermutationSet {
public:
    FileSet* array[100];

    PermutationSet() {
        memset(array, 0, sizeof(array));
    }
    ~PermutationSet() {
        for(int i = 0; i < sizeof(array) / sizeof(FileSet*); i++) {
            if (array[i]) delete array[i];
        }
    }
};

void EgtbNewGenFileMng::copyPermutations()
{
//    std::cout << "\n\n********** EgtbNewGenFileMng::copyPermutations BEGIN at " << Lib::currentTimeDate() << std::endl;
//
//    // name, group, file set
//    std::map<std::string, PermutationSet*> fileMap;
//
////    // Load & parse
////    auto vec = Lib::listdir(permutationFolder);
//
////    auto vec = Lib::listdir(EgtbFolderBase + "egtb/winning/zip/2/hp");
////    auto vec2 = Lib::listdir(EgtbFolderBase + "egtb/winning/lookup/hp");
////    vec.insert(vec.end(), vec2.begin(), vec2.end());
//
//    auto vec = Lib::listdir(EgtbFolderBase + "egtb/dtm/1"); // + "egtb/testing/zip/3/ppp");
//
//    for (auto && path : vec) {
//        FileInfo* fileInfo = new FileInfo(path);
//        if (!fileInfo->isValid()) {
//            delete fileInfo;
//            continue;
//        }
//
//        if (!fileInfo->isCompressed && fileInfo->type != EgtbType::lookup) {
//            std::cerr << "Error: copyPermutations, NOT COMPRESSED file " << fileInfo->fileName << std::endl;
//            exit(-1);
//        }
//
//        PermutationSet *m;
//        if (fileMap.find(fileInfo->name) != fileMap.end()) {
//            m = fileMap[fileInfo->name];
//        } else {
//            m = new PermutationSet();
//            fileMap[fileInfo->name] = m;
//        }
//
//
//        auto groupIdx = std::max(0, fileInfo->groupIdx);
//        FileSet* fileSet = m->array[groupIdx];
//        if (fileSet == nullptr) {
//            fileSet = new FileSet();
//            m->array[groupIdx] = fileSet;
//        }
//
//        fileSet->add(fileInfo);
//    }
//
//    std::cout << "fileMap sz: " << fileMap.size() << std::endl;
//
//    // Copy
//    for(auto && m : fileMap) {
//        assert(m.second);
//
//        int minSd = -1;
//        i64 minSize = -1;
//        FileSet* minFileSet = nullptr;
//        for(int i = 0; i < sizeof(m.second->array) / sizeof(FileSet*); i++) {
//            FileSet* fileSet = m.second->array[i];
//            if (!fileSet) {
//                continue;
//            }
//            assert(fileSet->egtbFileInfo[0] && fileSet->egtbFileInfo[1]);
//
//            for(int sd = 0; sd < 2; sd++) {
//                if (minSize < 0 || minSize > fileSet->egtbFileInfo[sd]->size) {
//                    minSize = fileSet->egtbFileInfo[sd]->size;
//                    minFileSet = fileSet;
//                    minSd = sd;
//                }
//            }
//        }
//
//        if (minFileSet) {
//            assert(minSd == 0 || minSd == 1);
//
//            auto theName = minFileSet->egtbFileInfo[minFileSet->egtbFileInfo[0] ? 0 : 1]->name;
//            auto wFolder = EgtbFolderBase + "egtb/testing/best";
//            Lib::createFolder(wFolder);
//            wFolder +=  + "/";
//
//            SubfolderParser parser(theName);
//            parser.createAllSubfolders(wFolder);
//
//            wFolder += parser.subfolder + "/";
//
//            const std::string dFile = wFolder + minFileSet->egtbFileInfo[minSd]->fileName;
//            Lib::copy_file(dFile, minFileSet->egtbFileInfo[minSd]->path);
//
//            if (minFileSet->loopupFileInfo[minSd]) {
//                wFolder += "lu/";
//                Lib::createFolder(wFolder);
//
//                const std::string dFile = wFolder + minFileSet->loopupFileInfo[minSd]->fileName;
//                Lib::copy_file(dFile, minFileSet->loopupFileInfo[minSd]->path);
//            }
//        }
//    }
//
//    for(auto && m : fileMap) {
//        delete m.second;
//    }
//
//    std::cout << "EgtbNewGenFileMng::copyPermutations END\n\n";
}

void EgtbNewGenFileMng::testLoading()
{
    std::cout << "\n\nEgtbNewGenFileMng::testLoading BEGIN\n";
    auto egtbMemMode = EgtbMemMode::all;

//    EgtbFileWritting ef;
//    ef.preload("/Users/TonyPham/workspace/egtb/dtm/2/cc/kccaaeekaaeeb.xtb", egtbMemMode);

//    std::string folder = EgtbFolderBase + "egtb/dtm/3";
    std::string folder = "/Volumes/Seagate Expansion Drive/Nguyen/egtb/compressed/2/ch"; // EgtbFolderBase + "egtb/winning/1-newzip";
    addFolder(folder);


    for (auto && folderName : folders) {
        auto vec = Lib::listdir(folderName);

        for (auto && path : vec) {
            if (path.find("kchaaeekaaee") == std::string::npos) {
                continue;
            }
            auto p = EgtbFile::getExtensionType(path);
            if (p.first == EgtbType::dtm || p.first == EgtbType::newdtm) { //} || p.first == EgtbType::NewDtm2) {
                std::cout << "EgtbNewGenFileMng::testLoading path: " << path << std::endl;
                EgtbFileWritting egtbFile;
                if (!egtbFile.preload(path, egtbMemMode, EgtbLoadMode::loadnow)) {
//                    std::cerr << "Error: cannot load: " << path << std::endl;
                }
            } else if (p.first == EgtbType::lookup) {
                EgtbLookup lookup;
                if (!lookup.preload(path, egtbMemMode, EgtbLoadMode::loadnow)) {
//                    std::cerr << "Error: cannot load lookup: " << path << std::endl;
                }
            } else {
            }
        }
    }

    std::cout << "EgtbNewGenFileMng::testLoading END\n";
}

