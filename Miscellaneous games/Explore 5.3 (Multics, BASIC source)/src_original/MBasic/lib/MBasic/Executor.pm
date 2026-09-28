package MBasic::Executor;
use strict;
use warnings;
# BASIC subroutine calls recurse through the Perl call stack; recursion is
# bounded explicitly by $MAX_CALL_DEPTH below, so Perl's cosmetic
# "Deep recursion" warning (which fires at 100 frames) is not wanted.
no warnings 'recursion';
our $VERSION = '1.2';
use MBasic::Expr;
use MBasic::Env;
use MBasic::Arg;
use MBasic::File;

# Largest column a print `tab(e)` will space out to.  A BASIC program could
# otherwise `tab(1e9)` and drive the host into an out-of-memory abort building
# the pad string; past this we raise a loud BASIC error instead.
our $MAX_TAB = 100_000;

# Maximum nesting depth of BASIC `call`s, to bound host stack/recursion (a
# program with unbounded mutual recursion would otherwise abort the process).
our $MAX_CALL_DEPTH = 500;

# ============================================================================
#  MBasic::Executor -- the PC-driven run loop over a Program's IR, against a
#  runtime state (RunState) that bundles the Env + the runtime stacks + the
#  DATA read-pointer + file channels + output sink.
#
#  This increment implements: let, print (to terminal), dim, data/read,
#  goto/if/ifend/ifmore/on, for/next, gosub/return, rem, randomize, stop/end.
#  `call` and file-channel I/O (file/input/linput/print#/scratch/reset) are
#  wired to hooks that the interpreter fills in (registry + File model) in the
#  next increments; here they die "not yet wired" if exercised.
# ============================================================================

# raise a run-time error with an authentic message plus the BASIC line number.
sub _rt_line { my ($msg, $ln) = @_; die "$msg" . (defined $ln ? " (line $ln)" : "") . "\n"; }

# Called from inside the $SIG{__WARN__} handler: was the numeric-coercion
# warning raised by the interpreter's own expression/environment code (a BASIC
# type error) rather than by a native builtin (the embedder's own code)?
#
# It is "ours" iff MBasic::Expr or MBasic::Env appears in the current call
# stack.  Those packages perform arithmetic only while evaluating a BASIC
# expression or assigning to a BASIC variable, so their presence marks a
# BASIC-level type error.  A native builtin doing its own Perl arithmetic runs
# with those frames already returned (the call arguments were evaluated and
# popped before the builtin body executes), so neither package is on the stack
# and the warning is left for the embedder's handler.
sub _warn_from_interp {
    for my $lvl (0 .. 200) {
        my @c = caller($lvl) or last;
        return 1 if $c[0] eq 'MBasic::Expr' || $c[0] eq 'MBasic::Env';
    }
    return 0;
}

# A RunState is the per-program-unit execution context.
sub new_runstate {
    my ($class, %opt) = @_;
    my $prog = $opt{program};
    return {
        program  => $prog,
        env      => $opt{env} // MBasic::Env->new(%opt),
        pc       => 0,
        forstk   => [],           # [ {var,limit,step,top_idx} ... ]
        gosubstk => [],           # [ return_idx ... ]
        dataptr  => 0,            # index into program->{data}
        files    => {},           # chan => file object (later)
        out      => $opt{out} // sub { print $_[0] },   # output sink
        input    => $opt{input},  # optional coderef returning a line (tests)
        interp   => $opt{interp}, # back-ref for call dispatch (later)
        pathxlate=> $opt{pathxlate},
        halt     => 0,
        stopall  => 0,            # set by stop/end: terminate the WHOLE program
        depth    => $opt{depth} // 0,   # BASIC call-nesting depth
        col      => 0,            # current print column (for tab/comma zones)
    };
}

