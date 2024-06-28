Perft speed
==========

The speed when generating EGTB depends much on the speed of some so basic functions:
- Move generator
- Check InCheck
- Make move, takeback

Perft is a perfect one to check the correctness and speed of those functions. In this task, we attempt to make those functions be fast enough.

Board representation
--------------------

The board of Felicity EGTB uses the mailbox technique. We love that representation because of being simple and easily support multi chess variants. We start by taking the board from our OCGDB code project (another of our open-source projects), add Perft code. To know how fast our perft is, we compare it with Stockfish 16.1. Stockfish is compiled for standard x86-64 (make -j profile-build ARCH=x86-64) without using any special struct such as BMI, or AVX). All run on my old computer iMac 3.6 GHz Quad-Core Intel Core i7 (7-year-old)

Stockish run for pertf 5 with starting position and took 24 ms

Bellow attempts are elapsed time for perft 5 too. All node counts are correct.

Attempt 1
---------
Origin board took from OCGDB project: 734 ms

The board of Felicity EGTB is 734/24 = 31 times as slow as Stockfish.

Attempt 2
---------

We removed all "redundant" functions/features such as move comments, hash key, mutex locks... It took 579 ms = 579/24 = 21 times slower than Stockfish

Attempt 3
---------
We used the piece list technique to speed up. It took 374 ms = 374/24 = 16 times slower than Stockfish

It is amazing Stockfish is so fast even it is much more complicated than our current code. The speed of our code is so disappointing.


Attempt 4
---------
We run perft with Xiangqi (Chinese chess) board with the same techniques (mailbox representation + piece list). We used Pikafish (a Xiangqi chess engine developed from Stockfish) to compare. Pikafish was compiled for standard x86-64. All perft depths are 5 and for start position.
Pikafish took 1498 ms.
Our code took 6260 ms = 6260/1498 = 4 times slower than Pikafish
The gap between two programs for Xiangqi is much smaller than ones for Chess but it is still so large.


Attempt 5
---------
We tried to reduce the number of calling the function incheck for the Xiangqi board. Typically that function is called whenever making a move to check if that move is valid or invalid. The new code calls that function considering if it may affect the status of being incheck: the positions of the move are the same rank or column with the king or on checkable, blockable positions of the horse. It used some simple tables to check the status.

The number of calling the incheck function is reduced significantly to about half.

The new code took 6536 ms, a bit larger than Attempt 5. It is a surprising and upsetting result.

All code has been pushed into a new branch "incheck".


Attempt 6
---------

Just clean the code, rewrite some parts, and cut some redundant variants and functions.
- Chess perft 5 took 324 ms
- Xiangqi perft 5 took 5453 ms
 
The code looks better and becomes a bit faster


Attempt 7
---------

Our code for basic functions such as board representations, move generators, move makes, take back, check in-check… is simple and straightforward. Logically, it should be fast, their speed should be comparable with the fastest chess engines such as Stockfish, not the big gaps as in previous attempts. Thus we doubt the way to calculate perfts. Stockfish and Pikafish use a method named "Bulk-counting", to ignore making/taking back leaf nodes, and save a lot of time. In contrast, our code uses a different method and has to make/take back all nodes, including leaf nodes, which cannot save as much time as Stockfish. Since we need to compare the speed of basic functions it’s better to use the same method for perfts.

We have modified code of Stockfish/Pikafish to calculate perfts similar to our code and got new statistics:

- Stockfish perft 5 took 187 ms
- Pikafish perft 5 took 8023 ms

Stockfish is still faster than our program (187 ms vs 324 ms = 1.7 times faster) but the gap is now acceptable. We guess Stockfish becomes faster due to having a lot of pre-calculation data and the chess board of 64 squares is optimized for model computers of 64 bits.

On the other hand, our program is surprisingly faster than Pikafish on the Xiangqi variant (5453 ms vs 8023 ms = 1.5 times faster). We guess the huge board of 90 squares is not fit for integers of 64 bits, making it not optimised for running on model computers. For those kinds of huge boards, mailbox board representation has some advantages.

We list some advantages of mailbox mailbox board representation:
- simple: all code is short, clear, easy to understand and maintenance
- easy to support different chess variants, from small to huge boards
- fast (for basic functions such as in/out, move generators, make, takeback, incheck...) enough for chess, be better than ones with bitboards for huge board

It is good that our work targets mainly Xiangqi but not chess. Thus we are satisfied with the results, finish that task, and move on to others.


Attempt 16
----------

When making a move, whole board data is copied into Hist record, thus the takeback function could be simplified by copying back stored board.

- Chess perft 5 took 523 ms, 161% longer (slower) than Attempt 7

The code is in the branch "wholeboardinhist".
