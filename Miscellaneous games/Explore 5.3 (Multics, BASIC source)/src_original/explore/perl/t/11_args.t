use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib"; use lib ($ENV{MBASIC_LIB} // ());
use Test::More;
use File::Temp qw(tempdir);
use File::Copy;
use Config;

# The command line carries two vocabularies: this runner's double-dash options
# and the game's authentic single-dash Multics control arguments, which reach
# the BASIC through cnt and arg$(n).  These tests pin the split.
#
# Previously the parser stopped scanning at the first argument that was not
# double-dash, so a runner option written after a game argument was silently
# not parsed and was passed to the game instead, inflating cnt.

BEGIN {
    eval { require MBasic::Interp; require MBasic::Registry; 1 }
      or plan skip_all => 'MBasic interpreter not available';
}

my $share = "$FindBin::Bin/../share";
plan skip_all => "game data (share/) not found" unless -f "$share/explore.basic";

my $runner = "$FindBin::Bin/../explore";
plan skip_all => "runner not found" unless -f $runner;

my $t   = tempdir(CLEANUP => 1);
my $var = tempdir(CLEANUP => 1);
for my $f (glob "$share/*") { my ($b) = $f =~ m{([^/]+)$}; copy($f, "$t/$b"); }
open my $rw, '>', "$t/explore.rwdir" or die $!; print $rw "none\n"; close $rw;

# A stand-in for the game that simply reports what it was handed.  Using a
# probe rather than explore.basic keeps the assertions about the argument
# vector itself, independent of what the game does with any given argument.
open my $p, '>', "$t/probe.basic" or die $!;
print $p <<'BASIC';
10 print "cnt=";cnt
20 for i = 1 to cnt
30 print "[";arg$(i);"]"
40 next i
50 end
BASIC
close $p;

# Run the real runner as a subprocess and return what the probe printed.
sub probe {
    my @args = @_;
    local $ENV{PERL5LIB} = join $Config{path_sep},
        "$FindBin::Bin/../lib", ($ENV{MBASIC_LIB} // ()), ($ENV{PERL5LIB} // ());
    local $ENV{EXPLORE_VARDIR} = $var;
    my @cmd = ($^X, $runner, '--basic', "$t/probe.basic", '--share', $t, @args);
    # merge the child's stderr into the pipe so diagnostics are captured (and
    # assertable) rather than printed over the test output
    my $pid = open my $fh, '-|';
    die "cannot fork: $!" unless defined $pid;
    unless ($pid) { open STDERR, '>&', \*STDOUT; exec @cmd; die "exec: $!"; }
    my $out = do { local $/; <$fh> };
    close $fh;
    $out = '' unless defined $out;
    my ($cnt)  = $out =~ /cnt=\s*(\d+)/;
    my @got    = $out =~ /\[([^\]]*)\]/g;
    return (defined $cnt ? $cnt : -1, \@got, $out);
}

# --- the game sees its own arguments, and only those ---
{
    my ($cnt, $args) = probe('-brief');
    is($cnt, 1, 'one game argument gives cnt 1');
    is_deeply($args, ['-brief'], 'the game argument arrives intact');
}

# --- a runner option AFTER a game argument is still parsed, not passed on ---
{
    my ($cnt, $args) = probe('-brief', '--var', $var);
    is($cnt, 1, 'a trailing runner option does not inflate cnt');
    is_deeply($args, ['-brief'], 'the runner option is not handed to the game');
}

# --- options may be interleaved anywhere ---
{
    my ($cnt, $args) = probe('-brief', '--var', $var, '-ts');
    is($cnt, 2, 'options between game arguments are removed');
    is_deeply($args, ['-brief', '-ts'], 'game arguments keep their order');
}

# --- order and adjacency survive, which -pathname/-modes/-abbrev rely on ---
{
    my ($cnt, $args) = probe('-modes', 'x,y', '-brief');
    is($cnt, 3, 'a value-taking game argument keeps its value');
    is_deeply($args, ['-modes', 'x,y', '-brief'],
              'the value stays adjacent to its argument, in order');
}

# --- "--" ends option processing ---
{
    my ($cnt, $args) = probe('--', '-brief');
    is($cnt, 1, '"--" passes the remainder to the game');
    is_deeply($args, ['-brief'], 'argument after "--" arrives intact');

    my ($cnt2, $args2) = probe('--', '-pn', '--odd-name');
    is($cnt2, 2, '"--" covers a value that itself begins with "--"');
    is_deeply($args2, ['-pn', '--odd-name'],
              'a double-dash game value is not claimed by the runner');
}

# --- no game arguments at all ---
{
    my ($cnt, $args) = probe();
    is($cnt, 0, 'cnt is 0 when only runner options are given');
    is_deeply($args, [], 'and the game gets an empty argument vector');
}

# --- an unknown double-dash option is still an error, not a game argument ---
{
    my (undef, undef, $out) = probe('--no-such-option');
    unlike($out, qr/cnt=/, 'an unknown runner option does not reach the game');
    like($out, qr/unknown option --no-such-option/,
         'and it is reported rather than silently ignored');
}

done_testing;
