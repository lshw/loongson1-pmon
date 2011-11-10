#ifndef		__LWDHCP_PACKET__
#define		__LWDHCP_PACKET__

#undef		KERNEL
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <netinet/udp.h>


/* DHCP protocol -- see RFC 2131 */
#define SERVER_PORT     67
#define CLIENT_PORT     68

#define DHCP_MAGIC      0x63825363

/* DHCP option codes (partial list) */
#define DHCP_PADDING        0x00
#define DHCP_SUBNET     0x01
#define DHCP_TIME_OFFSET    0x02
#define DHCP_ROUTER     0x03
#define DHCP_TIME_SERVER    0x04
#define DHCP_NAME_SERVER    0x05
#define DHCP_DNS_SERVER     0x06
#define DHCP_LOG_SERVER     0x07
#define DHCP_COOKIE_SERVER  0x08
#define DHCP_LPR_SERVER     0x09
#define DHCP_HOST_NAME      0x0c
#define DHCP_BOOT_SIZE      0x0d
#define DHCP_DOMAIN_NAME    0x0f
#define DHCP_SWAP_SERVER    0x10
#define DHCP_ROOT_PATH      0x11
#define DHCP_IP_TTL     0x17
#define DHCP_MTU        0x1a
#define DHCP_BROADCAST      0x1c
#define DHCP_NTP_SERVER     0x2a
#define DHCP_WINS_SERVER    0x2c
#define DHCP_REQUESTED_IP   0x32
#define DHCP_LEASE_TIME     0x33
#define DHCP_OPTION_OVER    0x34
#define DHCP_MESSAGE_TYPE   0x35
#define DHCP_SERVER_ID      0x36
#define DHCP_PARAM_REQ      0x37
#define DHCP_MESSAGE        0x38
#define DHCP_MAX_SIZE       0x39
#define DHCP_T1         0x3a
#define DHCP_T2         0x3b
#define DHCP_VENDOR     0x3c
#define DHCP_CLIENT_ID      0x3d
#define DHCP_FQDN       0x51

#define DHCP_END        0xFF


#define BOOTREQUEST     1
#define BOOTREPLY       2

#define ETH_10MB        1
#define ETH_10MB_LEN        6

#define DHCPDISCOVER        1
#define DHCPOFFER       2
#define DHCPREQUEST     3
#define DHCPDECLINE     4
#define DHCPACK         5
#define DHCPNAK         6
#define DHCPRELEASE     7
#define DHCPINFORM      8

#define BROADCAST_FLAG      0x8000

#define OPTION_FIELD        0
#define FILE_FIELD      1
#define SNAME_FIELD     2


#define DHCP_UDP_OVERHEAD	(20 + /* IP header */			\
		8)   /* UDP header */
#define DHCP_SNAME_LEN		64
#define DHCP_FILE_LEN		128
#define DHCP_FIXED_NON_UDP	236
#define DHCP_FIXED_LEN		(DHCP_FIXED_NON_UDP + DHCP_UDP_OVERHEAD)
/* Everything but options. */
#define DHCP_MTU_MAX		1500
#define DHCP_OPTION_LEN		(308)		//(DHCP_MTU_MAX - DHCP_FIXED_LEN)

#define BOOTP_MIN_LEN		300
#define DHCP_MIN_LEN            548



/* miscellaneous defines */
#define MAC_BCAST_ADDR      (uint8_t *) "\xff\xff\xff\xff\xff\xff"
#define OPT_CODE 0
#define OPT_LEN 1
#define OPT_DATA 2




