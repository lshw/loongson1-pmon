/*
 * pp pin15-pin2
 *    pin13-pin3
 *    pin12-pin4
*/
static int pptest()
{
unsigned char c,d;
#ifdef DEVBD2F_FIREWALL
volatile unsigned char *p=0xbe000040;
#else
volatile unsigned char *p=0xbfd00378;
#endif
int errors=0;
printf("pptest now\n");
for(c=0;c<7;c++)
{
	p[0]=c;
	delay(500);
	d=p[1]>>3&7;
	delay(500);
	if(d!=c){printf("error: pin[15,13,12]!=pin[2,3,4](%03b!=%03b)\n",d,c);errors++;}
}
if(errors){
printf("pp test error\n");
printf("please make sure connection as bellow:\n pin15-pin2 \n pin13-pin3 \n pin12-pin4 \n");
}
else printf("pp test ok\n");
return 0;
}
