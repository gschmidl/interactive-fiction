use strict;
use warnings;

use 5.008;

use ExtUtils::MakeMaker;
WriteMakefile
    (
     NAME          => 'Explore',
     DISTNAME      => 'Explore',
     VERSION_FROM  => 'lib/Explore/Builtins.pm',
     ABSTRACT      => 'The reconstructed 1980 Multics game "Explore", run via MBasic',
     AUTHOR        => 'Jim Lippard <lippard@discord.org>',
     LICENSE       => 'bsd',
     MIN_PERL_VERSION => '5.008',
     PREREQ_PM     => {
         'MBasic'      => '1.0',   # the interpreter (separate distribution)
         'Fcntl'       => 0,
         'File::Copy'  => 0,
         'File::Path'  => 0,
     },
     TEST_REQUIRES => {
         'Test::More'  => 0,
         'File::Temp'  => 0,
     },
     EXE_FILES     => [ 'explore' ],
     # The Perl module (lib/Explore/Builtins.pm) and the "explore" program are
     # installed to the standard system locations.  The game DATA files (in
     # share/) and the historical ARTIFACTS are NOT installed by this
     # Makefile.PL: on a packaged install the OpenBSD port's PLIST places them
     # (typically under /usr/local/share/explore); for a manual install from
     # the tarball, copy share/* to your share directory (see the README).
    );
