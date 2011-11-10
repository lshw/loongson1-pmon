/*	$Id: tftpd.c,v 1.2 2006/09/01 15:27:43 pefo Exp $	*/
/*	$OpenBSD: tftpd.c,v 1.40 2005/03/10 10:22:32 claudio Exp $	*/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton <guyton@rand-unix>
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/device.h>
//#include <sys/exec_elf.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <crc32.h>

#include <sys/socket.h>
#undef _KERNEL
#include <netinet/in.h>
#define _KERNEL
#include <net/if.h>
#include <arpa/tftp.h>
#include <netdb.h>

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#define ELFMAG "\177ELF"

//#include <pmon/dev/gpiodefs.h>

#include "pmon.h"

#define	TIMEOUT		5
#define	MAX_TIMEOUTS	5
#define EXIT_CMD_OK 0   /* Command executed OK */
#define EXIT_CMD_FAIL   1   /* Failed, command displayed error message */
#define OACK    06          /* option acknowledgement */
int errno;
#define CMD_SPAWN   8   /* Run as a subprocess */
extern int memorysize;
#define memtop PHYS_TO_CACHED (memorysize)

int	cmd_tftpd(int argc, char *argv[]);

int	tftpd_server(void);
int	tftpd_exec(struct tftphdr *tp, int size);
void	nak(int error);
void	oack(void);

struct	formats;

struct tftphdr *w_init(void);
struct tftphdr *r_init(void);
int	validate_access(char *filename, int mode);
int	recvfile(struct formats *pf);
int	sendfile(struct formats *pf);
int	tftpd_select(int fd);
int	readit(FILE *file, struct tftphdr **dpp, int convert);
void	read_ahead(FILE *file, int convert);
int	writeit(FILE *file, struct tftphdr **dpp, int ct, int convert);
int	write_behind(FILE *file, int convert);
int	synchnet(int f);

struct {
	struct	sockaddr_storage s_in;
	struct	sockaddr_storage sock_listen;
	struct	sockaddr_storage sock_from;
	int	peer;
	int	rexmtval;
	int	max_rexmtval;
	int	pktsize;		/* Negotiated packet size */
	int	timeouts;
	int	trfsize;		/* Total number of bytes transfered */
	int	do_crc;			/* Set if CRC32 check should be done */
	int	flash_it;		/* Set if the loaded image should be flashed */
	long	offset;			/* Offset for flashing */
	int	verbose;		/* Be a little more chatty */
	FILE	*file;			/* Opened destination "file" */
} v;

const Optdesc cmd_tftpd_opts[] =
{
	{"-b", "run in background"},
	{"-c", "allow file creation"},
	{NULL}
};


#define	PKTSIZE	SEGSIZE+4		/* Default packet size */
#define	BLKSIZE (v.pktsize-4)		/* Current data size used */
#define	MAX_BLKSIZE 8192		/* Maximum data block size negotiated */

char	cmdbuf[PKTSIZE];
char	ackbuf[PKTSIZE];

int	secure;
int	cancreate = 1;


struct formats {
	const char	*f_mode;
	int	(*f_validate)(char *, int);
	int	(*f_send)(struct formats *);
	int	(*f_recv)(struct formats *);
	int	f_convert;
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
	{ NULL,		NULL,			NULL,		NULL,	  0 }
};
struct options {
	const char	*o_type;
	char		*o_request;
	int		o_reply;	/* turn into union if need be */
} options[] = {
	{ "tsize",	NULL, 0 },	/* OPT_TSIZE */
	{ "timeout",	NULL, 0 },	/* OPT_TIMEOUT */
	{ "blksize",	NULL, 0 },	/* OPT_BLKSIZE */
	{ NULL,		NULL, 0 }
};
enum opt_enum {
	OPT_TSIZE = 0,
	OPT_TIMEOUT,
	OPT_BLKSIZE
};

static char *go_args[] = { "client", NULL };

