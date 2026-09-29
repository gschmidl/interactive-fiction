use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use File::Temp qw(tempdir);
use MBasic::Interp;
use MBasic::Program; use MBasic::Linker; use MBasic::Executor;

# Regression tests for the issues found in the 2026 external code review.
# Each is labelled with its review finding number.

# run source (\@lines) capturing output; returns the output string.
sub run {
    my ($lines, %opt) = @_;
    my $prog = MBasic::Program->load_lines($lines, '(test)');
    MBasic::Linker->link_program($prog);
    my $out = '';
    MBasic::Executor->run_program($prog, out => sub { $out .= $_[0] }, %opt);
    return $out;
}
# try to load+run; return the die message ('' on success).
sub err {
    my ($lines, %opt) = @_;
    my $out = '';
    my $ok = eval {
        my $prog = MBasic::Program->load_lines($lines, '(test)');
        MBasic::Linker->link_program($prog);
        MBasic::Executor->run_program($prog, out => sub { $out .= $_[0] }, %opt);
        1;
    };
    return $ok ? '' : $@;
}

# ---- #6: a zero-trip inner for must not corrupt the enclosing loop ----
is(run(['10 for i = 1 to 3','20 for k = 1 to 0','30 print "inner"','40 next k',
        '50 print "i=";i','60 next i','70 end']),
   "i= 1 \ni= 2 \ni= 3 \n", '#6 zero-trip inner for does not leak its frame');

# ---- #9: stop/end inside a BASIC sub terminates the whole program ----
{
    my $i = MBasic::Interp->new;
    $i->load_helper_lines(['10 sub "ss"','20 print "in sub"','30 stop','40 subend'],'ss');
    $i->{main} = MBasic::Program->load_lines(
        ['10 call "ss"','20 print "after"','30 end'],'(m)');
    MBasic::Linker->link_program($i->{main}); $i->_index_subs($i->{main});
    my $out=''; $i->run(out=>sub{$out.=$_[0]});
    is($out, "in sub\n", '#9 stop in sub halts the whole program');
}

# ---- #10: an indented rem is a comment, not a lex error ----
is(run(['10   rem  50% off! see notes','20 print "ok"','30 end']),
   "ok\n", '#10 indented rem tolerated');

# ---- #11: trailing tokens are rejected (fail loudly) ----
like(err(['10 goto 30 junk','20 print "x"','30 end']),
     qr/extra tokens/, '#11 trailing garbage after goto rejected');
like(err(['10 if 1=1 then 40 else 50','40 print "a"','50 end']),
     qr/extra tokens/, '#11 unsupported if-else rejected, not mis-run');

# ---- #12: rnd is repeatable across runs and doesn't touch host rand() ----
is(run(['10 print rnd','20 print rnd','30 end']),
   run(['10 print rnd','20 print rnd','30 end']),
   '#12 rnd sequence repeats across runs (no randomize)');
{
    srand(12345); my @host = (rand(), rand());
    srand(12345); rand();               # advance host stream once
    run(['10 randomize','20 print rnd','30 end']);   # must NOT reseed host
    my @after = (rand());
    srand(12345); rand(); my @expect = (rand());
    is($after[0], $expect[0], '#12 randomize does not reseed the host rand() stream');
}

# ---- #13: math domain errors are authentic BASIC messages, not raw Perl ----
like(err(['10 print sqr(0-1)','20 end']),
     qr/^Square root of negative number \(line 10\)/, '#13 sqr(-1) authentic message');
like(err(['10 print 0 ^ (0-1)','20 end']),
     qr/^Negative power of zero \(line 10\)/, '#13 0^-1 authentic message');

# ---- #14: numeric/string type mismatch is a BASIC error at the BASIC line ----
like(err(['10 print a$ + 0','20 end']),
     qr/^Mixed string and numeric expression \(line 10\)/,
     '#14 type mismatch reported with BASIC line, no leaked Perl warning');

# ---- #15: terminal input enforces "Not enough input, add more" ----
{
    my @q = ('5');
    like(err(['10 input a,b,c','20 end'], input => sub { @q ? shift @q : undef }),
         qr/^Not enough input, add more/, '#15 short terminal input errors');
}
# and re-prompting completes when more input arrives
{
    my @q = ('5', '6,7');
    is(run(['10 input a,b,c','20 print a;b;c','30 end'],
           input => sub { @q ? shift @q : undef }),
       "? Not enough input, add more\n?  5  6  7 \n",
       '#15 input re-prompts and completes');
}

