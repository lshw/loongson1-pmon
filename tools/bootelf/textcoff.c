
#include <stdio.h>

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

	if( fseek(in, 0x190, 0)) {
		fprintf(stderr, "fseek fails\n");
		exit(1);
	}

	do {
		if( (c = getc(in)) != EOF )
			putchar(c);
		else
			break;
	} while(c != EOF);

	fclose(in);
	return 0;
}