struct dhcp_packet {
	u_int8_t  				op;		/* 0: Message opcode/type */
	u_int8_t  				htype;	/* 1: Hardware addr type (net/if_types.h) */
	u_int8_t  				hlen;		/* 2: Hardware addr length */
	u_int8_t  				hops;		/* 3: Number of relay agent hops from client */
	u_int32_t 				xid;		/* 4: Transaction ID */
	u_int16_t 				secs;		/* 8: Seconds since client started looking */
	u_int16_t 				flags;	/* 10: Flag bits */
	struct in_addr 			ciaddr;	/* 12: Client IP address (if already in use) */
	struct in_addr 			yiaddr;	/* 16: Client IP address */
	struct in_addr 			siaddr;	/* 18: IP address of next server to talk to */
	struct in_addr 			giaddr;	/* 20: DHCP relay agent IP address */
	unsigned char 			chaddr [16];	/* 24: Client hardware address */
	char 					sname [DHCP_SNAME_LEN];	/* 40: Server name */
	char 					file [DHCP_FILE_LEN];	/* 104: Boot filename */
	u_int32_t				cookie;
	unsigned char options [DHCP_OPTION_LEN - 4];
	/* 212: Optional parameters
	   (actual length dependent on MTU). */
};

#define		IPVERSION 		4

struct iphdr
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int ihl:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;
	unsigned int ihl:4;
#else
# error "Please fix <bits/endian.h>"
#endif
	u_int8_t tos;
	u_int16_t tot_len;
	u_int16_t id;
	u_int16_t frag_off;
	u_int8_t ttl;
	u_int8_t protocol;
	u_int16_t check;
	u_int32_t saddr;
	u_int32_t daddr;
	/*The options start here. */
};


/* UDP header as specified by RFC 768, August 1980. */
#ifdef __FAVOR_BSD

struct udphdr
{
	u_int16_t uh_sport;       /* source port */
	u_int16_t uh_dport;       /* destination port */
	u_int16_t uh_ulen;        /* udp length */
	u_int16_t uh_sum;     /* udp checksum */
};

#else

struct udphdr
{
	u_int16_t source;
	u_int16_t dest;
	u_int16_t len;
	u_int16_t check;
};
#endif




struct udp_dhcp_packet {
	struct iphdr ip;
	struct udphdr udp;
	struct dhcp_packet data;
};


/* BOOTP (rfc951) message types */
#define	BOOTREQUEST	1
#define BOOTREPLY	2

/* Possible values for flags field... */
#define BOOTP_BROADCAST 32768L

/* Possible values for hardware type (htype) field... */
#define HTYPE_ETHER	1               /* Ethernet 10Mbps              */
#define HTYPE_IEEE802	6               /* IEEE 802.2 Token Ring...	*/
#define HTYPE_FDDI	8		/* FDDI...			*/

/* Magic cookie validating dhcp options field (and bootp vendor
   extensions field). */
#define DHCP_OPTIONS_COOKIE	"\143\202\123\143"

/* DHCP Option codes: */

