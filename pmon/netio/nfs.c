#undef  _KERNEL

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/endian.h>

#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <file.h>
#include <net/if.h>
#include <pmon.h>
#include "nfs.h"
#include "netio.h"
#include <signal.h>

#define HASHES_PER_LINE 65	/* Number of "loading" hashes per line	*/
#define NFS_RETRY_COUNT 30
#define NFS_TIMEOUT 2000UL

struct nfsfile {
struct sockaddr_in sin;
int     sock;
int     foffs;
int start, end, off;
struct rpc_t readbuf;
int	NfsDownloadState;
int	NfsSrvMountPort;
int	NfsSrvNfsPort;
int	NfsOurPort;
int	NfsTimeoutCount;
int	NfsState;
char *nfs_path;
char *nfs_filename;
char nfs_url[256];
int fs_mounted;
unsigned long rpc_id;
unsigned long nfs_offset;
int nfs_len;
char dirfh[NFS_FHSIZE];	/* file handle of directory */
char filefh[NFS_FHSIZE]; /* file handle of kernel image */
};

static char* basename (char *);
static char* dirname (char *);
extern void log __P((int kind, const char *fmt, ...));
extern in_addr_t inet_addr __P((const char *));
static void NfsSend (struct nfsfile *nfs);
static void NfsHandler (struct nfsfile *nfs, struct rpc_t *pkt, void * buf,  unsigned len);

#define STATE_RPCLOOKUP_PROG_MOUNT_REQ	1
#define STATE_RPCLOOKUP_PROG_NFS_REQ	2
#define STATE_MOUNT_REQ			3
#define STATE_UMOUNT_REQ		4
#define STATE_LOOKUP_REQ		5
#define STATE_READ_REQ			6
#define STATE_READLINK_REQ		7
#define STATE_DONE 8

static int nfs_open (int , struct Url *, int, int);
static int nfs_read (int, void *, int);
static int nfs_write (int, const void *, int);
static off_t nfs_lseek (int , long, int);
static int nfs_close(int);

static sig_t	pre_handler;
static int nfs_sock;

static void terminate()
{
extern struct proc *curproc;
		signal(SIGINT, pre_handler);
		close(nfs_sock);
		if(pre_handler) pre_handler(SIGINT);
		else sigexit(curproc,SIGINT);
}

static int nfs_open(int fd, struct Url *url, int flags, int perms) {
    struct hostent *hp;
    const char *mode;
    char *host;
    NetFile *nfp = (NetFile *)_file[fd].data;
    struct nfsfile *nfs;
    int sock;
	int client_port;
    short   oflags;
    struct sockaddr_in sin;

    oflags = flags & O_ACCMODE;
    if(oflags != O_RDONLY && oflags != O_WRONLY) {
        errno = EACCES;
        return -1;
    }

    if(strlen(url->hostname) != 0) {
        host = url->hostname;
    } else {
            printf( "nfs: missing/bad host name: %s\n", url->filename);
            errno = EDESTADDRREQ;
            return -1;
    }
    
    nfs = (struct nfsfile *)malloc(sizeof(struct nfsfile));
    if(!nfs) {
        errno = ENOBUFS;
        return -1;
    }
    
	bzero(nfs, sizeof(struct nfsfile));

	nfp->data = (void *)nfs;

    nfs->nfs_url[0]='/';
    strcpy(nfs->nfs_url+1, url->filename);
    nfs->nfs_filename = basename(nfs->nfs_url);
    nfs->nfs_path = dirname(nfs->nfs_url);

    puts("File transfer via NFS from server ");
    printf("nfs_filename : %s\n", nfs->nfs_filename);
    printf("nfs_path : %s\n", nfs->nfs_path);
    nfs_sock = nfs->sock = sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) goto error;

    for(client_port=800;client_port<=1000;client_port++)
    {
    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(client_port);
    if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) == 0)
	    break;
    printf("bind %d error\n",client_port);
    }
    hp = gethostbyname(host);
    if(hp) {
        sin.sin_family = hp->h_addrtype;
        bcopy(hp->h_addr, (void *)&sin.sin_addr, hp->h_length);
    } 
    else goto error;

    pre_handler = signal(SIGINT, (sig_t)terminate);

    nfs->sin = sin;
    nfs->NfsState = STATE_RPCLOOKUP_PROG_NFS_REQ;
    NfsSend(nfs);
    while(nfs->NfsState != STATE_READ_REQ && nfs->NfsState != STATE_DONE) {
        NfsHandler(nfs, &nfs->readbuf, NULL, sizeof(struct rpc_t));
    }

    if(nfs->NfsState == STATE_READ_REQ)
        return 0;

