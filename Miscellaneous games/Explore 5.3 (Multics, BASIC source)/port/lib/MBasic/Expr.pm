package MBasic::Expr;
use strict;
use warnings;
our $VERSION = '1.2';

# The source line number of the statement currently being evaluated.  The
# executor sets this before each statement so that run-time errors raised deep
# in expression evaluation can name the BASIC line (not an interpreter file /
# line) in their authentic Multics message.
our $LINE;

# raise a run-time error with the authentic message text plus the BASIC line.
sub _rt {
    my ($msg) = @_;
    die "$msg" . (defined $LINE ? " (line $LINE)" : "") . "\n";
}

# ============================================================================
#  MBasic::Expr -- parse and evaluate Multics BASIC (Explore subset)
#  expressions.
#
#  PARSE (compile time): tokens -> an expr-node tree (plain hashrefs).
#  EVAL  (run time):      expr-node + Env -> a value (Perl number or string).
#
#  Expr-node shapes:
#    { k=>'num',  v=>N }
#    { k=>'str',  v=>S }
#    { k=>'var',  name=>'a$'  }                       scalar variable
#    { k=>'idx',  name=>'a',  args=>[node,...] }      array element a(i) / b(i,j)
#    { k=>'unop', op=>'-'|'+', a=>node }
#    { k=>'binop',op=>'+'|'-'|'*'|'/'|'^'|'&', a=>node, b=>node }
#    { k=>'call', name=>'sst$', args=>[node,...] }    built-in function call
#    { k=>'rel',  op=>'='|'<>'|'<'|'>'|'<='|'>=', a=>node, b=>node }  (only in if)
#
#  Note: a(i) and fn(x) are syntactically identical (name '(' args ')').  We
#  parse both as {k=>'idx'-or-'call'} deciding by whether 'name' is a known
#  built-in function; anything else is treated as an array reference.  (User
#  functions fnX are not used by Explore; not implemented.)
#
#  Numeric operations use Perl doubles (float bin(27) precision difference is
#  immaterial for this game).  String concat is '&'.
#
#  Precedence (from the manual, AM82-01):
#     4 unary -, unary +      (right assoc)
#     3 ^                     (right assoc)
#     2 * /                   (left assoc)
#     1 + -                   (left assoc)
#     ( & is the string concatenation operator; treated at the +/- tier for
#       Explore's usage -- strings and numbers never mix in one operator. )
# ============================================================================

# ---- built-in function table: name => arity (exact), or 'v' for variadic ----
# Arity is checked at PARSE time (Multics reports wrong-arg-count as a
# compile-time error with "no execution").
my %FN_ARITY = (
    'sst$'  => 3, 'seg$' => 3, 'left$' => 2, 'right$' => 2, 'mid$' => 3,
    'len'   => 1, 'val'  => 1, 'str$'  => 1, 'pos'    => 3,
    'int'   => 1, 'abs'  => 1, 'sgn'   => 1, 'sqr'    => 1,
    'chr$'  => 1, 'asc'  => 1, 'tst'   => 1,
    'rnd'   => 0, 'cnt'  => 0,                      # niladic (no parens)
    'arg$'  => 1,
    # (trig/log etc. from the manual can be added if a program needs them)
);
# niladic specials usable as bare identifiers (no parens): usr$ dat$ clk$
#   handled as variables (intercepted by Env), NOT here.
# rnd/cnt: appear as bare idents too (no parens) -> handled in parse of primary.

sub fn_arity { my ($name) = @_; $FN_ARITY{$name} }
sub is_builtin { my ($name) = @_; exists $FN_ARITY{$name} }

