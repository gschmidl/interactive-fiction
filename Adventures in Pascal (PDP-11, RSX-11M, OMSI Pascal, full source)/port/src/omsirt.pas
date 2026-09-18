{ OMSIRT - the pieces of OMSI Pascal-1 V1.2 / RSX-11M that Barry Breen's
  "Adventures in Pascal" leans on, for Free Pascal on Windows.

  The game source itself is compiled unchanged apart from the mechanical
  substitutions made by tools/weave.py; everything those substitutions call
  lives here:

    OMSIRESET / OMSIREWRITE / OMSISEEK / OMSIGET / OMSIPUT / OMSICLOSE
        OMSI random-access "FILE OF T".  Records are numbered from 1.  SEEK(F,n)
        positions on record n and loads it into F^; GET moves to the next record
        and loads it; PUT writes F^ at the current record and moves on.  On disk
        a record never spans a 512-byte block: each block holds
        512 DIV SIZEOF(T) records and the rest of the block is zero - this is
        the layout of the .DTA files on the DECUS tape, which the game reads
        as they are.
    OMSIRDLN / OMSIRDLNF
        READLN into a character array (blank padded) or an integer.
    OMSIGTIM, TIME
        the RSX GTIM$ directive and OMSI's TIME (hours since midnight).

  plus what a present-day player needs: where the data and the saved-game
  file live, a clock that can be frozen, and -u to lift the cave hours. }
{$mode objfpc}{$H+}{$R-}{$Q-}
unit omsirt;

interface

