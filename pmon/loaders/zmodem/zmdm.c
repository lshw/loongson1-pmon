/******************************************************************************/
/* Project : Unite!       File : zmodem general        Version : 1.02         */
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

/*
 * zmodem primitives and other code common to zmtx and zmrx
 */

#include <stdio.h>
#undef _KERNEL
#include <sys/termios.h>
#include <errno.h>
#define _KERNEL
#include <signal.h>
#include <unistd.h>
#include <string.h>

#ifdef UNITE
#include <sys/select.h>
#endif

#ifdef SUNOS4
#include <sys/types.h>
#endif

#include <sys/time.h>

#include "zmodem.h"
#define ZMDM
#include "zmdm.h"
#include "crctab.h"
#include <pmon.h>

int port_fd;
FILE *port_fp;

int receive_32_bit_data;
int raw_trace;
int want_fcs_32 = TRUE;
long ack_file_pos;		/* file position used in acknowledgement of correctly */
 /* received data subpackets */

extern FILE *logfp;

#define XXX_SKIP_CRC 1 /* XXX: Skip CRC check at the moment */

void fd_init(FILE *fp)
{
  port_fd = fileno(fp);
  port_fp = fp;
}
/*
 * read bytes as long as rdchk indicates that
 * more data is available.
 */

void rx_purge(void)
{
    struct timeval t;
    fd_set f;
    unsigned char c;

    t.tv_sec = 0;
    t.tv_usec = 0;

    FD_ZERO(&f);
    FD_SET(port_fd, &f);

    while (select(port_fd, &f, NULL, NULL, &t)) {
	read(port_fd, &c, 1);
    }

}

int last_sent = -1;

/*
 * transmit a character.
 * this is the raw modem interface
 */

void tx_raw(int c)
{

#ifdef DEBUG
    if (raw_trace) {
	fprintf(logfp, "%02x ", c);
    }
#endif

    last_sent = c & 0x7f;

#if 0
    putchar(c);
#else
    {
	char tmp;
	tmp = c;
	write(port_fd, &tmp, 1);
    }
#endif
}

/*
 * transmit a character ZDLE escaped
 */

void tx_esc(int c)
{
    tx_raw(ZDLE);
    /*
     * exclusive or; not an or so ZDLE becomes ZDLEE
     */
    tx_raw(c ^ 0x40);
}

/*
 * transmit a character; ZDLE escaping if appropriate
 */

void tx(unsigned char c)
{
    switch (c) {
	case ZDLE:
	tx_esc(c);
	return;
	break;
    case 0x8d:
    case 0x0d:
	if (escape_all_control_characters && last_sent == '@') {
	    tx_esc(c);
	    return;
	}
	break;
    case 0x10:
    case 0x90:
    case 0x11:
    case 0x91:
    case 0x13:
    case 0x93:
	tx_esc(c);
	return;
	break;
    default:
	if (escape_all_control_characters && (c & 0x60) == 0) {
	    tx_esc(c);
	    return;
	}
	break;
    }
    /*
     * anything that ends here is so normal we might as well transmit it.
     */
    tx_raw((int) c);
}

/*
 * send the bytes accumulated in the output buffer.
 */

void tx_flush(void)
{
    fflush(port_fp);
}

/*
 * transmit a hex header.
 * these routines use tx_raw because we're sure that all the
 * characters are not to be escaped.
 */

void tx_nibble(int n)
{
    n &= 0x0f;
    if (n < 10) {
	n += '0';
    } else {
	n += 'a' - 10;
    }

    tx_raw(n);
}

void tx_hex(int h)
{
    tx_nibble(h >> 4);
    tx_nibble(h);
}

void tx_hex_header(unsigned char *p)
{
    int i;
    unsigned short int crc;

#ifdef DEBUG
    fprintf(logfp, "* tx_hex_header *\n ");
#endif

    tx_raw(ZPAD);
    tx_raw(ZPAD);
    tx_raw(ZDLE);

    if (use_variable_headers) {
	tx_raw(ZVHEX);
	tx_hex(HDRLEN);
    } else {
	tx_raw(ZHEX);
    }

    /*
     * initialise the crc
     */

    crc = 0;

    /*
     * transmit the header
     */

    for (i = 0; i < HDRLEN; i++) {
	tx_hex(*p);
	crc = UPDCRC16(*p, crc);
	p++;
    }

    /*
     * update the crc as though it were zero
     */

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    /* transmit the crc */

    tx_hex(crc >> 8);
    tx_hex(crc);

    /*
     * end of line sequence
     */

    tx_raw(0x0d);
    tx_raw(0x0a);

    tx_raw(XON);

    tx_flush();

#ifdef DEBUG
    fprintf(logfp, "\n");
#endif
}