error:
    if(sock >= 0)
        close(sock);
    free(nfs);
    return -1;
}

static int nfs_close(int fd) {
	NetFile *nfp = (NetFile *)_file[fd].data;
	struct nfsfile *nfs = nfp->data;
	close(nfs->sock);
	signal(SIGINT, pre_handler);
    free(nfs);
	return 0;
}


static int nfs_read_reply (struct nfsfile *nfs, struct rpc_t *, unsigned int);

static int nfs_read(int fd, void *buf, int nread) {
	NetFile *nfp = (NetFile *)_file[fd].data;
	struct nfsfile *nfs = nfp->data;
	int nb, n;

    for(nb = nread; nb != 0; ) {
	if(nfs->off >= nfs->start && nfs->off < nfs->end) {
		n = nfs->end - nfs->off;
		if(n > nb) n = nb;
		bcopy((uint8_t *)&nfs->readbuf+sizeof(nfs->readbuf.u.reply) + (nfs->off - nfs->start), buf, n);
		nfs->off += n;
		nb -= n;
		buf += n;
	}
	if(nfs->off >= nfs->end) {
		NfsSend(nfs);
		nfs->start = nfs->off;
		n = nfs_read_reply (nfs, &nfs->readbuf, n);
		if(n <= 0) break;
		nfs->end = nfs->off + n;
		dotik (25600, 0);
	}
    }
    return nread - nb;
}

static int nfs_write(int fd, const void *buf, int n) {
	return -1;
}
    
static off_t nfs_lseek(int fd, long offset, int whence) {
	NetFile *nfp = (NetFile *)_file[fd].data;
	struct nfsfile *nfs = nfp->data;
    return (nfs->off = offset);
}

static NetFileOps nfsops = {
    "nfs", 
    nfs_open,
    nfs_read,
    nfs_write,
    nfs_lseek,
    NULL,
    nfs_close,
};

static void init_nfs __P((void)) __attribute__ ((constructor));

static void init_nfs(void) {
    netfs_init(&nfsops);
}


static char*
basename (char *path)
{
	char *fname;

	fname = path + strlen(path) - 1;
	while (fname >= path) {
		if (*fname == '/') {
			fname++;
			break;
		}
		fname--;
	}
	return fname;
}

static char*
dirname (char *path)
{
	char *fname;

	fname = basename (path);
	--fname;
	*fname = '\0';
	return path;
}

/**************************************************************************
RPC_ADD_CREDENTIALS - Add RPC authentication/verifier entries
**************************************************************************/
static long *rpc_add_credentials (long *p)
{
	int hl;
	int hostnamelen;
	char hostname[256];

	strcpy (hostname, "");
	hostnamelen=strlen (hostname);

	/* Here's the executive summary on authentication requirements of the
	 * various NFS server implementations:	Linux accepts both AUTH_NONE
	 * and AUTH_UNIX authentication (also accepts an empty hostname field
	 * in the AUTH_UNIX scheme).  *BSD refuses AUTH_NONE, but accepts
	 * AUTH_UNIX (also accepts an empty hostname field in the AUTH_UNIX
	 * scheme).  To be safe, use AUTH_UNIX and pass the hostname if we have
	 * it (if the BOOTP/DHCP reply didn't give one, just use an empty
	 * hostname).  */

	hl = (hostnamelen + 3) & ~3;

	/* Provide an AUTH_UNIX credential.  */
	*p++ = htonl(1);		/* AUTH_UNIX */
	*p++ = htonl(hl+20);		/* auth length */
	*p++ = htonl(0);		/* stamp */
	*p++ = htonl(hostnamelen);	/* hostname string */
	if (hostnamelen & 3) {
		*(p + hostnamelen / 4) = 0; /* add zero padding */
	}
	memcpy (p, hostname, hostnamelen);
	p += hl / 4;
	*p++ = 0;			/* uid */
	*p++ = 0;			/* gid */
	*p++ = 0;			/* auxiliary gid list */

	/* Provide an AUTH_NONE verifier.  */
	*p++ = 0;			/* AUTH_NONE */
	*p++ = 0;			/* auth length */

	return p;
}

