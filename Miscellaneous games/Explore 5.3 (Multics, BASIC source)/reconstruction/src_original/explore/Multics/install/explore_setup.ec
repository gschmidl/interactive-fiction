&version 2
&-
&- HISTORY COMMENTS:
&-  1) change(2026-mm-dd,Lippard), approve(2026-mm-dd,XXXnnn):
&-     Initial setup exec_com for the Explore game (AML component).
&-     Installs explore_dir (read-only base data) into a specified
&-     directory, and for a "standard" install also sets up the writable
&-     data and multiplayer files (left READ-ONLY; the admin grants rw
&-     explicitly to enable multiplayer).  Ring brackets/gates are NOT
&-     needed (ACLs only).
&-                                                      END HISTORY COMMENTS
&-
&- Usage:  ec explore_setup TYPE DIR [RW_DIR]
&-
&-   TYPE   read-only (or ro): install ONLY the read-only base files;
&-                             no writable dir, no explore.rwdir, no
&-                             multiplayer files.  The game runs read-only
&-                             (no saved wins, no multiplayer).
&-          standard         : base files (always read-only) PLUS the
&-                             writable working files and the multiplayer
&-                             registry/comms/lock files.  Everything is
&-                             created READ-ONLY; nothing is world-writable.
&-                             The ec prints the sa commands to grant rw
&-                             to players/sorcerer if you want writes and
&-                             multiplayer.
&-
&-   DIR    the directory the explore_dir is installed INTO.  Give ">site"
&-          for a normal install, or any other EXISTING directory to
&-          install elsewhere (e.g. a throwaway test dir).  DIR must
&-          already exist; this ec creates DIR>explore_dir but never
&-          creates DIR itself (it aborts if DIR is missing).
&-
&-   RW_DIR (standard only, optional) where the writable files live.
&-          Default is DIR>explore_dir>private.
&-
&- ASSUMPTIONS:
&-   * The library maintainer has already installed bound_explore_, its
&-     archives, and the info segment(s) into >aml / >ldd>aml.  This ec
&-     installs only the runtime data (explore_dir).
&-   * The four base data files must be in the working directory when this
&-     ec runs:  explore.data explore.help hours.data winners.data
&-
&trace &command on
&goto entry.&ec_name
&-
&label entry.explore_setup
&-
&set SAVED_WDIR &[pwd]
&-
&- ---- argument 1: install TYPE ----
&if &[not [exists argument &1]] &then &goto USAGE
&if &[equal &1 "-help"]  &then &goto USAGE
&if &[equal &1 "-usage"] &then &goto USAGE
&if &[equal &1 "read-only"] &then &set TYPE ro
&else &if &[equal &1 "ro"] &then &set TYPE ro
&else &if &[equal &1 "standard"] &then &set TYPE standard
&else &do
  &print &ec_name: unknown install type "&1" (use read-only, ro, or standard).
  &goto USAGE
&end
&-
&- ---- argument 2: DIR (where explore_dir is installed) ----
&if &[not [exists argument &2]] &then &do
  &print &ec_name: missing DIR argument (e.g. >site).
  &goto USAGE
&end
&set DIR &[path &2]
&if &[not [exists dir &(DIR)]] &then &do
  &print &ec_name: the target directory &(DIR) does not exist.
  &print Create it (or specify an existing one such as >site) and re-run.
  &goto QUIT
&end
&-
&set GAMEDIR &(DIR)>explore_dir
&-
&- ---- argument 3: RW_DIR (standard only; default GAMEDIR>private) ----
&- A read-only install has no writable dir, so RW_DIR is meaningless there;
&- warn (but continue) if one was given, in case the user meant "standard".
&if &[exists argument &3] &then &do
  &if &[equal &(TYPE) ro] &then &print &ec_name: note -- RW_DIR "&3" is ignored for a read-only (ro) install.
  &set RWDIR &[path &3]
