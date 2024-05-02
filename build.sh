mkdir exect
cd exect
rm *

g++ -c ../src/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_
g++ -c ../src/base/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_
g++ -c ../src/chess/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_CHESS_

gcc -std=c99 -c ../src/lzma/*.c -D_7ZIP_ST

g++ -o egtbdemo *.o
rm *.o
cd ..
./exect/egtbdemo