int
cmd_tftpd(int argc, char *argv[])
{
	char pmoncmdbuf[50];
	int c, i, result;

	bzero(&v, sizeof(v));
	options[OPT_TSIZE].o_request = NULL;
	options[OPT_TIMEOUT].o_request = NULL;
	options[OPT_BLKSIZE].o_request = NULL;
	options[OPT_TSIZE].o_reply = 0;
	options[OPT_TIMEOUT].o_reply = 0;
	options[OPT_BLKSIZE].o_reply = 0;

	while ((c = getopt(argc, argv, "bcs")) != -1) {
		switch (c) {
		case 'b':
		case 'c':
			cancreate = 1;
			break;
		case 's':
			secure = 1;
			break;
		default:
			break;
		}
	}

	do {
		if (getenv("tftpdverbose"))
			v.verbose = atoi(getenv("tftpdverbose"));
		printf("Starting tftp server\n");
		result = tftpd_server();
		if (v.verbose > 0) {
			fprintf(stderr, "tftpd: transfered %d bytes\n", v.trfsize);
			fprintf(stderr, "tftpd: done %d\n", result);
		}

		/*
		 * Check if this is a special upload. If so check
		 * crc and signature before launching.
		 */
		if (v.do_crc && result == 0) {
			if (v.flash_it) {
#if defined(USERFLASH_BASE)
				int crc;

				tgt_flashprogram((void *)(long)PA_TO_VA(USERFLASH_BASE + v.offset),
				    v.trfsize, (void *)CLIENTPC, 0);
				crc = crc32_check((void *)(long)PA_TO_VA(USERFLASH_BASE + v.offset),
				    v.trfsize);
				if (crc) {
					fprintf(stderr, "tftpd: flashing failed\n");
					continue;
				}
#endif
			} else if (strncmp((char *)CLIENTPC, ELFMAG, sizeof(ELFMAG) - 1) == 0) {
				/* ELF image. Copy to high memory and boot it */
				u_char *bootimage;
				bootimage = (u_char *)(((u_long)memtop - v.trfsize) & ~0x0f);
				bcopy((u_char *)CLIENTPC, bootimage, v.trfsize);
				sprintf(pmoncmdbuf, "boot /dev/ram@%p,%p", bootimage, v.trfsize);
				do_cmd(pmoncmdbuf);
			} else {
				/* Binary image, just run it */
				sprintf(pmoncmdbuf, "g -e %x", CLIENTPC);
				do_cmd(pmoncmdbuf);
			}
			return EXIT_CMD_OK;
		}
	} while (result >= 0);

	printf("tftpd: fatal error, exiting\n");
	return EXIT_CMD_FAIL;
}

