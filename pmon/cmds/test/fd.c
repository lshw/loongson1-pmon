//#include <sys/device.h>
//#include <sys/disk.h>
#include "fd.h"
#include <sys/buf.h>
#include <machine/param.h>
extern struct cfdriver fd_cd;
#define fdlookup(unit) (struct fd_softc *)device_lookup(&fd_cd, (unit))
extern void (*__msgbox)(int yy,int xx,int height,int width,char *msg);
void floppy_info();
	
int fdtest()
{
FILE *fp;
char *fname="/dev/disk/fd0";
unsigned char buf[512];
unsigned char buf1[512];
int j;
int errors;
int count=0;
printf("begin floppy test\n");
//---register test--
#if NFD
	floppy_info();
#endif
	fp=fopen(fname,"r+");
	fseek(fp,1024,SEEK_SET);
	count=fread(buf,512,1,fp);
	fclose(fp);
	if(count!=1)printf("floppy check error,maybe no floppy in drive\n",count);
	return -1;
//-----read write test-----
	printf("start floppy read write test\n");
	fp=fopen(fname,"r+");
	fseek(fp,1024,SEEK_SET);
	fread(buf,512,1,fp);
	fclose(fp);

	memset(buf1,0x5a,512);
	fp=fopen(fname,"r+");
	fseek(fp,1024,SEEK_SET);
	fwrite(buf1,512,1,fp);
	fclose(fp);

	fp=fopen(fname,"r+");
	fseek(fp,1024,SEEK_SET);
	fread(buf1,512,1,fp);
	fclose(fp);

	fp=fopen(fname,"r+");
	fseek(fp,1024,SEEK_SET);
	fwrite(buf,512,1,fp);
	fclose(fp);
	
	errors=0;
	for(j=0;j<512;j++)
	if(buf1[j]!=0x5a)
	{
		printf("read write test error,write 0x5a read %x\n",buf1[j]);
		errors++;
	}
	if(!errors)printf("floppy read write test ok\n");


return 0;
}
