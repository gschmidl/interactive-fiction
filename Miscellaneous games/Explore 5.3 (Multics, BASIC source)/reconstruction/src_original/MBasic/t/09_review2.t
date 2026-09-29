use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use File::Temp qw(tempdir);
use MBasic::Interp; use MBasic::Env;
use MBasic::Program; use MBasic::Linker; use MBasic::Executor;

# Regression tests for the SECOND review pass (problems the first round of
# fixes introduced or under-covered).

sub run {
    my ($lines, %opt) = @_;
    my $prog = MBasic::Program->load_lines($lines, '(test)');
    MBasic::Linker->link_program($prog);
    my $out = '';
    MBasic::Executor->run_program($prog, out => sub { $out .= $_[0] }, %opt);
    return $out;
}
sub err {
    my ($lines, %opt) = @_;
    my $ok = eval {
        my $prog = MBasic::Program->load_lines($lines, '(test)');
        MBasic::Linker->link_program($prog);
        MBasic::Executor->run_program($prog, out => sub {}, %opt);
        1;
    };
    return $ok ? '' : $@;
}

# ==== R2-1: a pre-planted (predictable) temp name must not be followed ====
# This is the FIRST file write in this process, so File.pm's temp sequence is
# at its initial value and the first candidate name is deterministic.
{
    my $dir = tempdir(CLEANUP => 1);
    my $victim = "$dir/victim"; open my $v, '>', $victim; print $v "SECRET\n"; close $v;
    my $data = "$dir/data";
    my $first_tmp = sprintf('%s/.mbtmp.%d.%d.%d', $dir, $$, 1, 1);
    symlink($victim, $first_tmp) or die "symlink: $!";
    my $e = err(["10 file #1: \"$data\"", '20 print #1: "clean"', '30 end']);
    open my $r, '<', $victim; my $vc = <$r>; close $r; chomp $vc;
    my $dc = ''; if (open my $g, '<', $data) { $dc = <$g>; close $g; chomp $dc; }
    is($vc, 'SECRET', 'R2-1 pre-planted temp symlink does not clobber its target');
    is($dc, 'clean',  'R2-1 write still succeeds (retries past the planted temp name)');
    is($e, '',        'R2-1 no error raised');
}

# ==== R2-2: the target file's permission bits survive a write ====
{
    my $dir = tempdir(CLEANUP => 1);
    my $p = "$dir/scores"; open my $f, '>', $p; print $f "old\n"; close $f;
    chmod 0664, $p;
    my $m0 = (stat $p)[2] & 07777;
    run(["10 file #1: \"$p\"", '20 print #1: "new"', '30 end']);
    my $m1 = (stat $p)[2] & 07777;
    is($m1, $m0, 'R2-2 file mode preserved across the atomic write');

    # R3: a NEW file honors the process umask (not the temp's private 0600), so
    # a shared-game file is not created owner-only, locking other players out.
    my $um = umask; umask 0002;
    my $np = "$dir/newshared";
    run(["10 file #1: \"$np\"", '20 print #1: "hi"', '30 end']);
    is((stat $np)[2] & 07777, 0664, 'R3 new file created 0664 under umask 0002');
    umask $um;
}

# ==== R2-3: writable file in a NON-writable directory (in-place fallback) ====
SKIP: {
    skip "cannot exercise dir-permission fallback as root", 1 if $> == 0;
    my $dir = tempdir(CLEANUP => 1);
    my $p = "$dir/state"; open my $f, '>', $p; print $f "seed\n"; close $f;
    chmod 0666, $p;
    chmod 0555, $dir;    # directory not writable: temp create must fail -> fallback
    my $e = err(["10 file #1: \"$p\"", '20 print #1: "updated"', '30 end']);
    chmod 0755, $dir;    # restore for cleanup
    my @lines; if (open my $r, '<', $p) { @lines = <$r>; close $r; chomp @lines; }
    is($e, '', 'R2-3 in-place fallback writes a pre-created file in a read-only dir');
    ok((grep { $_ eq 'updated' } @lines), 'R2-3 the update reached the file');
}

