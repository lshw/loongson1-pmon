/*	$Id: pcicmds.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by
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
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>

#include <pmon.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

/*
 * Prototypes
 */
static int pci_query_dev (pcireg_t tag);
static int pci_query_bar (pcireg_t tag, pcireg_t index);

int cmd_pcicfg __P((int, char *[]));
int cmd_pciscan __P((int, char *[]));

/*
 *  PCI config space read/write
 */

int
cmd_pcicfg(ac, av)
	int ac;
	char *av[];
{
	int32_t	bus, dev, fun, offs;
	register_t v;
	char *p;
	int c;
	int dopt;
	pcitag_t tag;
extern int optind;

	optind = 0;
	dopt = 0;
	while ((c = getopt (ac, av, "d")) != EOF) {
		switch (c) {
		case 'd':
			dopt = 1;
			break;

		default:
			return (-1);
		}
	}

	if(optind >= ac) {
		return (-1);
	}

	if(!get_rsa (&bus, av[optind++])) {
		return (-1);
	}

	fun = 0;
	if((p = strchr(av[optind], ':')) != NULL) {
		*p++ = '\0';
		if(!get_rsa (&fun, p))
			return (-1);
	}
	if(!get_rsa (&dev, av[optind++])) {
		return (-1);
	}

	if(!dopt && !get_rsa (&offs, av[optind++])) {
		return (-1);
	}

	tag = _pci_make_tag(bus, dev, fun);

	if(offs & 3) {
		printf ("%08x: non 32 bit aligned register\n", offs);
	return (1);
			}
	if (optind < ac) {
	/* command mode */
		if (!get_rsa_reg (&v, av[optind])) {
			return (-1);
		}
		_pci_conf_write(tag, offs, v);
	}
	else if(dopt) {
		printf("Offset  Values.....");
		for(offs = 0; offs < 64; offs += 4) {
			if((offs & 0xf) == 0) {
				printf ("\n %04x  ", offs);
			}
			v = _pci_conf_read(tag, offs);
			printf("%08x ", v);
		}
		printf("\n");
	}
	else {
	/* interactive mode */
		while(1) {
			v = _pci_conf_read(tag, offs);
			printf ("%04x %08x ", offs, v);

			line[0] = '\0'; get_line (line, 0);
			for (p = line; *p == ' '; p++);
			if (*p == '.') {
				break;
			}
			else if(*p == '-' || *p == '^') {
				offs -= 8;
			}
			else if(*p == '=') {
				offs -= 4;
			}
			else if(*p != '\0') {
				if(get_rsa_reg(&v, p)) {
					_pci_conf_write(tag, offs, v);
				}
				else {
					return(1);
				}
			}
			offs += 4;
		}
	}
	return (0);
}

static const Optdesc cmd_pciscan_opts[] =
{
    {"-b <bus>", "bus no"},
    {"-d <dev>", "dev no"},
    {0}};


/*
 *  PCI config space read/write
 */

int
cmd_pciscan(ac, av)
        int ac;
        char *av[];
{
        int c, bus, dev, func;
        pcireg_t tag, bhlc;
        int ndevs, maxbus, maxdev, firstdev;
extern int optind;
extern char *optarg;

	optind = 0;
	bus = 0;
	maxbus = 255;
	firstdev = 0;
	maxdev = 31;
	while ((c = getopt (ac, av, "b:d:")) != EOF) {
	switch (c) {
		case 'b':
			if(!get_rsa(&bus, optarg)) {
				return(-1);
			}
			if(bus < 0 || bus > 255) {
				return(-1);
			}
			maxbus = bus;
			break;

		case 'd':
			if(!get_rsa(&firstdev, optarg)) {
				return(-1);
			}
			if(firstdev < 0 || firstdev > 31) {
				return(-1);
			}
			maxdev = firstdev;
			break;

		default:
			return (-1);
		}
	}

        for (; bus <= maxbus; bus++) {
                ndevs = 0;
                for(dev = firstdev; dev <= maxdev; dev++) {
                        tag = _pci_make_tag(bus, dev, 0);
                        bhlc = _pci_conf_read(tag, PCI_ID_REG);
                        if(bhlc == 0 || bhlc == 0xffffffff) {
                                continue;
                        }
			if (ndevs == 0) {
				printf(">> BUS %2d <<\n", bus);
				printf("Dev Fun Device description\n");
				printf("--------------------------\n");
			}
                        bhlc = _pci_conf_read(tag, PCI_BHLC_REG);
                        if (PCI_HDRTYPE_MULTIFN(bhlc)) {
                                for(func = 0; func < 8; func++) {
                                        tag = _pci_make_tag(bus, dev, func);
                                        bhlc = _pci_conf_read(tag, PCI_ID_REG);
                                        if(bhlc == 0 || bhlc == 0xffffffff) {
                                                continue;
                                        }
                                        if(pci_query_dev(tag) < 0) {
                                                break;
                                        }
                                        ndevs++;
                                }
                        }
                        else {
                                if(pci_query_dev(tag) >= 0) {
                                        ndevs++;
                                }
                        }
                }
        }
        return(0);
}

