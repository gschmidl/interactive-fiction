/*
** resetid() -- set "effective" uid & gid to "real" uid & gid
*/

resetid()
{
	setuid(getuid());			/* V7 */
	setgid(getgid());			/* V7 */
/*	setuid(getuid() & 0377);		/* V6 */
/*	setgid(getgid() & 0377);		/* V6 */
}
