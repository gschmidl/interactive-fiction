package MBasic::Program;
use strict;
use warnings;
our $VERSION = '1.2';
use MBasic::Parser;

# ============================================================================
#  MBasic::Program -- one loaded BASIC program unit (main OR a helper file).
#  Per the manual each unit has its own line-number space and is entered with
#  a fresh Environment.
#    ir        [ IR-record, ... ]   statements in source order
#    linemap   { line_no => ir_index }
#    subs      { subname => { entry => idx, params => [...] } }
#    data      [ {num=>..}|{str=>..}, ... ]  this unit's DATA pool, in order
#    path      source path (for messages)
#  Validation at load: line numbers present, strictly ascending, unique.
# ============================================================================

sub new {
    my ($class, %f) = @_;
    bless {
        ir      => [],
        linemap => {},
        subs    => {},
        data    => [],
        path    => $f{path} // '(memory)',
    }, $class;
}

# load_file($path) / load_lines(\@lines, $path) -> MBasic::Program
sub load_file {
    my ($class, $path) = @_;
    open my $fh, '<', $path or die "cannot open $path: $!\n";
    local $/ = "\n";   # never inherit a caller's alternate record separator
    my @lines = <$fh>; close $fh;
    return $class->load_lines(\@lines, $path);
}

sub load_lines {
    my ($class, $lines, $path) = @_;
    my $self = $class->new(path => $path);
    my $prev = -1;
    for my $line (@$lines) {
        my $raw = $line;    # copy: never chomp the caller's array in place
        chomp $raw;
        next if $raw =~ /^\s*$/;
        my ($lineno, $rec) = MBasic::Parser->parse_line($raw);
        die "load error ($path): line $lineno not greater than previous $prev "
          . "(lines must be strictly ascending, unique)\n"
            if $lineno <= $prev;
        $prev = $lineno;
        my $idx = scalar @{$self->{ir}};
        push @{$self->{ir}}, $rec;
        $self->{linemap}{$lineno} = $idx;
        # index sub entry points (a duplicate definition is an error, errata
        # 070, rather than silently letting the last one win)
        if ($rec->{op} eq 'sub') {
            die "load error ($path): Subroutine \"$rec->{name}\" defined more "
              . "than once (line $lineno)\n"
                if exists $self->{subs}{ $rec->{name} };
            $self->{subs}{ $rec->{name} } = { entry => $idx, params => $rec->{params} };
        }
        # collect DATA values into this unit's pool, in source order
        if ($rec->{op} eq 'data') {
            push @{$self->{data}}, @{ $rec->{values} };
        }
    }
    return $self;
}

sub index_of_line {
    my ($self, $lineno) = @_;
    return $self->{linemap}{$lineno};   # undef if none
}

1;

__END__

=head1 NAME

MBasic::Program - one loaded Multics BASIC program unit

=head1 DESCRIPTION

Represents one loaded program unit -- the main program, or a helper file
that defines one or more C<sub>s.  Holds the IR (an arrayref of statement
records), the line-number-to-index map, the C<sub> entry-point index, and
the unit's C<data> pool.  Validates at load that line numbers are present,
unique, and strictly ascending.  Per the manual each unit has its own
line-number space and is entered with a fresh environment.

=head1 METHODS

=head2 load_file($path) / load_lines(\@lines, $path)

Class methods.  Parse a file (or an arrayref of source lines) into a Program.

=head2 index_of_line($lineno)

Return the IR index for a source line number, or undef.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