/**************************************************************************
RPC_LOOKUP - Lookup RPC Port numbers
**************************************************************************/
static void
rpc_req (struct nfsfile *nfs, int rpc_prog, int rpc_proc, uint32_t *data, int datalen)
{
	struct rpc_t *pkt = &nfs->readbuf;
	unsigned long id;
	uint32_t *p;
	int pktlen;
	int sport;
	int n = 0;


	id = ++nfs->rpc_id;
	pkt->u.call.id = htonl(id);
	pkt->u.call.type = htonl(MSG_CALL);
	pkt->u.call.rpcvers = htonl(2);	/* use RPC version 2 */
	pkt->u.call.prog = htonl(rpc_prog);
	pkt->u.call.vers = htonl(2);	/* portmapper is version 2 */
	pkt->u.call.proc = htonl(rpc_proc);
	p = (uint32_t *)&(pkt->u.call.data);

	if (datalen)
		memcpy ((char *)p, (char *)data, datalen*sizeof(uint32_t));

	pktlen = (char *)p + datalen*sizeof(uint32_t) - (char *)pkt;


	if (rpc_prog == PROG_PORTMAP)
		sport = SUNRPC_PORT;
	else if (rpc_prog == PROG_MOUNT)
		sport = nfs->NfsSrvMountPort;
	else
		sport = nfs->NfsSrvNfsPort;

    nfs->sin.sin_port = htons(sport);
    n = sendto(nfs->sock, pkt, pktlen, 0, (struct sockaddr *)&nfs->sin, sizeof(nfs->sin));
}

/**************************************************************************
RPC_LOOKUP - Lookup RPC Port numbers
**************************************************************************/
static void
rpc_lookup_req (struct nfsfile *nfs, int prog, int ver)
{
	uint32_t data[16];

	data[0] = 0; data[1] = 0;	/* auth credential */
	data[2] = 0; data[3] = 0;	/* auth verifier */
	data[4] = htonl(prog);
	data[5] = htonl(ver);
	data[6] = htonl(17);	/* IP_UDP */
	data[7] = 0;

	rpc_req (nfs, PROG_PORTMAP, PORTMAP_GETPORT, data, 8);
}

/**************************************************************************
NFS_MOUNT - Mount an NFS Filesystem
**************************************************************************/
static void
nfs_mount_req (struct nfsfile *nfs, char *path)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int pathlen;

	pathlen = strlen (path);

	p = &(data[0]);
	p = (uint32_t *)rpc_add_credentials((long *)p);

	*p++ = htonl(pathlen);
	if (pathlen & 3) *(p + pathlen / 4) = 0;
	memcpy (p, path, pathlen);
	p += (pathlen + 3) / 4;

	len = (uint32_t *)p - (uint32_t *)&(data[0]);

	rpc_req (nfs, PROG_MOUNT, MOUNT_ADDENTRY, data, len);
}

/**************************************************************************
NFS_UMOUNTALL - Unmount all our NFS Filesystems on the Server
**************************************************************************/
static void
nfs_umountall_req (struct nfsfile *nfs)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;

	if ((nfs->NfsSrvMountPort == -1) || (!nfs->fs_mounted)) {
		/* Nothing mounted, nothing to umount */
		return;
	}

	p = &(data[0]);
	p = (uint32_t *)rpc_add_credentials ((long *)p);

	len = (uint32_t *)p - (uint32_t *)&(data[0]);

	rpc_req (nfs, PROG_MOUNT, MOUNT_UMOUNTALL, data, len);
}

