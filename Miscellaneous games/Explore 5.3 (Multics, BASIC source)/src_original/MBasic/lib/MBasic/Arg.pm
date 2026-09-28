package MBasic::Arg;
use strict;
use warnings;
our $VERSION = '1.2';

# ============================================================================
#  MBasic::Arg -- an argument adapter passed to a native builtin (and used for
#  BASIC-sub parameter binding).  Honors Appendix B:
#    * The current value is read via ->get.
#    * ->set($v) writes back to the CALLER's variable (an lvalue) so an
#      out-parameter (esp. a pre-sized string like h9$) is updated at return.
#    * A non-lvalue argument (a literal or a computed expression) is read-only;
#      ->set on it is a silent no-op (Appendix B: value expressions are passed
#      by value; only variables receive write-back).
#
#  Constructed by the executor from a call-argument expr-node + the caller Env:
#    * if the node is a bare scalar var or array element -> a writable adapter
#      bound to that lvalue in the caller env.
#    * otherwise -> a read-only adapter holding the evaluated value.
# ============================================================================

sub new_readonly {
    my ($class, $value) = @_;
    bless { value => $value, writable => 0 }, $class;
}

sub new_lvalue {
    my ($class, %o) = @_;
    # %o: env, name, subs (arrayref of already-evaluated subscripts) or undef
    bless {
        env      => $o{env},
        name     => $o{name},
        subs     => $o{subs},        # undef => scalar; arrayref => array elem
        writable => 1,
    }, $class;
}

sub get {
    my ($self) = @_;
    return $self->{value} if !$self->{writable};
    if (defined $self->{subs}) {
        return $self->{env}->get_array($self->{name}, $self->{subs});
    }
    return $self->{env}->get_scalar($self->{name});
}

sub set {
    my ($self, $v) = @_;
    return unless $self->{writable};   # value args: write-back is a no-op
    if (defined $self->{subs}) {
        $self->{env}->set_array($self->{name}, $self->{subs}, $v);
    } else {
        $self->{env}->set_scalar($self->{name}, $v);
    }
    return;
}

sub is_writable { $_[0]->{writable} }

1;

__END__

=head1 NAME

MBasic::Arg - an argument adapter honoring BASIC pass-by-reference

=head1 DESCRIPTION

Wraps a C<call> argument so that a native builtin (or a BASIC subroutine's
parameter binding) can both read the incoming value and write an
out-parameter back to the caller's variable, per the Multics BASIC
convention (AM82-01 Appendix B): variables are passed by reference and
strings are copied in at entry and written back at exit; value expressions
are read-only, and writing back to one is a silent no-op.

=head1 METHODS

=head2 new_lvalue(%opt) / new_readonly($value)

Construct a writable adapter bound to a caller variable (C<env>, C<name>, and
optional C<subs> for an array element), or a read-only adapter holding a value.

=head2 get / set($v) / is_writable

Read the current value; write it back (no-op if read-only); test writability.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