# run_program($program, %opt) -> RunState  (runs to stop/end or fall-off-end)
sub run_program {
    my ($class, $prog, %opt) = @_;
    my $rs = $class->new_runstate(program => $prog, %opt);
    # Convert Perl's numeric-coercion warning (which names interpreter internals
    # and would otherwise leak to stderr while the statement silently proceeds
    # with a wrong value) into an authentic BASIC error at the current line --
    # but ONLY when the coercion happened inside the interpreter's own
    # expression/environment code (MBasic::Expr / MBasic::Env).  A warning
    # raised inside a native builtin (a registered Perl coderef) is the
    # embedder's, not a BASIC type error, so it is passed through unchanged.
    # Any warning we do not convert is delegated to whatever handler the
    # embedder had installed (so their logging is not swallowed), or to the
    # default if they had none.
    my $prev = $SIG{__WARN__};
    local $SIG{__WARN__} = sub {
        my $w = shift;
        if ($w =~ /isn't numeric/ && _warn_from_interp()) {
            _rt_line("Mixed string and numeric expression", $MBasic::Expr::LINE);
        }
        if    (ref $prev eq 'CODE') { $prev->($w); }
        elsif (defined $prev && $prev ne 'DEFAULT' && $prev ne 'IGNORE') {
            # a handler named by string (e.g. 'main::handler')
            no strict 'refs'; &{$prev}($w);
        }
        else { CORE::warn($w); }
    };
    $class->run_loop($rs);
    return $rs;
}

sub run_loop {
    my ($class, $rs) = @_;
    my $ir = $rs->{program}{ir};
    while (!$rs->{halt} && $rs->{pc} <= $#$ir) {
        my $stmt = $ir->[$rs->{pc}];
        my $jumped = $class->exec_stmt($rs, $stmt);
        $rs->{pc}++ unless $jumped;
    }
    # flush/close any open file channels (commit buffered writes to disk)
    for my $chan (keys %{$rs->{files}}) {
        $rs->{files}{$chan}->close if $rs->{files}{$chan};
    }
    return;
}