/***************************************************************************
 * NFS_READLINK (AH 2003-07-14)
 * This procedure is called when read of the first block fails -
 * this probably happens when it's a directory or a symlink
 * In case of successful readlink(), the dirname is manipulated,
 * so that inside the nfs() function a recursion can be done.
 **************************************************************************/
static void
nfs_readlink_req (struct nfsfile *nfs)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;

	p = &(data[0]);
	p = (uint32_t *)rpc_add_credentials ((long *)p);

	memcpy (p, nfs->filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);

	len = (uint32_t *)p - (uint32_t *)&(data[0]);

	rpc_req (nfs, PROG_NFS, NFS_READLINK, data, len);
}

/**************************************************************************
NFS_LOOKUP - Lookup Pathname
**************************************************************************/
static void
nfs_lookup_req (struct nfsfile *nfs, char *fname)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int fnamelen;

	fnamelen = strlen (fname);

	p = &(data[0]);
	p = (uint32_t *)rpc_add_credentials ((long *)p);

	memcpy (p, nfs->dirfh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = htonl(fnamelen);
	if (fnamelen & 3) *(p + fnamelen / 4) = 0;
	memcpy (p, fname, fnamelen);
	p += (fnamelen + 3) / 4;

	len = (uint32_t *)p - (uint32_t *)&(data[0]);

	rpc_req (nfs, PROG_NFS, NFS_LOOKUP, data, len);
}

/**************************************************************************
NFS_READ - Read File on NFS Server
**************************************************************************/
static void
nfs_read_req (struct nfsfile *nfs, int offset, int readlen)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;

	p = &(data[0]);
	p = (uint32_t *)rpc_add_credentials ((long *)p);

	memcpy (p, nfs->filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = htonl(offset);
	*p++ = htonl(readlen);
	*p++ = 0;

	len = (uint32_t *)p - (uint32_t *)&(data[0]);

	rpc_req (nfs, PROG_NFS, NFS_READ, data, len);
}

/**************************************************************************
RPC request dispatcher
**************************************************************************/

static void
NfsSend (struct nfsfile *nfs)
{
#ifdef NFS_DEBUG
	printf ("%s\n", __FUNCTION__);
#endif

	switch (nfs->NfsState) {
	case STATE_RPCLOOKUP_PROG_MOUNT_REQ:
		rpc_lookup_req (nfs, PROG_MOUNT, 1);
		break;
	case STATE_RPCLOOKUP_PROG_NFS_REQ:
		rpc_lookup_req (nfs, PROG_NFS, 2);
		break;
	case STATE_MOUNT_REQ:
		nfs_mount_req (nfs, nfs->nfs_path);
		break;
	case STATE_UMOUNT_REQ:
		nfs_umountall_req (nfs);
		break;
	case STATE_LOOKUP_REQ:
		nfs_lookup_req (nfs, nfs->nfs_filename);
		break;
	case STATE_READ_REQ:
		nfs_read_req (nfs, nfs->off, nfs->nfs_len);
		break;
	case STATE_READLINK_REQ:
		nfs_readlink_req (nfs);
		break;
	}
}

/**************************************************************************
Handlers for the reply from server
**************************************************************************/

static int
rpc_lookup_reply (struct nfsfile *nfs, int prog, struct rpc_t *pkt, unsigned len)
{

#ifdef NFS_DEBUG
	printf ("%s\n", __FUNCTION__);
#endif

	if (ntohl(pkt->u.reply.id) != nfs->rpc_id)
		return -1;

	if (pkt->u.reply.rstatus  ||
	    pkt->u.reply.verifier ||
	    pkt->u.reply.astatus) {
		return -1;
	}

	switch (prog) {
	case PROG_MOUNT:
		nfs->NfsSrvMountPort = ntohl(pkt->u.reply.data[0]);
        printf("PROG_MOUNT -- NfsSrvMountPort = %d\n", nfs->NfsSrvMountPort);
		break;
	case PROG_NFS:
		nfs->NfsSrvNfsPort = ntohl(pkt->u.reply.data[0]);
        printf("PROG_NFS -- NfsSrvNfsPort = %d\n", nfs->NfsSrvNfsPort);
		break;
	}

	return 0;
}

