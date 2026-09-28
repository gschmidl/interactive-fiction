# Written 2026-09-27 by Claude Opus 5.5 at the direction of Jim Lippard.

COMMENT =	Adventure 1.2 (462 points) from Phoenix Multics, 1980

V =		1.0
DISTNAME =	adv462-${V}

GH_ACCOUNT =	lippard661
GH_PROJECT =	adv462
GH_TAGNAME =	v${V}

CATEGORIES =	games

HOMEPAGE =	https://github.com/lippard661/adv462

# BSD license for the 2026 transcription and porting work; Crowther,
# Woods, Palter and Platt retain their rights in the game (see README.md)
PERMIT_PACKAGE =	Yes

# gfortran from lang/gcc/15, as for adv550.  (The fortran module would
# instead require the ports tree's current default GCC.)
BUILD_DEPENDS =	lang/gcc/15,-f95
LIB_DEPENDS =	lang/gcc/15,-libs
WANTLIB +=	c m gfortran>=10
.if ${MACHINE_ARCH} == "amd64" || ${MACHINE_ARCH} == "i386"
WANTLIB +=	quadmath
.endif

# The portable Fortran source (generated from the Multics source by
# mkunix.py and checked in) and its Makefile are in unix/
WRKSRC =	${WRKDIST}/unix

# GAMEDIR is the writable game directory (created by pkg/PLIST)
MAKE_FLAGS =	FC=${LOCALBASE}/bin/egfortran \
		SHAREDIR=${TRUEPREFIX}/share/adv462 GAMEDIR=/usr/games/adv462

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/adv462 ${PREFIX}/bin
	${INSTALL_DATA_DIR} ${PREFIX}/share/adv462
	${INSTALL_DATA} ${WRKDIST}/Multics/adventure.data \
	    ${WRKSRC}/adventure.newgame ${PREFIX}/share/adv462
	${INSTALL_MAN} ${WRKSRC}/adv462.6 ${PREFIX}/man/man6

.include <bsd.port.mk>
