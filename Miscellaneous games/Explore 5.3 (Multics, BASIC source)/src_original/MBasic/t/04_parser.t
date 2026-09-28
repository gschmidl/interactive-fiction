use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use MBasic::Parser;

sub P { my ($src)=@_; my ($ln,$rec)=MBasic::Parser->parse_line($src); return $rec; }

# let (scalar and array targets)
my $r = P('35 let h9$ = "x"');
is($r->{op}, 'let', 'let op'); is($r->{lhs}{name}, 'h9$', 'let scalar lhs');
$r = P('670 let w(1,x) = val(sst$(y$,1,y-1))');
is($r->{lhs}{name}, 'w', 'let array lhs'); is(scalar @{$r->{lhs}{subs}}, 2, 'array 2 subs');

# print with ; , tab and channel
$r = P('180 print "Version 5.3"');
is($r->{op},'print','print op'); is($r->{items}[0]{kind},'expr','print expr item');
$r = P('1700 print tab(20);sst$(f$(1),1,5);" - ";sst$(j$(1),1,5)');
is($r->{items}[0]{kind},'tab','tab element'); 
$r = P('3740 print #2: dat$;",";f$;",(";r$;" points)"');
ok(defined $r->{chan}, 'print channel');
$r = P('3675 print "text";');
is($r->{items}[-1]{kind},'sep','trailing ; sep');

# goto / gosub / return / stop / end / randomize
is(P('100 goto 75')->{target}, 75, 'goto target');
is(P('1640 gosub 1660')->{op}, 'gosub', 'gosub');
is(P('1800 return')->{op}, 'return', 'return');
is(P('1920 stop')->{op}, 'stop', 'stop');
is(P('100 randomize')->{op}, 'randomize', 'randomize');

# if forms
$r = P('75 if d9$ = "none" then 100');
is($r->{op},'if','if op'); is($r->{target},100,'if target'); is($r->{cond}{k},'rel','if cond is rel');
$r = P('65 if end #2 then 100');
is($r->{op},'ifend','if end'); is($r->{target},100,'ifend target');
$r = P('2175 if more #1 goto 200');
is($r->{op},'ifmore','if more');

# on X goto
$r = P('170 on q2 + 1 goto 180,200,210');
is($r->{op},'on','on op'); is($r->{kind},'goto','on goto'); is(scalar @{$r->{targets}},3,'3 targets');

# for / next
$r = P('260 for i = 1 to 7');
is($r->{op},'for','for'); is($r->{var},'i','for var');
$r = P('280 next i');
is($r->{op},'next','next'); is($r->{var},'i','next var');

# input / linput with channel and terminal
$r = P('940 input #2: a$(x),a(x),d$(x),s(x)');
is($r->{op},'input','input'); ok(defined $r->{chan},'input chan'); is(scalar @{$r->{vars}},4,'4 input vars');
$r = P('70 linput #2: d9$');
is($r->{op},'linput','linput'); is($r->{var}{name},'d9$','linput var');

# file / scratch / reset
$r = P('60 file #2: ">site>x"');
is($r->{op},'file','file'); 
is(P('4900 scratch #2')->{op},'scratch','scratch');
is(P('2100 reset #2')->{op},'reset','reset');

# dim (multiple)
$r = P('430 dim a(75),a$(75),b(100,6),c(100,6),d$(75),s(75)');
is($r->{op},'dim','dim'); is(scalar @{$r->{decls}},6,'6 dim decls');
is($r->{decls}[2]{name},'b','third decl name'); is(scalar @{$r->{decls}[2]{bounds}},2,'b is 2-D');

# call
$r = P('45 call "exp_before_": h9$, h9$, " "');
is($r->{op},'call','call'); is($r->{name},'exp_before_','call name'); is(scalar @{$r->{args}},3,'3 call args');
$r = P('400 call "quit_off"');
is(scalar @{$r->{args}},0,'call no args');

# sub / subend / data / read (from helpers)
$r = P('110 sub "exp_day_": r$');
is($r->{op},'sub','sub'); is($r->{name},'exp_day_','sub name'); is($r->{params}[0],'r$','sub param');
is(P('310 subend')->{op},'subend','subend');
$r = P('290 data "Saturday", "Sunday", "Monday"');
is($r->{op},'data','data'); is(scalar @{$r->{values}},3,'3 data values'); is($r->{values}[0]{str},'Saturday','data value');
$r = P('270 read d$(i)');
is($r->{op},'read','read'); is($r->{vars}[0]{name},'d$','read var');

# rem
is(P('10 rem hello')->{op},'rem','rem');

done_testing;
