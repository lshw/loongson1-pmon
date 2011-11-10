/////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                 //	
//                                  Light weight DHCP client                                       //
//                                                                                                 //
/////////////////////////////////////////////////////////////////////////////////////////////////////


#include <sys/param.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/endian.h>

#ifdef		PMON
	#ifdef		KERNEL
	#undef		KERNEL
		#include <sys/socket.h>
		#include <sys/ioctl.h>
		#include <netinet/in.h>
	#else
		#include <sys/socket.h>
		#include <sys/ioctl.h>
		#include <netinet/in.h>
	#endif

	#define	 KERNEL
	#include <pmon/netio/bootp.h>
	#include <sys/types.h>
	#include <sys/net/if.h>
#else
	#include <linux/if_packet.h>
	#include <linux/if_ether.h>
	#include <net/if.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
#endif

#include <pmon.h>
#include <setjmp.h>
#include <signal.h>

#include "lwdhcp.h"
#include "packet.h"
#include "options.h"

struct client_config_t  client_config;
int 	dhcp_request;
int		fd = 0;
sig_t	pre_handler = 0;

static jmp_buf  jmpb;

static void terminate()
{
	DbgPrint("Program terminated by user.\n");
	dhcp_request = 0;
	
	if(fd > 0)
		close(fd);
	
	longjmp(jmpb, 1);
}

static void	init()
{
	memset((void *)&client_config, 0, sizeof(struct client_config_t));

	strcpy(client_config.interface, "rtl0");

	pre_handler = signal(SIGINT, (sig_t)terminate);
}

int		listen_socket()
{
	int		sock, n;
	int		flag;
	int		dwValue;
	struct ifreq 	ifr;
	struct sockaddr_in		clnt;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0)
	{
		PERROR("socket create error");
		return sock;
	}

	bzero((char*)&clnt, sizeof(clnt));
	clnt.sin_family = AF_INET;
	clnt.sin_port = htons(CLIENT_PORT);
	clnt.sin_addr.s_addr = INADDR_ANY;

	if (bind (sock, (struct sockaddr *)&clnt, sizeof(struct sockaddr_in)) < 0) {
		PERROR ("bind failed");
		close (sock);
		return -1;
	}

	flag = 1;
	setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(flag)); 

	n = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&n, sizeof(n));

	return sock;
}


int read_interface(char *interface, int *ifindex, uint32_t *addr, uint8_t *arp)
{
	int fd;
	struct ifreq ifr;
	struct sockaddr_in *our_ip;

	memset(&ifr, 0, sizeof(struct ifreq));
	if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) 
	{
		ifr.ifr_addr.sa_family = AF_INET;

		strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);

		if (addr) 
		{
			if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) 
			{
				our_ip = (struct sockaddr_in *) &ifr.ifr_addr;
				*addr = our_ip->sin_addr.s_addr;
				DbgPrint("%s (our ip) = %s\n", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
			} else 
			{
				PERROR("SIOCGIFADDR failed, is the interface up and configured?\n");
				close(fd);
				return -1;
			}
		}

		//the following code needs to change the PMON source code pmon/pmon/netio/bootp.c line 101
		//int
		//getethaddr (unsigned char *eaddr, char *ifc)
		//{
		//    struct ifnet *ifp;
		//        struct ifaddr *ifa;
		// ......
#ifndef		PMON
		if(ioctl(fd, SIOCGIFINDEX, &ifr) == 0) 
		{
			DbgPrint("adapter index %d\n", ifr.ifr_ifindex);
			*ifindex = ifr.ifr_ifindex;
		} else 
		{
			PERROR("SIOCGIFINDEX failed!\n");
			close(fd);
			return -1;
		}
#endif

#ifndef		PMON
		if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) 
		{
			memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
#else
		if(getethaddr(arp, client_config.interface) >= 0)
		{
#endif
			DbgPrint("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x\n",
					arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
		} 
		else{
			PERROR("SIOCGIFHWADDR failed!\n");
			close(fd);
			return -1;
		}
	} 
	else {
		PERROR("socket failed!\n");
		return -1;
	}
	close(fd);
	return 0;
}


