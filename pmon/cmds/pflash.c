/*	$Id: pflash.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <machine/pio.h>

#include <pmon.h>

#include <pflash.h>
#include <dev/pflash_tgt.h>

extern int optind;
extern char *optarg;

/*
 *  Flash programming functionality.
 */

int cmd_flash __P((int, char *[]));

int
cmd_flash(ac, av)
	int ac;
	char *av[];
{
	int	page;
	int	c;
	int	opt_erase = 0;
	int	opt_verify = 0;
	u_int32_t base_addr;
	u_int32_t offset = -1;
	u_int32_t flashsize = -1;

	if(tgt_flashwrite_enable() == 0) {
		printf("FLASH can't be write enabled by PMON2000. Please\n");
		printf("consult the HW manual on how to enable writing!\n");
		return 0 ;
	}
	tgt_flashwrite_disable();

	if(ac == 1) {	/* No arguments given, display flash info */
		fl_query_info();
		return 0;
	}

	optind = 0;
	page = -1;
	while((c = getopt(ac, av, "eqvp:")) != EOF) {
		switch(c) {
		case 'e':		/* Erase */
			opt_erase = 1;
			break;

		case 'v':		/* Verify */
			opt_verify = 1;
			break;

		case 'q':		/* query info */
			fl_query_info();
			return 0;	/* Don't bother with the rest */

#ifdef PFLASH_PAGED_FLASH
		case 'p':
			if (!get_rsa ((u_int32_t *)&page, optarg) || 
			    tgt_flashsetpageno(page) != 0) {
				printf("Invalid page number\n");
				return 0;
			}
			if (optind == ac)
				return 0;
			break;
#endif
		default:
			return -1;	/* catch -p when not enabled */
		}
	}

#ifdef PFLASH_PAGED_FLASH
	/* if -p <page> not given, select -1. have meaning on some targets */
	if (page < 0) {
		tgt_flashsetpageno(page);
	}
#endif

	if(optind >= ac) {
		return -1;	/* Argument error */
	}

	if(!get_rsa(&base_addr, av[optind++])) {
		return -1;
	}
	if(optind < ac && !get_rsa (&flashsize, av[optind++])) {
		return -1;
	}
	if(optind < ac && !get_rsa (&offset, av[optind++])) {
		return -1;
	}

	if(opt_erase) {
		c = fl_erase_device((void *)base_addr, flashsize, TRUE);

		switch(c) {
		case -4:
			printf("Bad address and/or size for erase!\n");
			return 0;

		case -3:
			printf("No FLASH memory on that address!\n");
			return 0;

		case -1:
			printf("Erase status failed!\n");
			return 0;
		}
	}
	if(offset != -1) {
		if(opt_verify) {
			fl_verify_device((void *)base_addr, (void *)offset,
						flashsize, TRUE);
			return 0;
		}

		c = fl_program_device((void *)base_addr, (void *)offset,
						flashsize, TRUE);
		switch(c) {
		case -4:
			printf("Bad address and/or size!\n");
			break;

		case -3:
			printf("No FLASH memory on that address!\n");
			break;

		case -1:
			printf("Programming failed!\n");
			break;

		case 0:
			fl_verify_device((void *)base_addr, (void *)offset,
						flashsize, TRUE);
			break;
		}
	}

	return 0;
}

/*
 *  Command table registration
 *  ==========================
 */

const Optdesc cmd_flash_opts[] = {
	{ "-q",	"display info about flash memory" },
	{ "-e",	"erase flash" },
	{ "-v",	"verify flash" },
#ifdef PFLASH_PAGED_FLASH
	{ "-p <page>",	"select page <page>" },
#endif
	{ 0 }
};

static const Cmd Cmds[] =
{
	{"Misc"},
#ifdef PFLASH_PAGED_FLASH
	{"flash",	"[-qev] [-p <page>] [[[addr] size] data]",
#else
	{"flash",	"[-qev] [[[addr] size] data]",
#endif
			cmd_flash_opts,
			"program flash memory",
			cmd_flash, 1, 6,
			0},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

