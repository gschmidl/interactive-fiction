! brt.f90 - the port's run-time for the Burroughs B7700 Dungeon: the options,
! the terminal, the files of CHANGE / INQUIRE PRESENT / CLOSE DISP, the
! clock (TIME(1) and TIME(7)) and a stand-in for RANDOM.  Compiled like the
! game: 8-byte INTEGER, REAL and LOGICAL.
module brts
  implicit none
  logical :: fixclk = .false., echo = .false.
  integer :: nlines = 0
  character(len=512) :: dirx = '', dirs = ''
  character(len=640) :: upath(0:99) = ''
end module brts

subroutine usage
  write (6, '(a)') &
    'Usage: dungeon [OPTION]...', &
    'Dungeon V2.0 - the DECUS FORTRAN Dungeon, V1.2c code with Tom Fota''s', &
    'international V2.0 text - as a Burroughs B7700 ran it, from the source on', &
    'the INTEREX CSL/1000 release 2213 tape.', &
    '', &
    '      --saves=DIR      where SAVE keeps the game (default: saves\ beside', &
    '                       dungeon.exe)', &
    '      --fixed-clock    the clock counts the lines typed, so that a run', &
    '                       repeats exactly (the game draws its random numbers', &
    '                       from the clock)', &
    '  -h, --help           show this help and exit', &
    '', &
    'Type sentences and press Enter; the end of the input ends the game.'
end subroutine usage

