package MBasic::Parser;
use strict;
use warnings;
our $VERSION = '1.2';
use MBasic::Lexer;
use MBasic::Expr;

# ============================================================================
#  MBasic::Parser -- tokens -> IR statement records.
#
#  IR RECORD SHAPE (the parser<->executor contract).  Each is a plain hashref
#  with an 'op' key.  Expression operands are Expr nodes (see MBasic::Expr).
#  An "lvalue" is { name=>'a$' } (scalar) or { name=>'a', subs=>[node...] }.
#
#    { op=>'rem' }                                            (no-op)
#    { op=>'let',    lhs=>lvalue, expr=>node }
#    { op=>'print',  chan=>node|undef, items=>[ printitem... ] }
#         printitem: { kind=>'expr', node=>n }
#                    { kind=>'sep',  sep=>','|';' }
#                    { kind=>'tab',  node=>n }
#    { op=>'goto',   target=>lineno }        (rewritten to index by linker)
#    { op=>'gosub',  target=>lineno }
#    { op=>'return' }
#    { op=>'if',     cond=>relnode, target=>lineno }
#    { op=>'ifend',  chan=>node, neg=>0, target=>lineno }   (if end #n)
#    { op=>'ifmore', chan=>node, neg=>0, target=>lineno }   (if more #n)
#    { op=>'on',     expr=>node, kind=>'goto'|'gosub', targets=>[lineno...] }
#    { op=>'for',    var=>'i', from=>node, to=>node, step=>node|undef }
#    { op=>'next',   var=>'i' }
#    { op=>'input',  chan=>node|undef, vars=>[lvalue...], trailing_comma=>0/1 }
#    { op=>'linput', chan=>node|undef, var=>lvalue }
#    { op=>'file',   chan=>node, path=>node }
#    { op=>'scratch',chan=>node }
#    { op=>'reset',  chan=>node }
#    { op=>'dim',    decls=>[ { name=>'a', bounds=>[node...] } ... ] }
#    { op=>'call',   name=>'exp_x_', args=>[ node... ] }
#    { op=>'sub',    name=>'exp_x_', params=>['s','d'] }
#    { op=>'subend' }
#    { op=>'data',   values=>[ const... ] }   (const = number or string, raw)
#    { op=>'read',   vars=>[lvalue...] }
#    { op=>'randomize' }
#    { op=>'stop' } / { op=>'end' }
#
#  Every record also carries {line=>N} (the source line number) for errors.
# ============================================================================

# parse_line($text) -> ($lineno, $ir_record)
sub parse_line {
    my ($class, $text) = @_;
    my ($lineno, $toks) = MBasic::Lexer->tokenize_line($text);
    my $rec = $class->_parse_tokens($lineno, $toks);
    $rec->{line} = $lineno;
    return ($lineno, $rec);
}

# ---- token-cursor helpers ----
sub _peek { my ($t,$p)=@_; $t->[$$p]; }
sub _kw   { my ($t,$p)=@_; my $x=$t->[$$p]; ($x && $x->{type} eq 'ident') ? $x->{val} : undef; }
sub _eat_punct {
    my ($t,$p,$v,$ln)=@_;
    my $x=$t->[$$p];
    die "parse error (line $ln): expected '$v'\n"
        unless $x && $x->{type} eq 'punct' && $x->{val} eq $v;
    $$p++;
}
sub _is_punct { my ($t,$p,$v)=@_; my $x=$t->[$$p]; $x && $x->{type} eq 'punct' && $x->{val} eq $v; }
sub _at_end   { my ($t,$p)=@_; $$p >= scalar(@$t); }

# Explicit keyword -> handler dispatch table.  Built from the statement
# grammar, NOT from method names via ->can, so no future helper method named
# _stmt_* can silently become a reachable statement keyword.
my %DISPATCH = map { $_ => "_stmt_$_" } qw(
    rem let print goto gosub return stop end randomize subend
    if on for next input linput file scratch reset dim call sub data read
);

sub _parse_tokens {
    my ($class, $ln, $toks) = @_;
    my $p = 0;
    my $kw = _kw($toks, \$p);
    die "parse error (line $ln): statement does not begin with a keyword\n"
        unless defined $kw;

    # dispatch on the statement keyword ('if end'/'if more' handled in _stmt_if)
    my $m = $DISPATCH{$kw}
        or die "parse error (line $ln): unknown/unimplemented statement '$kw'\n";
    $p = 1;   # consume the keyword; each handler starts after it
    my $rec = $class->$m($ln, $toks, \$p);

    # fail loudly on tokens the statement did not consume, so a construct
    # outside the implemented subset (e.g. a dropped `else`, or trailing
    # garbage) is rejected rather than silently mis-run.  `rem` keeps its
    # free-text remainder token by design, so it is exempt.
    die "parse error (line $ln): extra tokens after statement\n"
        unless $rec->{op} eq 'rem' || _at_end($toks, \$p);

    return $rec;
}

