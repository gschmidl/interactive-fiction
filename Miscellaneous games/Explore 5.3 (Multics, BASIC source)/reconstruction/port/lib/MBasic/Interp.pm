package MBasic::Interp;
use strict;
use warnings;
our $VERSION = '1.2';
use MBasic::Program;
use MBasic::Linker;
use MBasic::Registry;
use MBasic::Executor;
use MBasic::Env;

# ============================================================================
#  MBasic::Interp -- orchestrator.  Holds the registry (native builtins) and
#  the loaded program units (main + BASIC-sub helpers), resolves `call` names,
#  and runs the main program.
#
#    registry   MBasic::Registry            native builtins
#    programs   { path_or_tag => Program }  loaded units
#    subindex   { subname => Program }      which unit defines each `sub`
#    main       Program                     the main program
#    search     [ dirs ]                    helper .basic search path
# ============================================================================

sub new {
    my ($class, %opt) = @_;
    bless {
        registry => $opt{registry} // MBasic::Registry->new,
        programs => {},
        subindex => {},
        main     => undef,
        search   => $opt{search_path} || [],
        argv     => $opt{argv} || [],
        user     => $opt{user},
        now      => $opt{now},   # deterministic date/time for tests
    }, $class;
}

# ---- loading ----
sub load_main {
    my ($self, $path) = @_;
    my $prog = MBasic::Program->load_file($path);
    MBasic::Linker->link_program($prog);
    $self->{main} = $prog;
    $self->{programs}{$path} = $prog;
    $self->_index_subs($prog);
    return $prog;
}

# load a helper program unit (a .basic file that may define one or more subs)
sub load_helper_file {
    my ($self, $path) = @_;
    return $self->{programs}{$path} if $self->{programs}{$path};
    my $prog = MBasic::Program->load_file($path);
    MBasic::Linker->link_program($prog);
    $self->{programs}{$path} = $prog;
    $self->_index_subs($prog);
    return $prog;
}

