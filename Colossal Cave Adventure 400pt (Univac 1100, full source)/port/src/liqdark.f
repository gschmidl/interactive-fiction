C  LIQ/DARK were zero-argument "define" macros in the original Univac
C  source (see comment in main.f).  Standard Fortran statement
C  functions need real dummy-argument parens, so these are ordinary
C  external functions instead, reading the shared game state via
C  gamecom/compla.  Callers now say LIQ()/DARK().  (main.f's own
C  LIQ2/LIQLOC statement functions -- which do take a real argument --
C  are untouched; LIQ below just inlines the same LIQ2 formula rather
C  than duplicating it as a same-named external function.)

      INTEGER FUNCTION LIQ()
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'gamecom.fi'
      PBOTL = MAX0(PROP(BOTTLE),-1-PROP(BOTTLE))

      LIQ = (1-PBOTL)*WATER+(PBOTL/2)*(WATER+OIL)
      RETURN
      END


      LOGICAL FUNCTION DARK()
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'gamecom.fi'
      include 'compla.fi'
      LOGICAL HEREUN

      HEREUN = PLACE(LAMP).EQ.LOC .OR. PLACE(LAMP).EQ.-1
      DARK = MOD(COND(LOC),2).EQ.0 .AND.
     1       (PROP(LAMP).EQ.0 .OR. .NOT.HEREUN)
      RETURN
      END