static int
nfs_mount_reply (struct nfsfile *nfs, struct rpc_t *pkt, unsigned len)
{
	struct rpc_t rpc_pkt;

#ifdef NFS_DEBUG
	printf ("%s\n", __FUNCTION__);
#endif
	if (ntohl(pkt->u.reply.id) != nfs->rpc_id)
		return -1;

	if (pkt->u.reply.rstatus  ||
	    pkt->u.reply.verifier ||
	    pkt->u.reply.astatus  ||
	    pkt->u.reply.data[0]) {
		return -1;
	}

	nfs->fs_mounted = 1;
	memcpy (nfs->dirfh, pkt->u.reply.data + 1, NFS_FHSIZE);

	return 0;
}

static int
nfs_umountall_reply (struct nfsfile *nfs, struct rpc_t *pkt, unsigned len)
{
#ifdef NFS_DEBUG
	printf ("%s\n", __FUNCTION__);
#endif

	if (ntohl(pkt->u.reply.id) != nfs->rpc_id)
		return -1;

	if (pkt->u.reply.rstatus  ||
	    pkt->u.reply.verifier ||
	    pkt->u.reply.astatus) {
		return -1;
	}

	nfs->fs_mounted = 0;
	memset (nfs->dirfh, 0, sizeof(nfs->dirfh));

	return 0;
}

static int
nfs_lookup_reply (struct nfsfile *nfs, struct rpc_t *pkt, unsigned len)
{

#ifdef NFS_DEBUG
	printf ("%s\n", __FUNCTION__);
#endif

	if (ntohl(pkt->u.reply.id) != nfs->rpc_id)
		return -1;

	if (pkt->u.reply.rstatus  ||
	    pkt->u.reply.verifier ||
	    pkt->u.reply.astatus  ||
	    pkt->u.reply.data[0]) {
		return -1;
	}

	memcpy (nfs->filefh, pkt->u.reply.data + 1, NFS_FHSIZE);

	return 0;
}

#if 0
static int
nfs_readlink_reply (uint8_t *pkt, unsigned len)
{
	struct rpc_t rpc_pkt;
	int rlen;

#ifdef NFS_DEBUG
	printf ("%s\n", __FUNCTION__);
#endif

	memcpy ((unsigned char *)&rpc_pkt, pkt, len);

	if (ntohl(rpc_pkt.u.reply.id) != nfs->rpc_id)
		return -1;

	if (rpc_pkt.u.reply.rstatus  ||
	    rpc_pkt.u.reply.verifier ||
	    rpc_pkt.u.reply.astatus  ||
	    rpc_pkt.u.reply.data[0]) {
		return -1;
	}

	rlen = ntohl (rpc_pkt.u.reply.data[1]); /* new path length */

	if (*((char *)&(rpc_pkt.u.reply.data[2])) != '/') {
		int pathlen;
		strcat (nfs_path, "/");
		pathlen = strlen(nfs_path);
		memcpy (nfs_path+pathlen, (uint8_t *)&(rpc_pkt.u.reply.data[2]), rlen);
		nfs_path[pathlen+rlen+1] = 0;
	} else {
		memcpy (nfs_path, (uint8_t *)&(rpc_pkt.u.reply.data[2]), rlen);
		nfs_path[rlen] = 0;
	}
	return 0;
}
#endif

static int
nfs_read_reply (struct nfsfile *nfs, struct rpc_t *pkt, unsigned len)
{
	int rlen;
	
	struct sockaddr_in from;
	int fromlen, n;

#ifdef NFS_DEBUG_nop
	printf ("%s\n", __FUNCTION__);
#endif

	n = recvfrom(nfs->sock, pkt, sizeof(struct rpc_t), 0, (struct sockaddr *)&from, &fromlen);

	if (ntohl(pkt->u.reply.id) != nfs->rpc_id)
		return -1;

	if (pkt->u.reply.rstatus  ||
	    pkt->u.reply.verifier ||
	    pkt->u.reply.astatus  ||
	    pkt->u.reply.data[0]) {
		if (pkt->u.reply.rstatus) {
			return -9999;
		}
		if (pkt->u.reply.astatus) {
			return -9999;
		}
		return -ntohl(pkt->u.reply.data[0]);;
	}

	rlen = ntohl(pkt->u.reply.data[18]);

	return rlen;
}

