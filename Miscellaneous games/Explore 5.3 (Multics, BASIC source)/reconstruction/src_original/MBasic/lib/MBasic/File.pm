package MBasic::File;
use strict;
use warnings;
our $VERSION = '1.2';

use Fcntl qw(O_WRONLY O_CREAT O_EXCL O_TRUNC);
use Errno qw(EACCES EPERM EROFS ENOENT);

# O_NOFOLLOW is present on Linux/*BSD/OpenBSD but not universally; fall back to
# 0 (no-op) where the platform lacks it.
use constant O_NOFOLLOW_ => do { my $v = eval { Fcntl::O_NOFOLLOW() }; defined $v ? $v : 0 };

# per-process counter so two channels writing the same file don't collide on
# the temp name within one process.
my $TMPSEQ = 0;

# ============================================================================
#  MBasic::File -- one terminal-format file channel (#1..#4).
#
#  Terminal-format files are the only kind Explore uses: a sequence of lines
#  of ASCII text.  We hold the file's lines in memory (loaded on open), serve
#  reads from a cursor, and flush writes back to disk.
#
#  Model / semantics (from AM82-01 + verified Multics behaviors):
#    open(path):   load the file's lines if it exists; if absent, start empty.
#                  LAZY -- a missing file does NOT error on open; it simply
#                  reads as immediately at-end (verified: `file` on a
#                  nonexistent file makes `if end` true, no error).
#    input fields: fields are delimited by COMMA or NEWLINE; the cursor
#                  advances past the delimiter.  Reads span lines as needed.
#    linput:       returns the rest of the current line (a whole line), cursor
#                  moves to the start of the next line.
#    print:        the FIRST print sets the pointer to END (append).  We model
#                  print as appending a line to the buffer (BASIC print #n
#                  writes a line of output; a trailing ';' would suppress the
#                  newline, but Explore's print #n statements each emit one
#                  logical line -- see note).  Writes flush to disk.
#    scratch:      erase all content, cursor to beginning.
#    reset:        cursor to beginning (re-read).
#    at_end:       true when the cursor is past the last datum.
#
#  NOTE on print #n and ';':  a print list builds one output line; a trailing
#  ';' suppresses the newline so the NEXT print continues the same line.  We
#  track a pending (unterminated) output line and only commit it as a buffer
#  line when a newline is produced (print with no trailing ';', or an explicit
#  break).  For Explore's usage each print #n produces a complete comma-joined
#  line, so this is straightforward.
# ============================================================================

sub new {
    my ($class, %o) = @_;
    bless {
        path    => $o{path},
        lines   => [],        # the file content, one string per line
        line    => 0,         # cursor: current line index (0-based)
        col     => 0,         # cursor: character offset within lines[line]
        dirty   => 0,         # needs flush?
        pending => undef,     # partial output line not yet committed (print ;)
        wrote   => 0,         # has any print happened (append semantics)?
    }, $class;
}

# open a file on this channel: load its content (lazy -- missing is empty).
sub open_path {
    my ($class, $path) = @_;
    my $self = $class->new(path => $path);
    if (defined $path && -f $path) {
        open my $fh, '<', $path or do { return $self; };  # unreadable -> empty
        local $/ = "\n";   # never inherit a caller's alternate record separator
        my @l = <$fh>; close $fh;
        chomp @l;
        $self->{lines} = \@l;
    }
    return $self;   # cursor at beginning; if no lines, at_end is immediately true
}

# ---- read position helpers ----
sub at_end {
    my ($self) = @_;
    return 1 if $self->{line} > $#{$self->{lines}};
    # at last line but past its end, with no more lines -> end
    if ($self->{line} == $#{$self->{lines}}
        && $self->{col} >= length($self->{lines}[$self->{line}])) {
        return 1;
    }
    return 0;
}

# reset cursor to the beginning (reset #n)
sub reset_pointer {
    my ($self) = @_;
    $self->{line} = 0; $self->{col} = 0;
    return;
}

# scratch: erase content
sub scratch {
    my ($self) = @_;
    $self->{lines} = []; $self->{line}=0; $self->{col}=0;
    $self->{pending} = undef; $self->{dirty} = 1;
    $self->_flush;
    return;
}

# ---- reading ----
# linput: return the remainder of the current line; advance to next line start.
sub linput {
    my ($self) = @_;
    die "End-of-file ($self->{path})\n" if $self->at_end;
    my $cur = $self->{lines}[$self->{line}] // '';
    my $val = substr($cur, $self->{col});
    $self->{line}++; $self->{col} = 0;
    return $val;
}

