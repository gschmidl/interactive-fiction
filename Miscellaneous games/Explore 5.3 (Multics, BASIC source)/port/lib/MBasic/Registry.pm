package MBasic::Registry;
use strict;
use warnings;
our $VERSION = '1.2';

# ============================================================================
#  MBasic::Registry -- the native-builtin table (generic mechanism).
#  Maps a call-name -> a Perl coderef with the standard calling convention.
#
#  Calling convention (so builtins are independently implementable):
#     $code->($ctx, \@args)
#  where @args are ARGUMENT ADAPTERS (MBasic::Arg) honoring Appendix B --
#  each supports ->get (read the incoming value) and ->set($v) (write an
#  out-parameter back to the caller's variable).  $ctx is a hashref of
#  interpreter services the builtin may need:
#     { rs => runstate, interp => interp, out => sub, ... }
# ============================================================================

sub new { my ($class) = @_; bless { table => {} }, $class }

sub register {
    my ($self, $name, $code) = @_;
    $self->{table}{$name} = $code;
    return;
}
sub lookup { my ($self, $name) = @_; $self->{table}{$name} }
sub has    { my ($self, $name) = @_; exists $self->{table}{$name} }
sub names  { my ($self) = @_; sort keys %{$self->{table}} }

1;

__END__

=head1 NAME

MBasic::Registry - the native-builtin table for MBasic

=head1 DESCRIPTION

Maps a C<call> name to a Perl coderef (a "native builtin").  This is how a
BASIC program invokes operations not written in BASIC.  A registered coderef
is called as C<< $code->($ctx, \@args) >>, where C<@args> are L<MBasic::Arg>
adapters and C<$ctx> is a hashref of interpreter services (C<rs>, C<interp>,
C<out>).  The interpreter core is program-agnostic; everything specific to a
program lives in the builtins registered here.

=head1 METHODS

=head2 new

Constructor.

=head2 register($name, $code)

Register a native builtin.

=head2 lookup($name) / has($name) / names

Look up a builtin, test for one, or list all registered names.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
