# mkruntime.pl OUTDIR
#
# Make the Perl that ships with the port, in OUTDIR (port\perl): perl.exe, its
# DLLs, and the modules the game loads, copied from the Strawberry Perl that
# runs this script.  The module list is measured, not written down: the game is
# played twice under this Perl (src\deps.pl records what it loads) - once
# through most of its commands up to a SAVE, once from the RESTORE to QUIT.
#
# Run it with a native Windows Perl (the shipped one is Strawberry Perl
# 5.32.1.1, 64-bit), not with the MSYS Perl of Git Bash.
use strict;
use warnings;
use Config;
use File::Spec;
use File::Basename qw(dirname);
use File::Path qw(make_path remove_tree);
use File::Copy qw(copy);
use File::Temp qw(tempdir);

die "mkruntime.pl: run this with a Windows Perl such as Strawberry Perl "
  . "(this one is built for $^O)\n" unless $^O eq 'MSWin32';
my $out = shift or die "usage: perl mkruntime.pl OUTDIR\n";
$out = File::Spec->rel2abs($out);

my $src  = dirname(File::Spec->rel2abs(__FILE__));    # port\src
my $port = dirname($src);                               # port
my $bin  = dirname($^X);                                # the Perl's bin
my $top  = dirname(dirname($bin));                      # Strawberry's top

sub slash { my $p = shift; $p =~ tr{\\}{/}; $p }

# --- 1. what the game loads ---------------------------------------------------
my @sessions = (
    # most commands, then SAVE (which ends the game)
    join("\n", qw(help), 'help moving', qw(commands news info hours modes in get
         what on d look brief full score turns ab), '.a zz what', qw(zz .l),
         '.d zz', qw(.q sorcerer nothing), 'stm fast', 'save rt') . "\n",
    # RESTORE, then QUIT
    join("\n", 'restore rt', qw(score quit yes)) . "\n",
);
my $tmp = tempdir(CLEANUP => 1);
my (%inc, %dll);
for my $i (0 .. $#sessions) {
    my $in = "$tmp/in$i.txt";
    open my $fh, '>', $in or die "mkruntime.pl: $in: $!\n";
    print $fh $sessions[$i];
    close $fh;
    my $list = "$tmp/list$i.txt";
    open my $savein,  '<&', \*STDIN  or die;
    open my $saveout, '>&', \*STDOUT or die;
    open STDIN,  '<', $in              or die;
    open STDOUT, '>', "$tmp/out$i.txt" or die;
    my $rc = system($^X, "$src/deps.pl", $list, "$port/explore.pl",
                    '--var', "$tmp/saves");
    open STDIN,  '<&', $savein  or die;
    open STDOUT, '>&', $saveout or die;
    die "mkruntime.pl: the game failed in session $i (status $rc); see "
      . "$tmp/out$i.txt\n" if $rc != 0 || !-s $list;
    open $fh, '<', $list or die;
    while (<$fh>) {
        chomp;
        my @f = split /\t/;
        if ($f[0] eq 'inc') { $inc{$f[1]} = slash($f[2]) }
        else                { $dll{slash($f[1])} = 1 }
    }
    close $fh;
}
# the session must have reached its end
open my $fh, '<', "$tmp/out1.txt" or die;
my $text = do { local $/; <$fh> };
close $fh;
die "mkruntime.pl: the test sessions did not run through\n"
    unless $text =~ /You scored/;

# --- 2. copy ------------------------------------------------------------------
remove_tree($out);
make_path("$out/bin", "$out/lib");
my $n = 0;
sub put {
    my ($from, $to) = @_;
    make_path(dirname($to));
    copy($from, $to) or die "mkruntime.pl: cannot copy $from to $to: $!\n";
    $n++;
}

# perl.exe, the Perl DLL and the gcc runtime it needs (all msvcrt builds)
opendir my $bd, $bin or die "mkruntime.pl: $bin: $!\n";
my ($perldll) = grep { /^perl5\d+\.dll\z/i } readdir $bd;
closedir $bd;
die "mkruntime.pl: no perl5NN.dll in $bin\n" unless $perldll;
for my $f ('perl.exe', $perldll, qw(libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)) {
    die "mkruntime.pl: $bin/$f is missing\n" unless -f "$bin/$f";
    put("$bin/$f", "$out/bin/$f");
}

