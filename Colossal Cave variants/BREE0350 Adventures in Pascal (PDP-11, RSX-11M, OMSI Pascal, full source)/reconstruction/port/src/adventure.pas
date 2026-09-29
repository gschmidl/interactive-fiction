{ Adventures in Pascal - the game.  ADVGBL + every game module + MAIN, which
  is how the OMSI task was built (PAS <module>=ADVGBL,<module>/E, then TKB). }
{$mode tp}
program adventure;
uses omsirt;
{$I glue.inc}
{$I advgbl.inc}
{$I forwards.inc}
{$I wizhint.inc}
{$I modules.inc}
{$I debug.inc}
{$I main.inc}
