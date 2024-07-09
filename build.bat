md exect
cd exect
del /Q *

cl -std:c99 -c ../src/lzma/*.c -D_7ZIP_ST

cl -c ../src/*.cpp -std:c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_
cl -c ../src/base/*.cpp -std:c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_
cl -c ../src/chess/*.cpp -std:c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_
cl -c ../src/fegtb/*.cpp -std:c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_
cl -c ../src/fegtbgen/*.cpp -std:c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_

cl -o fegtb *.obj
del *.obj
cd ..
exect\fegtb.exe