# exec_stmt: returns true if it set the PC (a jump), false to advance normally.
sub exec_stmt {
    my ($class, $rs, $s) = @_;
    my $op = $s->{op};
    my $env = $rs->{env};

    # make the current BASIC line available to run-time errors raised inside
    # expression evaluation (see MBasic::Expr::_rt).
    local $MBasic::Expr::LINE = $s->{line};

    if ($op eq 'rem' || $op eq 'data' || $op eq 'sub') { return 0; }  # no-ops here
    if ($op eq 'randomize') { $env->randomize_seed; return 0; }
    # `stop` and `end` terminate the WHOLE program (stopall bubbles up through
    # any enclosing subroutine calls); `subend` only ends the current sub.
    if ($op eq 'stop' || $op eq 'end') { $rs->{halt}=1; $rs->{stopall}=1; return 1; }
    if ($op eq 'subend') { $rs->{halt}=1; return 1; }

    if ($op eq 'let') {
        my $v = MBasic::Expr->eval($s->{expr}, $env);
        $class->_assign($rs, $s->{lhs}, $v);
        return 0;
    }

    if ($op eq 'dim') {
        for my $d (@{$s->{decls}}) {
            my @b = map { int(MBasic::Expr->eval($_, $env)) } @{$d->{bounds}};
            $env->declare_array($d->{name}, \@b);
        }
        return 0;
    }

    if ($op eq 'goto')  { $rs->{pc} = $s->{tidx}; return 1; }

    if ($op eq 'if') {
        my $c = MBasic::Expr->eval($s->{cond}, $env);
        if ($c) { $rs->{pc} = $s->{tidx}; return 1; }
        return 0;
    }
    if ($op eq 'ifend' || $op eq 'ifmore') {
        my $chan = int(MBasic::Expr->eval($s->{chan}, $env));
        my $atend = $class->_file_at_end($rs, $chan);
        my $take = ($op eq 'ifend') ? $atend : !$atend;
        if ($take) { $rs->{pc} = $s->{tidx}; return 1; }
        return 0;
    }
    if ($op eq 'on') {
        my $n = int(MBasic::Expr->eval($s->{expr}, $env));   # 1-based selector
        my $tidx = $s->{tidx};
        if ($n >= 1 && $n <= @$tidx) {
            my $target = $tidx->[$n-1];
            if ($s->{kind} eq 'gosub') { push @{$rs->{gosubstk}}, $rs->{pc}+1; }
            $rs->{pc} = $target; return 1;
        }
        # out of range is an error on Multics (errata 1.2), NOT a fall-through
        # (verified live: "On evaluated out of range").
        die "On evaluated out of range (line $s->{line})\n";
    }

    if ($op eq 'gosub') { push @{$rs->{gosubstk}}, $rs->{pc}+1; $rs->{pc}=$s->{tidx}; return 1; }
    if ($op eq 'return') {
        die "Return before gosub (line $s->{line})\n"
            unless @{$rs->{gosubstk}};
        $rs->{pc} = pop @{$rs->{gosubstk}}; return 1;
    }

    if ($op eq 'for') {
        my $from = MBasic::Expr->eval($s->{from}, $env);
        my $to   = MBasic::Expr->eval($s->{to}, $env);
        my $step = defined $s->{step} ? MBasic::Expr->eval($s->{step}, $env) : 1;
        $env->set_scalar($s->{var}, $from);
        # zero-trip loop: if the initial value already passes the limit, skip
        # the body WITHOUT pushing a frame (otherwise the stale frame would sit
        # on the stack and break the enclosing loop's `next`).
        if (($step >= 0 && $from > $to) || ($step < 0 && $from < $to)) {
            $class->_skip_to_after_next($rs, $s->{var});
            return 1;
        }
        # otherwise push a frame; the loop body runs starting at pc+1
        push @{$rs->{forstk}}, { var=>$s->{var}, limit=>$to, step=>$step,
                                 top=>$rs->{pc}+1 };
        return 0;
    }
    if ($op eq 'next') {
        my $stk = $rs->{forstk};
        die "Next without for (line $s->{line})\n" unless @$stk;
        # Find the nearest frame for this variable.  A jump OUT of an inner
        # for-loop (goto / if-then-line) leaves that inner frame on the stack;
        # when control later reaches an ENCLOSING loop's `next`, Multics BASIC
        # closes those abandoned inner loops rather than faulting.  (The
        # compile-time "For-next mismatch", errata 044, checks static nesting;
        # at run time an outer `next` discards still-open inner frames.)  So we
        # search down for the matching variable and drop any frames above it.
        my $idx = -1;
        for (my $k = $#$stk; $k >= 0; $k--) {
            if ($stk->[$k]{var} eq $s->{var}) { $idx = $k; last; }
        }
        die "For-next mismatch (line $s->{line})\n" if $idx < 0;
        splice(@$stk, $idx + 1) if $idx < $#$stk;   # discard abandoned inner loops
        my $fr = $stk->[-1];
        my $cur = $env->get_scalar($fr->{var}) + $fr->{step};
        $env->set_scalar($fr->{var}, $cur);
        if (($fr->{step} >= 0 && $cur <= $fr->{limit})
         || ($fr->{step} <  0 && $cur >= $fr->{limit})) {
            $rs->{pc} = $fr->{top}; return 1;      # loop again
        }
        pop @$stk;                                  # this loop done
        return 0;
    }

    if ($op eq 'read') {
        for my $lv (@{$s->{vars}}) {
            my $pool = $rs->{program}{data};
            die "Out of data (line $s->{line})\n"
                if $rs->{dataptr} > $#$pool;
            my $d = $pool->[ $rs->{dataptr}++ ];
            my $v = exists $d->{num} ? $d->{num} : $d->{str};
            $class->_assign($rs, $lv, $v);
        }
        return 0;
    }

    if ($op eq 'print') { $class->_do_print($rs, $s); return 0; }

    # ---- terminal input/linput (no channel) ----
    if (($op eq 'input' || $op eq 'linput') && !defined $s->{chan}) {
        return $class->_do_terminal_input($rs, $s);
    }

    # ---- wired-but-deferred: call and file I/O ----
    if ($op eq 'call')   { return $class->_do_call($rs, $s); }
    if ($op eq 'file' || $op eq 'input' || $op eq 'linput'
        || $op eq 'scratch' || $op eq 'reset') {
        return $class->_do_file_op($rs, $s);
    }

    die "internal: unhandled op '$op' at line $s->{line}\n";
}