# ============================================================================
#  PARSE
#  parse(\@tokens, \$pos, %opt) -> expr-node
#    Consumes tokens starting at $$pos (a shared index the statement parser
#    advances).  Stops at the first token that cannot extend an expression
#    (comma, semicolon, colon, close-paren at depth 0, 'then', 'goto', EOL...).
#    %opt: allow_rel => 1  to permit a top-level relational (for `if`).
# ============================================================================
sub parse {
    my ($class, $toks, $pos, %opt) = @_;
    my $node = _parse_add($toks, $pos);
    if ($opt{allow_rel}) {
        my $t = $toks->[$$pos];
        if ($t && $t->{type} eq 'op' && $t->{val} =~ /^(=|<>|<|>|<=|>=)$/) {
            my $op = $t->{val}; $$pos++;
            my $rhs = _parse_add($toks, $pos);
            $node = { k=>'rel', op=>$op, a=>$node, b=>$rhs };
        }
    }
    return $node;
}

# additive tier: + - &  (left assoc)   [& grouped here per Explore usage]
sub _parse_add {
    my ($toks, $pos) = @_;
    my $left = _parse_mul($toks, $pos);
    while (1) {
        my $t = $toks->[$$pos];
        last unless $t && $t->{type} eq 'op' && $t->{val} =~ /^[+\-&]$/;
        my $op = $t->{val}; $$pos++;
        my $right = _parse_mul($toks, $pos);
        $left = { k=>'binop', op=>$op, a=>$left, b=>$right };
    }
    return $left;
}

# multiplicative tier: * /  (left assoc)
sub _parse_mul {
    my ($toks, $pos) = @_;
    my $left = _parse_pow($toks, $pos);
    while (1) {
        my $t = $toks->[$$pos];
        last unless $t && $t->{type} eq 'op' && $t->{val} =~ m{^[*/]$};
        my $op = $t->{val}; $$pos++;
        my $right = _parse_pow($toks, $pos);
        $left = { k=>'binop', op=>$op, a=>$left, b=>$right };
    }
    return $left;
}

# power tier: ^  (right assoc)
sub _parse_pow {
    my ($toks, $pos) = @_;
    my $left = _parse_unary($toks, $pos);
    my $t = $toks->[$$pos];
    if ($t && $t->{type} eq 'op' && $t->{val} eq '^') {
        $$pos++;
        my $right = _parse_pow($toks, $pos);   # right assoc
        return { k=>'binop', op=>'^', a=>$left, b=>$right };
    }
    return $left;
}

# unary tier: - +  (right assoc)
sub _parse_unary {
    my ($toks, $pos) = @_;
    my $t = $toks->[$$pos];
    if ($t && $t->{type} eq 'op' && ($t->{val} eq '-' || $t->{val} eq '+')) {
        my $op = $t->{val}; $$pos++;
        my $operand = _parse_unary($toks, $pos);
        return { k=>'unop', op=>$op, a=>$operand };
    }
    return _parse_primary($toks, $pos);
}

