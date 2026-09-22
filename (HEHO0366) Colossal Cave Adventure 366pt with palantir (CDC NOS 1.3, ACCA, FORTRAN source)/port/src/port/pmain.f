C  The port's main program: the options, the database, then the game.
C  It is on its own so that the test drivers in tests\ can link the game
C  with a main of their own.
      PROGRAM PADVENT
      IMPLICIT INTEGER (A-Z)
      CALL POPTS
      CALL PIOINI
      CALL ADVENT
      END
