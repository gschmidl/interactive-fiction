package MBasic;
use strict;
use warnings;

our $VERSION = '1.2';

1;

__END__

=head1 NAME

MBasic - an interpreter for the subset of Multics BASIC used by the Explore game

=head1 SYNOPSIS

    use MBasic::Interp;
    use MBasic::Registry;

    my $registry = MBasic::Registry->new;
    # register any native (Perl-implemented) subroutines the program calls:
    $registry->register('my_helper', sub { my ($ctx, $args) = @_; ... });

    my $interp = MBasic::Interp->new(
        registry    => $registry,
        search_path => [ '/path/to/basic/helpers' ],
        argv        => [ @program_arguments ],
    );
    $interp->load_main('program.basic');
    $interp->run;

=head1 DESCRIPTION

C<MBasic> is a small, dependency-free interpreter for Multics BASIC, written in
Perl.  It implements the subset of the language exercised by the 1980 Multics
adventure game I<Explore> and its helper subroutines, and it runs the authentic
BASIC source B<unmodified>.

The design goal is fidelity: the interpreter reproduces what Multics BASIC
actually does (verified against the AM82-01 manual and, where the manual is
silent or ambiguous, against a running Multics system), and it B<fails loudly>
on any statement or construct outside the implemented subset rather than
silently mis-running it.

=head2 What "subset" means

The following statements are implemented (the complete set used by Explore and
its helpers):

    call  data  dim   end   file  for   gosub goto  if    input let   linput
    next  on    print randomize   read  rem   reset return scratch stop  sub   subend

Expressions support C<+ - * / ^> and string concatenation C<&>, the relational
operators, and these built-in functions:

    sst$ seg$ left$ right$ mid$ len val str$ pos int abs sgn sqr chr$ asc tst
    rnd cnt arg$

The special read-only variables C<usr$>, C<dat$>, and C<clk$> are provided.

Any statement or function outside this set produces a load-time or run-time
error naming the offending line, so an unsupported program is rejected rather
than mishandled.

=head1 ARCHITECTURE

The interpreter is a classic parse-to-intermediate-representation-then-execute
design.  Each component is a separate module:

=over 4

=item L<MBasic::Lexer>

Tokenizes one source line into a line number and a list of tokens.

=item L<MBasic::Parser>

Turns a token stream into an IR statement record (one plain-data hashref per
statement).  Embedded expressions are parsed by L<MBasic::Expr>.

=item L<MBasic::Expr>

Parses and evaluates expressions; implements the built-in functions.

=item L<MBasic::Env>

The per-program-unit variable environment: numeric and string scalars and
arrays (separate namespaces), and the C<usr$>/C<dat$>/C<clk$> specials.

=item L<MBasic::Program>

One loaded BASIC "program unit" (the main program, or a helper file that
defines one or more C<sub>s): its IR, its line-number map, its C<sub> entry
points, and its C<data> pool.

=item L<MBasic::Linker>

Resolves every jump target (from C<goto>/C<if>/C<on>/C<gosub>) to an IR index
and validates that no reference dangles.

=item L<MBasic::Executor>

The program-counter run loop.  Executes statements, manages the C<for>/C<gosub>
stacks, dispatches C<call>, and performs terminal and file I/O.

=item L<MBasic::File>

