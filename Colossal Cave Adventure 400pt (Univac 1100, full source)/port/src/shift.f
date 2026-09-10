       INTEGER FUNCTION SHIFT(VAL,DIST)
       IMPLICIT INTEGER(A-Z)

C  RETURN VAL LEFT-SHIFTED (LOGICALLY) DIST BITS (RIGHT-SHIFT IF DIST<0).
C  (Original Univac version masked/rotated within a 36-bit word for use
C  by the WIZARD cipher and DATIME's packed-word decoder; both of those
C  callers were replaced by portable code in this port, so the only
C  remaining uses are small shifts of 0-24 bits for hour-of-day bitmaps,
C  which ISHFT handles directly.)

       SHIFT = ISHFT(VAL,DIST)
       RETURN
       END
