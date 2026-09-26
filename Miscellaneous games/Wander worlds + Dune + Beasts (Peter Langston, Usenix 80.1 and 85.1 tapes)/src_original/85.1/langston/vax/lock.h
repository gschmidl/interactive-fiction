#define	NSSIG	16	/* number of "safe" (un-Berkeleyed) signals */

struct	lockstr {
	char	*l_node;	/* The "permanent" name for the file */
	char	*l_file;	/* The "temporary" link to l_node */
	char	*l_warn;	/* Msg to warn of lock condition */
	int	l_delay;	/* (600) how long before a lock is stale */
	int	l_limit;	/* (12) how many times to try */
	int	l_secs;		/* (5) how long to wait between tries */
	int	(*l_done)();	/* Routine to handle interrupts while locked */
	/* the preceding set by the user; the following by lock() & unlock() */
	int	l_count;	/* how many levels deep we've locked it */
	int	l_sigs[NSSIG];	/* saved signal values */
};
