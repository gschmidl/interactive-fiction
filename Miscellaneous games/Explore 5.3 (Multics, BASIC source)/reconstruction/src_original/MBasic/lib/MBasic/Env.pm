package MBasic::Env;
use strict;
use warnings;
our $VERSION = '1.2';

# ============================================================================
#  MBasic::Env -- a program unit's runtime variable environment.
#
#  Per the manual, each program unit / sub call has its OWN environment,
#  reset on entry.  An Env holds this unit's variables (and, later, its file
#  channels / DATA pointer / RNG -- added when the executor needs them).
#
#  Variable model:
#    * numeric and string SCALARS, separate namespaces:  a  vs  a$
#    * numeric and string ARRAYS (1-D and 2-D):  a(i), b(i,j), c$(i)
#      (an array and a scalar of the same name may coexist)
#    * special read-only vars (intercepted on read): usr$, dat$, clk$
#      and the niladic builtins cnt / arg$(n) are provided by the run context
#      (command-line args), passed in at construction.
#
#  Naming: keys are the identifier lexemes as written ('a', 'a$', 'x1', 'p$').
#  We keep numeric and string in separate hashes so 'a' and 'a$' never collide.
#
#  Numbers are Perl doubles.  Uninitialized numeric scalar reads as 0;
#  uninitialized string scalar reads as "" (BASIC default-initialization).
# ============================================================================

# Cap on the total number of cells a single array may hold.  A BASIC program
# could otherwise `dim a(100000000)` and drive the host process into an
# uncatchable Perl "Out of memory!" abort; a loud BASIC-level error is both
# safer for an embedding process and more faithful (Multics would fault).
our $MAX_ARRAY_CELLS = 5_000_000;

# Aggregate cap across ALL arrays in one environment, so many separately-legal
# dims cannot together exhaust host memory.
our $MAX_TOTAL_CELLS = 20_000_000;

# Default RNG seed for a fresh program (repeatable across runs until RANDOMIZE).
# A mid-range value, not 1: Park-Miller from a tiny seed yields a tiny opening
# draw (16807/(2^31-1) ~= 7.8e-6), which would make the first random event of
# every un-RANDOMIZEd run always take the lowest outcome.  We also discard the
# first few draws when seeding (see _fresh_rng) so the opening value is well
# mixed regardless of seed.
our $RNG_DEFAULT_SEED = 471_634_512;   # warms to a mid-range first draw (~0.50)
our $RNG_WARMUP       = 12;   # opening draws to discard when a stream is seeded

sub new {
    my ($class, %opt) = @_;
    my $self = bless {
        nscalar => {},     # name  => number
        sscalar => {},     # name$ => string
        narray  => {},     # name  => { dims=>[...], data=>[...] }
        sarray  => {},     # name$ => { dims=>[...], data=>[...] }
        # run context (supplied by the interpreter for specials):
        argv    => $opt{argv} || [],   # command-line args -> arg$()/cnt
        user    => $opt{user},         # usr$   (defaults computed lazily)
        # date/time are computed live on read (dat$/clk$) unless overridden
        _now    => $opt{now},          # optional fixed epoch for deterministic tests
        # pseudo-random generator state.  Shared (by reference) with the
        # program's subroutine environments so the whole program draws from one
        # repeatable stream; see MBasic::Executor.  A fresh program starts from
        # a fixed (warmed) seed -- repeatable across runs -- unless RANDOMIZE
        # reseeds it.
        rng     => $opt{rng} || _fresh_rng($RNG_DEFAULT_SEED),
    }, $class;
    return $self;
}

# ---- pseudo-random generator ----
# Park-Miller "minimal standard" generator (multiplier 16807, modulus
# 2^31 - 1) implemented with Schrage's method so that NO intermediate product
# exceeds 2^31 - 1.  This keeps the arithmetic exact even on a Perl built with
# 32-bit integers (a plain `seed * 16807` would overflow to a double and lose
# the low bits that matter), so the "same sequence every run" guarantee holds
# on every platform.  It is self-contained -- it never calls Perl's global
# rand()/srand(), so an embedding process's own random stream is untouched.
use constant {
    _RNG_A => 16807,
    _RNG_M => 2147483647,   # 2^31 - 1 (prime)
    _RNG_Q => 127773,       # M div A
    _RNG_R => 2836,         # M mod A
};

