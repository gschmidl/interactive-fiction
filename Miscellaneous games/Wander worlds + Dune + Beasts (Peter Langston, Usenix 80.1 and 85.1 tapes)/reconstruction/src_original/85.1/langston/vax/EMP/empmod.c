#define D_COMSTR
#define D_NATSTAT
#include    "empdef.h"
/*
**      Header for emp modules     (c) psl 1981
**      This file makes emp1.o, emp2.o, etc.
*/

int     thisprog    = EMPMOD;

#ifdef  NOSEP
#define  addMOD 7
#define anneMOD 7
#define assaMOD 3
#define attaMOD 3
#define boarMOD 3
#define builMOD 4
#define censMOD 1
#define chanMOD 2
#define checMOD 2
#define coasMOD 4
#define collMOD 5
#define commMOD 1
#define consMOD 5
#define contMOD 5
#define counMOD 1
#define declMOD 2
#define deliMOD 1
#define desiMOD 1
#define dissMOD 7
#define enliMOD 2
#define fireMOD 3
#define fleeMOD 4
#define  flyMOD 7
#define granMOD 2
#define infoMOD 1
#define ledgMOD 5
#define loadMOD 4
#define lookMOD 4
#define  mapMOD 1
#define mineMOD 4
#define moveMOD 1
#define natiMOD 1
#define naviMOD 4
#define newsMOD 6
#define nukeMOD 3
#define offeMOD 5
#define offsMOD 7
#define poweMOD 6
#define radaMOD 4
#define  reaMOD 6
#define realMOD 1
#define repaMOD 5
#define routMOD 1
#define  setMOD 5
#define  shiMOD 4
#define  spyMOD 2
#define teleMOD 6
#define tendMOD 4
#define torpMOD 3
#define tradMOD 5
#define tranMOD 3
#define treaMOD 2
#define turnMOD 7
#define updaMOD 1
#define versMOD 7
#define weatMOD 1
#endif

#ifdef  SEP
#define  addMOD 3
#define anneMOD 2
#define assaMOD 2
#define attaMOD 2
#define boarMOD 2
#define builMOD 1
#define censMOD 1
#define chanMOD 3
#define checMOD 2
#define coasMOD 2
#define collMOD 2
#define commMOD 1
#define consMOD 2
#define contMOD 1
#define counMOD 1
#define declMOD 2
#define deliMOD 1
#define desiMOD 1
#define dissMOD 3
#define enliMOD 2
#define fireMOD 2
#define fleeMOD 1
#define  flyMOD 2
#define granMOD 2
#define infoMOD 1
#define ledgMOD 2
#define loadMOD 1
#define lookMOD 1
#define  mapMOD 1
#define mineMOD 2
#define moveMOD 1
#define natiMOD 1
#define naviMOD 1
#define newsMOD 1
#define nukeMOD 2
#define offeMOD 2
#define offsMOD 3
#define poweMOD 1
#define radaMOD 1
#define  reaMOD 1
#define realMOD 1
#define repaMOD 2
#define routMOD 1
#define  setMOD 2
#define  shiMOD 1
#define  spyMOD 2
#define teleMOD 1
#define tendMOD 2
#define torpMOD 2
#define tradMOD 2
#define tranMOD 1
#define treaMOD 1
#define turnMOD 3
#define updaMOD 1
#define versMOD 3
#define weatMOD 1
#endif

#ifdef  VAX
#define  addMOD 1
#define anneMOD 1
#define assaMOD 1
#define attaMOD 1
#define boarMOD 1
#define builMOD 1
#define censMOD 1
#define chanMOD 1
#define checMOD 1
#define coasMOD 1
#define collMOD 1
#define commMOD 1
#define consMOD 1
#define contMOD 1
#define counMOD 1
#define declMOD 1
#define deliMOD 1
#define desiMOD 1
#define dissMOD 1
#define enliMOD 1
#define fireMOD 1
#define fleeMOD 1
#define  flyMOD 1
#define granMOD 1
#define infoMOD 1
#define ledgMOD 1
#define loadMOD 1
#define lookMOD 1
#define  mapMOD 1
#define mineMOD 1
#define moveMOD 1
#define natiMOD 1
#define naviMOD 1
#define newsMOD 1
#define nukeMOD 1
#define offeMOD 1
#define offsMOD 1
#define poweMOD 1
#define radaMOD 1
#define  reaMOD 1
#define realMOD 1
#define repaMOD 1
#define routMOD 1
#define  setMOD 1
#define  shiMOD 1
#define  spyMOD 1
#define teleMOD 1
#define tendMOD 1
#define torpMOD 1
#define tradMOD 1
#define tranMOD 1
#define treaMOD 1
#define turnMOD 1
#define updaMOD 1
#define versMOD 1
#define weatMOD 1
#endif

