#  Chess Index

Below is the table of all endgames of 5 men for chess. They are a total of 149 endgames and their index space is 21 G. Suppose we store information of one index within 1 byte and we need to store information of both sides, total size = 21 G x 2 sides x 1 byte = 42 GB. Suppose we compress all endgames with a ratio of 4 times (0.25), the size will be 42 x 0.25 = 10.5 GB (over 10 GB).

Compared with current existing EGTBs, our one (uncompressed 42 GB) is better than Edward's (uncompressed, 56 GB estimated), but it is worse, compared with Gaviota (6.5 GB, vs 8.75 GB). Especially, it is not on the same rank to be compared with Syzygy 5 men (0.9 GB)!

However, our size is just an estimate, we hope the real compress ratio is a bit better or be improved. We may reduce the size but omit some unnecessary endgames too (such as kqqqk, krrrk…).

The largest endgames are just 355 M (kqbkp). They will take about 710 MB if we allocate in memory for 2 sides, using only 1 byte to store information for each index. That size is fine to store in the memory of modern computers. The total size of 9 GB is fine too for downloading and storing.

All interesting 5 men endgames (order - name - index size)
  1)              knk           36'096
  2)             knkn        2'310'144
  3)              kbk           36'096
  4)             kbkn        2'310'144
  5)             kbkb        2'310'144
  6)              krk           36'096
  7)             krkn        2'310'144
  8)             krkb        2'310'144
  9)             krkr        2'310'144
 10)            krknn       72'769'536
 11)              kqk           36'096
 12)             kqkn        2'310'144
 13)             kqkb        2'310'144
 14)             kqkr        2'310'144
 15)            kqknn       72'769'536
 16)            kqkbn      147'849'216
 17)            kqkbb       72'769'536
 18)            kqkrn      147'849'216
 19)            kqkrb      147'849'216
 20)            kqkrr       72'769'536
 21)             kqkq        2'310'144
 22)             knnk        1'137'024
 23)            knnkn       72'769'536
 24)            knnkb       72'769'536
 25)            knnkr       72'769'536
 26)             kbnk        2'310'144
 27)            kbnkn      147'849'216
 28)            kbnkb      147'849'216
 29)            kbnkr      147'849'216
 30)             kbbk        1'137'024
 31)             krnk        2'310'144
 32)            kbbkn       72'769'536
 33)            krnkn      147'849'216
 34)            krnkb      147'849'216
 35)            kbbkb       72'769'536
 36)            kbbkr       72'769'536
 37)            krnkr      147'849'216
 38)             krbk        2'310'144
 39)            krbkn      147'849'216
 40)            krbkb      147'849'216
 41)            krbkr      147'849'216
 42)             krrk        1'137'024
 43)            krrkn       72'769'536
 44)            krrkb       72'769'536
 45)            krrkr       72'769'536
 46)             kqnk        2'310'144
 47)            kqnkn      147'849'216
 48)            kqnkb      147'849'216
 49)            kqnkr      147'849'216
 50)            kqnkq      147'849'216
 51)             kqbk        2'310'144
 52)            kqbkn      147'849'216
 53)            kqbkb      147'849'216
 54)            kqbkr      147'849'216
 55)            kqbkq      147'849'216
 56)             kqrk        2'310'144
 57)            kqrkn      147'849'216
 58)            kqrkb      147'849'216
 59)            kqrkr      147'849'216
 60)            kqrkq      147'849'216
 61)             kqqk        1'137'024
 62)            kqqkn       72'769'536
 63)            kqqkb       72'769'536
 64)            kqqkr       72'769'536
 65)            kqqkq       72'769'536
 66)            knnnk       23'498'496
 67)            kbnnk       72'769'536
 68)            kbbnk       72'769'536
 69)            krnnk       72'769'536
 70)            kbbbk       23'498'496
 71)            krbnk      147'849'216
 72)            krrnk       72'769'536
 73)            krbbk       72'769'536
 74)            krrbk       72'769'536
 75)            krrrk       23'498'496
 76)            kqnnk       72'769'536
 77)            kqbnk      147'849'216
 78)            kqrnk      147'849'216
 79)            kqbbk       72'769'536
 80)            kqrbk      147'849'216
 81)            kqrrk       72'769'536
 82)            kqqnk       72'769'536
 83)            kqqbk       72'769'536
 84)            kqqrk       72'769'536
 85)            kqqqk       23'498'496
 86)              kpk           86'688
 87)             knkp        5'548'032
 88)             kbkp        5'548'032
 89)            kbknp      355'074'048
 90)             krkp        5'548'032
 91)            krknp      355'074'048
 92)            krkbp      355'074'048
 93)             kqkp        5'548'032
 94)            kqknp      355'074'048
 95)            kqkbp      355'074'048
 96)            kqkrp      355'074'048
 97)             knpk        5'548'032
 98)            knpkn      355'074'048
 99)            knpkb      355'074'048
100)             kbpk        5'548'032
101)            knnkp      174'763'008
102)            kbpkn      355'074'048
103)            kbpkb      355'074'048
104)            kbpkr      355'074'048
105)             krpk        5'548'032
106)            kbnkp      355'074'048
107)            krpkn      355'074'048
108)            krpkb      355'074'048
109)            krpkr      355'074'048
110)            krnkp      355'074'048
111)            kbbkp      174'763'008
112)            krbkp      355'074'048
113)            krrkp      174'763'008
114)             kqpk        5'548'032
115)            kqpkn      355'074'048
116)            kqpkb      355'074'048
117)            kqpkr      355'074'048
118)            kqpkq      355'074'048
119)            kqnkp      355'074'048
120)            kqbkp      355'074'048
121)            kqrkp      355'074'048
122)            kqqkp      174'763'008
123)            knnpk      174'763'008
124)            kbnpk      355'074'048
125)            kbbpk      174'763'008
126)            krnpk      355'074'048
127)            krbpk      355'074'048
128)            krrpk      174'763'008
129)            kqnpk      355'074'048
130)            kqbpk      355'074'048
131)            kqrpk      355'074'048
132)            kqqpk      174'763'008
133)             kpkp        4'161'024
134)            knkpp      130'378'752
135)            kbkpp      130'378'752
136)            krkpp      130'378'752
137)            kqkpp      130'378'752
138)             kppk        2'037'168
139)            kppkn      130'378'752
140)            knpkp      266'305'536
141)            kbpkp      266'305'536
142)            krpkp      266'305'536
143)            kqpkp      266'305'536
144)            knppk      130'378'752
145)            kbppk      130'378'752
146)            krppk      130'378'752
147)            kqppk      130'378'752
148)            kppkp       97'784'064
149)            kpppk       31'236'576

Total files: 149, total size: 20'854'389'552


