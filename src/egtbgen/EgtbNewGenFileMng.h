//
//  EgtbNewGenFileMng.h
//
//  Created by TonyPham on 12/3/17.
//

#ifndef EgtbNewGenFileMng_hpp
#define EgtbNewGenFileMng_hpp

#include<sys/stat.h>
#include<sys/types.h>


#include "EgtbGenFileMng.h"
#include "TbLookupMng.h"

namespace egtb {

enum class StrongDConfig {
    OpenMid0,
    OpenMid1,
    OpenLeft,
    OpenRight,

    CloseMid0,
    CloseMid1,
    CloseLeft,
    CloseRight,

    Max
};

class EgtbNewGenFileMng : public EgtbGenFileMng {
public:
    void genWinningEgtbs(std::string traFolder, std::string folder, const std::string& name, bool includeSubs, bool permutate);

    bool genSingleWinningEgtb(EgtbDb* EgtbDb, const std::string& folder, const std::string& name, bool permutate);

    static int* createOrders(int n);

private:
    int testSizePermutation();
    void convertLookupForNewOrder(EgtbFileWritting* orgEgtbFile, EgtbFileWritting* newEgtbFile);

public:
    bool genSingleWinningEgtb_loop(int threadIdx);

    void createLookupTables();
    void createLookupTables(const std::string& traditionalFolder, const std::string& winningFolder, const std::string& lookupFolder, const int* orders = nullptr);
    bool createLookupTables(int threadIdx, EgtbLookupWritting& tbLookup, EgtbDb& traEgtbFileMng, EgtbFile* egtbFile, i64 fromIdx, i64 toIdx, int sd, const int* orders);

public:
    void loadLookupTables();
    void testLookupTables();

private:
    void createLookupTables(EgtbDb& traEgtbFileMng, EgtbFile* egtbFile, std::string& saveFolder, CompressMode compressMode, const int* orders);

public:
    void verifyData(const std::string& traFolder, const std::string& newFolder, const std::string& name, bool includeSubs);

protected:
    bool verifyData_loop(int threadIdx, EgtbFile* traEgtbFile);

protected:
    bool verifyDataWithTraditional(EgtbFile* pTraEgtbFile);
    bool verifyDataWithNew(EgtbNewGenFileMng* traEgtbFileMng, const std::string& name);

public:
    void convert();
    void convertVersion();

    void compress();
    void copyEgtb();

    void convertPermutations(CompressMode compressMode);
    void testConvertPermutations();
    void createLookupTablesWithPermutations();
    void copyPermutations();

    void testLoading();
    
protected:
    EgtbLookupWritting tbLookup[2];
    EgtbDb* traEgtbFileMng;
};

} // namespace egtb

#endif /* EgtbNewGenFileMng_hpp */
