#include <stdio.h>

int main(int argc, char**argv)
{
	FILE *fi,*fo;
	int ch;
	long i=0;
	
	if (argc<3)
		return -1;
	fi = fopen(argv[1], "rb");
	fo = fopen(argv[2], "w");
	if (!fi||!fo) {
		fcloseall();
		return -2;
	}
	while ((ch=fgetc(fi))!=EOF) {
		if (!(i%16))
			fprintf(fo,"\n");
		fprintf(fo,"0x%02X, ",ch);
		i++;
	}
	fcloseall();
	return 0;
}
