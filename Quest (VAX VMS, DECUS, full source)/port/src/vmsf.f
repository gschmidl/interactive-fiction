*****
*
*  vmsf.f -- the three helpers the port needs that were not routines on
*  the VAX at all.  Everything else in port/src is the DECUS source, and
*  is regenerated from it by tools/build_sources.sh; this file is not.
*
*****
*
*  QCLEAR marks an internal-file array as unwritten.  A FORTRAN WRITE to a
*  character array writes one record per element and leaves the elements
*  past the last record alone, so a NUL in column 1 says "this record was
*  never written".  No record QUEST writes can start with one: column 1 is
*  the carriage control character.
*
*****

      SUBROUTINE QCLEAR(BUF,N)
      CHARACTER BUF(*)*(*)
      INTEGER N,I

      DO I=1,N
        BUF(I)=CHAR(0)
      ENDDO
      RETURN
      END



*****
*
*  QEMIT writes out the records QCLEAR/WRITE just produced, each with the
*  carriage control it carries in column 1.
*
*****

      SUBROUTINE QEMIT(BUF,N)
      CHARACTER BUF(*)*(*)
      INTEGER N,I,J,LENGTH

      DO I=1,N
        IF(BUF(I)(1:1).EQ.CHAR(0))RETURN
        J=LENGTH(BUF(I))
        IF(J.LT.2)THEN
          CALL TTYNL(1)
        ELSE
          CALL TTYREC(ICHAR(BUF(I)(1:1)),BUF(I)(2:J))
          ENDIF
      ENDDO
      RETURN
      END



*****
*
*  SCRINF stands in for LIB$SCREEN_INFO, which was not part of the DECUS
*  distribution -- it came out of LIBRARY.OLB, whose source did not
*  survive.  CREATE uses only the second value, to choose between the ANSI
*  and the VT52 form of "erase to end of line":
*
*      CALL SINGLE(27)                        ESC
*      IF(B.EQ.96)CALL SINGLE(ICHAR('['))     [
*      CALL SINGLE(75)                        K
*
*  96 is TT$_VT100, so this reports a VT100, which is what the port drives.
*
*****

      SUBROUTINE SCRINF(A,B,C,D)
      INTEGER A,B,C,D

      A=24
      B=96
      C=80
      D=0
      RETURN
      END
