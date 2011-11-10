/******************************************************************************/
/* Project : Unite!       File : zmodem transmit       Version : 1.02         */
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
#include <time.h>
#include "version.h"
#include <unistd.h>

#include "zmodem.h"
#include "zmdm.h"
#include "opts.h"

#define MAX_SUBPACKETSIZE 1024

int port_fd;
char *line = NULL;		/* device to use for io */
int opt_v = FALSE;		/* show progress output */
int opt_d = FALSE;		/* show debug output */
int subpacket_size = MAX_SUBPACKETSIZE;	/* data subpacket size. may be modified during a
					 * session */
int n_files_remaining;
unsigned char tx_data_subpacket[1024];

long current_file_size;
time_t transfer_start;

/*
 * show the progress of the transfer like this:
 * zmtx: sending file "garbage" 4096 bytes ( 20%)
 */

void show_progress(char *name, FILE * fp)
{
    time_t duration;
    int cps;
    int percentage;

    if (current_file_size > 0) {
	percentage = (ftell(fp) * 100) / current_file_size;
    } else {
	percentage = 100;
    }

    duration = time(NULL) - transfer_start;

    if (duration == 0l) {
	duration = 1l;
    }
    cps = ftell(fp) / duration;

    fprintf(stderr, "sending file \"%s\" %8ld bytes (%3d %%/%5d cps)\r",
	    name, ftell(fp), percentage, cps);
    fflush(stderr);
}

/*
 * send from the current position in the file
 * all the way to end of file or until something goes wrong.
 * (ZNAK or ZRPOS received)
 * the name is only used to show progress
 */

int send_from(char *name, FILE * fp)
{
    int n;
    int type = ZCRCG;
    char zdata_frame[] = {ZDATA, 0, 0, 0, 0};
    static count = 0;

    /*
 * put the file position in the ZDATA frame
     */

    zdata_frame[ZP0] = ftell(fp) & 0xff;
    zdata_frame[ZP1] = (ftell(fp) >> 8) & 0xff;
    zdata_frame[ZP2] = (ftell(fp) >> 16) & 0xff;
    zdata_frame[ZP3] = (ftell(fp) >> 24) & 0xff;

    tx_header(zdata_frame);
    /*
     * send the data in the file
     */

    while (!feof(fp)) {
//	if (opt_v) {
	if (++count > 4) {
	    count = 0;
	    show_progress(name, fp);
	}
	/*
	 * read a block from the file
	 */
	n = fread(tx_data_subpacket, 1, subpacket_size, fp);

	if (n == 0) {
	    /*
	     * nothing to send ?
	     */
	    break;
	}
	/*
	 * at end of file wait for an ACK
	 */
	if (ftell(fp) == current_file_size) {
	    type = ZCRCW;
	}

	tx_data(type, tx_data_subpacket, n);

	if (type == ZCRCW) {
	    int type;
	    do {
		type = rx_header(10000);
		if (type == ZNAK || type == ZRPOS) { printf("exit here...");
		    return type;
		}
	    } while (type != ZACK);

	    if (ftell(fp) == current_file_size) {
		if (opt_d) {
		    fprintf(stderr, "end of file\n");
		}
		return ZACK;
	    }
	}
	/* characters from the other side check out that header */

	while (rx_poll()) {
	    int type;
	    int c;
	    c = rx_raw();
	    if (c == ZPAD) {
		type = rx_header(1000);
		if (type != TIMEOUT && type != ACK) {
		    return type;
		}
	    }
	}
    }

    /*
     * end of file reached.
     * should receive something... so fake ZACK
     */

    return ZACK;
}

/*
 * send a file; returns true when session is aborted.
 * (using ZABORT frame)
 */