# ---- assignment to an lvalue (scalar or array element) ----
sub _assign {
    my ($class, $rs, $lv, $value) = @_;
    my $env = $rs->{env};
    if ($lv->{subs}) {
        my @subs = map { MBasic::Expr->eval($_, $env) } @{$lv->{subs}};
        $env->set_array($lv->{name}, \@subs, $value);
    } else {
        $env->set_scalar($lv->{name}, $value);
    }
}

# ---- print (terminal) ----
#  Numbers print with a LEADING sign-blank and a TRAILING blank (field
#  separator) -- distinct from str$ which has no trailing blank.  ';' = no
#  extra space; ',' = advance to next 15-col zone; tab(e) = go to column e.
sub _do_print {
    my ($class, $rs, $s) = @_;
    my $env = $rs->{env};
    if (defined $s->{chan}) {
        my $chan = int(MBasic::Expr->eval($s->{chan}, $env));
        return $class->_do_print_file($rs, $s, $chan);
    }
    my $emit = sub { my $txt=shift; $rs->{out}->($txt);
                     # track column, resetting on newline
                     if ($txt =~ /\n([^\n]*)$/) { $rs->{col} = length $1; }
                     else { $rs->{col} += length $txt; } };
    my $items = $s->{items};
    my $printed_trailing_sep = 0;
    for my $it (@$items) {
        if ($it->{kind} eq 'sep') {
            if ($it->{sep} eq ',') {
                my $zone = 15;
                my $target = ((int($rs->{col}/$zone))+1)*$zone;
                $emit->(' ' x ($target - $rs->{col}));
            }
            # ';' : no spacing
            $printed_trailing_sep = 1;
            next;
        }
        $printed_trailing_sep = 0;
        if ($it->{kind} eq 'tab') {
            my $target = int(MBasic::Expr->eval($it->{node}, $env));
            _rt_line("Invalid margin", $s->{line}) if $target > $MAX_TAB;
            if ($target > $rs->{col}) { $emit->(' ' x ($target - $rs->{col})); }
            next;
        }
        # expr item
        my $node = $it->{node};
        my $v = MBasic::Expr->eval($node, $env);
        my $txt = MBasic::Expr::_is_string_expr($node) ? $v : _print_number($v);
        $emit->($txt);
    }
    # newline unless the list ended with a separator (; or ,)
    $emit->("\n") unless $printed_trailing_sep;
}

sub _do_print_file {
    my ($class, $rs, $s, $chan) = @_;
    my $env = $rs->{env};
    my $f = $class->_chan($rs, $chan, $s->{line});
    my $line = q{}; my $trailing_sep = 0;
    for my $it (@{$s->{items}}) {
        if ($it->{kind} eq q{sep}) {
            $trailing_sep = 1;
            if ($it->{sep} eq q{,}) { my $z=15; my $t=((int(length($line)/$z))+1)*$z; $line .= q{ } x ($t-length($line)); }
            next;
        }
        $trailing_sep = 0;
        if ($it->{kind} eq q{tab}) { my $t=int(MBasic::Expr->eval($it->{node},$env));
            _rt_line("Invalid margin", $s->{line}) if $t > $MAX_TAB;
            $line .= q{ } x ($t-length($line)) if $t>length($line); next; }
        my $node = $it->{node};
        my $v = MBasic::Expr->eval($node, $env);
        $line .= MBasic::Expr::_is_string_expr($node) ? $v : _print_number($v);
    }
    $f->print_chunk($line, $trailing_sep ? 0 : 1);
    return;
}

# number formatting for PRINT: leading sign-blank + digits + TRAILING blank.
sub _print_number {
    my ($x) = @_;
    my $sign = $x < 0 ? '-' : ' ';
    my $mag = abs($x);
    my $body = ($mag == int($mag) && $mag < 134_217_728)
             ? sprintf('%d', $mag) : MBasic::Expr::_g6($mag);
    return $sign . $body . ' ';
}

