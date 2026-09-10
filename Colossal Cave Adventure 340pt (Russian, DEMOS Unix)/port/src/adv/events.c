/*d* === treat events ===       19.01.85   version   10 */

#include "../common/adv_common"
#include <stdio.h>

int actfla;
FILE *F1;

events() {
    int actres;
    actfla=1;
    actres=act(tevent,0);
    actfla=0;
}


ini() {
    int actres;

    loadcm();

    if( (F1=fopen("adv:frozen","r")) != NULL ) {
	loadfr();

    } else {
	/* PORT: на PDP-11 tim[1] был младшей половиной 32-битного времени;
	   на 64-битной машине это его старшая (всегда нулевая) половина,
	   т.е. каждая партия получала бы одно и то же случайное число. */
	srand( (unsigned) time( (time_t *) 0 ) );
	loc=1;
	rndini=rand();
	actfla=1;
	actres=act(tiniti,0);
	actfla=0;
    }
    descr();
}
