# Board index


Index
=====
Each chessboard position on endgames could be represented by a unique index/key. It is a (64-bit) integer number.

For a given chess position we can calculate its index from sub-indexes of its chess pieces. In the simplest way, we can use the location of a piece as a sub-index. For example, in chess, a King can stay in any cell from 0 to 63 and we use that number as its sub-index:

```
 0,  1,  2,  3,  4,  5,  6,  7,
 8,  9, 10, 11, 12, 13, 14, 15,
16, 17, 18, 19, 20, 21, 22, 23,
24, 25, 26, 27, 28, 29, 30, 31,
32, 33, 34, 35, 36, 37, 38, 39,
40, 41, 42, 43, 44, 45, 46, 47,
48, 49, 50, 51, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61, 62, 63
```

When adding one more piece we can use a similar way to assign a sub-index for the added one. For example, when adding a Rook (r), it has a sub-index in the range of [0, 63] too.

From the sub-index of pieces, we can calculate the combined index or index of the endgame.

For example, the endgame "krk" has k0, r0, k1 are positions of the first king, the rook and the second king. They become sub-indexes and the index of the position is:

```index = k0 * 64 * 64 + r0 * 64 + k1```

The chess Pawns have a limited area to display on the chessboard. They should be on 6 middle ranks but the first and the last ranks. That means Pawns can be located on 48 cells and their index should be in the range of [0, 47].

Xiangqi and Jeiqi can index their pieces in the simplest and similar way, with the range of [0, 89] for non-Pawn pieces, [0, 54] for Pawns, [0, 8] for Kings, [0, 4] for Advisors, and [0, 6] for Elephants.

Index space
===========
The index space of an endgame is the maximum number of its index. For the simplest way (above) of calculating indexes, the endgame "krk" has the max number:

```index space = 64 * 64 * 64 = 262144```

The total number of all index spaces of all endgames in an EGTB is an index space of that EGTB.


Reduce size
===========
Popular EGTBs have huge sizes. Reducing their sizes are the most important factor of their success since smaller on sizes can get much easier to generate, using cheaper hardware, easier to have more endgames, easier to contribute, users get easier to download and store on their computers.

There are some ways to reduce sizes:
- Reduce the size of each item
- Reduce index spaces, of individual endgames and EGTB as whole
- Increase the compress ratios


Symmetries
==========

The simple way to calculate index space creates huge numbers. To reduce index spaces, we use some symmetries.

1. Chess symmetries
===================

8 fold
------
In chess, for non-Pawn endgames, we can use 8-fold symmetry. The chess positions will apply some flips/rotates to make sure the first (stronger or white side) King stays on a specific triangle and will be indexed from 0 to 9 (10 numbers) as below:

```
     0, 1, 2, 3, -1,-1,-1,-1,
    -1, 4, 5, 6, -1,-1,-1,-1,
    -1,-1, 7, 8, -1,-1,-1,-1,
    -1,-1,-1, 9, -1,-1,-1,-1,

    -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1
```

Index space now become `10 * 64 * 64 = 40960` or 64/10 = 6.4 time smaller.

Horizontal flip
---------------
For Pawn-endgame we will use a horizontal flip to make sure the strong King is always on the left side with indexed from 0 to 31 (32 numbers)

King combinations
-----------------
In 8-fold symmetry as above, two Kings create a range = 10 * 64 = 640. We may reduce further by combining them, considering that they should be not overlapped nor close (attackable) to each other. The size of their index space now is 564.

Similar pieces
--------------
Similar pieces, say two white Rooks, could combine, and swape their positions to reduce their index spaces.

For example, for two similar pieces, when using the simple way, their range is 64 * 64 = 4096. When combined, their range is 2016, half size only.


2. Xiangqi and Jeiqi symmetries
===============================


Defender sub-indexes
--------------------
All defence pieces are located in a small area (King Place), and their sub-indexes are small numbers.

Defender combinations
---------------------
Defenders could block each other, reducing their ranges of index spaces significantly.

Similar pieces
--------------
They could combine to reduce their ranges of index spaces as chess one.

Horizontal flip
---------------
Xiangqi board is always horizontal symmetry. We could flip horizontally the board to make sure the King of the stronger side is always on the left half board.

When calculating sub-indexes, we should consider horizontal symmetry. We may assign that factor to Kings or Defender. For example, for King only, the new King is one 6 cells, instead of 9 cells. That is a save of 9/6 = 1.5 times.

If we assign horizontal symmetry for the first attacker, that attacker is on 50 cells instead of 90, a save of 90/50 = 1.8 times, larger than the above one. That is the reason we use this way.



Indexes vs hash keys
====================

It is somewhat similar to the hash key, in terms of position representation. However, there are some differences between them, when indexing:
- is guaranty unique. Two similar or different (on terms of an endgame) chess positions will have the same index or different indexes, respective. However, two different chess positions may create the same hash key event it is very rare
- is two ways. From an (endgame) index, we can get back the chess position. Hash key is one way only which cannot get back to the position
- starts from 0 and continuously increases to the end. The space or range of those indexes is quite small, thus we can store them efficiently in memory/hard disk. In contrast, hash keys are typical random numbers of 64-bit integers, their space is all range of 64-bit integers, so huge, makes them inefficiently to store or use as references/pointers


Database pointers
=================
We will calculate and store DTM (distance to mate) or some similar thing for each endgame position and then store it in some database files. We will use indexes of those positions to refer or point to those values.