int
tftpd_server()
{
	struct tftphdr *tp;
static	char cbuf[CMSG_SPACE(sizeof(struct sockaddr_storage))];
	struct cmsghdr *cmsg;
	struct msghdr msg;
	struct iovec iov;
	socklen_t j;
	struct timeval sel_tout;
	fd_set sel_set;
	int sel_maxfd;
	int listen_fd, result;
	int n = 0, on = 1;
#ifdef TFTPD_BCAST
	struct sockaddr_in bcast, send;
	int x, ec, elen, bcast_fd;
	char *envbuf;
#endif

	/*
	 *  Initialize program data
	 */
	v.rexmtval = TIMEOUT;
	v.max_rexmtval = 2*TIMEOUT;
	v.pktsize = PKTSIZE;

	/*
	 *  Create a socket and bind the tftp port
	 */
	do {
		struct device *dev, *next_dev;
		struct ifnet *ifp;
		struct ifaddr *ifa;
		struct sockaddr_in *s_in = (struct sockaddr_in *)&v.sock_listen;
		s_in->sin_family = AF_INET;
		s_in->sin_port = htons(69);	/* tftp listen port */
		s_in->sin_len = sizeof(struct sockaddr_in);

		/* Dig out network info. Use the first found active interface */
		for (dev = TAILQ_FIRST(&alldevs), ifa = NULL; dev != NULL; dev = next_dev) {
			next_dev = TAILQ_NEXT(dev, dv_list);
			if (dev->dv_class == DV_IFNET) {
				ifp = ifunit(dev->dv_xname);
				if (ifp->if_flags & IFF_UP) {
					for (ifa = ifp->if_addrlist.tqh_first; ifa != 0; ifa = ifa->ifa_list.tqe_next)
						if (ifa->ifa_addr->sa_family == AF_INET)
							break;
				}
			}
			if (ifa != NULL)
				break;
		}

		if (dev == NULL) {
			fprintf(stderr, "tftpd: no network found\n");
			return -1;
		}
		s_in->sin_addr = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
		if (v.verbose > 0)
			fprintf(stderr, "tftpd: using %s %s\n", dev->dv_xname, inet_ntoa(s_in->sin_addr));
	} while (0);

	listen_fd = socket(v.sock_listen.ss_family, SOCK_DGRAM, 0);
	if (listen_fd < 0) {
		return -1;
	}

	if (bind(listen_fd, (struct sockaddr *)&v.sock_listen, v.sock_listen.ss_len) < 0) {
		return -1;
        }

#ifdef TFTPD_BCAST
	/*
	 *  Create a socket for the broadcast message.
	 */
	bcast_fd = socket(v.sock_listen.ss_family, SOCK_DGRAM, 0);
	if (bcast_fd < 0) {
		close(listen_fd);
		return -1;
	}
	x = 1;
	if (setsockopt (bcast_fd, SOL_SOCKET, SO_BROADCAST, &x, sizeof (x)) < 0) {
		close(listen_fd);
		close(bcast_fd);
		return -1;
	}

	bzero(&bcast, sizeof(bcast));
	bcast.sin_family = AF_INET;
	bcast.sin_addr.s_addr = htonl(INADDR_ANY);
	bcast.sin_port = htons(0);
	if (bind(bcast_fd, (struct sockaddr *)&bcast, sizeof(bcast)) < 0) {
		close(listen_fd);
		close(bcast_fd);
		return -1;
	}

	bzero(&send, sizeof(send));
	send.sin_family = AF_INET;
	send.sin_addr.s_addr = INADDR_BROADCAST;
	send.sin_port = htons(TFTPD_BCAST);

	/* We want to dump the entire environment setting as broadcast data */
	envsize(&ec, &elen);	/* calculate size required */
	envbuf = malloc(elen);
	if (envbuf == NULL) {
		close(listen_fd);
		close(bcast_fd);
		return -1;
	}
	envbuild(NULL, envbuf, 0); /* Build the environment strings */
#endif

	/*
	 *  Wait for an incoming msg.
	 *  TODO: While waiting announce our existence.
	 */
	if (v.verbose > 0)
		fprintf(stderr, "tftpd: ready %d\n", listen_fd);

	do {
#ifdef TFTPD_BCAST
		/* Broadcast attention message */
		sendto (bcast_fd, envbuf, elen, 0,
                             (struct sockaddr *)&send, sizeof(send));
#endif

		FD_ZERO(&sel_set);
		sel_maxfd = 0;
		FD_SET(listen_fd, &sel_set);
		if (listen_fd > sel_maxfd)
			sel_maxfd = listen_fd;
		sel_tout.tv_sec = 1;
		sel_tout.tv_usec = 0;
#ifdef TFTPD_LED
		/* Enable status LED if supported */
		gpio_set_led(TFTPD_LED, LED_A_BLINK, 5000, TFTPD_LED_ON, TFTPD_LED_OFF);
#endif

		if (select(sel_maxfd + 1, &sel_set, NULL, NULL, &sel_tout) < 0) {
			return -1;
		}
		if (FD_ISSET(listen_fd, &sel_set)) {
			break;
		}
	} while (1);

#ifdef TFTPD_BCAST
	free(envbuf);	/* Don't need this anymore */
#endif
	j = sizeof(v.s_in);
	if (getsockname(listen_fd, (struct sockaddr *)&v.s_in, &j) == -1) {
		return -1;
	}
	
	switch (v.s_in.ss_family) {
	case AF_INET:
		if (setsockopt(listen_fd, IPPROTO_IP, IP_RECVDSTADDR, &on,
		    sizeof(on)) == -1) {
			return -1;
		}
		break;
#ifdef NOTYET
	case AF_INET6:
		if (setsockopt(listen_fd, IPPROTO_IPV6, IPV6_RECVDSTADDR, &on,
		    sizeof(on)) == -1) {
			return -1;
		}
		break;
#endif
	}

	bzero(&msg, sizeof(msg));
	iov.iov_base = cmdbuf;
	iov.iov_len = sizeof (cmdbuf);
	msg.msg_name = (void *)&v.sock_from;
	msg.msg_namelen = sizeof(v.sock_from);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cbuf;
	msg.msg_controllen = CMSG_LEN(sizeof(struct sockaddr_storage));
	
	n = recvmsg(listen_fd, &msg, 0);
	if (n < 0) {
		return -1;
	}

	close(listen_fd);
	v.peer = socket(v.sock_from.ss_family, SOCK_DGRAM, 0);
	if (v.peer < 0) {
		return -1;
	}
	memset(&v.s_in, 0, sizeof(v.s_in));
	v.s_in.ss_family = v.sock_from.ss_family;
	v.s_in.ss_len = v.sock_from.ss_len;

	/* get local address if possible */
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
	    cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP &&
		    cmsg->cmsg_type == IP_RECVDSTADDR) {
			memcpy(&((struct sockaddr_in *)&v.s_in)->sin_addr,
			    CMSG_DATA(cmsg), sizeof(struct in_addr));
			break;
		}
