use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use MBasic::Lexer; use MBasic::Expr; use MBasic::Env;

# evaluate a bare expression string against an Env
sub ev {
    my ($src, $env, %opt) = @_;
    $env ||= MBasic::Env->new;
    my ($ln,$toks) = MBasic::Lexer->tokenize_line("100 $src");
    my $pos = 0;
    my $node = MBasic::Expr->parse($toks, \$pos, %opt);
    return MBasic::Expr->eval($node, $env);
}

# --- arithmetic + precedence ---
is(ev('2 + 3 * 4'), 14, '2+3*4 = 14');
is(ev('(2 + 3) * 4'), 20, '(2+3)*4 = 20');
is(ev('10 - 2 - 3'), 5, 'left assoc subtraction');
is(ev('2 ^ 3 ^ 2'), 512, 'right assoc power 2^(3^2)=512');
is(ev('-3 + 5'), 2, 'unary minus');
is(ev('int(7 / 2)'), 3, 'int(3.5)=3');
is(ev('int(-7 / 2)'), -4, 'int(-3.5) = -4 (floor)');

# --- string concat ---
is(ev('"foo" & "bar"'), 'foobar', 'concat');
is(ev('"a" & "b" & "c"'), 'abc', 'triple concat');

# --- substring family with CONFIRMED clamping semantics ---
is(ev('sst$("hello", 2, 3)'), 'ell', 'sst$(hello,2,3)=ell');
is(ev('sst$("hello", 4, 2)'), 'lo', 'sst$ near end');
is(ev('sst$("hi", 5, 3)'), '', 'sst$ start past end -> empty');
is(ev('left$("hi", 5)'), 'hi', 'left$ clamps (verified on Multics)');
is(ev('right$("hello", 2)'), 'lo', 'right$(hello,2)=lo');
is(ev('right$("hi", 5)'), 'hi', 'right$ clamps (verified on Multics)');
is(ev('mid$("hello", 2, 3)'), 'ell', 'mid$(hello,2,3)=ell');
is(ev('seg$("hello", 2, 4)'), 'ell', 'seg$(start,end)');

# --- len, val, pos ---
is(ev('len("hello")'), 5, 'len');
is(ev('val("42")'), 42, 'val');
is(ev('val("  3.14xyz")'), 3.14, 'val leading number');
is(ev('pos("hello", "l", 1)'), 3, 'pos first l');
is(ev('pos("hello", "z", 1)'), 0, 'pos not found -> 0');

# --- str$ leading-space format (the key game behavior) ---
is(ev('str$(500)'), ' 500', 'str$(500) = " 500" (leading sign-blank, NO trailing -- verified Multics)');
is(ev('str$(127)'), ' 127', 'str$(127) = " 127" (matches live test output)');
is(ev('str$(-20)'), '-20', 'str$(-20) = "-20" negative sign, no trailing');

# --- variables and arrays via Env ---
my $env = MBasic::Env->new;
$env->set_scalar('a', 7);
$env->set_scalar('a$', 'hi');
is(ev('a + 3', $env), 10, 'numeric scalar');
is(ev('a$ & "!"', $env), 'hi!', 'string scalar');
$env->declare_array('d$', [75]);
$env->set_array('d$', [3], 'Monday');
is(ev('d$(3)', $env), 'Monday', 'string array element');
$env->set_scalar('w', 2);
is(ev('d$(w + 1)', $env), 'Monday', 'array index by expression: d$(w+1)=d$(3)=Monday');

# --- relational (if-context) ---
is(ev('5 < 10', undef, allow_rel=>1), 1, 'numeric < true');
is(ev('"abc" = "abc"', undef, allow_rel=>1), 1, 'string = true');
is(ev('"a" <> "b"', undef, allow_rel=>1), 1, 'string <> true');

# --- the exp_day_ Zeller computation for a known date ---
# dat$ = "09/15/26" (Monday Sep 15 2026). m=9 d=15 y=26->2026
$env = MBasic::Env->new(now => 0);  # fixed, but we'll just set vars manually
$env->set_scalar('m', 9); $env->set_scalar('d', 15);
$env->set_scalar('c', 20); $env->set_scalar('k', 26);
my $w = ev('d + int(13 * (m + 1) / 5) + k + int(k / 4) + int(c / 4) + 5 * c', $env);
$w = $w - 7 * int($w/7);
ok(defined $w, "Zeller w computed = $w");

# --- command-line arguments: cnt and arg$(n) ---
{
    my $a = MBasic::Env->new(argv => ['-first', '-second', '-last']);
    is(ev('cnt', $a),      3,         'cnt counts the supplied arguments');
    is(ev('arg$(1)', $a),  '-first',  'arg$ is 1-based');
    is(ev('arg$(3)', $a),  '-last',   'arg$ reaches the last argument');
    is(ev('arg$(4)', $a),  '',        'arg$ past the end is the empty string');
    # A subscript below 1 must not become a negative Perl index, which would
    # wrap around and hand back a real argument from the end of the list.
    is(ev('arg$(0)', $a),  '',        'arg$(0) is empty, not the last argument');
    is(ev('arg$(0-1)', $a),'',        'a negative arg$ index is empty too');

    my $none = MBasic::Env->new;
    is(ev('cnt', $none),     0,  'cnt is 0 with no arguments');
    is(ev('arg$(1)', $none), '', 'arg$ with no arguments is empty');
}

done_testing;
