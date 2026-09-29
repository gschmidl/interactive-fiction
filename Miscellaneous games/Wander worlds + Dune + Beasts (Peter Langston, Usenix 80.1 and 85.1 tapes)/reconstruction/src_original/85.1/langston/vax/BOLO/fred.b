; fred -- Fred looks around for a target & runs if he's hit

	RANDOM <- 250

START:	D <- DAMAGE
SEEK:	if DAMAGE # D {
	    goto MOVE
	}
	AIM <- AIM + 17
SPOT:
	SCAN <- AIM
	if TYPE < 0 {
	    goto SEEK
	}
	RANGE <- DIST
	goto SPOT

MOVE:
	H <- RANDOM / 67 - 245
	V <- RANDOM / 67 - 245

	HEADING <- 256
MOVEX:
	SPEED <- H - XLOC
	if H - XLOC > 20 {
	    goto MOVEX
	}
	if H - XLOC < -20 {
	    goto MOVEX
	}
	SPEED <- 0

	HEADING <- 0
MOVEY:
	SPEED <- V - YLOC
	if V - YLOC > 20 {
	    goto MOVEY
	}
	if V - YLOC < -20 {
	    goto MOVEY
	}
	SPEED <- 0
	goto START
