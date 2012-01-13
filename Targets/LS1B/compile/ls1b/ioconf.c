/*
 * MACHINE GENERATED: DO NOT EDIT
 *
 * ioconf.c, from "ls1b"
 */

#include "mainbus.h"
#if NMAINBUS > 0
#include <sys/param.h>
#include <sys/device.h>

extern struct cfdriver mainbus_cd;
extern struct cfdriver pcibr_cd;
extern struct cfdriver loopdev_cd;
extern struct cfdriver localbus_cd;
extern struct cfdriver syn_cd;
extern struct cfdriver sdcard_cd;
extern struct cfdriver pci_cd;

extern struct cfattach mainbus_ca;
extern struct cfattach pcibr_ca;
extern struct cfattach loopdev_ca;
extern struct cfattach localbus_ca;
extern struct cfattach syn_ca;
extern struct cfattach sdcard_ca;
extern struct cfattach pci_ca;


/* locators */
static int loc[1] = {
	-1,
};

#ifndef MAXEXTRALOC
#define MAXEXTRALOC 32
#endif
int extraloc[MAXEXTRALOC];
int nextraloc = MAXEXTRALOC;
int uextraloc = 0;

char *locnames[] = {
	"base",
	"bus",
};

/* each entry is an index into locnames[]; -1 terminates */
short locnamp[] = {
	-1, 0, -1, 1, -1,
};

/* size of parent vectors */
int pv_size = 6;

/* parent vectors */
short pv[6] = {
	3, -1, 1, -1, 0, -1,
};

#define NORM FSTATE_NOTFOUND
#define STAR FSTATE_STAR
#define DNRM FSTATE_DNOTFOUND
#define DSTR FSTATE_DSTAR

struct cfdata cfdata[] = {
    /* attachment       driver        unit  state loc     flags parents nm ivstubs starunit1 */
/*  0: mainbus0 at root */
    {&mainbus_ca,	&mainbus_cd,	 0, NORM,     loc,    0, pv+ 1, 0, 0,    0},
/*  1: pcibr* at mainbus0 */
    {&pcibr_ca,		&pcibr_cd,	 0, STAR,     loc,    0, pv+ 4, 0, 0,    0},
/*  2: loopdev0 at mainbus0 */
    {&loopdev_ca,	&loopdev_cd,	 0, NORM,     loc,    0, pv+ 4, 0, 0,    0},
/*  3: localbus0 at mainbus0 */
    {&localbus_ca,	&localbus_cd,	 0, NORM,     loc,    0, pv+ 4, 0, 0,    0},
/*  4: syn0 at localbus0 base -1 */
    {&syn_ca,		&syn_cd,	 0, NORM, loc+  0,    0, pv+ 0, 1, 0,    0},
/*  5: syn1 at localbus0 base -1 */
    {&syn_ca,		&syn_cd,	 1, NORM, loc+  0,    0, pv+ 0, 1, 0,    1},
/*  6: sdcard0 at localbus0 base -1 */
    {&sdcard_ca,	&sdcard_cd,	 0, NORM, loc+  0,    0, pv+ 0, 1, 0,    0},
/*  7: pci* at pcibr* bus -1 */
    {&pci_ca,		&pci_cd,	 0, STAR, loc+  0,    0, pv+ 2, 3, 0,    0},
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
