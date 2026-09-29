.so mac
.th WANDER 5/26/78
.sh NAME
wander \*- Fantasy story tool
.sh SYNOPSIS
.bd wander
[
.bd -c[savefile]
] [ world ]
.sh DESCRIPTION
.it Wander
is a tool for writing non-deterministic fantasy "stories".
The product is a "story" whose unfolding is affected by decisions
made by the "reader".
.sp
The
.bd -c
flag allows continuation from a previously "saved" wander.
If the form "-csavefile" is used the saved wander is retrievd from
"savefile".  "-c" by itself refers to the default save file, called
"wand.save".
.sp
The optional world argument is described below under "WANDER FILES".
.sh COMMANDS
Wander includes several built-in commands.  Aside from recognizing the
standard compass directions (and their abbreviations) the following
commands are recognized:
.nf
inventory   list objects being carried
take        pick up specified object
take all    try to pick up all objects at a location
drop        drop specified object
drop all    drop all objects being carried
quit        stop playing
save        quit & save environment for later continuation
look        print the long description of the current location
re-start    (re)read new .wrld & .misc files, (switch worlds)
    The following debugging verbs only work if you own the files
~snoop      give a condensed list of possible actions (cheat)
~goto m     move, magically, to location number "m"
~goto m.n   move to location "m" and put it in state "n"
~vars       print a list of all non-zero variables
.fi
.sh "WANDER FILES"
The
.it world
argument is used to specify which fantasy "world" you wish to
explore.
Each world is described by a pair of files.
One file, (with the extension
.it .wrld),
contains all location-specific information, (long and short descriptions,
local action keywords, conditions and results); another file,
(with the extension
.it .misc),
contains all the global information, (initial message, verb and object
synonyms, initial object locations, global action keywords, conditions and
results).
.s3
These two files have identical first parts of their names, e.g. if you
wish your world to be called "oz", you would name the files
.it oz.wrld
and
.it oz.misc
respectively and you would execute "wander oz".
If no
.it world
is given when executing
.it Wander,
the default
.it world
"wander" is used, (i.e. the files are "wander.wrld"
and "wander.misc").
.sp
A third, optional file related to a particular world is the
.it .mon
file.  If such a file exists, (e.g.
.it oz.mon),
a record of each user's commands will be kept there.
If no such file exists, but a file named "/sys/games/.../wand/wand.mon",
(this name can be changed in "wandglb.c"),
does exist then the record of user's commands will be kept there.
If neither file exists or if the symbol "MONITOR" is defined as "0"
in wanddef.h no record will be kept.
This record is often useful in two ways.  It allows the author to see
how other users respond to hi/her world.  It allows the author
to can type notes and suggestions as commands while running
.it Wander
and later use these notes while modifying the
.it .wrld
and
.it .misc
files.
.sp
When a user "saves" his/her environment it is saved in a file called
"wand.save" in the current working directory.
.sh "FILE PROTECTION"
Typically, the author of a
.it Wander
world will want the
.it .misc
and
.it .wrld
files to be unreadable by others except through
.it Wander
itself.  A simple way to do this is to generate, for each world, a small
C program that runs setuid to the owner of the world and execs
.it Wander.
For example, if "smith" has files /u/smith/egypt.misc and /u/smith/egypt.wrld,
the following program would suffice:
.nf
main()
{
	execl("/usr/games/wander", "wander", "/u/smith/egypt", 0);
}
.fi
This program would be compiled, a.out moved to "egypt" in some convenient
location and "chmod 4755 egypt" would be done.
At this point running "egypt" would make the user effectively "smith"
and thereby allow the egypt files, (which would have mode 0600), to be read.
.s3
It was decided that using encryption on the files was too weak a defense
against a dedicated world-cracker and too much overhead to be worth the
effort.
.sh VARIABLES
.it Wander
provides 128 variables (numbered 0 through 127) which can contain numeric
values of -32768 through 32767.
The variables numbered 0 through 99 are general purpose and may be used
freely; variables 100 through 127 are set aside for pre-defined uses.
These variables are referenced with two syntaxes.
Some constructions require the specification of a variable number;
for instance, "v=6.3" is used to set variable 6 to the value 3.
Note that here the "6" is automatically a variable number, while
the "3" is a simple number.
"\*(VC6\*(VC" would be used to specify substitution of the value
contained in variable 6.
Thus, if variable 3 contains 5 and variable 6 contains 2,
\*(VC3\*(VC is equal to 5, and \*(VC6\*(VC is equal to 2.
Moreover:
.nf
"v=7.\*(VC3\*(VC" will set variable 7 to 5
"v+\*(VC6\*(VC.4" will add 4 to variable 2
"v?\*(VC6\*(VC.\*(VC3\*(VC" will test whether variable 2 is equal to 5.
"m=The answer is \*(VC3\*(VC." will print out as "The answer is 5."
.fi
.sp
The special variables are and their mnemonic names are:
.nf
100  CUR_LOC   The current location number (read/write)
101  PREV_LOC  The previous location number (read/write)
102  INP_VRB   The verb part of the most recent input line (read only)
103  INP_OBJ   The object part of the most recent input line (read only)
104  INP_WC    The number of words in the most recent input line (read only)
105  NUM_CARRY The number of objects being carried currently (read only)
106  MAX_CARRY The max number of objects that can be carried (read/write)
107  NOW_YEAR  The year of the decade (0:99) (read only)
108  NOW_MONTH The month of the year (1:12) (read only)
109  NOW_DOM   The day of the month (1:31) (read only)
110  NOW_DOW   The day of the week (0:6, 0 = sunday) (read only)
111  NOW_HOUR  The hour of the day (0:23) (read only)
112  NOW_MIN   The minute of the hour (0:59) (read only)
113  NOW_SEC   The second of the minute (0:59) (read only)
114  NOW_ET    Seconds elapsed since Wander was run (read/write)
115-127        Currently available for general use; I'm open to suggestions.
.fi
Note that "%INP_VRB%" and "%INP_OBJ%" are replaced by the actual verb and
object words when used in a text message.
Also note that, if you are carrying 5 things,
"v=NUM_CARRY.5" will be true
while "v=\*(VCNUM_CARRY\*(VC.5" will be true if variable 5 is equal to 5.

The ".wrld" documentation describes the uses and syntax of variables in
greater detail.
.sh LOCATIONS
.it Wander
is usually set up for 256 numbered locations (rooms, chambers, whatever),
but this limit can be changed by a parameter in the "wanddef.h" file.
Again, the ".wrld" documentation describes these further.
.sh "SYNTACTIC CONVENTIONS"
In reading both the .misc and .wrld files Wander uses the following
conventions.  In order to allow reasonable formatting of lines
the sequence \*(EC<LF> is ignored,
\*(ECn is replaced by <LF>,
\*(ECb is replaced by a <BS>,
\*(ECt is replaced by <HT>,
\*(ECr is replaced by <CR>,
\*(EC" is replaced by " (not considered a "quote"),
and \*(EC<SP> is replaced by <SP>.  In reading the file, each tab is
replaced by a single space, and quotes are stripped off.  <SP> and <LF>
codes created by either the escape sequence using \*(EC, or within quotes
are turned into non-delimiting <SP> or <LF> codes.  Otherwise, <SP> is
used as the field delimiter, and <LF> is used as the line delimiter.
Throughout the doc files the field delimiter may be described as
"spaces or tabs" indicating that one or more of these characters
may be used to delimit fields.  In some places "<SEP>" is used, meaning
a separator, either spaces or tabs.
.sh CAVEAT
Peter found lots of time to write
.it Wander;
however, when it came time to write documentation he found he was pressed for
time (so what's new?).  As a result, this documentation is of the bare-bones
variety and probably loaded with errors.  Sorry.
.sh BUGS
Ho ho ho.
.sh "SEE ALSO"
wrld.doc, misc.doc, wandaid.doc
.CC
