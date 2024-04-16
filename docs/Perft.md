Perft speed
==========

The speed when generating EGTB depends much on the speed of some so basic functions:
- Move generator
- Check InCheck
- Make move, takeback

Perft is a perfect one to check the correctness and speed of those functions. In this task, we attempt to make those functions be fast enough.

Board representation
--------------------

The board of Felicity EGTB uses the mailbox technique. We start by taking the board from our OCGDB code project (another of our open-source projects), add Perft code. To know how fast our perft is, we compare it with Stockfish 16.1. Stockfish is compiled for standard x86-64 (make -j profile-build ARCH=x86-64) without using any special struct such as BMI, or AVX). All run on my old computer iMac 3.6 GHz Quad-Core Intel Core i7 (7-year-old)

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
