use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib"; use lib ($ENV{MBASIC_LIB} // ());
use Test::More;
use File::Temp qw(tempdir);
use File::Copy;
use Time::Local qw(timelocal);

# Regression test for the abbreviation feature end to end.  This exercises the
# code path (explore.basic ~5280-5480) that jumps out of an inner FOR loop to
# expand a matched abbreviation -- which used to abort with "For-next mismatch"
# until (a) exp_home_ returned a real, writable home so the feature actually
# ran, and (b) MBasic's `next` learned to close abandoned inner loops.

BEGIN {
    eval { require MBasic::Interp; require MBasic::Registry; 1 }
      or plan skip_all => 'MBasic interpreter not available';
}
require Explore::Builtins;
no warnings 'once';

my $share = "$FindBin::Bin/../share";
plan skip_all => "game data (share/) not found" unless -f "$share/explore.basic";

my $t = tempdir(CLEANUP => 1);
for my $f (glob "$share/*") { my ($b) = $f =~ m{([^/]+)$}; copy($f, "$t/$b"); }
open my $rw, '>', "$t/explore.rwdir"; print $rw "none\n"; close $rw;

# a private, writable HOME so the abbrev file can actually be created
my $home = tempdir(CLEANUP => 1);
local $ENV{HOME} = $home;

$Explore::Builtins::ROOT = '/';
Explore::Builtins::clear_prefixes();
Explore::Builtins::add_prefix('>site>explore_dir', $t);

my $reg = MBasic::Registry->new;
Explore::Builtins::register_all($reg);
$reg->register('set_acl', sub {}); $reg->register('exec_com', sub {});

my $when = timelocal(0, 0, 12, 9, 8, 2026);   # a Wednesday: cave open
my $interp = MBasic::Interp->new(registry => $reg, search_path => [ $t ], now => $when);
$interp->load_main("$t/explore.basic");
$interp->load_all_helpers($t);

# enter abbrev mode, define an abbrev, then invoke it: "i" -> "what" (inventory)
my @input = ("ab", ".ab i what", "i", "quit", "yes");
my $out = '';
my $ok = eval {
    $interp->run(
        pathxlate => \&Explore::Builtins::mult_path,
        out   => sub { $out .= $_[0] },
        input => sub { die "out of input\n" unless @input; shift @input },
    );
    1;
};

ok($ok, 'abbrev session runs to completion without dying')
    or diag("died: $@\noutput so far:\n$out");
unlike($out, qr/For-next mismatch/, 'no "For-next mismatch" from the abbrev expander');
like($out, qr/carrying/i, 'the "i" abbrev expanded to "what" (inventory shown)');

done_testing;
