#include <stdio.h>

main()
{
int	c;

	for( ;; ) {
		if((c = getchar()) == EOF )
			break;
	        c ^= 0xff;
		putchar(c);
	}
}