# advance the shared seed one step and return it (an integer in 1 .. M-1).
sub _rng_step {
    my ($r) = @_;
    my $seed = $r->{seed};
    my $hi = int($seed / _RNG_Q);
    my $lo = $seed % _RNG_Q;
    my $s  = _RNG_A * $lo - _RNG_R * $hi;   # |s| < M  (Schrage's guarantee)
    $s += _RNG_M if $s <= 0;
    return $r->{seed} = $s;
}

# build a fresh RNG state from a seed: normalize into 1 .. M-1 and discard the
# opening draws so the first user-visible value is well mixed even for a small
# seed (avoids Park-Miller's degenerate tiny first output).
sub _fresh_rng {
    my ($seed) = @_;
    $seed = int($seed) % (_RNG_M - 1);
    $seed += 1 if $seed < 1;                 # -> 1 .. M-1, never 0
    my $rng = { seed => $seed };
    _rng_step($rng) for 1 .. $RNG_WARMUP;
    return $rng;
}

# rnd -> a double in [0,1)
sub rnd {
    my ($self) = @_;
    return _rng_step($self->{rng}) / _RNG_M;
}

# `randomize`: reseed from a non-deterministic source so the sequence differs
# per run (the whole point of the statement), WITHOUT calling Perl's global
# srand()/rand() (which would disturb an embedding process's stream).  Every
# term is kept under 2^31 and combined with xor, so this is 32-bit safe too.
sub randomize_seed {
    my ($self) = @_;
    my ($s, $us) = (time, 0);
    if (eval { require Time::HiRes; 1 }) { ($s, $us) = Time::HiRes::gettimeofday(); }
    my $mix = ($s & 0x7fffffff) ^ ($us & 0x7fffffff)
            ^ (($$ & 0x7fff) << 8) ^ ($self->{rng}{seed} & 0x7fffffff);
    # reseed in place and warm the stream (so a small mixed seed doesn't yield
    # a degenerate first draw), keeping the shared rng hashref that subs hold.
    my $fresh = _fresh_rng($mix);
    $self->{rng}{seed} = $fresh->{seed};
    return;
}

# ---- helpers to classify a name ----
sub _is_string_name { my $n = shift; return substr($n, -1) eq '$'; }

# ---- special read-only variables (bare idents, no parens) ----
#   usr$  -> user id
#   dat$  -> MM/DD/YY
#   clk$  -> HH:MM:SS
#   (cnt and arg$ are handled by the evaluator via the run context.)
sub _special {
    my ($self, $name) = @_;
    if ($name eq 'usr$') {
        return defined $self->{user} ? $self->{user}
             : (getpwuid($<))[0] // $ENV{USER} // $ENV{LOGNAME} // 'user';
    }
    if ($name eq 'dat$' || $name eq 'clk$') {
        my @lt = localtime(defined $self->{_now} ? $self->{_now} : time);
        if ($name eq 'dat$') {
            return sprintf('%02d/%02d/%02d', $lt[4]+1, $lt[3], $lt[5] % 100);
        } else {
            return sprintf('%02d:%02d:%02d', $lt[2], $lt[1], $lt[0]);
        }
    }
    return undef;   # not a special
}
sub is_special { my ($self, $name) = @_; defined $self->_special($name) ? 1 : 0; }

# ---- run-context accessors used by the evaluator for cnt / arg$ ----
sub arg_count { my ($self) = @_; scalar @{$self->{argv}} }
# arg$(n) is 1-based.  An index below 1 must be rejected BEFORE it reaches the
# Perl array: $argv[$n-1] with $n <= 0 is a negative subscript, which silently
# wraps to the end of the list and hands back a real (but wrong) argument
# instead of failing.  Out of range in either direction reads as the empty
# string, the same as an unset string variable.
sub arg_at    { my ($self, $n) = @_;
                return '' if $n < 1;
                my $v = $self->{argv}[$n-1]; defined $v ? $v : '' }

# ---- scalar read ----
# get_scalar('a')  -> number (0 if unset)
# get_scalar('a$') -> string ("" if unset), or a special value
sub get_scalar {
    my ($self, $name) = @_;
    if (_is_string_name($name)) {
        if (defined(my $sp = $self->_special($name))) { return $sp; }
        return exists $self->{sscalar}{$name} ? $self->{sscalar}{$name} : '';
    } else {
        return exists $self->{nscalar}{$name} ? $self->{nscalar}{$name} : 0;
    }
}

# ---- scalar write ----
sub set_scalar {
    my ($self, $name, $value) = @_;
    die "Invalid variable \"$name\" (read-only)\n"
        if $self->is_special($name);
    if (_is_string_name($name)) { $self->{sscalar}{$name} = "$value"; }
    else                        { $self->{nscalar}{$name} = $value + 0; }
    return $value;
}

