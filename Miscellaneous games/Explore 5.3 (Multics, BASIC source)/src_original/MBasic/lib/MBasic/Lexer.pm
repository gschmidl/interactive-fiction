package MBasic::Lexer;
use strict;
use warnings;
our $VERSION = '1.2';

# ============================================================================
#  MBasic::Lexer -- tokenize one source line of Multics BASIC (the Explore
#  subset) into a line number and a list of tokens.
#
#  Token: a hashref { type => ..., val => ... }
#    type is one of:
#      'num'    numeric literal            val => the Perl number
#      'str'    string literal             val => the (unescaped) string
#      'ident'  identifier / keyword       val => the lexeme (e.g. 'let','a$',
#                                                 'sst$','x1','goto')
#      'op'     operator                   val => one of + - * / & = <> < > <= >=
#      'punct'  punctuation                val => one of ( ) , ; : #
#    (Keywords are not distinguished from identifiers here; the parser decides
#     based on position -- the first ident on a line is the statement keyword.)
#
#  Notes on Multics BASIC lexical rules used by Explore:
#    * Identifiers: a letter, optionally followed by a digit, optionally
#      followed by '$'  (a, a$, x1, p$, h9$).  Built-in function/keyword names
#      are longer lowercase words (let, print, sst$, linput, randomize, ...);
#      we lex any run of [a-z0-9_]+ optionally followed by '$' as one ident and
#      let the parser classify it.  (Underscore appears in called names like
#      exp_home_, but those only occur inside "..." string literals, never as
#      bare idents -- still, we allow '_' in idents defensively.)
#    * String literals are double-quoted; an embedded quote is written "" .
#    * Inline comment: an apostrophe (') not inside a string begins a comment
#      that runs to end of line (a Multics BASIC convention Explore uses).
#    * The 'rem' statement takes the rest of the line as free text.
#    * Numbers: integer or fractional (no exponent seen in Explore, but we
#      accept a trailing E+/-digits scientific form defensively).
# ============================================================================

# tokenize_line($text) -> ($line_number, \@tokens)
#   $text is one raw source line (no trailing newline required).
#   Dies with a clear message on a malformed line (e.g. no line number,
#   unterminated string).
sub tokenize_line {
    my ($class, $text) = @_;
    my $orig = $text;

    # ---- line number (required, leading) ----
    $text =~ s/^\s+//;
    unless ($text =~ s/^(\d+)\s*//) {
        die "lex error: line does not begin with a line number: <<$orig>>\n";
    }
    my $lineno = $1 + 0;

    # ---- special case: rem  -> rest of line is a single free-text comment ----
    #   We still return it as a token so the parser can make a no-op; the text
    #   is preserved for round-tripping/debugging.
    if ($text =~ /^rem\b(.*)$/s) {
        my $rest = defined $1 ? $1 : '';
        return ($lineno, [ { type => 'ident', val => 'rem' },
                           { type => 'remtext', val => $rest } ]);
    }

    my @tok;
    my $n = length $text;
    my $i = 0;

    while ($i < $n) {
        my $c = substr($text, $i, 1);

        # whitespace
        if ($c =~ /\s/) { $i++; next; }

        # inline comment: ' to end of line (not inside a string; we are not in
        # one here since strings are consumed atomically below)
        if ($c eq "'") { last; }

        # string literal
        if ($c eq '"') {
            my ($str, $adv) = _lex_string($text, $i, $orig);
            push @tok, { type => 'str', val => $str };
            $i += $adv;
            next;
        }

        # number (leading digit, or leading '.' followed by digit)
        if ($c =~ /[0-9]/ or ($c eq '.' and substr($text, $i+1, 1) =~ /[0-9]/)) {
            my ($num, $adv) = _lex_number($text, $i);
            push @tok, { type => 'num', val => $num };
            $i += $adv;
            next;
        }

        # identifier / keyword: [A-Za-z][A-Za-z0-9_]* optionally '$'
        if ($c =~ /[A-Za-z]/) {
            my ($id, $adv) = _lex_ident($text, $i);
            push @tok, { type => 'ident', val => $id };
            $i += $adv;
            next;
        }

        # two-char operators first
        my $two = substr($text, $i, 2);
        if ($two eq '<=' or $two eq '>=' or $two eq '<>') {
            push @tok, { type => 'op', val => $two };
            $i += 2; next;
        }

        # single-char operators
        if (index('+-*/&=<>^', $c) >= 0) {
            push @tok, { type => 'op', val => $c };
            $i++; next;
        }

        # punctuation
        if (index('(),;:#', $c) >= 0) {
            push @tok, { type => 'punct', val => $c };
            $i++; next;
        }

        die "lex error (line $lineno): unexpected character '$c' in <<$orig>>\n";
    }

    return ($lineno, \@tok);
}

