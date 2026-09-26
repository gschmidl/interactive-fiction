C  DARKRM/DARK were zero-argument "define" macros in the original
C  Univac source (see comment in main.f).  Standard Fortran statement
C  functions need real dummy-argument parens, so these are ordinary
C  external functions instead, reading the shared game state via
C  gamecom/compla.  Callers now say DARKRM()/DARK().

      LOGICAL FUNCTION DARKRM()
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'gamecom.fi'
      include 'compla.fi'

      DARKRM = MOD(COND(LOC),2).EQ.0 .AND.
     1         (PROP(LAMP).EQ.0 .OR.
     2          .NOT.(PLACE(LAMP).EQ.LOC .OR. PLACE(LAMP).EQ.-1))
      RETURN
      END


      LOGICAL FUNCTION DARK()
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'gamecom.fi'
      include 'compla.fi'
      LOGICAL DARKRM, HEREUN

      HEREUN = PLACE(UNICRN).EQ.LOC .OR. PLACE(UNICRN).EQ.-1
      DARK = DARKRM() .AND. (.NOT.HEREUN .OR. PROP(UNICRN).EQ.0)
      RETURN
      END
