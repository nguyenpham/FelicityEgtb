Felicity EGTB - Generation and probe code for chess, Xiangqi and Jeiqi
==============


Overview
--------
Felicity EGTB (Endgame database/tablebase) is an opensource for generating and probing EGTB. All code is  written in C++ (using standad C++17 library). It supports some chess variants: chess, Xiangqi, Jeiqi.

From testing, EGTB doesn't help chess engines much, in terms of strength/Elo gaining. 6 men-Syzygy EGTB can help Stockfish to gain < 13 Elo.
In contrast, we believe EGTB can help much more for Xiangqi/Jeiqi engines because the endgames of those chess variants are much more complicated, with a lot of exceptions/tricky wins, which are much harder for human/program to learn. However, we don't have real statistics. We need to build some good EGTB first before measuring.
This project focuses on Xiaqqi and Jeiqi. But we will support chess too, for testing, learning and comparing.

Restart
--------
April 2024: restart the project. Plan to rewrite all. Support chess, Xiangqi, Jeiqi

Research
--------
This is a long journey, to find good code, algorithmns.


History
--------


Terms of use
---------------

Our code and data (egtb files) are released under the liberal [MIT license](http://en.wikipedia.org/wiki/MIT_License), so basically you can use it with almost no restrictions.


Credits
--------

Felicity Egtb was written by Nguyen Hong Pham (axchess at yahoo dot com).


