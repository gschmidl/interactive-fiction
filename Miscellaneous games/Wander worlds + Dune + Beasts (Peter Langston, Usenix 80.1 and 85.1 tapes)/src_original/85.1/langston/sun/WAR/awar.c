#include    "wardef.h"

/*      Auto War        */

static  char    *awarid = "@(#)awar.c	1.4 10/17/84 -- (c) psl 1976";
static  char    *wardefid  = DEF_SCCS;
static  char    ocapbuf[6];             /* our capital loc in chars */
static  int     ocx, ocy;               /* our capitol loc in x, y */
static  int     ocxy;                   /* our capital index into b_cell[] */
static  struct  cellstr     *ocapcell;  /* pointer to our capital cell */
static  struct  cellstr     *tcapcell;  /* pointer to their capital cell */
static  struct  boardstr    dboard;     /* distance board (from their cap) */

char	*cptoa();

cap_pos(argc, argv)
char	*argv[];
{
        register int x, y;
        char *bp;
        int n, cno;

        if (argc == 2) {
            bp = argv[1];
	    ocxy = get_coords(&bp);
	    ocx = ocxy % MAX_X;
	    ocy = ocxy / MAX_X;
            goto gotit;
        }
	x = tnum==0? W_P0_RDY : W_P1_RDY;
        do {
            getboard(&dboard, SORRY);
	} while (argc != 2 && (dboard.b_state & x) != x);
        set_dboard();
        n = 0;
        for (x = dboard.b_xmax + 1; --x > 0; ) {
            for (y = dboard.b_ymax + 1; --y > 0; ) {
                cno = dboard.b_cell[y * MAX_X + x].c_flg;
                if (cno > n && cno < 30000) {
                    n = cno;
		    ocx = x;
		    ocy = y;
                }
            }
        }
	ocxy = ocy * MAX_X + ocx;
gotit:
	ocapcell = &board.b_cell[ocxy];
	sprintf(ocapbuf, "%c%d", 'a' + ocx - 1, ocy - 1);
	return(ocxy);
}

num_para()
{
	return((rand() & 010)? 3 : 1);
}

num_shell()
{
	return(rand() % 5 + 1);
}

set_dboard()
{
        register int x, y;
        register struct cellstr *cp;
        int i, j, cno, inc, maxdist;

        for (x = dboard.b_xmax + 1; --x > 0; ) {
            for (y = dboard.b_ymax + 1; --y > 0; ) {
                cp = &dboard.b_cell[y * MAX_X + x];
                if ((cp->c_flg & C_TYPE) == C_MTN)
                    cp->c_flg = -1;
		else if ((cp->c_flg & C_TYPE) == their_home) {
                    cp->c_flg = 0;
		    tcapcell = &board.b_cell[y * MAX_X + x];
		} else
                    cp->c_flg = 32000;
            }
        }
        maxdist = 3;
        for (i = 0; i <= maxdist; i++) {
            for (x = dboard.b_xmax + 1; --x > 0; ) {
                for (y = dboard.b_ymax + 1; --y > 0; ) {
                    cno = y * MAX_X + x;
                    if (dboard.b_cell[cno].c_flg != i)
                        continue;
                    for (j = 4; --j >= 0; ) {
                        cp = &dboard.b_cell[cno + delta[j]];
                        if (cp->c_mil > 0) {
                            if (cp->c_occ == tnum)
                                inc = 1 + cp->c_mil / 2;
                            else
                                inc = 1;
                        } else
                            inc = 2;
                        cp->c_flg = min(cp->c_flg, i + inc);
			if (cp->c_flg > maxdist)
			    maxdist = cp->c_flg;
                    }
                }
            }
        }
}

start_cmnd()
{
}

