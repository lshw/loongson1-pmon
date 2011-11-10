#include <sys/param.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/endian.h>

#ifdef	KERNEL
#undef	KERNEL
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#endif

#include <termio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>

#include "packet.h"
#include "lwdhcp.h"
#include "options.h"

void init_header(struct dhcp_packet *packet, char type)
{
	memset(packet, 0, sizeof(struct dhcp_packet));
	switch (type) {
		case DHCPDISCOVER:
		case DHCPREQUEST:
		case DHCPRELEASE:
		case DHCPINFORM:
			packet->op = BOOTREQUEST;
			break;
		case DHCPOFFER:
		case DHCPACK:
		case DHCPNAK:
			packet->op = BOOTREPLY;
	}
	packet->htype = ETH_10MB;
	packet->hlen = ETH_10MB_LEN;
	packet->cookie = htonl(DHCP_MAGIC);
	packet->options[0] = DHCP_END;
	add_simple_option(packet->options, DHCP_MESSAGE_TYPE, type);
}


/* initialize a packet with the proper defaults */
static void init_packet(struct dhcp_packet *packet, char type)
{
	init_header(packet, type);

	memcpy(packet->chaddr, client_config.arp, 6);
}


uint16_t checksum(void *addr, int count)
{
    /* Compute Internet Checksum for "count" bytes
     *         beginning at location "addr".
     */
    register int32_t sum = 0;
    uint16_t *source = (uint16_t *) addr;

    while (count > 1)  {
        /*  This is the inner loop */
        sum += *source++;
        count -= 2;
    }

    /*  Add left-over byte, if any */
    if (count > 0) {
        /* Make sure that the left-over byte is added correctly both
         * with little and big endian hosts */
        uint16_t tmp = 0;
        *(uint8_t *) (&tmp) = * (uint8_t *) source;
        sum += tmp;
    }
    /*  Fold 32-bit sum to 16 bits */
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}



#ifndef			PMON
int raw_packet(struct dhcp_packet *payload, uint32_t source_ip, int source_port,
           uint32_t dest_ip, int dest_port, uint8_t *dest_arp, int ifindex)
{
    int fd;
    int result;
    struct sockaddr_ll dest;
    struct udp_dhcp_packet packet;

    if ((fd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) {
        perror("socket call failed");
        return -1;
    }

    memset(&dest, 0, sizeof(dest));
    memset(&packet, 0, sizeof(packet));

    dest.sll_family = AF_PACKET;
    dest.sll_protocol = htons(ETH_P_IP);
    dest.sll_ifindex = ifindex;
    dest.sll_halen = 6;
    memcpy(dest.sll_addr, dest_arp, 6);
    if (bind(fd, (struct sockaddr *)&dest, sizeof(struct sockaddr_ll)) < 0) 
	{
		perror("bind call failed");
		close(fd);
		return -1;
	}
	packet.ip.protocol = IPPROTO_UDP;
	packet.ip.saddr = source_ip;
	packet.ip.daddr = dest_ip;
	packet.udp.source = htons(source_port);
	packet.udp.dest = htons(dest_port);
	packet.udp.len = htons(sizeof(packet.udp) + sizeof(struct dhcp_packet)); /* cheat on the psuedo-header */
	packet.ip.tot_len = packet.udp.len;
	memcpy(&(packet.data), payload, sizeof(struct dhcp_packet));
	packet.udp.check = checksum(&packet, sizeof(struct udp_dhcp_packet));

	packet.ip.tot_len = htons(sizeof(struct udp_dhcp_packet));
	packet.ip.ihl = sizeof(packet.ip) >> 2;
	packet.ip.version = IPVERSION;
	packet.ip.ttl = IPDEFTTL;
	packet.ip.check = checksum(&(packet.ip), sizeof(packet.ip));

	result = sendto(fd, &packet, sizeof(struct udp_dhcp_packet), 0, (struct sockaddr *) &dest, sizeof(dest));
	if (result <= 0) {
		perror("write on socket failed");
	}
	close(fd);
	return result;
}
#else
int raw_packet(struct dhcp_packet *payload, uint32_t source_ip, int source_port,
           uint32_t dest_ip, int dest_port, uint8_t *dest_arp, int ifindex)
{
	int		n;
	int		sock;
	struct sockaddr_in	clnt, srvr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror ("bootp socket");
        return -1;
    }

    n = 1;
    if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, &n, sizeof (n)) < 0) {
        perror ("bootp setsockopt(BROADCAST)");
        close (sock);
        return -1;
    }
	/*
    if (setsockopt (sock, SOL_SOCKET, SO_DONTROUTE, &n, sizeof (n)) < 0) {
        perror ("bootp setsockopt(DONTROUTE)");
        close (sock);
        return -1;
    }*/

	bzero((char *)&clnt, sizeof(clnt));
	clnt.sin_family = AF_INET;
	clnt.sin_port = htons(CLIENT_PORT+2);
	clnt.sin_addr.s_addr = INADDR_ANY;

    if (bind (sock, (struct sockaddr *)&clnt, sizeof(clnt)) < 0) {
        PERROR ("bind failed");
        close (sock);
        return -1;
    }

	//change_promisc(sock, 0);
	
    bzero ((char *)&srvr, sizeof (srvr));
    srvr.sin_family = AF_INET;
    srvr.sin_addr.s_addr = INADDR_BROADCAST;
    srvr.sin_port = htons (SERVER_PORT);

	n = sendto(sock, payload, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&srvr, sizeof(srvr));
	if(n < 0)
	{
		PERROR("sendto failed");
		close(sock);
		return -1;
	}

	close(sock);

	return 0;
}

#endif


/* Add a parameter request list for stubborn DHCP servers. Pull the data
 * from the struct in options.c. Don't do bounds checking here because it
 * goes towards the head of the packet. */
static void add_requests(struct dhcp_packet *packet)
{
	int end = end_option(packet->options);
	int i, len = 0;

	packet->options[end + OPT_CODE] = DHCP_PARAM_REQ;
	for (i = 0; dhcp_options[i].code; i++)
		if (dhcp_options[i].flags & OPTION_REQ)
			packet->options[end + OPT_DATA + len++] = dhcp_options[i].code;
	packet->options[end + OPT_LEN] = len;
	packet->options[end + OPT_DATA + len] = DHCP_END;

}

int send_request(unsigned long xid, uint32_t server, uint32_t requested_ip)
{
	struct dhcp_packet packet;

	init_packet(&packet, DHCPREQUEST);
	packet.xid = xid;

	add_simple_option(packet.options, DHCP_SERVER_ID, server);
	add_simple_option(packet.options, DHCP_REQUESTED_IP, requested_ip);

	DbgPrint("Sending DHCPREQUEST...\n");

	return raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST, 
			SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
}

int send_discover(unsigned long xid)
{
	struct dhcp_packet packet;

	init_packet(&packet, DHCPDISCOVER);
	packet.xid = xid;

	DbgPrint("Sending DHCPDISCOVER...\n");
	return raw_packet(&packet, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
			SERVER_PORT, MAC_BCAST_ADDR, client_config.ifindex);
}