# ---- call dispatch ----
#  Builds argument adapters (honoring Appendix B write-back for lvalue args),
#  resolves the name via the interpreter, and dispatches:
#    * builtin -> Perl coderef($ctx, \@args)
#    * BASIC sub -> fresh RunState/Env for the sub's program unit; bind params
#      by reference to the caller's arg lvalues; run entry..subend; string
#      out-params are written back through the adapters.
sub _do_call {
    my ($class, $rs, $s) = @_;
    my $interp = $rs->{interp}
        or die "runtime error (line $s->{line}): no interpreter for call \"$s->{name}\"\n";

    # build argument adapters from the call-argument expr-nodes
    my @args = map { $class->_make_arg($rs, $_) } @{$s->{args}};

    my ($kind, @info) = $interp->resolve_call($s->{name});

    if ($kind eq 'undef') {
        die "Attempt to transfer to missing function \"$s->{name}\" (line $s->{line})\n";
    }

    if ($kind eq 'builtin') {
        my $code = $info[0];
        my $ctx = { rs => $rs, interp => $interp, out => $rs->{out} };
        $code->($ctx, \@args);
        return 0;
    }

    # kind eq 'sub': run a BASIC subroutine as its own program unit
    my ($subprog, $entryinfo) = @info;
    $class->_call_basic_sub($rs, $subprog, $entryinfo, \@args, $s->{line});
    # a `stop`/`end` reached inside the sub terminates the whole program.
    return $rs->{halt} ? 1 : 0;
}

# build an Arg adapter from a call-argument expr-node.
#   bare scalar var  -> writable lvalue adapter
#   array element    -> writable lvalue adapter (subs evaluated now)
#   anything else    -> read-only value adapter
sub _make_arg {
    my ($class, $rs, $node) = @_;
    my $env = $rs->{env};
    if ($node->{k} eq 'var') {
        return MBasic::Arg->new_lvalue(env => $env, name => $node->{name});
    }
    if ($node->{k} eq 'idx') {
        my @subs = map { MBasic::Expr->eval($_, $env) } @{$node->{args}};
        return MBasic::Arg->new_lvalue(env => $env, name => $node->{name}, subs => \@subs);
    }
    return MBasic::Arg->new_readonly( MBasic::Expr->eval($node, $env) );
}

