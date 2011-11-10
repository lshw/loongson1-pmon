/* $Id: progress.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
 /*
 * PMON Written for LSI LOGIC Corp. by Phil Bunce.
 *              Phil Bunce, 100 Dolores St. Ste 242, Carmel CA 93923
 *
 * PMON Ported/rewritten for PowerPC by Per Fogelstrom, Opsycon AB.
 */

/*
 *  ID's for progress indicator during bootup.
 */
#define CHKPNT_SBDR	0x01	/* in sbdreset */
#define CHKPNT_CACH	0x02	/* calling init_cache */
#define CHKPNT_ITLB	0x03	/* calling init_tlb */
#define	CHKPNT_DATA	0x04	/* copying data area */
#define CHKPNT_ZBSS	0x05	/* clearing bss */
#define CHKPNT_HAND	0x06	/* installing exception handlers */
#define CHKPNT_SBDM	0x07	/* calling sbdmachinit */
#define CHKPNT_CREG	0x08	/* setting initial client conditions */
#define CHKPNT_DBGI	0x09	/* calling dbginit */
#define CHKPNT_MAIN	0x0a	/* calling main */
#define CHKPNT_EXIT	0x0b	/* calling exit */
#define CHKPNT_FREQ	0x0c	/* determining CPU frequency */
#define CHKPNT_0MEM	0x0d	/* no memory detected */
#define CHKPNT_WAIT	0x20	/* waiting for debugger connect */
#define CHKPNT_RDBG	0x21	/* entering remote debug mode */
#define CHKPNT_AUTO	0x22	/* waiting for autoboot timeout */
#define CHKPNT_ENVI	0x23	/* calling envinit */
#define CHKPNT_SBDD	0x24	/* calling sbddevinit */
#define CHKPNT_DEVI	0x25	/* calling devinit */
#define CHKPNT_NETI	0x26	/* calling init_net */
#define CHKPNT_HSTI	0x27	/* calling histinit */
#define CHKPNT_SYMI	0x28	/* calling syminit */
#define CHKPNT_DEMO	0x29	/* calling demoinit */
#define CHKPNT_MACH	0x2a	/* calling getmachtype */
#define CHKPNT_SBDE	0x2b	/* calling sbdenable */
#define CHKPNT_LOGO	0x2c	/* printing PMON signon */
#define CHKPNT_SHRC	0x2d	/* calling shrc */
#define CHKPNT_PMON	0x2e	/* set state */
#define CHKPNT_PRAM	0x2f	/* set state */
#define CHKPNT_RUN	0x30	/* set state */
#define CHKPNT_MAPV	0x31	/* calling sbd_mapenv */
#define CHKPNT_STDV	0x32	/* setting env defaults */

#define CHKPNT_PCIH	0x40	/* calling pci_hwinit */
#define CHKPNT_PCIS	0x41	/* scanning PCI bus */
#define CHKPNT_PCIR	0x42	/* calling pci_hwreinit */
#define CHKPNT_PCIW	0x43	/* configuring PCI windows */
#define CHKPNT_PCID	0x44	/* calling pci_setup_devices */
 
