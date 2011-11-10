extern char *vgabh;
extern char *heaptop;
static void cprint(int y, int x,int width,char color, const char *text)
{
	register int i;
	char *dptr;

	dptr = (char *)(vgabh + (160*y) + (2*x));
	for (i=0;text && text[i]; i++) {
		*dptr = text[i];
		dptr[1]=color;
		dptr += 2;
		if(width && i==width-1)return;
        }
	if(width)
	{
		char *end=(char *)(vgabh + (160*y) + (2*(x+width)));
		for(;dptr<end;dptr+=2)
		{
		dptr[0]=0x20;
		dptr[1]=color;
		}
	}
}

static int display()
{
char str[100];
int i;
for(i=0;i<25*80;i++)
cprint(i/80,i%80,0,i,"a");
gets(str);
return 0;
}
