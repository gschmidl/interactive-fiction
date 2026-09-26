10 DIM BA(5,4),KB(42)
20 HO=1:BA=1
100 FORI=1TO5:FORJ=1TO4:READ BA(I,J):NEXTJ,I
110 DATA 9,11,17,11,10,11,9,12,8,2,4,6,3,5,7,1,13,16,14,15
120 GOSUB8000:GOSUB9000:IFA$="H"GOTO280
200 PRINT"Ⓒ";:GOSUB11000
210 POKE4464,1:GOSUB900:INPUT"ﾄﾞｳｽﾙ? ｢ｱｹﾙ｣ﾄ ｲﾚﾃ ｸﾀﾞｻｲ ";V$
220 IF(V$="ｱｹﾙ")=0GOTO210
230 GOSUB900:INPUT"ﾅﾆ ｦ ｱｹﾏｽｶ ? ｢ﾄﾞｱ｣ﾄ ｲﾚﾃ ｸﾀﾞｻｲ ";O$
240 IF(O$="ﾄﾞｱ")=0GOTO230
250 GOSUB3140
260 GOSUB900:INPUT"ﾄﾞｳｽﾙ? ｢ｱﾙｸ｣ﾄ ｲﾚﾃ ｸﾀﾞｻｲ ";V$
270 IF(V$="ｱﾙｸ")=0GOTO260
280 GOSUB2110:TI$="000000"
800 GOTO1000
900 CURSOR0,23:PRINTSPC(39):CURSOR0,23:RETURN
1000 REM=====MAIN=====
1010 GOSUB900:POKE4464,1:INPUT"ﾄﾞｳｽﾙ ?";V$
1020 V=-1*((V$="ﾑｸ")+(V$="ｱｹﾙ")*2+(V$="ｱﾙｸ")*3+(V$="ｼﾗﾍﾞﾙ")*4+(V$="ｼﾒﾙ")*5)
1030 ON V GOSUB2000,3000,4500,5000,2500
1032 IFV$="ﾄｹｲ"GOSUB11500
1040 IFVAL(TI$)>=10000THENGOTO12000
1050 GOTO1000
2000 REM=====MUKU SUB=====
2010 IFOP<>0GOTO7600
2020 GOSUB900:INPUT"ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?";O$
2030 O1=-1*((O$="ﾋﾀﾞﾘ")+(O$="ﾐｷﾞ")*2+(O$="ﾃｲｾｲ")*3)
2040 ON O1 GOTO2100,2200,2120
2050 GOTO2020
2100 HO=HO+1:IFHO=5THENHO=1
2110 PRINT"Ⓒ";
2112 ONBA(BA,HO)GOSUB10000,10100,10200,10300,10400,10500,10600,10700,10800
2115 ONBA(BA,HO)-9 GOSUB10900,11000,11000,11100,11300,11200,11400,10900
2120 OP=0:RETURN
2200 HO=HO-1:IFHO=0THENHO=4
2210 GOTO2110
2500 PRINT"Ⓗ";:GOTO2112
3000 REM=====AKERU SUB=====
3010 IFOP<>0GOTO7600
3020 GOSUB900:INPUT"ﾅﾆｦ ｱｹﾏｽｶ ?";O$
3030 O2=(O$="ﾄﾞｱ")+(O$="ﾀﾝｽ")*2+(O$="ｵｼｲﾚ")*3+(O$="ﾎﾞｯｸｽ")*4
3032 O3=(O$="ﾚｲｿﾞｳｺ")*5+(O$="ﾅｶﾞｼ")*6+(O$="ｼｮｯｷﾀﾞﾅ")*7
3035 O2=-1*(O2+O3)
3037 IFO$="ﾃｲｾｲ"THENRETURN
3040 ON O2 GOTO3100,3200,3300,3400,3500,3700,3900
3045 GOSUB900:PRINTO$;" ﾊ ｱｹﾚﾏｾﾝ !!"
3046 FORWA=0TO1500:NEXT:GOTO3020
3100 GOSUB900
3110 O2=-1*((BA(BA,HO)=11)+((BA(BA,HO)=6)+(BA(BA,HO)=5)+(BA(BA,HO)=15))*2)
3120 ON O2 GOTO 3140,3170
3130 GOTO3020
3140 OP=1:POKE53740,0:CURSOR16,1:PRINT"￣⑤③下左左左   ⑦⑤▁"
3142 FORIZ=53303TO54063STEP40:POKEIZ,61:NEXT:POKE53704,15:POKE53343,51
3144 GOSUB7300:CURSOR16,1:PRINT"   下左左左▁▁▁▁▁▁":POKE53303,0:POKE53343,60
3146 FORIZ=53383TO54063STEP40:POKEIZ,0:NEXT:POKE53704,0
3148 FORIZ=53270TO54070STEP40:POKEIZ,55:NEXT:GOSUB7300
3152 POKE53270,60:CURSOR22,1:PRINT"   ③⑤￣下左左左左左左▁⑤⑦"
3154 FORIZ=53390TO54070STEP40:POKEIZ,113:NEXT:FORIZ=53316TO54076STEP40
3156 POKEIZ,113:NEXT:POKE53715,15:GOSUB7300:POKE53715,0
3158 CURSOR25,1:PRINT"    下左左左左左左左▁▁▁▁▁▁▁▁▁▁":FORIZ=53396TO54076STEP40
3160 POKEIZ,0:NEXT:FORIZ=53400TO54080STEP40:POKEIZ,113:NEXT:POKE53759,72
3162 RETURN
3170 OP=1:POKE53717,0:FORIZ=53395TO53995STEP40:POKEIZ,113:NEXT
3172 CURSOR23,19:PRINT"③⑤￣￣":POKE53674,15:GOSUB7300
3174 POKE53674,0:FORIZ=53395TO53995STEP40:POKEIZ,0:NEXT
3176 CURSOR23,19:PRINT"     "
3178 FORIZ=53391TO53911STEP40:POKEIZ,53:NEXT:POKE53951,117:POKE53991,49
3180 POKE54031,113:GOSUB7300
3182 FORIZ=53391TO54031STEP40:POKEIZ,0:NEXT:RETURN
3199 REM=============TANSU OPEN=====
3200 IFBA(BA,HO)<>3GOTO3000
3201 GOSUB900
3202 CURSOR10,12:PRINT"ﾄ ﾋﾀﾞﾘ";TAB(18);"ﾄ ﾐｷﾞ"
3203 CURSOR16,17:PRINT"ﾋｷﾀﾞｼｳｴ":CURSOR16,20:PRINT"ﾋｷﾀﾞｼｼﾀ"
3205 GOSUB900:INPUT"ﾄﾞｺｦ ｱｹﾏｽｶ ?";O$
3206 O3=-1*((O$="ﾄﾋﾀﾞﾘ")+(O$="ﾄﾐｷﾞ")*2+(O$="ﾋｷﾀﾞｼｳｴ")*3+(O$="ﾋｷﾀﾞｼｼﾀ")*4)
3210 CURSOR10,12:PRINT"      ";TAB(18);"      "
3212 CURSOR16,17:PRINT"       ":CURSOR16,20:PRINT"       "
3217 ON O3 GOTO3220,3240,3260,3280
3218 GOTO3201
3220 FORIZ=53424TO53864STEP40:POKEIZ,0:NEXT
3225 CURSOR6,2:PRINT"⎾⑦⑤③":POKE53864,60:FORIZ=53374TO53894STEP40:POKEIZ,113
3230 POKEIZ+1,0:POKEIZ+2,0:NEXT:CURSOR6,17:PRINT"⎿⑤上━⏋"
3235 TX=10:GOSUB7000:OP=6:RETURN
3240 FORIZ=53385TO53825STEP40:POKEIZ,0:NEXT:POKE53865,60
3245 CURSOR24,2:PRINT"③⑤⑦⏋":FORIZ=53355TO53915STEP40:POKEIZ,61:NEXT
3250 CURSOR24,16:PRINT"⎾⑤下⑤⏌":TX=17:GOSUB7000:OP=7:RETURN
3260 TY=16:GOSUB7020:OP=8:RETURN
3280 TY=19:GOSUB7020:OP=9:RETURN
3300 IFBA(BA,HO)<>1THENRETURN
3310 CURSOR22,16:PRINT"ﾄ下下左ﾋﾀﾞﾘ":CURSOR27,16:PRINT"ﾄ下下左ﾐｷﾞ"
3330 GOSUB900:INPUT"ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?";O$
3335 CURSOR22,16:PRINT" 下下左    ":CURSOR27,16:PRINT" 下下左   "
3340 O4=-1*(((O$="ﾄﾋﾀﾞﾘ")+(O$="ﾋﾀﾞﾘ"))+((O$="ﾄﾐｷﾞ")+(O$="ﾐｷﾞ"))*2)
3350 ON O4 GOTO3360,3380
3355 GOTO3310
3360 FORIZ=53351TO54031STEP40:POKEIZ,61:NEXT
3361 CURSOR21,10:PRINT"⎿▁▁左左左下░░░◘左左左左下⎾￣⏋"
3365 GOSUB7300:FORIZ=0TO7:POKE53351+IZ*40,0:POKE53751+IZ*40,0:NEXT
3370 CURSOR23,10:PRINT"▁▁▁下左左░░▐下左左左左￣￣￣":OX=22:GOSUB7030:OP=2:RETURN
3380 FORIZ=53357TO54037STEP40:POKEIZ,113:NEXT
3385 CURSOR29,10:PRINT"⎿▁▁下左左左左◘░░░下左左左⎾￣￣":GOSUB7300
3390 FORIZ=0TO7:POKE53357+40*IZ,0:POKE53757+40*IZ,0:NEXT
3395 CURSOR27,10:PRINT"③③③下左左左左▌░░░下左左左￣￣￣":OX=27:GOSUB7030:OP=3:RETURN
3400 CURSOR30,9:PRINT"ｳ左下ｴ下下左ｼ左下ﾀ"
3420 GOSUB900:INPUT"ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?";O$
3425 CURSOR30,9:PRINT" 左下 下下左 左下 "
3430 O5=-1*((O$="ｳｴ")+(O$="ｼﾀ")*2+(O$="ﾃｲｾｲ")*3)
3440 ON O5 GOTO3460,3480,3000
3450 GOTO3400
3460 IY=10:GOSUB7220:OP=4:RETURN
3480 IY=12:GOSUB7220:OP=5:RETURN
3500 CURSOR12,7:PRINT"ｳ左下ｴ下下下下下左ｼ下左ﾀ"
3520 GOSUB900:INPUT"ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?";O$
3525 CURSOR12,7:PRINT" 左下 下下下下下左 下左 "
3530 O2=-1*((O$="ｳｴ")+(O$="ｼﾀ")*2+(O$="ﾃｲｾｲ")*3)
3540 ON O2 GOTO3560,3620,2120
3550 GOTO3500
3560 POKE53542,0:POKE53582,0:CURSOR16,7:PRINT"━⑥上③⑤￣":CURSOR16,9
3565 PRINT"      ":CURSOR15,8:PRINT"▕左下⏌下左▕下左▕━⑥上③⑤￣":":GOSUB7300
3570 CURSOR16,7:PRINT"  上   ":CURSOR15,8:PRINT" 下左▁▁▁▁▁▁"
3575 CURSOR15,10:PRINT" 左下   上   ":FORIZ=53509TO53749STEP40:POKEIZ,55:NEXT
3580 GOSUB7300:FORIZ=53509TO53749STEP40:POKEIZ,113:NEXT
3585 FORIZ=53594TO53714STEP40:POKEIZ,113:POKEIZ-44,0:NEXT
3590 CURSOR21,6:PRINT"⎾⑤▁下⑥▁":CURSOR21,10:PRINT"⎾⑤③下⑥━":GOSUB7300:OP=17
3600 CURSOR14,6:PRINT " ⎿▁▁ ▕"
3602 CURSOR14,7:PRINT "/⑤━/▏▕"
3604 CURSOR14,8:PRINT "░░░/⎿⏌"
3606 CURSOR14,9:PRINT "⎾￣⏋/  \"
3608 CURSOR14,10:PRINT"￣￣￣￣￣￣￣":RETURN
3620 POKE53822,0:POKE53862,0:CURSOR16,11:PRINT"③⑥上③⑤￣":CURSOR16,21
3625 PRINT"  ③⑤￣":FORIZ=53744TO54104STEP40:POKEIZ,113:NEXT:GOSUB7300
3630 CURSOR16,11:PRINT"  上   ":CURSOR16,21:PRINT"￣￣￣￣￣"
3635 FORIZ=53744TO54064STEP40:POKEIZ,0:NEXT:FORIZ=53669TO54109STEP40
3640 POKEIZ,55:NEXT:GOSUB7300:FORIZ=53669TO54069STEP40:POKEIZ,113:NEXT
3645 POKE54109,0:CURSOR21,10:PRINT"⎾⑤③下⑥━":FORIZ=53710TO54030STEP40:POKEIZ,0
3650 NEXT:CURSOR21,20:PRINT"▏      ":FORIZ=53753TO54113STEP40:POKEIZ,61:NEXT
3655 CURSOR21,21:PRINT"⑦⑤▁"
3660 CURSOR14,10:PRINT"○○  ▃▃ "
3662 CURSOR14,11:PRINT"￣￣￣￣￣￣￣"
3664 CURSOR14,12:PRINT"￣O ░░▃ "
3666 CURSOR14,13:PRINT"￣￣￣￣￣￣￣"
3668 CURSOR14,14:PRINT"⎾▏ III "
3670 CURSOR14,15:PRINT"▏▏ IIII"
3672 CURSOR14,16:PRINT"⎿▏○●○●●"
3674 CURSOR14,17:PRINT"￣￣￣￣￣￣￣"
3676 CURSOR14,18:PRINT"┳┳┳┳┳┳┳"
3678 CURSOR14,19:PRINT"┃┃┃┃┃┃┃"
3680 CURSOR14,20:PRINT"┃┃┃┃┃┃┃"
3690 OP=18:RETURN
3700 CURSOR11,17:PRINT"ｲ右右ﾆ右右ｻ右右ﾖ":CURSOR11,18:PRINT"ﾁ右右右右右ﾝ右右ﾝ"
3705 CURSOR26,15:PRINT"ｺﾞ左左下下ﾛ左下ｸ下左左左ｼ左下ﾁ"
3710 GOSUB900:INPUT"ﾄﾞｺ ｦ ｱｹﾏｽｶ ?";O$
3712 CURSOR11,17:PRINT" 右右 右右 右右 ":CURSOR11,18:PRINT" 右右右右右 右右 "
3713 CURSOR26,15:PRINT"  左左下下 左下 下左左左 左下 "
3715 O3=(O$="ｲﾁ")+(O$="ﾆ")*2+(O$="ｻﾝ")*3+(O$="ﾖﾝ")*4+(O$="ｺﾞ")*5+(O$="ﾛｸ")*6
3720 O4=-1*(O3+(O$="ｼﾁ")*7)
3725 ON O4GOTO 3730,3750,3770,3790,3820,3830,3840
3726 GOSUB900:PRINTO$;" ﾅﾝﾃ ｱﾘﾏｾﾝ !":GOTO3700
3730 NX=10:I1=53940:I2=53897:I3=53935:GOSUB7040:OP=10
3740 CURSOR10,17:PRINT" II"
3741 CURSOR10,18:PRINT" ■░"
3742 CURSOR10,19:PRINT" ■░"
3743 CURSOR10,20:PRINT"/▀░":RETURN
3750 NX=13:I1=53941:I2=53904:I3=53946:GOSUB7120:OP=11
3755 POKE53906,112:POKE54106,114
3760 CURSOR13,17:PRINT"I  "
3761 CURSOR13,18:PRINT"■ "
3762 CURSOR13,19:PRINT"■I"
3763 CURSOR13,20:PRINT"═■":RETURN
3770 NX=16:I1=53946:I2=53903:I3=53941:GOSUB7040:OP=12
3775 POKE53901,112:POKE54101,115
3780 CURSOR16,16:PRINT"⏋￣￣"
3781 CURSOR16,17:PRINT"▕▍▌"
3782 CURSOR16,18:PRINT"▕┣┫"
3783 CURSOR16,19:PRINT"▕┃┃"
3784 CURSOR16,20:PRINT"/┗┛":RETURN
3790 NX=19:I1=53947:I2=53910:I3=53952:GOSUB7120:OP=13
3800 POKE53912,60:POKE53992,50:POKE54072,50
3810 CURSOR19,17:PRINT"← "
3811 CURSOR19,18:PRINT"▏\"
3812 CURSOR19,19:PRINT"■■"
3813 CURSOR19,20:PRINT"■■":RETURN
3820 NY=15:GOSUB7200:CURSOR22,15:PRINT"⎾￣⏋":OP=14:RETURN
3830 NY=17:GOSUB7200:OP=15:RETURN
3840 NY=19:GOSUB7200:OP=16:RETURN
3900 CURSOR8,9:PRINT"ﾋ左下ﾀﾞ下左左ﾘ下下左ｳ左下ｴ":CURSOR22,9:PRINT"ﾐ左下ｷﾞ左左下下ｳ左下ｴ"
3910 CURSOR11,17:PRINT"ﾋﾀﾞﾘ右右ﾐｷﾞ":CURSOR13,18:PRINT"ｼﾀ右右右右ｼﾀ"
3920 GOSUB900:INPUT"ﾄﾞｺ ｦ ｱｹﾏｽｶ ?";O$
3930 CURSOR8,9:PRINT" 左下  下左左 下下左 左下 ":CURSOR22,9:PRINT" 左下  左左下下 左下 "
3940 CURSOR11,17:PRINT"     右右    ":CURSOR13,18:PRINT"  右右右右   "
3950 O5=-1*((O$="ﾋﾀﾞﾘｳｴ")+(O$="ﾐｷﾞｳｴ")*2+(O$="ﾋﾀﾞﾘｼﾀ")*3+(O$="ﾐｷﾞｼﾀ")*4)
3955 ON O5 GOTO3970,4020,4060,4090
3960 GOTO3900
3970 CURSOR11,6:PRINT"⑦⑤③ ▕左左左左左下⑵━╮▏▕":CURSOR11,15:PRINT"⑵━╯▏▕左左左左左下⑦⑤⏌⎾"
3975 FORIZ=53579TO53819STEP40:POKEIZ,117:POKEIZ+2,121:POKEIZ+3,113:POKEIZ+4,61
3980 NEXT:GOSUB7300:FORIZ=53498TO53938STEP40:POKEIZ,63:NEXT
3990 CURSOR11,6:PRINT"②▁▁②"
3991 CURSOR11,7:PRINT"━③③━"
3992 CURSOR11,8:PRINT"━③③━"
3993 CURSOR11,9:PRINT"▀▀▀▀▀"
3994 CURSOR11,10:PRINT"  ■╯"
3995 CURSOR11,11:PRINT"▐■╯╰■"
3996 CURSOR11,12:PRINT" ░╯■"
3997 CURSOR11,13:PRINT"▀▀▀▀▀"
3998 CURSOR11,14:PRINT"②▁②▁"
3999 CURSOR11,15:PRINT"②▁②②"
4000 CURSOR11,16:PRINT"￣￣￣￣"
4010 GOSUB7300:FORIZ=53498TO53938STEP40:POKEIZ,61:NEXT
4015 CURSOR8,6:PRINT"③⑤⏋":CURSOR8,16:PRINT"③⑤⏋":OP=19:RETURN
4020 CURSOR16,6:PRINT"▏ ▁⑤￣左左左左左下 ▕╭━":FORIZ=53584TO53864STEP40
4025 POKEIZ,113:POKEIZ+1,61:POKEIZ+2,117:NEXT:CURSOR17,16:PRINT"⏋▁⑤⑦"
4027 GOSUB7300
4030 CURSOR16,6: PRINT"▏\\\\"
4031 CURSOR16,7: PRINT"▏▕▕▕▕"
4032 CURSOR16,8: PRINT"▏////"
4033 CURSOR16,9: PRINT"▀▀▀▀▀"
4034 CURSOR16,10:PRINT"▏    "
4035 CURSOR16,11:PRINT"▏╰■  "
4036 CURSOR16,12:PRINT"▏■╰■ "
4037 CURSOR16,13:PRINT"▀▀▀▀▀"
4038 CURSOR16,14:PRINT"▏/▏  "
4039 CURSOR16,15:PRINT"▏\\  "
4040 CURSOR16,16:PRINT"⎾￣￣￣￣"
4045 FORIZ=53509TO53949STEP40:POKEIZ,55:NEXT:GOSUB7300
4050 CURSOR21,6:PRINT"⎾⑤③":FORIZ=53549TO53949STEP40:POKEIZ,113:POKEIZ+3,113
4055 NEXT:CURSOR21,16:PRINT"⎾⑤▁下 ":OP=20:RETURN
4060 CURSOR11,16:PRINT"⑦⑤③￣￣":CURSOR11,21:PRINT"  ▕▁▁"
4065 FORIZ=53941TO54101STEP40:POKEIZ,61:POKEIZ+2,0:NEXT:GOSUB7300
4070 CURSOR10,16:PRINT"▍￣￣￣"
4071 CURSOR10,17:PRINT"▍ ┏┓┓"
4072 CURSOR10,18:PRINT"▍┏┛┗┓┓"
4073 CURSOR10,19:PRINT"▍┃MZ┃┃"
4074 CURSOR10,20:PRINT"▍┗━━┛┛"
4075 CURSOR10,21:PRINT"▍◢■■■■"
4080 GOSUB7300:CURSOR8,16:PRINT"▁⑤⏋":FORIZ=53936TO54096STEP40
4085 POKEIZ,113:POKEIZ+1,0:POKEIZ+2,61:NEXT:OP=21:RETURN
4090 CURSOR16,16:PRINT"￣￣③⑤⑦":FORIZ=53946TO54066STEP40:POKEIZ,113:POKEIZ-2,0
4095 NEXT:CURSOR16,21:PRINT"▁⏌   ":GOSUB7300
4100 CURSOR16,16:PRINT"￣￣￣￣￣▎
4101 CURSOR16,17:PRINT" ┏┏┓ ▎"
4102 CURSOR16,18:PRINT"┏┏┛┗┓▎"
4103 CURSOR16,19:PRINT"⑷⑶MZ┃▎"
4104 CURSOR16,20:PRINT"┗┗━━┛▎"
4105 CURSOR16,21:PRINT"■■■■◣▎"
4110 GOSUB7300
4115 CURSOR21,16:PRINT"⎾⑤▁":FORIZ=53949TO54109STEP40:POKEIZ,113:POKEIZ+1,0
4120 POKEIZ+2,61:NEXT:OP=22:RETURN
4500 REM=====ARUKU SUB=====
4510 IF(BA=1)*(HO=1)THENBA=2:GOTO2110
4520 IF(BA=2)*(HO=3)THENBA=1:GOTO2110
4530 IF(BA=1)*(HO=4)*(OP=1)THENBA=4:GOTO2110
4540 IF(BA=2)*(HO=2)*(OP=1)THENBA=3:GOTO2110
4550 IF(BA=3)*(HO=4)*(OP=1)THENBA=2:GOTO2110
4560 IF(BA=4)*(HO=2)*(OP=1)THENBA=1:GOTO2110
4570 IF(BA=1)*(HO=2)*(OP=1)THENBA=5:GOTO2110
4575 IF(BA=5)*(HO=4)*(OP=1)THENBA=1:GOTO2110
4590 GOSUB900:PRINT" ｱﾙｹ ﾏｾﾝ !":MUSIC"▂C":RETURN
5000 REM=====SHIRABERU SUB=====
5001 GOSUB900:INPUT"ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?";O$
5010 O1=(O$="ﾎﾟｽﾀｰ")+(O$="ﾌﾄﾝ")*2+(O$="ﾊｺ")*3+(O$="ﾎﾞｯｸｽ")*4+(O$="ﾍﾞｯﾄﾞ")*5
5012 O2=(O$="ﾎﾟｹｯﾄ")*6+(O$="ﾌｸ")*7+(O$="ﾋｷﾀﾞｼ")*8+(O$="ﾎﾝ")*9+(O$="ﾏﾄﾞ")*10
5014 O3=(O$="ｶﾝｷｾﾝ")*11+(O$="ｺﾝﾛ")*12+(O$="ﾋﾞﾝ")*13+(O$="ﾎｳﾁｮｳ")*14
5016 O4=(O$="ﾊﾞｹﾂ")*15+(O$="ｺｯﾌﾟ")*16+(O$="ｲｽ")*17+(O$="ﾂｸｴ")*18
5018 O5=(O$="ﾚｲｿﾞｳｺ")*19+(O$="ｻﾗ")*20+(O$="ｳｲｽｷｰ")*21
5020 O8=-1*(O1+O2+O3+O4+O5)
5025 IFO$="ﾃｲｾｲ"THENRETURN
5030 ON O8 GOTO5100,5150,5200,5250,5300,5350,5400,5450,5500,5550,5700,5750
5032 ON O8-12 GOTO5800,5850,5900,5950,6050,6100,6150,6250,6300
5040 CURSOR0,23:PRINTSPC(39):CURSOR0,23:PRINTO$;" ﾊ ｼﾗﾍﾞ ﾗﾚﾏｾﾝ !!"
5050 FORIZ=0TO1200:NEXT:GOTO5000
5100 IF(BA(BA,HO)<>1)*(BA(BA,HO)<>2)GOTO5040
5110 FORIZ=10TO16:CURSORIZ,14:PRINT"↑":GOSUB7300:MUSIC"═A0":CURSORIZ,14
5120 PRINT" ":NEXT:KA=BA(BA,HO):GOSUB6500:RETURN
5150 IF(OP<>2)*(OP<>3)GOTO5040
5160 IX=21+(OP-2)*6:FORIZ=IXTOIX+4:CURSORIZ,0:PRINT"◥◤"
5170 GOSUB7500:MUSIC"═B0":CURSORIZ,0:PRINT" ":NEXT:KA=OP+1:GOSUB6500:RETURN
5200 IF(OP<>2)*(OP<>3)*(OP<>13)GOTO5040
5205 IFOP=13GOTO5230
5210 IX=21+(OP-2)*5:FORIZ=IXTOIX+3:CURSORIZ,21:PRINT"↑"
5220 GOSUB7400:MUSIC"═C0":CURSORIZ,21:PRINT" ":NEXT:KA=OP+3:GOSUB6500:RETURN
5230 FORI=0TO3:FORIZ=19TO20:CURSORIZ,22:PRINT"↑":GOSUB7400:MUSIC"C0"
5240 CURSORIZ,22:PRINT" ":NEXTIZ,I:KA=7:GOSUB6500:RETURN
5250 IF(OP<>4)*(OP<>5)GOTO5040
5260 FORIZ=23TO28:CURSORIZ,7:PRINT"◥◤":GOSUB7300:MUSIC"═D0"
5270 CURSORIZ,7:PRINT" ":NEXT:KA=OP+4:GOSUB6500:RETURN
5300 IFBA(BA,HO)<>2GOTO5040
5310 FORIZ=10TO30:CURSORIZ,22:PRINT"↑":GOSUB7400:MUSIC"═E0"
5320 CURSORIZ,22:PRINT" ":NEXT:KA=10:GOSUB6500:RETURN
5350 IF(OP<>6)*(OP<>7)GOTO5040
5360 IX=10+(OP-6)*7:FORIZ=IXTOIX+4STEP2:CURSORIZ,13:PRINT"↑"
5370 GOSUB7300:MUSIC"═G0":CURSORIZ,13:PRINT" ":NEXT:KA=OP+5:GOSUB6500:RETURN
5400 IF(OP<>6)*(OP<>7)GOTO5040
5410 IX=10+(OP-6)*7:FORIZ=IXTOIX+4:CURSORIZ,14:PRINT"◢◣"
5420 GOSUB7300:MUSIC"═B0═G0":CURSORIZ,14:PRINT" ":NEXT:KA=OP+7:GOSUB6500
5430 RETURN
5450 IF(OP=14)+(OP=15)+(OP=16)GOTO5480
5455 IF(OP<>8)*(OP<>9)GOTO5040
5460 IY=16+(OP-8)*3:FORIZ=11TO21:CURSORIZ,IY:PRINT"◥◤":GOSUB7400
5470 MUSIC"═F0═C0":CURSORIZ,IY:PRINT" ":NEXT:KA=OP+7:GOSUB6500:RETURN
5480 IY=16+(OP-14)*2:FORIZ=IYTOIY+1:CURSOR26,IZ:PRINT"←":FORIX=21TO25
5485 CURSORIX,12:PRINT"◥◤":GOSUB7400:MUSIC"▂B0":CURSORIX,12:PRINT" ":NEXT
5490 CURSOR26,IZ:PRINT" ":NEXT:KA=OP+3:GOSUB6500:RETURN
5500 IFBA(BA,HO)<>4GOTO5040
5510 FORIY=3TO19STEP4:CURSOR8,IY:PRINT"◣左下◤":FORIX=10TO24STEP2:CURSORIX,0
5520 PRINT"◥◤":GOSUB7700:GOSUB7300:MUSIC"═C0═D0":CURSORIX,0:PRINT"  "
5530 GOSUB7720:NEXT:CURSOR8,IY:PRINT" 左下 ":NEXT:POKE54016,60
5540 KA=20:GOSUB6500:RETURN
5550 SW=(BA(BA,HO)=7)+(BA(BA,HO)=10)+(BA(BA,HO)=17)+(BA(BA,HO)=13)*2
5555 SA=-1*(SW+(BA(BA,HO)=8)*3):ON SA GOTO5560,5600,5650
5560 FORIZ=12TO26:CURSORIZ,2:PRINT"◥◤":GOSUB7300:USR(62):CURSORIZ,2:PRINT"  "
5565 NEXT:FORIZ=3TO14:CURSOR28,IZ:PRINT"◢左下◥":GOSUB7300:USR(62):CURSOR28,IZ
5570 PRINT" 左下 ":NEXT:FORIZ=26TO12STEP-1:CURSORIZ,16:PRINT"◢◣":GOSUB7300
5575 USR(62):CURSORIZ,16:PRINT"  ":NEXT:FORIZ=14TO3STEP-1:CURSOR11,IZ
5580 PRINT"◣左下◤":GOSUB7300:USR(62):CURSOR11,IZ:PRINT" 左下 ":NEXT
5590 KA=-1*((BA(BA,HO)=7)*21+(BA(BA,HO)=10)*22+(BA(BA,HO)=17)*23)
5595 GOSUB6500:RETURN
5600 FORIZ=17TO27:CURSORIZ,2:PRINT"◥◤":GOSUB7300:MUSIC"B0"
5605 CURSORIZ,2:PRINT"  ":NEXT:FORIZ=3TO10:CURSOR29,IZ:PRINT"◢左下◥"
5610 GOSUB7300:MUSIC"B0":CURSOR29,IZ:PRINT" 左下 ":NEXT
5615 FORIZ=27TO17STEP-1:CURSORIZ,12:PRINT"◢◣":GOSUB7300:MUSIC"B0"
5620 CURSORIZ,12:PRINT"  ":NEXT
5625 FORIZ=10TO3STEP-1:CURSOR16,IZ:PRINT"◣左下◤":GOSUB7300:MUSIC"B0"
5630 CURSOR16,IZ:PRINT" 左下 ":NEXT:KA=24:GOSUB6500:RETURN
5650 FORIZ=12TO26:CURSORIZ,2:PRINT"◥◤":GOSUB7300:MUSIC"C0":CURSORIZ,2
5655 PRINT"  ":NEXT:FORIZ=3TO13:CURSOR28,IZ:PRINT"◢下左◥":GOSUB7300:MUSIC"C0"
5660 CURSOR28,IZ:PRINT" 左下 ":NEXT:FORIZ=26TO21STEP-1:CURSORIZ,15
5665 PRINT"◢◣":GOSUB7300:MUSIC"C0":CURSORIZ,15:PRINT"  ":NEXT
5670 FORIZ=12TO3STEP-1:CURSOR11,IZ:PRINT"◣左下◤":GOSUB7300:MUSIC"C0"
5675 CURSOR11,IZ:PRINT" 左下 ":NEXT:KA=25:GOSUB6500:RETURN
5700 FORIZ=0TO5
5710 CURSOR9,3:PRINT"◣左下◤":GOSUB7300:CURSOR9,3:PRINT" 左下 ":GOSUB7300:MUSIC"C0"
5720 CURSOR12,7:PRINT"◢◣":GOSUB7300:CURSOR12,7:PRINT"  ":GOSUB7300:MUSIC"▂C0"
5730 CURSOR15,4:PRINT"◢左下◥":GOSUB7300:CURSOR15,4:PRINT" 左下 ":GOSUB7300
5740 NEXT:KA=26:GOSUB6500:RETURN
5750 FORIZ=10TO16:CURSORIZ,11:PRINT"◥◤":GOSUB7300:MUSIC"▂C0▂D0"
5760 CURSORIZ,11:PRINT" ":NEXT:KA=27:GOSUB6500:RETURN
5800 IF(OP<>10)*(OP<>11)GOTO5040
5810 IX=11+(OP-10)*2:FORI=0TO2:FORIZ=IXTOIX+1:CURSORIZ,22:PRINT"↑"
5820 GOSUB7400:MUSIC"▂B0":CURSORIZ,22:PRINT" ":NEXTIZ,I
5830 KA=OP+18:GOSUB6500:RETURN
5850 IFOP<>12GOTO5040
5860 FORI=0TO5:FORIZ=17TO18:CURSORIZ,22:PRINT"↑":GOSUB7400:MUSIC"B0"
5870 CURSORIZ,22:PRINT" ":NEXTIZ,I:KA=30:GOSUB6500:RETURN
5900 IFBA(BA,HO)<>13GOTO5040
5910 FORIZ=0TO3:FORI=27TO29:CURSORI,21:PRINT"↑":GOSUB7400:MUSIC"═B0"
5920 CURSORI,21:PRINT" ":NEXTI,IZ:KA=31:GOSUB6500:RETURN
5950 IF(BA(BA,HO)<>14)*(OP<>19)*(OP<>20)GOTO5040
5960 ON OP-18GOTO6000,6020
5970 FORIZ=3TO11:CURSOR22,IZ:PRINT"◥◤":GOSUB7300:MUSIC"═C0"
5980 CURSOR22,IZ:PRINT"  ":NEXT:KA=32:GOSUB6500:RETURN
6000 FORIZ=9TO12:CURSOR9,IZ:PRINT"◣左下◤":GOSUB7400:MUSIC"═B0"
6010 CURSOR9,IZ:PRINT" 左下 ":NEXT:KA=33:GOSUB6500:RETURN
6020 FORIZ=9TO12:CURSOR22,IZ:PRINT"◢左下◥":GOSUB7400:MUSIC"═B0"
6030 CURSOR22,IZ:PRINT" 左下 ":NEXT:KA=34:GOSUB6500:RETURN
6050 IFBA(BA,HO)<>14GOTO5040
6060 FORIZ=13TO20:CURSORIZ,21:PRINT"=>":GOSUB7400:USR(62)
6070 CURSORIZ,21:PRINT"  ":NEXT:KA=35:GOSUB6500:RETURN
6100 IFBA(BA,HO)<>14GOTO5040
6110 FORIZ=10TO26:CURSORIZ,11:PRINT"◥◤":GOSUB7500:MUSIC"═G0"
6120 CURSORIZ,11:PRINT"  ":NEXT:KA=36:GOSUB6500:RETURN
6150 IF(OP<>17)*(OP<>18)GOTO5040
6160 ON OP-17 GOTO6190
6170 FORIZ=14TO20:CURSORIZ,4:PRINT"◥◤":GOSUB7400:USR(62)
6180 CURSORIZ,4:PRINT"  ":NEXT:KA=37:GOSUB6500:RETURN
6190 FORIZ=10TO19:CURSOR11,IZ:PRINT"=>":GOSUB7500:MUSIC"═F0"
6200 CURSOR11,IZ:PRINT"  ":NEXT:KA=38:GOSUB6500:RETURN
6250 IF(OP<>19)*(OP<>20)GOTO5040
6260 IX=11+(OP-19)*5:FORIZ=IXTOIX+4:CURSORIZ,4:PRINT"◥◤":GOSUB7400
6270 CURSORIZ,4:PRINT"  ":USR(62):NEXT:KA=OP+20:GOSUB6500:RETURN
6300 IF(OP<>21)*(OP<>22)GOTO5040
6310 IX=11+(OP-21)*5:FORIZ=IXTOIX+4:CURSORIZ,22:PRINT"↑":GOSUB7400
6320 MUSIC"←B0":CURSORIZ,22:PRINT" ":NEXT:KA=OP+20:GOSUB6500:RETURN
6500 REM==================
6510 IFKB(KA)=1GOTO6600
6520 CURSOR0,23:PRINTSPC(39)
6530 CURSOR0,23:PRINT" ｺｺ ﾆﾊ ｱﾘﾏｾﾝ !! "
6540 TEMPO7:MUSIC"R7E7F7"
6550 TI=VAL(TI$):TI=TI+200:TT$=STR$(TI):I=LEN(TT$)
6555 IFVAL(TT$)>5959GOTO12000
6560 TI$=LEFT$("000000",6-I)+TT$:RETURN
6600 CURSOR0,23:PRINTSPC(39):GOSUB7500
6610 CURSOR0,23:PRINT"  ｱｯﾀ-------!!!":GOSUB7400
6620 CURSOR10,1:PRINT"▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁"
6621 CURSOR10,2:PRINT"▏                   ▏"
6622 CURSOR10,3:PRINT"▏   - PC-8001 -     ▏"
6623 CURSOR10,4:PRINT"▏                   ▏"
6624 CURSOR10,5:PRINT"▏  1982.5.7         ▏"
6625 CURSOR10,6:PRINT"▏      ｶｲｼｬ ﾖﾘ .....▏"
6626 CURSOR10,7:PRINT"▏                   ▏"
6627 CURSOR10,8:PRINT"▏  1982.5.15        ▏"
6628 CURSOR10,9:PRINT"▏     I/O ﾍﾝｼｭｳﾌﾞ.. ▏"
6629 CURSOR10,10:PRINT"▏                   ▏"
6630 CURSOR10,11:PRINT"▏  1982.6.2         ▏"
6631 CURSOR10,12:PRINT"▏     ｶｲｼｬ ｶﾗ ..... ▏"
6632 CURSOR10,13:PRINT"▏                   ▏"
6633 CURSOR10,14:PRINT"▏  1982.7.30        ▏"
6634 CURSOR10,15:PRINT"▏      ｺﾑﾊﾟｯｸ ..... ▏"
6635 CURSOR10,16:PRINT"▏                   ▏"
6636 CURSOR10,17:PRINT"▏ 1982.8.1 ｹﾞﾝｻﾞｲ ﾃﾞ▏"
6637 CURSOR10,18:PRINT"▏ 200ﾏﾝ ｲｼﾞｮｳ ﾉ ｾｯﾄｳ▏"
6638 CURSOR10,19:PRINT"▏ ｱﾘ｡ ③━③⑤⑤⑥▁⑤⑥⑦⑤③⑤⑥"
6639 CURSOR10,20:PRINT"￣￣￣￣￣￣"
6640 TEMPO6:MUSIC"C5FFGG₤C6A3F6R0F3D5#AAGF7":OW=1
6650 FORIZ=0TO2000:NEXT:GOTO12000
7000 REM=====SUB=====
7010 CURSORTX,3:PRINT"┳━┳━┳━"
7011 CURSORTX,4:PRINT"▃ ▃ ▃ "
7012 CURSORTX,5:PRINT" \ ╲ \"
7013 CURSORTX,6:PRINT" ▏▏▏▏▏▏"
7014 CURSORTX,7:PRINT"═▏▏▏▏▏▏"
7015 CURSORTX,8:PRINT" ▏▏▏▏▏▏"
7016 CURSORTX,9:PRINT"▏▏▏▏▏▏▏ "
7017 CURSORTX,10:PRINT" ⎿▏⎿▏⎿▏"
7018 CURSORTX,11:PRINT"▃▕▃▕▃▕ "
7019 CURSORTX,12:PRINT"▁⏌▁⏌▁⏌ ":RETURN
7020 CURSOR9,TY  :PRINT"/▏            ▕\"
7021 CURSOR9,TY+1:PRINT"⎾￣￣￣￣￣￣￣￣￣￣￣￣￣￣⏋"
7022 CURSOR9,TY+2:PRINT"▏              ⑺"
7023 CURSOR9,TY+3:PRINT"⎿▁▁▁▁▁▁▁▁▁▁▁▁▁▁⏌":RETURN
7030 CURSOROX,3:PRINT"￣⑦⑤⑦"
7031 CURSOROX,5:PRINT" ⑤⑤⑤"
7032 CURSOROX,6:PRINT"▁②③②"
7033 CURSOROX,8:PRINT"━③▁ "
7034 CURSOROX,9:PRINT"②③⑤③"
7035 CURSOROX,15:PRINT"▁▁▁"
7036 CURSOROX,16:PRINT"   \"
7037 CURSOROX,17:PRINT"￣￣￣⏋"
7038 CURSOROX,18:PRINT"   ▕"
7039 CURSOROX,19:PRINT"▁▁▁⏌":RETURN
7040 CURSORNX,16:PRINT"⑥③":CURSORNX,21:PRINT"⑥③":FORIZ=I1TOI1+160STEP40
7050 POKEIZ,113:NEXT:POKEI1-40,112:GOSUB7300
7060 CURSORNX,16:PRINT"  ":CURSORNX,21:PRINT"￣￣￣":FORIZ=I1TOI1+120STEP40
7070 POKEIZ,0:NEXT:FORIZ=I2TOI2+200STEP40:POKEIZ,63:NEXT
7080 CURSORNX,16:PRINT"￣￣":GOSUB7300
7090 FORIZ=I2TOI2+160STEP40:POKEIZ,61:NEXT:POKEI2+200,0
7100 FORIZ=I3TOI3+160STEP40:POKEIZ,61:POKEIZ+1,0:NEXT
7110 CURSORNX-2,16:PRINT"━⏋":CURSORNX-2,21:PRINT"③⑥":RETURN
7120 CURSORNX,16:PRINT"￣③⑥":FORIZ=I1TOI1+160STEP40:POKEIZ,61:POKEIZ+2,0:NEXT
7130 CURSORNX,21:PRINT"▕③⑥":GOSUB7300
7140 CURSORNX,16:PRINT"￣￣￣":FORIZ=I1TOI1+160STEP40:POKEIZ,0:NEXT
7150 CURSORNX,21:PRINT"￣￣￣":FORIZ=I2TOI2+200STEP40:POKEIZ,55:POKEIZ-1,61:NEXT
7160 GOSUB7300:FORIZ=I2TOI2+160STEP40:POKEIZ,0:NEXT
7170 FORIZ=I3TOI3+160STEP40:POKEIZ,113:POKEIZ-1,0:NEXT
7180 CURSORNX+2,16:PRINT"⏋￣③":CURSORNX+2,21:PRINT"￣⑤▁"
7190 RETURN
7200 CURSOR22,NY:  PRINT"▏ ⑺\"
7201 CURSOR22,NY+1:PRINT"⎾￣￣⏋"
7202 CURSOR22,NY+2:PRINT"⎿▁▁⏌":RETURN
7220 CURSOR23,IY  :PRINT"▕￣￣￣￣⏋\"
7221 CURSOR23,IY+1:PRINT"▕￣￣══￣￣▏"
7222 CURSOR23,IY+2:PRINT"▕▁▁▁▁▁▁▏":RETURN
7300 FORIQ=0TO300:NEXT:RETURN
7400 FORIQ=0TO800:NEXT:RETURN
7500 FORIQ=0TO1200:NEXT:RETURN
7600 CURSOR0,23:PRINTSPC(39);:CURSOR0,23:PRINTV$;" ｺﾄ ﾊ ﾃﾞｷﾏｾﾝ ﾏｽﾞ ｼﾒﾃｸﾀﾞｻｲ"
7610 FORWA=0TO2000:NEXT:RETURN
7700 AD=53248+IX+IY*40:D1=PEEK(AD):D2=PEEK(AD+40)
7710 POKEAD,0:POKEAD+40,0:RETURN
7720 POKEAD,D1:POKEAD+40,D2:RETURN
8000 REM=====ANGO=====
8010 FORI=0TO10:R=RND(1):NEXT
8020 R=INT(RND(1)*42)+1:FORI=1TOR:READRR$:NEXT:KB(R)=1
8030 FORI=0TO10:R=RND(2):NEXT
8040 HA=LEN(RR$):RR=INT(RND(1)*(HA-2))+1
8050 A1=ASC(MID$(RR$,RR,1)):A2=ASC(MID$(RR$,RR+1,1))
8060 R=INT(RND(1)*11)+95:A1=A1-R:A2=A2-R
8070 AN$=STR$(A1)+STR$(A2):RETURN
8090 DATA ﾎﾟｽﾀｰ,ﾎﾟｽﾀｰ,ﾌﾄﾝ,ﾌﾄﾝ,ﾊｺ,ﾊｺ,ﾊｺ,ﾎﾞｯｸｽ,ﾎﾞｯｸｽ,ﾍﾞｯﾄﾞ,ﾎﾟｹｯﾄ,ﾎﾟｹｯﾄ,ﾌｸ,ﾌｸ
8100 DATA ﾋｷﾀﾞｼ,ﾋｷﾀﾞｼ,ﾋｷﾀﾞｼ,ﾋｷﾀﾞｼ,ﾋｷﾀﾞｼ,ﾎﾝ,ﾏﾄﾞ,ﾏﾄﾞ,ﾏﾄﾞ,ﾏﾄﾞ,ﾏﾄﾞ,ｶﾝｷｾﾝ,ｺﾝﾛ
8110 DATA ﾋﾞﾝ,ﾋﾞﾝ,ﾎｳﾁｮｳ,ﾊﾞｹﾂ,ｺｯﾌﾟ,ｺｯﾌﾟ,ｺｯﾌﾟ,ｲｽ,ﾂｸｴ,ﾚｲｿﾞｳｺ,ﾚｲｿﾞｳｺ,ｻﾗ,ｻﾗ
8120 DATA ｳｲｽｷｰ,ｳｲｽｷｰ
9000 REM=====HAJIME=====
9001 PRINT"Ⓒ   - MEMO ｻﾂｼﾞﾝ -"
9002 PRINTTAB(23);" BY T.SHIMOKAWA"
9005 CURSOR10,4:PRINT"▁下￣⑦⑥⑤⑤━③②下￣⑦⑥⑤⑤━③②"
9006 CURSOR9,5:PRINT"/":CURSOR2,13:PRINT"￣￣"
9008 FORIZ=53496TO53730STEP39:POKEIZ,118:POKEIZ+1,118:NEXT
9009 FORIZ=53458TO53786STEP41:POKEIZ,119:POKEIZ+97,119:NEXT
9010 CURSOR19,14:PRINT"￣⑦⑥⑤⑤━③②下￣⑦⑥⑤⑤━③②下￣"
9011 FORIZ=53732TO54012STEP40:POKEIZ,55:NEXT
9012 FORIZ=53785TO54025STEP40:POKEIZ,63:NEXT
9013 FORIZ=53920TO54040STEP40:POKEIZ,113:NEXT
9014 FORIZ=54052TO54079:POKEIZ,54:NEXT
9015 FORIZ=13TO19:CURSOR9,IZ:PRINT"▏    ▏":NEXT
9016 CURSOR10,12:PRINT"▁▁▁▁":CURSOR10,16:PRINT"ﾟ"
9017 CURSOR19,15:PRINT"━③②▁  ▁▁"
9018 CURSOR19,16:PRINT"▏ ▏▕  ▏ ⎾⏋"
9019 CURSOR19,17:PRINT"⎿▁⎿⏌  ⎿▁⎿⏌"
9020 CURSOR0,22:PRINT" ｾﾂﾒｲ ｲﾘﾏｽｶ ..? [Y/N]"
9030 GETA$:A=-1*((A$="Y")+(A$="N")*2):ON A+1 GOTO9030,9100,9300
9100 PRINT"Ⓒ - MEMO ｻﾂｼﾞﾝ  ｾﾂﾒｲ -下下"
9101 PRINT"1982.9ｶﾞﾂ ｱﾙ ｱﾒﾉ ﾋﾆ MZ ﾉ ﾄﾓﾀﾞﾁ TRS ﾊ 下"
9102 PRINT"  ｺﾛｻﾚﾀ....｡  PC ﾉ ﾃﾆﾖｯﾃ..!!下"
9103 PRINT" TRS ﾊ ｶﾚｶﾞ ｼﾇﾏｴ ﾆ PC ﾉ ﾔｯﾀ ﾊﾝｻﾞｲ ｦ下"
9104 PRINT" ｼﾗﾍﾞ ﾒﾓ ﾆ ｶｲﾀ ﾄｲｯﾀ｡下"
9105 PRINT" ｿｺﾃﾞ TRS ﾄ ｶﾀｷ ｦ ﾄﾙﾍﾞｸ MZ ﾊ TRS ﾉ下"
9106 PRINT" ｲｴ ｦ ｵﾄｽﾞﾚﾀ.....｡下下"
9107 PRINT"   - GAME -下"
9108 PRINT" ｱﾅﾀ ﾊ TRS ﾉ ﾍﾔｦ ｼﾗﾍﾞﾃ MEMO ｦ 下"
9109 PRINT"   ｻｶﾞｼ ﾀﾞｼﾃ ｸﾀﾞｻｲ !下下"
9110 PRINT"    PUSH KEY !!"
9120 GETA$:IFA$=""GOTO9120
9200 PRINT"Ⓒ  - MEMO ｻﾂｼﾞﾝ ｺﾏﾝﾄﾞ -下"
9201 PRINT" * ｺﾏﾝﾄﾞ"
9202 PRINT"    ｱｹﾙ..  ﾄﾞｱ. ﾀﾝｽ. ｵｼｲﾚ. ﾎﾞｯｸｽ. ﾅｶﾞｼ."
9203 PRINT"           ﾚｲｿﾞｳｺ. ｼｮｯｷﾀﾞﾅ下"
9204 PRINT"    ｼﾗﾍﾞﾙ ﾎﾟｽﾀｰ. ﾌﾄﾝ. ﾊｺ. ﾎﾞｯｸｽ. ﾍﾞｯﾄﾞ"
9205 PRINT"           ﾎﾟｹｯﾄ. ﾌｸ. ﾋｷﾀﾞｼ. ﾎﾝ. ﾏﾄﾞ. ｲｽ"
9206 PRINT"           ｶﾝｷｾﾝ. ｺﾝﾛ. ﾋﾞﾝ. ﾎｳﾁｮｳ. ﾊﾞｹﾂ"
9207 PRINT"           ｺｯﾌﾟ. ﾂｸｴ. ﾚｲｿﾞｳｺ. ｻﾗ. ｳｲｽｷｰ下"
9208 PRINT"    ﾑｸ... ﾋﾀﾞﾘ. ﾐｷﾞ下"
9209 PRINT"    ｱﾙｸ.. ｾﾞﾝｼﾝ ｲｯﾎﾟ ﾀﾞｹ下"
9210 PRINT"    ｼﾒﾙ.. ｱｹﾃｲﾙ ﾓﾉ ｦ ｼﾒﾏｽ"
9211 PRINT"          *ｼﾒﾅｲﾄ ｲﾄﾞｳ ﾃﾞｷﾏｾﾝ 下"
9212 PRINT"    ﾄｹｲ.. ﾄｹｲ ｦ ﾐﾏｽ (分:秒 ﾄｹｲ)下"
9213 PRINT"    ﾃｲｾｲ. ｺﾏﾝﾄﾞ ﾄﾘｹｼ下"
9220 PRINTTAB(12);"PUSH KEY"
9230 GETA$:IFA$=""GOTO9230
9300 PRINT"Ⓒ        \     ⑺     ▏     ▏    /"
9301 PRINT"         \     ⑵    ▏    ⑹    /"
9302 PRINT"   ￣￣⑤③   \    ⑷    ▏    ┃   /"
9303 PRINT"      ○￣￣⑤③\   ⑹￣￣￣￣￣￣￣￣￣⑵  /     ③⑤"
9304 PRINT"           ￣⎾￣￣⏋￣￣￣￣￣￣￣￣￣⎾￣⏋   ▁⑤⑦"
9305 PRINT"            ▏   ￣￣￣￣￣￣￣￣￣  ▕  ▕"
9306 PRINT"            ▏              ▕  ▕"
9307 PRINT"            ▏       ○      ▕  ▕"
9308 PRINT"            ▏   ┏━┓ ⎾▏     ▕  ▕"
9309 PRINT"   ⑤⑤⑤⑥⑥⑦￣￣￣▏   ⑷ ┃ ▏▏     ▕  ▕━③▁▁▁"
9310 PRINT"            ▏   ┗┳┛//      ▕▁▁▁▁▁▁▁▁"
9311 PRINT"            ▏  /￣￣￣/ ■      ■■■■■■■■"
9312 PRINT"            ▏ //▏T▕▌■■      ■■■■■■■■"
9313 PRINT"            ▏ ▏■ R ■■ ■     ⎾￣￣￣￣￣￣￣"
9314 PRINT"            ▏▕⏌ ⎿S⏌ ■▌ ■   ▕"
9315 PRINT"            ▏ O ┃ ┃        ▕"
9316 PRINT"            ▏   ┃┃┃        ▕"
9317 PRINT"            ▏   ┃┃┃        ▕￣￣⑤③"
9318 PRINT"            ▏  ▕⏋￣⎾▏       ▕    ￣￣⑤③"
9319 PRINT"            ▏   ￣ ￣        ▕"
9320 PRINT"           /￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣\"
9321 PRINT"          /    ▂▂▂▂▂▂▂▂▂▂    \"
9322 PRINT"         /    /    ⑺     \    \"
9330 CURSOR20,6:PRINTAN$
9340 CURSOR9,23:PRINT"START ?  [S] OR [H]  KEY"
9350 GETA$:IF((A$="S")+(A$="H"))=0GOTO9350
9360 RETURN
9999 REM=====HYOUJI SUB=====
10000 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10001 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
10002 PRINT"       ▏          ▕■■▏    ┃     ▏   ③⑤￣￣";
10003 PRINT"▁▁     ▏  ▁▁▁▁▁▁▁ ▕■■▏    ┃     ▏━━⑦  ▏"
10004 PRINT"■■⎾⑤③  ▏ ⑺ ▁▁▁   ▏▕■■▏    ┃     ▏▏    ▏"
10005 PRINT"■■▏  ⑦⑤▁ ⑺ ▏ ▕\  ▏▕■■▏    ┃     ▏▏    ▏"
10006 PRINT"■■▏  ▏ ┃ ⑺ ▏ ▕ \ ▏▕■■▏    ┃     ▏▏    ▏"
10007 PRINT"■■▏  ▏ ┃ ⑺ ⎿▁⏌▀▀⑵▏▕■■▏    ┃     ▏▏    ▏"
10008 PRINT"■■▏  ▏ ┃ ⑺/    /▏▏▕■■▏    ┃     ▏▏    ▏"
10009 PRINT"■■▏  ▏ ┃ ⑺￣￣￣￣￣￣ ▏▕■■▏    ┃     ▏▏    ▏"
10010 PRINT"■■▏  ▏ ┃ ⑺I LOVE ▏▕■■▏    ┃     ▏▏    ▏"
10011 PRINT"■■▏ ▐▌ ┃ ⑺  MZ⑤80▏▕■■◘■■■■■■■■■◘▏▏    ▏"
10012 PRINT"■■▏  ▏ ┃ ⑺ SHARP▕ ▕■■▏    ┃     ▏▏    ▏"
10013 PRINT"■■▏  ▏ ┃  ￣￣￣￣￣￣￣ ▕■■▏    ┃     ▏▏    ▏"
10014 PRINT"■■▏  ▏ ┃          ▕■■▏    ┃     ▏▏    ▏"
10015 PRINT"■■▏  ▏③┫          ▕■■▏    ┃     ▏⑦⑤③  ▏"
10016 PRINT"■■⎿━━⑦ ┃          ▕■■▏    ┃     ▏   ⑦⑤▁"
10017 PRINT"■■▏    ┃          ▕■■▏    ┃     ▏      ▁"
10018 PRINT"■■▏   ③┫          ▕■■▏    ┃     ▏"
10019 PRINT"■■⎿━━⑦ ⎿▁▁▁▁▁▁▁▁▁▁⏌■■⎿▁▁▁▁┃▁▁▁▁⏌ "
10021 PRINT"■■▏    ┃ ";TAB(32);"\"
10022 PRINT"■■▏   ▁┫";TAB(33);"\"
10050 RETURN
10100 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10101 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
10102 PRINT"       ▏          ▕■■▏          ▏   ③⑤⑦￣";
10103 PRINT"▁▁     ▏          ▕■■▏          ▏━━⑦ ▕ "
10104 PRINT"■■⎾⑥⑤  ▏          ▕■■▏          ▏▏   ▕ "
10105 PRINT"■■▏  ⑥⑤③▁ ▁▁▁▁▁▁▁ ▕■■▏          ▏▏   ▕ "
10106 PRINT"■■▏      ▏▏SOFT ▕ ▕■■▏          ▏▏   ▕ "
10107 PRINT"■■▏      ▏▏  OF ▕ ▕■■▏          ▏▏   ▕ "
10108 PRINT"■■⎾⑦⑤③   ▏▏   MZ▕ ▕■■▏  ▁▁▁▁    ▏▏   ▕ "
10109 PRINT"■■▏   ⑦⑤③▏▏┏━━━┓▕ ▕■■▏ ⑺    \   ▏▏   ▕ "
10110 PRINT"■■▏      ▏▏┃○=○┃▕ ▕■■▏ ⑺￣￣▀￣⏋   ▏▏   ▕ "
10111 PRINT"■■⎾⑦⑥⑤⑤━③▏▏┗━━━┛▕ ▕■■▏ ⑺    ▕   ▏▏   ▕ "
10112 PRINT"■■       ▏▏ｺﾑﾊﾟｯｸ▏▕■■▏ ⑺￣￣▀￣⏋   ▏▏   ▕ "
10113 PRINT"■■▏      ▏￣￣￣￣￣￣￣ ▕■■▏ ⑺    ▕   ▏▏   ▕ "
10114 PRINT"■■⎿▁②③━⑤⑥▏        ▕■■▏ ⑺￣￣▀￣⏋   ▏▏   ▕ "
10115 PRINT"■■▏      ▏        ▕■■▏ ⑺    ⏌   ▏▏   ▕ "
10116 PRINT"■■▏      ▏        ▕■■▏ ⑺￣￣￣⏋\\  ▏⑦⑤━ ▕ "
10117 PRINT"■■▏   ③⑤⑥▏ /￣⑦⑥￣⑤⑤⑥⑦⑤￣⑥⑤⑦⑦⑤￣\\\ ▏   ￣⑤━"
10118 PRINT"■■⎿━━⑦   ▏/                  ▕￣▏▏"
10119 PRINT"■■▏      U⎾⑦⑤￣⑤⑤⑤⑤￣⑤⑤⑥⑦⑤⑤⑥￣⑦⑤⏋▕⏌ "
10120 PRINT"■■▏     ▕ ■■■■■■■■■■■■■■■■■■■■▕ \"
10121 PRINT"■■▏   ③⑤⑥ ■ /               \▕▕  \"
10150 RETURN
10200 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10201 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
10202 PRINT"  ▕    ▏▏ ◢■■■■■■■■■■■■◣       ⑺      ▕■";
10203 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺      ▕■";
10204 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺ ③⑤￣⏋▏▕■";
10205 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺▕   ▕▏▕■";
10206 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺▕⎾⏋╲▕▏▕■";
10207 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺▕⎿⏌━┓▏▕■";
10208 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺▕▀▀▀▕▏▕■";
10209 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺▕   ▕▏▕■";
10210 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺▕MZ ▕▏▕■";
10211 PRINT"  ▕   O▏▏▕      ▕▏      ▏      ⑺▕ IS▕▏▕■";
10212 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺▕BEST▏▕■";
10213 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺ ╲  ▕▏▕■";
10214 PRINT"  ▕    ▏▏▕      ▕▏      ▏      ⑺  \ ▕▏▕■";
10215 PRINT"  ▕    ▏▏ ⎿▁▁▁▁▁⏌⎿▁▁▁▁▁⏌       ⑺   \▕▏▕■";
10216 PRINT"  ▕    ▏▏▕              ▏      ⑺    ╲▏▕■";
10217 PRINT"  ▕    ▏▏▕              ▏      ⑺      ▕■";
10218 PRINT"  ▕    ▏▏ ⎿▁▁▁▁▁▁▁▁▁▁▁▁⏌       ⑺      ▕■";
10219 PRINT"  ▕    ▏⎿⏌              ⎿▁▁▁▁▁▁⏌      ▕■";
10220 PRINT"  ▕   ▕/ ⑺              ▏       \     ▕■";
10221 PRINT"  ▕   /  ⑺▁▁▁▁▁▁▁▁▁▁▁▁▁▁▏        \    ▕■";
10222 PRINTSPC(30)
10250 RETURN
10300 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10301 PRINT"  ▕ ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
10302 PRINT"  ▕   ▏▏ ┣━┳━┳━┳━┳━┳━┳━┳━┫     ▕ "
10303 PRINT"  ▕   ▏▏ ┃I┃I┃I┃I┃I┃I┃I┃I┃     ▕ "
10304 PRINT"  ▕   ▏▏ ┃O┃O┃O┃O┃O┃O┃O┃O┃     ▕  ③⑤￣⏋▏"
10305 PRINT"  ▕   ▏▏ ┣━┻━┻━┻━┻━┻━┻━┻━┫     ▕ ⑺   ▕▏"
10306 PRINT"  ▕   ▏▏ ┣━┳━┳━┳━┳━┳━┳━┳━┫     ▕ ⑺┏━━┓▏"
10307 PRINT"  ▕   ▏▏ ┃M┃M┃M┃M┃M┃M┃M┃M┃     ▕ ⑺┃○○┃▏"
10308 PRINT"  ▕   ▏▏ ┃Z┃Z┃Z┃Z┃Z┃Z┃Z┃Z┃     ▕ ⑺┗━━┛▏"
10309 PRINT"  ▕   ▏▏ ┣━┻━┻━┻━┻━┻━┻━┻━┫     ▕ ⑺   ▕▏"
10310 PRINT"  ▕   ▏▏ ┣━┳━┳━┳━┳━┳━┳━┳━┫     ▕ ⑺SOFT▏"
10311 PRINT"  ▕  O▏▏ ┃C┃C┃C┃C┃C┃C┃C┃C┃     ▕ ⑺ IS▕▏"
10312 PRINT"  ▕  ▕▏▏ ┃M┃M┃M┃M┃M┃M┃M┃M┃     ▕ ⑺   ▕▏"
10313 PRINT"  ▕   ▏▏ ┣━┻━┻━┻━┻━┻━┻━┻━┫     ▕ ⑺  & ▏"
10314 PRINT"  ▕   ▏▏ ┣━┳━┳━┳━┳━┳━┳━┳━┫     ▕ ⑺HUDS▏"
10315 PRINT"  ▕   ▏▏ ┃ﾎ┃ﾎ┃ﾎ┃ﾎ┃ﾎ┃ﾎ┃ﾎ┃ﾎ┃     ▕  \  ▕▏"
10316 PRINT"  ▕   ▏▏ ┃ﾝ┃ﾝ┃ﾝ┃ﾝ┃ﾝ┃ﾝ┃ﾝ┃ﾝ┃     ▕   \ ▕ "
10317 PRINT"  ▕   ▏▏ ┣━┻━┻━┻━┻━┻━┻━┻━┫     ▕    \▕▏"
10318 PRINT"  ▕   ▏▏ ┣━┳━┳━┳━┳━┳━┳━┳━┫     ▕     ▕▏"
10319 PRINT"  ▕   ▏⎿▁┃M┃M┃M┃M┃M┃M┃M┃M┃▁▁▁▁▁⏌ "
10320 PRINT"  ▕   ▏  ┃B┃B┃B┃B┃B┃B┃B┃B┃      \"
10321 PRINT"  ▕  /   ┗━┻━┻━┻━┻━┻━┻━┻━┛       \"
10350 RETURN
10400 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10401 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
10402 PRINT"￣⑦⑤③   ⑺               ▁▁▁▁▁▁▁ ▕ "
10403 PRINT" ⑺  ⑦⑤③⑺              ⑺      ▕ ▕ "
10404 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕ "
10405 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕    ③⑤⑦￣";
10406 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10407 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10408 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10409 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10410 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10411 PRINT" ⑺    ▕⑺              ⑺      O▏▕   ⑺"
10412 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10413 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10414 PRINT" ⑺    ▕⑺              ⑺      ▕ ▕   ⑺"
10415 PRINT" ⑺  ③⑤⑦⑺              ⑺      ▕ ▕   ⑺"
10416 PRINT" ③⑤⑦   ⑺              ⑺      ▕ ▕   ⑺"
10417 PRINT"⑦      ⑺              ⑺      ▕ ▕   ⑺"
10418 PRINT"       ⑺              ⑺      ▕ ▕   ⑺￣⑦⑤③";
10419 PRINT"       ⑺              ⑺      ▕ ▕   ⑺"
10420 PRINT"       /￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣\  ⑺"
10421 PRINT"      /                          \ ⑺￣━▁"
10450 RETURN
10500 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10501 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
10502 PRINT"￣⑦⑤③    ▏              ▁▁▁▁▁▁▁  ▏"
10503 PRINT" ▕  ⑦⑤③ ▏             ▕       ▏ ▏"
10504 PRINT" ▕    ▕ ▏             ▕       ▏ ▏"
10505 PRINT" ▕    ▕ ▏             ▕       ▏ ▏   ③⑤⑦￣";
10506 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺"
10507 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺"
10508 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺"
10509 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺▁③━⑤";
10510 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺￣"
10511 PRINT" ▕    ▕ ▏             ▕      O▏ ▏  ⑺"
10512 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺"
10513 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺━━━━";
10514 PRINT" ▕    ▕ ▏             ▕       ▏ ▏  ⑺"
10515 PRINT" ▕  ③⑤⑥ ▏             ▕       ▏ ▏  ⑺"
10516 PRINT"③⑤⑦￣   ▕▏             ▕       ▏ ▏  ⑺"
10517 PRINT"        ▏             ▕       ▏ ▏  ⑺⑥⑤⑤③";
10518 PRINT"        ▏             ▕       ▏ ▏  ⑺"
10519 PRINT"        ▏             ▕       ▏ ▏  ⑺"
10520 PRINT"       /￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣\  ⑺￣⑦⑤③";
10521 PRINT"      /                          \ ⑺"
10550 RETURN
10600 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10601 PRINT"░   ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦ ▏"
10602 PRINT"░▏  ⑺   ▏                      ▕     ▏"
10603 PRINT"░   ⑺   ▏    ▂▂▂▂▂▂▂▂▂▂▂▂▂▂    ▕     ▏"
10604 PRINT"░▏  ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10605 PRINT"░▏  ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10606 PRINT"░▏  ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10607 PRINT"░▏  ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10608 PRINT"░▏  ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10609 PRINT"░▏  ⑺   ▏   ▍      ⑺╮     ▕▏   ▕     ▏"
10610 PRINT"░▏  ⑺  O▏   ▍      ⑺┃     ▕▏   ▕     ▏"
10611 PRINT"░▏O ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10612 PRINT"░▏  ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10613 PRINT"░   ⑺   ▏   ▍      ⑺      ▕▏   ▕     ▏"
10614 PRINT"░▏  ⑺   ▏    ▀▀▀▀▀▀▀▀▀▀▀▀▀▀    ▕     ▏"
10615 PRINT"░▏  ⑺   ▏                      ▕     ▏"
10616 PRINT"░▏  ⑺   ▏                      ▕     ▏"
10617 PRINT"░▏  ⑺   ▏                      ▕     ▏"
10618 PRINT"░▏  ⑺   ▏                      ▕     ▏"
10619 PRINT"░   ⑺  /￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣\    ▏"
10620 PRINT"░▏  ⑺ /                          \   ▏"
10650 RETURN
10700 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10701 PRINT"░   ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦  ▏"
10702 PRINT"░▏     ▕                        ▏     ▏"
10703 PRINT"░▏     ▕     ▂▂▂▂▂▂▂▂▂▂▂▂▂▂     ▏     ▏"
10704 PRINT"░▏     ▕    ▍       ▏     ▕▏    ▏     ▏"
10705 PRINT"░▏     ▕    ▍       ▏     ▕▏    ▏     ▏"
10706 PRINT"░▏     ▕    ▍       ▏     ▕▏    ▏     ▏"
10707 PRINT"░▏     ▕    ▍       ▏     ▕▏    ▏     ▏"
10708 PRINT"░▏     ▕    ▍       ▏     ▕▏    ▏     ▏"
10709 PRINT"░▏     ▕    ▍      ╭▏     ▕▏    ▏     ▏"
10710 PRINT"░▏     ▕    ▍      ┃▏     ▕▏    ▏     ▏"
10711 PRINT"░▏     ▕    ▍       ▏     ▕▏    ▏     ▏"
10712 PRINT"░▏     ▕    ▍       ▏     ▕▏    ▏     ▏"
10713 PRINT"░▏   ▁▁⏌▁   ▁▁▁▁▁▁▁ ▏     ▕▏    ▏     ▏"
10714 PRINT"░▏③⑤⑥ ③⑤⑥▏ /       /▏▀▀▀▀▀▀     ▏     ▏"
10715 PRINT"░▏⎾￣￣⏋   ▏ ⎾￣￣￣￣￣￣⏋ ▏           ▏     ▏"
10716 PRINT"░▏▏  ⑺  ⏌▏ ▏      ▕ ▏           ▏     ▏"
10717 PRINT"░▏▏  ⑺③⑥ ▏ ⎿②③②▁②③⏌ ▏           ▏     ▏"
10718 PRINT"░▏▏  ⑺  ┫ / ━③━    /           ▕▏     ▏"
10719 PRINT"░▏▏  ⑺③⑥ /        / ▏           ▏     ▏"
10720 PRINT"░▏▏  ⑺  /    ⑥⑤⑥ / ▕⎾￣￣￣￣￣￣￣￣￣￣￣\     ▏"
10721 PRINT"░▏▏  ⑺ / ▁②⑤    /  /▏            \    ▏"
10750 RETURN
10800 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10801 PRINT"   ▏￣⑥⑤③";SPC(24);"③⑤￣￣ ▏"
10802 PRINT"   ▏   ▕ ￣⑤③";TAB(28);"③⑤￣⏋     ▏"
10803 PRINT"   ▏   ▕   ⑺￣￣⑤▁▁▁▁▁▁▁▁▁▁⑤￣⏋    ▏   ▕ "
10804 PRINT"   ▏   ▕   ⑺  ▕ ▏ ▂▂▂▂ ▕▕   ▏   ▏   ▕ "
10805 PRINT"   ▏   ▕   ⑺  ▕ ▏▍  ▏▕▏▕ ▏  ▏   ▏   ▕ "
10806 PRINT"   ▏   ▕   ⑺  ▕ ▏▍ ┏▏▕▏▕ ▏  ▏   ▏   ▕ "
10807 PRINT"   ▏   ▕   ⑺  ▕ ▏▍  ▏▕▏▕ ▏  ▏   ▏   ▕ "
10808 PRINT"   ▏   ▕   ⑺  ▕ ▏ ════ ▕ ▏  ▏   ▏   ▕ "
10809 PRINT"   ▏   ▕   ⑺  ▕ ▏      ▕ ▏ ○▏   ▏   ▕ "
10810 PRINT"   ▏   ▕   ⑺  ▕ ▏      ▕ ▏  ▏   ▏   ▕ "
10811 PRINT"   ▏   ○▏  ⑺  ▕ ▏      ▕ ▏  ▏   ▏   ▕ "
10812 PRINT"   ▏   ▕   ⑺  ▕/￣￣￣￣￣￣￣\ ▏  ▏   ▏   ○▏"
10813 PRINT"   ▏   ▕   ⑺  /          \  ▏   ▏   ▕ "
10814 PRINT"   ▏   ▕   ⑺ /            \ ▏   ▏   ▕ "
10815 PRINT"   ▏   ▕   ⑺/              \▏   ▏   ▕ "
10816 PRINT"   ▏   ▕   /                \   ▏   ▕ "
10817 PRINT"   ▏   ▕  /                  \  ▏   ▕ "
10818 PRINT"   ▏   ▕ /                    \ ▏   ▕ "
10819 PRINT"   ▏   ▕/                      \▏   ▕ "
10820 PRINT"   ▏   /                        \   ▕ "
10821 PRINT"   ▏  /                          \  ▕ "
10850 RETURN
10900 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
10901 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
10902 PRINT"      ▏ ▏                      ▕  ▏"
10903 PRINT"      ▏ ▏    ▂▂▂▂▂▂▂▂▂▂▂▂▂▂    ▕  ▏"
10904 PRINT"      ▏ ▏   ▍       ▏      ▎   ▕  ▏"
10905 PRINT"      ▏ ▏   ▍       ▏      ▎   ▕  ▏"
10906 PRINT"      ▏ ▏   ▍       ▏      ▎   ▕  ▏"
10907 PRINT"      ▏ ▏   ▍       ▏      ▎   ▕  ▏"
10908 PRINT"      ▏ ▏   ▍       ▏      ▎   ▕  ▏"
10909 PRINT"      ▏ ▏   ▍      ╭▏      ▎   ▕  ▏"
10910 PRINT"      ▏ ▏   ▍      ┃▏      ▎   ▕  ▏"
10911 PRINT"     O▏ ▏   ▍       ▏      ▎   ▕  ▏"
10912 PRINT"      ▏ ▏   ▍       ▏      ▎   ▕  ▏"
10913 PRINT"      ▏ ▏   ▍       ▏      ▎   ▕  ▏"
10914 PRINT"      ▏▕    ▍       ▏      ▎   ▕  ▏"
10915 PRINT"      ▏▕     ══════════════    ▕  ▏"
10916 PRINT"      ▏▕                       ▕  ▏"
10917 PRINT"      ▏▕                       ▕  ▏"
10918 PRINT"      ▏▕                       ▕  ▏"
10919 PRINT"      ▏▕                       ▕  ▏"
10920 PRINT"      ▏/￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣\ ▏"
10921 PRINT"     ⑺/                          \▏"
10950 RETURN
11000 PRINT"▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁"
11010 CURSOR0,21:PRINT"￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣"
11020 CURSOR12,2:PRINT"▁▁▁▁▁▁▁▁▁▁";SPC(10)
11030 FORIZ=53379TO54059STEP40:POKEIZ,61:POKEIZ+11,113:POKEIZ+21,0:NEXT
11040 POKE53740,72:POKE53759,0
11050 RETURN
11100 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
11101 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦  ▏"
11102 PRINT"       ⑺   ▂▂▂                 ▕      ▏"
11103 PRINT"③      ⑺  ▍/○\▎   ▂▂▂▂▂▂▂▂▂▂   ▕      ▏"
11104 PRINT"┓￣⑦⑤③  ⑺  ▍○●○▎  ▍     ▏   ▕▏  ▕      ▏"
11105 PRINT"┃▕┏━③⏋▏⑺  ▍\○/▎  ▍     ▏   ▕▏  ▕      ▏"
11106 PRINT"┃▕┃  ▏▏⑺  ▕▀▀▀   ▍     ▏   ▕▏  ▁━⑥⎾￣⏋▕ "
11107 PRINT"┃▕┃  ▏▏⑺  ▕      ▍    ╭▏   ▕▏  ▏  ▏ ▕▕ "
11108 PRINT"┃▕┃  ▏▏⑺  ▍      ▍    ┃▏   ▕▏  ▏  ▏ ▕▕ "
11109 PRINT"┃▕┃  ▏▏⑺         ▍     ▏   ▕▏  ⎿▁②▏ ▕▕ "
11110 PRINT"┃▕┃  ▏▏⑺         ▍     ▏   ▕▏  ▏  ▏ ▕▕ "
11111 PRINT"┃▕┃  ▏▏⑺          ══════════   ▏  ▏ ▕▕ "
11112 PRINT"┃▕┃  ▏▏⑺   ▁▁▁▁▁▁              ▏  ▏ ▕▕ "
11113 PRINT"┃▕┃  ▏▏⑺  /━/ /━/⎿▁▁▁▁▁▁       ▏  ▏ ▕▕ "
11114 PRINT"┃▕╰⑥￣ ▏⑺ ⑺○⎾￣⏋○⑺/▏    ⑵ \      ▏  ▏ ▕▕ "
11115 PRINT"┛▕③⑤￣⏋▏⑺ ⑺￣￣￣￣￣⏋⎾￣￣￣￣⏋⎾￣⏋▏     ▏  ▏ ▕▕ "
11116 PRINT"￣￣   ▕▏⑺ ⑺￣￣⏋⎾￣⏋⎾￣⏋⎾￣⏋②②②▏     ▏  ▏ ▕▕ "
11117 PRINT"      ▏⑺ ⑺  ▕▏ ▕▏ ▕▏ ▕▏  ▏ /￣\ ▏  ▏ ▕▕ "
11118 PRINT"   ③⑤⏋▏⑺ ⑺  ▕▏ ▕▏ ▕▏ ▕⎿▁▁▏ ⎾▀⏋ ▏  ▏ ▕▕ "
11119 PRINT"▁⑤⑦   ▏⑺ ⑺  ▕▏ ▕▏ ▕▏ ▕▏  ▏ ⑵ ⑹ ▏  ▏ ▕▕ "
11120 PRINT"      ▏/￣⏋  ▕▏ ▕▏ ▕▏ ▕▏  ⎾￣┗━┛￣▏  ▏ ▕▕ "
11121 PRINT"     ⑺/   ￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣      \  ▏ ▕▕ "
11150 RETURN
11200 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
11201 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
11202 PRINT"       ▕               ▁▁▁▁▁▁▁  ▏"
11203 PRINT"⑤③     ▕              ⑺       ▏ ▏"
11204 PRINT" ▕￣￣⑤③ ▕              ⑺       ▏ ▏"
11205 PRINT" ▕    ▏▕      ◢■■■■■◣ ⑺       ▏ ▏"
11206 PRINT" ▕    ▏▕     ⑺       ▏⑺       ▏ ▏"
11207 PRINT" ▕    ▏▕     ⑺┃      ▏⑺       ▏ ▏"
11208 PRINT" ▕    ▏▕     ⑺┃      ▏⑺       ▏ ▏"
11209 PRINT" ▕    ▏▕     ⑺▁▁▁▁▁▁▁▏⑺       ▏ ▏"
11210 PRINT" ▕    ▏▕     ⑺       ▏⑺       ▏ ▏"
11211 PRINT" ▕③⑤⑦￣ ▕     ⑺       ▏⑺      O▏ ▏"
11212 PRINT"￣￣     ▕     ⑺       ▏⑺       ▏ ▏"
11213 PRINT"       ▕     ⑺       ▏⑺       ▏▁⎿▁▁▁▁"
11214 PRINT"    ▁▁▁⏌▁    ⑺┃      ▏⑺       ▏⎾⑤③   ⑦⑤▁";
11215 PRINT"③━⑥⑦▁▁▁⑤⑦▏   ⑺┃      ▏⑺       ▏▏  ⑦⑤▁"
11216 PRINT"▁▁▁ ⑤ ▏  ▏   ⑺       ▏⑺       ▏▏     ⑥⑤▁";
11217 PRINT"▁⑤⑦▏  ⎿━⑥▏   ⑺       ▏⑺       ▏▏⎾⑤③"
11218 PRINT"   ▏  ▏  ▏   ⑺       ▏⑺       ▏▏▏▏▕▕⑦⑤▁"
11219 PRINT"   ▏  ▏  ▏   ⑺       ▏⑺       ▏▏▏▏▕▕▕  ⑤";
11220 PRINT"   ▏  ⎿⑤￣⎾￣￣￣⏋       ⎾￣￣￣￣￣￣￣￣ ▏▏▏▕▕▕ "
11221 PRINT"   ▏  ▏  ▏    ￣￣￣￣￣￣￣          ▏▏▏▕▕▕ "
11250 RETURN
11300 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
11301 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
11302 PRINT" ▕   ▕ ⑺                        ▏"
11303 PRINT" ▕   ▕ ⑺                        ▏"
11304 PRINT" ▕   ▕ ⑺                        ▏   ③⑤⑦￣";
11305 PRINT" ▕   ▕ ⑺                        ▏  ⑺┏⑤⑥⑦";
11306 PRINT" ▕   ▕ ⑺                        ▏  ⑺┃"
11307 PRINT" ▕   ▕ ⑺                        ▏  ⑺┃"
11308 PRINT" ▕   ▕ ⑺                        ▏  ⑺┃"
11309 PRINT" ▕   ▕ ⑺                        ▏  ⑺┃"
11310 PRINT" ▕   ▕ ⑺                        ▏  ⑺┃"
11311 PRINT" ▕   ▕ ⑺                        ▏  ⑺┃"
11312 PRINT" ▕   O▏⑺    ▁▁▁▁▁▁▁▁▁▁▁▁▁       ▏  ⑺┃"
11313 PRINT" ▕   ▕ ⑺   /          ■┛ \      ▏  ⑺┃"
11314 PRINT" ▕   ▕ ⑺  /             ▕▎\     ▏  ⑺┗━③▁";
11315 PRINT" ▕   ▕ ⑺ ▕■■■■■■■■■■■■■■■▎■▏    ▏  ⑺￣⑦⑤③";
11316 PRINT" ▕   ▕ ⑺ ▕■■■■■■■■■■■■■■■┃■▏    ▏  ⑺"
11317 PRINT" ▕   ▕ ⑺ ▕■■■■■■■■■■■■■■■┃■▏    ▏  ⑺"
11318 PRINT" ▕   ▕ ⑺ ▕■⎾⎾⎾￣￣￣￣￣￣￣￣■■■╯■▏    ▏  ⑺"
11319 PRINT" ▕   ▕ ⑺ ▕■▏▏▏        ┃ ┃▏■▏    ▏  ⑺￣⑤③"
11320 PRINT" ▕   ▕ /￣⏋■▏⎾￣￣￣￣￣￣￣￣￣┣━┫▏■⎾￣￣￣￣\  ⑺   ⑦";
11321 PRINT" ▕   ▕/  ▕■/          ┃ ┃\■▏     \ ⑺"
11350 RETURN
11400 PRINT"￣⑦⑤③";TAB(36);"③⑤⑦￣";
11401 PRINT"    ￣⑦⑤②▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁②━⑤⑦"
11402 PRINT"       ⑺                       ▕ "
11403 PRINT"       ⑺                       ▕ "
11404 PRINT"       ⑺                       ▕ "
11405 PRINT"       ⑺   ◢■■■■■■■■◣          ▕ "
11406 PRINT"       ⑺  ▕╭━━━╮╭━━━╮▏         ▕ "
11407 PRINT"       ⑺  ▕┃━━━┃┃⑺⑺⑺┃▏         ▕ "
11408 PRINT"       ⑺  ▕┃━━━┃┃///┃▏         ▕ "
11409 PRINT"       ⑺  ▕┃▀▀▀┃┃▀▀▀┃▏         ▕ "
11410 PRINT"       ⑺  ▕┃ ■┛┃┃   ┃▏         ▕ "
11411 PRINT"       ⑺  ▕┃■┛ ┃┃╰■ ┃▏         ▕ "
11412 PRINT"       ⑺  ▕┃░┛■┃┃■╰■┃▏         ▕ "
11413 PRINT"       ⑺  ▕┃▀▀▀┃┃▀▀▀┃▏         ▕ "
11414 PRINT"       ⑺  ▕┃▁▁ ┃┃/▏/┃▏        ▁▁▁▁▁"
11415 PRINT"▁▁▁▁▁▁ ⑺  ▕╰━━━╯╰━━━╯▏       ⑺○⑥⑤③ ═⑥-③"
11416 PRINT"   ▁⑤⑦▏⑺  ▕￣￣￣￣⏋⎾￣￣￣⏋        ⑺￣⑥⑤○■■■■■⎿";
11417 PRINT"▁⑤⑦   ▏⑺  ▕    ▕▏   ▕        ⑺ ⑺  ⎾⑤③"
11418 PRINT"      ▏⑺  ▕    ▕▏   ▕        ⑺ ⑺  ▏ ▕═-③";
11419 PRINT"   ━⑦▏▏⑺  ▕    ▕▏   ▕        ⑺ ⑺  ▏ ▕ "
11420 PRINT"③⑤￣ ▏▏▏⑺  ▕    ▕▏   ▕        ⑺ ⑺  ▏ ▕ "
11421 PRINT"    ▏▏▏/￣￣⏋▁▁▁▁⏌⎿▁▁▁▁⎾￣￣￣￣￣￣￣⏋ ⑺  ▏ ▕▏"
11450 RETURN
11490 REM=====TOKEI SUB=====
11500 IF OP THENRETURN
11501 CURSOR28,12:PRINT"▁━⑤⑦⑤━▁"
11502 CURSOR0,13:PRINT"▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁         ③⑤￣￣   \   \"
11503 CURSOR0,14:PRINT"        ╭╮╭╮  ▕￣￣￣░░░￣￣￣       ▕   ▕"
11504 CURSOR0,15:PRINT"        ╰╯╰╯  ⑹  ◢■■■◣         /   /"
11505 CURSOR0,16:PRINT"              ⑶ /WATCH\        \  ￣￣\"
11506 CURSOR0,17:PRINT"              ⑵▐╭━━━━━╮▌        ▏     "
11507 CURSOR0,18:PRINT"              ⑷▕┃  :  ⑷⑷       ╱  ▁▁/"
11508 CURSOR0,19:PRINT"              ⑹▐╰━━━━━╯▌       \  ￣￣╲"
11509 CURSOR0,20:PRINT"              ⑺ \SHARP/        ▕    ⑺"
11510 CURSOR0,21:PRINT"               ▏ ◥■■■◤         /  ▁▁/"
11520 CURSOR0,23:PRINTSPC(39)
11530 CURSOR0,23:PRINT" ﾄｹｲ ﾐﾙﾉ ｦ ﾔﾒﾙ - PUSH ANY KEY"
11540 CURSOR17,18:PRINTMID$(TI$,3,2);"下";RIGHT$(TI$,2)
11545 IFVAL(TI$)>10000GOTO12000
11550 GETA$:IFA$=""GOTO11540
11560 GOTO2110
11990 REM=====OWARI=====
12000 GOSUB14000:FORIZ=0TO3500:NEXT
12001 PRINT"Ⓒ    - MEMO ｻﾂｼﾞﾝ -下"
12005 PRINT" ｵﾜﾘ ｦ ﾂｹﾞﾀ....｡  ﾅﾆﾓｶﾓ...｡下"
12010 IFOW=0GOTO12020
12011 PRINT" TRS ﾉ ｶﾀｷ ｦ ﾄｯﾀ MZ ﾃﾞﾊ ｱｯﾀ.!下"
12012 PRINT" ｼｶｼ ｿﾉ ｺｺﾛ ﾊ ﾅｾﾞｶ ﾊﾙﾅｶｯﾀ...!下"
12013 PRINT" ｿﾚﾊ PCﾉ ｺﾄﾊﾞ ﾃﾞｱﾙ...下"
12014 PRINT" ｢TRS ﾊ ｱﾉ MEMO ﾃﾞ ｵﾚｦ下"
12015 PRINT"  ｵﾄｼﾃ ｶﾈ ｦ ﾏｷｱｹﾞﾃｲﾀ...!下"
12016 PRINT"   TRS ｺｿ ﾊﾝｻﾞｲｼｬ ﾀﾞ..!｣ ﾄ....!下"
12017 GOTO13000
12020 PRINT" TRS ﾉ ｶﾀｷ ﾊ ﾄﾚﾅｶｯﾀ MZ ﾃﾞｱｯﾀ.!下"
12021 PRINT" ｿﾉ ｺｺﾛ ﾆﾊ TRS ﾉ ｻｲｺﾞﾆ ｲｯﾀ下"
12022 PRINT"   ｺﾄﾊﾞｶﾞ.....下"
12023 PRINT" ｢MZ ﾓｼ ｵﾚ ﾆ ﾅﾆｶ ｱｯﾀﾗ下"
12024 PRINT"   ｺﾉ MEMO ｦ ｹｲｻﾂﾍ ﾓｯﾃｲｯﾃ ｸﾚ下"
12025 PRINT"    ﾀﾉﾝﾀﾞｿﾞ ｾﾞｯﾀｲﾆ ﾀﾞｿﾞ...!｣ﾄ....!下"
13000 PRINT"下下    TRY AGAIN ? [ Y / N ]"
13010 GETA$: IFA$="ﾐ"THENEND
13020 IFA$="ﾝ"THEN RUN
13030 GOTO13010
14000 PRINT"Ⓒ - MEMO ｻﾂｼﾞﾝ -"
14001 PRINT"  ░░░░░ ░   ░ ░░░░░"
14002 PRINT"    ░   ░   ░ ░    "
14003 PRINT"    ░   ░░░░░ ░░░░ "
14004 PRINT"    ░   ░   ░ ░    "
14005 PRINT"    ░   ░   ░ ░░░░░下下"
14006 PRINT"       ▕■■■■■■■■■ ▕■■     ▕■■ ▕■■■■■■"
14007 PRINT"       ▕■■■■■■■■■ ▕■■■    ▕■■ ▕■■■■■■"
14008 PRINT"       ▕■■        ▕■■■■   ▕■■ ▕■■   ▕■■"
14009 PRINT"       ▕■■▃▃▃▃    ▕■■▕■■  ▕■■ ▕■■   ▕■■"
14010 PRINT"       ▕■■■■■■    ▕■■  ■■ ▕■■ ▕■■   ▕■■"
14011 PRINT"       ▕■■▀▀▀▀    ▕■■  ▕■■▕■■ ▕■■   ▕■■"
14012 PRINT"       ▕■■        ▕■■   ▕■■■■ ▕■■   ▕■■"
14013 PRINT"       ▕■■■■■■■■■ ▕■■    ▕■■■ ▕■■■■■■"
14014 PRINT"       ▕■■■■■■■■■ ▕■■     ▕■■ ▕■■■■■■下"
14020 PRINTTAB(20);AN$;" ﾊ ";RR$;" ﾃﾞｼﾀ｡"
14030 PRINT"         ┏━⑤━━③②③▁③⑤━③  ┏━━┓"
14031 PRINT"■■⑥⑥⑥⑥⑥━━┫ TRS     (￣￣￣￣￣￣￣￣￣⏋⑤━━┓ "
14032 PRINT"■■       ⑶    ░░ ░░╰   ▁▁▁▁▁ ▕   ┃ "
14033 PRINT"■ ▁②━━━⑤━⎿▁▁▁░░░░ ░░￣⑦￣￣┗▃━┛⑦⑤⑤━━┛ "
14034 PRINT"⑦⑦⑦⑦⑦⑦⑦⑦⑦⑦⑦░░░░░ ░░░░￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣":RETURN