subroutine brtini
  use brts
  integer :: i, n, bisatty
  character(len=512) :: a
  n = command_argument_count()
  do i = 1, n
    call get_command_argument(i, a)
    if (a == '-h' .or. a == '--help') then
      call usage()
      call exit(0)
    else if (a == '--fixed-clock') then
      fixclk = .true.
    else if (a(1:8) == '--saves=') then
      dirs = a(9:)
      if (dirs == '') call bad('option ''--saves'' requires a directory', '')
    else if (a == '--saves') then
      call bad('option ''--saves'' requires an argument', '')
    else if (a(1:2) == '--') then
      call bad('unrecognized option', a)
    else if (a(1:1) == '-') then
      call bad('invalid option --', a(2:2))
    else
      call bad('unexpected argument', a)
    end if
  end do
  call bexep(dirx)
  if (dirs == '') dirs = trim(dirx) // 'saves'
  n = len_trim(dirs)
  if (dirs(n:n) /= '/' .and. dirs(n:n) /= '\') dirs(n + 1:n + 1) = '/'
  call bmkdir(trim(dirs))
  echo = bisatty() == 0
contains
  subroutine bad(msg, arg)
    character(len=*) :: msg, arg
    if (arg == '') then
      write (0, '(a)') 'dungeon: ' // msg
    else
      write (0, '(a)') 'dungeon: ' // msg // ' ''' // trim(arg) // ''''
    end if
    write (0, '(a)') 'Try ''dungeon --help'' for more information.'
    call exit(2)
  end subroutine bad
end subroutine brtini

! ---- the terminal ----------------------------------------------------------

! a line from the player: tabs are blanks, small letters capitals (the parser
! knows capitals only); the end of the input ends the game
subroutine bline(l)
  use brts
  character(len=*) :: l
  integer :: ios, i, c
  flush (6)
  read (5, '(a)', iostat=ios) l
  if (ios /= 0) call exit(0)
  nlines = nlines + 1
  if (echo) write (6, '(a)') trim(l)
  do i = 1, len(l)
    c = ichar(l(i:i))
    if (c == 9) then
      l(i:i) = ' '
    else if (c >= ichar('a') .and. c <= ichar('z')) then
      l(i:i) = char(c - 32)
    end if
  end do
end subroutine bline

! READ(INPCH,100) INBUF with FORMAT(78A1)
subroutine bread(inbuf)
  integer :: inbuf(78), i
  character(len=256) :: l
  call bline(l)
  do i = 1, 78
    inbuf(i) = transfer(l(i:i) // '       ', 0)
  end do
end subroutine bread

! READ(INPCH,110) ANS with FORMAT(An): the first N characters in one word
subroutine breadw(w, n)
  integer :: w, n
  character(len=256) :: l
  call bline(l)
  w = transfer(l(1:n) // repeat(' ', 8 - n), 0)
end subroutine breadw

! READ with FORMAT(nAw) into N words: the characters copied, commas and all
! (gfortran's A editing into an INTEGER ends a field at a comma)
subroutine breada(a, n, w)
  integer :: n, w, a(n), i
  character(len=256) :: l
  call bline(l)
  do i = 1, n
    a(i) = transfer(l((i - 1) * w + 1:i * w) // repeat(' ', 8 - w), 0)
  end do
end subroutine breada

! READ /,J and READ /,J,K: free-field numbers from the terminal
subroutine brdn1(j)
  integer :: j, ios
  character(len=256) :: l
  call bline(l)
  read (l, *, iostat=ios) j
end subroutine brdn1

subroutine brdn2(j, k)
  integer :: j, k, ios
  character(len=256) :: l
  call bline(l)
  read (l, *, iostat=ios) j, k
end subroutine brdn2

! ---- the files -------------------------------------------------------------

! a B7700 title - "(00661)ZORK/PTXT ON SYMBOL30." or "ZORK/SAVDATA." - as a
! file: under a usercode (the installation's) beside dungeon.exe, the
! player's own in the saves folder; ZORK/PTXT is the file ZORK_PTXT
subroutine btitle(t, path)
  use brts
  integer :: t(*), i, k, e
  character(len=*) :: path
  character(len=64) :: s, name
  logical :: inst
  s = ''
  do i = 1, 8
    s(8 * i - 7:8 * i) = transfer(t(i), '12345678')
    if (index(s(8 * i - 7:8 * i), '.') > 0) exit
  end do
  inst = s(1:1) == '('
  k = 1
  if (inst) k = index(s, ')') + 1
  e = index(s(k:), '.') + k - 2
  i = index(s(k:), ' ON ')
  if (i > 0) e = min(e, i + k - 2)
  name = s(k:e)
  do i = 1, len_trim(name)
    if (name(i:i) == '/') name(i:i) = '_'
  end do
  if (inst) then
    path = trim(dirx) // trim(name)
  else
    path = trim(dirs) // trim(name)
  end if
end subroutine btitle

! CHANGE(U,TITLE=T,...,MYUSE=IN/OUT/IO): unit 20, the game's DBCH, is the
! data base's text, direct access; ZORK/DBTXT, the text the data base is
! built from, is the one text file; the rest are the program's own
subroutine bchang(u, t, mode)
  use brts
  integer :: u, t(*), mode
  logical :: op, ex, formatted
  call btitle(t, upath(u))
  inquire (unit=u, opened=op)
  if (op) close (u)
  inquire (file=trim(upath(u)), exist=ex)
  if (mode == 1 .and. .not. ex) return               ! INQUIRE PRESENT will say so
  formatted = index(upath(u), 'ZORK_DBTXT') > 0
  if (u == 20) then
    if (mode == 1) then
      open (u, file=trim(upath(u)), access='direct', form='unformatted', recl=640, &
            status='old', action='read')
    else
      open (u, file=trim(upath(u)), access='direct', form='unformatted', recl=640, &
            status='replace')
    end if
  else if (formatted) then
    open (u, file=trim(upath(u)), form='formatted', status='old', action='read')
  else if (mode == 1) then
    open (u, file=trim(upath(u)), form='unformatted', status='old', action='read')
  else
    open (u, file=trim(upath(u)), form='unformatted', status='replace')
  end if
end subroutine bchang

! INQUIRE(U,PRESENT=THERE)
subroutine bpres(u, there)
  use brts
  integer :: u
  logical :: there
  inquire (file=trim(upath(u)), exist=there)
end subroutine bpres

! CLOSE(U,DISP=KEEP/CRUNCH), LOCK(U) and CLOSE(U,DISP=DELETE).  A file the
! B7700 closes stays declared, and the next READ opens it again: the game
! closes its data base (unit 20) once it has built it and goes on reading
! it, so a kept unit 20 is opened again at once
subroutine bclose(u, del)
  use brts
  integer :: u, del
  logical :: op, ex
  inquire (unit=u, opened=op)
  if (op) then
    if (del /= 0) then
      close (u, status='delete')
    else
      close (u)
      if (u == 20) open (u, file=trim(upath(u)), access='direct', form='unformatted', &
                         recl=640, status='old', action='read')
    end if
  else if (del /= 0 .and. upath(u) /= '') then
    inquire (file=trim(upath(u)), exist=ex)
    if (ex) then
      open (u, file=trim(upath(u)), status='old')
      close (u, status='delete')
    end if
  end if
end subroutine bclose

! ---- the clock and RANDOM --------------------------------------------------

! TIME(1): the time of day in sixtieths of a second (--fixed-clock: noon,
! and a second more for every line typed)
integer function btick()
  use brts
  integer :: v(8)
  if (fixclk) then
    btick = (43200 + nlines) * 60
  else
    call date_and_time(values=v)
    btick = ((v(5) * 60 + v(6)) * 60 + v(7)) * 60 + v(8) * 60 / 1000
  end if
end function btick

! ITIME's hour, minute and second (TIME(7) on the B7700)
subroutine btime(h, m, s)
  use brts
  integer :: h, m, s, v(8), t
  if (fixclk) then
    t = 43200 + nlines
    h = mod(t / 3600, 24)
    m = mod(t / 60, 60)
    s = mod(t, 60)
  else
    call date_and_time(values=v)
    h = v(5)
    m = v(6)
    s = v(7)
  end if
end subroutine btime

! RANDOM(SEED): a number from 0 up to 1, and a new seed.  A stand-in: the
! B7700's generator is not known here; RND hands it TIME(1) every time.
real function brand(seed)
  real :: seed
  integer :: s
  s = mod(int(abs(seed), 8), 2147483648_8)
  s = mod(s * 1103515245_8 + 12345_8, 2147483648_8)
  seed = real(s)
  brand = real(s) / 2147483648.0
end function brand

! ---- words -----------------------------------------------------------------

! R50CNV's CONCAT(0,A,6,45,7).EQ.0 asks whether A is a number (its exponent
! field is zero - -1, the end of a list, too) or characters: here a word of
! characters, blank-filled, is far larger than any number of the game
integer function kstr(a)
  integer :: a
  if (abs(a) > 2_8**40) then
    kstr = 1
  else
    kstr = 0
  end if
end function kstr

! CONCAT(" ",OLD,47,48-J,8), J = 1, 9, 17: the first, second or third
! character of OLD alone in a word
integer function kchr(old, j)
  integer :: old, j
  character(len=8) :: c
  c = transfer(old, '12345678')
  kchr = transfer(c((j + 7) / 8:(j + 7) / 8) // '       ', 0)
end function kchr
