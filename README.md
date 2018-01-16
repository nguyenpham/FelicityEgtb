Felicity Egtb - A Xiangqi (Chinese chess) Endgame database
==============


Overview
-----------
Felicity Egtb is a Xiangqi Endgame database (tablebase) released with some endgames and probing code. The probing code is  written in C++ (using standad C++11 library). Xiangqi developers could use this to add freely and quickly tablebase probing to their Xiangqi engines.


New design
--------------
We have developed succefully a special design and algorithms for our tablebase to massively reduce the data size, compared with traditional ones. All one-attacker endgames are about 10 times smaller, take only 14 MB in storage (traditional one may take over 250 MB in compressed form). They can be loaded partly or all into memory (depend on loading params). If loading all into memory, they will be auto decompressed and take about 250 MB memory (traditional one may take over 2 GB memory in uncompressed form).


Endgames
------------
At current state, the tablebase has all endgames of one attacker (Rook, Cannon, Horse, Pawn) with all combinations of defenders. Total is 108 endgames, 14 MB.

Felicity egtb uses DTM metric.


Demo
-------
The source code is realeased with a demo in the file main.cpp (remove this file when adding source to your project). You may compile and run it.

#### MacOS with XCode
Click to open FelicityEgtb.xcodeproj with XCode and run it

#### Linux / MacOS with gcc, g++

    bash build.sh

#### Windows with VisualStudio
Click to open VisualStudio.sln with VisualStudio  (2017) and run it


The screen shot of the demo as the below:

![Demo](https://github.com/nguyenpham/FelicityEgtb/blob/master/demo1.png)


Using
-------
To declare:

    #include "Egtb.h"
    ...
    egtb::EgtbDb egtbDb;

To init data you must give the main folder of those endgames (endgames can locate in some sub folders). The library will auto scan that folder as well as any sub folders to load all data it has known:

    const char* egtbDataFolder = "c:\\myfolder\\egtb";
    egtbDb.preload(egtbDataFolder, egtb::EgtbMemMode::all, egtb::EgtbLoadMode::loadnow);

If endgames locate in several folders, you may add them one by one:

    const char* egtbDataFolder0 = "c:\\egtb0";
    egtbDb.addFolder(egtbDataFolder0);
    const char* egtbDataFolder1 = "c:\\egtb1";
    egtbDb.addFolder(egtbDataFolder1);
    egtbDb.preload(egtb::EgtbMemMode::all, egtb::EgtbLoadMode::loadnow);


You may check if it could load some tablebases and print out an error message:

    if (egtbDb.getSize() == 0) {
        std::cerr << "Error: could not load any data" << std::endl;
    }

With memory mode egtb::EgtbMemMode::all as above example, all data will be loaded, auto decompressed into memory (RAM) and the library won't access external storage anymore. With mode egtb::EgtbMemMode::tiny, the library will load only files' headers into memory and alloc some buffers in the memory (total about few MB). If the data in those buffers are out of range (missed the caches), The library will access external storage to read data in block, decompressed and return results when probing.

Now you may query scores (distance to mate) for any position. Your input could be fen strings or vectors of pieces which each piece has type, side and location:

    std::vector<egtb::Piece> pieces;
    pieces.push_back(egtb::Piece(egtb::PieceType::rook, egtb::Side::white, egtb::Squares::d0));
    pieces.push_back(egtb::Piece(egtb::PieceType::king, egtb::Side::white, egtb::Squares::e0));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::white, egtb::Squares::d2));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::white, egtb::Squares::e1));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::white, egtb::Squares::c0));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::white, egtb::Squares::e2));
    pieces.push_back(egtb::Piece(egtb::PieceType::king, egtb::Side::black, egtb::Squares::e7));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::black, egtb::Squares::d7));
    pieces.push_back(egtb::Piece(egtb::PieceType::advisor, egtb::Side::black, egtb::Squares::e8));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::black, egtb::Squares::a7));
    pieces.push_back(egtb::Piece(egtb::PieceType::elephant, egtb::Side::black, egtb::Squares::c9));

    auto score = egtbDb.getScore(pieces);
    std::cout << "Queried, score: " << score << std::endl;


Compile
----------
If you use other C++ IDE such as Visual Studio, xCode, you need to creat a new project and add all our code with prefix Egtb and all file in lzma folder, set compile flags to C++11, then those IDEs can compile those source code automatically.

In case you want to compile those code manually with gcc, g++, you may use gcc to compile all C files, g++ for cpp files, then use g++ to link into executive file:

    gcc -std=c99 -c ./lzma/*.c
    g++ -std=c++11 -c *.cpp -O3 -DNDEBUG
    g++ -o yourenginename *.o


Working
---------
- Better probing code
- Reduce sizes
- Add more endgames


History
--------

- 16 Jan 2018: version 1.00


Terms of use
---------------

The files in folder lzma are copyrighted by [7-zip.org](http://7-zip.org) and were released under public domain (free).
Our code and data (egtb files) are released under the liberal [MIT license](http://en.wikipedia.org/wiki/MIT_License), so basically you can use it with almost no restrictions.


Credits
--------

Felicity Egtb was written by Nguyen Hong Pham (axchess at yahoo dot com).


