#!/usr/bin/env python3
"""Generate the portable (gfortran) source adv462.f from ../Multics/adventure_.fortran.
Written 2026-09-27 by Claude Opus 5.5 at the direction of Jim Lippard.

The Multics source is the master copy.  This script applies the small set of
changes needed for gfortran on Unix:
  - code1: take a character*5 argument (the Multics version picks characters out of
    36-bit Hollerith words);
  - and/or/xor: use the gfortran intrinsics (the Multics versions rely on Multics
    Fortran's bitwise logical operators); shift: use ishft;
  - ioinit: open the database with a plain OPEN (path from ADV462_DATA or a default);
  - datime: use DATE_AND_TIME instead of clock_/decode_clock_value_/getime;
  - main program: declare size external (it is an intrinsic in Fortran 90);
  - poof: no prime-time hours by default (the cave is always open);
  - getin: stop quietly at end of input;
  - every unit: declare ran external (it is a GNU intrinsic).
addr, size, ldcomn and svcomn are supplied in C (adv462_util.c).
Build with: gfortran -std=legacy -fdec-char-conversions -fdefault-integer-8
            -ffixed-line-length-none -fno-range-check -w   (see Makefile)
"""
import re, sys, os
here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, '..', 'Multics', 'adventure_.fortran')).read().split('\n')
if src[-1] == '': src.pop()

def unit_bounds(lines, header_re):
    """Return (start, end) indexes of the program unit whose header matches header_re."""
    for i, l in enumerate(lines):
        if re.match(header_re, l):
            for j in range(i, len(lines)):
                if re.match(r'^      end\s*$', lines[j]):
                    return i, j
    raise SystemExit('unit not found: ' + header_re)

def replace_unit(lines, header_re, new):
    s, e = unit_bounds(lines, header_re)
    return lines[:s] + new.strip('\n').split('\n') + lines[e+1:]

def delete_unit(lines, header_re):
    s, e = unit_bounds(lines, header_re)
    return lines[:s] + lines[e+1:]

out = src[:]

# main program: size is a user function, not the F90 intrinsic
i = out.index('      logical dseen,blklin,hinted,yes,start')
out.insert(i + 1, '      external size')

out = replace_unit(out, r'^      integer function code1\(words\)', r'''
      integer function code1(words)
c  convert external characters to internal format (5 chars/integer).
c  unix version: words is a character*5 literal.  same table as code2
c  (index-1 of the character in the table, packed in base 90).
      implicit integer(a-z)
      character*5 words
      character*90 tab
      tab=' !"#$%&''()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz'
     &  //'[\]^_ABCDEFGHIJKLMNOPQRSTUVWXYZ'
      result=0
      do 10 i=1,5
         chridx=index(tab,words(i:i))
         if(chridx.eq.0)chridx=15
         result=result*90+chridx-1
   10 continue
      code1=result
      return
      end
''')

out = replace_unit(out, r'^      subroutine ioinit\(dummy\)', r'''
      subroutine ioinit(dummy)
c  i/o system initialization (unix version).
c  the database is found at $ADV462_DATA, else at the compiled-in default
c  (advdat, in adv462_util.c).
      implicit integer(a-z)
      common /ioscom/ ttyi,ttyo,blklin,dbfi
      character*256 path
      ttyi=5
      ttyo=6
      dbfi=1
      call get_environment_variable('ADV462_DATA',path,status=ios)
      if(ios.ne.0.or.path.eq.' ')call advdat(path)
      close(1,iostat=ios)
      open(unit=1,file=path,status='old',action='read',iostat=ios)
      if(ios.eq.0)return
      write(ttyo,1)path(1:len_trim(path))
    1 format(' adventure: cannot open database ',a)
      stop
      end
''')

out = replace_unit(out, r'^      integer function shift\(val,dist\)', r'''
      integer function shift(val,dist)
      implicit integer(a-z)
c  return val left-shifted (logically) dist bits (right-shift if dist<0).
      shift=ishft(val,dist)
      return
      end
''')
for f in ('and', 'or', 'xor'):
    out = delete_unit(out, r'^      integer function %s\(a,b\)' % f)

out = replace_unit(out, r'^      subroutine datime\(d,t\)', r'''
      subroutine datime(d,t)
c  return the current date and time (unix version).
c  d is set to the number of days since 01/01/77 (a saturday).
c  t is set to the number of minutes past midnight.
      implicit integer(a-z)
      integer*4 v(8)
      dimension days(12)
      data days/31,28,31,30,31,30,31,31,30,31,30,31/
      call date_and_time(values=v)
      year=v(1)
      month=v(2)
      day=v(3)
      d=day-1
      do 10 i=1,12
         if(i.eq.month)goto 11
         d=d+days(i)
   10 continue
   11 d=d+365*(year-1977)+(year-1977)/4
      if(mod(year-1977,4).eq.3.and.month.gt.2)d=d+1
      t=v(5)*60+v(6)
      return
      end
''')

# poof: no prime time.  The 1980 default closed the cave to all but wizards
# from 8:00 to 16:59 on weekdays; a wizard can still set hours in magic mode.
for i, l in enumerate(out):
    if l == '      wkday=261888':
        out[i] = '      wkday=0'
        break
else:
    raise SystemExit('poof wkday not found')

# getin: stop quietly at end of input (Multics gives an error at a real
# terminal, so the original never needed this)
s, e = unit_bounds(out, r'^      subroutine getin\(')
for i in range(s, e):
    if out[i] == '    2 read(ttyi,3)line':
        out[i] = '    2 read(ttyi,3,end=9998)line'
        break
else:
    raise SystemExit('getin read not found')
out[e:e] = [' 9998 stop']

# ran is a GNU intrinsic: make every unit (except ran itself) use ours
res=[]; inran=False
for l in out:
    if re.match(r'^      integer function ran\(range\)', l): inran=True
    elif re.match(r'^      (subroutine|integer function|logical function)', l): inran=False
    res.append(l)
    if re.match(r'^      implicit integer\(a-z\)\s*$', l) and not inran:
        res.append('      external ran')
out=res

open(os.path.join(here, 'adv462.f'), 'w').write('\n'.join(out) + '\n')
print('wrote adv462.f,', len(out), 'lines')
