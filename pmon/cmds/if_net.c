#include <machine/pio.h>
#include <stdio.h>
#include <pmon.h>
#include <cpu.h>

#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <file.h>

#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/net/if.h>
#include <sys/netinet/in.h>
#include <sys/netinet/in_var.h>
#include <sys/net/route.h>

#define SIN(x) ((struct sockaddr_in *)&(x))

static void setsin(struct sockaddr_in *sa, int family, u_long addr)
{
    bzero (sa, sizeof (*sa));
    sa->sin_len = sizeof (*sa);
    sa->sin_family = family;
    sa->sin_addr.s_addr = addr;
}

static int mydelrt(struct radix_node *rn, void *w)
{
	struct rtentry *rt = (struct rtentry *)rn;

	if (*(char *)w=='*'||rt->rt_ifp) {
		if(*(char *)w=='*'||!strcmp(rt->rt_ifp->if_xname,(char *)w)) {
			rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway, rt_mask(rt), 0, 0);
		}
	}

	return 0;
}

/*
 * Function to print all the route trees.
 * Use this from ddb:  "call db_show_arptab"
 */
static int del_if_rt(char *ifname)
{
	struct radix_node_head *rnh;

	rnh = rt_tables[AF_INET];
	rn_walktree(rnh, mydelrt, ifname);
	if (rnh == NULL) {
		printf(" (not initialized)\n");
		return 0;
	}
	return 0;
}

static int cmd_ifconfig(int argc, char **argv)
{
	struct ifreq *ifr;
	struct in_aliasreq *ifra;
	struct in_aliasreq data;
	int s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s < 0) {
		perror("ifconfig: socket");
		return -1;
	}
	ifra = (void *)&data;
	ifr = (void *)&data;
	bzero(ifra, sizeof(*ifra));
	strcpy(ifr->ifr_name,argv[1]);
	if (argc == 2) {
		(void) ioctl(s, SIOCGIFADDR, ifr);
		printf("ip:%s\n",inet_ntoa(satosin(&ifr->ifr_addr)->sin_addr));
		(void) ioctl(s,SIOCGIFNETMASK, ifr);
		printf("netmask:%s\n",inet_ntoa(satosin(&ifr->ifr_addr)->sin_addr));
		(void) ioctl(s,SIOCGIFBRDADDR, ifr);
		printf("boradcast:%s\n",inet_ntoa(satosin(&ifr->ifr_addr)->sin_addr));
		(void) ioctl(s,SIOCGIFFLAGS,ifr);
		printf("status:%s %s\n",ifr->ifr_flags&IFF_UP?"up":"down",ifr->ifr_flags&IFF_RUNNING?"running":"stoped");
	}
	else if (argc >= 3) {
		char *cmds[]={"down","up","remove","stat","setmac","readrom","writerom"};
		int i;
		for (i=0; i<sizeof(cmds)/sizeof(char *); i++)
			if (!strcmp(argv[2],cmds[i]))
				break;
		switch(i) {
		case 0://down
			(void) ioctl(s,SIOCGIFFLAGS,ifr);
			ifr->ifr_flags &=~IFF_UP;
			(void) ioctl(s,SIOCSIFFLAGS,ifr);
			break;
		case 1://up
			(void) ioctl(s,SIOCGIFFLAGS,ifr);
			ifr->ifr_flags |=IFF_UP;
			(void) ioctl(s,SIOCSIFFLAGS,ifr);
			break;
		case 2://remove
			(void) ioctl(s,SIOCGIFFLAGS,ifr);
			ifr->ifr_flags &=~IFF_UP;
			(void) ioctl(s,SIOCSIFFLAGS,ifr);
			while(ioctl(s, SIOCGIFADDR, ifra)==0) {
				(void) ioctl(s, SIOCDIFADDR, ifr);
			}
			del_if_rt(argv[1]);
			break;
		case 3: //stat
			{
			register struct ifnet *ifp;
			ifp = ifunit(argv[1]);
			if(!ifp) {
				printf("can not find dev %s.\n", argv[1]);
				return -1;
			}
			printf("RX packets:%d,TX packets:%d,collisions:%d\n" \
					"RX errors:%d,TX errors:%d\n" \
					"RX bytes:%d TX bytes:%d\n" ,
					ifp->if_ipackets, 
					ifp->if_opackets, 
					ifp->if_collisions,
					ifp->if_ierrors, 
					ifp->if_oerrors,  
					ifp->if_ibytes, 
					ifp->if_obytes);
			if (ifp->if_baudrate)
				printf("link speed up to %d Mbps\n", ifp->if_baudrate);
			}
			break;
		case 4: //setmac
			{
			struct ifnet *ifp;
			ifp = ifunit(argv[1]);
			if (ifp) {
				char arg[2] = {argc-2, (long)&argv[2]};
				ifp->if_ioctl(ifp, SIOCETHTOOL, arg);
			}
			}
			break;
		case 5: //read eeprom
			{
			struct ifnet *ifp;
			ifp = ifunit(argv[1]);
			if (ifp) {
				char arg[2] = {argc-2, (long)&argv[2]};
				ifp->if_ioctl(ifp, SIOCRDEEPROM, arg);
			}
			}
			break;
		case 6: //write eeprom
			{
			struct ifnet *ifp;
			ifp = ifunit(argv[1]);
			if (ifp) {
				char arg[2] = {argc-2, (long)&argv[2]};
				ifp->if_ioctl(ifp, SIOCWREEPROM, arg);
			}
			}
			break;

		default:
			while (ioctl(s, SIOCGIFADDR, ifra)==0) {
				(void) ioctl(s, SIOCDIFADDR, ifr);
			}
			setsin (SIN(ifra->ifra_addr), AF_INET, inet_addr(argv[2]));
			(void) ioctl(s, SIOCSIFADDR, ifra);
			if (argc >= 4) {
				setsin (SIN(ifra->ifra_addr), AF_INET, inet_addr(argv[3]));
				(void) ioctl(s,SIOCSIFNETMASK, ifra);
			}
			break;
		}
	}
	close(s);
	return 0;
}