/*
 * Send ZMODEM binary header hdr
 */

void tx_bin32_header(unsigned char *p)
{
    int i;
    unsigned long crc;

#ifdef DEBUG
    fprintf(logfp, "tx binary header 32 bits crc\n");
    raw_trace = 1;
#endif

    tx_raw(ZPAD);
    tx_raw(ZPAD);
    tx_raw(ZDLE);

    if (use_variable_headers) {
	tx_raw(ZVBIN32);
	tx(HDRLEN);
    } else {
	tx_raw(ZBIN32);
    }

    crc = 0xffffffffL;

    for (i = 0; i < HDRLEN; i++) {
	crc = UPDCRC32(*p, crc);
	tx(*p++);
    }

    crc = ~crc;

    tx(crc);
    tx(crc >> 8);
    tx(crc >> 16);
    tx(crc >> 24);
}

void tx_bin16_header(unsigned char *p)
{
    int i;
    unsigned int crc;

#ifdef DEBUG
    fprintf(logfp, "tx binary header 16 bits crc\n");
#endif

    tx_raw(ZPAD);
    tx_raw(ZPAD);
    tx_raw(ZDLE);

    if (use_variable_headers) {
	tx_raw(ZVBIN);
	tx(HDRLEN);
    } else {
	tx_raw(ZBIN);
    }

    crc = 0;

    for (i = 0; i < HDRLEN; i++) {
	crc = UPDCRC16(*p, crc);
	tx(*p++);
    }

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    tx(crc >> 8);
    tx(crc);
}


/*
 * transmit a header using either hex 16 bit crc or binary 32 bit crc
 * depending on the receivers capabilities
 * we dont bother with variable length headers. I dont really see their
 * advantage and they would clutter the code unneccesarily
 */

void tx_header(unsigned char *p)
{
    if (can_fcs_32) {
	if (want_fcs_32) {
	    tx_bin32_header(p);
	} else {
	    tx_bin16_header(p);
	}
    } else {
	tx_hex_header(p);
    }
}

/*
 * data subpacket transmission
 */

void tx_32_data(int sub_frame_type, unsigned char *p, int l)
{
    unsigned long crc;

#ifdef DEBUG
    fprintf(logfp, "tx_32_data\n");
#endif

    crc = 0xffffffffl;

    while (l > 0) {
	crc = UPDCRC32(*p, crc);
	tx(*p++);
	l--;
    }

    crc = UPDCRC32(sub_frame_type, crc);

    tx_raw(ZDLE);
    tx_raw(sub_frame_type);

    crc = ~crc;

    tx((int) (crc) & 0xff);
    tx((int) (crc >> 8) & 0xff);
    tx((int) (crc >> 16) & 0xff);
    tx((int) (crc >> 24) & 0xff);
}

void tx_16_data(int sub_frame_type, unsigned char *p, int l)
{
    unsigned short crc;

#ifdef DEBUG
    fprintf(logfp, "tx_16_data\n");
#endif

    crc = 0;

    while (l > 0) {
	crc = UPDCRC16(*p, crc);
	tx(*p++);
	l--;
    }

    crc = UPDCRC16(sub_frame_type, crc);

    tx_raw(ZDLE);
    tx_raw(sub_frame_type);

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    tx(crc >> 8);
    tx(crc);
}

/*
 * send a data subpacket using crc 16 or crc 32 as desired by the receiver
 */

void tx_data(int sub_frame_type, unsigned char *p, int l)
{
    if (want_fcs_32 && can_fcs_32) {
	tx_32_data(sub_frame_type, p, l);
    } else {
	tx_16_data(sub_frame_type, p, l);
    }

    if (sub_frame_type == ZCRCW) {
	tx_raw(XON);
    }
    tx_flush();
}

void tx_pos_header(int type, long pos)
{
    char header[5];

    header[0] = type;
    header[ZP0] = pos & 0xff;
    header[ZP1] = (pos >> 8) & 0xff;
    header[ZP2] = (pos >> 16) & 0xff;
    header[ZP3] = (pos >> 24) & 0xff;

    tx_hex_header(header);
}

