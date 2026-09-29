use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib"; use lib ($ENV{MBASIC_LIB} // ());
use Test::More;
use File::Temp qw(tempdir);
use File::Copy;
use Time::Local qw(timelocal);

# This test runs the actual game far enough to prove it loads and executes:
# it starts the game, issues a couple of commands, and quits, asserting the
# expected output appears.  Requires the MBasic distribution to be available.
BEGIN {
    eval { require MBasic::Interp; require MBasic::Registry; 1 }
      or plan skip_all => 'MBasic interpreter not available';
}
require Explore::Builtins;
no warnings "once";

# locate the game data: the share/ dir in the distribution
my $share = "$FindBin::Bin/../share";
plan skip_all => "game data (share/) not found" unless -f "$share/explore.basic";

# Run read-only: copy the share data to a temp dir and set explore.rwdir to
# "none" so the game reads hours.data/winners.data from that dir (no /var
# dependency in the test).
my $t = tempdir(CLEANUP => 1);
for my $f (glob "$share/*") { my ($b) = $f =~ m{([^/]+)$}; copy($f, "$t/$b"); }
open my $rw, '>', "$t/explore.rwdir"; print $rw "none\n"; close $rw;

$Explore::Builtins::ROOT = '/';
Explore::Builtins::clear_prefixes();
Explore::Builtins::add_prefix('>site>explore_dir', $t);

my $reg = MBasic::Registry->new;
Explore::Builtins::register_all($reg);
$reg->register('set_acl', sub {}); $reg->register('exec_com', sub {});

# Pin the clock to a known WEEKDAY (Wed 2026-09-09, noon local) so the test is
# deterministic: it does not depend on the day it happens to run, and it never
# enters the weekend/holiday "cave closed" path.  (MBasic's Env derives dat$ /
# clk$ from localtime(now).)
my $when = timelocal(0, 0, 12, 9, 8, 2026);   # sec,min,hour, mday, mon(0=Jan), year
my $interp = MBasic::Interp->new(registry => $reg, search_path => [ $t ], now => $when);
$interp->load_main("$t/explore.basic");

# Eagerly load/link/index every shipped helper up front, exactly as the runner
# does.  This both matches the runner's behavior and permanently verifies that
# all of the game's helper .basic files parse, link, and index cleanly (a
# regression guard for future edits to a helper).  The 10 exp_*.basic helpers
# load; explore.basic (the main) is already loaded and is skipped.
my $nhelpers = eval { $interp->load_all_helpers($t) };
is($nhelpers, 10, 'all 10 helper .basic files load, link, and index cleanly');

# feed a short command sequence; capture output.  The input callback DIES when
# the script is exhausted, so any desync (the game asking for more than we
# scripted) fails the test in seconds instead of hanging the build forever.
my @input = ("in", "look", "quit", "yes");
my $out = '';
$interp->run(
    pathxlate => \&Explore::Builtins::mult_path,
    out   => sub { $out .= $_[0] },
    input => sub {
        die "game requested more input than the test script provides\n" unless @input;
        return shift @input;
    },
);

like($out, qr/Version 5\.3/,        'game prints its version banner');
like($out, qr/small wooden/i,       'room description appears (the shack)');
like($out, qr/You scored/,          'quit reaches the scoring sequence');

done_testing;
