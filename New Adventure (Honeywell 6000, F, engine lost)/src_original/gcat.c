#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>


typedef struct {
	unsigned dl;
	unsigned du;
} GWORD;
#define HIBYTE(d)	(((d) & 0777000) >> 9)
#define LOBYTE(d)	((d) & 0777)

FILE* gf = NULL;
int   geven = 0;
int   glast = 0;
int   geof = 0;
int   gword = 0;


	void
msg( char* fmt, ... )
{
	va_list ap;

	fprintf( stderr, "GCAT[%#o,%#o]: ", gword/320, gword%320 );
	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
	fprintf( stderr, "\n" );
}


	void
err_if( int cond, char* fmt, ... )
{
	va_list ap;

	if( !cond )
		return;
	fprintf( stderr, "GCAT[%#o,%#o]: ", gword/320, gword%320 );
	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
	fprintf( stderr, "\n" );
	exit( EXIT_FAILURE );
}


	void
gopen( char* fname )
{
	gf = fopen( fname, "rb" );
	err_if( !gf, "Can't open '%s'", fname );
	geven = 1;
	glast = 0;
	gword = 0;
}


	void
ggetc( void )
{
	glast = getc( gf );
	if( glast == EOF ) {
		geof = 1;
		glast = 0;
	}
}


	GWORD
gread( void )
{
	GWORD w = { 0, 0 };

	if( geven ) {
		ggetc();
		if( geof )
			return w;
		         w.du  = (glast & 0xff) << 10;
		ggetc(); w.du |= (glast & 0xff) << 2;
		ggetc(); w.du |= (glast & 0xc0) >> 6;
		         w.dl  = (glast & 0x3f) << 12;
		ggetc(); w.dl |= (glast & 0xff) << 4;
		ggetc(); w.dl |= (glast & 0xf0) >> 4;
	}
	else {
		         w.du  = (glast & 0x0f) << 14;
		ggetc(); w.du |= (glast & 0xff) << 6;
		ggetc(); w.du |= (glast & 0xfc) >> 2;
		         w.dl  = (glast & 0x03) << 16;
		ggetc(); w.dl |= (glast & 0xff) << 8;
		ggetc(); w.dl |= (glast & 0xff);
	}

	geven = !geven;
	++gword;
	return w;
}


	void
gskip( int n )
{
	while( n-- > 0 )
		(void) gread();
}


	void
gclose( void )
{
	fclose( gf );
	gf = NULL;
}


	int
pchar( int c )
{
	c &= 0777;
	if( c > 0177 )
		return '~';
	return isprint(c) ? c : '~';
}


	void
fdp( char* fname )
{
	int   llink = 0;
	int   word  = 0;
	GWORD w[4];
	int   i;

	gopen( fname );
	for(;;) {
		for( i = 0; i < 4; ++i ) {
			w[i] = gread();
			if( geof )
				goto end_of_file;
		}
		if( word == 320 ) {
			printf( "\n" );
			++llink;
			word = 0;
		}
		if( word == 0 )
			printf( "Llink %o\n", llink );
		printf( "%03.3o:", word );
		for( i = 0; i < 4; ++i )
			printf( " %06.6o%06.6o", w[i].du, w[i].dl );
		printf( " " );
		for( i = 0; i < 4; ++i ) {
			printf( " %c%c%c%c",
					pchar(HIBYTE(w[i].du)), pchar(LOBYTE(w[i].du)),
					pchar(HIBYTE(w[i].dl)), pchar(LOBYTE(w[i].dl)) );
		}
		printf( "\n" );
		word += 4;
	}
  end_of_file:;
	gclose();
}


	void
list_ascii_word( GWORD w )
{
	err_if( geof, "unexpect EOF in ascii record" );

	if( HIBYTE(w.du) == 0177 )
		return;
	putchar( HIBYTE(w.du) );

	if( LOBYTE(w.du) == 0177 )
		return;
	putchar( LOBYTE(w.du) );

	if( HIBYTE(w.dl) == 0177 )
		return;
	putchar( HIBYTE(w.dl) );

	if( LOBYTE(w.dl) == 0177 )
		return;
	putchar( LOBYTE(w.dl) );
}


	void
list_record( void )
{
	GWORD w;
	int   size;
	int   mark;
	int   code;
	int   avail;
	int   i;

	w = gread();
	size = w.du;
	avail = (w.dl >> 16) & 03;
	mark = (w.dl >> 12) & 017;
	code = (w.dl >> 6) & 017;
	#if 0
	fprintf( stderr, ">>> size=%o avail=%o mark=%o code=%o\n", size, avail, mark, code );
	#endif

	if( size == 0 && (mark == 017 || mark == 023) ) {
		geof = 1;
		return;
	}

	switch( code ) {
		case 05:	/* TSS ascii */
		case 06:	/* standard ascii */
			for( i = 0; i < size; ++i )
				list_ascii_word( gread() );
			putchar( '\n' );
			break;
		case 010:	/* TSS info record */
			for( i = 0; i < size; ++i )
				(void) gread();
			break;
		default:
			msg( "can't handle media code %o", code );
			for( i = 0; i < size; ++i )
				(void) gread();
			break;
	}
}


	void
list( char* fname )
{
	GWORD w;
	int   file_start;
	int   block_start;
	int   block_size;
	int   block_end;

	gopen( fname );

	/*
	 * First word is a BCW of the archive file.
	 * The second word has the offset of the start of the actual file contents 
	 * in DL. The pathname is in this header bit.
	 */
	(void) gread();	/* archive BCW */
	w = gread();
	file_start = w.dl;

	/* ... this is where we can find the filename */

	/* seek to start of actual file */
	while( gword < file_start )
		w = gread();

	/*
	 * Each block is 320 words, but only the block size (per BCW) is used.
	 */
	for(;;) {
		w = gread();
		if( geof )
			break;
		block_size = w.dl;
		block_start = gword;
		block_end = gword + block_size;
		fprintf( stderr, ">>> block start=%o size=%o\n", block_start, block_size );
		while( !geof && gword < block_end )
			list_record();
		while( !geof && gword < block_start+320-1 )
			(void) gread();
	}

	gclose();
}


	int
main( int argc, char** argv )
{
	err_if( argc < 2 || argc > 3, "usage: gcat [+d] file" );

	if( argc == 2 ) {
		err_if( argv[1][0] == '+', "usage: gcat [+d] file" );
		list( argv[1] );
	}

	else {
		err_if( argv[1][0] != '+' && argv[1][1] != 'd' && argv[1][2] != 0,
				"usage: gcat [+d] file" );
		fdp( argv[2] );
	}

	return EXIT_SUCCESS;
}
