Board index
==========

Each chessboard position could be represented by a unique index/key. It is an integer number.

It is somewhat similar to the hash key, in terms of position representation. However, there are some differences between them:
- It is two ways. From an index, we can get back the position. Hash key is one way only which cannot get back to the position
- It starts from 0 and continuously increases to the end. The space of those indexes is quite small, thus we can store them efficiently in memory/hard disk. In contrast, hash keys are typical random numbers of integers 64 bits, their space is all range of 64-bit integers, so huge, makes them inefficiently to store

Typically we will calculate and store DTM (distance to mate) or some similar thing for each endgame position and then store it in some database files. We will use indexes of those positions to refer to those values.