# ---- dim: declare an array with given dimension bounds ----
# dims are the DECLARED upper bounds; BASIC arrays are 0..bound inclusive.
# declare('a', [75]) ; declare('b', [100,6]) ; declare('c$', [100])
sub declare_array {
    my ($self, $name, $bounds) = @_;
    my $store = _is_string_name($name) ? $self->{sarray} : $self->{narray};
    for my $b (@$bounds) {
        die "Subscript out of bounds (dim \"$name\": negative bound)\n" if $b < 0;
    }
    my $size = 1; $size *= ($_+1) for @$bounds;   # 0..bound inclusive
    die "Out of room (dim \"$name\" needs $size cells, limit $MAX_ARRAY_CELLS)\n"
        if $size > $MAX_ARRAY_CELLS;
    # aggregate cap: many separately-legal dims must not together exhaust memory.
    # If this name already had an array, its old cells are being replaced.
    my $old = $store->{$name};
    my $old_size = $old ? do { my $n = 1; $n *= ($_+1) for @{$old->{dims}}; $n } : 0;
    my $total = ($self->{total_cells} || 0) - $old_size + $size;
    die "Out of room (total array storage would reach $total cells, "
      . "limit $MAX_TOTAL_CELLS)\n"
        if $total > $MAX_TOTAL_CELLS;
    $self->{total_cells} = $total;
    my $init = _is_string_name($name) ? '' : 0;
    $store->{$name} = { dims => [ @$bounds ], data => [ ($init) x $size ] };
    return;
}

# compute flat index from subscripts (row-major), with bounds checking.
# Arrays used without an explicit dim default to bound 10 in BASIC; we
# auto-create on first access with bound 10 per dimension if not declared.
sub _array_slot {
    my ($self, $name, $subs, $for_write) = @_;
    my $store = _is_string_name($name) ? $self->{sarray} : $self->{narray};
    unless (exists $store->{$name}) {
        # implicit dimension: bound 10 in each subscript position
        $self->declare_array($name, [ (10) x scalar(@$subs) ]);
    }
    my $a = $store->{$name};
    my $dims = $a->{dims};
    die "Wrong number of dimensions for \"$name\"\n"
        if @$subs != @$dims;
    my $flat = 0;
    for my $d (0 .. $#$dims) {
        my $ix = int($subs->[$d]);      # subscripts truncate to integer
        die "Subscript out of bounds ($name subscript $ix, range 0..$dims->[$d])\n"
            if $ix < 0 || $ix > $dims->[$d];
        $flat = $flat * ($dims->[$d] + 1) + $ix;
    }
    return ($a, $flat);
}

# ---- array element read / write ----
sub get_array {
    my ($self, $name, $subs) = @_;
    my ($a, $flat) = $self->_array_slot($name, $subs, 0);
    return $a->{data}[$flat];
}
sub set_array {
    my ($self, $name, $subs, $value) = @_;
    my ($a, $flat) = $self->_array_slot($name, $subs, 1);
    $a->{data}[$flat] = _is_string_name($name) ? "$value" : $value + 0;
    return $value;
}

1;

__END__

=head1 NAME

MBasic::Env - a Multics BASIC variable environment

=head1 DESCRIPTION

Holds one program unit's variables: numeric and string scalars and 1- and
2-dimensional arrays (numeric and string namespaces are separate, so C<a>
and C<a$> are distinct).  Provides the read-only special variables C<usr$>
(user id), C<dat$> (date, C<MM/DD/YY>) and C<clk$> (time, C<HH:MM:SS>).
Uninitialized numeric variables read as 0 and string variables as the empty
string.  Array subscripts truncate to integers and are bounds-checked;
undeclared arrays default to bound 10 per dimension.

=head1 METHODS

=head2 new(%opt)

Constructor.  Options: C<argv> (arrayref, for C<arg$>/C<cnt>), C<user> (for
C<usr$>), C<now> (a fixed epoch time making C<dat$>/C<clk$> deterministic).

=head2 arg_count / arg_at($n)

The run-context accessors behind the C<cnt> and C<arg$(n)> built-ins.
C<arg_at> is 1-based, as C<arg$> is in BASIC; an index outside
C<1 .. arg_count> returns the empty string.

=head2 get_scalar($name) / set_scalar($name, $value)

Read/write a scalar.  Writing a special variable dies.

=head2 declare_array($name, \@bounds)

Declare an array with the given upper bounds (0..bound inclusive per dimension).

=head2 get_array($name, \@subs) / set_array($name, \@subs, $value)

Read/write an array element.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
