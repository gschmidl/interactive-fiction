.so mac
.th "WANDER .WRLD SYNTAX" 5/26/78
.sh "SYNTAX OF THE .WRLD FILE FOR WANDER"

The
.it .wrld
file contains all the location-specific information to describe
a particular "world".
.s1
The file consists of a series of entries separated by blank lines,
each describing one location in a particular state
or in all states.
.s3
The description of a location for a particular
state consists of a location number followed by a period and a state number,
an optional short description,
an optional long description,
and a series of actions possible in that state of the location.
.s3
The description of a location for all states
consists of a location number with no state number,
an optional short description,
an optional long description,
and a series of actions possible in all states of the location.
.s1
.sh "LOCATION NUMBER & SHORT-DESCRIPTION"
The format of the location number & short-description is:
(a sequence of spaces or tabs is represented by <SEP> here)
.s3
.in 0
.it #lnumber
<SEP>
.it short-description
LF
.i0
or
.s3
.in 0
.it #lnumber.snumber
<SEP>
.it short-description
LF
.i0
.br
.s3
.it Lnumber,
(location number), is a decimal number in the range 1 to 256, (this limit
can be changed in the file "wanddef.h"),
preceded by '#'.
.s3
.it Snumber,
(state number), is a decimal number in the range 0 to 127.
.s3
If the first form is used, (#lnumber), the follwing lines describe the
"base state", i.e. characteristics shared by ALL states of the given
location.  If the second form is used, (#lnumber.snumber), the following
lines are only active when the specified state is active.
.s3
.it Short-description
is a one-line description used to identify the location after its
full description has already been given.
.s3
Wander initially places the user in location 1 and in state 0;
thus location 1 is the only required location.
The assignment of numbers to other
locations is entirely up to the user and they need not be in
any particular order in the
.it .wrld
file.
.sh "LONG-DESCRIPTION"
.it Long-description
may be up to 1024 characters in length and may
contain any number of lines, but may
not contain a line starting with a <SEP>, (i.e. neither spaces nor tabs),
or '#'
unless the whole thing is in quotes
because the first such line indicates the beginning of the next section,
(actions or a new location/state).
It's common to enclose the entire long description in quotes in which case
lines may begin with a <SEP> or '#'.
If the long description is not quoted but you
need spaces at the beginning of a line a line starting with an escaped space,
"\*(EC ", will work.
.sh "ACTIONS"
The format of actions is as follows:
.s3
.in 0
.na
<SEP>
.it match
<SEP>
.it dest
<SEP>
.it contin
<SEP>
.it field
<SEP>
.it field
<SEP> . . .
.it "m=text message"
LF
.i0
.ad
.s3
.it Match
is either one or two words, usually thought of as verb or verb & object.
If it is two words they are separated by an escaped space or are
enclosed in quotes.  This word (or words) is
matched against the user's input by a minimal match process which
facilitates abbreviation.
.it Match
patterns can be combined with logical OR as
.it match
|
.it match
| . . .
.nf
For example,
.EX
light\*(EC up|smoke  t?cigarettes  m="Eeech!  These butts taste awful!"
Which is equivalent to:
.EX
light\*(EC up  t?cigarettes  m="Eeech!  These butts taste awful!"
.EX
smoke  t?cigarettes  m="Eeech!  These butts taste awful!"
.fi
.s3
The verb "*" is a special case that matches anything typed by the user.
.s1
.it Dest
is the number of the location to which the user will be moved if all
tests succeed and is optional.  A value of 0, (or omission of the dest
field), provides no movement while a negative
value ends the game (-1 is a verbose exit, -2 is a silent exit like "quit").
A variable value like "\*(VC4\*(VC" will be replaced by the contents of
of the appropriate variable, (variable 4 in this example),
evaluated at the time the user types the command.
.s1
The optional
.it contin
argument is either "..." or ",,,".
Either form indicates that even if this action passes all tests and is
carried out the search should continue for any further actions that match,
thus allowing multiple actions to take place for a single command.
The ",,," form requires that another match be found for the command to be
"satisfied" and the program will complain if none is found, while the
"..." form does not;
(it is common to use ",,," for actions that are "transparent" to the user).
.s1
The
.it "m=text message"
argument may contain an arbitrary text message of up to 512 characters.
This message is output as the last step in carrying out the results of
an action.  "\*(ECb", "\*(ECn", "\*(ECr", and "\*(ECt" are interpreted as
backspace, return-line-feed, return and tab respectively.
The construction "\*(VC3\*(VC" is replaced by the value of variable 3.
The constructions "\*(VCINP_VRB\*VC" and "\*(VCINP_OBJ\*VC" are replaced
by the actual words used in the last input command.
The replacement of "\*(EC" and "\*(VC" symbols is done at output time.
.s1
The
.it field
arguments contain test/result specifications.  There may be up to eight
.it fields
in any action and there may be several of a single type.
The result fields are carried out in left-to-right order.
.nf
.s3
The possible test fields are:
.s1
.it "test field      succeeds if"
t?obj           user has obj ("tool")
t?obj\*(SClval  user has obj ("tool") and is at loc #val
t~obj           user does not have obj ("tool")
t~obj\*(SClval  user does not have obj ("tool") or is not at loc #val
o?obj           obj is here (either being carried or in location)
o?obj\*(SClval  obj is at loc #val (either being carried or in location)
o~obj           obj is not here
o~obj\*(SClval  obj is not at loc #val
c?val           random chance of success, (val% in this case)
s?lval1.sval2   location val1 is in state val2
s~lval1.sval2   location val1 is not in state val2
v?sval1.bval2   variable val1 is equal to val2
v~sval1.bval2   variable val1 is not equal to val2
v<sval1.bval2   variable val1 is less than val2
v>sval1.bval2   variable val1 is greater than val2
.s1
The possible test fields are:
.s1
.it "field           result"
s=lval1.sval2   Set state of location val1 to val2
s+lval1.sval2   Add val2 to state of location val1
s-lval1.sval2   Subtract val2 from state of location val1
v=sval1.bval2   Set variable val1 to val2
v+sval1.bval2   Add val2 to variable val1 (16 bits, two's complement)
v-sval1.bval2   Subtract val2 from variable val1 (= 0 if result is negative)
v*sval1.bval2   multiply variable val1 by val2 (16 bits, two's complement)
v/sval1.bval2   divide variable val1 by val2 (16 bits, two's complement)
o-obj           Object "obj" disappears from this location (if here)
o-obj\*(SClval  Object "obj" disappears from loc val (if there)
o+obj           Object "obj" appears at this location
o+obj\*(SClval  Object "obj" appears at loc val
t-obj           If object "obj" was being carried it disappears
t-obj\*(SClval  If object "obj" was being carried at loc val it disappears
t+obj           Object "obj" is now being carried
t+obj\*(SClval  Object "obj" is now being carried if currently in loc val
c=command       "command" replaces the input command typed by the user
w=file          "file.misc" and "file.wrld" become the new environment
.fi
In the above,
"sval" must be in the range 0 to 127,
"lval" must be in the range 1 to 255, (or whatever the maximum location is),
and "bval" must be in the range -32768 to 32767,
Any of these may be expressions of the form "\*(VCsval\*(VC" where sval is
the number of a variable (in the range 0 to 127).
In the \*(VCsval\*(VC case the value of the variable is substituted at the
time of the action.  For instance "v=1.\*(VC3\*(VC" will save the current value of
variable 3 in variable 1.
The special variables described in the main
.it Wander
documentation file may also be used, (CUR_LOC, MAX_CARRY, etc).

.sh EXAMPLES
A few examples may help: (lines beginning with ":" are comments
and describe the line or lines preceding them.)
.s3
.in 0
.nf
#12	Spaceport Bar
:  short description used for all states of location 12
You are in the Spaceport Bar on Aldebaran III, which is now deserted
except for you and the burly bartender whose eyestalks keep twitching
in your direction.  A large sign hangs over the bar.
:  long description for all states of location 12
     read\*(EC sign  t~dictionary  m="The sign is in some local dialect."
:  if the user has no dictionary and asks to read the sign then this
:  message is output and Wander awaits further input.  (Note no dest)
     read  m="The sign says \*(EC"All humans MUST show papers before leaving!\*(EC""
:  if the user asks to read the sign and gets this far he/she must have
:  the dictionary; the message is output and Wander awaits further input.
:  note the escaped quotes.
     buy\*(EC drink  v>6.4  v-6.5  m="It cost 5 credits but it's good!"
:  if the user types "buy drink" and variable 6 (which, in this case,
:  represents how many credits she/he has) is greater than 4 (i.e.
:  5 or more) then the user gets a drink and loses 5 credits.  Note
:  that if the user types "buy" there will only be a partial match
:  and Wander will ask "buy what?"  If, on the other hand, the user
:  types "buy peanuts" Wander will answer "You can't buy peanuts here".
     buy\*(EC drink  m="A drink costs 5 credits and you ain't got it."
:  this catches the paupers
     buy   o?\*(VCINP_OBJ\*(VC  m="The \*(VCINP_OBJ\*(VC's not for sale."
     buy   m="I'm all out of \*(VCINP_OBJ\*(VC, buddy."
:  if the user asks to buy something else, e.g. drugs, a reasonable
:  answer is given, depending on whether it's there or not.

#12.0
:  Header for state 0 of loc 12.  Since no short description or long
:  description are provided they default to the all-state ones above.
:  the following are state 1 actions
     leave  -1  m="With an amazingly graceful movement for someone\*(EC
his size, the bartender leaps\*(ECnover the bar and clubs you to death\*(EC
while pointing at the sign!"
:  if the user types "leave" or "out" without first showing papers
:  the bartender kills him/her and the dest of -1 ends the game.
:  Note that the message has escaped line-feeds in it for readability
:  which are ignored by Wander.  When real line-feeds are needed,
:  "\*(ECn" is used instead.
     show\*(EC papers  s=12.1  m="He checks you papers and grunts happily."
:  to match this the user must have typed "show papers".  Wander
:  prints the happy grunting bartender message and the state of
:  this location (12) is changed to 1, (it was zero), all further
:  input will be interpreted according to the state 1 actions below

#12.1
:  because no short description is provided here the all-state one
:  (above) is used.
You are in the Spaceport Bar on Aldebaran III, which is now deserted
except for you and the burly bartender who has brought you a drink,
(on the house), after learning that you are a user of UNIX software.
:  the long description for state 1 (followed by state 1 actions)
     leave  13  m="The bartender wriggles a pseudopod in farewell."
:  now it's okay to leave.  The user is moved to location 13.
     ask  o+map  m=\*(EC
"The barkeep feigns ignorance, but leaves something lying on the bar."
:  if the user types anything beginning with "ask", the bartender
:  plays innocent but the "o+map" leaves the map to be taken.
     read map  o?map  m="It's too dark in here to decipher it."
:  we expect the user to try this, but we want to put it off a little.

:  the preceding blank line ends location 12.
#13	Outside Bar
You're just outside a large door with a peeling sign that reads
"      Ladies and Frelks welcome"
To the north an alley winds between this building and another one to
the west while to the east lies a large spaceport.
:  the long description for state 0 (the only one we've used) of loc 13
:  note the " was used to begin a line with spaces.
     *   t?map  ...  m=\*(EC
"The map in your hand glows briefly and  you read the words:
Find Ignarp in Crystal City and tell him 'Axolotyl'.
Then the map turns into sooty ash."
:  If the user has the map, no matter what's been typed, this one
:  is invoked.  The long message is output and then Wander contimues
:  to try to match the user's input (because of the "...").
     north  14  v>6.15  v-6.127  m="As you step into the alley two\*(EC
 ruffians bump into you\*(ECnand make profuse apologies."
:  if the user enters the alley and has more than 15 units of credit the
:  ruffians pick his pocket and the "v-6.127" leaves him with 0 credits.
     north  14
:  this matches "north" when the previous one doesn't
     east   20
     west   18
:  simple movement to other locations
.fi
.i0
.s3
.s1
These few examples can't demonstrate all the possibilities, but they should
provide a beginning understanding of the mechanisms involved.
.s3
One last example --
Let's assume that, rather than express money simply as Cosmic Credits,
we'd like to also have a Credit Card which you must use to establish
your identity.  We'd like the ruffians to lift the card itself and
then use up 20 credits in the bar...
In location 13 the ruffian line would be replaced by this one:
.s1
.in 0
.nf
     north  v>6.30  t?card  o+card\*(SC12  v-6.20  s+12.2  m=\*(EC
"As you step into the alley two ruffians bump into you & make profuse apologies
(better check your pockets)."
:  if you have more than 30 credits (v>6.30)
:  and you're carrying the credit card (t?card)
:  the card is moved to loc 12 (the Bar), and left there (o+card\*(SC12),
:  twenty credits are spent from your account (v-6.20),
:  the state of the bar is increased by 2, (s+12.2)
:  loc/state 12.2 will contain two ruffians and an unfriendly barkeep,
:  loc/state 13.2 will contain two ruffians and a friendly barkeep,
:  finally, the message is printed.
.fi
.i0
.s3
After this line has been executed the user can go back to the
bar and retrieve his credit card from the now-drunk ruffians.
You might want to allow pocket checking with something like:
.in 0
.nf
.s3
     check\*(EC pockets  t?watch ...  m="You're carrying a watch."
:  if you have a watch mention it and continue
     check\*(EC pockets  t?ring  ...  m="You're carrying a ring."
:  if you have a ring mention it and continue
     check\*(EC pockets  t~watch t~ring  ...  m="You're empty-pocketed."
:  if none of the expected items are there mention that and continue
:  (note that the program has a similar built-in response for "inventory"
     check\*(EC pockets  v=6.1  m="Your account now holds one measly credit."
:  being careful about singular/plural
     check\*(EC pockets  m="Your account now holds \*(VC6\*(VC credits."
:  if it gets here then plural is safe
.fi
.i0
.s3
.sh BUGS
I found the last bug, but when I killed it ... Well, the funeral
procession reached three times around the bar.
.sh "SEE ALSO"
misc.doc, wander.doc, wandaid.doc
.CC
