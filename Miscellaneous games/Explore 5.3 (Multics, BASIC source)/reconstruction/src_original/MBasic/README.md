MBasic - a Multics BASIC interpreter in Perl
============================================

MBasic is a small, dependency-free interpreter for the subset of Multics
BASIC used by the 1980 Multics adventure game "Explore" and its helper
subroutines.  It runs the authentic BASIC source unmodified.

The design goal is fidelity: the interpreter reproduces what Multics BASIC
actually does (verified against the AM82-01 manual, and against a running
Multics system where the manual is silent or ambiguous).  It fails loudly on
any statement or construct outside the implemented subset rather than
silently mis-running it.

This distribution provides the interpreter library only.  The game itself is
distributed separately (the "Explore" distribution), which registers its own
native helpers and supplies the game data.

INSTALLATION FROM OPENBSD PACKAGE

The p5-MBasic-1.1.tgz file is a Legion of Dynamic Discord-signed
OpenBSD package which can be installed on OpenBSD with pkg_add and can
also be installed on macOS or Linux using my install.pl script which
can be found in this repo:

https://github.com/lippard661/distribute

Ihe public key for the signing key is:

https://www.discord.org/lippard/software/discord.org-2026-pkg.pub

INSTALLATION

    perl Makefile.PL
    make
    make test
    make install

The modules install to your system's Perl library location.  After
installation:

    perldoc MBasic              # overview and architecture
    perldoc MBasic::Interp      # the orchestrator (start here to embed it)
    perldoc MBasic::Registry    # registering native builtins
    perldoc MBasic::<Component> # any individual component

USING IT

    use MBasic::Interp;
    use MBasic::Registry;

    my $registry = MBasic::Registry->new;
    $registry->register('my_op', sub {
        my ($ctx, $args) = @_;          # $args are MBasic::Arg adapters
        $args->[1]->set( compute($args->[0]->get) );   # write an out-param
    });

    my $interp = MBasic::Interp->new(
        registry    => $registry,
        search_path => [ '/path/to/basic/subs' ],
    );
    $interp->load_main('program.basic');
    $interp->run;

SUBSET SCOPE

Statements: call data dim end file for gosub goto if input let linput next on
print randomize read rem reset return scratch stop sub subend.

Functions: sst$ seg$ left$ right$ mid$ len val str$ pos int abs sgn sqr chr$
asc tst rnd cnt arg$.  Specials: usr$ dat$ clk$.

Anything else is rejected with a message naming the line.

OMITTED

* def/fnend: user-defined functions (def fnx(a,b)..)
* change: convert between string and numeric character array
* change bit / bit operations
* margin/margin-file: set output line width
* print using/print-file-using/image: formatted output with format strings
* mat read/input/print matrix operations
* random access/keyed file operations
* trig functions sin, cos, tan, atn, exp, log
* matrix functions: det, inv, trn, con, zer
* num - numeric input count used with mat input
* user functions (fna-fnz); requires def
* extended/double precision
* full compiler diagnostics (e.g., doesn't enforce "end must be
last statement")

REQUIREMENTS

Perl 5.8+; core modules only (Fcntl and Errno for the atomic, symlink-safe
file writes, and the file locking used by builtins; Test::More and File::Temp
for the test suite).

LICENSE

BSD 3-Clause; see the LICENSE file.

AUTHOR

Jim Lippard directing Claude Opus 4.8
