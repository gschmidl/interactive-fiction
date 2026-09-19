#include	<stdio.h>
#include	<crt.h>
#include	<sm_def.h>
#include	<signal.h>
#include	"grid.h"
#include	"daemon.h"

/*
**	DVIZ -- Graphics for daemon for "grid", labyrinth game
*/

map(pp, flg)
struct	plyrstr	*pp;
{
	register int i, j, c, k;
	int l, lx, hx, ly, hy, n;

	l = pp->p_l;
	lx = 0;
	hx = MW;
	ly = 0;
	hy = MH;
	if (flg == 0) {			/* player area only */
	    lx = pp->p_x - 1;
	    hx = pp->p_x + 2;
	    ly = pp->p_y - 1;
	    hy = pp->p_y + 2;
	}
	for (j = hy; --j >= ly; ) {
	    for (i = hx; --i >= lx; ) {
		c = ' ';
		if (isseen(pp, i, j) || (pp->p_flg & MAPW))
		    c = grid(i, j, pp->p_l);
		if (isseen(pp, i, j) || (pp->p_flg & MAPS)) {
		    for (k = NSTAIRS; --k >= 0; ) {
			if (lev[l].l_us[k].X == i
			 && lev[l].l_us[k].Y == j)
			    c = 'u';
			else if (lev[l].l_ds[k].X == i
			 && lev[l].l_ds[k].Y == j)
			    c = 'd';
		    }
		}
		if ((pp->p_flg & MAPG) && i > 0) {
		    for (n = maxg; --n >= 0; )
			if (lev[l].l_g[n].g_x == i
			 && lev[l].l_g[n].g_y == j) {
			    c = '$';
			    break;
			}
		}
		if ((pp->p_flg & MAPO) && i > 0) {
		    for (n = maxo; --n >= 0; )
			if (lev[l].l_o[n].o_x == i
			 && lev[l].l_o[n].o_y == j) {
			    c = 'o';
			    break;
			}
		}
		if (pp->p_flg & MAPP) {
		    for (n = maxpnum; --n >= 0; ) {
			if (p[n].p_health >= livelim
			 && p[n].p_l == l
			 && p[n].p_x == i
			 && p[n].p_y == j
			 && pp != &p[n])
			    c = 'A' + n;
			    break;
			}
		}
		if (i == pp->p_x && j == pp->p_y)
		    c = "^>v<"[pp->p_d];
		sm_char(pp->p_msmp, i, j, c);
	    }
	}
	sm_spcl(pp->p_msmp, SM_FLUSH);
	pp->p_needs |= N_REPOS;
/****
	tputsfh(crt(pp->p_crtyp, 0, pp->p_vh), 1, pp->p_fh, pp->p_ospeed);
****/
}

scroll(pp, buf)
struct	plyrstr	*pp;
char	*buf;
{
	register char *cp;

/*8888*sleep(1);write(pp->p_fh, "about to move to scroll line ", 29);sleep(1);
/*8888*/
	tputsfh(crt(pp->p_crtyp, 0, pp->p_vh), 1, pp->p_fh, pp->p_ospeed);
/*8888*sleep(1);write(pp->p_fh, "about to close up a line ", 25);sleep(1);
/*8888*/
	tputsfh(crt(pp->p_crtyp, C_LINE_DEL), 3, pp->p_fh, pp->p_ospeed);
/*8888*sleep(1);write(pp->p_fh, "about to move down two lines ", 28);sleep(1);
/*8888*/
	tputsfh(crt(pp->p_crtyp, 0, pp->p_vh + 2), 1, pp->p_fh, pp->p_ospeed);
/*8888*sleep(1);write(pp->p_fh, "about to write buf ", 18);sleep(1);
/*8888*/
	for (cp = buf; *cp; cp++);
	write(pp->p_fh, buf, cp - buf);
/*8888*sleep(1);write(pp->p_fh, "done writing buf ", 16);sleep(1);
/*8888*/
	pp->p_needs &= ~N_REPOS;
}

tellall(xpp, buf)
struct	plyrstr	*xpp;
char	*buf;
{
	register struct plyrstr *pp;

	for (pp = &p[maxpnum]; --pp >= p; )
	    if (pp->p_health >= livelim && pp != xpp)
		scroll(pp, buf);
}

/* if flg is set only update the "_seen" array; do no graphics */
view(pp, flg)
struct	plyrstr	*pp;
{
	register int x, y;
	int i, dist, side, lx, ly, rx, ry, l;
	double dc1, dc2, dc1a, dc2a, x1, x2, x3, x4, y1, y2, y3, y4;
	struct plyrstr	*opp;
	struct levstr *lp;

