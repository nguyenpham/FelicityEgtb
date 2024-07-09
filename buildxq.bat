md exect
cd exect
del /Q *

cl -std:c99 -c ../src/lzma/*.c =O3 -D_7ZIP_ST

cl -c ../src/*.cpp -std:c++17 -O3 -DNDEBUG -D_FELICITY_XQ_
cl -c ../src/base/*.cpp -std:c++17 -O3 -DNDEBUG -D_FELICITY_XQ_
cl -c ../src/xq/*.cpp -std:c++17 -O3 -DNDEBUG -D_FELICITY_XQ_
cl -c ../src/fegtb/*.cpp -std:c++17 -O3 -DNDEBUG -D_FELICITY_XQ_
cl -c ../src/fegtbgen/*.cpp -std:c++17 -O3 -DNDEBUG -D_FELICITY_XQ_

cl -o fegtbxq *.obj
del *.obj
cd ..
exect\fegtbxq.exe