#define IGN     ((int (*) ()) 0)   /* dummy address spec */

#if  addMOD == EMPMOD
extern	int  add();
#else
#define  add IGN
#endif
#if anneMOD == EMPMOD
extern  int anne();
#else
#define anne IGN
#endif
#if assaMOD == EMPMOD
extern	int assa();
#else
#define assa IGN
#endif
#if attaMOD == EMPMOD
extern	int atta();
#else
#define atta IGN
#endif
#if boarMOD == EMPMOD
extern	int boar();
#else
#define boar IGN
#endif
#if builMOD == EMPMOD
extern	int buil();
#else
#define buil IGN
#endif
#if censMOD == EMPMOD
extern	int cens();
#else
#define cens IGN
#endif
#if chanMOD == EMPMOD
extern	int chan();
#else
#define chan IGN
#endif
#if checMOD == EMPMOD
extern	int chec();
#else
#define chec IGN
#endif
#if coasMOD == EMPMOD
extern	int coas();
#else
#define coas IGN
#endif
#if collMOD == EMPMOD
extern	int coll();
#else
#define coll IGN
#endif
#if commMOD == EMPMOD
extern	int comm();
#else
#define comm IGN
#endif
#if consMOD == EMPMOD
extern	int cons();
#else
#define cons IGN
#endif
#if contMOD == EMPMOD
extern	int cont();
#else
#define cont IGN
#endif
#if counMOD == EMPMOD
extern	int coun();
#else
#define coun IGN
#endif
#if declMOD == EMPMOD
extern	int decl();
#else
#define decl IGN
#endif
#if deliMOD == EMPMOD
extern	int deli();
#else
#define deli IGN
#endif
#if desiMOD == EMPMOD
extern	int desi();
#else
#define desi IGN
#endif
#if dissMOD == EMPMOD
extern	int diss();
#else
#define diss IGN
#endif
#if enliMOD == EMPMOD
extern	int enli();
#else
#define enli IGN
#endif
#if fireMOD == EMPMOD
extern	int fire();
#else
#define fire IGN
#endif
#if fleeMOD == EMPMOD
extern	int flee();
#else
#define flee IGN
#endif
#if  flyMOD == EMPMOD
extern	int  fly();
#else
#define  fly IGN
#endif
#if granMOD == EMPMOD
extern	int gran();
#else
#define gran IGN
#endif
#if infoMOD == EMPMOD
extern	int info();
#else
#define info IGN
#endif
#if ledgMOD == EMPMOD
extern	int ledg();
#else
#define ledg IGN
#endif
#if loadMOD == EMPMOD
extern	int load();
#else
#define load IGN
#endif
#if lookMOD == EMPMOD
extern	int look();
#else
#define look IGN
#endif
#if  mapMOD == EMPMOD
extern	int  map();
#else
#define  map IGN
#endif
#if mineMOD == EMPMOD
extern	int mine();
#else
#define mine IGN
#endif
#if moveMOD == EMPMOD
extern  int move();
#else
#define move IGN
#endif
#if natiMOD == EMPMOD
extern	int nati();
#else
#define nati IGN
#endif
#if naviMOD == EMPMOD
extern	int navi();
#else
#define navi IGN
#endif
#if newsMOD == EMPMOD
extern  int head(), news();
#else
#define head IGN
#define news IGN
#endif
#if nukeMOD == EMPMOD
extern  int nuke();
#else
#define nuke IGN
#endif
#if offeMOD == EMPMOD
extern	int offe();
#else
#define offe IGN
#endif
#if offsMOD == EMPMOD
extern  int offs();
#else
#define offs IGN
#endif
#if poweMOD == EMPMOD
extern	int powe();
#else
#define powe IGN
#endif
#if radaMOD == EMPMOD
extern	int rada();
#else
#define rada IGN
#endif
#if  reaMOD == EMPMOD
extern	int  rea();
#else
#define  rea IGN
#endif
#if realMOD == EMPMOD
extern	int real();
#else
#define real IGN
#endif
#if repaMOD == EMPMOD
extern	int repa();
#else
#define repa IGN
#endif
#if routMOD == EMPMOD
extern	int rout();
#else
#define rout IGN
#endif
#if  setMOD == EMPMOD
extern	int  set();
#else
#define  set IGN
#endif
#if  shiMOD == EMPMOD
extern	int  shi();
#else
#define  shi IGN
#endif
#if  spyMOD == EMPMOD
extern	int  spy();
#else
#define  spy IGN
#endif
#if teleMOD == EMPMOD
extern	int tele();
#else
#define tele IGN
#endif
#if tendMOD == EMPMOD
extern	int tend();
#else
#define tend IGN
#endif
#if torpMOD == EMPMOD
extern	int torp();
#else
#define torp IGN
#endif
#if tradMOD == EMPMOD
extern	int trad();
#else
#define trad IGN
#endif
#if tranMOD == EMPMOD
extern  int tran();
#else
#define tran IGN
#endif
#if treaMOD == EMPMOD
extern	int trea();
#else
#define trea IGN
#endif
#if turnMOD == EMPMOD
extern	int turn();
#else
#define turn IGN
#endif
#if updaMOD == EMPMOD
extern	int upda();
#else
#define upda IGN
#endif
#if versMOD == EMPMOD
extern	int vers();
#else
#define vers IGN
#endif
#if weatMOD == EMPMOD
extern  int weat(), fore();
#else
#define weat IGN
#define fore IGN
#endif