	l = pp->p_l;
	if (flg) {
	    sm_spcl(pp->p_vsmp, SM_ERASE);
	    pp->p_needs |= N_VFLSH;
	    pp->p_needs &= ~N_VIEW;
	    lp = &lev[l];
	}
	x = (pp->p_vd + 3) % 4;
	lx = dx[x];
	ly = dy[x];
	x = (pp->p_vd + 1) % 4;
	rx = dx[x];
	ry = dy[x];
	for (dist = 1; dist < MAXDIST; dist++) {
	    x = pp->p_vx + dist * dx[pp->p_vd];
	    y = pp->p_vy + dist * dy[pp->p_vd];
	    if (!INGRID(x, y))
		break;
	    seen(pp, x, y);
	    if (flg) {
		dc1 = 1. / (dist * 2. - 1.);
		dc2 = 1. / (dist * 2. + 1.);
		y1 = pp->p_vmy * (1. - dc1);
		y4 = pp->p_vmy * (1. + dc1);
	    }
	    /* center view */
	    if (iswall(x, y, l)) {
		/* wall */
		seen(pp, x + lx, y + ly);
		seen(pp, x + rx, y + ry);
		if (flg) {
		    x1 = pp->p_vmx * (1. - dc1);
		    x2 = pp->p_vmx * (1. + dc1);
		    conn(pp, x1, y1, x1, y4); /* vert at x1 from y1 to y4 */
		    conn(pp, x2, y1, x2, y4); /* vert at x2 from y1 to y4 */
		    conn(pp, x1, y1, x2, y1);  /* hor at y1 from x1 to x2 */
		    conn(pp, x1, y4, x2, y4);  /* hor at y4 from x1 to x2 */
		}
		break;
	    }
	    if (flg) {
		dc1a = (3. * dc1 + dc2) / 4.;
		dc2a = (dc1 + 3. * dc2) / 4.;
		for (i = NSTAIRS; --i >= 0; ) {
		    /* up trapdoors */
		    if (x == lp->l_us[i].X && y == lp->l_us[i].Y) {
			x1 = pp->p_vmx * (1. - dc1a / 2.);
			x2 = pp->p_vmx * (1. - dc2a / 2.);
			x3 = pp->p_vmx * (1. + dc2a / 2.);
			x4 = pp->p_vmx * (1. + dc1a / 2.);
			y2 = pp->p_vmy * (1. - dc2a);
			y3 = pp->p_vmy * (1. - dc1a);
			conn(pp, x2, y2, x2, y3);      /* left hole vertical */
			conn(pp, x3, y2, x3, y3);     /* right hole vertical */
			conn(pp, x1, y3, x2, y2);       /* left edge of hole */
			conn(pp, x4, y3, x3, y2);      /* right edge of hole */
			conn(pp, x2, y2, x3, y2);   /* hor, far edge of hole */
			conn(pp, x1, y3, x4, y3);  /* hor, near edge of hole */
			break;
		    }
		}
		for (i = NSTAIRS; --i >= 0; ) {
		    /* down trapdoors */
		    if (x == lp->l_ds[i].X && y == lp->l_ds[i].Y) {
			x1 = pp->p_vmx * (1. - dc1a / 2.);
			x2 = pp->p_vmx * (1. - dc2a / 2.);
			x3 = pp->p_vmx * (1. + dc2a / 2.);
			x4 = pp->p_vmx * (1. + dc1a / 2.);
			y2 = pp->p_vmy * (1. + dc2a);
			y3 = pp->p_vmy * (1. + dc1a);
			conn(pp, x2, y2, x2, y3);      /* left hole vertical */
			conn(pp, x3, y2, x3, y3);     /* right hole vertical */
			conn(pp, x1, y3, x2, y2);       /* left edge of hole */
			conn(pp, x4, y3, x3, y2);      /* right edge of hole */
			conn(pp, x2, y2, x3, y2);   /* hor, far edge of hole */
			conn(pp, x1, y3, x4, y3);  /* hor, near edge of hole */
			break;
		    }
		}
	    }
	    /* side views */
	    if (flg) {
		y2 = pp->p_vmy * (1. - dc2);
		y3 = pp->p_vmy * (1. + dc2);
	    }
	    for (side = -1; side <= 1; side += 2) {
		if (side == -1) {
		    x = pp->p_vx + dist * dx[pp->p_vd] + lx;
		    y = pp->p_vy + dist * dy[pp->p_vd] + ly;
		    if (flg) {
			x1 = pp->p_vmx * (1. - dc1);
			x2 = pp->p_vmx * (1. - dc2);
		    }
		} else {
		    x = pp->p_vx + dist * dx[pp->p_vd] + rx;
		    y = pp->p_vy + dist * dy[pp->p_vd] + ry;
		    if (flg) {
			x1 = pp->p_vmx * (1. + dc1);
			x2 = pp->p_vmx * (1. + dc2);
		    }
		}
		if (!INGRID(x, y))
		    continue;
		seen(pp, x, y);
		if (flg) {
		    if (iswall(x, y, l)) {
			/* wall */
			conn(pp, x1, y1, x2, y2);  /* diag x1,y1 to x2,y2 */
			conn(pp, x1, y4, x2, y3);  /* diag x1,y4 to x2,y3 */
		    } else {
			/* hall */
			conn(pp, x1, y2, x2, y2);		/* hor at y2 */
			conn(pp, x1, y3, x2, y3);		/* hor at y3 */
			conn(pp, x1, y1, x1, y4);	       /* vert at x1 */
			conn(pp, x2, y2, x2, y3);	       /* vert at x2 */
		    }
		}
	    }
	}
	if (flg) {
	    while (--dist > 0) {
		x = pp->p_vx + dist * dx[pp->p_vd];
		y = pp->p_vy + dist * dy[pp->p_vd];
		/* talismans */
		for (i = maxt; --i >= 0; ) {
		    if (t[i].t_type != 0
		     && t[i].t_l == l
		     && t[i].t_x == x
		     && t[i].t_y == y)
			break;
		}
		if (i >= 0) {
		    i = pp->p_vmy * (1. + 0.5 / dist);
		    sm_char(pp->p_vsmp, pp->p_vmx, i, '*');
		}
		/* players */
		for (i = maxpnum; --i >= 0; ) {
		    opp = &p[i];
		    if (opp->p_health >= livelim
		     && (opp->p_flg & INVIS) == 0
		     && opp != pp
		     && opp->p_l == l
		     && opp->p_x == x
		     && opp->p_y == y) {
			entity(pp, dist, pp->p_vmx, pp->p_vmy, opp->p_d,
			 'A' + i);
			break;
		    }
		}
		/* others */
		for (i = maxo; --i >= 0; )
		    if (lp->l_o[i].o_x == x && lp->l_o[i].o_y == y)
			break;
		if (i >= 0)
		    entity(pp, dist, pp->p_vmx, pp->p_vmy, lp->l_o[i].o_d, 'o');
		/* gold */
		for (i = maxg; --i >= 0; )
		    if (lp->l_g[i].g_x == x && lp->l_g[i].g_y == y)
			break;
		if (i >= 0) {
		    y = pp->p_vmy * (1. + 0.5 / dist);
		    goldc(pp, dist, pp->p_vmx, y, lp->l_g[i].g_a);
		}
	    }
	}
}

