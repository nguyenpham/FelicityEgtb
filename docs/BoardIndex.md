Board Index
==========

Each chessboard position could be represented by a uniquare index/key. It is a integer number.

It is somewhat similar to hash key, in term of position representation. However, there are some differences between them:
- It is two ways. From an index we can get back the position. Hash key is one way only which cannot get back to the position
- It starts from 0 and continuously increase to the end. The space of those indexes are quite small, thus we can store efficiently in memory/hard disk. In contrast, hash keys are typical random numbers of integers 64 bits, their space is all range of 64-bit integer, so huge, make them unefficiently to store

Typically we will calculate and store DTM (distance to mate) or some similar thing for each endgame position and then store in some database files. We will use indexes of those positions to refer those values.