One terminal-format file channel (C<#1>..C<#4>): C<file>/C<input>/C<linput>/
C<print #n>/C<scratch>/C<reset>/C<if end>/C<if more>.

=item L<MBasic::Registry>

The native-builtin table: maps a C<call> name to a Perl coderef.  This is how a
program calls operations that are not written in BASIC.

=item L<MBasic::Arg>

An argument adapter passed to native builtins and used for BASIC C<sub>
parameter binding, honoring the Multics BASIC pass-by-reference /
copy-string-in-and-out convention (AM82-01 Appendix B).

=item L<MBasic::Interp>

The orchestrator: holds the registry and the loaded program units, resolves
C<call> names (a BASIC C<sub> takes precedence over a same-named builtin), and
runs the main program.

=back

=head2 The C<call> mechanism

C<call "name": args> resolves C<name> to one of two things, mirroring Multics
dynamic linking over the search rules:

=over 4

=item *

a B<native builtin> (a Perl coderef in the registry), invoked with argument
adapters that write back to the caller's variables for out-parameters; or

=item *

a B<BASIC subroutine> (a C<sub "name"> in a loaded C<.basic> file), which is
run as its own program unit in a B<fresh environment> -- its own variables,
file channels and C<data> pointer, all reset on entry -- with parameters bound
by reference.  Only the parameters connect a subroutine to its caller.  The
pseudo-random generator is the one exception to the fresh environment: it is a
single stream shared across the whole program (main and all its subroutines),
so the random sequence is continuous and repeatable regardless of where C<rnd>
is called.  It is self-contained (it does not use Perl's global C<rand>/C<srand>,
so an embedding process's own random stream is left undisturbed), starts from a
fixed seed for a repeatable sequence across runs, and is reseeded from an
entropy source only by the C<randomize> statement.

=back

Helper C<.basic> files are located over a configurable search path (the Unix
analog of the Multics search rules) and indexed by the C<sub> names they
declare.

=head1 EXTENDING

To run a different Multics-BASIC program that calls native operations, register
your own builtins into a L<MBasic::Registry> before running:

    $registry->register('read_sensor', sub {
        my ($ctx, $args) = @_;         # $args are MBasic::Arg adapters
        my $id  = $args->[0]->get;     # read an incoming argument
        $args->[1]->set( sensor_value($id) );   # write an out-parameter back
    });

The interpreter core knows nothing program-specific; everything particular to a
given program lives in the builtins you register (and in the BASIC helper files
you supply on the search path).  See L<Explore::Builtins> (in the companion
Explore distribution) for a complete worked example.

=head1 ERROR MESSAGES

Where the interpreter detects a run-time error condition that Multics BASIC
also reports, it uses the authentic Multics BASIC message text (from the AM82
manual's error-message appendix), so a failure reads the way it would on
Multics -- for example C<Division by zero>, C<Out of data>, C<Subscript out
of bounds>, C<Return before gosub>, C<Next without for>, C<End-of-file>,
C<Not enough input, add more>, and C<Wrong number of arguments for "zzz">.
Conditions with no Multics analogue (internal consistency checks, or
constructs outside the implemented subset) use descriptive messages and,
for anything unimplemented, report it plainly rather than mis-running it.

=head1 DIAGNOSTICS

Run-time errors are reported using the message text from the Multics BASIC
manual's error list (AM82 Appendix E, "Error Messages", added by the MR 12.2
errata) -- for example "Subscript out of bounds", "Return before gosub",
"Out of data", "Division by zero", "End-of-file", and "On evaluated out of
range" -- so that a failure reads as it would on Multics.  Constructs outside
the implemented subset produce "Unimplemented run-time operator" (or a parse-
time rejection), matching the manual's treatment of unimplemented operations.

=head1 SECURITY

MBasic runs an interpreted language, and a program can open files, read C<data>,
and (via C<file>/C<print #n>) write files.  When the BASIC program and its
helper files are trusted (as with the Explore game), this is unremarkable.  When
a program or its input might be B<untrusted>, note the following boundaries:

=over 4

=item *

B<File pathnames are the containment boundary.>  A C<file #n: path> statement
opens whatever path the program computes.  The interpreter does not itself
sandbox pathnames; an embedder that needs containment supplies a C<pathxlate>
hook (passed to C<run>) that maps every BASIC pathname into an allowed subtree
before it is opened.  The companion L<Explore::Builtins> C<mult_path> is a
worked example (it maps Multics C<< >a>b >> names under a fixed root).  Without
such a hook, a program can name any path the host process can access; install a
C<pathxlate> that rejects absolute paths and C<..> when running untrusted input.

=item *

B<Writes are crash-safe and resist symlink attacks, but are not locked.>  By
default each write is committed via a temp file (created
C<O_CREAT|O_EXCL|O_NOFOLLOW>, so a pre-planted temp name cannot redirect it)
plus C<rename>, so a crash or full disk never leaves a truncated file, a planted
symlink at the target is replaced rather than followed, and the target's
permission bits (and, where the process may, its owner/group) are preserved
across the replace.  When the containing directory is not writable but the
target file is -- the shared-game layout Explore's installer sets up, the Unix
analog of Multics per-segment ACLs -- MBasic falls back to rewriting the file
in place (also C<O_NOFOLLOW>); this one path is not atomic.  There is no
multi-writer locking in either mode: the whole-file-rewrite model means
concurrent writers can still lose updates, and the atomic mode replaces the
inode so a reader holding the file open must re-open by path to see changes.
Coordinating concurrent writers is the embedder's responsibility (Explore uses
advisory locking around its shared files).

=item *

B<Resource use is bounded, not eliminated.>  Array size, C<print tab()> width,
and C<call> recursion depth are capped so a program raises a loud BASIC error
rather than aborting the host with an out-of-memory or stack fault; but a
program can still consume CPU and memory up to those caps.

=item *

B<call names are validated.>  A C<call> target must be an identifier (optionally
with a Multics C<segment$entrypoint> suffix), so a call name cannot be used to
traverse the filesystem or load an arbitrary C<.basic>-suffixed file from the
search path.

=back

=head1 VERSION

Version 1.2.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

The interpreter was developed in 2026 to run the reconstructed 1980 Multics
game I<Explore> portably; see the companion Explore distribution.

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  This is free software, released under the
BSD 3-Clause License; see the F<LICENSE> file in the distribution.

=cut