struct  comstr  coms[]  = {
/* command form                               prog# cost  addr   permit */
"add {a new country}",                       addMOD, 0,   add,   GOD,
"annex <SECTS> {from} <CNUM/CNAME>",        anneMOD, 2,   anne,  NORM+MONEY+CAP,
"announce",				    teleMOD, 2,   tele,  NORM,
"assault <SECT> {from ship}",               assaMOD, 2,   assa,  NORM+MONEY,
"attack <SECT> {from sect}",                attaMOD, 2,   atta,  NORM+MONEY+CAP,
"board <SHIP> {from ship}",                 boarMOD, 2,   boar,  NORM+MONEY,
"build {ships in} <SECTS> [type]",          builMOD, 2,   buil,  NORM+MONEY+CAP,
"bye {log-off}",                               QUIT, 0,   IGN,   VIS,
"census {on} <SECTS>",                      censMOD, 0,   cens,  NORM,
"change country|representative|user",       chanMOD, 0,   chan,  NORM,
"checkpoint <SECTS>",                       checMOD, 2,   chec,  NORM+CAP,
"coastwatch {from} <SECTS>",                coasMOD, 2,   coas,  NORM,
"collect {on} <LOAN>",                      collMOD, 2,   coll,  NORM+CAP,
"commodity {report} <SECTS>",		    commMOD, 0,   comm,  NORM,
"consider loan|treaty <NUM>",               consMOD, 2,   cons,  NORM+CAP,
"contract {to sell} <ITEM> {in} <SECTS>",   contMOD, 2,   cont,  NORM+CAP,
"country roster",                           counMOD, 0,   coun,  VIS,
"declare ally|neutral|war <CNUM/CNAME>",    declMOD, 2,   decl,  NORM,
"deliver <ITEM> <SECTS> [[+|-]thresh]",     deliMOD, 1,   deli,  NORM,
"designate <SECTS>",                        desiMOD, 1,   desi,  NORM,
"detonate {nuclear device at} <SECTS>",     nukeMOD, 2,   nuke,  NORM+MONEY+CAP,
"dissolve {government}",                    dissMOD, 0,   diss,  NORM,
"enlist {in} <SECTS>",                      enliMOD, 2,   enli,  NORM+MONEY+CAP,
"execute {commands from} file",                EXEC, 0,   IGN,   NORM,
"fail-safe {set code & loc} <SECTS>",       nukeMOD, 2,   nuke,  NORM+CAP,
"fire {on} <SECT> | <SHIP>",                fireMOD, 2,   fire,  NORM+MONEY,
"fixup {nations, sectors, etc}",                FIX, 0,   IGN,   GOD,
"fleetadd <FLEET> <SHIP/FLEET>",            fleeMOD, 1,   flee,  NORM,
"fly {mission from} <SECT> | <SHIP>",        flyMOD, 2,   fly,   NORM+MONEY,
"forecast {weather in} <SECTS>",            weatMOD, 1,   fore,  NORM,
"grant <SECTS> {to country}",               granMOD, 1,   gran,  NORM,
"headlines [days] {from \"news\"}",         newsMOD, 0,   head,  VIS,
"info {on} <topic>",                        infoMOD, 0,   info,  VIS,
"ledger {loan report}",                     ledgMOD, 0,   ledg,  NORM,
"list of commands",                           CLIST, 0,   IGN,   VIS,
"load <SHIP/FLEET>",                        loadMOD, 2,   load,  NORM,
"lookout {from} <SHIP/FLEET>",              lookMOD, 2,   look,  NORM,
"map {from} <SECT>",                         mapMOD, 0,   map,   NORM,
"mine {from} <SHIP>",                       mineMOD, 2,   mine,  NORM+MONEY,
"move c|m|s|g|p|i|o|b|f",                   moveMOD, 2,   move,  NORM+CAP,
"nation {report}",                          natiMOD, 0,   nati,  NORM,
"navigate <SHIP/FLEET>",                    naviMOD, 2,   navi,  NORM,
"newspaper [days]",                         newsMOD, 0,   news,  VIS,
"nuke {report} <SECTS>",                    nukeMOD, 2,   nuke,  NORM+CAP,
"offer loan|treaty <CNUM/CNAME>",           offeMOD, 2,   offe,  NORM,
"offset {capital} <x> <y>",                 offsMOD, 0,   offs,  GOD,
"power report",                             poweMOD, 0,   powe,  VIS,
"quit {end a session}",                        QUIT, 0,   IGN,   VIS,
"radar <SHIP/FLEET> | <SECTS>",             radaMOD, 2,   rada,  NORM,
"read telegrams",                            reaMOD, 0,   rea,   VIS,
"realm <number> [<SECTS>]",                 realMOD, 0,   real,  NORM,
"repay <LOAN>",                             repaMOD, 2,   repa,  NORM,
"route {delivery} <ITEM> <SECTS>",          routMOD, 1,   rout,  NORM+CAP,
"set {price} <SHIP/FLEET> | <SECTS>",        setMOD, 2,   set,   NORM,
"shell {spawn a subshell}",                   SHELL, 0,   IGN,   NORM,
"ship {report on} <SHIP/FLEET>",             shiMOD, 0,   shi,   NORM,
"spy {on} <SECTS>",                          spyMOD, 1,   spy,   NORM+CAP,
"telegram {to} <CNUM/CNAME> [file]",        teleMOD, 0,   tele,  NORM,
"tend <SHIP>",                              tendMOD, 2,   tend,  NORM+MONEY,
"test {move} c|m|s|g|p|i|o|b|f",            moveMOD, 2,   move,  NORM+CAP,
"torpedo <SHIP>",                           torpMOD, 2,   torp,  NORM+MONEY,
"trade {report}",                           tradMOD, 2,   trad,  NORM,
"transport <NUKE>",                         tranMOD, 2,   tran,  NORM+CAP,
"treaty report",                            treaMOD, 1,   trea,  NORM,
"turn {the game} on|off|mess",              turnMOD, 0,   turn,  GOD,
"unload <SHIP/FLEET>",                      loadMOD, 2,   load,  NORM,
"update <SECTS> [quiet | verbose]",         updaMOD, 1,   upda,  NORM,
"version {identification}",                 versMOD, 0,   vers,  VIS,
"weather {map for} <SECTS>",                weatMOD, 0,   weat,  NORM,
0
};
