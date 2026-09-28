use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use MBasic::Interp; use MBasic::Registry;

# helper: build an interp with given helper-sub lines + main lines, run, capture out
sub run_with_subs {
    my ($helpers, $main, %opt) = @_;
    my $reg = MBasic::Registry->new;
    if ($opt{builtins}) { $opt{builtins}->($reg); }
    my $interp = MBasic::Interp->new(registry => $reg, now => $opt{now}, argv => $opt{argv}||[]);
    my $hi = 0;
    for my $h (@$helpers) { $interp->load_helper_lines($h, "H".$hi++); }
    $interp->load_helper_lines($main, "MAIN");
    $interp->{main} = $interp->{programs}{"MAIN"};
    my $out = '';
    $interp->run(out => sub { $out .= $_[0] });
    return $out;
}

# --- call a BASIC sub that writes back a string out-param ---
my $sub_double = ['10 sub "dbl": x$',
                  '20 let x$ = x$ & x$',
                  '30 subend'];
is(run_with_subs([$sub_double],
    ['10 let a$ = "ab"', '20 call "dbl": a$', '30 print a$', '40 end']),
   "abab\n", 'sub writes back string out-param');

# --- fresh environment: sub's locals do NOT leak to caller ---
my $sub_uses_i = ['10 sub "loop": r',
                  '20 for i = 1 to 100',
                  '30 next i',
                  '40 let r = 42',
                  '50 subend'];
is(run_with_subs([$sub_uses_i],
    ['10 let i = 7', '20 call "loop": x', '30 print i; x', '40 end']),
   " 7  42 \n", "caller's i unaffected by sub's i (fresh env)");

# --- numeric param write-back ---
my $sub_addone = ['10 sub "inc": n', '20 let n = n + 1', '30 subend'];
is(run_with_subs([$sub_addone],
    ['10 let v = 10', '20 call "inc": v', '30 print v', '40 end']),
   " 11 \n", 'numeric out-param write-back');

# --- multiple params ---
my $sub_swap = ['10 sub "combine": a$, b$, r$',
                '20 let r$ = a$ & "-" & b$',
                '30 subend'];
is(run_with_subs([$sub_swap],
    ['10 let x$="L"', '20 let y$="R"', '30 call "combine": x$, y$, z$',
     '40 print z$', '50 end']),
   "L-R\n", 'multiple params, one out');

# --- value (non-lvalue) args: literal passed by value, no write-back needed ---
is(run_with_subs([$sub_double],
    ['10 call "dbl": "hi"', '20 print "ok"', '30 end']),
   "ok\n", 'literal arg (value) to sub does not crash');

# --- a NATIVE builtin via the registry ---
is(run_with_subs([],
    ['10 call "upcase": s$', '20 print s$', '30 end'],
    builtins => sub {
        my $reg = shift;
        $reg->register('upcase', sub {
            my ($ctx, $args) = @_;
            $args->[0]->set( uc($args->[0]->get) );
        });
    }),
   "\n", 'native builtin runs (s$ empty -> uc empty)');

# native builtin that sets a value
is(run_with_subs([],
    ['10 let s$="hi"', '20 call "upcase": s$', '30 print s$', '40 end'],
    builtins => sub {
        my $reg = shift;
        $reg->register('upcase', sub { my ($ctx,$a)=@_; $a->[0]->set(uc $a->[0]->get); });
    }),
   "HI\n", 'native builtin uppercases via write-back');

# --- undefined call -> error ---
eval { run_with_subs([], ['10 call "nonesuch": x', '20 end']) };
like($@, qr/missing function.*nonesuch/i, "undefined call errors (errata: missing function)");

# --- the real exp_day_ helper as a sub (Zeller) ---
my @day = split /\n/, <<'DAY';
100 sub "exp_day_": r$
120 let m = val(left$(dat$, 2))
130 let d = val(mid$(dat$, 4, 2))
140 let y = val(right$(dat$, 2))
150 if y < 50 then 170
160 let y = y + 1900
165 goto 180
170 let y = y + 2000
180 if m > 2 then 210
190 let m = m + 12
200 let y = y - 1
210 let c = int(y / 100)
220 let k = y - 100 * c
230 let w = d + int(13 * (m + 1) / 5) + k + int(k / 4) + int(c / 4) + 5 * c
240 let w = w - 7 * int(w / 7)
250 dim d$(7)
260 for i = 1 to 7
270 read d$(i)
280 next i
290 data "Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"
300 let r$ = d$(w + 1)
310 subend
DAY
# now=1789200000 is 09/12/26 which is a Saturday
is(run_with_subs([[@day]],
    ['10 call "exp_day_": y$', '20 print y$', '30 end'],
    now => 1789200000),
   "Saturday\n", 'real exp_day_ helper called as sub -> Saturday for 09/12/26');

done_testing;