static int
pci_query_bar(tag, index)
	pcireg_t tag;
	pcireg_t index;
{
	pcireg_t old_bar, bar, skipnext = 0;
	old_bar = _pci_conf_read(tag, index);
	_pci_conf_write(tag, index, 0xffffffff);
	bar = _pci_conf_read(tag, index);
	if( old_bar & 0x1 ){
		printf("0x%08x:0x%08x i/o @0x%08x, %d bytes\n", old_bar, bar, old_bar & 0xfffffffc, (~(bar&0xfffffffc))+1);
	}else if(old_bar & 0x4){
		printf("64-bit mem\n");
		skipnext = 1;
	}else {
		printf("0x%08x:0x%08x mem @0x%08x, %d bytes\n", old_bar, bar, old_bar & 0xfffffff0, (~(bar&0xfffffff0))+1);

	}

	_pci_conf_write(tag, index, old_bar);
	return skipnext;
}

static int
pci_query_dev(tag)
        pcireg_t tag;
{
        pcireg_t class, id;
        int dev, func, i;
        char devinfo[256];

        class = _pci_conf_read(tag, PCI_CLASS_REG);
        id = _pci_conf_read(tag, PCI_ID_REG);
        if(class < 0) {
                return(-1);
        }
        _pci_break_tag (tag, NULL, &dev, &func);
        _pci_devinfo(id, class, NULL, devinfo);
        printf("%3d %3d %s\n", dev, func, devinfo);
	
	for(i = 0x10; i <= 0x24; i +=4) {
		if(pci_query_bar(tag, i)) i += 4;
	}

        return(0);
} 

#ifdef ENABLE_FXPETH
/*
 *  Command handler for maintaining the eeproms on 8255x chips.
 */

#define FXP_PCI_IOBA 0x14		/* Base register used for mapping */

/*
 * Control/status registers.
 */
#define	FXP_CSR_EEPROMCONTROL	14	/* eeprom control (2 bytes) */

/*
 * Serial EEPROM control register bits
 */
#define FXP_EEPROM_EESK		0x01	/* shift clock */
#define FXP_EEPROM_EECS		0x02	/* chip select */
#define FXP_EEPROM_EEDI		0x04	/* data in */
#define FXP_EEPROM_EEDO		0x08	/* data out */

/*
 * Serial EEPROM opcodes, including start bit
 */
#define FXP_EEPROM_OPC_ERASE	0x4
#define FXP_EEPROM_OPC_WRITE	0x5
#define FXP_EEPROM_OPC_READ	0x6
#define FXP_EEPROM_OPC_WRITENB	((FXP_EEPROM_OPC_ERASE << 2) | 3)
#define FXP_EEPROM_OPC_WRITDIS	((FXP_EEPROM_OPC_ERASE << 2) | 0)

#define	CSR_R_2(st, sh, reg)		bus_space_read_2(st, sh, (reg))
#define	CSR_W_2(st, sh, reg, val)	bus_space_write_2(st, sh, (reg), (val))

extern struct tgt_bus_space bus_iot;

static void read_eeprom __P((int, u_int16_t *, int, int));
static void write_eeprom __P((int, u_int16_t *, int, int));
static int probe_dev __P((int, int));

