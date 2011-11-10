/******************************************************************************/
/* Project : Unite!       File : zmodem receive        Version : 1.02         */
/*                                                                            */
/* (C) Mattheij Computer Service 1994                                         */
/*                                                                            */
/* contact us through (in order of preference)                                */
/*                                                                            */
/*   email:          jacquesm@hacktic.nl                                      */
/*   mail:           MCS                                                      */
/*                   Prinses Beatrixlaan 535                                  */
/*                   2284 AT  RIJSWIJK                                        */
/*                   The Netherlands                                          */
/*   voice phone:    31+070-3936926                                           */
/******************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#include <sys/utime.h>
#include "version.h"

#include "zmodem.h"
#include "zmdm.h"
#include "opts.h"

//
#define DEBUG
//
#define FILE_CHECK

extern int port_fd;

FILE *fp = NULL;		/* fp of file being received or NULL */
long mdate;			/* file date of file being received */
char filename[0x80];		/* filename of file being received */
char *name;			/* pointer to the part of the filename used in the actual
				 * open */

char *line = NULL;		/* device to use for io */
int opt_v = FALSE;		/* show progress output */
int opt_d = FALSE;		/* show debug output */
int opt_q = FALSE;
int junk_pathnames = FALSE;	/* junk incoming path names or keep them */
unsigned char rx_data_subpacket[8192];	/* zzap = 8192 */

long current_file_size;
time_t transfer_start;

/*
 * show the progress of the transfer like this:
 * zmrx: receiving file "garbage" 4096 bytes ( 20%)
 * avoids the use of floating point.
 */

void show_progress(char *name, FILE * fp)
{
    int percentage;
    time_t duration;
    int cps;

    if (current_file_size > 0) {
	percentage = (ftell(fp) * 100) / current_file_size;
    } else {
	percentage = 100;
    }

    duration = time(NULL) - transfer_start;

    if (duration == 0l) {
	duration = 1;
    }
    cps = ftell(fp) / duration;

    fprintf(stderr, "receiving file \"%s\" %8ld bytes (%3d %%/%5d cps)\r",
	    name, ftell(fp), percentage, cps);
}

/*
 * receive a header and check for garbage
 */

/*
 * receive file data until the end of the file or until something goes wrong.
 * the name is only used to show progress
 */

int receive_file_data(char *name, FILE * fp)
{
    static int first = TRUE;
    long pos;
    int n;
    int type;

    /*
 * create a ZRPOS frame and send it to the other side
     */

    tx_pos_header(ZRPOS, ftell(fp));

/*	fpr
intf(stderr,"re-transmit from %d\n",ftell(fp));
*/
    /*
     * wait for a ZDATA header with the right file offset
     * or a timeout or a ZFIN
     */

    do {
	do {
	    type = rx_header(10000);
	    if (type == TIMEOUT) {
		return TIMEOUT;
	    }
	} while (type != ZDATA);

#ifdef DEBUG
	fprintf(stderr, "got ZDATA header...\n");
#endif

	pos = rxd_header[ZP0] | (rxd_header[ZP1] << 8) |
	    (rxd_header[ZP2] << 16) | (rxd_header[ZP3] << 24);
    } while (pos != ftell(fp));

    do {
	type = rx_data(rx_data_subpacket, &n);

/*		fprintf(stderr,"packet len %d type %d\n",n,type);
*/

#ifdef DEBUG
	fprintf(stderr, "reading subpacket len %d type(expecting 2, -1) = %d!\n", n, type);
#endif

	if (type == ENDOFFRAME || type == FRAMEOK) {
	    fwrite(rx_data_subpacket, 1, n, fp);
	}
	if (opt_v) {
	    show_progress(name, fp);
	}
    } while (type == FRAMEOK);

    return type;
}

void tx_zrinit()
{
    unsigned char zrinit_header[] = {ZRINIT, 0, 0, 0, 4 | ZF0_CANFDX | ZF0_CANOVIO | ZF0_CANFC32};

    tx_hex_header(zrinit_header);
}


/*
 * receive a file
 * if the file header info packet was garbled then send a ZNAK and return
 * (using ZABORT frame)
 */