# ---- #3: a BASIC program cannot OOM-abort the host ----
like(err(['10 dim a(100000000)','20 end']), qr/^Out of room/, '#3 huge dim capped');
like(err(['10 print tab(1000000000);"x"','20 end']),
     qr/^Invalid margin/, '#3 huge tab capped');
{   # unbounded recursion -> loud error, not a host stack overflow
    my $i = MBasic::Interp->new;
    $i->load_helper_lines(['10 sub "rr"','20 call "rr"','30 subend'],'rr');
    $i->{main} = MBasic::Program->load_lines(['10 call "rr"','20 end'],'(m)');
    MBasic::Linker->link_program($i->{main}); $i->_index_subs($i->{main});
    my $ok = eval { $i->run(out=>sub{}); 1 };
    like($@, qr/Stack space exhausted/, '#3 unbounded recursion capped');
}

# ---- #1: a call name cannot traverse the filesystem ----
like(err(['10 call "../secret/creds"','20 end']),
     qr/Invalid subroutine name/, '#1 traversal call rejected at parse time');
{
    my $i = MBasic::Interp->new(search_path => ['/tmp']);
    my ($kind) = $i->resolve_call('../secret/creds');
    is($kind, 'undef', '#1 resolve_call guards traversal names even past the parser');
}

# ---- #2/#7: file writes are atomic, symlink-safe, and flushed on close ----
{
    my $dir = tempdir(CLEANUP => 1);
    # #2 symlink target is not clobbered
    my $target = "$dir/precious"; open my $f,'>',$target; print $f "keep\n"; close $f;
    my $link = "$dir/l.data"; symlink($target,$link) or die;
    run(["10 file #1: \"$link\"", '20 scratch #1', '30 print #1: "pwned"', '40 end']);
    open my $r,'<',$target; my $c=<$r>; close $r; chomp $c;
    is($c, 'keep', '#2 planted symlink target is not clobbered');
    ok(!-l $link, '#2 the symlink was replaced by a real file, not followed');

    # #7 the last write (trailing ; = no newline) reaches disk consistently
    my $p = "$dir/o.data";
    run(["10 file #1: \"$p\"", '20 print #1: "x";', '30 end']);
    open my $r2,'<',$p; local $/; my $d=<$r2>; close $r2;
    is($d, 'x', '#7 final buffered write is flushed to disk on close');
}

# ---- #8: file/program reads are immune to a caller's $/ ----
{
    my $dir = tempdir(CLEANUP => 1);
    my $p = "$dir/three"; open my $f,'>',$p; print $f "a\nb\nc\n"; close $f;
    local $/ = undef;    # hostile record separator
    is(run(["10 file #1: \"$p\"", '20 linput #1: a$', '30 linput #1: b$',
            '40 print a$;b$', '50 end']),
       "ab\n", '#8 file read localizes $/');
}

# ---- fidelity: reject $-suffixed for var; asc(""); str$ E-format; chan 1..4 ----
like(err(['10 for a$ = 1 to 2','20 next a$','30 end']),
     qr/Numeric variable required/, 'fidelity: $-suffixed for var rejected');
like(err(['10 print asc("")','20 end']),
     qr/Invalid "ASC" function arg/, 'fidelity: asc("") errors');
is(run(['10 print str$(200000000)','20 end']),
   " 2E+08\n", 'fidelity: str$ uses Multics uppercase-E scientific format');
like(err(['10 file #99: "/tmp/x"','20 end']),
     qr/Invalid file number/, 'fidelity: channel numbers restricted to 1..4');

# ---- fidelity: duplicate sub in one unit is an error; load_lines is non-mutating ----
like(err(['10 sub "dup"','20 subend','30 sub "dup"','40 subend']),
     qr/defined more than once/, 'fidelity: duplicate sub rejected');
{
    my @src = ("10 print \"hi\"\n", "20 end\n");   # note trailing newlines
    MBasic::Program->load_lines(\@src, '(t)');
    is($src[0], "10 print \"hi\"\n", 'fidelity: load_lines does not chomp caller array');
}

done_testing;