static u_int16_t ee_init_proto[] = {
0x0000, 0x0000, 0x0000, 0x0203, 0x0000, 0x0201, 0x4d01,
0x0000, 0x7213, 0x8306, 0x48a2, 0x000c, 0x8086, 0x0000
};
static const Optdesc fxp_opts[] =
{
    {"-b <bus>", "bus no"},
    {"-d <dev>", "dev no"},
    {0}};

int fxp_eth __P((int, char *[]));

int
fxp_eth (int ac, char *av[])
{
	u_int32_t busno = 0;
	u_int32_t devno = 256;	/* 256 means not specified */
	u_int32_t rv;
	int i, csrbase;
	u_int8_t ee_init_data[32];
	char *tp, *p;
	int c;
extern int optind;
extern char *optarg;

	optind = 0;
	while ((c = getopt (ac, av, "b:d:")) != EOF) {
		switch (c) {
		case 'b':
			if(!get_rsa(&busno, optarg)) {
				return(-1);
			}
			if(busno < 0 || busno > 255) {
				return(-1);
			}
			break;

		case 'd':
			if(!get_rsa(&devno, optarg)) {
				return(-1);
			}
			if(devno < 0 || devno > 30) {
				return(-1);
			}
			break;

		default:
			return (-1);
		}
	}

	if (optind >= ac) { /* No ethernet address given, display */
		do {
			csrbase = probe_dev(busno, devno & 255);
			if(csrbase != 0) {
				if(devno > 255) {
					if(busno != 0) {
						printf("bus %d ", busno);
					}
					printf("device %2d ", devno & 255);
				}
				read_eeprom(csrbase, (u_int16_t *)&ee_init_data, 0, 14);
				printf("ethernet address = %02x", ee_init_data[0]);
				for(i = 1; i < 6; i++) {
					printf(":%02x", ee_init_data[i]);
				}
				printf("\n");
			}
			else if(devno < 256) {
				printf("No fxp on bus %d slot %d\n", busno, devno);
			}
			if(++devno > 256+30) {
				devno = 0;
			}
		} while(devno > 256);
		return(0);
	}

	/*
	 * Arguments given, validate that the given dev is a fxp.
	 * Then try to decode an ethernet address from input data.
	 */
	if(devno < 0 || devno > 31) {
		return(-1);
	}
	csrbase = probe_dev(busno, devno);
	if(csrbase == 0) {
		printf("No fxp on bus %d slot %d\n", busno, devno);
		return(0);
	}

	bcopy((const char *)ee_init_proto, ee_init_data, sizeof(ee_init_proto));
	p = av[optind];
	for (i = 0 ; p; i++) {
		tp = p;
		while(*p && *p != ':') {
			p++;
		}
		if(*p) {
			*p++ = 0;
		}
		else {
			p = 0;
		}
			
		if(!get_rsa(&rv, tp)) {
			return(-1);
		}
		ee_init_data[i] = rv;
	}
	if(i != 6) {
		printf("badly formed ethernet address!\n");
		return(0);
	}
	write_eeprom(csrbase, (u_int16_t *)&ee_init_data, 0, 14);
	return(0);
}

static int
probe_dev(busno, devno)
	int busno;
	int devno;
{
	pcitag_t tag;
	pcireg_t preg;
	bus_addr_t iobase;
	bus_size_t iosize;
	bus_addr_t vaddr;

	tag = _pci_make_tag(busno, devno, 0);

	preg = _pci_conf_read(tag, PCI_ID_REG);

	switch (PCI_PRODUCT(preg)) {
	case PCI_PRODUCT_INTEL_82557:
	case PCI_PRODUCT_INTEL_82559:
	case PCI_PRODUCT_INTEL_82559ER:
		if (pci_io_find(NULL, tag, FXP_PCI_IOBA, &iobase, &iosize)) { 
			printf(": can't find i/o space\n");
			return(-1);
		}
		if (bus_space_map(&bus_iot, iobase, iosize, 0, &vaddr)) {
			printf(": can't map i/o space\n");
			return(-1);
		}
		return((int)vaddr);
	}

	return (0);
}

/*
 * Read from the serial EEPROM. Basically, you manually shift in
 * the read opcode (one bit at a time) and then shift in the address,
 * and then you shift out the data (all of this one bit at a time).
 * The word size is 16 bits, so you have to provide the address for
 * every 16 bits of data.
 */