# input one field: gather characters up to the next comma or end-of-line,
# then advance past that delimiter.  A field may be numeric or string; the
# caller (executor) coerces per the target variable's type.
sub input_field {
    my ($self) = @_;
    die "Not enough input, add more ($self->{path})\n" if $self->at_end;
    # if cursor is at end of the current line, move to next line
    while ($self->{line} <= $#{$self->{lines}}
           && $self->{col} >= length($self->{lines}[$self->{line}])) {
        $self->{line}++; $self->{col} = 0;
        die "Not enough input, add more ($self->{path})\n"
            if $self->{line} > $#{$self->{lines}};
    }
    my $cur = $self->{lines}[$self->{line}];
    my $comma = index($cur, ',', $self->{col});
    my $field;
    if ($comma < 0) {
        # rest of line is the field; advance to next line
        $field = substr($cur, $self->{col});
        $self->{line}++; $self->{col} = 0;
    } else {
        $field = substr($cur, $self->{col}, $comma - $self->{col});
        $self->{col} = $comma + 1;   # past the comma
    }
    # trim surrounding blanks for numeric-ish fields (caller decides coercion);
    # for strings, Multics allows quoted fields -- strip surrounding quotes.
    $field =~ s/^\s+//; $field =~ s/\s+$//;
    if ($field =~ /^"(.*)"$/) { $field = $1; $field =~ s/""/"/g; }
    return $field;
}

# ---- writing ----
# print a rendered chunk to the file's current output line; if $newline,
# terminate the line (commit it to the buffer).  First write triggers append
# semantics: the pointer is at end, so we append new lines to {lines}.
#
# NOTE on Multics vs Unix visibility:  On Multics, print writes into the
# single-level-store segment but the segment's BIT-COUNT (recorded length)
# lags until close (or an adjust_bit_count/abc call), so an EXTERNAL process
# reading mid-run sees "zero length" or stale data -- which is exactly why the
# game uses the "reopen the channel" trick or `abc` before other players read
# a shared file.  On Unix we flush eagerly here so a player who RE-OPENS the
# shared file sees prior writes at once (the behavior the game's abc/trick
# machinery was straining to achieve on Multics), and `abc` becomes a harmless
# no-op.  Same-program reads are served from {lines}.
#
# Caveat: the atomic-rename flush (see _flush) replaces the file's inode, so a
# process holding the OLD file descriptor open keeps reading the old content;
# visibility requires re-opening by path (which the game always does).  This is
# the normal trade-off for crash-safe replacement, and per-print flushing means
# every print #n rewrites the whole (small) file -- fine for the game's use.
sub print_chunk {
    my ($self, $text, $newline) = @_;
    $self->{wrote} = 1;
    $self->{pending} = '' unless defined $self->{pending};
    $self->{pending} .= $text;
    if ($newline) {
        push @{$self->{lines}}, $self->{pending};
        $self->{pending} = undef;
    }
    $self->{dirty} = 1;
    $self->_flush;              # eager flush (Unix cross-process visibility)
    return;
}

# serialize the current buffer (committed lines + any unterminated pending
# line) to an already-open filehandle.  Dies (caught by the caller) on error.
sub _write_to {
    my ($self, $fh) = @_;
    print $fh map { "$_\n" } @{$self->{lines}} or die "$!\n";
    # a pending (unterminated) partial line is written without a newline
    if (defined $self->{pending}) { print $fh $self->{pending} or die "$!\n"; }
    close $fh or die "$!\n";
    return 1;
}

# flush buffered content to disk (called on scratch, close, and after writes).
#
# Strategy 1 -- atomic temp-file + rename (the default).  Crash-safe (a partial
# or ENOSPC write never truncates the real file), and it replaces a planted
# symlink AT THE TARGET rather than clobbering the link's destination.  The
# temp file is created with O_CREAT|O_EXCL|O_NOFOLLOW so a predictable temp name
# cannot be pre-planted as a symlink to redirect the write.  The original
# file's permission bits (and, where permitted, its owner/group) are copied
# onto the replacement so a group-writable shared file stays group-writable.
#
# Strategy 2 -- in-place rewrite (fallback).  Creating the temp needs WRITE
# access to the containing directory.  A common shared-game layout (see
# Explore's explore_setup.ec) makes the data directory NON-writable to players
# but pre-creates the data files writable -- the Unix analog of Multics
# per-segment ACLs, where you can write a segment without modify access to its
# directory.  When the temp create fails for that reason (EACCES/EPERM/EROFS),
# we fall back to rewriting the existing file in place, opened O_NOFOLLOW so a
# planted symlink at the path is still refused.  This loses atomicity for that
# one case, but it is the only way to honor the intended "writable file in a
# read-only directory" deployment.
#
# NOTE: neither strategy provides multi-writer locking; the whole-file rewrite
# model means concurrent writers can still lose updates.  Coordinating that is
# the embedder's responsibility (Explore uses advisory locking around its
# shared files).  What is fixed here is data-destruction: partial writes,
# symlink attacks, silent write errors, and permission/owner drift.
sub _flush {
    my ($self) = @_;
    return unless defined $self->{path};
    my $path = $self->{path};

    # --- Strategy 1: atomic temp + rename ---
    my $dir  = ($path =~ m{^(.*)/[^/]+$}) ? $1 : '.';
    my $fh;
    my $opened = 0;
    my $tmp;
    # Try a few temp names.  O_EXCL means a name already present (a stale temp,
    # or one an attacker pre-planted -- including as a symlink) makes the create
    # fail with EEXIST; we simply pick another name rather than following or
    # clobbering it.  O_NOFOLLOW is belt-and-suspenders on the same point.
    for my $try (1 .. 8) {
        $tmp = sprintf('%s/.mbtmp.%d.%d.%d', $dir, $$, ++$TMPSEQ, $try);
        if (sysopen($fh, $tmp, O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW_, 0600)) { $opened = 1; last; }
        last unless $!{EEXIST};   # a non-collision error -> stop trying temps
    }
    if ($opened) {
        my $ok = eval { $self->_write_to($fh); 1 };
        unless ($ok) {
            my $err = $@ || 'write error';
            unlink $tmp;
            die "Cannot write into file ($path): $err\n";
        }
        # preserve the target's permissions and (best-effort) ownership so a
        # shared group-writable file does not become owner-only after a write.
        # For a NEW file (no existing target) the private 0600 the temp was
        # created with would lock other players out of a shared directory, so
        # apply the process umask (0666 & ~umask) as an ordinary create would.
        if (my @st = stat $path) {
            chmod $st[2] & 07777, $tmp;
            chown $st[4], $st[5], $tmp;   # succeeds for root / matching owner; ignored otherwise
        } else {
            my $um = umask;
            chmod( (0666 & ~$um), $tmp ) if defined $um;
        }
        if (rename $tmp, $path) { $self->{dirty} = 0; return; }
        # rename failed: clean up and report (do NOT silently fall through to a
        # destructive in-place write for an unexpected rename failure).
        my $err = $!;
        unlink $tmp;
        die "Cannot write into file ($path): $err\n";
    }

    # temp create failed.  If it was a directory-permission issue, fall back to
    # an in-place rewrite; otherwise it's a real error (e.g. ENOSPC) -> report.
    my $why = $!;
    unless ($!{EACCES} || $!{EPERM} || $!{EROFS}) {
        die "Cannot write into file ($path): $why\n";
    }

    # --- Strategy 2: in-place rewrite (writable file in a read-only dir) ---
    # The file must already exist and be writable; O_NOFOLLOW refuses a symlink.
    my $fh2;
    unless (sysopen($fh2, $path, O_WRONLY|O_TRUNC|O_NOFOLLOW_)) {
        die "Cannot write into file ($path): $!\n";
    }
    my $ok2 = eval { $self->_write_to($fh2); 1 };
    unless ($ok2) {
        my $err = $@ || 'write error';
        die "Cannot write into file ($path): $err\n";
    }
    $self->{dirty} = 0;
    return;
}

sub close {
    my ($self) = @_;
    # Flush only if there is unflushed data.  With eager flushing (print_chunk
    # flushes each write) dirty is already 0 here, so close does NOT perform a
    # redundant second rewrite; it exists so that a future non-eager mode, or a
    # sub whose channels are closed on return, still commits.  A read-only
    # channel (never written) has dirty=0 and is never rewritten, so closing it
    # cannot touch mtime or clobber its file.  A trailing ';' on the last write
    # leaves an unterminated final line on disk, as intended for terminal-format
    # files.
    $self->_flush if $self->{dirty};
    return;
}

1;

__END__

=head1 NAME

MBasic::File - a Multics BASIC terminal-format file channel

=head1 DESCRIPTION

One terminal-format file channel.  Loads a file's lines on open (a missing
file is not an error -- it simply reads as immediately at-end, matching
Multics), serves sequential field and line reads, and appends writes.
Writes are flushed eagerly so that other processes see them immediately;
this is the behavior the Multics game works to achieve with its
adjust-bit-count and channel-reopen tricks, and on Unix it is automatic.

=head1 METHODS

=head2 open_path($path)

Class method.  Open a channel on a file, loading its content.

=head2 input_field / linput / print_chunk($text, $newline) / scratch / reset_pointer / at_end / close

Field read, whole-line read, buffered write, erase, rewind, end-of-file test,
and flush-and-close, respectively.

=head1 SEE ALSO

L<MBasic> for the overview and architecture.

=head1 AUTHOR

Jim Lippard <lippard@discord.org>

=head1 LICENSE

Copyright (c) 2026 Jim Lippard.  Free software under the BSD 3-Clause License.

=cut
