/* выдает сообщение 'n' и ждет ответа yes/no */

yes(x)
    int x;
{
    char c, c1;

beg:
    rspeak(x);

    for(;;) {
	do {
	    if( !read(0,&c,1) )  goto quit;
	} while( c==' ' || c=='\t' );
	do {
	    if( !read(0,&c1,1) ) goto quit;
	} while( c1!='\n' );

	if( c == 'y' || c=='Y' || c=='д' || c=='Д' ) {
	    x = 1;  break;
	} else if( c == 'n' || c=='N' || c=='н' || c=='Н' ) {
	    x = 0;  break;
	} else {
	    rspeak(40);
	}
    }
    return( x );

quit:
    /* PORT: ввод исчерпан (Ctrl-Z или конец файла).  В оригинале здесь
       снова задавался вопрос -- и read() снова возвращал 0, до
       бесконечности.  Выходим. */
    exit(0);
}