# ==== R2-4: __WARN__ handler is scoped and delegates ====
# (a) a native builtin doing its own string arithmetic must NOT be blamed on the
#     BASIC line, and the embedder's warn handler must still see the warning.
{
    my @caught;
    local $SIG{__WARN__} = sub { push @caught, $_[0] };
    my $i = MBasic::Interp->new;
    $i->{registry}->register('lookup', sub { my ($ctx,$a)=@_; my $n = $a->[0]->get + 1; });
    $i->{main} = MBasic::Program->load_lines(
        ['10 let k$ = "north"', '20 call "lookup": k$, r', '30 print "ok"', '40 end'], '(m)');
    MBasic::Linker->link_program($i->{main}); $i->_index_subs($i->{main});
    my $out = ''; my $ok = eval { $i->run(out => sub { $out .= $_[0] }); 1 };
    ok($ok && $out eq "ok\n", 'R2-4a builtin string-arith does not abort the BASIC program');
    ok((grep { /isn't numeric/ } @caught),
       'R2-4a embedder warn handler still receives the builtin warning (not swallowed)');
}
# (b) a BASIC-level string/number mix is still converted to the authentic error.
like(err(['10 print a$ + 0', '20 end']),
     qr/^Mixed string and numeric expression \(line 10\)/,
     'R2-4b BASIC-level type mismatch still reported at the BASIC line');

# ==== R2-5: RNG is repeatable, 32-bit safe, and has no degenerate first draw ==
{
    my $prog1 = ['10 for i=1 to 3', '20 print rnd', '30 next i', '40 end'];
    is(run($prog1), run($prog1), 'R2-5 rnd sequence repeats across runs');

    # the underlying Park-Miller/Schrage step is exact (this is the value that
    # would be lost to double-rounding on a 32-bit-IV Perl if not for Schrage).
    is(MBasic::Env::_rng_step({ seed => 1 }), 16807,
       'R2-5 Park-Miller step is exact (seed 1 -> 16807, 32-bit safe)');

    # the first *user-visible* draw is warmed, so it is NOT the degenerate tiny
    # value that made int(6*rnd)+1 always land on 1.
    my $first = MBasic::Env->new->rnd;
    ok($first > 0.01, 'R2-5 first rnd is not the degenerate tiny opening value');

    # first-draw outcomes across distinct seeds are varied (not all lowest).
    my %seen;
    for my $s (1..8) {
        my $e = MBasic::Env->new(rng => MBasic::Env::_fresh_rng($s));
        $seen{ int(6 * $e->rnd) + 1 } = 1;
    }
    ok(keys(%seen) > 1, 'R2-5 int(6*rnd)+1 first draw varies across seeds');

    my $bad = 0;
    my $e2 = MBasic::Env->new;
    for (1..1000) { my $x = $e2->rnd; $bad++ if $x < 0 || $x >= 1; }
    is($bad, 0, 'R2-5 1000 draws all lie in [0,1)');
}

# ==== R2-6: the same sub defined in two units on the path is rejected ====
{
    my $i = MBasic::Interp->new;
    $i->load_helper_lines(['10 sub "dup"', '20 subend'], 'unitA');
    my $ok = eval { $i->load_helper_lines(['10 sub "dup"', '20 subend'], 'unitB'); 1 };
    like($@, qr/defined in more than one unit/,
         'R2-6 cross-unit duplicate sub rejected (no silent load-order win)');
}

# ==== R2-7: array storage is capped in aggregate, not just per-array ====
{
    local $MBasic::Env::MAX_TOTAL_CELLS = 100;   # small, so the test is cheap
    local $MBasic::Env::MAX_ARRAY_CELLS = 100;
    like(err(['10 dim a(60)', '20 dim b(60)', '30 end']),
         qr/^Out of room \(total array storage/,
         'R2-7 aggregate array-cell cap enforced across separate dims');
}

# ==== name regex allows Multics seg$entry, still rejects traversal ====
{
    my $i = MBasic::Interp->new;
    $i->{registry}->register('get$val', sub { my ($ctx,$a)=@_; $a->[0]->set(42); });
    $i->{main} = MBasic::Program->load_lines(
        ['10 call "get$val": r', '20 print r', '30 end'], '(m)');
    MBasic::Linker->link_program($i->{main}); $i->_index_subs($i->{main});
    my $out = ''; my $ok = eval { $i->run(out => sub { $out .= $_[0] }); 1 };
    is($out, " 42 \n", 'name regex: a seg$entry builtin name is callable');
}
like(err(['10 call "../secret/x": a', '20 end']),
     qr/Invalid subroutine name/, 'name regex: traversal name still rejected');

# ==== R4: `next` closes inner loops abandoned by a jump out of them ====
# (Multics runtime behavior; the strict top-of-stack check faulted legal
# programs such as Explore's abbrev expander, which jumps out of an inner FOR.)
{
    # jump out of the inner j loop, then hit the outer `next i`
    is(run(['10 for i=1 to 3','20 for j=1 to 5','30 if j=1 then 50',
            '40 next j','50 next i','60 print i','70 end']),
       " 4 \n", 'R4 outer next closes an abandoned inner loop and keeps iterating');
    # normal nested loops are unaffected
    is(run(['10 for i=1 to 2','20 for j=1 to 2','30 print i;j;',
            '40 next j','50 next i','60 end']),
       " 1  1  1  2  2  1  2  2 ", 'R4 normal nested for/next still correct');
    # a next for a variable with no open loop is still an error
    like(err(['10 for i=1 to 2','20 next k','30 end']),
         qr/^For-next mismatch/, 'R4 next of an unopened variable still errors');
    like(err(['10 next i','20 end']),
         qr/^Next without for/, 'R4 next with no loop at all still errors');
}

done_testing;