# ---- lvalue: name  or  name(subs) ----
sub _parse_lvalue {
    my ($class, $ln, $t, $p) = @_;
    my $x = $t->[$$p];
    die "parse error (line $ln): expected a variable\n"
        unless $x && $x->{type} eq 'ident';
    my $name = $x->{val}; $$p++;
    if (_is_punct($t,$p,'(')) {
        $$p++;
        my @subs = ( MBasic::Expr->parse($t, $p) );
        while (_is_punct($t,$p,',')) { $$p++; push @subs, MBasic::Expr->parse($t,$p); }
        _eat_punct($t,$p,')',$ln);
        return { name=>$name, subs=>\@subs };
    }
    return { name=>$name };
}

# ---- individual statements ----

sub _stmt_rem { my ($c,$ln,$t,$p)=@_; return { op=>'rem' }; }

sub _stmt_let {
    my ($c,$ln,$t,$p)=@_;
    my $lhs = $c->_parse_lvalue($ln,$t,$p);
    my $x = $t->[$$p];
    die "parse error (line $ln): expected '=' in let\n"
        unless $x && $x->{type} eq 'op' && $x->{val} eq '=';
    $$p++;
    my $expr = MBasic::Expr->parse($t,$p);
    return { op=>'let', lhs=>$lhs, expr=>$expr };
}

sub _stmt_print {
    my ($c,$ln,$t,$p)=@_;
    my $chan;
    if (_is_punct($t,$p,'#')) { $$p++; $chan = MBasic::Expr->parse($t,$p);
                                _eat_punct($t,$p,':',$ln); }
    my @items;
    while (!_at_end($t,$p)) {
        if (_is_punct($t,$p,';')) { $$p++; push @items,{kind=>'sep',sep=>';'}; next; }
        if (_is_punct($t,$p,',')) { $$p++; push @items,{kind=>'sep',sep=>','}; next; }
        # tab(e) print element
        if (_kw($t,$p) && $t->[$$p]{val} eq 'tab'
            && $t->[$$p+1] && $t->[$$p+1]{type} eq 'punct' && $t->[$$p+1]{val} eq '(') {
            $$p += 2;
            my $e = MBasic::Expr->parse($t,$p);
            _eat_punct($t,$p,')',$ln);
            push @items,{kind=>'tab',node=>$e}; next;
        }
        my $e = MBasic::Expr->parse($t,$p, allow_rel=>0);
        push @items,{kind=>'expr',node=>$e};
    }
    return { op=>'print', chan=>$chan, items=>\@items };
}

sub _stmt_goto {
    my ($c,$ln,$t,$p)=@_;
    my $x=$t->[$$p]; die "parse error (line $ln): goto needs a line number\n"
        unless $x && $x->{type} eq 'num';
    $$p++; return { op=>'goto', target=>$x->{val} };
}
sub _stmt_gosub {
    my ($c,$ln,$t,$p)=@_;
    my $x=$t->[$$p]; die "parse error (line $ln): gosub needs a line number\n"
        unless $x && $x->{type} eq 'num';
    $$p++; return { op=>'gosub', target=>$x->{val} };
}
sub _stmt_return { my ($c,$ln,$t,$p)=@_; return { op=>'return' }; }
sub _stmt_stop   { my ($c,$ln,$t,$p)=@_; return { op=>'stop' }; }
sub _stmt_end    { my ($c,$ln,$t,$p)=@_; return { op=>'end' }; }
sub _stmt_randomize { my ($c,$ln,$t,$p)=@_; return { op=>'randomize' }; }
sub _stmt_subend { my ($c,$ln,$t,$p)=@_; return { op=>'subend' }; }

sub _stmt_if {
    my ($c,$ln,$t,$p)=@_;
    # if end #n then N   |   if more #n then N   |   if e1 rel e2 then N
    my $next = _kw($t,$p);
    if (defined $next && ($next eq 'end' || $next eq 'more')) {
        $$p++;  # consume end/more
        _eat_punct($t,$p,'#',$ln);
        my $chan = MBasic::Expr->parse($t,$p);
        my $target = $c->_if_target($ln,$t,$p);
        return { op => ($next eq 'end' ? 'ifend' : 'ifmore'), chan=>$chan, target=>$target };
    }
    my $cond = MBasic::Expr->parse($t,$p, allow_rel=>1);
    my $target = $c->_if_target($ln,$t,$p);
    return { op=>'if', cond=>$cond, target=>$target };
}
# after the condition: 'then N' or 'goto N'
sub _if_target {
    my ($c,$ln,$t,$p)=@_;
    my $kw=_kw($t,$p);
    die "parse error (line $ln): if without then/goto\n"
        unless defined $kw && ($kw eq 'then' || $kw eq 'goto');
    $$p++;
    my $x=$t->[$$p];
    die "parse error (line $ln): if then/goto needs a line number\n"
        unless $x && $x->{type} eq 'num';
    $$p++; return $x->{val};
}

