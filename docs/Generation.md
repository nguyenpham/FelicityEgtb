# Generate EGTB


The generator will generate endgames one by one. A file contains a data array, each entry mapped by a key/index or a chess position. The array is compressed and stored in chunks.

Entry size
==========
Theoretically, we can store what we want with the data array. However, due to the hugeness of the array (because the index spaces may be so huge), we try to store as small as possible for each entry. Typically, we store on an entry the score of Distance To Mate (DTM). Depending on endgames we need 1 or 2 bytes for an entry.

Score
-----
Typically in computer chess scores have value ranges fit in two bytes (16 bits). Our score range is similar to the range [-1000, +1000].


One byte
--------
One byte can store 256 values or a range of -128 to +128. We take out the first 4 values for special purposes (to mark the position as illegal, unused, missing and unknown) thus the range is sunk into [-126, +126]. We use the number 5 for draw value and use 130 as the middle point of the range. In computer chess we store typically mate scores by plies or half moves, that range equals mate in +/-126/2 = +/-63 or [-63, +63]. However, in our case, we used full move instead, thus that range expanded back and was mated in +/- 126 or [-126, +126]. By doing that we can delay using 2 bytes for endgames, and reduce a lot of size.

We need to convert scores (of two bytes) into the values of 1 byte to store or load.

Two bytes
---------
Thus those scores are stored straightforwardly in data entries, without any converting. Using two bytes is faster when generating. However, it uses more and may create more stress on memory unnecessarily.

After all data is generated, before compressing, the program will scan all data to verify if two bytes are really necessary not not. If not, it will convert data back into 1 byte to save space.



Buffers and RAM
================

The program will create two main arrays with the size of the index space of the endgame. Those are the largest ones and the generator will work mainly with them. To make sure the generator can work as fast as possible, we allocate them all in the memory. For large endgames, they may eat all RAM and it becomes a huge challenge to create just a bit larger endgames.

When generating, some sub-endgames may be probed. To get high speed, they load their whole data into the RAM too, stretching the memory whenever lacing. However, modern operating systems nowadays are so clever that they can manage to make the program run smoothly when missing RAM a bit.

We didn’t have the right size of RAM and sizes of endgames. Just from our experience, the generator can still work well with the sizes of two main buffers take about 90% of RAM.

Each entry of the array is mapped to a chess position. Not all positions are valid since some have pieces that overlap with others because of the way we index them (we can use more complicated ways to avoid being overlapped but that requires much more computing/slowing down everything). Some positions are valid for one side but invalid for the other side since the side to move can capture the opposite King. For Xiangqi/Jeiqi a position may be invalid if two Kings can see each other.


Generate
========

There are two main methods to generate endgames:


I. Generate forwarding
----------------------
This is the most simple and straightforward.


Initial
-------

In the initial, the generator will scan all indexes, create chess positions for them, check if it is invalid, mate in 0 and draw. All other indexes will be set as a special value of UNSET.


Main loops
----------

We use an algorithm similar to MiniMax for one fly only. For each key and chess position, we generate all moves, make them get scores from new positions then create the best score for the position. Say, a new position is a mate in -n, which means the current position should be mate in +n + 1. If the best score is different from the current one, it will be stored.

The loop will stop if there are no changes in the last two loops.

 
II. Generate backward
------------------------

Backward move generator 
------------------------

In this method, we will use a backward/retro move generator. From a given chess position the backward move generator will create a list of all moves to reach that position. Those moves will lead to be back of all parent positions. Theoretically, those parents could differ on materials with the given position because of captures or promotions.

However, we don’t need a full backward move generator but a simplified version or a “quiet” one that ignores all capture and promotion moves. It means all parent positions of the given position will have the same material as the given one.


Bit flag buffers
----------------

We will allocate an extra 4 bits (half a byte) per index/key. Those bits are devised by 2 for each side and work as marks for special purposes. Typically we allocate 2 bytes per index (one for White and one for Black sides). Thus the allocated memory will increase by 25%. However, we will save some memory later.

Initial
-------

Similar to the forwarding method, for each index we will check if its position is valid or not, as well as if one side is mated or the game is a draw.

The extra work is that if the given position has some captures or promotions, we will probe all sub-endgames, get the best score and write down the score buffers. Then we mark the index as a capture.

Main loops
----------
We loop with a variant ply from 0 and increase one after a loop. A ply is associated with a mate score alternately between winning and losing. Ply 0 is mate in 0, ply 1 is mate in +1, ply 2 is mate in -2 (plies). For each loop, we don't compute with all positions but only ones whose scores are associated with the current ply.

We start from the index with a mate in 0 (ply), create their boards, generate “quiet” backward/retro moves, reach their parents’ positions and fill their parents’ indexes with the value mate in 1 (ply). Those parents can always make the move to that given position to win a mate in +1, regardless of other moves.

In the next loop, the value to fill should be mated in -2 (plies). We reach their parents’ positions in the above way. However, we can’t find immediate mate in -2 for those parents since those moves are losing but their parents may have other moves as better choices, say to be drawn or even win back. Instead, we will probe those parents’ positions, and get their scores based on their children's scores. That probe function is somehow similar to the one of the forwarding method. However, we have a special bit flag to mark if a position has captured moves and we have probed already all sub-end games thus we don’t need to probe again and again, saving some computing and time for that.


The redundancy of board symmetry
--------------------------------
We use 8-fold symmetry to reduce index space for none-Pawn endgames. That kind of symmetry can cause redundancy when the white King is on one of two middle diagonal lines. Two different positions that symmetries each other via that line may have different indexes. Because of our symmetries, they should have the same score. When working with the forwarding method, we don’t have any problem with that kind of symmetry position since the algorithm will scan multiple times to make sure everything is up to date. However, in this method, we update a winning position only once when one of its children has the right mating score. The problem the retro move generator can’t reach some symmetry positions, leading to not being up to date in time. We solved the problem by detecting those positions and updating them.


Finish
======

When the generating is done, the program will verify all data then compress it into multiple chunks and store it in 2 files by sides.


Verify
======
All scores on arrays should be verified. That function checks the consistency: the score of a given position should be consistent with the scores of its children. The algorithm works like a minimax with one ply only. The endgame is considered good and be saved only if it passes the verification step. If not, the generator will print error message and stop immediately.


Compress
========

Data will be compressed and stored in small chunks. Original chunks have a fixed size of 4 Kb. After compressing, their sizes are smaller and not the same size. When saving to disk, to know what size and where location of each chunk we need to store that info in a compressed table.


Multithreading
==============
The generator is multi-threading. Generation is a long and heavy process, requiring all the power and memory of computers. Users should use threads as many as possible.

The way the program divides tasks for threads is quite straightforward. Each thread will be given a small chunk of main arrays to work. Thus they don't get conflicts when writing and don't need mutex locks. However, they may get into conflicts when probing sub-endgames thus they need to mutex locks. Accessing values outside their index range also has some (small) chance of conflicts when they read values being updated. However, we think that these kinds of conflicts are quite small and almost not seen in practice (any wrong on generating will be detected by the verification step).