void tx_znak()
{
#ifdef DEBUG
   fprintf(logfp, "tx_znak\n");
#endif
    tx_pos_header(ZNAK, ack_file_pos);
}

void tx_zskip()
{
    tx_pos_header(ZSKIP, 0L);
}

/*
 * receive any style header within timeout milliseconds
 */

void alrm(int a)
{
    signal(SIGALRM, SIG_IGN);
}

int rx_poll()
{
    struct timeval t;
    fd_set f;

    t.tv_sec = 0;
    t.tv_usec = 0;

    FD_ZERO(&f);
    FD_SET(port_fd, &f);

    if (select(port_fd, &f, NULL, NULL, &t)) {
	return 1;
    }
    return 0;
}

unsigned char inputbuffer[1024];
int n_in_inputbuffer = 0;
int inputbuffer_index;

/*
 * rx_raw ; receive a single byte from the line.
 * reads as many are available and then processes them one at a time
 * check the data stream for 5 consecutive CAN characters;
 * and if you see them abort. this saves a lot of clutter in
 * the rest of the code; even though it is a very strange place
 * for an exit. (but that was wat session abort was all about.)
 */

int rx_raw(int to)
{
    unsigned char c;
    static int n_cans = 0;

    if (n_in_inputbuffer == 0) {
	/*
	 * change the timeout into seconds; minimum is 1
	 */
#if 0
	to /= 1000;

	if (to == 0) {
	    to++;
	}
	/*
 	 * setup an alarm in case io takes too long
	 */

	signal(SIGALRM, alrm);
	
	to /= 1000;
	  
	if (to == 0) {
	  to = 2;
	}

	alarm(to);
#endif
	n_in_inputbuffer = read(port_fd, inputbuffer, 1); /* was 1024 */
#if 0
	if (n_in_inputbuffer <= 0) {
	    n_in_inputbuffer = 0;
	}
#endif
	/*
 	 * cancel the alarm in case it did not go off yet
	 */
#if 0
	signal(SIGALRM, SIG_IGN);
#endif
	if (n_in_inputbuffer < 0 && (errno != 0 && errno != EINTR)) {
	    fprintf(logfp, "zmdm : fatal error reading device\n");
	    return(1);
	}
	if (n_in_inputbuffer == 0) {
	    return TIMEOUT;
	}
	inputbuffer_index = 0;
    }
    c = inputbuffer[inputbuffer_index++];
    n_in_inputbuffer--;

    if (c == CAN) {
	n_cans++;
	if (n_cans == 5) {
#if 0
	    /*
	     * the other side is serious about this. just shut up;
	     * clean up and exit.
	     */
	    cleanup();
#endif
	    return(CAN);
	}
    } else {
	n_cans = 0;
    }

    return c;
}

/*
 * rx; receive a single byte undoing any escaping at the
 * sending site. this bit looks like a mess. sorry for that
 * but there seems to be no other way without incurring a lot
 * of overhead. at least like this the path for a normal character
 * is relatively short.
 */

int rx(int to)
{
    int c;

    /*
     * outer loop for ever so for sure something valid
     * will come in; a timeout will occur or a session abort
     * will be received.
     */

    while (TRUE) {

	/*
 	 * fake do loop so we may continue
	 * in case a character should be dropped.
	 */

	do {
	    c = rx_raw(to);
	    if (c == TIMEOUT) {
		return c;
	    }
	    switch (c) {
	    case ZDLE:
		break;
	    case 0x11:
	    case 0x91:
	    case 0x13:
	    case 0x93:
		continue;
		break;
	    default:
		/*
		 * if all control characters should be escaped and
		 * this one wasnt then its spurious and should be dropped.
		 */
		if (escape_all_control_characters && (c & 0x60) == 0) {
		    continue;
		}
		/*
		 * normal character; return it.
		 */
		return c;
	    }
	} while (FALSE);

	/*
 	 * ZDLE encoded sequence or session abort.
	 * (or something illegal; then back to the top)
	 */

	do {
	    c = rx_raw(to);

	    if (c == 0x11 || c == 0x13 || c == 0x91 || c == 0x93 || c == ZDLE) {
		/*
		 * these can be dropped.
		 */
		continue;
	    }
	    switch (c) {
		/*
		 * these four are really nasty.
		 * for convenience we just change them into
		 * special characters by setting a bit outside the
		 * first 8. that way they can be recognized and still
		 * be processed as characters by the rest of the code.
		 */
	    case ZCRCE:
	    case ZCRCG:
	    case ZCRCQ:
	    case ZCRCW:
		return (c | ZDLEESC);
		break;
	    case ZRUB0:
		return 0x7f;
		break;
	    case ZRUB1:
		return 0xff;
		break;
	    default:
		if (escape_all_control_characters && (c & 0x60) == 0) {
		    /*
		     * a not escaped control character; probably
		     * something from a network. just drop it.
		     */
		    continue;
		}
		/*
		 * legitimate escape sequence.
		 * rebuild the orignal and return it.
		 */
		if ((c & 0x60) == 0x40) {
		    return c ^ 0x40;
		}
		break;
	    }
	} while (FALSE);
    }

    /*
     * not reached.
     */

    return 0;
}