# load a helper program unit from in-memory lines (tests)
sub load_helper_lines {
    my ($self, $lines, $tag) = @_;
    my $prog = MBasic::Program->load_lines($lines, $tag // '(helper)');
    MBasic::Linker->link_program($prog);
    $self->{programs}{$tag // "$prog"} = $prog;
    $self->_index_subs($prog);
    return $prog;
}

# eagerly load, link, and index every "*.basic" helper in the given directories
# (or the interpreter's search path when none are given).  This surfaces any
# load-time error -- a parse rejection, a dangling jump, or the same sub defined
# in two units -- at STARTUP, instead of the first time a given helper happens
# to be called (which for an interactive program can be far into a session,
# where a die would lose the user's work).  Files already loaded (by path) are
# skipped, so calling this after load_main() will not reload the main program.
# Returns the number of helper files newly loaded.
sub load_all_helpers {
    my ($self, @dirs) = @_;
    @dirs = @{$self->{search}} unless @dirs;
    my $count = 0;
    for my $dir (@dirs) {
        next unless defined $dir && -d $dir;
        opendir(my $dh, $dir) or next;
        my @files = sort grep { /\.basic\z/ } readdir $dh;
        closedir $dh;
        for my $f (@files) {
            my $path = "$dir/$f";
            next unless -f $path;
            next if $self->{programs}{$path};   # already loaded (e.g. the main)
            $self->load_helper_file($path);
            $count++;
        }
    }
    return $count;
}

sub _index_subs {
    my ($self, $prog) = @_;
    for my $name (keys %{$prog->{subs}}) {
        if (my $other = $self->{subindex}{$name}) {
            next if $other == $prog;    # re-indexing the same unit is harmless
            # the SAME sub name defined in two different units on the search
            # path is an ambiguity that would otherwise resolve silently by
            # load order; reject it loudly (errata 070, across units).
            die "load error: subroutine \"$name\" defined in more than one unit "
              . "($other->{path} and $prog->{path})\n";
        }
        $self->{subindex}{$name} = $prog;
    }
}

# resolve a helper .basic file by sub-name over the search path, load + index.
sub _find_and_load_sub {
    my ($self, $name) = @_;
    return 1 if $self->{subindex}{$name};
    return 0 if $self->{_notfound}{$name};   # negative cache: don't re-stat

    # SECURITY: the name is about to be interpolated into a filesystem path
    # ("$dir/$name.basic").  Accept only an identifier, optionally with a
    # Multics "segment$entrypoint" suffix, so a call name cannot contain '/',
    # '.', '..', '<' or '>' to escape the search directories or open an
    # arbitrary .basic-suffixed file (a path-traversal read / builtin-hijack
    # primitive).  ('$' is a safe filename character on Unix.)
    unless ($name =~ /^[A-Za-z][A-Za-z0-9_]*(?:\$[A-Za-z][A-Za-z0-9_]*)?\z/) {
        $self->{_notfound}{$name} = 1;
        return 0;
    }

    for my $dir (@{$self->{search}}) {
        my $cand = "$dir/$name.basic";
        if (-f $cand) { $self->load_helper_file($cand);
                        return 1 if $self->{subindex}{$name}; }
    }
    $self->{_notfound}{$name} = 1;
    return 0;
}

# ---- call resolution: name -> ('builtin', code) | ('sub', program, entryinfo) ----
sub resolve_call {
    my ($self, $name) = @_;
    # BASIC sub takes precedence over a same-named builtin (run authentic source)
    if (!$self->{subindex}{$name}) { $self->_find_and_load_sub($name); }
    if (my $prog = $self->{subindex}{$name}) {
        return ('sub', $prog, $prog->{subs}{$name});
    }
    if ($self->{registry}->has($name)) {
        return ('builtin', $self->{registry}->lookup($name));
    }
    return ('undef');
}

# ---- run the main program ----
sub run {
    my ($self, %opt) = @_;
    die "no main program loaded\n" unless $self->{main};
    my $env = MBasic::Env->new(argv => $self->{argv}, user => $self->{user},
                               now => $self->{now});
    return MBasic::Executor->run_program(
        $self->{main},
        env    => $env,
        interp => $self,
        out    => $opt{out},
        input  => $opt{input},
        pathxlate => $opt{pathxlate},
    );
}

1;

__END__

=head1 NAME

MBasic::Interp - the MBasic interpreter orchestrator

=head1 DESCRIPTION

Ties the pieces together: holds the native-builtin L<MBasic::Registry> and
the loaded program units, resolves C<call> names (a BASIC C<sub> takes
precedence over a same-named builtin, and helper C<.basic> files are located
over a search path and indexed by the C<sub> names they declare), and runs
the main program.

=head1 METHODS

=head2 new(%opt)

Constructor.  Options: C<registry>, C<search_path> (arrayref of directories to
find helper C<.basic> files), C<argv>, C<user>, C<now>.

=head2 load_main($path)

Load, link, and index the main program.

=head2 load_helper_file($path) / load_helper_lines(\@lines, $tag)

Load an additional program unit (defining C<sub>s) from a file or memory.

=head2 load_all_helpers(@dirs)

Eagerly load, link, and index every C<*.basic> helper found in C<@dirs> (or the
configured search path when called with no arguments), returning the number of
files newly loaded.  Use it after C<load_main> to validate all helpers up front
so a load-time error (a parse rejection, a dangling jump, a sub defined in two
units) is reported at startup rather than the first time that helper is called.
Files already loaded (by path) are skipped, so the main program is not reloaded.

=head2 resolve_call($name)

Resolve a call name to C<('builtin', $code)>, C<('sub', $program, $entry)>, or
C<('undef')>.

=head2 run(%opt)

Run the main program.  Options include C<out>, C<input>, and C<pathxlate>.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
