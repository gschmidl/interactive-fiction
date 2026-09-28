use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use File::Temp qw(tempdir);
use MBasic::Program; use MBasic::Linker; use MBasic::Executor;

my $dir = tempdir(CLEANUP => 1);

sub run {
    my ($lines, %opt) = @_;
    my $prog = MBasic::Program->load_lines($lines, '(test)');
    MBasic::Linker->link_program($prog);
    my $out = '';
    MBasic::Executor->run_program($prog, out => sub { $out .= $_[0] }, %opt);
    return $out;
}

# --- write to a file, then read it back ---
my $f1 = "$dir/t1";
run(["10 file #2: \"$f1\"",
     '20 print #2: "hello"',
     '30 print #2: "world"',
     '40 end']);
open my $fh, '<', $f1 or die; my @got = <$fh>; close $fh; chomp @got;
is_deeply(\@got, ['hello','world'], 'print #n writes lines to file');

# --- read the file back with linput ---
is(run(["10 file #2: \"$f1\"",
        '20 linput #2: a$',
        '30 linput #2: b$',
        '40 print a$; "/"; b$',
        '50 end']),
   "hello/world\n", 'linput #n reads lines');

# --- if end #n on a fully-read file ---
is(run(["10 file #2: \"$f1\"",
        '20 linput #2: a$',
        '30 linput #2: b$',
        '40 if end #2 then 70',
        '50 print "more"',
        '60 goto 80',
        '70 print "atend"',
        '80 end']),
   "atend\n", 'if end true after reading all lines');

# --- if end #n on a NONEXISTENT file (verified Multics: true, no error) ---
is(run(['10 file #2: "'.$dir.'/doesnotexist"',
        '20 if end #2 then 40',
        '30 print "notend"',
        '40 print "atend"',
        '50 end']),
   "atend\n", 'if end true on nonexistent file (no error)');

# --- if more #n ---
is(run(["10 file #2: \"$f1\"",
        '20 if more #2 then 40',
        '30 print "empty"',
        '40 print "hasmore"',
        '50 end']),
   "hasmore\n", 'if more true when data present');

# --- input #n with comma-separated fields ---
my $f2 = "$dir/t2";
open my $w, '>', $f2; print $w "alpha,10,beta,20\n"; close $w;
is(run(["10 file #2: \"$f2\"",
        '20 input #2: a$, x, b$, y',
        '30 print a$; x; b$; y',
        '40 end']),
   "alpha 10 beta 20 \n", 'input #n comma fields (mixed types)');

# --- scratch then rewrite ---
run(["10 file #2: \"$f1\"",
     '20 scratch #2',
     '30 print #2: "fresh"',
     '40 end']);
open $fh, '<', $f1; @got = <$fh>; close $fh; chomp @got;
is_deeply(\@got, ['fresh'], 'scratch erases then print rewrites');

# --- reset re-reads from the beginning ---
open $w, '>', $f2; print $w "one\ntwo\n"; close $w;
is(run(["10 file #2: \"$f2\"",
        '20 linput #2: a$',
        '30 reset #2',
        '40 linput #2: b$',
        '50 print a$; b$',
        '60 end']),
   "oneone\n", 'reset #n re-reads from start');

# --- print #n with ; separators (comma-joined record like the game) ---
my $f3 = "$dir/t3";
is(run(["10 file #2: \"$f3\"",
        '20 let s = 500',
        '30 print #2: "09/07/26"; ","; "Name"; ",("; str$(s); " points)"',
        '40 end']) . do { open my $r,'<',$f3; local $/; my $c=<$r>; close $r; $c },
   "09/07/26,Name,( 500 points)\n", 'print #n record with str$ (matches winners format)');

# --- the multi-field write/read round trip (like the game registry) ---
my $f4 = "$dir/t4";
run(["10 file #2: \"$f4\"",
     '20 print #2: "user1" & "," & "5" & "," & "100"',
     '30 end']);
is(run(["10 file #2: \"$f4\"",
        '20 input #2: u$, a, s',
        '30 print u$; a; s',
        '40 end']),
   "user1 5  100 \n", 'write-then-read multi-field record');

# ============================================================================
#  Eager-flush test: writes are visible to an EXTERNAL reader mid-run.
#  On Multics, print buffers into the segment but the bit-count lags until
#  close/abc, so an external `pr` sees "zero length" mid-run (verified live).
#  On Unix we flush eagerly, so cross-process reads see writes immediately --
#  the behavior the game's abc/reopen trick was straining to achieve, and what
#  multiplayer needs.  We verify by peeking at the file from a native builtin
#  called BETWEEN two writes.
# ============================================================================
{
    my $f = "$dir/eager.dat";
    my $seen_midrun = '';
    require MBasic::Interp; require MBasic::Registry;
    my $reg = MBasic::Registry->new;
    $reg->register('peek', sub {
        my ($ctx, $args) = @_;
        # read the file from disk right now (external view)
        if (open my $r, '<', $f) { local $/; $seen_midrun = <$r>; close $r; }
    });
    my $interp = MBasic::Interp->new(registry => $reg);
    $interp->load_helper_lines(
        ["10 file #1: \"$f\"", '20 print #1: "first"',
         '30 call "peek"', '40 print #1: "second"', '50 end'], 'MAIN');
    $interp->{main} = $interp->{programs}{'MAIN'};
    $interp->run(out => sub {});
    like($seen_midrun, qr/first/,
         'write visible to external reader mid-run (eager flush; Unix-native, beats Multics bit-count lag)');
}

done_testing;