/*
 * receive a data subpacket as dictated by the last received header.
 * return 2 with correct packet and end of frame
 * return 1 with correct packet frame continues
 * return 0 with incorrect frame.
 * return TIMEOUT with a timeout
 * if an acknowledgement is requested it is generated automatically
 * here.
 */

/*
 * data subpacket reception
 */

int rx_32_data(unsigned char *p, int *l)
{
    int c;
    unsigned long rxd_crc;
    unsigned long crc;
    int sub_frame_type;

#ifdef DEBUG
    fprintf(logfp, "rx_32_data\n");
#endif

    crc = 0xffffffffl;

    do {
	c = rx(1000);

	if (c == TIMEOUT) {
	    return TIMEOUT;
	}
	if (c < 0x100) {
	    crc = UPDCRC32(c, crc);
	    *p++ = c;
	    (*l)++;
	    continue;
	}
    } while (c < 0x100);

    sub_frame_type = c & 0xff;

    crc = UPDCRC32(sub_frame_type, crc);

    crc = ~crc;

    rxd_crc = rx(1000);
    rxd_crc |= rx(1000) << 8;
    rxd_crc |= rx(1000) << 16;
    rxd_crc |= rx(1000) << 24;

#ifndef XXX_SKIP_CRC
    if (rxd_crc != crc) {
	return FALSE;
    }
#endif    
    ack_file_pos += *l;

    return sub_frame_type;
}

int rx_16_data(register unsigned char *p, int *l)
{
    register int c;
    int sub_frame_type;
    register unsigned short crc;
    unsigned short rxd_crc;

#ifdef DEBUG
    fprintf(logfp, "rx_16_data:\n");
#endif

    crc = 0;

    do {
	c = rx(5000);

	if (c == TIMEOUT) {
	    return TIMEOUT;
	}
	if (c < 0x100) {
	    crc = UPDCRC16(c, crc);
	    *p++ = c;
	    (*l)++;
	}
    } while (c < 0x100);

    sub_frame_type = c & 0xff;

    crc = UPDCRC16(sub_frame_type, crc);

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    rxd_crc = rx(1000) << 8;
    rxd_crc |= rx(1000);

#ifndef XXX_SKIP_CRC
    if (rxd_crc != crc) {
#ifdef DEBUG
       fprintf(logfp, "%s: bad crc - orininal crc %4.4x calculated crc %4.4x\n", __FUNCTION__, rxd_crc, crc);
#endif
	return FALSE;
    }
#endif
    ack_file_pos += *l;

    return sub_frame_type;
}

int rx_data(unsigned char *p, int *l)
{
    int sub_frame_type;
    long pos;

    /*
     * fill in the file pointer in case acknowledgement is requested.
     * the ack file pointer will be updated in the subpacket read routine;
     * so we need to get it now
     */

    pos = ack_file_pos;

    /*
     * receive the right type of frame
     */

    *l = 0;

    if (receive_32_bit_data) {
	sub_frame_type = rx_32_data(p, l);
    } else {
	sub_frame_type = rx_16_data(p, l);
    }

    switch (sub_frame_type) {
    case TIMEOUT:
	return TIMEOUT;
	break;
	/*
	 * frame continues non-stop
	 */
    case ZCRCG:
	return FRAMEOK;
	break;
	/*
	 * frame ends
	 */
    case ZCRCE:
	return ENDOFFRAME;
	break;
	/*
	 * frame continues; ZACK expected
	 */
    case ZCRCQ:
	tx_pos_header(ZACK, pos);
	return FRAMEOK;
	break;
	/*
	 * frame ends; ZACK expected
	 */
    case ZCRCW:
	tx_pos_header(ZACK, pos);
	return ENDOFFRAME;
	break;
    }

    return FALSE;
}

