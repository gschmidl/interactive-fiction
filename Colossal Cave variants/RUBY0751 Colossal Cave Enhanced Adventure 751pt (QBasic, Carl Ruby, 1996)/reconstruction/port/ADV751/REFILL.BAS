0 DIM DI(247, 10), I1(71), B$(141), CO(12),BB$(15), P$(100), A$(44),I$(108)
20 FOR X = 1 TO 246: FOR Y = 1 TO 10
22 READ DI(X, Y)
25 NEXT Y, X
29 FOR X = 1 TO 141: READ B$(X): NEXT X
32 FOR X = 1 TO 86: READ P$(X): NEXT X
33 FOR X = 1 TO 10: READ MZ$(X): NEXT X
34 FOR X=1 TO 12:READ CO(X):NEXT X
35 FOR X=1 TO 4:READ WUMP$(X):NEXT X
40 FOR X=2 TO 9:READ I$(X):NEXT X
41 FOR X=13 TO 23:READ I$(X):NEXT X
42 FOR X=25 TO 44:READ I$(X):NEXT X
43 FOR X=46 TO 61:READ I$(X):NEXT X:READ I$(63):READ I$(64)
44 FOR X=66 TO 71:READ I$(X):NEXT X
45 FOR X=73 TO 75:READ I$(X):NEXT X
46 GOSUB 28000
50 N$ = "VAR"
100 FOR X = 0 TO 15: PRINT X; BB$(X): NEXT X
260 OPEN N$ FOR OUTPUT AS #1
267 FOR X = 1 TO 246: FOR Y = 1 TO 10
268 PRINT #1, DI(X, Y)
269 NEXT Y, X
277 FOR X = 1 TO 141: PRINT #1, B$(X): NEXT X
280 FOR X = 1 TO 86: PRINT #1, P$(X): NEXT X
285 FOR X = 1 TO 10: PRINT #1, MZ$(X): NEXT X
300 FOR X = 0 TO 15: PRINT #1, BB$(X): NEXT X
320 FOR X = 0 TO 3: PRINT #1, TR$(X): NEXT X
330 PRINT #1, TH$
340 PRINT #1, NH$
380 FOR X = 1 TO 4: PRINT #1, HLTH$(X): NEXT X
381 FOR X = 1 TO 4: PRINT #1, H2$(X): NEXT X
382 FOR X = 1 TO 5: PRINT #1, A$(X): NEXT X
383 PRINT #1, A$(8)
384 FOR X = 10 TO 13: PRINT #1, A$(X): NEXT X
386 FOR X = 15 TO 17: PRINT #1, A$(X): NEXT X
387 FOR X = 21 TO 27: PRINT #1, A$(X): NEXT X
388 FOR X = 29 TO 32: PRINT #1, A$(X): NEXT X
389 FOR X = 34 TO 39: PRINT #1, A$(X): NEXT X
390 FOR X=1 TO 12:PRINT #1, CO(X):NEXT X
391 FOR X=1 TO 4:PRINT #1, WUMP$(X):NEXT X
392 FOR X=2 TO 9:PRINT #1,I$(X):NEXT X
393 FOR X=13 TO 23:PRINT #1,I$(X):NEXT X
394 FOR X=25 TO 44:PRINT #1,I$(X):NEXT X
395 FOR X=46 TO 61:PRINT #1,I$(X):NEXT X:PRINT #1,I$(63):PRINT #1,I$(64)
396 FOR X=66 TO 71:PRINT #1,I$(X):NEXT X
397 FOR X=73 TO 75:PRINT #1,I$(X):NEXT X
400 CLOSE #1
9998 PRINT "Reload complete."
9999 SYSTEM
20000 REM DIRECTIONS
20001 DATA 3,4,121,7,3,7,0,0,0,0
20002 DATA 0,0,0,1,0,0,0,0,0,0
20003 DATA 122,3,4,3,4,0,0,0,0,0
20004 DATA 1,5,3,3,5,3,0,0,0,0
20005 DATA 4,8,3,3,502,0,0,0,0,0
20006 DATA 1,3,4,4,4,0,0,0,0,0
20007 DATA 1,3,1,123,1,0,0,0,0,0
20008 DATA 5,3,3,3,9,0,0,0,0,0
20009 DATA 0,0,0,10,0,8,0,0,0,0
20010 DATA 0,0,9,47,0,0,0,0,0,0
20011 DATA 0,0,0,24,0,0,43,0,0,0
20012 DATA 26,73,0,0,0,0,0,0,0,0
20013 DATA 0,0,77,14,77,0,0,0,0,0
20014 DATA 0,0,13,505,15,0,0,0,0,0
20015 DATA 23,16,0,17,23,14,0,0,0,0
20016 DATA 15,0,0,0,0,0,0,0,0,0
20017 DATA 0,0,15,518,518,0,0,0,0,0
20018 DATA 83,68,83,28,0,0,0,0,0,0
20019 DATA 0,0,23,79,74,0,0,0,0,0
20020 DATA 70,69,0,0,0,0,0,0,0,0
20021 DATA 88,75,0,0,0,0,0,0,0,0
20022 DATA 75,0,0,0,0,0,0,0,0,0
20023 DATA 65,64,15,67,0,15,170,0,0,19
20024 DATA 0,0,11,0,0,0,0,48,0,0
20025 DATA 0,65,71,30,0,0,0,0,0,0
20026 DATA 0,12,0,0,0,0,0,0,0,0
20027 DATA 0,0,0,0,60,0,0,0,0,0
20028 DATA 70,231,18,20,70,18,0,0,0,0
20029 DATA 0,0,66,31,0,65,0,0,0,0
20030 DATA 25,0,0,0,0,0,0,0,0,0
20031 DATA 0,0,29,0,32,65,0,0,0,0
20032 DATA 60,0,61,76,0,31,0,0,0,0
20033 DATA 0,0,34,36,0,0,76,35,0,0
20034 DATA 0,0,0,33,0,0,0,0,0,0
20035 DATA 48,0,82,0,0,0,0,0,33,0
20036 DATA 0,0,33,37,38,0,0,0,0,0
20037 DATA 0,0,36,40,39,0,0,0,0,0
20038 DATA 0,0,0,0,0,36,0,0,0,0
20039 DATA 0,0,0,0,0,37,0,0,0,0
20040 DATA 0,37,0,0,0,73,0,0,0,0
20041 DATA 0,0,39,42,0,0,0,0,0,0
20042 DATA 63,41,87,0,0,0,0,0,0,0
20043 DATA 0,11,0,0,0,0,0,0,0,0
20044 DATA 45,63,0,45,208,0,0,0,0,0
20045 DATA 44,0,0,0,82,0,0,0,0,0
20046 DATA 107,113,108,105,13,0,0,0,0,0
20047 DATA 0,0,10,77,0,77,0,0,0,0
20048 DATA 0,35,0,24,0,0,181,0,0,522
20049 DATA 0,0,0,0,82,50,0,0,0,0
20050 DATA 0,0,0,0,0,0,51,0,0,49
20051 DATA 0,0,0,239,0,0,52,0,0,50
20052 DATA 0,0,53,51,0,0,0,0,0,0
20053 DATA 0,0,0,52,55,0,58,0,55,0
20054 DATA 0,0,0,58,0,0,0,0,0,0
20055 DATA 0,0,0,0,56,53,0,0,0,0
20056 DATA 0,0,59,0,0,55,0,0,0,0
20057 DATA 0,58,0,0,515,0,0,0,0,0
20058 DATA 57,53,54,0,0,0,0,0,0,0
20059 DATA 0,0,0,56,0,0,0,0,0,0
20060 DATA 0,32,0,0,81,27,0,0,0,0
20061 DATA 0,0,62,76,0,32,0,0,0,0
20062 DATA 61,61,61,514,61,61,61,61,61,61
20063 DATA 44,42,0,0,0,0,0,0,0,0
20064 DATA 23,0,0,0,0,0,0,0,0,0
20065 DATA 25,23,0,0,29,0,0,0,0,0
20066 DATA 0,0,0,29,80,0,0,0,0,0
20067 DATA 0,0,23,70,23,70,0,0,0,0
20068 DATA 0,0,101,0,0,18,0,0,0,0
20069 DATA 91,90,97,92,20,94,95,93,98,96
20070 DATA 294,20,67,28,67,28,0,0,0,0
20071 DATA 0,0,0,0,25,15,0,0,0,0
20072 DATA 0,0,0,0,0,81,0,0,0,0
20073 DATA 12,79,0,0,40,0,0,0,0,0
20074 DATA 78,86,0,0,0,0,0,0,0,0
20075 DATA 21,22,0,0,76,0,0,0,0,0
20076 DATA 0,0,32,33,0,75,0,0,0,0
20077 DATA 0,0,47,13,47,13,0,0,0,0
20078 DATA 33,0,85,0,0,0,0,0,0,0
20079 DATA 73,0,19,0,0,0,0,0,0,0
20080 DATA 0,0,0,0,502,66,0,0,0,0
20081 DATA 0,0,0,0,72,60,0,0,0,0
20082 DATA 84,0,0,0,0,0,49,0,0,49
20083 DATA 18,0,518,18,0,0,0,0,0,0
20084 DATA 0,82,0,0,0,0,0,0,0,0
20085 DATA 78,0,0,0,0,0,0,0,0,0
20086 DATA 0,0,0,74,0,0,0,0,0,0
20087 DATA 42,0,0,0,0,0,0,0,0,0
20088 DATA 0,0,0,21,0,0,0,0,0,0
20089 DATA 0,0,0,0,0,0,0,0,108,0
20090 DATA 92,91,99,69,97,93,94,95,96,98
20091 DATA 98,92,96,95,94,97,0,90,93,69
20092 DATA 94,93,90,98,69,95,0,99,97,0
20093 DATA 99,94,69,90,98,96,0,97,0,0
20094 DATA 69,95,98,93,96,99,0,0,0,0
20095 DATA 97,99,93,96,90,69,0,0,0,0
20096 DATA 95,98,92,94,91,90,0,0,0,0
20097 DATA 93,96,91,99,95,98,0,0,0,0
20098 DATA 90,97,94,91,93,92,0,0,0,0
20099 DATA 96,295,95,97,92,91,0,0,0,0
20100 DATA 104,118,0,109,0,0,0,0,0,0
20101 DATA 0,102,11,68,0,0,0,0,0,0
20102 DATA 116,103,101,0,117,0,0,0,0,0
20103 DATA 0,104,102,106,109,0,0,0,0,0
20104 DATA 105,104,109,103,115,100,0,0,0,0
20105 DATA 105,0,46,104,114,0,0,0,0,0
20106 DATA 0,0,106,109,0,0,0,0,0,0
20107 DATA 0,46,108,119,0,0,0,0,0,0
20108 DATA 46,0,0,107,0,0,0,89,0,0
20109 DATA 0,100,104,106,0,103,0,0,0,0
20110 DATA 68,110,222,110,0,0,0,0,0,0
20111 DATA 101,240,241,68,112,112,0,0,0,0
20112 DATA 0,0,0,0,111,111,0,0,0,0
20113 DATA 0,0,46,0,0,0,0,0,0,0
20114 DATA 0,0,0,0,0,105,0,0,0,0
20115 DATA 0,0,0,0,0,104,0,0,0,0
20116 DATA 0,102,0,0,0,0,0,0,0,0
20117 DATA 0,0,0,0,0,102,0,0,0,0
20118 DATA 0,0,0,100,0,0,0,0,0,0
20119 DATA 0,0,107,0,0,0,0,0,0,0
20120 DATA 0,0,0,0,0,0,0,0,0,0
20121 DATA 0,0,0,1,0,0,0,0,0,0
20122 DATA 124,6,4,0,4,0,0,0,0,0
20123 DATA 127,6,7,130,0,7,0,0,0,0
20124 DATA 128,122,122,127,0,0,0,0,0,0
20125 DATA 129,0,0,138,0,0,0,0,0,126
20126 DATA 138,135,127,296,0,0,125,0,0,0
20127 DATA 135,130,135,135,0,0,0,0,0,0
20128 DATA 129,124,128,125,0,0,0,0,0,0
20129 DATA 0,128,139,125,0,0,0,0,510,510
20130 DATA 135,0,123,131,0,0,0,0,0,0
20131 DATA 132,131,130,0,0,0,0,0,0,0
20132 DATA 133,131,0,0,134,133,0,0,0,0
20133 DATA 0,132,0,0,132,0,0,0,0,0
20134 DATA 0,132,137,0,0,132,0,0,0,0
20135 DATA 291,135,126,136,0,0,0,0,0,0
20136 DATA 138,126,135,135,0,0,0,0,0,0
20137 DATA 0,0,0,134,517,134,0,0,0,0
20138 DATA 0,225,129,0,0,0,0,0,0,0
20139 DATA 0,0,0,129,0,0,0,0,0,0
20140 DATA 0,0,0,0,0,0,0,0,0,0
20141 DATA 0,0,142,0,142,0,0,0,0,0
20142 DATA 144,506,143,141,0,144,0,0,0,0
20143 DATA 144,0,0,142,142,144,0,0,0,0
20144 DATA 145,142,143,141,143,0,0,0,0,0
20145 DATA 146,144,0,152,0,0,0,0,0,0
20146 DATA 511,145,147,148,0,0,147,148,0,0
20147 DATA 153,146,0,151,0,0,0,0,0,146
20148 DATA 0,146,0,149,0,0,0,0,146,0
20149 DATA 0,0,148,0,0,0,0,0,0,0
20150 DATA 0,0,149,0,0,0,0,0,0,0
20151 DATA 0,0,147,0,0,0,0,0,0,0
20152 DATA 0,0,145,0,0,0,0,0,0,0
20153 DATA 0,147,0,0,0,0,0,0,0,0
20154 DATA 155,161,0,0,0,0,0,0,0,0
20155 DATA 0,154,0,0,503,0,0,0,0,0
20156 DATA 0,0,0,0,158,512,0,0,0,0
20157 DATA 0,139,158,0,159,0,0,0,0,0
20158 DATA 0,0,506,157,0,156,0,0,0,0
20159 DATA 0,157,0,160,0,157,0,0,0,0
20160 DATA 0,157,159,0,0,157,0,0,0,0
20161 DATA 154,0,162,156,156,0,0,0,0,0
20162 DATA 0,0,0,161,230,0,0,0,0,0
20163 DATA 0,0,0,0,0,0,0,0,0,0
20164 DATA 165,0,0,0,165,230,0,0,0,0
20165 DATA 519,164,0,0,210,0,0,0,0,0
20166 DATA 0,0,0,0,0,0,0,0,0,0
20167 DATA 0,0,0,0,0,0,0,0,0,0
20168 DATA 0,0,0,0,0,0,0,0,0,0
20169 DATA 0,0,0,0,0,0,0,0,0,0
20170 DATA 0,0,171,23,0,0,0,0,0,0
20171 DATA 0,0,0,170,0,0,0,0,0,0
20172 DATA 0,27,0,173,0,0,0,0,0,0
20173 DATA 0,0,172,0,174,0,0,0,0,0
20174 DATA 0,0,0,0,236,173,0,0,0,0
20175 DATA 236,178,237,238,0,0,0,0,0,0
20176 DATA 0,0,238,0,0,0,0,0,0,0
20177 DATA 521,237,0,0,0,521,0,0,0,0
20178 DATA 175,0,0,0,0,0,0,0,0,0
20179 DATA 0,0,0,0,0,0,0,0,0,0
20180 DATA 0,0,0,0,0,0,231,234,0,0
20181 DATA 0,182,0,48,0,0,0,0,0,0
20182 DATA 181,0,0,0,0,183,0,0,0,0
20183 DATA 223,0,184,222,240,0,0,0,0,240
20184 DATA 0,0,0,183,0,0,0,0,0,0
20185 DATA 223,191,240,224,0,0,0,0,0,0
20186 DATA 197,192,0,0,0,0,0,0,0,0
20187 DATA 201,0,225,188,0,224,0,0,0,0
20188 DATA 0,0,187,0,228,187,0,228,0,0
20189 DATA 0,508,229,0,0,229,0,0,0,508
20190 DATA 0,0,0,227,0,227,0,0,0,0
20191 DATA 0,193,227,0,227,193,0,0,0,229
20192 DATA 186,0,0,194,0,225,0,0,0,0
20193 DATA 0,0,226,191,0,226,0,0,0,0
20194 DATA 0,0,192,226,0,0,0,0,0,0
20195 DATA 0,0,0,197,0,0,0,0,0,0
20196 DATA 0,0,197,0,0,201,0,0,0,0
20197 DATA 0,186,195,196,0,0,0,0,0,0
20198 DATA 508,508,0,508,0,228,0,0,228,0
20199 DATA 0,0,508,0,0,0,0,0,0,0
20200 DATA 0,0,0,0,0,0,0,0,0,0
20201 DATA 187,0,239,196,196,239,0,0,0,0
20202 DATA 0,226,0,0,203,0,0,0,0,0
20203 DATA 0,0,0,204,0,202,0,0,0,0
20204 DATA 0,0,203,205,205,0,0,0,0,0
20205 DATA 0,0,204,0,206,204,0,0,0,0
20206 DATA 0,0,0,207,0,203,0,0,0,205
20207 DATA 0,508,0,0,0,0,206,0,508,0
20208 DATA 0,0,209,0,0,209,0,0,0,0
20209 DATA 0,0,48,208,48,0,0,0,0,0
20210 DATA 0,0,0,0,0,0,0,0,0,0
20211 DATA 0,0,0,0,0,0,0,0,0,0
20212 DATA 0,0,0,0,0,0,0,0,0,0
20213 DATA 0,0,0,0,0,0,0,0,0,0
20214 DATA 0,0,0,0,0,0,0,0,0,0
20215 DATA 0,0,0,0,0,0,0,0,0,0
20216 DATA 0,0,0,0,0,0,0,0,0,0
20217 DATA 0,0,0,0,0,0,0,0,0,0
20218 DATA 0,0,0,0,0,0,0,0,0,0
20219 DATA 0,0,0,0,0,0,0,0,0,0
20220 DATA 0,0,0,0,0,0,0,0,0,0
20221 DATA 0,0,0,0,0,0,0,0,0,0
20222 DATA 0,0,0,0,0,0,0,0,183,185
20223 DATA 0,0,183,185,0,0,0,0,0,0
20224 DATA 0,0,185,187,0,0,0,0,0,0
20225 DATA 0,0,0,187,192,0,0,0,0,0
20226 DATA 202,0,194,193,193,0,0,0,0,0
20227 DATA 0,0,0,0,190,191,0,0,0,0
20228 DATA 0,0,188,198,198,188,0,0,0,0
20229 DATA 0,0,191,189,189,191,0,0,0,0
20230 DATA 0,0,0,0,509,162,0,0,0,0
20231 DATA 28,180,0,0,0,0,0,0,0,0
20232 DATA 0,234,0,0,28,0,0,0,0,0
20233 DATA 28,0,0,0,0,0,0,0,0,0
20234 DATA 232,0,180,0,235,0,0,0,0,0
20235 DATA 0,0,0,0,0,234,0,0,0,0
20236 DATA 0,175,0,0,0,174,0,0,0,0
20237 DATA 0,0,0,175,0,0,516,0,0,0
20238 DATA 0,0,175,176,0,0,0,0,0,0
20239 DATA 0,0,507,201,201,507,0,0,0,0
20240 DATA 0,0,0,0,0,0,183,185,0,0
20291 DATA 0,0,111,0,0,0,0,0,0,0
20292 DATA 0,0,0,111,0,0,0,0,0,0
20293 DATA 99,0,0,110,0,0,0,0,0,0
20294 DATA 0,70,0,0,0,0,0,0,0,0
20295 DATA 99,0,0,0,0,0,0,0,0,0
20296 DATA 0,0,126,0,0,0,0,0,0,0
23500 REM B$ (1-62)
23501 DATA "at end of road again.","inside building.","in forest.","in valley.","at slit in streambed.","x"
23510 DATA "at hill in road.","outside grate.","below the grate.","in cobble crawl."
23515 DATA "in plover room.","in mirror canyon.","in bird chamber.","at top of small pit."
23520 DATA "in hall of mists.","in Nugget of Gold room.","on east bank of fissure."
23525 DATA "at west end of Hall of Mists.","in secret E/W canyon above tight canyon."
23530 DATA "at west end of long hall.","at junction of three secret canyons."
23535 DATA "at top of stalactite.","in Hall of mt king","in alcove.","at 'Y2'."
23540 DATA "at reservoir.","in arched hall.","at east end of long hall.","in dirty passage."
23545 DATA "at window on pit.","in dusty rock room.","at complex junction."
23550 DATA "in swiss cheese room.","in soft room.","in oriental room.","at east end of twopit room."
23555 DATA "at west end of twopit room.","in east pit.","in west pit.","in slab room."
23560 DATA "in narrow corridor.","in Giant Room.","in Dark-Room.","in cavern with waterfall."
23565 DATA "at steep incline.","at brink of pit.","in debris room.","in Misty cavern."
23570 DATA "in sloping corridor.","on SW side of chasm.","on NE side of chasm."
23575 DATA "in corridor.","at fork in path.","in chamber of boulders.","in limestone passage."
23580 DATA "in front of barren room.","at Breath-Taking View.","at junction with warm walls."
23585 DATA "in barren room.","in shell room.","in anteroom.","at Witt's End."
23600 REM B$ (126-165) [63-102]
23603 DATA "in salt marsh.","at the edge of a trackless salt marsh."
23605 DATA "at the forest's edge.","in the swamp.","on grassy knoll.","on sandy beach."
23610 DATA "at blackened shoals.","at Ocean Vista.","at Thunder Hole."
23615 DATA "in salt marsh.","in salty marsh."
23620 DATA "at approach to River Styx.","in the swamp.","at wet sands."
23625 DATA "in the helicopter's cramped passenger cabin.  The only                 visible feature is a silver button on the wall."
23630 DATA "in West Court.","in the Central Court.","in the East Court."
23635 DATA "at entrance to Lord's Keep.","at south end of Great Hall."
23640 DATA "at north end of Great Hall.","in east hallway.","in west hallway."
23645 DATA "in front of tiled door.","inside a large steel vault."
23650 DATA "in room overlooking secret garden.","in the Aviary.","in Kitchen."
23655 DATA "on a narrow N/S ridge high above a stone wall and a wide moat."
23660 DATA "at the northwest end of a narrow ridge high above the               castle's inner courtyard."
23665 DATA "on cliff ledge.","on south side of ravine.","at foot of cliff."
23670 DATA "at east end of ravine.","at west end of ravine.","at south end of ridge."
23675 DATA "in mine entrance.","in engineer's room.","at bottom of mine."
23680 DATA "in dead-end shaft."
23700 REM B$ (170-178) [103-111]
23705 DATA "at entrance to Throne room.","on east side of Throne Room."
23710 DATA "at east end of short E/W corridor.","in the Crypt."
23715 DATA "at top of stalagmite.","in Gothic Cathedral.","in Gothic Chapel."
23720 DATA "at sham rock.","Over the Rainbow (Room)."
23800 REM B$ (180-209) [112-141]
23805 DATA "in the Cloakroom.","at Dante's Rest.","at Devil's Chair.","in Rotunda."
23810 DATA "in the Conservatory.","in Star Chamber.","in Red Rock Crawl."
23815 DATA "at Tongue of Rock.","in Bat Cave.","in Bubble Chamber.","in the Hall of Ice."
23820 DATA "in the Fairy Grotto.","in Green Lake Room.","in the Crystal Palace."
23825 DATA "in the Rainbow Room.","at east end of Lost River Canyon."
23830 DATA "at end of Lost River Canyon.","on south side of Lost River Canyon."
23835 DATA "at east end of Blue Grotto.","at west wall of Blue Grotto."
23840 DATA "in phone booth.","at niche in ledge above Lost River."
23845 DATA "on ledge above limestone pinnacles.","at limestone pinnacles."
23850 DATA "in the Winery.","in old spillway.","in dry basin.","at dark cove."
23855 DATA "at Sword Point.","at top of slide."
25001 DATA large gold nugget
25002 DATA bars of silver
25003 DATA rare coins
25004 DATA persian rug
25005 DATA golden chain
25006 DATA rare spices
25007 DATA precious jewelry
25008 DATA egg-sized emerald
25009 DATA jeweled trident
25010 DATA golden eggs
25011 DATA platinum pyramid
20512 DATA ming vase
25013 DATA several diamonds
25014 DATA treasure chest
25015 DATA glistening pearl
25016 DATA delicate lyre
25017 DATA silver horn
25018 DATA star sapphire
25019 DATA beautiful flowers
25020 DATA priceless tapestry
25021 DATA opal sphere
25022 DATA elven crown
25023 DATA ingot
25024 DATA rose
25025 DATA Holy Grail
25026 DATA marble statue
25027 DATA gleaming sword
25028 DATA 28
25029 DATA 29
25030 DATA 30
25031 DATA 31
25032 DATA 32
25033 DATA 33
25034 DATA 34
25035 DATA 35
25036 DATA 36
25037 DATA 37
25038 DATA 38
25039 DATA 39
25040 DATA 40
25041 DATA 41
25042 DATA 42
25043 DATA 43
25044 DATA vintage wine
25045 DATA huge yellow pill
25046 DATA set of keys
25047 DATA brass lantern
25048 DATA wicker cage
25049 DATA Little bird in cage
25050 DATA tasty food
25051 DATA small bottle
25052 DATA Giant clam >GRUNT!<
25053 DATA Giant oyster >GROAN!<
25054 DATA velvet pillow
25055 DATA 'Spelunker Today'
25056 DATA Batteries
25057 DATA grey powder
25058 DATA dwarf's axe
25059 DATA whiskbroom
25060 DATA leather sack
25061 DATA bulging knapsack
25062 DATA glowing stone
25063 DATA colored mushrooms
25064 DATA wooden pole
25065 DATA metal cannister
25066 DATA shovel
25067 DATA black bird   
25068 DATA coil of rope
25069 DATA wooden box
25070 DATA leaflet
25071 DATA faded poster
25072 DATA rare book
25073 DATA plastic card
25074 DATA bronze shield
25075 DATA official document
25076 DATA soiled paper
25077 DATA silver flute
25078 DATA ruby slippers
25079 DATA velvet cloak
25080 DATA cloth bag
25081 DATA oaken cask
25082 DATA matchbox
25083 DATA wax candle   
25084 DATA match
25085 DATA match
25086 DATA match
27000 DATA "maze of twisting little","maze of little twisty","maze of little twisting"
27001 DATA "twisty maze of little","twisty little maze of","twisting maze of little"
27002 DATA "little maze of twisty","little twisty maze of","twisting little maze of"
27003 DATA "little maze of twisting"
28000 BB$(0) = "There is no way to go that direction."
28002 BB$(4) = "With what?  Your bare hands?"
28005 BB$(1) = "Please answer the question first."
28007 BB$(7) = "water."
28008 BB$(8)="oil."
28009 BB$(9) = "water here."
28010 BB$(10)= "oil here."
28011 BB$(14) = "down the pit." 
28012 BB$(15) = "up the dome."
28016 TR$(0) = "The troll is nowhere to be seen."
28017 TR$(1) = "A burly troll stands by the bridge and insists you throw him a treasure before  you may cross."
28018 TR$(2) = TR$(0): TR$(3) = TR$(0)
28019 BB$(11) = "A drawbridge now spans the fissure."
28023 TH$ = "There is nothing here to "
28026 NH$ = "There is nothing here "
28027 BB$(12) = "B12"
28028 BB$(5) = BB$(4)+"  Against *HIS* bear hands??"
28029 BB$(2) = "The top of a 12-foot-tall beanstalk is poking out of the west pit."
28030 BB$(3) = "There is a gigantic beanstalk stretching out of the west pit up to the hole."
28031 BB$(6) = "It is now pitch dark. If you proceed you will likely fall into a pit."
28033 BB$(13) = "B13"
28051 HLTH$(1) = "Your strength is nearly gone.  Only a miracle can save you now."
28052 HLTH$(2) = "You are on the edge of collapse.  Lots of sun and fresh air will                speed your recovery."
28053 HLTH$(3) = "You really ought to take a break.  You're in tough shape."
28054 HLTH$(4) = "You are weaker than usual.  Better avoid fights."
28055 H2$(1) = "Sheeesh!  What a mess!  Your hair is falling out and you're getting boils!"
28056 H2$(2) = "You are a walking wound. If you don't find an antidote soon it's curtains."
28057 H2$(3) = "You don't look too good. Better sit down."
28058 H2$(4) = "Is it hot in here?  You are flushed and sweating."
28063 A$(1) = "at one end of an immense N/S passage."
28064 A$(2) = "in the south side chamber."
28065 A$(3) = "in a low N/S passage at a hole in the floor. The hole goes down to             an E/W passage."
28066 A$(4) = "on the brink of a small clean climbable pit. A crawl leads west."
28067 A$(5) = "in the west side chamber of the hall of the mountain king. A passage           continues west and up here."
28070 A$(8) = "at a crossover of a high N/S passage and a low E/W one."
28072 A$(10) = "in a cul-de-sac about eight feet across."
28073 A$(11) = "in a secret N/S canyon above a large room."
28074 A$(12) = "at a wide place in a very tight N/S canyon."
28075 A$(13) = "in a secret north/south canyon above a sizable passage."
28077 A$(15) = "in an awkward sloping east/west canyon."
28078 A$(16) = "in a tall E/W canyon. A low tight crawl goes 3 feet north and seems            to open up."
28079 A$(17) = "in a secret canyon which exits to the north and east."
28083 A$(21) = "on the west side of the fissure in the hall of mists."
28084 A$(22) = "Dead end crawl."
28085 A$(23) = "The canyon runs into a mass of boulders -- dead end."
28086 A$(24) = "The canyon here becomes too tight to go further south."
28087 A$(25) = "The passage here is blocked by a recent cave-in."
28222 A$(26) = "You are at an elbow in a winding E/W passage."
28223 A$(27) = "You're in a long narrow east/west passage which curves out of sight             at both ends."
28225 A$(29) = "You are at the east end of a level passage at a hole in the floor."
28226 A$(30) = "You are following a yellow sandstone path.  There is a glow to                  the west."
28227 A$(31) = "You're in a steeply sloping passage.  It is very cold here."
28228 A$(32) = "You are in a windy tunnel between two large rooms."
28230 A$(34) = "You are in a very steeply angled tunnel."
28231 A$(35) = "You are in a very tight N/S crack.  The passage seems to widen to               the south."
28232 A$(36) = "You are at a high hole in a rock wall."
28233 A$(37) = "You're in a dead-end crack."
28234 A$(38) = "You're in a room containing several small climbable pits.  Passages exit        to the east and north."
28235 A$(39) = "You are at the bottom of a small featureless pit."
28330 REM (CONTAINERS)
28340 DATA 131,48,65,80,71,60,51,82,83,130,125,69
28500 REM THE WUMPUS!
28501 DATA A sleepy Wumpus is ambling towards you.  He wants to invite you to                   dinner.  He wants you to *be* the dinner!
28502 DATA The Wumpus is still on your trail.  And he's getting closer!!
28503 DATA The Wumpus is only a few yards behind you!  All this exercise                        is making him veeerrrrry hungry! 
28504 DATA The Wumpus almost has you in his grasp!  You can feel his hot breath                 on your neck!
29000 REM I$
29002 DATA "There are bars of silver here!"
29003 DATA "There are many coins here!"
29004 DATA "The dragon is sprawled out on a persian rug!"
29005 DATA "The bear is locked to the wall with a golden chain!"
29006 DATA "There are rare spices here!"
29007 DATA "There is precious jewelry here!"
29008 DATA "There is an emerald here the size of a plover's egg!"
29009 DATA "There is a jewel-encrusted trident here!"
29013 DATA "There are diamonds here!"
29014 DATA "The pirate's treasure chest is here!"
29015 DATA "Off to one side lies a glistening pearl!"
29016 DATA "There is a delicate lyre here!"
29017 DATA "There is a silver horn here!"
29018 DATA "There is a star sapphire here!"
29019 DATA "There are some beautiful flowers here!"
29020 DATA "A priceless tapestry is hanging on the wall!"
29021 DATA "There is a polished sphere of pure opal here!"
29022 DATA "An ancient crown of elven kings lies here!"
29023 DATA "There is a pile of silver ingots here!"
29025 DATA "There is an ornate silver chalice here!"
29026 DATA "There is an ancient marble statue lying here!"
29027 DATA "A gleaming sword is stuck into the anvil!"
29028 DATA "28"
29029 DATA "29"
29030 DATA "30"
29031 DATA "31"
29032 DATA "32"
29033 DATA "33"
29034 DATA "34"
29035 DATA "35"
29036 DATA "36"
29037 DATA "37"
29038 DATA "38"
29039 DATA "39"
29040 DATA "40"
29041 DATA "41"
29042 DATA "42"
29043 DATA "43"
29044 DATA "There is a fountain of sparkling vintage wine here!"
29046 DATA "There are some keys on the ground here."
29047 DATA "There is a shiny brass lamp nearby."
29048 DATA "A small wicker cage lined with paper spattered with bird droppings               lies nearby."
29049 DATA "A cheerful little bird is sitting here singing."
29050 DATA "There is food here."
29051 DATA "There is a bottle of water here."
29052 DATA "There is an enormous clam here with its shell tightly closed."
29053 DATA "There is an enormous oyster here with its shell tightly closed."
29054 DATA "A small velvet pillow lies on the floor."
29055 DATA "There are a few recent issues of 'Spelunker Today' magazine here."
29056 DATA "There are fresh batteries here."
29057 DATA "There is grey powder scattered about."
29058 DATA "There is a little axe here."
29059 DATA "There is a small whiskbroom here."
29060 DATA "There is a leather sack here."
29061 DATA "There is a bulging knapsack here."
29063 DATA "There are some oddly-colored mushrooms here."
29064 DATA "A wooden pole has been stuck in the mud here."
29066 DATA "A shovel is lying nearby."
29067 DATA "There is a clay statue of a black bird here."
29068 DATA "There is a tightly coiled rope here."
29069 DATA "A large wooden box has washed up on the shore."
29070 DATA "A leaflet is affixed to the staircase."
29071 DATA "Taped to the wall is a faded poster."
29073 DATA "Nearby is a small plastic card."
29074 DATA "Nearby is an intricately-wrought bronze shield bearing the escutcheon            of Duke Aldor."
29075 DATA "An official-looking document is lying nearby."
29999 RETURN

