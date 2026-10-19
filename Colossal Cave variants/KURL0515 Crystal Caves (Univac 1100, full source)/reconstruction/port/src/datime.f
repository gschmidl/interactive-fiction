      SUBROUTINE DATIME(D,T)

C  RETURN THE DATE AND TIME IN D AND T.  D IS NUMBER OF DAYS SINCE 01-JAN-77,
C  I.E. IF TODAY WAS 01-JAN-77 THIS ROUTINE WOULD RETURN D=0.
C  T IS MINUTES PAST MIDNIGHT.
C
C  Portable rewrite for this port: the original Univac version unpacked
C  a TDATE$ system word (see SHIFT/BITS-based FLD macro in the original
C  source).  This version uses the standard DATE_AND_TIME intrinsic and
C  a Julian-day-number calculation instead, which needs no knowledge of
C  the host's word size or character encoding.

      IMPLICIT INTEGER(A-Z)
      CHARACTER*8 CDATE
      CHARACTER*10 CTIME
      INTEGER JDN

      CALL DATE_AND_TIME(CDATE,CTIME)
      READ(CDATE,'(I4,I2,I2)') YEAR,MONTH,DAY
      READ(CTIME,'(I2,I2)') HOUR,MINUTE

      D = JDN(YEAR,MONTH,DAY) - JDN(1977,1,1)
      T = HOUR*60 + MINUTE

      RETURN
      END


      INTEGER FUNCTION JDN(Y,M,D)
C  Fliegel & Van Flandern Julian Day Number, for Gregorian calendar dates.
      IMPLICIT INTEGER(A-Z)
      A = (14-M)/12
      YY = Y + 4800 - A
      MM = M + 12*A - 3
      JDN = D + (153*MM+2)/5 + 365*YY + YY/4 - YY/100 + YY/400 - 32045
      RETURN
      END