int rx_nibble(int to)
{
    int c;

    c = rx(to);

    if (c == TIMEOUT) {
	return c;
    }
    if (c > '9') {
	if (c < 'a' || c > 'f') {
	    /*
	     * illegal hex; different than expected.
	     * we might as well time out.
	     */
	    return TIMEOUT;
	}
	c -= 'a' - 10;
    } else {
	if (c < '0') {
	    /*
	     * illegal hex; different than expected.
	     * we might as well time out.
	     */
	    return TIMEOUT;
	}
	c -= '0';
    }

    return c;
}

int rx_hex(int to)
{
    int n1;
    int n0;

    n1 = rx_nibble(to);

    if (n1 == TIMEOUT) {
	return n1;
    }
    n0 = rx_nibble(to);

    if (n0 == TIMEOUT) {
	return n0;
    }
    return (n1 << 4) | n0;
}

/*
 * receive routines for each of the six different styles of header.
 * each of these leaves rxd_header_len set to 0 if the end result is
 * not a valid header.
 */

void rx_bin16_header(int to)
{
    int c;
    int n;
    unsigned short int crc;
    unsigned short int rxd_crc;

#ifdef DEBUG
    fprintf(logfp, "rx_bin16_header: rx binary header 16 bits crc\n");
#endif

    crc = 0;

    for (n = 0; n < 4; n++) {
	c = rx(to);
	if (c == TIMEOUT) {

#ifdef DEBUG
	    fprintf(logfp, "rx_bin16_header: timeout\n");
#endif
	    return;
	}
	crc = UPDCRC16(c, crc);
	rxd_header[n] = c;
    }

    crc = UPDCRC16(ZDLE, crc);

    for (; n < 6; n++) {
       c = rx(to);
       if (c == TIMEOUT) {

#ifdef DEBUG
	  fprintf(logfp, "rx_bin16_header: timeout\n");
#endif
	  return;
       }
       rxd_header[n] = c;
    }

#ifdef DEBUG
    fprintf(logfp, "rxd_header[] = [%2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x]\n",
	    rxd_header[0],
	    rxd_header[1],
	    rxd_header[2],
	    rxd_header[3],
	    rxd_header[4],
	    rxd_header[5]);
#endif
    
    rxd_crc = rxd_header[4] << 8;
    rxd_crc |= rxd_header[5];

#ifndef XXX_SKIP_CRC
    if (rxd_crc != crc) {

#ifdef DEBUG
      fprintf(logfp, "rx_bin16_header: bad crc - orininal crc %4.4x calculated crc %4.4x\n", rxd_crc, crc);
#endif
	return;
    }
#endif
    rxd_header_len = 5;
}

void rx_hex_header(int to)
{
    int c;
    int i;
    unsigned short int crc = 0;
    unsigned short int rxd_crc;

#ifdef DEBUG
    fprintf(logfp, "rx_hex_header :...\n");
#endif

    for (i = 0; i < 5; i++) {
	c = rx_hex(to);
	if (c == TIMEOUT) {
	    return;
	}
	crc = UPDCRC16(c, crc);

	rxd_header[i] = c;
    }

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    /*
     * receive the crc
     */

    c = rx_hex(to);

    if (c == TIMEOUT) {
	return;
    }
    rxd_crc = c << 8;

    c = rx_hex(to);

    if (c == TIMEOUT) {
	return;
    }
    rxd_crc |= c;

    if (rxd_crc == crc) {
	rxd_header_len = 5;
    }

#ifdef DEBUG
    else {
	fprintf(logfp, "bad crc.\n");
    }
#endif

    /*
     * drop the end of line sequence after a hex header
     */
    c = rx(to);
    if (c == CR) {
	/*
	 * both are expected with CR
	 */
	c = rx(to);
    }
}