static void
read_eeprom(csrbase, data, offset, words)
	int csrbase;
	u_int16_t *data;
	int offset;
	int words;
{
	u_int16_t reg;
	int i, x;

	for (i = 0; i < words; i++) {
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		/*
		 * Shift in read opcode.
		 */
		for (x = 3; x > 0; x--) {
			if (FXP_EEPROM_OPC_READ & (1 << (x - 1))) {
				reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
			} else {
				reg = FXP_EEPROM_EECS;
			}
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			delay(1);
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
			delay(1);
		}
		/*
		 * Shift in address.
		 */
		for (x = 6; x > 0; x--) {
			if ((i + offset) & (1 << (x - 1))) {
				reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
			} else {
				reg = FXP_EEPROM_EECS;
			}
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			delay(1);
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
			delay(1);
		}
		reg = FXP_EEPROM_EECS;
		data[i] = 0;
		/*
		 * Shift out data.
		 */
		for (x = 16; x > 0; x--) {
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			delay(1);
			if (CSR_R_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL) &
			    FXP_EEPROM_EEDO)
				data[i] |= (1 << (x - 1));
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
			delay(1);
		}
		data[i] = htole16(data[i]);
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, 0);
		delay(1);
	}
}

/*
 * Write to the serial EEPROM.
 */
void
write_eeprom(csrbase, data, offset, words)
	int csrbase;
	u_int16_t *data;
	int offset;
	int words;
{
	u_int16_t reg;
	int i, x;
	int d;

	/*
	 * Enable writing.
	 */
	d = (FXP_EEPROM_OPC_WRITENB << 4);
	x = 0x100;
	while (x != 0) {
		if (d & x) {
			reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
		} else {
			reg = FXP_EEPROM_EECS;
		}
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL,
		    reg | FXP_EEPROM_EESK);
		delay(1);
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
		delay(1);
		x >>= 1;
	}
/* XXXX Need data sheet to verify if raise CS is enough! (pefo) */
	CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, 0);
	delay(1);
	CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EESK);
	delay(1);
	CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, 0);
	delay(1);

	/*
	 * Now write the data words.
	 */
	for (i = 0; i < words; i++) {
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		/*
		 * Shift in write command, address and data.
		 */
		d = (FXP_EEPROM_OPC_WRITE << 6) | ((i + offset) & 0x3f);
		d = d << 16 | htole16(data[i]);
		x = 0x1000000;
		while (x != 0) {
			if (d & x) {
				reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
			} else {
				reg = FXP_EEPROM_EECS;
			}
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			delay(1);
			CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
			delay(1);
			x >>= 1;
		}
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, 0);
		delay(1);
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EESK);
		delay(1);
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, 0);
		delay(1);

		delay(200000);  /* Let write settle */
	}

	/*
	 * Disable writing.
	 */
	d = (FXP_EEPROM_OPC_WRITDIS << 4);
	x = 0x100;
	while (x != 0) {
		if (d & x) {
			reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
		} else {
			reg = FXP_EEPROM_EECS;
		}
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL,
		    reg | FXP_EEPROM_EESK);
		delay(1);
		CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, reg);
		delay(1);
		x >>= 1;
	}
	CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, 0);
	delay(1);
	CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EESK);
	delay(1);
	CSR_W_2(&bus_iot, csrbase, FXP_CSR_EEPROMCONTROL, 0);
	delay(1);
}
#endif

static const Cmd Cmds[] =
{
	{"Pci"},
	{"pcicfg",	"bus device[:func] reg",
			0,
			"pci config space",
			cmd_pcicfg, 3, 4, 0},
	{"pcfg",	"",
			0,
			"pcicfg",
			cmd_pcicfg, 3, 4, CMD_ALIAS},
	{"pciscan",	"[-b <bus>][-d <dev>]",
			cmd_pciscan_opts,
			"scan pci bus",
			cmd_pciscan, 1, 5, 0},
#ifdef ENABLE_FXPETH
	{"fxpeth",	"[-b <bus>][-d <dev>][addr]",
			cmd_fxp_opts,
			"fxp address read/set",
			cmd_fxp_eth, 1, 6, CMD_HIDE},
#endif
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