# ---- helpers ---------------------------------------------------------------

sub _lex_string {
    my ($s, $i, $orig) = @_;
    # $i points at the opening quote.
    my $n = length $s;
    my $j = $i + 1;
    my $out = '';
    while ($j < $n) {
        my $c = substr($s, $j, 1);
        if ($c eq '"') {
            # "" -> embedded quote; else end of string
            if (substr($s, $j+1, 1) eq '"') { $out .= '"'; $j += 2; next; }
            return ($out, $j - $i + 1);   # +1 to consume the closing quote
        }
        $out .= $c;
        $j++;
    }
    die "lex error: unterminated string literal in <<$orig>>\n";
}

sub _lex_number {
    my ($s, $i) = @_;
    my $rest = substr($s, $i);
    # integer / fractional / optional scientific (defensive)
    if ($rest =~ /^(\d+\.\d+|\.\d+|\d+)([eE][+-]?\d+)?/) {
        my $lit = $&;
        return ($lit + 0, length $lit);
    }
    die "lex error: bad number at <<" . substr($s, $i, 12) . "...>>\n";
}

sub _lex_ident {
    my ($s, $i) = @_;
    my $rest = substr($s, $i);
    # letter, then letters/digits/underscore, then an optional trailing '$'.
    #
    # VERIFIED on Multics: BASIC permits whitespace between the identifier and
    # its '$' suffix -- "f $" is the SAME string variable as "f$".  (A live
    # test of  print l;",";f $  read f$ correctly.)  This is a genuine dialect
    # quirk -- most BASICs reject it -- so the lexer folds the '$' in and
    # normalizes "f $" -> "f$".
    $rest =~ /^([A-Za-z][A-Za-z0-9_]*)([ \t]*\$)?/;
    my $base = $1;
    my $dollar = $2;                       # e.g. "  $" (with leading spaces) or undef
    my $consumed = length($base) + (defined $dollar ? length($dollar) : 0);
    my $id = $base . (defined $dollar ? '$' : '');   # normalized (no spaces)
    return ($id, $consumed);
}

1;

__END__

=head1 NAME

MBasic::Lexer - tokenize a line of Multics BASIC

=head1 DESCRIPTION

Tokenizes one raw source line into a line number and an arrayref of
token hashrefs.  A token is C<< { type => ..., val => ... } >> where C<type>
is one of C<num>, C<str>, C<ident>, C<op>, C<punct>, or C<remtext>.  Keyword
vs. identifier is not distinguished here; the parser decides by position.

Handles Multics BASIC lexical quirks: doubled-quote (C<"">) string escapes,
the apostrophe inline comment, the C<rem> free-text statement, and a C<$>
string-suffix that may be separated from its identifier by whitespace (e.g.
C<"f $"> is the same variable as C<"f$"> -- verified on Multics).

=head1 METHODS

=head2 tokenize_line($text)

Class method.  Returns C<($line_number, \@tokens)>.  Dies with a message that
includes the offending text on a malformed line (missing line number,
unterminated string, unexpected character).

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
