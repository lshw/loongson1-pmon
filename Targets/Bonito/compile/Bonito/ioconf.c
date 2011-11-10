/*
 * MACHINE GENERATED: DO NOT EDIT
 *
 * ioconf.c, from "Bonito"
 */

#include "mainbus.h"
#if NMAINBUS > 0
#include <sys/param.h>
#include <sys/device.h>

extern struct cfdriver mainbus_cd;
extern struct cfdriver pcibr_cd;
extern struct cfdriver fd_cd;
extern struct cfdriver localbus_cd;
extern struct cfdriver inphy_cd;
extern struct cfdriver pci_cd;
extern struct cfdriver fxp_cd;
extern struct cfdriver rtl_cd;
extern struct cfdriver em_cd;
extern struct cfdriver pciide_cd;
extern struct cfdriver wd_cd;
extern struct cfdriver ide_cd_cd;

extern struct cfattach mainbus_ca;
extern struct cfattach pcibr_ca;
extern struct cfattach fd_ca;
extern struct cfattach localbus_ca;
extern struct cfattach inphy_ca;
extern struct cfattach pci_ca;
extern struct cfattach fxp_ca;
extern struct cfattach rtl_ca;
extern struct cfattach em_ca;
extern struct cfattach pciide_ca;
extern struct cfattach wd_ca;
extern struct cfattach ide_cd_ca;


/* locators */
static int loc[2] = {
	-1, -1,
};

#ifndef MAXEXTRALOC
#define MAXEXTRALOC 32
#endif
int extraloc[MAXEXTRALOC];
int nextraloc = MAXEXTRALOC;
int uextraloc = 0;

char *locnames[] = {
	"phy",
	"bus",
	"dev",
	"function",
	"channel",
	"drive",
};

/* each entry is an index into locnames[]; -1 terminates */
short locnamp[] = {
	-1, 0, -1, 0, -1, 1, -1, 1,
	-1, 2, 3, -1, 4, 5, -1,
};

/* size of parent vectors */
int pv_size = 12;

/* parent vectors */
short pv[12] = {
	9, 7, -1, 1, 2, -1, 6, -1, 10, -1, 0, -1,
};

#define NORM FSTATE_NOTFOUND
#define STAR FSTATE_STAR
#define DNRM FSTATE_DNOTFOUND
#define DSTR FSTATE_DSTAR

struct cfdata cfdata[] = {
    /* attachment       driver        unit  state loc     flags parents nm ivstubs starunit1 */
/*  0: mainbus0 at root */
    {&mainbus_ca,	&mainbus_cd,	 0, NORM,     loc,    0, pv+ 2, 0, 0,    0},
/*  1: pcibr0 at mainbus0 */
    {&pcibr_ca,		&pcibr_cd,	 0, NORM,     loc,    0, pv+10, 0, 0,    0},
/*  2: pcibr1 at mainbus0 */
    {&pcibr_ca,		&pcibr_cd,	 1, NORM,     loc,    0, pv+10, 0, 0,    1},
/*  3: fd0 at mainbus0 */
    {&fd_ca,		&fd_cd,		 0, NORM,     loc,    0, pv+10, 0, 0,    0},
/*  4: localbus0 at mainbus0 */
    {&localbus_ca,	&localbus_cd,	 0, NORM,     loc,    0, pv+10, 0, 0,    0},
/*  5: inphy* at em*|fxp0 phy -1 */
    {&inphy_ca,		&inphy_cd,	 0, STAR, loc+  1,    0, pv+ 0, 1, 0,    0},
/*  6: pci* at pcibr0|pcibr1 bus -1 */
    {&pci_ca,		&pci_cd,	 0, STAR, loc+  1,    0, pv+ 3, 5, 0,    0},
/*  7: fxp0 at pci* dev -1 function -1 */
    {&fxp_ca,		&fxp_cd,	 0, NORM, loc+  0,    0, pv+ 6, 9, 0,    0},
/*  8: rtl* at pci* dev -1 function -1 */
    {&rtl_ca,		&rtl_cd,	 0, STAR, loc+  0,    0, pv+ 6, 9, 0,    0},
/*  9: em* at pci* dev -1 function -1 */
    {&em_ca,		&em_cd,		 0, STAR, loc+  0,    0, pv+ 6, 9, 0,    0},
/* 10: pciide* at pci* dev -1 function -1 */
    {&pciide_ca,	&pciide_cd,	 0, STAR, loc+  0,    0, pv+ 6, 9, 0,    0},
/* 11: wd* at pciide* channel -1 drive -1 */
    {&wd_ca,		&wd_cd,		 0, STAR, loc+  0,    0, pv+ 8, 12, 0,    0},
/* 12: ide_cd* at pciide* channel -1 drive -1 */
    {&ide_cd_ca,	&ide_cd_cd,	 0, STAR, loc+  0,  0x1, pv+ 8, 12, 0,    0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {(struct cfattach *)-1}
};

short cfroots[] = {
	 0 /* mainbus0 */,
	-1
};

int cfroots_size = 2;

/* pseudo-devices */
extern void loopattach (int);

char *pdevnames[] = {
	"loop",
};

int pdevnames_size = 1;

struct pdevinit pdevinit[] = {
	{ loopattach, 1 },
	{ 0, 0 }
};
#endif /* NMAINBUS */
