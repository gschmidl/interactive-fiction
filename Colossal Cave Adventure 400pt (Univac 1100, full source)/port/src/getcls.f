      SUBROUTINE GETCLS(CLASS)
C
C  Replacement for the original Univac 1100 MASM routine, which asked
C  the EXEC 8 accounting system (ER PCT$/MCT$/PROP$) whether the current
C  user was an ISD customer (class 1) or in-house staff (class 2), for
C  billing-log and prime-time-hours purposes.  This port is a standalone
C  single-player game with no accounting system behind it, so everyone
C  is simply a class-1 (customer) adventurer.
C
      IMPLICIT INTEGER (A-Z)
      CLASS = 1
      RETURN
      END
