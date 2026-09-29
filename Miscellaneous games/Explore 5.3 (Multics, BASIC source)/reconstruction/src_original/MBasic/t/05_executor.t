use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use MBasic::Program; use MBasic::Linker; use MBasic::Executor;

# run a small program (array of source lines) and capture its output
sub run {
    my (@lines) = @_;
    my $prog = MBasic::Program->load_lines(\@lines, '(test)');
    MBasic::Linker->link_program($prog);
    my $out = '';
    MBasic::Executor->run_program($prog, out => sub { $out .= $_[0] });
    return $out;
}

# --- basic print ---
is(run('10 print "hello"','20 end'), "hello\n", 'print string');

# --- number print format (leading sign-blank + trailing blank) ---
is(run('10 print 42','20 end'), " 42 \n", 'print number: leading+trailing blank');
is(run('10 print "x";42;"y"','20 end'), "x 42 y\n", 'print ; number embedded');

# --- let + arithmetic ---
is(run('10 let a = 6','20 let b = 7','30 print a * b','40 end'), " 42 \n", 'let + multiply');

# --- goto ---
is(run('10 goto 30','20 print "skip"','30 print "here"','40 end'), "here\n", 'goto skips');

# --- if then ---
is(run('10 let x = 5','20 if x > 3 then 40','30 print "no"','40 print "yes"','50 end'),
   "yes\n", 'if true jumps');
is(run('10 let x = 1','20 if x > 3 then 40','30 print "no"','40 end'),
   "no\n", 'if false falls through');

# --- for/next loop ---
is(run('10 for i = 1 to 3','20 print i;','30 next i','40 print "done"','50 end'),
   " 1  2  3 done\n", 'for loop 1..3');

# --- for/next that should not execute (from > to) ---
is(run('10 for i = 5 to 1','20 print "body"','30 next i','40 print "after"','50 end'),
   "after\n", 'for loop skipped when from>to');

# --- for with step ---
is(run('10 for i = 10 to 1 step -3','20 print i;','30 next i','40 end'),
   " 10  7  4  1 ", 'for step -3 (trailing ; = no newline)');

# --- gosub/return ---
is(run('10 gosub 100','20 print "back"','30 end','100 print "sub"','110 return'),
   "sub\nback\n", 'gosub/return');

# --- on goto ---
is(run('10 let k = 2','20 on k goto 100,200,300','100 print "one"','110 end',
       '200 print "two"','210 end','300 print "three"','310 end'),
   "two\n", 'on k goto selects 2nd');

# --- data/read ---
is(run('10 read a$','20 read b$','30 print a$;b$','40 data "foo","bar"','50 end'),
   "foobar\n", 'data/read strings');

# --- array via dim + read (like exp_day_) ---
is(run('10 dim d$(7)','20 for i=1 to 3','30 read d$(i)','40 next i',
       '50 print d$(1);d$(2);d$(3)','60 data "a","b","c"','70 end'),
   "abc\n", 'dim + read into array');

# --- string concat in print ---
is(run('10 let a$="foo"','20 print a$ & "bar"','30 end'), "foobar\n", 'concat print');

# --- nested computation (a real-ish fragment) ---
is(run('10 let s = 0','20 for i = 1 to 5','30 let s = s + i','40 next i',
       '50 print s','60 end'), " 15 \n", 'sum 1..5 = 15');

# --- comma print zones (15-col) ---
is(run('10 print "a","b"','20 end'), "a" . (" " x 14) . "b\n", 'comma zone to col 15');


# --- on out of range is an error (Multics errata 101), not fall-through ---
eval { run('10 let k = 5','20 on k goto 100,200','100 print "a"','200 print "b"','300 end') };
like($@, qr/On evaluated out of range/, 'on with out-of-range selector errors (errata 101)');

done_testing;