type
  OMSIFILE = record
    f: file;
    path: string;
    recsize, rpb: longint;
    recno: longint;              { current record, from 1 }
    bufp: pointer;               { the program's F^ }
    writable, iswiz: boolean;
    sig: longint;                { OPENSIG while open - locals start as garbage }
  end;
  OMSISPK = procedure(MSG: smallint);
  OMSIDBGHOOK = function(const cmd: shortstring): shortstring;

procedure OMSIRESET(var o: OMSIFILE; var buf; size: longint; const spec: string);
procedure OMSIREWRITE(var o: OMSIFILE; var buf; size: longint; const spec: string);
procedure OMSISEEK(var o: OMSIFILE; n: longint);
procedure OMSIGET(var o: OMSIFILE);
procedure OMSIPUT(var o: OMSIFILE);
procedure OMSICLOSE(var o: OMSIFILE);
procedure OMSIRESETTEXT(var t: text; const spec: string);
procedure OMSIRDLN(var a: array of char); overload;
procedure OMSIRDLN(var i: smallint); overload;
procedure OMSIRDLN(var w: word); overload;
procedure OMSIRDLNF(var t: text; var a: array of char);
procedure OMSIRDINTF(var t: text; var i: smallint);
procedure OMSIGTIM(var mo, day, year: smallint);
function TIME: single;
function OMSIFROZEN: boolean;          { the time of day is frozen (-t) }
procedure OMSIVT100;

var
  OmsiUnlimited: boolean = false;     { -u }
  OMSIDEBUG: OMSIDBGHOOK = nil;       { the game's '#' command handler (src/debug.inc) }
  OmsiDebugOn: boolean = false;       { --debug }
  OmsiEcho: boolean = false;          { --echo }
  OMSIFIXES: boolean = true;          { --no-fixes clears it: see README, "Fixes" }
  OmsiProgram: string = 'adventure';  { from the name of the .exe }

implementation

uses sysutils, windows;

const OPENSIG = $4F4D5349;

var
  DataDir, SaveDir: string;
  FrozenDate, FrozenTime: boolean;
  FzYear, FzMonth, FzDay, FzHour, FzMin, FzSec: word;
  ConOut: THandle;
  ConIsConsole: boolean = false;
  VtNoWrap: boolean = false;   { auto wrap turned off for the VT100 database }
  ConOldMode: DWORD;
  DbLine: string;              { the ASCII database, a line at a time }
  DbPos: integer;
  DbHave: boolean = false;

{ ------------------------------------------------------------ files }

function StripSwitches(const spec: string): string;
var p: integer;
begin
  p := Pos('/', spec);
  if p > 0 then StripSwitches := Copy(spec, 1, p - 1) else StripSwitches := spec;
end;

procedure Die(const msg: string);
begin
  Flush(Output);
  WriteLn(StdErr, OmsiProgram, ': ', msg);
  Halt(2);
end;

function CopyFileTo(const src, dst: string): boolean;
var a, b: file; buf: array[0..4095] of byte; n: longint;
begin
  CopyFileTo := false;
  if not FileExists(src) then exit;
  Assign(a, src); FileMode := 0; Reset(a, 1);
  Assign(b, dst); Rewrite(b, 1);
  repeat
    BlockRead(a, buf, SizeOf(buf), n);
    if n > 0 then BlockWrite(b, buf, n);
  until n = 0;
  Close(a); Close(b);
  CopyFileTo := true;
end;

{ ADVWIZ.DTA is the one file the game writes while it is played (wizard
  settings, the three saved games, the message of the day), so it lives in
  the save directory; the first time it is wanted it is copied from the
  pristine one in the data directory. }
function Resolve(const spec: string; forwrite: boolean; var iswiz: boolean): string;
var name: string;
begin
  name := UpperCase(StripSwitches(spec));
  iswiz := (name = 'ADVWIZ.DTA');
  if iswiz then begin
    if not DirectoryExists(SaveDir) then
      if not ForceDirectories(SaveDir) then Die('cannot create ' + SaveDir);
    Resolve := SaveDir + DirectorySeparator + name;
    if (not forwrite) and (not FileExists(Resolve)) then
      if not CopyFileTo(DataDir + DirectorySeparator + name, Resolve) then
        Die('no ' + name + ' in ' + DataDir + ' (run POOF to create one)');
  end else
    Resolve := DataDir + DirectorySeparator + name;
end;

function RecOffset(var o: OMSIFILE; n: longint): int64;
begin
  RecOffset := int64((n - 1) div o.rpb) * 512 + int64((n - 1) mod o.rpb) * o.recsize;
end;

{ -u: the cave-hours masks (records 11-16) and the restart latency (41) read
  as zero.  The file itself is not touched. }
procedure WizFilter(var o: OMSIFILE);
begin
  if o.iswiz and OmsiUnlimited and (o.recsize = 2) then
    if ((o.recno >= 11) and (o.recno <= 16)) or (o.recno = 41) then
      PSmallInt(o.bufp)^ := 0;
end;

procedure LoadRec(var o: OMSIFILE);
var off: int64; got: longint;
begin
  FillChar(o.bufp^, o.recsize, 0);
  if o.recno < 1 then exit;
  off := RecOffset(o, o.recno);
  if off + o.recsize <= FileSize(o.f) then begin
    Seek(o.f, off);
    BlockRead(o.f, o.bufp^, o.recsize, got);
  end;
  WizFilter(o);
end;

procedure OpenIt(var o: OMSIFILE; var buf; size: longint; const spec: string; create: boolean);
var sw: string;
begin
  if o.sig = OPENSIG then OMSICLOSE(o);
  sw := UpperCase(spec);
  o.path := Resolve(spec, create, o.iswiz);
  o.recsize := size;
  o.rpb := 512 div size;
  o.bufp := @buf;
  o.recno := 1;
  o.writable := create or (Pos('/RW', sw) > 0);
  Assign(o.f, o.path);
  {$I-}
  if create then Rewrite(o.f, 1)
  else begin
    if o.writable then FileMode := 2 else FileMode := 0;
    Reset(o.f, 1);
  end;
  {$I+}
  if IOResult <> 0 then Die('cannot open ' + o.path);
  FileMode := 2;
  o.sig := OPENSIG;
  if not create then LoadRec(o);
end;

procedure OMSIRESET(var o: OMSIFILE; var buf; size: longint; const spec: string);
begin
  OpenIt(o, buf, size, spec, false);
end;

procedure OMSIREWRITE(var o: OMSIFILE; var buf; size: longint; const spec: string);
begin
  OpenIt(o, buf, size, spec, true);
end;

procedure OMSISEEK(var o: OMSIFILE; n: longint);
begin
  if o.sig <> OPENSIG then Die('SEEK on a closed file');
  o.recno := n;
  LoadRec(o);
end;

procedure OMSIGET(var o: OMSIFILE);
begin
  if o.sig <> OPENSIG then Die('GET on a closed file');
  Inc(o.recno);
  LoadRec(o);
end;

procedure OMSIPUT(var o: OMSIFILE);
var off, size: int64; zero: array[0..511] of byte; n: longint;
begin
  if o.sig <> OPENSIG then Die('PUT on a closed file');
  if not o.writable then Die('PUT on a read-only file ' + o.path);
  off := RecOffset(o, o.recno);
  size := FileSize(o.f);
  if off > size then begin           { keep the gap (block slack) zero }
    FillChar(zero, SizeOf(zero), 0);
    Seek(o.f, size);
    while size < off do begin
      n := 512; if off - size < n then n := off - size;
      BlockWrite(o.f, zero, n);
      Inc(size, n);
    end;
  end;
  Seek(o.f, off);
  BlockWrite(o.f, o.bufp^, o.recsize);
  Inc(o.recno);
end;

procedure OMSICLOSE(var o: OMSIFILE);
var size: int64; zero: array[0..511] of byte; n: longint;
begin
  if o.sig <> OPENSIG then exit;
  if o.writable then begin           { files are whole 512-byte blocks }
    size := FileSize(o.f);
    n := (512 - (size mod 512)) mod 512;
    if n > 0 then begin
      FillChar(zero, SizeOf(zero), 0);
      Seek(o.f, size);
      BlockWrite(o.f, zero, n);
    end;
  end;
  Close(o.f);
  o.sig := 0;
end;

procedure OMSIRESETTEXT(var t: text; const spec: string);
var dummy: boolean;
begin
  Assign(t, Resolve(spec, false, dummy));
  {$I-} Reset(t); {$I+}
  if IOResult <> 0 then Die('cannot open ' + StripSwitches(spec) + ' in ' + DataDir);
  DbHave := false;
end;

{ ------------------------------------------------------------ terminal }

{ What the terminal has to be told when the game is over: ESC < because the
  game leaves a VT100 in VT52 mode, as RSX terminals usually were (it puts a
  present-day terminal back in ANSI mode and is ignored if it never left it),
  then auto wrap back on if OMSIVT100 turned it off. }
const
  TERM_RESTORE_WRAP = #27'<'#27'[?7h';
  TERM_RESTORE      = #27'<';

procedure RestoreTerminal;
begin
  Flush(Output);
  if ConIsConsole then begin
    if VtNoWrap then Write(TERM_RESTORE_WRAP) else Write(TERM_RESTORE);
    Flush(Output);
    SetConsoleMode(ConOut, ConOldMode);
  end;
end;

{ The player has said "yes, a VT100".  Every line of the VT100 database is
  written as all 72 columns, trailing blanks included, and the double-width
  and double-height lines only hold 40.  A VT100 came with auto wrap off, so
  the blanks piled up harmlessly at the right margin; a present-day terminal
  has it on, and wrapped them into a blank row under every big line - which
  also pulled the two halves of double-height text apart.  So: a VT100 with
  auto wrap off, which is what the text was written for. }
procedure OMSIVT100;
begin
  if ConIsConsole and not VtNoWrap then begin
    Flush(Output);
    Write(#27'[?7l');
    Flush(Output);
    VtNoWrap := true;
  end;
end;

{ Ctrl-C, Ctrl-Break or the window closing: the run-time's own exit code does
  not run, so put the terminal back by hand (from the handler's thread, hence
  WriteFile and not Write). }
function CtrlHandler(CtrlType: DWORD): BOOL; stdcall;
var s: shortstring; n: DWORD;
begin
  if ConIsConsole then begin
    if VtNoWrap then s := TERM_RESTORE_WRAP else s := TERM_RESTORE;
    n := 0;
    WriteFile(ConOut, s[1], Length(s), n, nil);
    SetConsoleMode(ConOut, ConOldMode);
  end;
  CtrlHandler := false;                { and now let Windows end the program }
end;

procedure EndOfInput;
begin
  WriteLn;
  RestoreTerminal;
  ConIsConsole := false;
  Halt(0);
end;

procedure ReadLine(var s: string);
begin
  Flush(Output);
  if Eof(Input) then EndOfInput;
  ReadLn(Input, s);
  if OmsiEcho then WriteLn(s);         { what a terminal would have shown }
end;

procedure OMSIRDLN(var a: array of char);
var s: string; i: integer; r: shortstring;
begin
  repeat
    ReadLine(s);
    if OmsiDebugOn and Assigned(OMSIDEBUG) and (Length(s) > 0) and (s[1] = '#') then begin
      r := OMSIDEBUG(s);
      if r = '' then begin Write('->'); continue; end;
      s := r;
    end;
    break;
  until false;
  for i := 0 to High(a) do
    if i < Length(s) then a[i] := s[i + 1] else a[i] := ' ';
end;

function ParseInt(const s: string): longint;
var v: longint; code: integer; t: string;
begin
  t := Trim(s);
  Val(t, v, code);
  if code <> 0 then begin            { take the leading number, as READ does }
    Val(Copy(t, 1, code - 1), v, code);
    if code <> 0 then v := 0;
  end;
  ParseInt := v;
end;

procedure OMSIRDLN(var i: smallint);
var s: string;
begin
  ReadLine(s);
  i := smallint(ParseInt(s));
end;

procedure OMSIRDLN(var w: word);
var s: string;
begin
  ReadLine(s);
  w := word(ParseInt(s));
end;

{ The ASCII database.  OMSI's READ of an integer stops at the first character
  that cannot belong to the number ("1You are standing" is 1, then the text)
  and takes commas as separators; Free Pascal's wants a blank after it.  So the
  database is read a line at a time here and picked apart by hand. }
procedure DbNextLine(var t: text);
begin
  if Eof(t) then Die('unexpected end of the database file');
  ReadLn(t, DbLine);
  DbPos := 1;
  DbHave := true;
end;

procedure OMSIRDINTF(var t: text; var i: smallint);
var v: longint; neg: boolean;
begin
  repeat
    if not DbHave then DbNextLine(t);
    while (DbPos <= Length(DbLine)) and (DbLine[DbPos] in [' ', #9, ',']) do Inc(DbPos);
    if DbPos > Length(DbLine) then DbHave := false;
  until DbHave;
  neg := false;
  if DbLine[DbPos] in ['+', '-'] then begin neg := DbLine[DbPos] = '-'; Inc(DbPos); end;
  if (DbPos > Length(DbLine)) or not (DbLine[DbPos] in ['0'..'9']) then
    Die('number expected in the database: ' + DbLine);
  v := 0;
  while (DbPos <= Length(DbLine)) and (DbLine[DbPos] in ['0'..'9']) do begin
    v := v * 10 + Ord(DbLine[DbPos]) - Ord('0');
    Inc(DbPos);
  end;
  if neg then v := -v;
  i := smallint(v);
end;

procedure OMSIRDLNF(var t: text; var a: array of char);
var k, n: integer;
begin
  if not DbHave then DbNextLine(t);
  for k := 0 to High(a) do begin
    n := DbPos + k;
    if n <= Length(DbLine) then a[k] := DbLine[n] else a[k] := ' ';
  end;
  DbHave := false;
end;

{ ------------------------------------------------------------ clock }

procedure Clock(var y, mo, d, h, mi, s: word);
var ms: word; n: TDateTime;
begin
  n := Now;
  DecodeDate(n, y, mo, d);
  DecodeTime(n, h, mi, s, ms);
  if FrozenDate then begin y := FzYear; mo := FzMonth; d := FzDay; end;
  if FrozenTime then begin h := FzHour; mi := FzMin; s := FzSec; end;
end;

{ GTIM$: G.TIYR is the year since 1900 }
procedure OMSIGTIM(var mo, day, year: smallint);
var y, m, d, h, mi, s: word;
begin
  Clock(y, m, d, h, mi, s);
  mo := m; day := d; year := y - 1900;
end;

{ OMSI TIME: hours since midnight }
function TIME: single;
var y, m, d, h, mi, s: word;
begin
  Clock(y, m, d, h, mi, s);
  TIME := h + mi / 60.0 + s / 3600.0;
end;

function OMSIFROZEN: boolean;
begin
  OMSIFROZEN := FrozenTime;
end;

{ ------------------------------------------------------------ options }

const MONTHS: array[1..12] of string[3] =
  ('JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC');

procedure SetDate(const v: string);        { DD-MMM-YYYY }
var p1, p2, i, code: integer; mm: string; d, y: longint;
begin
  p1 := Pos('-', v);
  p2 := 0;
  if p1 > 0 then p2 := p1 + Pos('-', Copy(v, p1 + 1, 255));
  if (p1 < 2) or (p2 <= p1 + 1) then Die('date must be DD-MMM-YYYY, e.g. 28-OCT-1980');
  Val(Copy(v, 1, p1 - 1), d, code);      if code <> 0 then Die('bad day in ' + v);
  Val(Copy(v, p2 + 1, 255), y, code);    if code <> 0 then Die('bad year in ' + v);
  mm := UpperCase(Copy(v, p1 + 1, p2 - p1 - 1));
  FzMonth := 0;
  for i := 1 to 12 do if MONTHS[i] = mm then FzMonth := i;
  if FzMonth = 0 then Die('bad month in ' + v);
  if y < 100 then y := y + 1900;
  if (d < 1) or (d > 31) or (y < 1977) or (y > 2066) then Die('date out of range: ' + v);
  FzDay := d; FzYear := y; FrozenDate := true;
end;

procedure SetTime(const v: string);        { HHMM or HHMMSS }
var n: longint; code: integer;
begin
  Val(v, n, code);
  if (code <> 0) or not (Length(v) in [4, 6]) then Die('time must be HHMM or HHMMSS');
  if Length(v) = 4 then n := n * 100;
  FzHour := n div 10000; FzMin := (n div 100) mod 100; FzSec := n mod 100;
  if (FzHour > 23) or (FzMin > 59) or (FzSec > 59) then Die('time out of range: ' + v);
  FrozenTime := true;
end;

function Summary(const prog: string): string;
begin
  if prog = 'advfls' then Summary := 'Build ADVTXT.DTA, KATAB.DTA, ADVDAT.DTA and ADVENT.DTA from ADVENTURE.DAT.'
  else if prog = '100fls' then Summary := 'Build ADVTXT.100 and ADVDAT.100 (the VT100 text) from ADVENTURE.100.'
  else if prog = 'poof' then Summary := 'Create a fresh ADVWIZ.DTA: magic word DWARF, number 11111, cave always open.'
  else if prog = 'peek' then Summary := 'Show what ADVWIZ.DTA holds: magic word and number, saved games, hours.'
  else Summary := 'Adventures in Pascal - Barry C. Breen, 1980-82 (OMSI Pascal, RSX-11M).';
end;

procedure Help(const prog: string);
begin
  WriteLn('Usage: ', prog, ' [OPTION]...');
  WriteLn(Summary(prog));
  WriteLn;
  WriteLn('  -u, --unlimited      ignore the cave hours and the wait before a saved');
  WriteLn('                       game may be resumed (ADVWIZ.DTA is not changed)');
  WriteLn('                       and answer the wizard test for you (try MAGIC MODE');
  WriteLn('                       as your first command)');
  WriteLn('  -d, --date=DD-MMM-YYYY   pretend it is this date');
  WriteLn('  -t, --time=HHMM[SS]      pretend it is this time; with the clock frozen the');
  WriteLn('                       random numbers, and so the whole game, repeat');
  WriteLn('      --echo           show each input line, for transcripts of redirected input');
  WriteLn('      --no-fixes       leave the original''s bugs in (see README.md)');
  WriteLn('      --data=DIR       game database directory     [<exe>\data]');
  WriteLn('      --save=DIR       where ADVWIZ.DTA is kept    [<exe>\save]');
  WriteLn('  -h, --help           show this help');
  WriteLn;
  WriteLn('Environment: ADVPAS_DATA, ADVPAS_SAVE, ADVPAS_DATE, ADVPAS_TIME.');
  Halt(0);
end;

procedure OmsiInit(const prog: string);
var i: integer; a, key, val: string; p: integer; hasval: boolean;

  function Value: string;
  begin
    if hasval then Value := val
    else begin
      Inc(i);
      if i > ParamCount then Die('option ' + key + ' needs a value');
      Value := ParamStr(i);
    end;
  end;

begin
  OmsiProgram := prog;
  DataDir := ExtractFilePath(ParamStr(0)) + 'data';
  SaveDir := ExtractFilePath(ParamStr(0)) + 'save';
  if SysUtils.GetEnvironmentVariable('ADVPAS_DATA') <> '' then DataDir := SysUtils.GetEnvironmentVariable('ADVPAS_DATA');
  if SysUtils.GetEnvironmentVariable('ADVPAS_SAVE') <> '' then SaveDir := SysUtils.GetEnvironmentVariable('ADVPAS_SAVE');
  if SysUtils.GetEnvironmentVariable('ADVPAS_DATE') <> '' then SetDate(SysUtils.GetEnvironmentVariable('ADVPAS_DATE'));
  if SysUtils.GetEnvironmentVariable('ADVPAS_TIME') <> '' then SetTime(SysUtils.GetEnvironmentVariable('ADVPAS_TIME'));
  i := 1;
  while i <= ParamCount do begin
    a := ParamStr(i);
    hasval := false; val := ''; key := a;
    if Copy(a, 1, 2) = '--' then begin
      p := Pos('=', a);
      if p > 0 then begin key := Copy(a, 1, p - 1); val := Copy(a, p + 1, 255); hasval := true; end;
    end;
    if (key = '-u') or (key = '--unlimited') then OmsiUnlimited := true
    else if (key = '-h') or (key = '--help') then Help(prog)
    else if (key = '-d') or (key = '--date') then SetDate(Value)
    else if (key = '-t') or (key = '--time') then SetTime(Value)
    else if key = '--no-fixes' then OMSIFIXES := false
    else if key = '--debug' then OmsiDebugOn := true
    else if key = '--echo' then OmsiEcho := true
    else if key = '--data' then DataDir := Value
    else if key = '--save' then SaveDir := Value
    else Die('unknown option ' + a + ' (try --help)');
    Inc(i);
  end;
  while (Length(DataDir) > 1) and (DataDir[Length(DataDir)] in ['\', '/']) do Delete(DataDir, Length(DataDir), 1);
  while (Length(SaveDir) > 1) and (SaveDir[Length(SaveDir)] in ['\', '/']) do Delete(SaveDir, Length(SaveDir), 1);
  if not DirectoryExists(DataDir) then Die('no data directory ' + DataDir + ' (see --data)');
  { a real console: let it interpret the VT100 sequences of the .100 database }
  ConOut := GetStdHandle(STD_OUTPUT_HANDLE);
  if GetConsoleMode(ConOut, ConOldMode) then begin
    ConIsConsole := true;
    SetConsoleMode(ConOut, ConOldMode or 4 { ENABLE_VIRTUAL_TERMINAL_PROCESSING });
    { a program started from a shell that ignores Ctrl-C inherits that }
    SetConsoleCtrlHandler(nil, false);
    SetConsoleCtrlHandler(@CtrlHandler, true);
  end;
end;

initialization
  OmsiInit(LowerCase(ChangeFileExt(ExtractFileName(ParamStr(0)), '')));
finalization
  RestoreTerminal;
end.