goldc(pp, dist, cx, cy, amt)			/* draw pile of gold coins */
struct	plyrstr	*pp;
{
	register int xr, yr, i;

	for (i = 9; --i > 0 && i * i >= amt; );	/* cheap square root */
	yr = pp->p_vh * i / (24. * dist);
	if (i > 6)
	    i = 6;
	xr = pp->p_vw * i / (24. * dist);
	pile(pp, cx, cy, xr, yr, '$');
}

/* draw triangle of char c with base center at cx,cy height ry, & width 2*rx */
pile(pp, cx, cy, xr, yr, c)
struct	plyrstr	*pp;
{
	register int x, y, xlim;

	for (y = 0; y <= yr; y++) {
	    if (yr > 0)
		xlim = (y * xr) / yr;
	    else
		xlim = 1;
	    for (x = -xlim; x <= xlim; x++)
		sm_char(pp->p_vsmp, cx + x, cy - yr + y, c);
	}
}

entity(pp, dist, cx, cy, d, sym)		/* draw an entity at cx, cy */
struct	plyrstr	*pp;
{
	register int xr, yr;

	xr = pp->p_vw / (4. * dist);
	yr = pp->p_vh / (4. * dist);
	circle(pp, cx, cy, xr, yr, sym);
	switch ((pp->p_vd - d + 4) % 4) {
	case 0:				/* back to us */
	    break;
	case 1:				/* looking to our left */
	    circle(pp, cx - (xr * 7) / 8, cy, xr / 4, yr / 4, '#');
	    break;
	case 2:				/* facing us */
	    circle(pp, cx, cy, xr / 2, yr / 2, '#');
	    break;
	case 3:				/* looking to our right */
	    circle(pp, cx + (xr * 7) / 8, cy, xr / 4, yr / 4, '#');
	    break;
	}
}

/* draw a diamond of character c centered on cx,cy with radii xr,yr */
circle(pp, cx, cy, xr, yr, c)
struct	plyrstr	*pp;
{
	register int x, y, xlim;

	for (y = -yr; y <= yr; y++) {
	    if (yr > 0)
		xlim = ((yr - (y < 0? -y : y)) * xr) / yr;
	    else
		xlim = 1;
	    for (x = -xlim; x <= xlim; x++)
		sm_char(pp->p_vsmp, cx + x, cy + y, c);
	}
}

conn(pp, x1, y1, x2, y2)		/* draw a line from x1,y1 to x2,y2 */
struct	plyrstr	*pp;
double	x1, y1, x2, y2;
{
	register int x, y, c;
	int i, m, n;
	double mx, my, dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;
	m = dx >= 0.? 1. + dx : 1. - dx;
	n = dy >= 0.? 1. + dy : 1. - dy;
	if (m > n) {
	    i = m;
	    m = n;
	    n = i;
	}
	if (dx == 0.)
	    c = '|';			/* vertical */
	else if (dy == 0.)
	    c = '-';			/* horizontal */
	else {
	    if (dx * dy > 0.)
		c = '\\';		/* \ diagonal */
	    else
		c = '/';		/* / diagonal */
	    n = m;
	}
	mx = dx / n;
	my = dy / n;
	for (i = 0; i <= n; i++) {
	    x = x1 + i * mx + 0.5;
	    y = y1 + i * my + 0.5;
	    sm_char(pp->p_vsmp, x, y, c);
	}
}
