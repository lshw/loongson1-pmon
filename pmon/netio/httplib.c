/* httplib.c, v1.0 2008/09/07 9:54 */
/*
 *
 * Note:(Http increased download a need to amend some of the things)
 *	1.cons/files:									add "file pmon/netio/httplib.c       inet"
 *	2.ib/libc/open.c:								add"else if(strpat(dname, "http:*"))" in function of open  
 *	3.pmon/netio/netio.c:							add"else {return fd;}" in line 89 of function of netiopen 
 *	4../sys/netinet/in_proto.c:						" #if NTCP > 0"->"#if NTCP = 0" in line 137
 *
 *The first test was a success in zloader.2fdev.cs5536 in 2008/09/07 9:50
 *test begin:
 *  
 *   ifaddr rtk0 10.0.0.187
 *   load tftp://10.0.0.111/gzram
 *   g
 *   ifaddr rtk0 10.0.0.187
 *   load http://10.0.0.111/gzram
 *   g
 *test end;
 *The last test was a success in zloader.2fdev.cs5536 in 2008/09/07 11:13
 *
 */
#undef _KERNEL
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <file.h>
#include <net/if.h>

#include "netio.h"
#include <pmon.h>
#define HPSIZE 0x2000
/*
 * 
 * struct httpfile:
 * sockaddr_in->hostaddr
 * sock       ->ID of socket
 * flags      ->limits of authority
 *
 */
struct httpfile {
	struct sockaddr_in sin;
	int		sock;
	short	flags;
	int		start;
	int		end;
	int		foffs;
	char	buf[HPSIZE];
	int		eof;
};

/*
 *
 * my_buf		->file
 * fp_buf		->Guidelines document
 * buf_size		->size of file
 *
 */
static int  buf_size;

static int	httpopen  (int,  struct Url *, int, int);
static int	httpread  (int, void *, int);
static int	httpwrite ();
static off_t	httplseek (int, long, int );
static int	httpioctl ();
static int	httpclose ();
static int  my_write(char *, int, char *);
static int	my_read(int, struct httpfile*);

/*
 *
 * init struct httpops
 *
 */
static NetFileOps httpops = {
	"http",
	httpopen,
	httpread,
	httpwrite,
	httplseek,
	httpioctl,
	httpclose
};
static void init_netfs __P((void)) __attribute__ ((constructor));

static void
init_netfs()
{
	netfs_init(&httpops);
}

static int
httpopen (int fd, struct Url *url, int flags, int perms)
{
	struct hostent *hp;
	struct httpfile *http;
	NetFile *nfp = (NetFile *)_file[fd].data;
	char hbuf[MAXHOSTNAMELEN];
	int oflags = flags & O_ACCMODE;					
	char *host;
	int connect_fd;
	// get hostname
	if(strlen(url->hostname) != 0)	
	{
		host = url->hostname;
	}
	else
	{
		host = getenv("httphost");
		if(!host)
		{
			log(LOG_INFO, "http: missing/bad host name: %s\n", url->filename);
			errno = EDESTADDRREQ;
			return -1;
		}
	}
	//set up struct httpfile*, and clear 0 
	http = (struct httpfile *)malloc(sizeof (struct httpfile));
	bzero(http, sizeof(struct httpfile));

	nfp->data = (void *)http;

	//set socket,and get number of socket
	http->sock = socket(AF_INET, SOCK_STREAM, 0);

	//sin_family = AF_INET,and bind
	http->sin.sin_family = AF_INET;
	http->sin.sin_port = htons(url->port?url->port:80);
	http->flags = flags;


	//get the message of host
	hp = gethostbyname(host);
	if(hp)
	{
		http->sin.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (void *)&http->sin.sin_addr, hp->h_length);
		strncpy(hbuf, hp->h_name, sizeof(hbuf)-1);
		hbuf[sizeof(hbuf)-1]='\0';
		host = hbuf;
	}
	else
		goto error;
/*
 *
 * connect the httpd
 *
 */
	connect_fd = connect(http->sock,&http->sin, sizeof(http->sin));
	if(connect_fd < 0)
		goto error;

/*
 *
 * my_write			->add head of http,send filename address and port of host
 * my_read			->del head of http,receive file
 *
 */
	my_write(host, http->sock, url->filename);
	my_read(http->sock, http);
	http->end=read(http->sock, http->buf, HPSIZE);
	http->start = 0;
	return 0;
