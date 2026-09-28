package MBasic::Linker;
use strict;
use warnings;
our $VERSION = '1.2';

# ============================================================================
#  MBasic::Linker -- resolve line-number references to IR indices and validate.
#  Fail-fast: a program with a dangling jump does not run.
#
#  link_program($program):
#    * every goto/gosub/if/ifend/ifmore target and every on-goto/gosub target
#      is a line number; verify it exists and record its IR index in a parallel
#      'tidx' field (target index) so the executor jumps in O(1).
#    * for/next pairing is handled at run time via the for-stack (not linked
#      here) since Explore's loops are well-formed; a future check could pair
#      them.
#  Cross-unit `call` name resolution is done by the interpreter (needs the
#  builtin registry + other loaded units), not here.
# ============================================================================

sub link_program {
    my ($class, $prog) = @_;
    my $ir = $prog->{ir};
    for my $i (0 .. $#$ir) {
        my $r = $ir->[$i];
        my $op = $r->{op};
        if ($op eq 'goto' || $op eq 'gosub' || $op eq 'if'
            || $op eq 'ifend' || $op eq 'ifmore') {
            $r->{tidx} = _resolve($prog, $r->{target}, $r->{line});
        }
        elsif ($op eq 'on') {
            $r->{tidx} = [ map { _resolve($prog, $_, $r->{line}) } @{$r->{targets}} ];
        }
    }
    return $prog;
}

sub _resolve {
    my ($prog, $lineno, $atline) = @_;
    my $idx = $prog->index_of_line($lineno);
    die "link error ($prog->{path}): line $atline references line $lineno "
      . "which does not exist\n"
        unless defined $idx;
    return $idx;
}

1;

__END__

=head1 NAME

MBasic::Linker - resolve and validate jump targets

=head1 DESCRIPTION

Resolves every jump target (from C<goto>, C<gosub>, C<if>, C<if end>,
C<if more>, and C<on ... goto/gosub>) from a source line number to a
concrete IR index, storing it alongside the statement for O(1) dispatch.
Fails loudly if any target references a line that does not exist, so a
program with a dangling jump never runs.

=head1 METHODS

=head2 link_program($program)

Class method.  Resolve and validate all jump targets in a L<MBasic::Program>,
in place.  Dies naming the referencing line if a target is missing.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