void receive_file()
{
    long size;
    struct stat s;
    int type;
    int l;
    int clobber;
    int protect;
    int newer;
    int exists;
    struct utimbuf tv;
    char *mode = "wb";


    /*
     * fetch the management info bits from the ZRFILE header
     */

    /*
     * management option
     */

    if (management_protect || (rxd_header[ZF1] & ZF1_ZMPROT)) {
	protect = TRUE;
    } else {
	if (management_clobber || (rxd_header[ZF1] & ZF1_ZMCLOB)) {
	    clobber = TRUE;
	}
    }

    if (management_newer || (rxd_header[ZF1] & ZF1_ZMNEW)) {
	newer = TRUE;
    }
    /*
     * read the data subpacket containing the file information
     */

#ifdef DEBUG
    fprintf(stderr, "receive_file: beginning of read data subpacket...\n");
#endif

    type = rx_data(rx_data_subpacket, &l);

#ifdef DEBUG
    fprintf(stderr, "receive_file: end of read data subpacket...\n");
#endif

    if (type != FRAMEOK && type != ENDOFFRAME) {
	if (type != TIMEOUT) {
	    /*
	     * file info data subpacket was trashed
	     */
	    tx_znak();
	}
	return;
    }
    /*
     * extract the relevant info from the header.
     */

    strcpy(filename, rx_data_subpacket);

#ifdef DEBUG
    fprintf(stderr, "receive_file: filename in the data subpacket is %s...\n", filename);
#endif

    if (junk_pathnames) {
	name = strrchr(filename, '/');
	if (name != NULL) {
	    name++;
	} else {
	    name = filename;
	}
    } else {
	name = filename;
    }

    if (opt_v) {
	fprintf(stderr, "receiving file \"%s\"\r", name);
    }
    sscanf(rx_data_subpacket + strlen(rx_data_subpacket) + 1,
	   "%ld %lo", &size, &mdate);

    current_file_size = size;

    /*
     * decide whether to transfer the file or skip it
     */

#ifdef FILE_CHECK
    fp = fopen(name, "rb");

    if (fp != NULL) {
	exists = TRUE;

	fstat(fileno(fp), &s);

	fclose(fp);
    } else {
	exists = FALSE;
    }

    /*
     * if the file already exists here the management options need to
     * be checked..
     */
    if (exists) {
	if (mdate == s.st_mtime) {
	    /*
	     * this is crash recovery
	     */
	    mode = "ab";
	} else {
	    /*
     	 * if the file needs to be protected then exit here.
	     */
	    if (protect) {
		tx_pos_header(ZSKIP, 0L);
		return;
	    }
	    /*
	     * if it is not ok to just overwrite it
	     */
	    if (!clobber) {
		/*
		 * if the remote file has to be newer
		 */
		if (newer) {
		    if (mdate < s.st_mtime) {
			tx_pos_header(ZSKIP, 0L);
			/*
		 	 * and it isnt then exit here.
		 	 */
			return;
		    }
		}
	    }
	}
    }
#endif

    /*
 * transfer the file
     * either not present; remote newer; ok to clobber or no options set.
     * (no options->clobber anyway)
     */

    fp = fopen(name, mode);

    if (fp == NULL) {
	tx_pos_header(ZSKIP, 0L);
	if (opt_v) {
	    fprintf(stderr, "zmrx: can't open file %s\n", name);
	}
	return;
    }
    transfer_start = time(NULL);

#ifdef DEBUG
    fprintf(stderr, "receive_file: to call receive_file_data...\n");
#endif

    while (ftell(fp) != size) {
	type = receive_file_data(filename, fp);
	if (type == ZEOF) {
	    break;
	}
    }

#ifdef DEBUG
    fprintf(stderr, "receive_file: end of receive_file_data...\n");
#endif

    /*
 * wait for the eof header
     */

    while (type != ZEOF) {
	type = rx_header_and_check(10000);
    }

    /*
     * close and exit
     */

    fclose(fp);

    fp = NULL;

    /*
     * set the time
     */

    tv.actime = mdate;
    tv.modtime = mdate;

    utime(name, &tv);

    /*
     * and close the input file
     */

    if (opt_v) {
	fprintf(stderr, "zmrx: received file \"%s\"\n", name);
    }
}

void cleanup(void)
{
    struct utimbuf tv;

    if (fp) {
	fflush(fp);
	fclose(fp);
	/*
	 * set the time (so crash recovery may work)
	 */

	tv.actime = mdate;
	tv.modtime = mdate;

	utime(name, &tv);
    }
    fd_exit();
}


void usage(void)
{
    printf("zmrx %s (C) Mattheij Computer Service 1994\n", VERSION);
    printf("usage : zmrx options\n");
    printf("	-lline      line to use for io\n");
    printf("	-j    		junk pathnames\n");
    printf("	-n    		transfer if source is newer\n");
    printf("	-o    	    overwrite if exists\n");
    printf("	-p          protect (don't overwrite if exists)\n");
    printf("\n");
    printf("	-d          debug output\n");
    printf("	-v          verbose output\n");
    printf("	-q          quiet\n");
    printf("	(only one of -n -c or -p may be specified)\n");

    cleanup();

    exit(1);
}