int send_file(char *name)
{
    long pos;
    long size;
    struct stat s;
    FILE *fp;
    unsigned char *p;
    char zfile_frame[] = {ZFILE, 0, 0, 0, 0};
    char zeof_frame[] = {ZEOF, 0, 0, 0, 0};
    int wait_for_header;
    int type;
    char *n;

    if (opt_v) {
	fprintf(stderr, "zmtx: sending file \"%s\"\r", name);
    }
    /*
     * before doing a lot of unnecessary work check if the file exists
     */
    fp = fopen(name, "rb");

    if (fp == NULL) {
	if (opt_v) {
	    fprintf(stderr, "zmtx: can't open file %s\n", name);
	}
	return;
    }
    fstat(fileno(fp), &s);
    size = s.st_size;
    current_file_size = size;

    /*
     * the file exists. now build the ZFILE frame
     */

    /*
     * set conversion option
     * (not used; always binary)
     */

    zfile_frame[ZF0] = ZF0_ZCBIN;

    /*
     * management option
     */

    if (management_protect) {
	zfile_frame[ZF1] = ZF1_ZMPROT;
	if (opt_d) {
	    fprintf(stderr, "zmtx: protecting destination\n");
	}
    }
    if (management_clobber) {
	zfile_frame[ZF1] = ZF1_ZMCLOB;
	if (opt_d) {
	    fprintf(stderr, "zmtx: overwriting destination\n");
	}
    }
    if (management_newer) {
	zfile_frame[ZF1] = ZF1_ZMNEW;
	if (opt_d) {
	    fprintf(stderr, "zmtx: overwriting destination if newer\n");
	}
    }
    /*
     * transport options
     * (just plain normal transfer)
     */

    zfile_frame[ZF2] = ZF2_ZTNOR;

    /*
     * extended options
     */

    zfile_frame[ZF3] = 0;

    /*
 * now build the data subpacket with the file name and lots of other
     * useful information.
     */

    /*
     * first enter the name and a 0
     */

    p = tx_data_subpacket;

    /*
     * strip the path name from the filename
     */

    n = strrchr(name, '/');
    if (n == NULL) {
	n = name;
    } else {
	n++;
    }

    strcpy(p, n);

    p += strlen(p) + 1;

    /*
     * next the file size
     */

    sprintf(p, "%ld ", size);

    p += strlen(p);

    /*
 * modification date
     */

    sprintf(p, "%lo ", s.st_mtime);

    p += strlen(p);

    /*
     * file mode
     */

    sprintf(p, "0 ");

    p += strlen(p);

    /*
     * serial number (??)
     */

    sprintf(p, "0 ");

    p += strlen(p);

    /*
     * number of files remaining
     */

    sprintf(p, "%d ", n_files_remaining);

    p += strlen(p);

    /*
     * file type
     */

    sprintf(p, "0");

    p += strlen(p) + 1;

    do {
	/*
 	 * send the header and the data
 	 */

	tx_header(zfile_frame);
	tx_data(ZCRCW, tx_data_subpacket, p - tx_data_subpacket);

	/*
	 * wait for anything but an ZACK packet
	 */

	do {
	    type = rx_header(10000);
	} while (type == ZACK);

	if (opt_d) {
	    fprintf(stderr, "type : %d\n", type);
	}
	if (type == ZSKIP) {
	    fclose(fp);
	    if (opt_v) {
		fprintf(stderr, "zmtx: skipped file \"%s\"                       \n", name);
	    }
	    return;
	}
    } while (type != ZRPOS);

    transfer_start = time(NULL);

    do {
	/*
	 * fetch pos from the ZRPOS header
	 */

	if (type == ZRPOS) {
	    pos = rxd_header[ZP0] | (rxd_header[ZP1] << 8) | (rxd_header[ZP2] << 16) | (rxd_header[ZP3] << 24);
	}
	/*
	 * seek to the right place in the file
	 */

	fseek(fp, pos, 0);

	/*
	 * and start sending
	 */

	type = send_from(n, fp);

	if (type == ZFERR || type == ZABORT) {
	    fclose(fp);
	    return TRUE;
	}
    } while (type == ZRPOS || type == ZNAK);

    /*
     * file sent. send end of file frame
     * and wait for zrinit. if it doesnt come then try again
     */

    zeof_frame[ZP0] = s.st_size & 0xff;
    zeof_frame[ZP1] = (s.st_size >> 8) & 0xff;
    zeof_frame[ZP2] = (s.st_size >> 16) & 0xff;
    zeof_frame[ZP3] = (s.st_size >> 24) & 0xff;

    do {
	tx_hex_header(zeof_frame);
	type = rx_header(10000);
    } while (type != ZRINIT);

    /*
     * and close the input file
     */

    if (opt_v) {
	fprintf(stderr, "zmtx: sent file \"%s\"                                    \n", name);
    }
    fclose(fp);

    return FALSE;
}

void cleanup(void)
{
    fd_exit();
}

void usage(void)
{
    printf("zmtx %s (C) Mattheij Computer Service 1994\n", VERSION);
    printf("usage : zmtx options files\n");
    printf("	-lline      line to use for io\n");
    printf("	-n    		transfer if source is newer\n");
    printf("	-o    	    overwrite if exists\n");
    printf("	-p          protect (don't overwrite if exists)\n");
    printf("\n");
    printf("	-d          debug output\n");
    printf("	-v          verbose output\n");
    printf("	(only one of -n -c or -p may be specified)\n");

    cleanup();

    exit(1);
}

