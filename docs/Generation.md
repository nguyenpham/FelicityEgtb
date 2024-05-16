# Generate EGTB


Endgame name
============

The generator will generate endgames one by one. The name of the endgame describes which pieces it has. "krkp" is an endgame of 4 pieces, beside of of-couse exist two King (k and k) there are two other pieces: a Rook (r) for one side and a Pawn (p) for the other side. The stronger side is always on the left/first part of the name and the weaker on the rest.

We don’t create all possible endgames but ignoring ones that could be probed replacely by others via some simple flipping, and mapping. For example, an endgame with a black Rook and a white Pawn is considered similar and is just flipped side with an endgame of a white Rook and a black Pawn. We keep and store the first endgame only.

We use the name of the endgame as its file name. An endgame typically has two files, one for the white side and the other for the black side and they have .w. and .b. in their extension perspective.

A file is containing a data array, each entry mapped by a key/index or a chess position. The array is compressed and stored by chunks.

Typically a data entry contains the value of Distance To Mate (DTM). To save size of endgames, almost all small endgames use only 1 byte to store that value. One byte can store 256 values or a range of -128 to +128. We take out first 4 values for special purposes (to mark the position is illegal, unused, missing and unknown) thus the range becomes [-126, +126]. We set 5 for draw value and use 130 as the middle point of the range. In computer chess we store typically mate scores by plies or half moves, that range equals +/- mate in 126/2 = +/-63 or [-63, +63]. However, in our case, we used full move instead, thus that range equals +/- mate in 126 or [-126, +126]. By doing that we can delay using 2 bytes for endgames, reduce a lot of size.



Folder
======

A folder is where to store that endgame. For convenience, we create and store endgames in some sub-folders, named by their attackers, such as sub-folder 2 to store all endgames of 2 vs 0, 2-1 is 2 vs 1.


Generate methods
================
There are two main methods to generate endgames:


I. Generate forward
-------------------
This is the most simple and straightforward.
Buffers and RAM
The program will create two main arrays with the size of the index space of the endgame. Those are the largest ones and the generator will work mainly with them. To make sure the generator can work as fast as possible, we allocate them all in the memory. For large endgames, they may eat all RAM and it becomes a huge challenge to create just a bit larger endgames.
When generating, some sub-endgames may be probed. To get high speed, they load their whole data into the RAM too, stretching the memory whenever lacing. However, modern operating systems nowadays are so clever that they can manage to make the program run smoothly when missing RAM a bit.
We didn’t have the right size of RAM and sizes of endgames. Just from our experience, the generator can still work well with the sizes of two main buffers take about 90% of RAM.


Initial
-------

Each entry of the array is mapped to a chess position. Not all positions are valid since some have pieces that overlap with others because of the way we index them (we can use more complicated ways to avoid being overlapped but that requires much more computing/slowing down everything). Some positions are valid for one side but invalid for the other side since the side to move can capture the opposite King. For Xiangqi/Jeiqi a position may be invalid if two Kings can see each other.

Besides checking invalid, we also find all positions are mated immediately (mate in 0) and draw.

In the initial, the generator will scan all indexes, create chess positions for them, check their validation, mate in 0 and draw. All other indexes will be set as a special value of UNSET.


Main loops
----------

We use an algorithm similar to MiniMax for one fly only. For each key and chess position, we generate all moves, make them get scores from new positions then create the best score for the position. Say, a new position is a mate in -n, which means the current position should be mate in +n + 1. If the best score is different from the current one, it will be stored.
The loop will stop if there are no changes in the last two loops.

Finish
------
When the generating is done, the program will verify all data then compress it into multi chunks and store in 2 files.

 
 
II. Generate backward
---------------------



 

Multi threading
===============
The generator is multi threading. Generation is a long and havy process, require all power and memory of computers. Users should use threads as many as possible.
The way the program divides tasks for threads are quite simple. Each thread will be given a small range of indexes to work. Thus they don't get conflicts when writting and don't need mutex locks. However, they may get conflicts when probing sub-endgames thus they need to mutex locks. Accessing values outside their index range also have some (small) chance of conflicts too when they read values being updated. However, we think that kind of conflicts are quite small and almost didnot see in practising.


Verify
======
All scores on arrays will be verified. The algorithms are somehow similar to the above probe one. The endgame is considered good and be saved only if it passes the verification step


Compress
========

Data will be devised compressed and stored in small chunks. Original chunks have a fixed size of 4 Kb. After compressing, their sizes are smaller and not the same. When saving to disk, to know what size and where location of each chunk we need to store that info in a compressed table.