# primary: number | string | ( expr ) | ident [ ( args ) ]
sub _parse_primary {
    my ($toks, $pos) = @_;
    my $t = $toks->[$$pos];
    die "expr error: unexpected end of expression\n" unless $t;

    if ($t->{type} eq 'num') { $$pos++; return { k=>'num', v=>$t->{val} }; }
    if ($t->{type} eq 'str') { $$pos++; return { k=>'str', v=>$t->{val} }; }

    if ($t->{type} eq 'punct' && $t->{val} eq '(') {
        $$pos++;
        my $inner = _parse_add($toks, $pos);
        my $c = $toks->[$$pos];
        die "expr error: expected ')'\n"
            unless $c && $c->{type} eq 'punct' && $c->{val} eq ')';
        $$pos++;
        return $inner;
    }

    if ($t->{type} eq 'ident') {
        my $name = $t->{val};
        $$pos++;
        # niladic specials as bare idents (no parens): rnd, cnt, usr$, dat$, clk$
        my $next = $toks->[$$pos];
        my $has_paren = $next && $next->{type} eq 'punct' && $next->{val} eq '(';
        if (!$has_paren) {
            # bare identifier: a niladic builtin, a special var, or a scalar var
            if ($name eq 'rnd' || $name eq 'cnt') {
                return { k=>'call', name=>$name, args=>[] };
            }
            return { k=>'var', name=>$name };
        }
        # has '(' : function call OR array reference
        $$pos++;   # consume '('
        my @args;
        unless ($toks->[$$pos] && $toks->[$$pos]{type} eq 'punct'
                && $toks->[$$pos]{val} eq ')') {
            push @args, _parse_add($toks, $pos);
            while ($toks->[$$pos] && $toks->[$$pos]{type} eq 'punct'
                   && $toks->[$$pos]{val} eq ',') {
                $$pos++;
                push @args, _parse_add($toks, $pos);
            }
        }
        my $c = $toks->[$$pos];
        die "expr error: expected ')' after args to '$name'\n"
            unless $c && $c->{type} eq 'punct' && $c->{val} eq ')';
        $$pos++;
        if (is_builtin($name)) {
            my $ar = fn_arity($name);
            die "Wrong number of arguments for \"$name\"\n"
              if $ar ne 'v' && @args != $ar;
            return { k=>'call', name=>$name, args=>\@args };
        }
        # otherwise: array element reference
        return { k=>'idx', name=>$name, args=>\@args };
    }

    die "expr error: unexpected token '".($t->{val}//'?')."' in expression\n";
}

# ============================================================================
#  EVAL   expr-node + Env -> Perl scalar (number or string)
# ============================================================================
sub eval {
    my ($class, $node, $env) = @_;
    my $k = $node->{k};

    if ($k eq 'num')  { return $node->{v}; }
    if ($k eq 'str')  { return $node->{v}; }
    if ($k eq 'var')  { return $env->get_scalar($node->{name}); }
    if ($k eq 'idx')  {
        my @subs = map { $class->eval($_, $env) } @{$node->{args}};
        return $env->get_array($node->{name}, \@subs);
    }
    if ($k eq 'unop') {
        my $a = $class->eval($node->{a}, $env);
        return $node->{op} eq '-' ? -$a : +$a;
    }
    if ($k eq 'binop') {
        my $op = $node->{op};
        if ($op eq '&') {   # string concat
            return $class->eval($node->{a}, $env) . $class->eval($node->{b}, $env);
        }
        my $a = $class->eval($node->{a}, $env);
        my $b = $class->eval($node->{b}, $env);
        return $a + $b if $op eq '+';
        return $a - $b if $op eq '-';
        return $a * $b if $op eq '*';
        if ($op eq '/') {
            _rt("Division by zero") if $b == 0;
            return $a / $b;
        }
        if ($op eq '^') {
            # Multics reports these as run-time errors rather than returning
            # Inf/NaN the way Perl's ** does (errata 095/096/1.2).
            if ($a == 0) {
                _rt("Zero power of zero")     if $b == 0;
                _rt("Negative power of zero")  if $b < 0;
            }
            _rt("Power of negative number") if $a < 0 && $b != int($b);
            return $a ** $b;
        }
        die "internal: unknown binop $op\n";
    }
    if ($k eq 'rel') {
        my $a = $class->eval($node->{a}, $env);
        my $b = $class->eval($node->{b}, $env);
        my $op = $node->{op};
        # string vs numeric comparison: if either side is a string node result,
        # BASIC compares by type (both sides same type per the grammar).  We
        # detect stringness structurally: string literals / string vars / string
        # functions.  Simpler: if both look numeric, compare numerically; else
        # ASCII string compare.  Explore's `if` always has matching types.
        my $strcmp = _is_string_expr($node->{a}) || _is_string_expr($node->{b});
        my $r = $strcmp
              ? ( $op eq '='  ? ($a eq $b)
                : $op eq '<>' ? ($a ne $b)
                : $op eq '<'  ? ($a lt $b)
                : $op eq '>'  ? ($a gt $b)
                : $op eq '<=' ? ($a le $b)
                :               ($a ge $b) )
              : ( $op eq '='  ? ($a == $b)
                : $op eq '<>' ? ($a != $b)
                : $op eq '<'  ? ($a <  $b)
                : $op eq '>'  ? ($a >  $b)
                : $op eq '<=' ? ($a <= $b)
                :               ($a >= $b) );
        return $r ? 1 : 0;
    }
    if ($k eq 'call') {
        return _call_builtin($class, $node->{name}, $node->{args}, $env);
    }
    die "internal: unknown expr node kind '$k'\n";
}

# structural check: does this expr-node yield a string?  (string literal,
# $-suffixed variable/array, or a $-suffixed builtin like sst$/left$/...).
sub _is_string_expr {
    my ($n) = @_;
    return 1 if $n->{k} eq 'str';
    return (substr($n->{name},-1) eq '$') if $n->{k} eq 'var' || $n->{k} eq 'idx';
    return (substr($n->{name},-1) eq '$') if $n->{k} eq 'call';
    return _is_string_expr($n->{a}) if $n->{k} eq 'binop' && $n->{op} eq '&';
    return 0;
}

# ---- built-in function implementations ----
# Substring family: all clamp per Multics (verified live).  sst$ formula
# from the manual: i' = max(i,1); length' = max(min(len, S-i'+1), 0),
# where S = len(string).  left$/right$/mid$ map onto the same clamping.
sub _substr_clamped {
    my ($s, $start, $len) = @_;              # 1-based start, wanted length
    my $S = length $s;
    my $i = $start < 1 ? 1 : int($start);
    my $avail = $S - $i + 1; $avail = 0 if $avail < 0;
    my $l = int($len); $l = $avail if $l > $avail; $l = 0 if $l < 0;
    return '' if $l <= 0;
    return substr($s, $i - 1, $l);
}

sub _call_builtin {
    my ($class, $name, $argnodes, $env) = @_;
    my @a = map { $class->eval($_, $env) } @$argnodes;

    if ($name eq 'sst$')  { return _substr_clamped($a[0], $a[1], $a[2]); }
    if ($name eq 'left$') { return _substr_clamped($a[0], 1, $a[1]); }
    if ($name eq 'right$'){ my $S=length $a[0]; return _substr_clamped($a[0], $S-$a[1]+1, $a[1]); }
    if ($name eq 'mid$')  { return _substr_clamped($a[0], $a[1], $a[2]); }
    if ($name eq 'seg$')  {   # (start,end) inclusive, clamped
        my $S = length $a[0];
        my $i = $a[1] < 1 ? 1 : int($a[1]);
        my $j = $a[2] > $S ? $S : int($a[2]);
        return '' if $j < $i;
        return substr($a[0], $i-1, $j-$i+1);
    }
    if ($name eq 'len')   { return length($a[0]); }
    if ($name eq 'val')   { return _to_number($a[0]); }
    if ($name eq 'str$')  { return _num_to_str($a[0]); }
    if ($name eq 'int')   { return _floor($a[0]); }     # largest int <= x
    if ($name eq 'abs')   { return abs($a[0]); }
    if ($name eq 'sgn')   { return $a[0] <=> 0; }
    if ($name eq 'sqr')   { _rt("Square root of negative number") if $a[0] < 0;
                            return sqrt($a[0]); }
    if ($name eq 'chr$')  { return chr(int($a[0]) % 128); }
    if ($name eq 'asc')   { _rt('Invalid "ASC" function arg') if length($a[0]) == 0;
                            return ord(substr($a[0],0,1)); }
    if ($name eq 'tst')   { return _looks_numeric($a[0]) ? 1 : 0; }
    if ($name eq 'pos')   {   # 1-based location of b$ in a$ at/after i; 0 if none
        my ($s,$sub,$i) = @a;
        return 0 if $i < 1 || $i > length($s);
        my $p = index($s, $sub, $i-1);
        return $p < 0 ? 0 : $p + 1;
    }
    if ($name eq 'rnd')   { return $env->rnd; }         # per-program repeatable RNG
    if ($name eq 'cnt')   { return $env->arg_count; }
    if ($name eq 'arg$')  { return $env->arg_at(int($a[0])); }

    die "Unimplemented run-time operator (\"$name\")\n";
}

# largest integer not greater than x (BASIC int()/floor)
sub _floor { my $x = shift; my $i = int($x); return ($x < 0 && $i != $x) ? $i-1 : $i; }

# numeric coercion for val(): parse a leading BASIC numeric constant
sub _to_number {
    my $s = shift;
    $s =~ s/^\s+//;
    return ($s =~ /^([+-]?(?:\d+\.\d+|\.\d+|\d+)(?:[eE][+-]?\d+)?)/) ? $1 + 0 : 0;
}
sub _looks_numeric {
    my $s = shift; $s =~ s/^\s+//; $s =~ s/\s+$//;
    return $s =~ /^[+-]?(?:\d+\.\d+|\.\d+|\d+)(?:[eE][+-]?\d+)?$/ ? 1 : 0;
}

# number -> string per Multics str$ rules.  VERIFIED on real Multics:
#   str$(127) yields " 127" -- a LEADING sign-blank (blank if >=0, '-' if <0)
#   and the digits, but NO trailing blank.  (The manual's "ends with a blank"
#   rule applies to PRINT output of numbers -- where the trailing blank is a
#   field separator -- NOT to str$.  print and str$ differ in the trailing
#   position.)  Integer format if integral and |x| < 2^27; else fractional
#   (<=6 significant digits, trailing zeros trimmed); else scientific.
sub _num_to_str {
    my ($x) = @_;
    my $sign = $x < 0 ? '-' : ' ';
    my $mag  = abs($x);
    my $body;
    if ($mag == int($mag) && $mag < 134_217_728) {
        $body = sprintf('%d', $mag);
    } else {
        # fractional / large: up to 6 significant digits, Multics-style.
        $body = _g6($mag);
    }
    return $sign . $body;      # leading sign-blank, NO trailing blank
}

# Format a non-negative magnitude to 6 significant digits in Multics BASIC
# style: a plain decimal (trailing zeros trimmed) when it fits, otherwise
# scientific notation with an UPPERCASE 'E', an explicit exponent sign, and a
# two-digit exponent (e.g. "2E+08") -- NOT Perl/C's lowercase "2e+08".
sub _g6 {
    my ($mag) = @_;
    my $body = sprintf('%.6g', $mag);
    if ($body =~ /^([0-9.]+)[eE]([+-]?)(\d+)$/) {
        my ($m, $sgn, $exp) = ($1, $2, $3);
        $sgn = '+' unless length $sgn;
        $exp = sprintf('%02d', $exp);
        $body = "${m}E${sgn}${exp}";
    }
    return $body;
}

1;

__END__

=head1 NAME

MBasic::Expr - parse and evaluate Multics BASIC expressions

=head1 DESCRIPTION

Parses a token stream into an expression-node tree and evaluates such a
tree against an L<MBasic::Env>.  Implements operator precedence (unary,
C<^>, C<* />, C<+ - &>), the relational operators (only permitted in an
C<if> context), and the built-in functions.  Built-in argument counts are
checked at parse time, matching Multics' compile-time "wrong number of
arguments" error.

=head1 METHODS

=head2 parse(\@tokens, \$pos, %opt)

Class method.  Parses an expression beginning at C<$$pos> (advanced in place).
With C<< allow_rel => 1 >> a top-level relational operator is permitted.
Returns an expression node (a plain hashref).

=head2 eval($node, $env)

Class method.  Evaluates an expression node against an L<MBasic::Env>, returning
a Perl number or string.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