static void NfsHandler (struct nfsfile *nfs, struct rpc_t *pkt, void * buf,  unsigned len)
{
	int rlen;
    struct sockaddr_in from;
    int fromlen;
    int n;
    int sock;
    sock = nfs->sock;

    n = recvfrom(sock, pkt, sizeof(struct rpc_t), 0, (struct sockaddr *)&from, &fromlen);


    if(n < 0) {
        perror("nfs: recvfrom");
        return;
    }

	switch (nfs->NfsState) {
	case STATE_RPCLOOKUP_PROG_MOUNT_REQ:
		rpc_lookup_reply (nfs, PROG_MOUNT, pkt, n);
		nfs->NfsState = STATE_MOUNT_REQ;
		NfsSend (nfs);
		break;

	case STATE_RPCLOOKUP_PROG_NFS_REQ:
		rpc_lookup_reply (nfs, PROG_NFS, pkt, n);
		nfs->NfsState = STATE_RPCLOOKUP_PROG_MOUNT_REQ;
		NfsSend (nfs);
		break;

	case STATE_MOUNT_REQ:
		if (nfs_mount_reply(nfs, pkt, n)) {
			puts ("*** ERROR: Cannot mount\n");
			/* just to be sure... */
			nfs->NfsState = STATE_UMOUNT_REQ;
			NfsSend (nfs);
		} else {
			nfs->NfsState = STATE_LOOKUP_REQ;
			NfsSend (nfs);
		}
		break;

	case STATE_UMOUNT_REQ:
		if (nfs_umountall_reply(nfs, pkt, n)) {
			puts ("*** ERROR: Cannot umount\n");
		} else {
			puts ("done");
		}
		nfs->NfsState = STATE_DONE;
		break;

	case STATE_LOOKUP_REQ:
		if (nfs_lookup_reply(nfs, pkt, n)) {
			printf ("*** ERROR: File lookup fail,umount..");
			nfs->NfsState = STATE_UMOUNT_REQ;
			NfsSend (nfs);
		} else {
			nfs->NfsState = STATE_READ_REQ;
			nfs->nfs_len = NFS_READ_SIZE;
		}
		break;
#if 0
	case STATE_READLINK_REQ:
		if (nfs_readlink_reply(pkt, n)) {
			puts ("*** ERROR: Symlink fail\n");
			NfsState = STATE_UMOUNT_REQ;
			NfsSend ();
		} else {
#ifdef NFS_DEBUG
			printf ("Symlink --> %s\n", nfs_path);
#endif
			nfs_filename = basename (nfs_path);
			nfs_path     = dirname (nfs_path);

			NfsState = STATE_MOUNT_REQ;
			NfsSend ();
		}
		break;

	case STATE_READ_REQ:
		rlen = nfs_read_reply (nfs, pkt, n);
		if (rlen > 0) {
			nfs_offset += rlen;
		}
		else if ((rlen == -NFSERR_ISDIR)||(rlen == -NFSERR_INVAL)) {
			/* symbolic link */
			NfsState = STATE_READLINK_REQ;
			NfsSend ();
		} else {
			if ( ! rlen ) //NfsDownloadState = NETLOOP_SUCCESS;
			NfsState = STATE_UMOUNT_REQ;
		}
		break;
#endif
	}
}


/*
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base) {
    unsigned long result = 0, value;

    if(*cp == '0') {
        cp++;
        if((*cp == 'x') && isxdigit(cp[1])) {
            base = 16;
            cp++;
        }
        if(!base) base = 8;
    }
    if(!base) base = 10;
    while(isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
                    ? toupper(*cp) : *cp)-'A'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if(endp)
        *endp = (char *)cp;
    return result;
}
*/