sub _stmt_on {
    my ($c,$ln,$t,$p)=@_;
    my $expr = MBasic::Expr->parse($t,$p);
    my $kw = _kw($t,$p);
    die "parse error (line $ln): on without goto/gosub\n"
        unless defined $kw && ($kw eq 'goto' || $kw eq 'gosub');
    $$p++;
    my @targets;
    my $x=$t->[$$p]; die "parse error (line $ln): on needs line numbers\n"
        unless $x && $x->{type} eq 'num';
    push @targets, $x->{val}; $$p++;
    while (_is_punct($t,$p,',')) { $$p++;
        my $y=$t->[$$p]; die "parse error (line $ln): bad on target\n"
            unless $y && $y->{type} eq 'num';
        push @targets, $y->{val}; $$p++;
    }
    return { op=>'on', expr=>$expr, kind=>$kw, targets=>\@targets };
}

sub _stmt_for {
    my ($c,$ln,$t,$p)=@_;
    my $x=$t->[$$p]; die "parse error (line $ln): for needs a variable\n"
        unless $x && $x->{type} eq 'ident';
    my $var=$x->{val}; $$p++;
    # a for loop control variable must be numeric (errata 042); a $-suffixed
    # (string) loop variable is rejected.
    die "parse error (line $ln): Numeric variable required (for)\n"
        if substr($var, -1) eq '$';
    my $eq=$t->[$$p]; die "parse error (line $ln): for missing '='\n"
        unless $eq && $eq->{type} eq 'op' && $eq->{val} eq '=';
    $$p++;
    my $from = MBasic::Expr->parse($t,$p);
    my $to_kw = _kw($t,$p);
    die "parse error (line $ln): for missing 'to'\n" unless defined $to_kw && $to_kw eq 'to';
    $$p++;
    my $to = MBasic::Expr->parse($t,$p);
    my $step;
    my $sk = _kw($t,$p);
    if (defined $sk && $sk eq 'step') { $$p++; $step = MBasic::Expr->parse($t,$p); }
    return { op=>'for', var=>$var, from=>$from, to=>$to, step=>$step };
}
sub _stmt_next {
    my ($c,$ln,$t,$p)=@_;
    my $x=$t->[$$p]; die "parse error (line $ln): next needs a variable\n"
        unless $x && $x->{type} eq 'ident';
    $$p++; return { op=>'next', var=>$x->{val} };
}

sub _stmt_input {
    my ($c,$ln,$t,$p)=@_;
    my $chan;
    if (_is_punct($t,$p,'#')) { $$p++; $chan=MBasic::Expr->parse($t,$p); _eat_punct($t,$p,':',$ln); }
    my @vars = ( $c->_parse_lvalue($ln,$t,$p) );
    while (_is_punct($t,$p,',')) {
        $$p++;
        last if _at_end($t,$p);   # trailing comma
        push @vars, $c->_parse_lvalue($ln,$t,$p);
    }
    my $trailing = (defined $t->[$$p-1] && $t->[$$p-1]{type} eq 'punct'
                    && $t->[$$p-1]{val} eq ',') ? 1 : 0;
    return { op=>'input', chan=>$chan, vars=>\@vars, trailing_comma=>$trailing };
}
sub _stmt_linput {
    my ($c,$ln,$t,$p)=@_;
    my $chan;
    if (_is_punct($t,$p,'#')) { $$p++; $chan=MBasic::Expr->parse($t,$p); _eat_punct($t,$p,':',$ln); }
    my $var = $c->_parse_lvalue($ln,$t,$p);
    return { op=>'linput', chan=>$chan, var=>$var };
}

sub _stmt_file {
    my ($c,$ln,$t,$p)=@_;
    _eat_punct($t,$p,'#',$ln);
    my $chan = MBasic::Expr->parse($t,$p);
    _eat_punct($t,$p,':',$ln);
    my $path = MBasic::Expr->parse($t,$p);
    return { op=>'file', chan=>$chan, path=>$path };
}
sub _stmt_scratch {
    my ($c,$ln,$t,$p)=@_;
    _eat_punct($t,$p,'#',$ln);
    my $chan = MBasic::Expr->parse($t,$p);
    return { op=>'scratch', chan=>$chan };
}
sub _stmt_reset {
    my ($c,$ln,$t,$p)=@_;
    _eat_punct($t,$p,'#',$ln);
    my $chan = MBasic::Expr->parse($t,$p);
    return { op=>'reset', chan=>$chan };
}

