use strict; use warnings;
use FindBin; use lib "$FindBin::Bin/../lib";
use Test::More;
use File::Temp qw(tempdir);
use MBasic::Interp;

# load_all_helpers: eagerly validate every helper up front so a load-time error
# surfaces at startup, not the first time a helper is lazily called.

# --- a directory of clean helpers loads and indexes every sub ---
{
    my $dir = tempdir(CLEANUP => 1);
    _write("$dir/exp_a.basic", '10 sub "aa"', '20 subend');
    _write("$dir/exp_b.basic", '10 sub "bb"', '20 subend');
    _write("$dir/explore.basic", '10 print "main"', '20 end');   # the "main"

    my $i = MBasic::Interp->new(search_path => [ $dir ]);
    $i->load_main("$dir/explore.basic");
    my $n = $i->load_all_helpers($dir);
    is($n, 2, 'load_all_helpers loads the two helpers (skips the already-loaded main)');
    my ($ka) = $i->resolve_call('aa');
    my ($kb) = $i->resolve_call('bb');
    is($ka, 'sub', 'sub "aa" indexed');
    is($kb, 'sub', 'sub "bb" indexed');
}

# --- a helper with a load-time error fails AT STARTUP (load_all_helpers dies) ---
{
    my $dir = tempdir(CLEANUP => 1);
    _write("$dir/exp_ok.basic",  '10 sub "ok"', '20 subend');
    _write("$dir/exp_bad.basic", '10 sub "bad"', '20 goto 30 junk here', '40 subend');

    my $i = MBasic::Interp->new(search_path => [ $dir ]);
    my $err = '';
    eval { $i->load_all_helpers($dir); 1 } or $err = $@;
    like($err, qr/extra tokens after statement/,
         'a broken helper is rejected at load_all_helpers time (not lazily mid-run)');
}

# --- called with no args, it uses the configured search path ---
{
    my $dir = tempdir(CLEANUP => 1);
    _write("$dir/exp_c.basic", '10 sub "cc"', '20 subend');
    my $i = MBasic::Interp->new(search_path => [ $dir ]);
    is($i->load_all_helpers, 1, 'load_all_helpers() defaults to the search path');
    my ($k) = $i->resolve_call('cc');
    is($k, 'sub', 'sub "cc" indexed from the search path');
}

sub _write {
    my ($path, @lines) = @_;
    open my $fh, '>', $path or die "write $path: $!";
    print $fh "$_\n" for @lines;
    close $fh;
}

done_testing;
