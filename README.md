# Felicity EGTB - Generation and probe code for chess, Xiangqi and Jeiqi


Overview
========

Felicity EGTB (Endgame database/tablebase) is an open source for generating and probing EGTB. All code is written in C++ (using the standard C++17 library). It supports some chess variants: chess, Xiangqi, and Jeiqi.

From testing, EGTB doesn't help chess engines much, in terms of strength/Elo gaining. 6 men-Syzygy EGTB can help Stockfish to gain smaller than 13 Elo. However, we believe EGTB can help much more for Xiangqi/Jeiqi engines because the endgames of those chess variants are much more complicated, with a lot of exceptions/tricky wins, which are much harder for humans/programs to learn. However, we don't have real statistics. We need to build some good EGTB first before measuring.

This project focuses on Xiaqqi and Jeiqi. But we support chess too, for testing, learning and comparing.

The name Felicity is inspired by the song Felicità by Al Bano & Romina Power.

Restart
=======

April 2024: restart the project. Plan to rewrite all. Support chess, Xiangqi, Jeiqi


Progress reports
================
It is not a ready-to-use EGTB but a work in progress for studying/researching. It is not stuck immediately to specific things such as metrics, board presentations, algorithms for compressing, or backward/forward generators. Instead, it tries testing as much as possible with multiple options to find the best ones. Not every attempt/work is successful. The project creates reports on some forums to report all attempts, works and results.


Info
====
All info will be updated in this repository and forum:

https://banksiagui.com/forums/viewforum.php?f=20


Attempts
========
Attempts 1-7: Test speeds of some basic functions move generator, make, takaback, incheck and compare with Stockfish family.

Concluded for current mailbox board representation:
- simple: all code is short, clear, easy to understand and maintenance
- easy to support different chess variants, from small to huge boards
- fast (for basic functions such as in/out, move generators, make, takeback, incheck...) enough for chess, be faster than ones with bitboards for huge board

Attempt 8: We compare the access speeds of some arrays in memory:
- created some huge arrays in memory (about 7 GB) using 3 ways: malloc, std::vector and std::array
- create random values and set them to random locations in those arrays
- measure time
Conclusion: their speeds are quite similar. We can use any of them.

Attempt 9: Position indexes work for both Chess and Xiangqi/Jeiqi.

Attempt 10: Calculate position indexes for Xiangqi/Jeiqi.

Attempt 11: Calculate 5-men Index space for chess.

Attempt 12: Generating all 5-men endgames for chess, using the forward method. Total time: 3 days 15 hours (generating time only). Total size: 8.88 GB, endgames: 145, files: 190. Hardware: AMD Ryzen 7 1700 8-Core 16 threads, 16 GB RAM, ran with 12 threads and took about 85% of the computer's power.

Attempt 13: Generate all 5-men endgames for chess using the backward method. Total time: 21 hours (generating time only), 4 times faster than Attempt 12.

Attempt 14: Improve the speed of generating all 5-man endgames for chess with the backward method. Total time: 17 hours (generating time only), 5 times faster than Attempt 12.

Attempt 15: Calculate index spaces for more men for chess. Some code has been added to make sure the generator can generate 7 men and get information to 9-man. Some variants (unsigned 64-bit) starts being overflowed when working with 10 and 11 men.

Attempt 16: Store whole board data in Hist record when making a move, thus the takeback function could be simplified by copying back the stored board. Chess perft 5 took 523 ms, 161% longer (slower) than Attempt 7. The code is in the branch "wholeboardinhist".

Attempt 17: Use the table move generator. It could speed up Perft to 10%. If using table move data for incheck function, the speed up is about 5% (slower). The result is good but not enough to use immediately. The code is in the branch "tablemovegen".

Attempt 18: Create a test EPD file from all existent endgames. The test file will be used to verify the correctness of an EGTB later.

Attempt 19: more compact for chess index space. For 5 men chess EGTB, the index space sank about 13%. The size becomes 7.5 GB, a 15.5% reduction (compared with the previous size of 8.88 GB). However, the time to generate is about 29% longer (22 hours vs 17 hours). We are still not clear yet why/where the speed was lost.

Attempt 20: Xiangqi generator has been added. It can generate endgames with a side has up to 2 attackers and one side has no attacker (armless). E.g., kraabkaabb, kcnaabbkaabb, kcpakaab.

Status
======

In development: All code has been still being written. You may use them for studying, trying to integrate them into your code, or generating some small EGTBs. However, you are suggested to not generate EGTBs seriously nor contribute them since everything could be changed, from algorithms to file structures.

- Chess: it could generate all 5 men. 6-man is possible but not verified yet
- Xiangqi: it could generate endgames when one side armless, other side has 1 or 2 attackers
- Jeiqi: no code yet


Terms of use
============

Our code and data (endgame files) are released under the liberal [MIT license](http://en.wikipedia.org/wiki/MIT_License), so basically you can use it with almost no restrictions.


Credits
=======

Felicity Egtb was written by Nguyen Hong Pham (axchess at yahoo dot com).