get_command(bp)
char	*bp;
{
        register int i, j;
        register struct cellstr *cp;
        char *chp;
	int listnum, k, l, m, dir, mindist;
	int importance, capfactor, danger;
	static int movenum;
        struct cellstr *ep, *tp;
        struct cellstr *list[99];

	movenum++;
        /* compile a list of our cells */
        listnum = 0;
        for (i = 1; i <= board.b_xmax; i++) {
            for (j = 1; j <= board.b_ymax; j++) {
                cp = &board.b_cell[j * MAX_X + i];
                if (cp->c_mil < 1
                 || cp->c_occ != onum)
                    continue;
                list[listnum++] = cp;
            }
        }
	importance = -999;
        /* check for potential losing battle */
        for (i = listnum; --i >= 0; ) {
            cp = list[i];
            if ((cp->c_flg & C_BAT_ANY) != 0)
                continue;
            for (j = 4; --j >= 0; ) {
                ep = cp + delta[j];
                if (ep->c_mil <= 0 || ep->c_occ == onum
                 || ep->c_mil <= cp->c_mil
                 || (cp->c_mil < 3 && cp->c_mil == ep->c_mil))
                    continue;
		cp->c_flg |= C_BAT | (j << 3);     /* fake existing battle */
            }
        }
        /* look for a fight to start */
        for (i = listnum; --i >= 0; ) {
            cp = list[i];
            if ((cp->c_flg & C_BAT_ANY) != 0)
                continue;
            for (j = 4; --j >= 0; ) {
                ep = cp + delta[j];
		capfactor = ((cp == ocapcell) || (ep == tcapcell))? 10 : 1;
		if (ep->c_mil < 1
		 || ep->c_occ != tnum
		 || (ep->c_flg & C_BAT) != 0
                 || (ep->c_mil >= cp->c_mil && capdist(ep) > 2)
		 || (ep->c_mil > 2 * cp->c_mil && capdist(ep) > 1)
		 || (k = capfactor * (cp->c_mil - ep->c_mil)) < importance)
                    continue;
		sprintf(bp, "a %s %c  [1]  ", cptoa(cp), "urdl"[j]);
		importance = k;
            }
        }
	/* look for a fight to stop or support, (also shelling) */
        for (i = listnum; --i >= 0; ) {
            cp = list[i];
	    if (cp->c_flg & (C_S0_SOON | C_S1_SOON | C_S0_NOW | C_S1_NOW))
		goto retreat;                       /* sorry, Mr. Djikstra */
            if ((cp->c_flg & C_BAT_ANY) == 0)
                continue;
            j = (cp->c_flg & C_BAT_ANY) >> 3;
            ep = cp + delta[j];
	    if (ep->c_mil < cp->c_mil)                   /* we're winning! */
                continue;
	    k = -1;                            /* losing, look for support */
            l = m = ep->c_mil - cp->c_mil;
	    danger = (cp == ocapcell)? 100 : cp->c_mil;
            for (j = listnum; --j >= 0; ) {
                ep = list[j];
                if (ep == cp
		 || (ep == ocapcell && ep->c_mil < m + 10)
		 || ((ep->c_flg & C_BAT_ANY) != 0 && cp != ocapcell)
/* really should check if action is winning or losing in ep and transfer
/* if losing ... */
		 || connected(ep, cp) == 0)
                    continue;
                l -= ep->c_mil;
                if (ep->c_mil > k) {
                    tp = ep;
		    k = tp->c_mil;
                }
            }
	    if (tp->c_mil > 0
	     && m > -3
	     && (l < 0 || cp == ocapcell)
	     && danger > importance) {
		sprintf(bp, "t %d %s %s  [2]  ",
		 min(tp->c_mil, m + 3), cptoa(tp), cptoa(cp));
		importance = k;
		continue;
            }
	/* look for troops to move toward trouble spot */
	/*     someday   */
	/* (also decide if it's worth it) */
	    if (cp == ocapcell)
		continue;                  /* looks bad, but tought it out */
retreat:
	/* look for a retreat to our own cell */
            for (j = 4; --j >= 0; ) {
                ep = cp + delta[j];
		if (ep->c_mil > 0
		 && ep->c_occ == onum
		 && danger > importance) {
		    sprintf(bp, "m 99 %s %c  [3]  ", cptoa(cp), "urdl"[j]);
		    importance = danger;
		    break;
                }
            }
	    if (j >= 0)                                       /* found one */
		continue;
	/* look for a retreat that keeps us close to their capital */
            getboard(&dboard, SORRY);
            set_dboard();
            mindist = 999;
            for (j = 4; --j >= 0; ) {
                ep = cp + delta[j];
                if (ep->c_mil > 0 || ep->c_flg == C_MTN)
                    continue;
                l = capdist(ep);
                if (l < mindist) {
                    mindist = l;
                    dir = j;
                }
            }
	    if (mindist < 999 && danger > importance) {
                sprintf(bp, "m %d %s %c  [4]  ",
		 cp->c_mil, cptoa(cp), "urdl"[dir]);
                return(1);
            }
        }
	/* look for a place to shell */
	if (shell > 0) {
	    for (i = 1; i <= board.b_xmax; i++) {
		for (j = 1; j <= board.b_ymax; j++) {
		    cp = &board.b_cell[j * MAX_X + i];
		    if (cp->c_mil < 1
		     || cp->c_occ != tnum
		     || cp == tcapcell
		     || (k = cp->c_mil / 2) < importance)
			continue;
		    sprintf(bp, "s %s  [5]  ", cptoa(cp));
		    importance = cp->c_mil / 2;
		}
	    }
	}
	/* look for a chance to "drop in" on their capital */
	if (importance < 99
	 && ((para > 0 && tcapcell->c_mil == 0
	 && movenum > 10)
	     || (para > 2 && tcapcell->c_mil == 1))) {
	    sprintf(bp, "p %s  [6]  ", cptoa(tcapcell));
	    importance = 99;
	}
        /* look for a good advance to make */
        getboard(&dboard, SORRY);
        set_dboard();
        mindist = 999;
        for (i = listnum; --i >= 0; ) {
            cp = list[i];
            l = capdist(cp);
            if (l < mindist
	     && connected(ocapcell, cp)
	     && (ocapcell->c_flg & C_BAT_ANY) == 0
	     && ocapcell->c_mil > 20) {
                tp = cp;
                dir = 5;
                mindist = l;
            }
            if (cp->c_mil < 2
             || (cp->c_flg & C_BAT_ANY) != 0)
                continue;
            j = k = rand() % 4;
            do {
                ep = cp + delta[j];
                if ((ep->c_mil > 0 && ep->c_occ == tnum)
                 || (ep->c_flg & C_TYPE) == C_MTN)
                    continue;
                l = capdist(ep);
                if (l < capdist(cp) && l < mindist) {
                    mindist = l;
                    tp = cp;
                    dir = j;
                }
            } while ((j = (j + 1) & 3) != k);
        }
	if (importance <= 0
	 && mindist < 999
	 && tp->c_mil > 1
	 && (tp != ocapcell || tp->c_mil > 9)) {
	    if (tp == ocapcell)
		j = min(capdist(tp) + 5, tp->c_mil - 9);
	    else
		j = tp->c_mil - 1;
	    importance = 1;
            if (dir == 5)
		sprintf(bp, "t %d %s %s  [7]  ", j, cptoa(ocapcell), cptoa(tp));
            else {
		sprintf(bp, "m %d %s %c  [8]  ", j, cptoa(tp), "urdl"[dir]);
            }
            return(1);
        }
        /* look for indirect advance */
	if (importance <= 0) {
	    mindist = 999;
	    for (i = listnum; --i >= 0; ) {
		cp = list[i];
		if ((cp->c_flg & C_BAT_ANY) != 0
		 || (cp == ocapcell && cp->c_mil < 20))
		    continue;
		j = m = rand() % 4;
		do {
		    ep = cp + delta[j];
		    if ((ep->c_mil > 0 && ep->c_occ == tnum)
		     || (ep->c_flg & C_TYPE) == C_MTN)
			continue;
		    for (k = 4; --k >= 0; ) {
			if (delta[j] + delta[k] == 0)
			    continue;
			ep = cp + delta[j] + delta[k];
			if ((ep->c_mil > 0 && ep->c_occ == tnum)
			 || (ep->c_flg & C_TYPE) == C_MTN)
			    continue;
			l = capdist(ep);
			if (l < mindist) {
			    mindist = l;
			    tp = cp;
			    dir = j;
			}
		    }
		} while ((j = (j + 1) & 3) != m);
	    }
	    if (mindist < 999) {
		j = (capdist(tp) < 4 || tp == ocapcell)?
		 (tp->c_mil + 1) / 2 : tp->c_mil;
		sprintf(bp, "m %d %s %c  [9]  ", j, cptoa(tp), "urdl"[dir]);
		importance = 1;
	    }
	}
	if (importance > 0)
	    return(1);
        return(0);
}

capdist(cp)
struct	cellstr	*cp;
{
	register int i;

	i = cp - board.b_cell;
	return(dboard.b_cell[i].c_flg);
}

char *
cptoa(cp)
struct	cellstr	*cp;
{
        register int i, j;
        static char buf[4][6];
        static int next;

        next = (next + 1) & 3;
        i = cp - &board.b_cell[0];
        j = i / MAX_X;
        i %= MAX_X;
        sprintf(buf[next], "%c%d", 'a' + i - 1, j - 1);
        return(buf[next]);
}

cleanup()
{
}
