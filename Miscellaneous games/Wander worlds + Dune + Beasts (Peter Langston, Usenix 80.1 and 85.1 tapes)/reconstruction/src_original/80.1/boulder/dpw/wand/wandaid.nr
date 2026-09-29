.so mac
.th WANDAID 2/8/79
.sh NAME
wandaid \*- Fantasy story tool tool
.sh SYNOPSIS
.bd wandaid
oldname
[
.bd newname
] [
.bd -t#
] [
.bd -lint
] [
.bd -n=\c
newfirst[,newstep[,oldfirst[,oldlast]]]]
.sh DESCRIPTION
.it Wandaid
is an adjunct to Wander which
is a tool for writing non-deterministic fantasy "stories".
.sp
.it Wandaid
currently provides either of two functions:
.in +3
1) Resequence location numbers and clean-up (standardize) formatting of
.misc and .wrld files,
.br
2) Do some "lint-picking" on the .misc & .wrld files.  At the moment this
is limited to checking the scope of quoted strings, which has been
particularly troublesome for
.it Wander
authors.
.in -3
.s1
The arguments and their meanings are:
.lp +12 12
oldname\*TName of existing
.t Wander
files, e.g. to process "foo.misc" and "foo.wrld", oldname would be "foo".
.lp +12 12
newname\*TThe name to be used for the files created by resequencing.
.it Wandaid
is not sufficiently bug-free for me to feel safe about wiping out the
oldfiles and replacing them with the newfiles automatically.
(CAVEAT EMPTOR!)
.s3
If no newname is specified no files are created, (common with "-lint", below).
.lp +12 12
-t#\*TThis is the usual trace flag.  "#" should be replaced with an integer
whose size determines, to some degree, the amount of gratuitous garbage
the program will spew out.
.lp +12 12
-lint\*TThis asks for lint-picking.  Diagnostic lines beginning "LINT:"
are produced by any weirdness that the program detects.
.lp +12 12
-n=#,#,#,#\*TThese are the values used by the resequencing.
Newfirst is the first location number assigned by resequencing;
newstep is the increment between new numbers generated;
oldfirst is the first of the existing location numbers that will be affected;
oldlast is the last of the existing location numbers that will be affected.
The defaults are equivalent to -n=1,1,1,1212.
.sh "SEE ALSO"
wander.doc wrld.doc, misc.doc
.CC