&end
&else &set RWDIR &(GAMEDIR)>private
&-
&- ==================================================================
&- 1. Read-only game directory:  DIR>explore_dir  (all install types)
&-    Holds the read-only base data.  World-readable, admin-writable.
&- ==================================================================
&if &[not [exists dir &(GAMEDIR)]] &then create_dir &(GAMEDIR)
&-
&-   Verify the source data files are present; abort cleanly if missing.
&if &[not [exists component &ec_dir>explore_data::explore.data]]  &then &goto NOSRC
&if &[not [exists component &ec_dir>explore_data::explore.help]]  &then &goto NOSRC
&if &[not [exists component &ec_dir>explore_data::hours.data]]     &then &goto NOSRC
&if &[not [exists component &ec_dir>explore_data::winners.data]]   &then &goto NOSRC
&- Base data files are ALWAYS read-only (pristine originals).
&set CURDIR &[pwd]
cwd &(GAMEDIR)
ac x &ec_dir>explore_data explore.data
ac x &ec_dir>explore_data explore.help
ac x &ec_dir>explore_data hours.data
ac x &ec_dir>explore_data winners.data
cwd &(CURDIR)
sa &(GAMEDIR)>explore.data r * -rp
sa &(GAMEDIR)>explore.help r * -rp
sa &(GAMEDIR)>hours.data   r * -rp
sa &(GAMEDIR)>winners.data r * -rp
&-
&- ---- read-only install stops here: no rwdir, no writable files ----
&if &[equal &(TYPE) ro] &then &do
  sa &(GAMEDIR) sma *.SysAdmin s * -rp
  &goto RO_DONE
&end  
  
