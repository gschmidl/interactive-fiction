use strict;
use warnings;

use 5.008;

use ExtUtils::MakeMaker;
WriteMakefile
    (
     NAME          => 'MBasic',
     DISTNAME      => 'MBasic',
     VERSION_FROM  => 'lib/MBasic.pm',
     ABSTRACT      => 'Interpreter for the subset of Multics BASIC used by Explore',
     AUTHOR        => 'Jim Lippard <lippard@discord.org>',
     LICENSE       => 'bsd',
     MIN_PERL_VERSION => '5.008',
     PREREQ_PM     => {
         'strict'     => 0,
         'warnings'   => 0,
         'Fcntl'      => 0,
     },
     TEST_REQUIRES => {
         'Test::More' => 0,
         'File::Temp' => 0,
     },
     # modules under lib/ are installed automatically to the system perl
     # location (INST_LIB); no explicit PM mapping is needed.
    );