#ifdef NOTYET
		if (cmsg->cmsg_level == IPPROTO_IPV6 &&
		    cmsg->cmsg_type == IPV6_RECVDSTADDR) {
			memcpy(&((struct sockaddr_in6 *)&v.s_in)->sin6_addr,
			    CMSG_DATA(cmsg), sizeof(struct in6_addr));
			break;
		}
#endif
	}
			
	if (bind(v.peer, (struct sockaddr *)&v.s_in, v.s_in.ss_len) < 0) {
		close(v.peer);
		return -1;
	}
	if (connect(v.peer, (struct sockaddr *)&v.sock_from, v.sock_from.ss_len) < 0) {
		close(v.peer);
		return -1;
	}
	tp = (struct tftphdr *)cmdbuf;
	tp->th_opcode = ntohs(tp->th_opcode);
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ) {
		result = tftpd_exec(tp, n);
	} else {
		result = EBADOP;
	}
	close(v.peer);
	return result;
}

/*
 * Handle initial connection protocol.
 */
int
tftpd_exec(struct tftphdr *tp, int size)
{
	char *cp;
	int i, first = 1, has_options = 0, ecode;
	struct formats *pf;
	char *filename, *mode = NULL, *option, *ccp;
	char fnbuf[MAXPATHLEN];

	cp = tp->th_stuff;
again:
	while (cp < cmdbuf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		return 1;
	}
	i = cp - tp->th_stuff;
	if (i >= sizeof(fnbuf)) {
		nak(EBADOP);
		return 1;
	}
	memcpy(fnbuf, tp->th_stuff, i);
	fnbuf[i] = '\0';
	filename = fnbuf;
	if (v.verbose > 2)
		fprintf(stderr, "tftpd: parsing filename %s\n", filename);
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		return 1;
	}
	while (++cp < cmdbuf + size) {
		for (i = 2, ccp = cp; i > 0; ccp++) {
			if (ccp >= cmdbuf + size) {
				/*
				 * Don't reject the request, just stop trying
				 * to parse the option and get on with it.
				 * Some Apple OpenFirmware versions have
				 * trailing garbage on the end of otherwise
				 * valid requests.
				 */
				goto option_fail;
			} else if (*ccp == '\0')
				i--;
		}
		for (option = cp; *cp; cp++)
			if (isupper(*cp))
				*cp = tolower(*cp);
		for (i = 0; options[i].o_type != NULL; i++)
			if (strcmp(option, options[i].o_type) == 0) {
				options[i].o_request = ++cp;
				has_options = 1;
			}
		cp = ccp-1;
	}

option_fail:
	if (options[OPT_TIMEOUT].o_request) {
		int to = atoi(options[OPT_TIMEOUT].o_request);
		if (to < 1 || to > 255) {
			nak(EBADOP);
			return EBADOP;
		}
		else if (to <= v.max_rexmtval)
			options[OPT_TIMEOUT].o_reply = v.rexmtval = to;
		else
			options[OPT_TIMEOUT].o_request = NULL;
	}

	if (options[OPT_BLKSIZE].o_request) {
		int blksize = atoi(options[OPT_BLKSIZE].o_request);
		if (blksize < 512) {
			nak(EBADOP);
			return EBADOP;
		}
		else if (blksize > MAX_BLKSIZE)
			options[OPT_BLKSIZE].o_reply = MAX_BLKSIZE;
			v.pktsize = options[OPT_BLKSIZE].o_reply + 4;
	}

	ecode = (*pf->f_validate)(filename, tp->th_opcode);
	if (has_options)
		oack();
	if (ecode) {
		nak(ecode);
		return 1;
	}

	if (tp->th_opcode == WRQ)
		ecode = (*pf->f_recv)(pf);
	else
		ecode = (*pf->f_send)(pf);
	return ecode;
}


/*
 * Check path and name of the destination/source.
 * May change the value of filename.
 */