int lwdhcp(int argc, char* argv[])
{
	int						xid;
	fd_set					fs;
	int						ret, totimes;
	char					buf[1500];
	struct sockaddr_in		from;
	int						size = sizeof(from);
	struct dhcp_packet*		p;
	uint8_t*				dhcp_message_type;
	struct	timeval			tv;

	if(getuid())
	{
		DbgPrint("Only root can run this program!\n");
		return 0;
	}

	DbgPrint("Light weight DHCP client starts...\n");
	init();

	if(setjmp(jmpb))
	{
		signal(SIGINT, pre_handler);
		sigsetmask(0);
		return 0;
	}

	
	if(read_interface(client_config.interface, &client_config.ifindex,
				&client_config.addr, client_config.arp) < 0)
	{
		DbgPrint("read_interface error");
		return 0;
	}

	if((fd = listen_socket()) < 0)
	{
		return 0;
	}

	//srand(time(NULL));
	//xid = rand();
	totimes = 3;

tryagain:
	if(send_discover(xid) < 0)
		if(--totimes > 0)
			goto tryagain;
		else
		{
			DbgPrint("Fail to send DHCPDISCOVER...\n");
			return 0;
		}

	FD_ZERO(&fs);
	FD_SET(fd, &fs);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	//receiving DHCPOFFER
	while(1)
	{
		dhcp_request = 1;
		ret = select(fd + 1, &fs, NULL, NULL, &tv);
		
		if(ret == -1)
			PERROR("select error");
		else if(ret == 0)
		{
			if(--totimes > 0)
				goto tryagain;
			else
			{
				dhcp_request = 0;
				DbgPrint("Fail to get IP from DHCP server, it seems that there is no DHCP server.\n");
				close(fd);
				return 0;
			}
		}

		size = sizeof(from);
		if(recvfrom(fd, buf, sizeof(struct dhcp_packet), (struct sockaddr *)0, &from, &size) < 0)
			continue;
		
		dhcp_request = 0;

		p = (struct dhcp_packet *)buf;

		if(p->xid != xid)
			continue;

		dhcp_message_type = get_dhcp_option(p, DHCP_MESSAGE_TYPE);
		if(!dhcp_message_type)
			continue;
		else if(*dhcp_message_type != DHCPOFFER)
			continue;

		DbgPrint("DHCPOFFER received...\n");
		break;
	}

	//sending DHCPREQUEST
	send_request(xid, *((uint32_t*)(&(p->siaddr))), *((uint32_t*)(&p->yiaddr)));

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	//receiving DHCPACK
	while(1)
	{
		dhcp_request = 1;
		ret = select(fd + 1, &fs, NULL, NULL, &tv);
		
		if(ret == -1)
			PERROR("select error");
		else if(ret == 0)
		{
			if(--totimes > 0)
				goto tryagain;
			else
			{
				dhcp_request = 0;
				DbgPrint("Fail to get IP from DHCP server, no ACK from DHCP server.\n");
				close(fd);
				return 0;
			}
		}

		//get_raw_packet(buf, fd);
		
		size = sizeof(struct sockaddr);
		recvfrom(fd, buf, sizeof(struct dhcp_packet), 0, (struct sockaddr*)&from, &size);

		dhcp_request = 0;
		
		if(p->xid != xid)
			continue;

		dhcp_message_type = get_dhcp_option(p, DHCP_MESSAGE_TYPE);
		if(!dhcp_message_type)
			continue;
		else if(*dhcp_message_type != DHCPACK)
			continue;

		DbgPrint("DHCPACK received...\n");
		DbgPrint("IP %s obtained from the DHCP server.\n", inet_ntoa(p->yiaddr));
		break;
	}

	close(fd);

	return 0;
}



/*
 *  Command table registration
 *  ==========================
 */

static const Cmd Cmds[] =
{
	{"Network"},
	{"lwdhcp",    "", 0,
		"Light weight DHCP client",
		lwdhcp, 1, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

	static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}





