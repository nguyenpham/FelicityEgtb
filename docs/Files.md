# File system


Source code
===========
There are three main types:
- Pure chess library. Those files have the namespace name “bslib” (those files are taken from Banksia project). The chessboard uses mailbox representation and supports variants of chess, Xiangqi, Jeiqi. It is easy to support almost all chess variants
- EGTB probe library. The folder is fegtb. The namespace is “fegtb”
- EGTB generator. The folder is fegtbgen. The namespace is “fegtb”

You should copy/integrate 1 and 2 into your chess engines or any projects you want to use our Felicity EGTB. All are designed to reduce conflict when integrating with existing chess codes. You may reuse them for chess purposes or replace them with your ones.

Endgames
========

Name and extensions
-------------------
Name: by all involving pieces. The stronger side is always on the left, the weaker on the right such as krnknExtension:- side:.w for white, .b for black- extension: fegtb for chess, fegxq for Xiangqi, fegjq for Jiangqi.

Examples: krnkn.w.fegtb

Folders
-------
Files of endgames could be stored in one or in multi-sub folders. Just give the loading function to the mother folder. When starting, the library will scan all the files in the main folders, including sub-folders.

When generating, the generator auto stores endgames in sub-folders, named by the numbers of their attackers.

