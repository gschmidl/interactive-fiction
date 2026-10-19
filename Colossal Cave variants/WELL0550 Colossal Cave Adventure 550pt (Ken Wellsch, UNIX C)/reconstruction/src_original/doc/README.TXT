
From kcwellsch@watdragon.UUCP (Ken C. Wellsch) Mon Jul  7 15:10:06 1986
Newsgroups: net.sources.games
Subject: Adventure -- part 0 of 7
Organization: U of Waterloo, Ontario

I have been meaning to submit this game for some time but never seemed
to find time to do it. This version of Adventure is taken from a Zerox
Sigma-9 (rest her soul!), originally written by Dave Platt of Honeywell
under CP-V (in Fort-77). I rewrote it into Ratfor many years ago and a
couple years ago rewrote it again in C. This is the 550 point version
of Adventure (for those who only know the 350 point original).

I had a lot of fun playing it on the SIGMA-9, and rewriting it, I hope
you enjoy it too! Watch out, having the database can certainly ruin
the game if you look at it!

The system components such as restricting the programs use etc. have
been left incomplete. I had done them back in the old Ratfor version
under V7 but each system can be quite different about determining load
etc. so I didn't bother (a lame excuse I know).

+--------------------------------------------------------------------------
|
|                       CP-V Adventure
|
|    Originally written by David Platt (from the 350 point Adventure),
|    UNIX C Version written by Ken Wellsch, with several modifications.
|
|    Original release notice from 12/1/79:
|
|        FOR THE OFFICIAL STUFF: PERMISSION IS HEREBY GRANTED TO ALL
|        USERS TO POSSESS, USE, COPY, DISTRIBUTE, AND MODIFY (BUT NOT
|        TO SELL) THE PROGRAMS AND FILES IN THIS PACKAGE.
|
|        IF YOU HAVE ANY PROBLEMS GETTING THIS TURKEY INSTALLED, FEEL
|        FREE TO WRITE OR CALL - AND HAVE FUN, FOLKS
|
|            DAVID PLATT
|            HONEYWELL LOS ANGELES DEVELOPMENT CENTER
|            5250 WEST CENTURY BOULEVARD
|            LOS ANGELES, CALIFORNIA  90045
|            (213) 649-6870 x253
|            HVN 369-1253
|
|    Current release notice as of 8/11/85:
|
|       Permission is hereby granted to all users to possess, use, copy,
|       distribute, and modify the programs and files in this package
|       provided it is not for direct commercial benefit and secondly,
|       that this notice and all modification information be maintained
|       along with the package.
|
|            Ken Wellsch
|            University of Waterloo
|            CS Graduate Department
|            Waterloo, Ontario  N2l 3G1
|            (519) 888-4518
|
|
|History:
|
|      1979 winter:
|            -- Running on XEROX SIGMA-9 under CP-V.
|               Written in Fortran-77 by David Platt.
|
|      1982 winter:
|            -- Completely rewritten for PDP 11/44 under UNIX Version 7.
|               Written in Rational FORTRAN (ratfor) by Ken Wellsch.
|      1984 fall:
|            -- Once again rewritten, this time for a VAX 11/780,
|               under UNIX BSD 4.2. Written in C by Ken Wellsch.
|
|
|       Copyright (c) 1979 David Platt    Database & Methods
|       Copyright (c) 1984 Ken Wellsch    C Code & Modifications
|
+--------------------------------------------------------------------------

--

------------------------------------------------------------------------------
-- Ken C. Wellsch, CS Dept., U. Waterloo, Waterloo, Ontario, Canada N2L 3G1 --
CSNET:kcwellsch%watdragon@waterloo.csnet                   HOME:1-519-746-4984
ARPA :kcwellsch%watdragon%waterloo.csnet@csnet-relay.arpa  OFFI:1-519-888-4518
UUCP :...!{allegra|clyde|linus|utzoo|inhp4|decvax}!watmath!watdragon!kcwellsch
------------------------------------------------------------------------------

