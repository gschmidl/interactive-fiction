C  The port's main program: the options, then the game.  It is on its own
C  so that a test driver can link the game with a main of its own.
      PROGRAM PADVENT
      IMPLICIT INTEGER (A-Z)
      CALL POPTS
      CALL ADVENT
      END
