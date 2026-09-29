; p2 -- P2 is a nervous pacifist; he runs all the time

	T_WALL <- -1
DODGE:
	SCAN <- HEADING
	SPEED <- DIST / 4
	if DIST < 30 {
	    HEADING <- HEADING + 32
	}
	if TYPE # T_WALL {
	    HEADING <- HEADING + 64
	}
	HEADING <- HEADING + 32
	goto DODGE