int
validate_access(char *filename, int mode)
{
	struct stat stbuf;
	char *cp, **dirp;
	int fd, wmode;

	/* Bail out... */
	if (strcmp(filename, "exit") == 0)
		exit(1);

#if defined(USERFLASH_BASE)
	/*
	 *  If destination is the flash memory, set flag and upload to usermem.
	 */
	if (mode == WRQ && strncmp(filename, "/dev/flash", 10) == 0) {
		v.offset = 0;
		if (filename[10] == '@' || filename[10] == '+') {
			if (!get_rsa(&v.offset, filename + 11))
				return EBADF;
			if (filename[10] == '@')
				v.offset -= (USERFLASH_BASE - FLASH_BASE);
		}
		v.flash_it = 1;
		strcpy(filename, "/dev/ram/usermem");
	}
#endif

	/*
	 *  Uploading to memory for execution require correct CRC32
	 */
	if (mode == WRQ && strcmp(filename, "/dev/ram/usermem") == 0)
		v.do_crc = 1;

	if (!secure) {
		if (*filename != '/')
			return (EACCESS);
		/*
		 * prevent tricksters from getting around the directory
		 * restrictions
		 */
		for (cp = filename + 1; *cp; cp++)
			if (*cp == '.' && strncmp(cp-1, "/../", 4) == 0)
				return(EACCESS);
	}

	/*
	 * We use a different permissions scheme if `cancreate' is
	 * set.
	 */
	wmode = O_TRUNC;
#if 0
	if (stat(filename, &stbuf) < 0) {
		if (!cancreate)
			return (errno == ENOENT ? ENOTFOUND : EACCESS);
		else {
			if ((errno == ENOENT) && (mode != RRQ))
				wmode |= O_CREAT;
			else
				return(EACCESS);
		}
	} else {
		if (mode == RRQ) {
			if ((stbuf.st_mode&(S_IREAD >> 6)) == 0)
				return (EACCESS);
		} else {
			if ((stbuf.st_mode&(S_IWRITE >> 6)) == 0)
				return (EACCESS);
		}
	}
#endif
	if (options[OPT_TSIZE].o_request) {
		if (mode == RRQ)
			options[OPT_TSIZE].o_reply = stbuf.st_size;
		else
			/* XXX Allows writes of all sizes. */
			options[OPT_TSIZE].o_reply =
				atoi(options[OPT_TSIZE].o_request);
	}
	fd = open(filename, mode == RRQ ? O_RDONLY : (O_WRONLY|wmode), 0666);
	if (fd < 0)
		return (errno + 100);
	close(fd);
	v.file = fopen(filename, (mode == RRQ)? "r":"w");
	if (v.file == NULL) {
		close(fd);
		return (errno + 100);
	}
	return (0);
}


/*
 * Send the requested file.
 */
int
sendfile(struct formats *pf)
{
	struct tftphdr *dp, *r_init(void);
	struct tftphdr *ap;    /* ack packet */
	volatile unsigned short block = 1;
	int size, n, ecode;

	dp = r_init();
	ap = (struct tftphdr *)ackbuf;
	v.trfsize = 0;
	ecode = 0;

	do {
		size = readit(v.file, &dp, pf->f_convert);
		v.trfsize += size;
		if (size < 0) {
			ecode = errno + 100;
			nak(ecode);
			return ecode;
		}
		dp->th_opcode = htons((u_short)DATA);
		dp->th_block = htons((u_short)block);
		v.timeouts = 0;

send_data:
		if (send(v.peer, dp, size + 4, 0) != size + 4) {
			return errno + 100;
		}
		read_ahead(v.file, pf->f_convert);
		for ( ; ; ) {
			if (tftpd_select(v.peer) != 1)
				goto send_data;
			n = recv(v.peer, ackbuf, sizeof (ackbuf), 0);
			if (n < 0) {
				return errno + 100;
			}
			ap->th_opcode = ntohs((u_short)ap->th_opcode);
			ap->th_block = ntohs((u_short)ap->th_block);

			if (ap->th_opcode == ERROR) {
				return errno + 100;
			}

			if (ap->th_opcode == ACK) {
				if (ap->th_block == block) {
					break;
				}
				/* Re-synchronize with the other side */
				(void) synchnet(v.peer);
				if (ap->th_block == (block -1)) {
					goto send_data;
				}
			}

		}
		block++;
	} while (size == BLKSIZE);

	fclose(v.file);
	return ecode;
}

/*
 * Receive a file.
 */