#define DHO_PAD				0
#define DHO_SUBNET_MASK			1
#define DHO_TIME_OFFSET			2
#define DHO_ROUTERS			3
#define DHO_TIME_SERVERS		4
#define DHO_NAME_SERVERS		5
#define DHO_DOMAIN_NAME_SERVERS		6
#define DHO_LOG_SERVERS			7
#define DHO_COOKIE_SERVERS		8
#define DHO_LPR_SERVERS			9
#define DHO_IMPRESS_SERVERS		10
#define DHO_RESOURCE_LOCATION_SERVERS	11
#define DHO_HOST_NAME			12
#define DHO_BOOT_SIZE			13
#define DHO_MERIT_DUMP			14
#define DHO_DOMAIN_NAME			15
#define DHO_SWAP_SERVER			16
#define DHO_ROOT_PATH			17
#define DHO_EXTENSIONS_PATH		18
#define DHO_IP_FORWARDING		19
#define DHO_NON_LOCAL_SOURCE_ROUTING	20
#define DHO_POLICY_FILTER		21
#define DHO_MAX_DGRAM_REASSEMBLY	22
#define DHO_DEFAULT_IP_TTL		23
#define DHO_PATH_MTU_AGING_TIMEOUT	24
#define DHO_PATH_MTU_PLATEAU_TABLE	25
#define DHO_INTERFACE_MTU		26
#define DHO_ALL_SUBNETS_LOCAL		27
#define DHO_BROADCAST_ADDRESS		28
#define DHO_PERFORM_MASK_DISCOVERY	29
#define DHO_MASK_SUPPLIER		30
#define DHO_ROUTER_DISCOVERY		31
#define DHO_ROUTER_SOLICITATION_ADDRESS	32
#define DHO_STATIC_ROUTES		33
#define DHO_TRAILER_ENCAPSULATION	34
#define DHO_ARP_CACHE_TIMEOUT		35
#define DHO_IEEE802_3_ENCAPSULATION	36
#define DHO_DEFAULT_TCP_TTL		37
#define DHO_TCP_KEEPALIVE_INTERVAL	38
#define DHO_TCP_KEEPALIVE_GARBAGE	39
#define DHO_NIS_DOMAIN			40
#define DHO_NIS_SERVERS			41
#define DHO_NTP_SERVERS			42
#define DHO_VENDOR_ENCAPSULATED_OPTIONS	43
#define DHO_NETBIOS_NAME_SERVERS	44
#define DHO_NETBIOS_DD_SERVER		45
#define DHO_NETBIOS_NODE_TYPE		46
#define DHO_NETBIOS_SCOPE		47
#define DHO_FONT_SERVERS		48
#define DHO_X_DISPLAY_MANAGER		49
#define DHO_DHCP_REQUESTED_ADDRESS	50
#define DHO_DHCP_LEASE_TIME		51
#define DHO_DHCP_OPTION_OVERLOAD	52
#define DHO_DHCP_MESSAGE_TYPE		53
#define DHO_DHCP_SERVER_IDENTIFIER	54
#define DHO_DHCP_PARAMETER_REQUEST_LIST	55
#define DHO_DHCP_MESSAGE		56
#define DHO_DHCP_MAX_MESSAGE_SIZE	57
#define DHO_DHCP_RENEWAL_TIME		58
#define DHO_DHCP_REBINDING_TIME		59
#define DHO_VENDOR_CLASS_IDENTIFIER	60
#define DHO_DHCP_CLIENT_IDENTIFIER	61
#define DHO_NWIP_DOMAIN_NAME		62
#define DHO_NWIP_SUBOPTIONS		63
#define DHO_USER_CLASS			77
#define DHO_FQDN			81
#define DHO_DHCP_AGENT_OPTIONS		82
#define DHO_SUBNET_SELECTION		118 /* RFC3011! */
/* The DHO_AUTHENTICATE option is not a standard yet, so I've
   allocated an option out of the "local" option space for it on a
   temporary basis.  Once an option code number is assigned, I will
   immediately and shamelessly break this, so don't count on it
   continuing to work. */
#define DHO_AUTHENTICATE		210

#define DHO_END				255

/* DHCP message types. */
#define DHCPDISCOVER	1
#define DHCPOFFER	2
#define DHCPREQUEST	3
#define DHCPDECLINE	4
#define DHCPACK		5
#define DHCPNAK		6
#define DHCPRELEASE	7
#define DHCPINFORM	8

/* Relay Agent Information option subtypes: */
#define RAI_CIRCUIT_ID	1
#define RAI_REMOTE_ID	2
#define RAI_AGENT_ID	3

/* FQDN suboptions: */
#define FQDN_NO_CLIENT_UPDATE		1
#define FQDN_SERVER_UPDATE		2
#define FQDN_ENCODED			3
#define FQDN_RCODE1			4
#define FQDN_RCODE2			5
#define FQDN_HOSTNAME			6
#define FQDN_DOMAINNAME			7
#define FQDN_FQDN			8
#define FQDN_SUBOPTION_COUNT		8


extern int send_request(unsigned long xid, uint32_t server, uint32_t requested_ip);
extern int send_discover(unsigned long xid);
extern int get_raw_packet(struct dhcp_packet *payload, int fd);

#endif		//	__LWDHCP_PACKET__
