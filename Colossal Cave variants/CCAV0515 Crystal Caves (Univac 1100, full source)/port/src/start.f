       LOGICAL FUNCTION START(class)

C  Originally checked whether it was "prime time" on the shared Univac
C  computer (only wizards could play then, others got a short demo
C  game), and enforced a minimum wait between SUSPEND and RESTART so
C  players couldn't save-scum a risky move on a shared, metered system.
C  Neither restriction makes sense for this standalone single-player
C  port, so play always proceeds immediately and is never shortened to
C  a demo -- the same design choice made for the sibling Adventure
C  port's login gate.

       IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'

       SAVED=-1
       START=.FALSE.
       RETURN
       END