int
recvfile(struct formats *pf)
{
	struct tftphdr *dp, *w_init(void);
	struct tftphdr *ap;    /* ack buffer */
	volatile unsigned short block = 0;
	int n, size, ecode;

	dp = w_init();
	ap = (struct tftphdr *)ackbuf;
	v.trfsize = 0;
	ecode = 0;

	do {
		v.timeouts = 0;
		ap->th_opcode = htons((u_short)ACK);
		ap->th_block = htons((u_short)block);
		block++;
send_ack:
		if (send(v.peer, ackbuf, 4, 0) != 4) {
			return -1;
		}
		write_behind(v.file, pf->f_convert);
		for ( ; ; ) {
			if (tftpd_select(v.peer) != 1)
				goto send_ack;
			n = recv(v.peer, dp, v.pktsize, 0);
			if (n < 0) {		/* really? */
				return 1;
			}
			dp->th_opcode = ntohs((u_short)dp->th_opcode);
			dp->th_block = ntohs((u_short)dp->th_block);
			if (dp->th_opcode == ERROR)
				return 1;
			if (dp->th_opcode == DATA) {
				if (dp->th_block == block) {
					break;   /* normal */
				}
				/* Re-synchronize with the other side */
				(void) synchnet(v.peer);
				if (dp->th_block == (block-1))
					goto send_ack;		/* rexmit */
			}
		}
		/*  size = write(v.file, dp->th_data, n - 4); */
		size = writeit(v.file, &dp, n - 4, pf->f_convert);
		v.trfsize += size;
		if (size != (n-4)) {			/* ahem */
			if (size < 0)
				ecode = errno + 100;
			else 
				ecode = ENOSPACE;
			nak(ecode);
			(void) fclose(v.file);		/* close data file */
			return ecode;
		}
	} while (size == BLKSIZE);
	write_behind(v.file, pf->f_convert);
	(void) fclose(v.file);		/* close data file */

	/*
	 * Check if this is a special upload. If so check
	 * crc and signature and ack/nak appropriately.
	 */
	if (v.do_crc) {
		u_int crc;
		u_char *sp;
		quad_t signature, check;
		int i;

		crc = crc32_check((void *)CLIENTPC, v.trfsize);
		if (crc == 0) {
			if (getenv("signature")) {
				sp = (u_char *)CLIENTPC;
				sp += *((int *)(CLIENTPC + 8)) - 8;
				signature = sp[7];
				for (i = 6; i >= 0; i--)
					signature = (signature << 8) | sp[i];
				(void)llatob(&check, getenv("signature"), 16);
				signature ^= check;
			} else
				signature = 0;

		}
		if (crc || signature) {
			if (v.verbose > 0)
				fprintf(stderr, "tftpd: upload error - %s\n",
				     crc ? "CRC32" : "signature");
			nak(ENOEXEC + 100);
			return ENOEXEC + 100;
		}
	}

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	(void) send(v.peer, ackbuf, 4, 0);

	if (tftpd_select(v.peer) != 1)
		return 0;

	n = recv(v.peer, cmdbuf, sizeof (cmdbuf), 0);
	if (n >= 4 &&			/* if read some data */
	    dp->th_opcode == DATA &&    /* and got a data block */
	    block == dp->th_block) {	/* then my last ack was lost */
		(void) send(v.peer, ackbuf, 4, 0);	/* resend final ack */
	}
	return 0;
}

int
tftpd_select(int fd)
{
	struct timeval sel_tout;
	fd_set sel_set;

	FD_ZERO(&sel_set);
	FD_SET(fd, &sel_set);
	sel_tout.tv_sec = v.rexmtval;
	sel_tout.tv_usec = 0;

	if (select(fd + 1, &sel_set, NULL, NULL, &sel_tout) < 0) {
		return -1;
	} else if (FD_ISSET(fd, &sel_set)) {
		return 1;
	} else {
		return 0;
	}
}

struct errmsg {
	int	e_code;
	const char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		NULL }
};

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
void
nak(int error)
{
	struct tftphdr *tp;
	struct errmsg *pe;
	int length;

	tp = (struct tftphdr *)cmdbuf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	length = strlcpy(tp->th_msg, pe->e_msg, sizeof (cmdbuf)) + 5;
	if (length > sizeof (cmdbuf))
		length = sizeof (cmdbuf);
	if (send(v.peer, cmdbuf, length, 0) != length)
		printf("tftpd: nak");
}

/*
 * Send an oack packet (option acknowledgement).
 */
