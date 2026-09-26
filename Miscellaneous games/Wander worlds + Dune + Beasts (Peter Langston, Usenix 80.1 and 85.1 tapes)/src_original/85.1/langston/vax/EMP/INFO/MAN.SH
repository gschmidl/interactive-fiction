onintr quit
set t = /tmp/emp$$
pump > $t
.so INFO.MAC
.pl 11i
.ll 6i
.lt 5i
.po 1i
.ce 2
.sp 15
EMPIRE INFORMATION MANUAL
.sp 3
Preliminary Edition
.sp 5
.nf
.in +26

   -------0000000000111111111
   76543210123456789012345678
-3 . . a ^ ^ * g . . f ^ ? ^  -3
-2  . . h b ^ ^ . ^ a . * ? ^ -2
-1 . . a w ^ a . ^ * . ? ^ ^  -1
 0  . . ^ c * = # f m . ? ? . 0
 1 . . ) m . . ^ ^ f . ?      1
 2  . . h . . . ? ? ? ? ^     2
 3 . . . . . . . ? ? ^ ?      3
   -------0000000000111111111
   76543210123456789012345678

.in
.fi
.sp 8
.ce 99
Peter Langston
.sp 3
\n(mo/\n(dy/\n(yr
.sp 10
Copyright (c) 1977 by P. Langston
.ce 0
.nr % 0
!
if "$1" != "" then
	cat $1 $2 $3 $4 $5 $6 $7 $8 $9 >>$t
else
	cat Overview >> $t
	cat [a-d]* >> $t
	cat [e-m]* >> $t
	cat [n-r]* >> $t
	cat [s-z]* >> $t
	cat {commands} >> $t
endif
echo ".TC" >> $t
detab -i < $t | nroff

: quit
rm -f $t
