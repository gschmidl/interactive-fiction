/* mailprog and mailarg need to be global so that other routines can
** diddle them as necessary.
*/
/*char	*mailprog	= "/etc/delivermail";	/* for BSD */
char	*mailprog	= "/bin/mail";		/* for Vanilla */
/*char	*mailprog	= "/bin/sendmail";	/* for IS/1 */
/*char	*mailprog	= "/usr/lfl/lib/sm";	/* for lfl */

char	*mailarg	= 0;			/* optional (e.g. oraglb.c) */

/*
**      SENDMSG(user-name, file-handle)
*/

sendmsg(to, fh)
char    *to;
{
	switch (fork()) {
	case 0:
	    if (fh != 0) {
		close(0);
		dup(fh);
		close(fh);
	    }
	    if (mailarg)
		execl(mailprog, "mail", mailarg, to, 0);
	    else
		execl(mailprog, "mail", to, 0);
	    perror(mailprog);
	    exit(3);
	default:
	    return;
	}
}
