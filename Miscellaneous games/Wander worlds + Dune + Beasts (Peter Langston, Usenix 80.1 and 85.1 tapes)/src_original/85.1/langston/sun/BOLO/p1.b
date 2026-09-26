; p1 -- P1 is a pacifist; he runs if hit, sits otherwise

	T_WALL <- -1
START:
	DAM <- DAMAGE

IDLE:
	if DAM = DAMAGE {
	    goto IDLE
	}

SEARCH:
	SCAN <- RANDOM
	if DIST < 200 {
	    goto SEARCH
	}
	SPEED <- DIST / 3
	HEADING <- SCAN
RUN:
	if RSPEED < SPEED {
	    goto RUN
	}
	SPEED <- 0
	goto START
