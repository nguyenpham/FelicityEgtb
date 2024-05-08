#  File system


Source code
===========

There are three main types:
- Pure chess library. Those files have name space name “bslib” (those files are taken from Banksia project). The chessboard is used mailbox representation and supports variants chess, Xiangqi, Jeiqi. It is easy to support almost all chess variants.
- EGTB probe library. The folder is fegtb. The name space is “fegtb”

= EGTB generator. The folder is fegtbgen. The name space is “fegtb”


You should copy/integrate 1 and 2 into your chess engines or any projects you want to use our Felicity EGTB. All are designed to reduce conflict when integrating with existing chess codes.
You may reuse them for chess purposes or replace by your own ones.


Endgames
========

Name and extensions
-------------------
Name: by all involving pieces. The stronger side always on the left, the weaker on the right such as krnkn
Extension:
- side:.w for white, .b for black
- extenstion: fegtb for chess, fegxq for Xiangqi, fegjq for Jiangqi

Examples: krnkn.w.fegtb

Folders
-------
Files of endgames could be stored in one or in multi-sub folders. Just give the loading function the mother folder. When starting, the library will scan all the files in the main folders, including sub-folders.

When generating, the generator auto stores endgames in sub-folders, named by the numbers of their attackers.