void rx_bin32_header(int to)
{
    int c;
    int n;
    unsigned long crc;
    unsigned long rxd_crc;

#ifdef DEBUG
    fprintf(logfp, "rx binary header 32 bits crc\n");
#endif

    crc = 0xffffffffL;

    for (n = 0; n < 5; n++) {
	c = rx(1000);
	if (c == TIMEOUT) {
	    return;
	}
	crc = UPDCRC32(c, crc);
	rxd_header[n] = c;
    }

    crc = ~crc;

    rxd_crc = rx(1000);
    rxd_crc |= rx(1000) << 8;
    rxd_crc |= rx(1000) << 16;
    rxd_crc |= rx(1000) << 24;

#ifndef XXX_SKIP_CRC    
    if (rxd_crc != crc) {
#ifdef DEBUG
       fprintf(logfp, "rx_bin32_header: bad crc - orininal crc %8.8x calculated crc %8.8x\n", rxd_crc, crc);
#endif
       return;
    }
#endif /* XXX_SKIP_CRC */    
    rxd_header_len = 5;
}

/*
 * receive any style header
 * if the errors flag is set than whenever an invalid header packet is
 * received INVHDR will be returned. otherwise we wait for a good header
 * also; a flag (receive_32_bit_data) will be set to indicate whether data
 * packets following this header will have 16 or 32 bit data attached.
 * variable headers are not implemented.
 */

int rx_header_raw(int to, int errors)
{
    int c;

#ifdef DEBUG
    fprintf(logfp, "rx_header_raw...\n");
#endif

    rxd_header_len = 0;

    do {
	do {
	    c = rx_raw(to);
	    if (c == TIMEOUT) {
		return c;
	    }
	} while (c != ZPAD);

	c = rx_raw(to);
	if (c == TIMEOUT) {
	    return c;
	}
	if (c == ZPAD) {
	    c = rx_raw(to);
	    if (c == TIMEOUT) {
		return c;
	    }
	}
	/*
	 * spurious ZPAD check
	 */

	if (c != ZDLE) {

#ifdef DEBUG
	    fprintf(logfp, "rx_read_raw: expected ZDLE; but got %c\n", c);
#endif

	    continue;
	}
	/*
	 * now read the header style
	 */

	c = rx(to);

	if (c == TIMEOUT) {
	    return c;
	}

#ifdef DEBUG
	fprintf(logfp, "got header type...to verify it...\n");
#endif

	switch (c) {
	case ZBIN:
#ifdef DEBUG
	   fprintf(logfp, "rx_header_raw: to call rx_bin16_header...\n");
#endif
	    rx_bin16_header(to);
	    receive_32_bit_data = FALSE;
	    break;
	case ZHEX:
	    rx_hex_header(to);

#ifdef DEBUG
	    fprintf(logfp, "to call rx_hex_header...\n");
#endif

	    receive_32_bit_data = FALSE;
	    break;
	case ZBIN32:
	    rx_bin32_header(to);

#ifdef DEBUG
	    fprintf(logfp, "rx_header_raw: to call rx_bin32_header...\n");
#endif

	    receive_32_bit_data = TRUE;
	    break;
	default:
	    /*
	     * unrecognized header style
	     */

#ifdef DEBUG
	    fprintf(logfp, "rx_header_raw: unrecognized header style %c\n", c);
#endif

	    if (errors) {
		return INVHDR;
	    }
	    continue;
	    break;
	}
	if (errors && rxd_header_len == 0) {
	    return INVHDR;
	}
    } while (rxd_header_len == 0);

    /*
 * this appears to have been a valid header.
     * return its type.
     */

    if (rxd_header[0] == ZDATA) {

#ifdef DEBUG
	fprintf(logfp, "rx_header_raw: got ZDATA header\n");
#endif

	ack_file_pos = rxd_header[ZP0] | (rxd_header[ZP1] << 8) |
	    (rxd_header[ZP2] << 16) | (rxd_header[ZP3] << 24);
    }
    if (rxd_header[0] == ZFILE) {

#ifdef DEBUG
	fprintf(logfp, "rx_header_raw: got ZFILE header\n");
#endif

	ack_file_pos = 0l;
    }

#ifdef DEBUG
    fprintf(logfp, "rx_header_raw: got header type %d\n", rxd_header[0]);
#endif

    return rxd_header[0];
}

int rx_header(int timeout)
{
    return rx_header_raw(timeout, FALSE);
}

int rx_header_and_check(int timeout)
{
    int type;
    while (TRUE) {
	type = rx_header_raw(timeout, TRUE);

	if (type != INVHDR) {
	    break;
	}
	tx_znak();
    }

    return type;
}


