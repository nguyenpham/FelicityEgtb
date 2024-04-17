mkdir exect
cd exect
rm *
g++ -c ../src/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_
g++ -c ../src/base/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_
g++ -c ../src/xq/*.cpp -std=c++17 -O2 -DNDEBUG -D_FELICITY_XQ_

g++ -o egtbdemo *.o
rm *.o
cd ..
./exect/egtbdemo
