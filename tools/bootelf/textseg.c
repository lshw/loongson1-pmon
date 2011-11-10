
#include <stdio.h>
#include <a.out.h>


static struct exec	a;


main(ac, av)
int	ac;
char	*av[];
{
FILE	*in;
int	c;
long	len;

	if( ac != 2 ) {
		fprintf(stderr, "use: %s file\n", av[0]);
		exit(1);
	}
	if( (in = fopen(av[1], "r")) == NULL ) {
		perror(av[1]);
		exit(1);
	}

	if( fread(&a, sizeof(a), 1, in) == 0 ) {
		fprintf(stderr, "fread fails\n");
		exit(1);
	}

#if 0
	len = a.a_text + a.a_data + sizeof(a);
#endif
	len = a.a_text + a.a_data;

	fprintf(stderr, "Entry    textsize datasize bsssize\n");
	fprintf(stderr, "%08X %08X %08X %08X\n",
	    a.a_entry, a.a_text, a.a_data, a.a_bss);

	/*
	 * fprintf(stderr, "text size $%lx\n", len);
	 */

	if( len > 0 ) do {
		if( (c = getc(in)) != EOF )
			putchar(c);
		else
			break;
	} while( --len > 0 );

	if( len > 0 )
		fprintf(stderr, "unexpected EOF on input, %d left\n", len);
	fclose(in);
}