# run a BASIC sub in a FRESH environment (own vars/files/data-ptr/RNG),
# binding its parameters by reference to the caller's argument adapters, then
# copying string out-params back at subend (Appendix B).
sub _call_basic_sub {
    my ($class, $caller_rs, $subprog, $entryinfo, $args, $callline) = @_;
    my $params = $entryinfo->{params};
    die "Subroutine called with wrong number of parameters (line $callline)\n"
        if @$args != @$params;

    # bound recursion depth so runaway/mutual recursion raises a loud BASIC
    # error instead of aborting the host process with a stack overflow.
    my $depth = $caller_rs->{depth} + 1;
    die "Stack space exhausted, subroutine/function calls beyond maximum depth"
      . " (line $callline)\n"
        if $depth > $MAX_CALL_DEPTH;

    # fresh environment for the sub, sharing the run context (argv/user) but
    # its OWN variables/arrays/data-pointer/files.  The sub's dat$/clk$/usr$
    # specials come from the same run context, and it draws from the SAME
    # pseudo-random stream (shared by reference) so the program's RNG sequence
    # is one repeatable stream across all its units.
    my $sub_env = MBasic::Env->new(
        argv => $caller_rs->{env}{argv},
        user => $caller_rs->{env}{user},
        now  => $caller_rs->{env}{_now},
        rng  => $caller_rs->{env}{rng},
    );

    # bind parameters: copy the argument's current value INTO the sub's param
    # variable (copy-in).  For string params this is the "copied in at entry"
    # rule; for numeric, pass-by-value-in.  Write-back happens at subend.
    my @entry;
    for my $i (0 .. $#$params) {
        my $pname = $params->[$i];
        my $val   = $args->[$i]->get;
        $sub_env->set_scalar($pname, $val);
	$entry[$i] = $sub_env->get_scalar($pname);  # post-coercion
    }

    # run the sub's program unit from its entry index to subend
    my $sub_rs = $class->new_runstate(
        program => $subprog,
        env     => $sub_env,
        interp  => $caller_rs->{interp},
        out     => $caller_rs->{out},
        input   => $caller_rs->{input},
        pathxlate => $caller_rs->{pathxlate},
        depth   => $depth,
    );
    $sub_rs->{pc} = $entryinfo->{entry} + 1;   # start after the `sub` statement
    # run until subend (halt) or fall-through
    {
        my $ir = $subprog->{ir};
        while (!$sub_rs->{halt} && $sub_rs->{pc} <= $#$ir) {
            my $st = $ir->[$sub_rs->{pc}];
            last if $st->{op} eq 'sub';   # reached the next sub def -> stop
            my $jumped = $class->exec_stmt($sub_rs, $st);
            $sub_rs->{pc}++ unless $jumped;
        }
    }

    # commit/close the sub's own file channels (the sub has its own {files};
    # without this a sub's writes would rely solely on eager flushing).
    for my $chan (keys %{$sub_rs->{files}}) {
        $sub_rs->{files}{$chan}->close if $sub_rs->{files}{$chan};
    }

    # a `stop`/`end` inside the sub terminates the whole program: bubble it up
    # to the caller so the caller's run loop halts too.
    if ($sub_rs->{stopall}) { $caller_rs->{halt} = 1; $caller_rs->{stopall} = 1; }

    # copy back: write each parameter's final value out to the caller's arg
    # adapter (write-back).  Per Appendix B, strings are written back; we write
    # back all params (numeric write-back to a value arg is a harmless no-op).
    for my $i (0 .. $#$params) {
        my $final = $sub_env->get_scalar($params->[$i]);
	next if $final eq $entry[$i];
        $args->[$i]->set($final);
    }
    return;
}
# ---- file channel operations ----
sub _do_file_op {
    my ($class, $rs, $s) = @_;
    my $env = $rs->{env};
    my $op = $s->{op};

    if ($op eq 'file') {
        my $chan = int(MBasic::Expr->eval($s->{chan}, $env));
        _rt_line("Invalid file number", $s->{line}) if $chan < 1 || $chan > 4;
        my $path = MBasic::Expr->eval($s->{path}, $env);
        # apply the optional path-translation hook (the Explore layer maps
        # Multics '>a>b' pathnames to Unix paths; the generic core stays
        # path-agnostic and just uses whatever the hook returns).
        $path = $rs->{pathxlate}->($path) if $rs->{pathxlate};
        if (my $old = $rs->{files}{$chan}) { $old->close; }
        $rs->{files}{$chan} = MBasic::File->open_path($path);
        return 0;
    }

    my $chan = int(MBasic::Expr->eval($s->{chan}, $env));

    if ($op eq 'scratch') { $class->_chan($rs,$chan,$s->{line})->scratch; return 0; }
    if ($op eq 'reset')   { $class->_chan($rs,$chan,$s->{line})->reset_pointer; return 0; }
    if ($op eq 'input') {
        my $f = $class->_chan($rs, $chan, $s->{line});
        for my $lv (@{$s->{vars}}) {
            my $field = $f->input_field;
            my $is_str = substr($lv->{name}, -1) eq '$';
            my $val = $is_str ? $field : MBasic::Expr::_to_number($field);
            $class->_assign($rs, $lv, $val);
        }
        return 0;
    }
    if ($op eq 'linput') {
        my $f = $class->_chan($rs, $chan, $s->{line});
        $class->_assign($rs, $s->{var}, $f->linput);
        return 0;
    }
    die "internal: unhandled file op '$op'\n";
}

sub _chan {
    my ($class, $rs, $chan, $ln) = @_;
    my $f = $rs->{files}{$chan};
    die "Invalid file number #$chan (not open, line $ln)\n" unless $f;
    return $f;
}

