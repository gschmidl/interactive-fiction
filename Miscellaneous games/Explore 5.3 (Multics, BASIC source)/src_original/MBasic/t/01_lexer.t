use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use MBasic::Lexer;

# --- basic tokenization ---
my ($ln, $tok) = MBasic::Lexer->tokenize_line('35 let h9$="   "');
is($ln, 35, 'line number parsed');
is($tok->[0]{val}, 'let', 'keyword ident');
is($tok->[1]{val}, 'h9$', 'string var ident with $');
is($tok->[2]{type}, 'op', 'assignment = is op');
is($tok->[2]{val}, '=', '= value');
is($tok->[3]{type}, 'str', 'string literal');
is($tok->[3]{val}, '   ', 'string literal preserves spaces');

# --- embedded "" quote ---
($ln,$tok) = MBasic::Lexer->tokenize_line('180 print "say ""hi"" now"');
is($tok->[1]{val}, 'say "hi" now', 'embedded doubled-quote unescaped');

# --- rem takes rest of line ---
($ln,$tok) = MBasic::Lexer->tokenize_line('10 rem this is, a comment; with punct');
is($tok->[0]{val}, 'rem', 'rem keyword');
is($tok->[1]{type}, 'remtext', 'rem free text token');
like($tok->[1]{val}, qr/this is, a comment; with punct/, 'rem text preserved');

# --- inline apostrophe comment is dropped ---
($ln,$tok) = MBasic::Lexer->tokenize_line("55 let w9\$ = \">x\"  ' trailing comment");
my @vals = map { $_->{val} } @$tok;
ok(!(grep { /trailing comment/ } @vals), 'inline comment dropped');
is($tok->[-1]{type}, 'str', 'last real token is the string, comment gone');

# --- call "name": args ---
($ln,$tok) = MBasic::Lexer->tokenize_line('45 call "exp_before_": h9$, h9$, " "');
is($tok->[0]{val}, 'call', 'call keyword');
is($tok->[1]{type}, 'str', 'called name is a string literal');
is($tok->[1]{val}, 'exp_before_', 'called name value');
is($tok->[2]{val}, ':', 'colon punct');
# args h9$ , h9$ , " "
is($tok->[3]{val}, 'h9$', 'first arg');
is($tok->[4]{val}, ',', 'comma');

# --- file #n: path ---
($ln,$tok) = MBasic::Lexer->tokenize_line('60 file #2: ">site>x"');
is($tok->[0]{val}, 'file', 'file keyword');
is($tok->[1]{val}, '#', 'hash punct');
is($tok->[2]{val}, 2, 'channel number');
is($tok->[3]{val}, ':', 'colon');

# --- operators <> <= >= ---
($ln,$tok) = MBasic::Lexer->tokenize_line('89 if d9$ <> "multip" then 100');
my ($opidx) = grep { $tok->[$_]{val} eq '<>' } 0..$#$tok;
ok(defined $opidx, '<> operator recognized');

# --- on X goto a,b,c ---
($ln,$tok) = MBasic::Lexer->tokenize_line('170 on q2 + 1 goto 180,200,210');
is($tok->[0]{val}, 'on', 'on keyword');
my @nums = grep { $_->{type} eq 'num' } @$tok;
is(scalar(@nums), 4, 'four numeric tokens (1,180,200,210)');

# --- run the lexer over a bundled sample program exercising many token kinds ---
my @sample = (
    '10 rem sample program exercising lexer token kinds',
    '20 let a$ = "hello, world"',
    '30 let x1 = 3.14',
    '40 for i = 1 to 10 step 2',
    '50 if x1 <> 0 then 70',
    '60 print #2: a$; ","; sst$(a$,1,5); tab(20); i',
    '70 next i',
    '80 call "helper": a$, x1, y',
    '90 on x1 goto 100,110,120',
    '100 dim b(75), c$(100,6)',
    '110 input #1: p, q$',
    '120 data "one", 2, "three"',
    '130 f $ = "space before dollar"',
    '140 end',
);
my $ok = 1; my $err = "";
for my $line (@sample) {
    eval { MBasic::Lexer->tokenize_line($line); 1 }
      or do { $ok = 0; $err = "$@ at: $line"; last };
}
ok($ok, "lexed the bundled sample program without error") or diag($err);

done_testing;
