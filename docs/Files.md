# File system


Source code
===========
There are three main types:
- Pure chess library. Those files have the namespace name “bslib” (those files are taken from Banksia project). The chessboard uses mailbox representation and supports variants of chess, Xiangqi, Jeiqi. It is easy to support almost all chess variants
- EGTB probe library. The folder is fegtb. The namespace is “fegtb”
- EGTB generator. The folder is fegtbgen. The namespace is “fegtb”

You should copy/integrate 1 and 2 into your chess engines or any projects you want to use our Felicity EGTB. All are designed to reduce conflict when integrating with existing chess codes. You may reuse them for chess purposes or replace them with your ones.


Endgames
========

Name
----
Each endgame has a specific name based on its material of chess pieces. Since chesss position always have two sides, those names always have two parts starting by 'k' (King). The stronger side is always on the left or the first part of the name and the weaker on the rest. For example, "krkp" is an endgame of 4 pieces, beside two Kings (k and k) there are two other pieces: a Rook (r) for one side and that is the stronger one, when the Pawn (p) is on the other side and it is the weaker one. 

We don’t create all possible endgames but ignore ones that could be probed replacely by others via some simple flipping, and mapping. For example, an endgame with a black Rook and a white Pawn is considered similar and is just flipped side with an endgame of a white Rook and a black Pawn. We keep and store the second endgame only in which the stronger side is white.

We use the name of an endgame as its file name. An endgame typically has two files, one for the white side and the other for the black side and they have .w. and .b. in their extension perspective.


Extensions
----------
- side: .w. for white, .b. for black
- extension: .fegtb for chess, .fegxq for Xiangqi, .fegjq for Jiangqi.

Examples: krnkn.w.fegtb

Folders
-------
Files of endgames could be stored in one or in multi-sub folders. Just give the loading function to the mother folder. When starting, the library will scan all the files in the main folders, including sub-folders.

When generating, the generator auto stores endgames in sub-folders, named by the numbers of their attackers, such as sub-folder 2 to store all endgames of 2 vs 0, 2-1 is 2 vs 1.

