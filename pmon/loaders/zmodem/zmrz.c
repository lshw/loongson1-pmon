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
#include <time.h>
#include "version.h"

#include "zmodem.h"
#include "zmdm.h"

long mdate;			/* file date of file being received */
char filename[0x80];		/* filename of file being received */
char *name;			/* pointer to the part of the filename used in the actual
				 * open */

unsigned char rx_data_subpacket[8192];	/* zzap = 8192 */

long current_file_size;
time_t transfer_start;
long file_pos;
FILE *logfp;

/*
 * receive a header and check for garbage
 */

/*
 * receive file data until the end of the file or until something goes wrong.
 * the name is only used to show progress
 */

int receive_file_data(FILE *fp, char *name, char *buf, int verbose)
{
	long pos;
	int n;
	int type;

	/*
	 * create a ZRPOS frame and send it to the other side
	 */

	tx_pos_header(ZRPOS, file_pos);

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
		fprintf(logfp, "got ZDATA header...\n");
#endif
		pos = rxd_header[ZP0] | (rxd_header[ZP1] << 8) |
			(rxd_header[ZP2] << 16) | (rxd_header[ZP3] << 24);
	} while (pos != file_pos);

	do {
		type = rx_data(rx_data_subpacket, &n);

#ifdef DEBUG
		fprintf(logfp, "reading subpacket len %d type(expecting 2, -1) = %d!\n", n, type);
#endif

		if (type == ENDOFFRAME || type == FRAMEOK) {
			memcpy ((buf + file_pos), rx_data_subpacket, n);
			file_pos += n-1;
		}
	} while (type == FRAMEOK);

	return type;
}

void tx_zrinit()
{
	unsigned char zrinit_header[] = {ZRINIT, 0, 0, 0, 4 | ZF0_CANFDX | ZF0_CANOVIO | ZF0_CANFC32};
    
	tx_hex_header(zrinit_header);
}

int
receiveFile(FILE *fp, char *buf, int bufsize, int verbose)
{
	int l;
	int i;
	int type;
	long size;

	fd_init(fp);

	file_pos = 0;
	
	/*
	 * establish contact with the sender
	 */

	if (verbose) {
		fprintf(logfp, "zmrx: establishing contact with sender\n");
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
			fprintf(logfp, "zmrx: can't establish contact with sender\n");
			return(3);
		}
		tx_zrinit();
		type = rx_header(7000);

#ifdef DEBUG
		if (type == ZRQINIT)
			fprintf(logfp, "main: got download invitation\n");
		else
			fprintf(logfp, "main: got frame type = %d\n", type);
#endif
	} while (type == TIMEOUT || type == ZRQINIT);
	fprintf(logfp, "After While\n");
	if (verbose) {
		fprintf(logfp, "zmrx: contact established\n");
		fprintf(logfp, "zmrx: starting file transfer\n");
	}

	/* and receive files (other packets are acknowledged with a ZCOMPL but ignored.) */
	
	do {
		switch (type) {
		case ZFILE:
#ifdef DEBUG
			fprintf(logfp, "main: beginning of receive file...\n");
#endif
			/*
			 * read the data subpacket containing the file information
			 */
#ifdef DEBUG
			fprintf(logfp, "receive_file: beginning of read data subpacket...\n");
#endif
			type = rx_data(rx_data_subpacket, &l);
#ifdef DEBUG
			fprintf(logfp, "receive_file: end of read data subpacket...\n");
#endif
			if (type != FRAMEOK && type != ENDOFFRAME) {
				if (type != TIMEOUT) {
					/*
					 * file info data subpacket was trashed
					 */
					tx_znak();
				}
				return(0);
			}
			/*
			 * extract the relevant info from the header.
			 */
			strcpy(filename, rx_data_subpacket);
#ifdef DEBUG
			fprintf(logfp, "receive_file: filename in the data subpacket is %s...\n", filename);
#endif
			if (verbose) {
				fprintf(logfp, "receiving file \"%s\"\n", filename);
			}
			sscanf(rx_data_subpacket + strlen(rx_data_subpacket) + 1,
			       "%ld %lo", &size, &mdate);
	
			current_file_size = size;
			fprintf(logfp, "%s: file size=%d\n", __FUNCTION__, size);
#ifdef DEBUG
			fprintf(logfp, "receive_file: to call receive_file_data...\n");
#endif
			while (file_pos != size) {
			      fprintf(logfp,"%s: file_pos=%d, size=%d\n", __FUNCTION__, file_pos, size);
			   type = receive_file_data(fp, filename, buf, verbose);
				if (type == ZEOF) {
					break;
				}
			}
#ifdef DEBUG
			fprintf(logfp, "receive_file: end of receive_file_data...\n");
#endif
			/*
			 * wait for the eof header
			 */
			while (type != ZEOF) {
				type = rx_header_and_check(10000);
			}
			/*
			 * and close the input file
			 */
#ifdef DEBUG
			fprintf(logfp, "main: ...end of receive file\n");
#endif
			break;
		default:
			tx_pos_header(ZCOMPL, 0l);
#ifdef DEBUG
			fprintf(logfp, "main: got unknown header type = %d\n", type);
#endif
			break;
		}
		do {
#ifdef DEBUG
			fprintf(logfp, "main: ask for next file transfer...\n");
#endif
			tx_zrinit();

			type = rx_header(7000);
#ifdef DEBUG
			fprintf(logfp, "main: waiting for ZFIN header...\n");
#endif
		} while (type == TIMEOUT);
	} while (type != ZFIN);
	/*
	 * close the session
	 */
	if (verbose) {
		fprintf(logfp, "zmrx: closing the session\n");
	}
	{
		unsigned char zfin_header[] = {ZFIN, 0, 0, 0, 0};
		
		tx_hex_header(zfin_header);
	}
	/*
	 * wait for the over and out sequence
	 */
	{
		int c;
		do {
			c = rx_raw(0);
		} while (c != 'O' && c != TIMEOUT);
		if (c != TIMEOUT) {
			do {
				c = rx_raw(0);
			} while (c != 'O' && c != TIMEOUT);
		}
	}
#ifdef DEBUG    
	fprintf(logfp, "zmrx: cleanup and exit\n");
#endif
	return 0;			/* to stop the compiler from complaining */
}

