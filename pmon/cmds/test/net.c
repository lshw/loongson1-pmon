#include "sys/socket.h"
#include <sys/netinet/in.h>
#include <sys/net/if.h>
#include <sys/netinet/in_var.h>
#include <sys/sys/device.h>
#define SIN(x) ((struct sockaddr_in *)&(x))
extern char *devclass[];
extern int errno;
static int transmitted,received,loss;

static int pingfilter(char *s)
{
	if(strstr(s,"transmitted")){
		sscanf(s,"%d packets transmitted,%d packets received,%d%%",&transmitted,&received,&loss);
	}
	return 0;
}

static int myping(char *serverip)
{
	char cmd[100];
	filterstdout(pingfilter);
//	strcpy(cmd,"ping -c 3 10.0.0.3");
	sprintf(cmd,"ping -c 3 %s",serverip);
	do_cmd(cmd);
	filterstdout(0);
//	printf("%d %d %d%%\n",transmitted,received,loss);
	return loss;
}

static void
setsin (struct sockaddr_in *sa, int family, u_long addr)
{
    bzero (sa, sizeof (*sa));
    sa->sin_len = sizeof (*sa);
    sa->sin_family = family;
    sa->sin_addr.s_addr = addr;
}

#define TEST_LEN 256

static int net_looptest()
{
	unsigned char buf[1500];
	int len;
	int i,j,idx;
	int s;
	struct sockaddr addr;
	fd_set ifds;
	struct timeval timo;
	int errors=0;
	struct device *dev, *next_dev;
struct ifreq *ifr;
struct in_aliasreq *ifra;
struct in_aliasreq data;

	for (dev  = TAILQ_FIRST(&alldevs); dev != NULL; dev = next_dev) {
		next_dev = TAILQ_NEXT(dev, dv_list);
		if(dev->dv_class == DV_IFNET) {
		printf("%-12s %s\n", &dev->dv_xname, devclass[dev->dv_class]);
		}
	}

	for (dev  = TAILQ_FIRST(&alldevs); dev != NULL; dev = next_dev) {
		next_dev = TAILQ_NEXT(dev, dv_list);
		if(dev->dv_class != DV_IFNET) {
		continue;
		}

		errors=0;
	
	s = socket (AF_INET, SOCK_DGRAM, 0);
		ifr=(void *)&data;
		ifra=(void *)&data;
		bzero (ifra, sizeof(*ifra));
		strcpy(ifr->ifr_name,&dev->dv_xname);
		setsin (SIN(ifra->ifra_addr), AF_INET, inet_addr("0.0.0.0"));
		(void) ioctl(s, SIOCSIFADDR, ifra);
		close(s);


		delay1(8000);
		printf("test %s ...", &dev->dv_xname);


	addr.sa_len=sizeof(addr);
	strcpy(addr.sa_data,&dev->dv_xname);

	s= socket (AF_UNSPEC, SOCK_RAW, 0);
	if(s==-1){
	printf("please select raw_ether\n");
	return -1;
	}
		bind(s,&addr,sizeof(addr));
		for(j=0;j<3;j++)
		{
			memset(buf,0xff,12);
			buf[12]=8;buf[13]=0;
			for(i=14;i<TEST_LEN;i++) buf[i]=i-14+j;
			memset(buf+TEST_LEN,0,4);//crc
			sendto(s,buf,TEST_LEN+4,0,&addr,sizeof(addr));
		FD_ZERO(&ifds);
		FD_SET(s, &ifds);
		timo.tv_sec = 1; timo.tv_usec = 0;
		switch (select (s + 1, &ifds, 0, 0, &timo)) {
			case -1:
    			fprintf (stderr, "error,cannot select\n");
				goto error;
			case 0:
    			fprintf (stderr, "error,receive timeout,maybe no connection\n");
				goto error;
		}
			len=recv(s,buf,1500,0);
			for(i=14;i<len-4;i++) 
			{
			if(buf[i]!=i-14+j)break;
			}

			if(i<TEST_LEN)
			{
			errors++;
			printf("error,received data miscompare,maybe wrong connection\n");
			if(getenv("verbose"))
			{
				for(i=0;i<len;i++)
				{
					if((i&15)==0)printf("\n%02x: ",i);
					printf("%02x ",buf[i]);
				}
			printf("\n");
			}
			}
			delay1(500);
		}
		if(errors==0)printf("ok\n");
error:
	close(s);
		s = socket (AF_INET, SOCK_DGRAM, 0);
		(void) ioctl(s,SIOCGIFFLAGS,ifr);
		ifr->ifr_flags &=~(IFF_UP|IFF_OACTIVE);
		(void) ioctl(s,SIOCSIFFLAGS,ifr);
		close(s);
	}

	return 0;
}
