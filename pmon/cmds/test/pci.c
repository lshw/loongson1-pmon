static int (*oldwrite)(int fd,char *buf,int len)=0;
static char *buffer;
static int total;
extern void (*__msgbox)(int yy,int xx,int height,int width,char *msg);
void *restdout(int  (*newwrite) (int fd, const void *buf, size_t n));
static int newwrite(int fd,char *buf,int len)
{
memcpy(buffer+total,buf,len);
total+=len;
return len;
}

int pcitest()
{
char cmd[40];
oldwrite=restdout(newwrite);
strcpy(cmd,"pciscan");
total=0;
buffer=heaptop+0x100000;
do_cmd(cmd);
restdout(oldwrite);
buffer[total]='\n';
buffer[total+1]=0;
__msgbox(0,0,24,80,buffer);
printf("press any key to continue");
return 0;
}