void
oack(void)
{
	struct tftphdr *tp, *ap;
	int size, i, n;
	char *bp;

	tp = (struct tftphdr *)cmdbuf;
	bp = cmdbuf + 2;
	size = sizeof (cmdbuf) - 2;
	tp->th_opcode = htons((u_short)OACK);
	for (i = 0; options[i].o_type != NULL; i++) {
		if (options[i].o_request) {
			n = snprintf(bp, size, "%s%c%d", options[i].o_type,
			    0, options[i].o_reply);
			if (n == -1 || n >= size) {
				printf("tftpd: oack, no buffer space");
				exit(-1);
			}
			bp += n+1;
			size -= n+1;
			if (size < 0) {
				printf("tftpd: oack, no buffer space");
				exit(-1);
			}
		}
	}
	size = bp - cmdbuf;
	ap = (struct tftphdr *)ackbuf;
	v.timeouts = 0;

send_oack:
	if (send(v.peer, cmdbuf, size, 0) != size) {
		printf("tftpd: oack");
		exit(-1);
	}

	for (;;) {
		if (tftpd_select(v.peer) != 1)
			goto send_oack;
		n = recv(v.peer, ackbuf, sizeof (ackbuf), 0);
		if (n < 0) {
			printf("tftpd: recv");
			exit(-1);
		}
		ap->th_opcode = ntohs((u_short)ap->th_opcode);
		ap->th_block = ntohs((u_short)ap->th_block);
		if (ap->th_opcode == ERROR)
			exit(-1);
		if (ap->th_opcode == ACK && ap->th_block == 0)
			break;
	}
}

/* Simple minded read-ahead/write-behind subroutines for tftp user and
   server.  Written originally with multiple buffers in mind, but current
   implementation has two buffer logic wired in.

   Todo:  add some sort of final error check so when the write-buffer
   is finally flushed, the caller can detect if the disk filled up
   (or had an i/o error) and return a nak to the other side.

			Jim Guyton 10/85
 */

struct bf {
	int counter;		/* size of data in buffer, or flag */
	char *buf;		/* room for data packet */
} bfs[2];

				/* Values for bf.counter  */
#define BF_ALLOC -3             /* alloc'd but not yet filled */
#define BF_FREE  -2             /* free */
/* [-1 .. BLKSIZE] = size of data in the data buffer */

static int nextone;		/* index of next buffer to use */
static int current;		/* index of buffer in use */

				/* control flags for crlf conversions */
int newline = 0;		/* fillbuf: in middle of newline expansion */
int prevchar = -1;		/* putbuf: previous char (cr check) */

static struct tftphdr *rw_init(int);

struct tftphdr *
w_init(void)
{
	return rw_init(0);	/* write-behind */
}

struct tftphdr *
r_init(void)
{
	return rw_init(1);	/* read-ahead */
}

/* init for either read-ahead or write-behind */
/* zero for write-behind, one for read-head */
static struct tftphdr *
rw_init(int x)
{
	int i;

	newline = 0;		/* init crlf flag */
	prevchar = -1;
	bfs[0].counter =  BF_ALLOC;     /* pass out the first buffer */
	current = 0;
	bfs[1].counter = BF_FREE;
	nextone = x;                    /* ahead or behind? */
	for (i = 0; i < 2; i++) {
		if (bfs[i].buf != NULL)
			free(bfs[i].buf);
		bfs[i].buf = malloc(v.pktsize);
		if (bfs[i].buf == NULL) {
			fprintf(stderr, "tftpd: out of memory!\n");
			exit(-1);
		}
	}
	return (struct tftphdr *)bfs[0].buf;
}


/* Have emptied current buffer by sending to net and getting ack.
   Free it and return next buffer filled with data.
 */
int
readit(FILE *file, struct tftphdr **dpp, int convert)
{
	struct bf *b;

	bfs[current].counter = BF_FREE; /* free old one */
	current = !current;             /* "incr" current */

	b = &bfs[current];              /* look at new buffer */
	if (b->counter == BF_FREE)      /* if it's empty */
		read_ahead(file, convert);      /* fill it */
/*      assert(b->counter != BF_FREE);*//* check */
	*dpp = (struct tftphdr *)b->buf;        /* set caller's ptr */
	return b->counter;
}

/*
 * fill the input buffer, doing ascii conversions if requested
 * conversions are  lf -> cr,lf  and cr -> cr, nul
 */
