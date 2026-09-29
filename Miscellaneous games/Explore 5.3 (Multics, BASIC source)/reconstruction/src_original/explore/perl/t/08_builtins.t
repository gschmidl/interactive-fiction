use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib"; use lib ($ENV{MBASIC_LIB} // ());
use Test::More;
use File::Temp qw(tempdir);
use MBasic::Interp; use MBasic::Registry;
use Explore::Builtins;

my $dir = tempdir(CLEANUP => 1);
$Explore::Builtins::ROOT = $dir;

sub run {
    my ($lines, %opt) = @_;
    my $reg = MBasic::Registry->new;
    Explore::Builtins::register_all($reg);
    my $interp = MBasic::Interp->new(registry => $reg, %opt);
    $interp->load_helper_lines($lines, 'MAIN');
    $interp->{main} = $interp->{programs}{'MAIN'};
    my $out = '';
    $interp->run(out => sub { $out .= $_[0] });
    return $out;
}

# --- path translation ---
is(Explore::Builtins::mult_path('>site>explore_dir>x'), "$dir/site/explore_dir/x",
   'Multics > path -> Unix / under ROOT');
is(Explore::Builtins::mult_path('relative/x'), 'relative/x', 'relative path unchanged');

# --- exp_canwrite_ : writable file -> 1 ---
my $wf = "$dir/writable"; open my $w,'>',$wf; print $w "x"; close $w;
is(run(['10 call "exp_canwrite_": ">writable", f', '20 print f', '30 end']),
   " 1 \n", 'exp_canwrite_ on writable file -> 1');

# --- exp_canwrite_ : read-only file -> 0 (skipped as root: perms don't block root) ---
SKIP: {
    skip "running as root: chmod perms do not block -w", 1 if $> == 0;
    my $rf = "$dir/readonly"; open my $w2,'>',$rf; print $w2 "x"; close $w2; chmod 0444, $rf;
    is(run(['10 call "exp_canwrite_": ">readonly", f', '20 print f', '30 end']),
       " 0 \n", 'exp_canwrite_ on read-only file -> 0');
    chmod 0644, $rf;
}

# --- exp_lock_ acquires (status 1), exp_unlock_ releases ---
my $lf = "$dir/thing.lock"; open $w,'>',$lf; close $w;
is(run(['10 call "exp_lock_": ">thing.lock", 5, st', '20 print st',
        '30 call "exp_unlock_": ">thing.lock"', '40 end']),
   " 1 \n", 'exp_lock_ acquires -> status 1');

# --- exp_lock_ on a read-only lock file -> -1 (skipped as root) ---
SKIP: {
    skip "running as root: chmod perms do not block -w", 1 if $> == 0;
    my $rl = "$dir/ro.lock"; open my $w3,'>',$rl; close $w3; chmod 0444, $rl;
    is(run(['10 call "exp_lock_": ">ro.lock", 5, st', '20 print st', '30 end']),
       "-1 \n", 'exp_lock_ on read-only lock file -> -1 (cannot lock)');
    chmod 0644, $rl;
}

# --- exp_lock_ mutual exclusion: hold in a child, TRY in parent gets busy(0) ---
{
    my $ef = "$dir/excl.lock"; open $w,'>',$ef; close $w;
    # hold the lock directly via the builtin machinery
    require Fcntl; Fcntl->import(':flock');
    open my $held, '+>>', $ef or die;
    flock($held, Fcntl::LOCK_EX() | Fcntl::LOCK_NB()) or die "couldn't pre-lock";
    # now the interpreter tries -> should get 0 (busy)
    my $got = run(['10 call "exp_lock_": ">excl.lock", 5, st', '20 print st', '30 end']);
    is($got, " 0 \n", 'exp_lock_ gets busy(0) when lock already held externally');
    flock($held, Fcntl::LOCK_UN()); close $held;
}

# --- exp_home_ sets h$ to the player's real home dir (per-user files) ---
{
    local $ENV{HOME} = '/home/tester';
    is(run(['10 call "exp_home_": h$', '20 print "["; h$; "]"', '30 end']),
       "[/home/tester]\n",
       'exp_home_ sets h$ to the user home (h$ & ">file" -> ~/file)');
}

# --- quit_off / quit_on set the SIGINT handler ---
run(['10 call "quit_off"', '20 end']);
is($SIG{INT}, 'IGNORE', 'quit_off sets SIGINT to IGNORE');
run(['10 call "quit_on"', '20 end']);
is($SIG{INT}, 'DEFAULT', 'quit_on restores SIGINT default');

