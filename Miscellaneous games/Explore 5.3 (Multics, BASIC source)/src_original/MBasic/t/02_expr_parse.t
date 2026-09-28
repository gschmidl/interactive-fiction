use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use MBasic::Lexer;
use MBasic::Expr;

# helper: lex a bare expression (prepend a dummy line number) and parse it
sub parse_expr {
    my ($src, %opt) = @_;
    my ($ln, $toks) = MBasic::Lexer->tokenize_line("100 $src");
    my $pos = 0;
    my $node = MBasic::Expr->parse($toks, \$pos, %opt);
    return ($node, $pos, $toks);
}

# --- precedence: a + b * c  ->  a + (b*c) ---
my ($n) = parse_expr('a + b * c');
is($n->{k}, 'binop', 'top is binop');
is($n->{op}, '+', 'top op is +');
is($n->{b}{op}, '*', 'right side is *');

# --- left assoc: a - b - c -> (a-b)-c ---
($n) = parse_expr('a - b - c');
is($n->{op}, '-', 'top -');
is($n->{a}{k}, 'binop', 'left nests');
is($n->{a}{op}, '-', 'left is a-b');

# --- power right assoc: a ^ b ^ c -> a^(b^c) ---
($n) = parse_expr('a ^ b ^ c');
is($n->{op}, '^', 'top ^');
is($n->{b}{op}, '^', 'right nests (right assoc)');

# --- unary minus ---
($n) = parse_expr('-a * b');
is($n->{k}, 'binop', 'unary binds tighter: (-a)*b');
is($n->{a}{k}, 'unop', 'left is unary');

# --- string concat ---
($n) = parse_expr('h9$ & ">" & usr$');
is($n->{op}, '&', 'concat op');

# --- built-in with correct arity: sst$(a$,1,2) ---
($n) = parse_expr('sst$(dat$, 1, 2)');
is($n->{k}, 'call', 'sst$ is a call');
is($n->{name}, 'sst$', 'name');
is(scalar @{$n->{args}}, 3, 'three args');

# --- left$/mid$/right$ recognized as builtins ---
($n) = parse_expr('left$(dat$, 2)');
is($n->{k}, 'call', 'left$ is a builtin call');
($n) = parse_expr('mid$(dat$, 4, 2)');
is($n->{name}, 'mid$', 'mid$ builtin');

# --- WRONG arity is a compile-time error (matching Multics) ---
eval { parse_expr('mid$(dat$, 4)') };   # 2-arg mid$ -> error
like($@, qr/wrong number of arguments.*mid/i, '2-arg mid$ rejected at parse time');

# --- array reference vs function: a(x) is array (a not a builtin) ---
($n) = parse_expr('a(x)');
is($n->{k}, 'idx', 'a(x) is array reference');
($n) = parse_expr('b(i, j)');
is($n->{k}, 'idx', '2-D array reference');
is(scalar @{$n->{args}}, 2, 'two subscripts');

# --- niladic builtins: rnd, cnt (no parens) ---
($n) = parse_expr('rnd');
is($n->{k}, 'call', 'rnd is niladic call');
is(scalar @{$n->{args}}, 0, 'rnd no args');
($n) = parse_expr('int(6 * rnd)');
is($n->{name}, 'int', 'int(...)');

# --- relational only when allowed (if context) ---
($n) = parse_expr('d9$ <> "multip"', allow_rel=>1);
is($n->{k}, 'rel', 'relational parsed in if-context');
is($n->{op}, '<>', '<> op');

# --- the exp_day_ Zeller expression (a real, complex one) ---
($n) = parse_expr('d + int(13 * (m + 1) / 5) + k + int(k / 4) + int(c / 4) + 5 * c');
is($n->{k}, 'binop', 'complex arithmetic parses');

# --- parenthesized ---
($n) = parse_expr('(a + b) / e');
is($n->{op}, '/', 'paren grouping: (a+b)/e');
is($n->{a}{op}, '+', 'left is (a+b)');

done_testing;