void
read_ahead(FILE *file, int convert)
{
	int i;
	char *p;
	int c;
	struct bf *b;
	struct tftphdr *dp;

	b = &bfs[nextone];              /* look at "next" buffer */
	if (b->counter != BF_FREE)      /* nop if not free */
		return;
	nextone = !nextone;             /* "incr" next buffer ptr */

	dp = (struct tftphdr *)b->buf;

	if (convert == 0) {
		b->counter = read(fileno(file), dp->th_data, BLKSIZE);
		return;
	}

	p = dp->th_data;
	for (i = 0 ; i < BLKSIZE; i++) {
		if (newline) {
			if (prevchar == '\n')
				c = '\n';       /* lf to cr,lf */
			else
				c = '\0';       /* cr to cr,nul */
			newline = 0;
		} else {
			c = getc(file);
			if (c == EOF) break;
			if (c == '\n' || c == '\r') {
				prevchar = c;
				c = '\r';
				newline = 1;
			}
		}
	       *p++ = c;
	}
	b->counter = (int)(p - dp->th_data);
}

/* Update count associated with the buffer, get new buffer
   from the queue.  Calls write_behind only if next buffer not
   available.
 */
int
writeit(FILE *file, struct tftphdr **dpp, int ct, int convert)
{
	bfs[current].counter = ct;      /* set size of data to write */
	current = !current;             /* switch to other buffer */
	if (bfs[current].counter != BF_FREE)     /* if not free */
		(void)write_behind(file, convert); /* flush it */
	bfs[current].counter = BF_ALLOC;        /* mark as alloc'd */
	*dpp =  (struct tftphdr *)bfs[current].buf;
	return ct;                      /* this is a lie of course */
}

/*
 * Output a buffer to a file, converting from netascii if requested.
 * CR,NUL -> CR  and CR,LF => LF.
 * Note spec is undefined if we get CR as last byte of file or a
 * CR followed by anything else.  In this case we leave it alone.
n */
int
write_behind(FILE *file, int convert)
{
	char *buf;
	int count;
	int ct;
	char *p;
	int c;                          /* current character */
	struct bf *b;
	struct tftphdr *dp;

	b = &bfs[nextone];
	if (b->counter < -1)            /* anything to flush? */
		return 0;               /* just nop if nothing to do */

	count = b->counter;             /* remember byte count */
	b->counter = BF_FREE;           /* reset flag */
	dp = (struct tftphdr *)b->buf;
	nextone = !nextone;             /* incr for next time */
	buf = dp->th_data;

	if (count <= 0) return -1;      /* nak logic? */

	if (convert == 0)
		return write(fileno(file), buf, count);

	p = buf;
	ct = count;
	while (ct--) {                  /* loop over the buffer */
		c = *p++;                   /* pick up a character */
		if (prevchar == '\r') {     /* if prev char was cr */
			if (c == '\n')          /* if have cr,lf then just */
				fseek(file, -1, 1);  /* smash lf on top of the cr */
			else if (c == '\0')       /* if have cr,nul then */
				goto skipit;    /* just skip over the putc */
			/* else just fall through and allow it */
		}
		putc(c, file);
skipit:
		prevchar = c;
	}
	return count;
}


/* When an error has occurred, it is possible that the two sides
 * are out of synch.  Ie: that what I think is the other side's
 * response to packet N is really their response to packet N-1.
 *
 * So, to try to prevent that, we flush all the input queued up
 * for us on the network connection on our host.
 *
 * We return the number of packets we flushed (mostly for reporting
 * when trace is active).
 */

int
synchnet(int f)
{
	int i, j = 0;
	struct sockaddr_in from;
	socklen_t fromlen;
	char *buf;

	buf = malloc(v.pktsize);
	if (buf == NULL) {
		fprintf(stderr, "tftpd: out of memory\n");
		exit(-1);
	}
	while (1) {
		(void) ioctl(f, FIONREAD, &i);
		if (i) {
			j++;
			fromlen = sizeof from;
			(void) recvfrom(f, buf, v.pktsize, 0,
			    (struct sockaddr *)&from, &fromlen);
		} else {
			free(buf);
			return(j);
		}
	}
}


/*
 *  Command table registration
 *  ==========================
 */

static const Cmd Cmds[] =
{
	{"Network"},
	{"tftpd",	"[-bcs]", cmd_tftpd_opts,
			"tftp server",
			cmd_tftpd, 1, 99, CMD_SPAWN},
        {NULL}
};

static void init_cmd(void) __attribute__ ((constructor));

static void
init_cmd()
{
        cmdlist_expand(Cmds, 1);
}