# --- exp_getpw_ reads a word (non-tty: from a fed input) ---
{
    # feed STDIN via a temp; exp_getpw_'s non-tty fallback reads a line
    my $inf = "$dir/pwin"; open my $iw,'>',$inf; print $iw "secret\n"; close $iw;
    open my $save, '<&', \*STDIN;
    open STDIN, '<', $inf;
    my $out = run(['10 call "exp_getpw_": "Word: ", w$', '20 print "got:"; w$', '30 end']);
    open STDIN, '<&', $save;
    like($out, qr/got:secret/, 'exp_getpw_ reads the word (non-tty fallback)');
}

# ============================================================================
#  Command stubs
# ============================================================================
{
    # create -> touch
    run(['10 call "create": ">newfile"', '20 end']);
    ok(-e "$dir/newfile", 'create makes an empty file');

    # delete_force -> unlink
    run(['10 call "delete_force": ">newfile", "-brief"', '20 end']);
    ok(!-e "$dir/newfile", 'delete_force removes the file (ignores -brief)');

    # sort_seg -> sort file in place
    my $sf = "$dir/sortme"; open my $w,'>',$sf; print $w "cherry\napple\nbanana\n"; close $w;
    run(['10 call "sort_seg": ">sortme", "-replace"', '20 end']);
    open my $r,'<',$sf; my @l=<$r>; close $r; chomp @l;
    is_deeply(\@l, ['apple','banana','cherry'], 'sort_seg sorts the file');

    # abc -> no-op (does not error, does not change the file)
    my $af="$dir/abctest"; open $w,'>',$af; print $w "data\n"; close $w;
    run(['10 call "abc": ">abctest"', '20 end']);
    open $r,'<',$af; my $c=do{local $/;<$r>}; close $r;
    is($c, "data\n", 'abc is a no-op (file unchanged)');

    # do -> command execution (echo, captured to output sink)
    my $out = run(['10 call "do": "echo hello_from_do"', '20 end']);
    like($out, qr/hello_from_do/, 'do executes a shell command');
}

# --- mult_path containment: reject parent-directory traversal ---
{
    my $err = '';
    eval { Explore::Builtins::mult_path('>site>explore_dir>..>..>..>etc>passwd'); 1 }
        or $err = $@;
    like($err, qr/path traversal/, 'mult_path rejects ">..>" traversal');
    $err = '';
    eval { Explore::Builtins::mult_path('a/../../etc/passwd'); 1 } or $err = $@;
    like($err, qr/path traversal/, 'mult_path rejects Unix "../" traversal');
    # a legitimate game path is unaffected
    is(Explore::Builtins::mult_path('>site>explore_dir>hours.data'),
       "$dir/site/explore_dir/hours.data", 'mult_path passes a normal path');
}

# --- exp_lock_ is re-entrant in one process (no handle leak / lost lock) ---
{
    my $lf2 = "$dir/reent.lock"; open my $x,'>',$lf2; close $x;
    my $got = run(['10 call "exp_lock_": ">reent.lock", 5, s1',
                   '20 call "exp_lock_": ">reent.lock", 5, s2',
                   '30 print s1; s2',
                   '40 call "exp_unlock_": ">reent.lock"', '50 end']);
    is($got, " 1  1 \n", 'exp_lock_ re-lock in same process returns 1 without leaking');
}

# --- create / sort_seg refuse a symlinked target (O_NOFOLLOW) ---
SKIP: {
    my $victim = "$dir/nofollow_victim"; open my $v,'>',$victim; print $v "keep\n"; close $v;
    my $link = "$dir/nf.link";
    skip "symlink unsupported here", 2 unless eval { symlink($victim,$link); 1 } && -l $link;
    run(['10 call "create": ">nf.link"', '20 end']);       # must not write through the link
    open my $r,'<',$victim; my $c=<$r>; close $r; chomp $c;
    is($c, 'keep', 'create does not follow a symlink (O_NOFOLLOW)');
    run(['10 call "sort_seg": ">nf.link"', '20 end']);     # must not sort through the link
    open my $r2,'<',$victim; my $c2=<$r2>; close $r2; chomp $c2;
    is($c2, 'keep', 'sort_seg does not follow a symlink (O_NOFOLLOW)');
}

done_testing;
