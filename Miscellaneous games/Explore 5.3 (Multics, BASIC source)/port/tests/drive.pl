# drive.pl LIBS SHARE WORK SEED < commands > transcript
#
# Play Explore with a fixed random seed and a fixed clock and user (dat$ =
# 09/09/26, a Wednesday, and clk$ = 12:00:00, as the author's t/09_game.t has
# it; usr$ = Explorer) and print the transcript.  LIBS are the folders of
# MBasic and Explore::Builtins, separated as in PERL5LIB (";" on Windows, ":"
# elsewhere), SHARE the game, and WORK is an empty scratch folder: the game's
# writable directory, its home and its working directory.
#
# check.py runs it twice on each input: with this port's lib on the port's
# own Perl, and with the author's unchanged lib (src_original) on Linux Perl
# in WSL.  The two transcripts must be the same.
use strict;
use warnings;
no warnings 'once';
use Config;

my ($lib, $share, $work, $seed) = @ARGV;
die "usage: perl drive.pl LIBS SHARE WORK SEED\n" unless defined $seed;
unshift @INC, split /\Q$Config{path_sep}\E/, $lib;
require MBasic::Interp;
require MBasic::Registry;
require Explore::Builtins;
require File::Copy;

$| = 1;

# the writable directory: the masters, and an rwdir naming it
for my $f (qw(hours.data winners.data)) {
    File::Copy::copy("$share/$f", "$work/$f") or die "drive.pl: $f: $!\n";
}
open my $rw, '>', "$work/explore.rwdir" or die;
print $rw ">site>explore_dir>private\n^multip\n";
close $rw;
$Explore::Builtins::ROOT = $work;
Explore::Builtins::clear_prefixes();
Explore::Builtins::add_prefix('>site>explore_dir', $share);
Explore::Builtins::add_prefix('>site>explore_dir>explore.rwdir', "$work/explore.rwdir");
Explore::Builtins::add_prefix('>site>explore_dir>private', $work);
# exp_home_ of both versions reads HOME; a Multics pathname keeps the
# transcripts free of the two computers' paths (".p" and "Creating" show it)
$ENV{HOME} = '>udd>Explore>Player';
Explore::Builtins::add_prefix('>udd>Explore>Player', $work);
chdir $work or die;

{
    no warnings 'redefine';
    # RANDOMIZE (line 100) seeds from the clock; here it seeds from SEED
    *MBasic::Env::randomize_seed = sub {
        my ($self) = @_;
        $self->{rng}{seed} = MBasic::Env::_fresh_rng($seed)->{seed};
    };
    # the same day, time and user on both computers, whatever their clocks
    # and time zones say
    my %fixed = ('usr$' => 'Explorer', 'dat$' => '09/09/26', 'clk$' => '12:00:00');
    *MBasic::Env::_special = sub { my ($self, $name) = @_; $fixed{$name} };
}

my $reg = MBasic::Registry->new;
Explore::Builtins::register_all($reg);
$reg->register('set_acl', sub {});
$reg->register('exec_com', sub {});
# no shell commands, messages or editors in a comparison
$reg->register($_, sub {}) for qw(do send_message ted);
my $interp = MBasic::Interp->new(registry => $reg, search_path => [ $share ]);
$interp->load_main("$share/explore.basic");
$interp->load_all_helpers($share);
$interp->run(
    pathxlate => \&Explore::Builtins::mult_path,
    input     => sub {
        my $l = <STDIN>;
        unless (defined $l) { print "\n[end of input]\n"; exit 0; }
        $l =~ s/\r?\n\z//;
        print "$l\n";           # the command, so the transcript reads as played
        return $l;
    },
);
print "[game ended]\n";
