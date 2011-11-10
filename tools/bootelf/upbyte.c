
#include <stdio.h>

main()
{
int	c;

	for( ;; ) {
		if( (c = getchar()) == EOF )
			break;
		putchar(c);
		if( getchar() == EOF )
			break;
	}
}

