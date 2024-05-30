# Board index


Index
=====
Each chessboard position on endgames could be represented by a unique index/key. It is an (64-bit) integer number.

For a given chess position we can calculate its index from sub-indexes of its chess pieces. In the simplest way we can use the location of a piece as a sub-index. For example, in chess, a King can stay in any cell from 0 to 63 and we use that number as its sub-index:

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

When adding one more piece we can use similar way to assign sub-index for the added one. For example, when add a Rook (r), it has sub-index in the range of [0, 63] too.
From sub-index of pieces, we can calculate the combined index or index of the endgame.

For example, the endgame krk has k0, r0, k1 are positions of first king, the rook and the second king. They becomes sub-indexes and the index of the position is:

```index = k0 * 64 * 64 + r0 * 64 + k1```

The chess Pawns have a limited area to display on the chess board. They should be on 6 middle ranks but the first and the last ranks. That means Pawns can be located on 48 celss and their index should be in range of [0, 47].

Xiangqi and Jeiqi can index their pieces in the simplest way, with range [0, 89] for non-Pawn, [0, 54] for Pawns.

Index space
===========
Index space of an endgame is the max number of its index. For above simple way of calculating indexes, the endgame "krk" has the max number is:

```index space = 64 * 64 * 64 = 262144```

The total number of all index spaces of all endgames in an EGTB is index space of that EGTB.


Symmetries
==========

The simple way to calculate index space creates huge numbers. To reduce index spaces, we use some symmetries.

1. Chess symmetries
-------------------

8 fold
------
In chess, for non-Pawn endgames we can use 8-fold symmetry. The chess positions will be applied some flips/rotates to make sure first (stronger or white side) King stays on a specific triangle and will be index from 0 to 9 (10 numbers) only:

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
For Pawn-endgame we will use horizontal flip to make sure the strong King is always on the left side with indexed from 0 to 31 (32 numbers)

King combinations
-----------------
In 8-fold symmetry as above, two Kings create a range = 10 * 64 = 640. We may reduce furthere by combining them, considering that they should be not overlapped nor close (attackable) each other. The size of their index space now is 564.

Similar pieces
--------------
From two similar pieces, say two white Rooks, could combine, swape their positions to recude their index spaces.

For example, for two similar pieces, if we calculate in the simple way, their range is 64 * 64 = 4096. When combining, their range is 2016, half only.


2. Xiangqi and Jeiqi symmetries
-------------------------------

Defender sub-indexes
--------------------
All defend pieces are located in a small area (King place), their sub-indexes are small numbers.

Defender combinations
---------------------
Defenders could block each other, make their ranges of index spaces reduce significant.

Similar pieces
--------------
They could combine to reduce their ranges of index spaces as chess one.


Indexes vs hash keys
====================

It is somewhat similar to the hash key, in terms of position representation. However, there are some differences between them:
- It is two ways. From an (endgame) index, we can get back the chess position. Hash key is one way only which cannot get back to the position
- It starts from 0 and continuously increases to the end. The space or range of those indexes is quite small, thus we can store them efficiently in memory/hard disk. In contrast, hash keys are typical random numbers of 64-bit integers, their space is all range of 64-bit integers, so huge, makes them inefficiently to store or being used as referencies/pointers


Database pointers
=================
We will calculate and store DTM (distance to mate) or some similar thing for each endgame position and then store it in some database files. We will use indexes of those positions to refer or to point to those values.


