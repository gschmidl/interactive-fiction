# Dungeon (Zork) - HP 1000 RTE FORTRAN, HP Contributed Software Library

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\HP\HP_1000_software_collection\specials\CSL-1000_Rev-2213 / 2240 / 2830.zip` (all eleven CSL-1000 zips
are in `..\_HP1000_work\CSL-1000\`). Each zip holds one tape image (`.tf.tape` = TF transfer format, `.fmgr.tape` = FMGR).

Rev-2213 tape: FORTRAN routines `RDOAL RDOBJS RDCLOK RDADVS ITIME R50CNV` at ~3833000, text records from ~3850000 in the
form `( (14,You are in the kitchen of the white house. ...`, `( (66,You are standing on the top of flood control dam #3`.
Rev-2240 has the same text at ~3464000; Rev-2830 has 42 "zork" hits. Program names / file list not yet extracted -
no TF/FMGR tape reader written (the 385/425 Adventure ports came from disc images, see reference_rte6vm_disc_layout).
Same source-port route as the HP 1000 Adventures (FTN4 -> gfortran).