static int cmd_ifdown(int argc, char **argv)
{
	struct ifreq ifr;

	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		return -1;
	}
	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name,argv[1]);
	(void)ioctl(s, SIOCGIFADDR, &ifr);
	printf("%s", inet_ntoa(satosin(&ifr.ifr_addr)->sin_addr));
	(void)ioctl(s, SIOCDIFADDR, &ifr);
	ifr.ifr_flags=0;
	(void)ioctl(s,SIOCSIFFLAGS, (void *)&ifr);
	close(s);
	return 0;
}

static int cmd_ifup(int argc,char **argv)
{
	struct ifreq ifr;
	int s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s < 0) {
		perror("ifconfig: socket");
		return -1;
	}
	bzero (&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name,argv[1]);
	ifr.ifr_flags=IFF_UP;
	(void) ioctl(s,SIOCSIFFLAGS,(void *)&ifr);
	close(s);
	return 0;
}


#if 0
static int cmd_testnet(int argc, char **argv)
{
	char buf[100];
	int i,j;
	int s;
	struct sockaddr addr;

	if (argc < 3)
		return -1;
	addr.sa_len = sizeof(addr);
	strcpy(addr.sa_data, argv[1]);

	s = socket(AF_UNSPEC, SOCK_RAW, 0);
	if (s == -1) {
		printf("please select raw_ether\n");
		return -1;
	}
	if (!strcmp(argv[2], "send")) {
		for (j=0; ; j++) {
			memset(buf,0xff,12);
			buf[12]=8;buf[13]=0;
			for (i=14; i<100; i++)
				buf[i] = i + j;
			sendto(s, buf, 100, 0, &addr, sizeof(addr));
			delay1(500);
			printf("%d\r", j);
		}
	}
	else if (!strcmp(argv[2],"recv")) {
		bind(s, &addr, sizeof(addr));
		while (1) {
			unsigned char buf[1500];
			int len;
			len = recv(s, buf, 1500, 0);
			for (i=0; i<len; i++) {
				if ((i&15) == 0)
					printf("\n%02x: ", i);
				printf("%02x ", buf[i]);
			}
		}
	}
	else {
		int errors = 0;
		bind(s, &addr, sizeof(addr));
		while (1) {
			unsigned char buf[1500];
			int len;
			for (j=0; ; j++) {
				memset(buf,0xff,12);
				buf[12]=8;buf[13]=0;
				for (i=14; i<100; i++)
					buf[i]=i-12+j;
				sendto(s, buf, 100, 0, &addr, sizeof(addr));
				len = recv(s, buf, 100, 0);
				for (i=12; i<100-4; i++) {
					if (buf[i] != (i-12+j))
						break;
				}

				if (i == (100-4)) {
					printf("\r%d,%d",j,errors);
				}
				else {
					errors++;
					for (i=0; i<len; i++) {
						if((i&15)==0)printf("\n%02x: ",i);
						printf("%02x ",buf[i]);
					}
				}
				delay1(500);
			}
		}
	}
	close(s);
	return 0;
}

static int cmd_rtlist(int argc, char **argv)
{
	db_show_arptab();
	return 0;
}

static int cmd_rtdel(int argc, char **argv)
{
	struct sockaddr dst;
	bzero(&dst,sizeof(dst));
	setsin (SIN(dst), AF_INET, inet_addr("10.0.0.3"));
	rtrequest(RTM_DELETE, &dst, 0, 0, 0,0);
	return 0;
}
#endif

static const Cmd Cmds[] = {
	{"Network"},
#ifdef INET /*THF*/
	{"ifconfig", "ifname", 0, "ifconig fx0 [up|down|remove|stat|setmac|readrom|setrom|addr [netmask]", cmd_ifconfig, 2, 99, CMD_REPEAT},
	{"ifup", "ifname", 0, "ifup fxp0", cmd_ifup, 2, 99, CMD_REPEAT},
	{"ifdown", "ifname", 0, "ifdown fxp0", cmd_ifdown, 2, 99, CMD_REPEAT},
//	{"testnet",	"", 0, "testnet rtl0 [recv|send|loop]", cmd_testnet, 0, 99, CMD_REPEAT},
//	{"rtlist", "", 0, "rtlist", cmd_rtlist, 0, 99, CMD_REPEAT},
//	{"rtdel", "", 0, "rtdel", cmd_rtdel, 0, 99, CMD_REPEAT},
#endif
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
