/*
**      LOGDIR -- Return login directory of current user
**      On our system this info comes out of the user page.
**      This version digs the login dir out of the passwd file.
*/

logdir()
{
	return(getudir(myruid()));
}