# if end #n / if more #n : an unopened/empty/absent file reads as at-end
# (verified: `file` on a nonexistent file makes `if end` true, no error).
sub _file_at_end {
    my ($class, $rs, $chan) = @_;
    my $f = $rs->{files}{$chan};
    return 1 unless $f;
    return $f->at_end ? 1 : 0;
}

# ---- terminal input/linput (no channel): read from the input source ----
#  The input source is $rs->{input} (a coderef returning a line, for tests) or
#  STDIN.  `linput` assigns the whole line to one string var; `input` splits
#  the line on commas into the (possibly multiple) variables, coercing numeric
#  targets.  The prompt "? " is printed before reading (BASIC input prompt).
sub _do_terminal_input {
    my ($class, $rs, $s) = @_;
    my $getline = $rs->{input} // sub {
        my $l = <STDIN>;
        return undef unless defined $l;
        chomp $l; return $l;
    };
    if ($s->{op} eq 'linput') {
        # Terminal linput DOES print the "? " prompt (manual: "Each time a
        # string value is required, a prompt is printed").
        $rs->{out}->("? ");
        my $line = $getline->();
        $line = '' unless defined $line;
        chomp $line if defined $line;
        $class->_assign($rs, $s->{var}, $line);
        return 0;
    }
    # input: print the prompt, read a line, split on commas.  If the line
    # supplies fewer values than there are variables, Multics prints "Not
    # enough input, add more" and reads more (errata 107) rather than silently
    # defaulting the missing variables to 0/"".
    my $nvars = scalar @{$s->{vars}};
    $rs->{out}->("? ");
    my $line = $getline->();
    _rt_line("Not enough input, add more", $s->{line}) unless defined $line;
    chomp $line;
    my @fields = split /,/, $line, -1;
    while (@fields < $nvars) {
        $rs->{out}->("Not enough input, add more\n? ");
        my $more = $getline->();
        _rt_line("Not enough input, add more", $s->{line}) unless defined $more;
        chomp $more;
        push @fields, split /,/, $more, -1;
    }
    for my $i (0 .. $nvars-1) {
        my $lv = $s->{vars}[$i];
        my $raw = defined $fields[$i] ? $fields[$i] : '';
        $raw =~ s/^\s+//; $raw =~ s/\s+$//;
        my $is_str = substr($lv->{name}, -1) eq '$';
        my $val = $is_str ? $raw : MBasic::Expr::_to_number($raw);
        $class->_assign($rs, $lv, $val);
    }
    return 0;
}

# ---- skip a for-loop whose body should not run (initial value past limit) ----
sub _skip_to_after_next {
    my ($class, $rs, $var) = @_;
    my $ir = $rs->{program}{ir};
    my $depth = 0;
    my $i = $rs->{pc} + 1;
    while ($i <= $#$ir) {
        my $o = $ir->[$i];
        if ($o->{op} eq 'for') { $depth++; }
        elsif ($o->{op} eq 'next') {
            if ($depth == 0 && $o->{var} eq $var) { $rs->{pc} = $i + 1; return; }
            $depth-- if $depth > 0;
        }
        $i++;
    }
    die "For without next ($var)\n";
}

1;

__END__

=head1 NAME

MBasic::Executor - the Multics BASIC run loop

=head1 DESCRIPTION

The program-counter-driven execution engine.  Runs a L<MBasic::Program>
against a run-state that bundles an L<MBasic::Env>, the C<for> and C<gosub>
stacks, the C<data> read pointer, the open file channels, and the I/O sinks.
Implements all executable statements, the C<call> dispatch (to native
builtins or BASIC subroutines), terminal and file I/O, and an optional
path-translation hook.

=head1 METHODS

=head2 run_program($program, %opt)

Class method.  Runs a program to completion (C<stop>/C<end> or fall-off-end)
and returns the run-state.  Options include C<env>, C<interp> (for C<call>
resolution), C<out> (an output coderef), C<input> (a line-source coderef), and
C<pathxlate> (a coderef translating file pathnames before they are opened).

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