sub _stmt_dim {
    my ($c,$ln,$t,$p)=@_;
    my @decls;
    while (1) {
        my $x=$t->[$$p]; die "parse error (line $ln): dim needs a name\n"
            unless $x && $x->{type} eq 'ident';
        my $name=$x->{val}; $$p++;
        _eat_punct($t,$p,'(',$ln);
        my @b = ( MBasic::Expr->parse($t,$p) );
        while (_is_punct($t,$p,',')) { $$p++; push @b, MBasic::Expr->parse($t,$p); }
        _eat_punct($t,$p,')',$ln);
        push @decls, { name=>$name, bounds=>\@b };
        last unless _is_punct($t,$p,',');
        $$p++;
    }
    return { op=>'dim', decls=>\@decls };
}

sub _stmt_call {
    my ($c,$ln,$t,$p)=@_;
    my $x=$t->[$$p];
    die "parse error (line $ln): call needs a quoted name\n"
        unless $x && $x->{type} eq 'str';
    my $name=$x->{val}; $$p++;
    # A subroutine/procedure name must be a valid identifier (Multics names
    # cannot contain '>' '<' '/' '.' etc.).  Enforcing the shape here rejects
    # path-traversal-style names at compile time (errata 073).
    die "parse error (line $ln): Invalid subroutine name \"$name\"\n"
        unless $name =~ /^[A-Za-z][A-Za-z0-9_]*(?:\$[A-Za-z][A-Za-z0-9_]*)?\z/;
    my @args;
    if (_is_punct($t,$p,':')) {
        $$p++;
        unless (_at_end($t,$p)) {
            push @args, MBasic::Expr->parse($t,$p);
            while (_is_punct($t,$p,',')) { $$p++; push @args, MBasic::Expr->parse($t,$p); }
        }
    }
    return { op=>'call', name=>$name, args=>\@args };
}

sub _stmt_sub {
    my ($c,$ln,$t,$p)=@_;
    my $x=$t->[$$p];
    die "parse error (line $ln): sub needs a quoted name\n"
        unless $x && $x->{type} eq 'str';
    my $name=$x->{val}; $$p++;
    die "parse error (line $ln): Invalid subroutine name \"$name\"\n"
        unless $name =~ /^[A-Za-z][A-Za-z0-9_]*(?:\$[A-Za-z][A-Za-z0-9_]*)?\z/;
    my @params;
    if (_is_punct($t,$p,':')) {
        $$p++;
        my $first=$t->[$$p];
        if ($first && $first->{type} eq 'ident') {
            push @params, $first->{val}; $$p++;
            while (_is_punct($t,$p,',')) { $$p++;
                my $y=$t->[$$p]; die "parse error (line $ln): bad sub param\n"
                    unless $y && $y->{type} eq 'ident';
                push @params, $y->{val}; $$p++;
            }
        }
    }
    return { op=>'sub', name=>$name, params=>\@params };
}

sub _stmt_data {
    my ($c,$ln,$t,$p)=@_;
    my @vals;
    # data values are literal constants (numbers or strings) separated by commas
    while (!_at_end($t,$p)) {
        my $x=$t->[$$p];
        if    ($x->{type} eq 'num') { push @vals, { num=>$x->{val} }; $$p++; }
        elsif ($x->{type} eq 'str') { push @vals, { str=>$x->{val} }; $$p++; }
        elsif ($x->{type} eq 'op' && $x->{val} eq '-'
               && $t->[$$p+1] && $t->[$$p+1]{type} eq 'num') {
            push @vals, { num=> -$t->[$$p+1]{val} }; $$p+=2;
        }
        else { die "parse error (line $ln): bad data constant\n"; }
        if (_is_punct($t,$p,',')) { $$p++; } else { last; }
    }
    return { op=>'data', values=>\@vals };
}

sub _stmt_read {
    my ($c,$ln,$t,$p)=@_;
    my @vars = ( $c->_parse_lvalue($ln,$t,$p) );
    while (_is_punct($t,$p,',')) { $$p++; push @vars, $c->_parse_lvalue($ln,$t,$p); }
    return { op=>'read', vars=>\@vars };
}

1;

__END__

=head1 NAME

MBasic::Parser - parse Multics BASIC statements into IR records

=head1 DESCRIPTION

Turns the tokens of one source line into an intermediate-representation
statement record: a plain-data hashref with an C<op> key and operand
fields, expression operands being L<MBasic::Expr> nodes.  One handler per
statement keyword covers the complete implemented statement set.  The IR is
plain data (no code references) so it could be serialized.

=head1 METHODS

=head2 parse_line($text)

Class method.  Lex and parse one source line.  Returns C<($line_number,
$ir_record)>.  Dies naming the line on an unknown statement or a syntax error.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