# the modules, all into lib (the port's own MBasic and Explore stay in
# port\lib, and explore.pl in port)
my $own = lc(slash("$port/"));
my $modules = 0;
for my $key (sort keys %inc) {
    my $path = $inc{$key};
    next if lc(substr($path, 0, length $own)) eq $own;
    put($path, "$out/lib/$key");
    $modules++;
}
# Config.pm reads the rest of its table from these when it is asked for it
if ($inc{'Config.pm'}) {
    my $dir = dirname($inc{'Config.pm'});
    for my $f (qw(Config_heavy.pl Config_git.pl)) {
        put("$dir/$f", "$out/lib/$f") if -f "$dir/$f" && !$inc{$f};
    }
}
# the XS DLLs, with everything else in their auto\ folders
for my $dll (sort keys %dll) {
    my ($rel) = $dll =~ m{/(auto/.+)/[^/]+$} or die "mkruntime.pl: odd DLL path $dll\n";
    my $dir = dirname($dll);
    opendir my $d, $dir or die;
    for my $f (grep { !/^\./ && -f "$dir/$_" } readdir $d) {   # not .packlist
        put("$dir/$f", "$out/lib/$rel/$f");
    }
}

# Strawberry writes the folder it is installed in into Config.pm and
# Config_heavy.pl (its relocation.txt names both, and the folder on its first
# line).  Put back its default folder, C:\strawberry\, so that the copy says
# nothing about the computer it was made on.
my @here = ($top . '\\');
if (open my $rl, '<', "$top/relocation.txt") {
    my $l = <$rl>;
    close $rl;
    $l =~ s/\s+\z//;
    push @here, $l if length $l;
}
tr{/}{\\} for @here;
for my $f (qw(Config.pm Config_heavy.pl Config_git.pl)) {
    my $p = "$out/lib/$f";
    next unless -f $p;
    open my $in, '<:raw', $p or die;
    my $text = do { local $/; <$in> };
    close $in;
    for my $h (@here) {
        (my $h2 = $h) =~ s/\\/\\\\/g;
        $text =~ s/\Q$h2\E/C:\\\\strawberry\\\\/gi;    # the doubled form (Config.pm)
        $text =~ s/\Q$h\E/C:\\strawberry\\/gi;         # the plain form
    }
    open my $o, '>:raw', $p or die;
    print $o $text;
    close $o;
}
# ... and nothing else may name it either
for my $h (@here) {
    (my $bare = $h) =~ s/\\\z//;
    (my $fwd = $bare) =~ tr{\\}{/};
    (my $dbl = $bare) =~ s/\\/\\\\/g;
    my @hit;
    my @dirs = ($out);
    while (my $d = shift @dirs) {
        opendir my $dh, $d or die;
        for my $e (grep { !/^\.\.?\z/ } readdir $dh) {
            my $p = "$d/$e";
            if (-d $p) { push @dirs, $p; next }
            open my $in, '<:raw', $p or die;
            my $text = do { local $/; <$in> };
            close $in;
            push @hit, $p if $text =~ /\Q$bare\E|\Q$fwd\E|\Q$dbl\E/i;
        }
    }
    die "mkruntime.pl: these still name $bare:\n  " . join("\n  ", @hit) . "\n" if @hit;
}

# the licences of Perl and of the gcc runtime DLLs
for my $pair (['licenses/perl', 'licenses/perl'],
              ['licenses/gcc-toolchain/gcc', 'licenses/gcc'],
              ['licenses/gcc-toolchain/mingw-w64', 'licenses/mingw-w64'],
              ['licenses/gcc-toolchain/winpthreads', 'licenses/winpthreads']) {
    my ($from, $to) = ("$top/$pair->[0]", "$out/$pair->[1]");
    next unless -d $from;
    opendir my $d, $from or die;
    put("$from/$_", "$to/$_")
        for grep { /^(?:Artistic|Copying|COPYING|DISCLAIMER|Readme)/ && -f "$from/$_" } readdir $d;
}

open my $rd, '>', "$out/README.txt" or die;
printf $rd <<'TEXT', sprintf('%vd', $^V), $Config{archname}, $modules, scalar(keys %dll);
This is the part of Strawberry Perl (perl %s, %s) that
Explore uses: perl.exe and its DLLs in bin, and in lib the %d library files
(and %d XS DLLs) that the game was seen to load.  src\mkruntime.pl made it;
it is meant to run the port's explore.pl and nothing else.  Config.pm and
Config_heavy.pl name C:\strawberry, Strawberry's own default folder, where
an installed Strawberry Perl names the folder it is in.  The licences of
Perl and of the gcc runtime DLLs are in licenses.
TEXT
close $rd;
print "mkruntime.pl: $n files in $out\n";
