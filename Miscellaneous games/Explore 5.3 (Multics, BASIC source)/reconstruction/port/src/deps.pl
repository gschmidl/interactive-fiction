# deps.pl LIST SCRIPT [ARGS...]
#
# Run SCRIPT (explore.pl) with ARGS on this Perl and write to LIST every file
# it loaded (%INC) and every XS DLL (DynaLoader), one per line.  mkruntime.pl
# builds the port's own Perl from these lists.
use strict;
use warnings;

my $list   = shift or die "usage: perl deps.pl LIST SCRIPT [ARGS...]\n";
my $script = shift or die "usage: perl deps.pl LIST SCRIPT [ARGS...]\n";

# Loaded by the game only on some paths: Term::ReadKey hides the sorcerer's
# magic word on a console (not on a pipe, as here), File::Temp makes the
# read-only fallback's rwdir.
require Term::ReadKey;
require File::Temp;
{
    my ($fh, $name) = File::Temp::tempfile(UNLINK => 1);
    close $fh;
}

END {
    no warnings 'once';
    my $status = $?;
    open my $fh, '>', $list or die "deps.pl: cannot write $list: $!\n";
    print $fh "inc\t$_\t$INC{$_}\n" for grep { defined $INC{$_} } sort keys %INC;
    print $fh "dll\t$_\n" for @DynaLoader::dl_shared_objects;
    close $fh;
    $? = $status;
}

$0 = $script;
do $script;
die $@ if $@;
