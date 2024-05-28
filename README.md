# Felicity EGTB - Generation and probe code for chess, Xiangqi and Jeiqi


Overview
========

Felicity EGTB (Endgame database/tablebase) is an open source for generating and probing EGTB. All code is written in C++ (using the standard C++17 library). It supports some chess variants: chess, Xiangqi, and Jeiqi.

From testing, EGTB doesn't help chess engines much, in terms of strength/Elo gaining. 6 men-Syzygy EGTB can help Stockfish to gain < 13 Elo.
In contrast, we believe EGTB can help much more for Xiangqi/Jeiqi engines because the endgames of those chess variants are much more complicated, with a lot of exceptions/tricky wins, which are much harder for humans/programs to learn. However, we don't have real statistics. We need to build some good EGTB first before measuring.
This project focuses on Xiaqqi and Jeiqi. But we will support chess too, for testing, learning and comparing.


Restart
=======

April 2024: restart the project. Plan to rewrite all. Support chess, Xiangqi, Jeiqi
The project has been restarted. We will rewrite all code when doing some research.

Research
========
This is a long journey, to find good code, algorithmns.


Info
====
All info will be updated in this github and in forum:

https://banksiagui.com/forums/viewforum.php?f=20


Attempts
========
Attempts 1-7: test speeds of some basic functions move generator, make, takaback, incheck and compare with Stockfish family.

Concluded for current mailbox board representation:
- simple: all code is short, clear, easy to understand and maintenance
- easy to support different chess variants, from small to huge boards
- fast (for basic functions such as in/out, move generators, make, takeback, incheck...) enough for chess, be faster than ones with bitboards for huge board

Attempt 8: We compare the access speeds of some arrays in memory:
- created some huge arrays in memory (about 7 GB) using 3 ways: malloc, std::vector and std::array
- create random values and set them to random locations in those arrays
- measure time
Conclusion: their speeds are quite similar. We can use any of them.

Attempt 9: Position indexes work for both Chess and Xiangqi/Jeiqi

Attempt 10: Calculate position indexes for Xiangqi/Jeiqi

Attempt 11: calculate 5-men Index space for chess

Attempt 12: generating all 5-men engames for chess, using the forward method.
Total time: 3 days 15 hours (generating time only). Total size: 8.88 GB, endgames: 145, files: 190.

Attempt 13: generate all 5-men endgames for chess using the backward method. Total time: 21 hours (generating time only), 4 times faster than Attempt 12.


Status
======

In development: All code has been still being written. You may use them for studying or trying to integrate into your code. However, you should not use them to generate endgames since the code has been in checking process and all could be changed seriously, from algorithms to file formats.

- Chess: it could generate all 5 men with forward/backward method. 6 men is possible but not verified yet
- Xiangqi: it could calculate index spaces
- Jeiqi: no code yet


Terms of use
============

Our code and data (egtb files) are released under the liberal [MIT license](http://en.wikipedia.org/wiki/MIT_License), so basically you can use it with almost no restrictions.


Credits
=======

Felicity Egtb was written by Nguyen Hong Pham (axchess at yahoo dot com).


