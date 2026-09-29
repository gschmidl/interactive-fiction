#include	<stdio.h>
/*
**	SUCKER -- Add name or delete it from any of the sucker lists
**	(c) P. Langston 3/84
*/

#include	"gamesdef.h"
char	*sllfil	= GAMESPATH(sucker.lists);	/* a list of sucker lists */
char	*tflfmt	= GAMESPATH(sl%d);		/* temporary file */
char	*gdir	= GAMESPATH(/);			/* default place for games */
char	*name;					/* my real logname */
int	privuid	= 257;				/* superuid; able to set sflg */
int	sflg	= 0;				/* snoop flag, show suckers */
FILE	*fp, *sfp;

char	*slf();
extern	char	*myrlog(), *copy();

main(argc, argv)
char	*argv[];
{
	register char *cp, *ep;
	char buf[128];

	name = myrlog();
	if ((fp = fopen(sllfil, "r")) == NULL) {
	    printf("Normally there's a file (`%s') that contains", sllfil);
	    printf(" a list of sucker lists, but not today...\n");
	    exit(1);
	}
	if (argc > 1) {
	    while (--argc > 0) {
		if (argv[argc][0] == '-')
		    disenroll(&argv[argc][1], 0);
		else if (argv[argc][0] == '+')
		    enroll(&argv[argc][1]);
		else if (argv[argc][0] == '?')
		    disenroll(&argv[argc][1], 1);	/* check */
		else if (argv[argc][0] == '*')
		    sflg++;
		else
		    goto syntax;
	    }
	    if (sflg == 0)
		exit(0);
	}
syntax:
	printf("\n");
	printf("This is `%s', the sucker list maintenance program.\n", argv[0]);
	printf("\n");
	printf("Sucker lists identify serious players of specific games.\n");
	printf("Anyone on the list is notified whenever the game starts.\n");
	printf("To add your name to the list for the game \"argle\"");
	printf(" run: %s +argle.\n", argv[0]);
	printf("To delete your name from the list for the game \"bargle\"");
	printf(" run: %s -bargle.\n", argv[0]);
	printf("To see if your name is on the list for the game \"gargle\"");
	printf(" run: %s '?gargle'.\n", argv[0]);
	printf("\n");
	printf("The current list of sucker lists contains:\n");
	fseek(fp, 0L, 0);
	while (fgets(buf, sizeof buf, fp) != NULL) {
	    if (*buf == '#')
		continue;
	    for (cp = buf; *cp > ' '; cp++);
	    *cp++ = '\0';
	    printf("    %s\n", buf);
	    if (sflg) {
		for (ep = cp; *ep > '\n'; ep++);
		*ep = '\0';
		if ((sfp = fopen(cp, "r")) != NULL) {
		    while (fgets(buf, sizeof buf, sfp) != NULL)
			printf("\t%s", buf);
		    fclose(sfp);
		}
	    }
	}
	exit(0);
}

enroll(game)
char	*game;
{
	char nbuf[10], *cp, *slfile;
	FILE *ofp;

	cp = copy(name, nbuf);
	*cp++ = '\n';
	*cp = '\0';
	if ((slfile = slf(game)) == 0) {
	    printf("%s doesn't have a sucker list.\n", game);
	    return;
	}
	if ((ofp = fopen(slfile, "a")) == (FILE *) NULL) {
	    perror(slfile);
	    return;
	}
	fputs(nbuf, ofp);
	fclose(ofp);
	fclose(ofp);
	printf("%s is now on the sucker list for %s.\n", name, game);
}

disenroll(game, check)
char	*game;
{
	char buf[128], nbuf[10], tfil[32], *cp, *slfile;
	int n;
	FILE *ifp, *ofp;

	if ((slfile = slf(game)) == 0) {
	    printf("%s doesn't have a sucker list.\n", game);
	    return;
	}
	if ((ifp = fopen(slfile, "r")) == (FILE *) NULL) {
	    perror(slfile);
	    return;
	}
	sprintf(tfil, tflfmt, getpid());
	if (check == 0 && (ofp = fopen(tfil, "w")) == (FILE *) NULL) {
	    perror(tfil);
	    return;
	}
	cp = copy(name, nbuf);
	*cp++ = '\n';
	*cp = '\0';
	n = 0;
	while (fgets(buf, sizeof buf, ifp) != NULL) {
	    if (strcmp(nbuf, buf)) {
		if (check == 0)
		    fputs(buf, ofp);
	    } else
		n++;
	}
	fclose(ifp);
	if (check == 0)
	    fclose(ofp);
	if (n <= 0)
	    printf("%s is not on the sucker list for %s.\n", name, game);
	else {
	    if (check)
		printf("%s is on the sucker list for %s.\n", name, game);
	    else {
		if (unlink(slfile) == -1)
		    perror("Unlinking sucker file");
		else if (link(tfil, slfile) == -1)
		    perror("Linking tempfile to sucker file");
		else {
		    unlink(tfil);
		    printf("%s is no longer on the sucker list for %s\n",
		     name, game);
		}
	    }
	}
}

char	*
slf(game)	/* retuen the name of the sucker list for specified game */
char	*game;
{
	register char *cp, *ep;
	register int line;
	static char buf[128], buf2[128];

	fseek(fp, 0L, 0);
	for (line = 1; fgets(buf, sizeof buf, fp) != NULL; line++) {
	    for (cp = buf; *cp > ' '; cp++);
	    *cp++ = '\0';
	    if (strcmp(buf, game) == 0) {
		while (*cp && *cp <= ' ')
		    cp++;
		if (*cp != '/') {
		    ep = copy(gdir, buf2);
		    while (*cp != '\n' && (*ep++ = *cp++));
		    cp = buf2;
		} else
		    for (ep = cp; *ep && *ep != '\n'; ep++);
		*ep = '\0';
		return(cp);
	    }
	}
	return((char *) 0);
}