&-
&- ==================================================================
&- 2. Read-write area:  <RWDIR>  (standard install only)
&-    Working files + multiplayer files.  Created READ-ONLY here; the
&-    admin grants rw explicitly (see the printed message) to enable
&-    writes / multiplayer.  Directory itself is owner-only.
&- ==================================================================
&if &[not [exists dir &(RWDIR)]] &then create_dir &(RWDIR)
&-
&- Working hours.data and winners.data (copies of the base).  IDEMPOTENT:
&- only create if absent, so a re-run does NOT wipe live edits/winners.
&if &[not [exists segment &(RWDIR)>hours.data]]   &then copy &(GAMEDIR)>hours.data   &(RWDIR)>hours.data
&if &[not [exists segment &(RWDIR)>winners.data]] &then copy &(GAMEDIR)>winners.data &(RWDIR)>winners.data
sml &(RWDIR)>hours.data   1024
sml &(RWDIR)>winners.data 4096
&-
&- Multiplayer registry / comms / lock files (created empty if absent).
&if &[not [exists segment &(RWDIR)>explore_usr]]            &then cr &(RWDIR)>explore_usr
&if &[not [exists segment &(RWDIR)>explore_usr.lock]]       &then cr &(RWDIR)>explore_usr.lock
&if &[not [exists segment &(RWDIR)>explore_usr_com]]        &then cr &(RWDIR)>explore_usr_com
&if &[not [exists segment &(RWDIR)>explore_usr_com.lock]]   &then cr &(RWDIR)>explore_usr_com.lock
&if &[not [exists segment &(RWDIR)>explore_com]]            &then cr &(RWDIR)>explore_com
&if &[not [exists segment &(RWDIR)>explore_com.lock]]       &then cr &(RWDIR)>explore_com.lock
sml &(RWDIR)>explore_usr          1024
sml &(RWDIR)>explore_usr.lock     1024
sml &(RWDIR)>explore_usr_com      1024
sml &(RWDIR)>explore_usr_com.lock 1024
sml &(RWDIR)>explore_com          1024
sml &(RWDIR)>explore_com.lock     1024
&-
&- All writable/multiplayer files are left READ-ONLY (r *).  The admin
&- enables writes/multiplayer by granting rw (see the printed message).
sa &(RWDIR)>hours.data            r * -rp
sa &(RWDIR)>winners.data          r * -rp
sa &(RWDIR)>explore_usr           r * -rp
sa &(RWDIR)>explore_usr.lock      r * -rp
sa &(RWDIR)>explore_usr_com       r * -rp
sa &(RWDIR)>explore_usr_com.lock  r * -rp
sa &(RWDIR)>explore_com           r * -rp
sa &(RWDIR)>explore_com.lock      r * -rp
sa &(RWDIR) sma *.SysAdmin s * -rp
&-
&- ==================================================================
&- 3. explore.rwdir pointer (standard install).
&-    Line 1 = the read-write directory.  Line 2 = "^multip" (multiplayer
&-    NOT permitted until the admin both grants rw AND changes this to
&-    "multip").  We write ^multip by default.
&- ==================================================================
&set MP ^multip
&-  Write the two-line explore.rwdir with qedx.  &attach (BEFORE invoking
&-  qedx) directs the following exec_com lines into qedx's input with
&-  &-substitution; we append the two lines, write, and quit.  Append is
&-  ended by the \f request on a line by itself.
&attach
qedx
a
&(RWDIR)
&(MP)
\f
w &(GAMEDIR)>explore.rwdir
q
&detach
sa &(GAMEDIR)>explore.rwdir r * -rp
sa &(GAMEDIR) sma *.SysAdmin s * -rp
&-
&- ==================================================================
&- Done (standard).
&- ==================================================================
&print Explore STANDARD setup complete.
&print   game directory (read-only) : &(GAMEDIR)
&print   read-write directory       : &(RWDIR)
&print   explore.rwdir points at     : &(RWDIR)  (line 2 = ^multip)
&print
&print All writable files were created READ-ONLY.  Grant rw as follows
&print (replace <players> with your player/sorcerer principals):
&print
&print For STANDARD single-player play (record wins, sorcerer edits hours):
&print   sa &(RWDIR)>winners.data          rw <players>
&print   sa &(RWDIR)>hours.data            rw <players>
&print For better security, hours.data can be restricted to sorcerer only;
&print   name of last winner in hours.data from winners.data using "hours"
&print   sorcerer can update with sorcerer "hous" command in game.
&print
&print For MULTIPLAYER (in addition to the two above):
&print   sa &(RWDIR)>explore_usr           rw <players>
&print   sa &(RWDIR)>explore_usr.lock      rw <players>
&print   sa &(RWDIR)>explore_usr_com       rw <players>
&print   sa &(RWDIR)>explore_usr_com.lock  rw <players>
&print   sa &(RWDIR)>explore_com           rw <players>
&print   sa &(RWDIR)>explore_com.lock      rw <players>
&print   ...and change explore.rwdir line 2 from "^multip" to "multip".
&print
&print Before real play, edit hours.data so line 1 is the rot13 of your
&print chosen sorcerer word.
&goto QUIT
&-
&label RO_DONE
&print Explore READ-ONLY setup complete.
&print   game directory (read-only) : &(GAMEDIR)
&print No writable directory, no explore.rwdir, no multiplayer files were
&print created.  The game runs read-only (no saved wins, no multiplayer).
&print Players run the game by typing:  explore
&goto QUIT
&-
&label USAGE
&print Usage:  ec explore_setup TYPE DIR [RW_DIR]
&print
&print   TYPE    read-only (or ro) : base files only, entirely read-only.
&print           standard          : base files + writable + multiplayer
&print                               files (created read-only; grant rw to
&print                               enable writes/multiplayer).
&print   DIR     the EXISTING directory to install explore_dir into
&print           (e.g. >site, or a test directory).  Not created; must
&print           already exist.
&print   RW_DIR  (standard, optional) writable-files dir; default
&print           DIR>explore_dir>private.
&print
&print The four data files (explore.data explore.help hours.data
&print winners.data) must be in the working directory when you run this.
&print Install bound_explore_ into >aml separately first.
&goto QUIT
&-
&label NOSRC
&print &ec_name: one or more source data files (explore.data explore.help
&print hours.data winners.data) are not in the working directory
&print (&(SAVED_WDIR)).  cwd to where they are (or copy them here) and re-run.
&goto QUIT
&-
&label QUIT
&if &[exists argument &(SAVED_WDIR)] &then cwd &(SAVED_WDIR)
&quit