error:
	return -1;
}

	static int 
my_write(char *host, int socketfd, char *filename)
{
	char html_http[1000];
	int n;
	sprintf(html_http, "%s%s%s%s%s", "GET /",filename,
			" HTTP/1.1\r\nHost:", host,":8000\r\n"
			"User-Agent:Mozilla/5.0(X11;U;Linux i686;en-US;rv:1.8.16)Gecko/20061201 Firefox/2.0.0.6(Ubuntu-feisty)\r\n"
			"Accept:text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\n"
			"Accept-Language:en-us,en;q=0.5\r\n"
			"Accept-Encoding:gzip,defalte\r\n"
			"Accept-Charset:ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
			"Keep-Alive:300\r\n"
			"Connecting:keep-alive\r\n\r\n");

again:
	if ((n = write(socketfd, html_http, strlen(html_http))) == -1)
	{
		if (errno == EINTR)
			goto again;
		else return -1;
	}
	return n;
}	
static int sum_number = 0;
int my_readline(int fd, char *buf, int count)
{
	char ch[2];
	int number = 0;
	ch[0] = '3';
	while(1)
	{
		read(fd, ch ,1);
		*buf++ = ch[0];
		number++;
		if(ch[0] == '\n')
			break;
	}
	sum_number += number;
	return number; 
}
	static int 
my_read(int socketfd, struct httpfile *http)
{
	char rhtml_http[1000];
	char buf_length[20];
	char *buf_c_l;
	int buf_len = 0;
	int n;
	char ch[2];
	while((n = my_readline(socketfd, rhtml_http, 1000)) > 2)
	{
//		write(1, rhtml_http, n);
		if(strstr(rhtml_http, "Content-Length") != NULL)
		{
			buf_c_l = rhtml_http + 16;
			strcpy(buf_length, buf_c_l);
			buf_len = atoi(buf_length);
			buf_size = buf_len;
		}
		memset(rhtml_http, 0 , 1000);
	}
	
	return 0;
}

/*
 *
 * read the buf		->read the file
 *
 */
static int
httpread (int fd,void* buf,int nread)
{
	struct httpfile *http;
	NetFile	*nfp;
	int nb, n;
	int nj;
	char *buf_flag;
	nfp = (NetFile *)_file[fd].data;
	http = (struct httpfile *)nfp->data;
	buf_flag = http->buf;

	for (nb = nread; nb != 0 && http->start < http->end; ) {

		if (http->foffs >= http->start && http->foffs < http->end) {
			/* got some data that's in range */
			n = http->end - http->foffs;
			if (n > nb) n = nb;
			bcopy(http->buf+http->foffs-http->start, buf, n);
			http->foffs += n;
			buf += n;
			nb -= n;
		} 
		if (http->foffs >= http->end) {
			http->start = http->end;
			if(http->foffs>=buf_size)break;
			n = read(http->sock, http->buf, HPSIZE);
			if(n<=0)break;
			http->end = http->start + n;
		  if (http->flags & O_NONBLOCK)
		  dotik (100, 0);
		}
	}
	return nread -nb;
}

	static int
httpwrite (fd, buf, nwrite)
    int fd;
    const void *buf;
    int nwrite;
{
	return -1;
}

/*
 *
 * lseek of file
 *
 */
	static off_t
httplseek (int fd, long offs, int how)
{
	struct httpfile *http;
	NetFile *nfp;
	
	nfp = (NetFile *)_file[fd].data;
	http = (struct httpfile *)nfp->data;

	switch (how) {
		case SEEK_SET:
			http->foffs = offs; 
			break;
		case SEEK_CUR:
			http->foffs += offs;
			break;
		case SEEK_END:
		default:
			return -1;
	}
	return http->foffs;
}


static int
httpioctl (fd, op, argp)
    int fd;
    int op;
    void *argp;
{
	return -1;
}

/*
 *
 * close the file
 *
 */
	static int
httpclose (int fd)
{
	NetFile *nfp;
	struct httpfile *http;

	nfp	= (NetFile *)_file[fd].data;
	http = (struct httpfile *)nfp->data;
	close (http->sock);
	free(http);
	return 0;
}
