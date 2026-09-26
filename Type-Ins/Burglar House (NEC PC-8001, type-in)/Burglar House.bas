10 '+---------------+
20 '| BURGLAR HOUSE |
30 '|  1983/ 4/19   |
40 '|   T.SHIMAYA   |
50 '+---------------+
100 CONSOLE 0,25,1,0:WIDTH 40,25
110 CLEAR 100,&H8FFF:DEFUSR=&H9000
120 KEY 1,"ﾐﾙ"+CHR$(13)
130 KEY 2,"ﾋｶﾞｼ"+CHR$(13)
140 KEY 3,"ﾐﾅﾐ"+CHR$(13)
150 KEY 4,"ﾆｼ"+CHR$(13)
160 KEY 5,"ｷﾀ"+CHR$(13)
170 KEY 6,"ｱｹﾙ"+CHR$(13)
180 KEY 7,"ｻｶﾞｽ"+CHR$(13)
190 KEY 8,"ﾄﾙ"+CHR$(13)
200 KEY 9,"ｳｴ"+CHR$(13)
210 KEY10,"ｼﾀ"+CHR$(13)
220 AA=USR(0)
230 END
