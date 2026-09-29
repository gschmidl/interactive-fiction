; lump -- Lump sits and scans until he finds a target

	T_WALL <- -1
SEEK:
	SCAN <- AIM
	if TYPE # T_WALL {
	    RANGE <- DIST
	}
	AIM <- AIM + 7
	goto SEEK