int main(int argc, char **argv)
{
    int i, x, fdflags;
    char *s;
    int type;
   
    argv++;
    while (--argc > 0 && ((*argv)[0] == '-')) {
	for (s = argv[0] + 1; *s != '\0'; s++) {
	    switch (toupper(*s)) {
		OPT_BOOL('D', opt_d);
		OPT_BOOL('V', opt_v);
		OPT_BOOL('Q', opt_q);

		OPT_BOOL('N', management_newer);
		OPT_BOOL('O', management_clobber);
		OPT_BOOL('P', management_protect);
		OPT_BOOL('J', junk_pathnames);
		OPT_STRING('L', line);
	    default:
		printf("zmrx: bad option %c\n", *s);
		usage();
	    }
	}
	argv++;
    }

    if (opt_d) {
	opt_v = TRUE;
    }
    if (opt_q) {
	opt_v = FALSE;
	opt_d = FALSE;
    }

#if 0
    if (!opt_v) {
	freopen("/usr/src/utils/zmnew/trace", "w", stderr);
	setbuf(stderr, NULL);
    }
#endif

    if ((management_newer + management_clobber + management_protect) > 1 || argc != 0) {
	usage();
    }
    if (line != NULL) {
	if ((port_fd = open(line, O_RDWR | O_NONBLOCK)) == -1)
			  fprintf(stderr, "unable to open port\n");
	else {
	    printf("opened %s ok.. reseting modes\n", line);
	    if ((fdflags = fcntl(port_fd, F_GETFL)) == -1
		|| fcntl(port_fd, F_SETFL, fdflags & ~O_NONBLOCK) < 0)
		fprintf(stderr, "Couldn't reset non-blocking mode on device");
	}

#if 0
	if (stdin == NULL) {
	    fprintf(stderr, "zmrx can't open line for input %s\n", line);
	    exit(2);
	}
	fclose(stdout);
	stdout = fdopen(x, "w");
	if (stdout == NULL) {
	    fprintf(stderr, "zmrx can't open line for output %s\n", line);
	    exit(2);
	}
#endif
    }

    /*
     * set the io device to transparent
     */

    fd_init();

    /*
     * establish contact with the sender
     */

    if (opt_v) {
	fprintf(stderr, "zmrx: establishing contact with sender\n");
    }
    /*
     * make sure we dont get any old garbage
     */

    rx_purge();

    /*
     * loop here until contact is established.
     * another packet than a ZRQINIT should be received.
     */

    i = 0;
    do {
	i++;
	if (i > 10) {
	    fprintf(stderr, "zmrx: can't establish contact with sender\n");
	    cleanup();
	    exit(3);
	}
	tx_zrinit();
	type = rx_header(7000);

#ifdef DEBUG
	if (type == ZRQINIT)
	    fprintf(stderr, "main: got download invitation\n");
	else
	    fprintf(stderr, "main: got frame type = %d\n", type);
#endif
    } while (type == TIMEOUT || type == ZRQINIT);

    if (opt_v) {
	fprintf(stderr, "zmrx: contact established\n");
	fprintf(stderr, "zmrx: starting file transfer\n");
    }
    /* and receive files (other packets are acknowledged with a ZCOMPL but ignored.) */

    do {
	switch (type) {
	case ZFILE:

#ifdef DEBUG
	    fprintf(stderr, "main: beginning of receive file...\n");
#endif

	    receive_file();

#ifdef DEBUG
	    fprintf(stderr, "main: ...end of receive file\n");
#endif

	    break;
	default:
	    tx_pos_header(ZCOMPL, 0l);

#ifdef DEBUG
	    fprintf(stderr, "main: got unknown header type = %d\n", type);
#endif

	    break;
	}

	do {

#ifdef DEBUG
	    fprintf(stderr, "main: ask for next file transfer...\n");
#endif

	    tx_zrinit();

	    type = rx_header(7000);

#ifdef DEBUG
	    fprintf(stderr, "main: waiting for ZFIN header...\n");
#endif
	} while (type == TIMEOUT);
    } while (type != ZFIN);

    /*
     * close the session
     */

    if (opt_v) {
	fprintf(stderr, "zmrx: closing the session\n");
    } {
	int type;
	unsigned char zfin_header[] = {ZFIN, 0, 0, 0, 0};

	tx_hex_header(zfin_header);
    }

    /*
     * wait for the over and out sequence
     */

    {
	int c;
	do {
	    c = rx_raw();
	} while (c != 'O' && c != TIMEOUT);

	if (c != TIMEOUT) {
	    do {
		c = rx_raw();
	    } while (c != 'O' && c != TIMEOUT);
	}
    }

    if (opt_d) {
	fprintf(stderr, "zmrx: cleanup and exit\n");
    }
    cleanup();

    exit(0);

    return 0;			/* to stop the compiler from complaining */
}