int
main(int argc, char **argv)
{
    int i, c, fdflags;
    char *s;
    
    extern int optind;
    extern char *optarg;

    int opt_d = 0;
    int opt_v = 0;
    int management_protect = 0;
    int management_newer = 0;
    int management_clobber = 0;

    while ((c = getopt (argc, argv, "dnvopl:")) != EOF) 
	switch (c) {
	case 'd':
	    opt_d = 1; 
	    n_files_remaining++;
	    break;
	case 'n':
	    management_newer = 1; 
	    n_files_remaining++;
	    break;
	case 'o':
	    management_clobber = 1;
	    n_files_remaining++;
	    break;
	case 'v':
	    opt_v = 1;
	    n_files_remaining++;
	case 'p':
	    management_protect = 1;
	    n_files_remaining++;
	    break;
	case 'l':
	    line = optarg; 
	    n_files_remaining++;
	    break;
	case '?':
        default:
	    usage();
	}

    if (optind < argc && !line)
	line = argv[optind++];

    if (opt_d) {
	opt_v = 1;
    }

    /* Open the line... */
#ifdef DEBUG
    printf("opening line '%s'\n", line);
#endif

    /*
     * Open the serial device and set it up to be the ppp interface.
     * First we open it in non-blocking mode so we can set the
     * various termios flags appropriately.  If we aren't dialling
     * out and we want to use the modem lines, we reopen it later
     * in order to wait for the carrier detect signal from the modem.
     */
    port_fd = open(line, O_RDWR | O_NONBLOCK, 0);
    if (port_fd == -1) {
	printf("unable to open port\n");
	exit(2);
    }
    else {
	printf("opened %s ok.. reseting modes\n", line);
	if ((fdflags = fcntl(port_fd, F_GETFL)) == -1
	    || fcntl(port_fd, F_SETFL, fdflags & ~O_NONBLOCK) < 0)
	    fprintf(stderr, "Couldn't reset non-blocking mode on device");
    }

    /* Set the io device to transparent */
    fd_init();

    /*
     * Clear the input queue from any possible garbage
     * this also clears a possible ZRINIT from an already started
     * zmodem receiver. this doesn't harm because we reinvite to
     * receive again below and it may be that the receiver whose
     * ZRINIT we are about to wipe has already died.
     */
    rx_purge();

    /* establish contact with the receiver */
    if (opt_v) {
	fprintf(stderr, "zmtx: establishing contact with receiver...");
    }

    i = 0;
    do {
	unsigned char zrqinit_header[] = {ZRQINIT, 0, 0, 0, 0};
	i++;
	if (i > 10) {
	    fprintf(stderr, "can't establish contact with receiver.\n");
	    cleanup();
	    exit(3);
	}
	tx_raw('z');
	tx_raw('m');
	tx_raw(13);
	tx_hex_header(zrqinit_header);
    } while (rx_header(7000) != ZRINIT);

    if (opt_v) {
	fprintf(stderr, "contact established...");
	fprintf(stderr, "starting file transfer\n");
    }
    /*
     * decode receiver capability flags
     * forget about encryption and compression.
     */

    can_full_duplex = (rxd_header[ZF0] & ZF0_CANFDX) != 0;
    can_overlap_io = (rxd_header[ZF0] & ZF0_CANOVIO) != 0;
    can_break = (rxd_header[ZF0] & ZF0_CANBRK) != 0;
    can_fcs_32 = (rxd_header[ZF0] & ZF0_CANFC32) != 0;
    escape_all_control_characters = (rxd_header[ZF0] & ZF0_ESCCTL) != 0;
    escape_8th_bit = (rxd_header[ZF0] & ZF0_ESC8) != 0;

    use_variable_headers = (rxd_header[ZF1] & ZF1_CANVHDR) != 0;

    if (opt_d) {
	fprintf(stderr, "receiver %s full duplex\n", can_full_duplex ? "can" : "can't");
	fprintf(stderr, "receiver %s overlap io\n", can_overlap_io ? "can" : "can't");
	fprintf(stderr, "receiver %s break\n", can_break ? "can" : "can't");
	fprintf(stderr, "receiver %s fcs 32\n", can_fcs_32 ? "can" : "can't");
	fprintf(stderr, "receiver %s escaped control chars\n", escape_all_control_characters ? "requests" : "doesn't request");
	fprintf(stderr, "receiver %s escaped 8th bit\n", escape_8th_bit ? "requests" : "doesn't request");
	fprintf(stderr, "receiver %s use variable headers\n", use_variable_headers ? "can" : "can't");
    }
    /* and send each file in turn */

    n_files_remaining = argc - n_files_remaining;

    while (n_files_remaining) { 
	if (send_file( argv[argc - n_files_remaining] )) { 
	    if (opt_v) {
		fprintf(stderr, "zmtx: remote aborted.\n");
	    }
	    break;
	}
	n_files_remaining--;
    }

    /*
     * close the session
     */

    if (opt_v) {
	fprintf(stderr, "zmtx: closing the session...");
    } {
	int type;
	unsigned char zfin_header[] = {ZFIN, 0, 0, 0, 0};

	tx_hex_header(zfin_header);
	do {
	    type = rx_header(10000);
	} while (type != ZFIN && type != TIMEOUT);

	/*
	 * these Os are formally required; but they don't do a thing
	 * unfortunately many programs require them to exit
	 * (both programs already sent a ZFIN so why bother ?)
	 */

	if (type != TIMEOUT) {
	    tx_raw('O');
	    tx_raw('O');
	}
    }

    /*
     * c'est fini
     */

    if (opt_d | opt_v) {
	fprintf(stderr, "zmtx: cleanup and exit\n");
    }
    cleanup();

    exit(0);

    return 0;
}


