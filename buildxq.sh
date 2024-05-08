mkdir exect
cd exect
rm *

gcc -std=c99 -c ../src/lzma/*.c -D_7ZIP_ST

g++ -c ../src/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_
g++ -c ../src/base/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_
g++ -c ../src/xq/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_
g++ -c ../src/fegtb/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_
g++ -c ../src/fegtbgen/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_

g++ -o fegtb *.o
rm *.o

cd ..
./exect/fegtb
