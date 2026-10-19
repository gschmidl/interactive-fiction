/* Windows (MinGW) compatibility for Ken Wellsch's 1985 BSD C sources.
   Force-included with -include; the original sources stay K&R C. */
#include <io.h>
#include <fcntl.h>
#include <process.h>
#define index	strchr
#define rindex	strrchr
#define srandom	srand
#define random	rand
/* the key and record files are binary */
#define open(p,f)	_open((p),(f)|_O_BINARY)
#define creat(p,m)	_open((p),_O_CREAT|_O_TRUNC|_O_RDWR|_O_BINARY,0600)
/* one player per freezer file: SAVGM keys saved games by user id */
#define getuid()	0
