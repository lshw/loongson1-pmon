#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <unistd.h>
#include <sys/unistd.h>
#include <pmon.h>
#include <stdio.h>
#include <file.h>
#include "string.h"
#include <sys/fcntl.h>
#include "autoconf.h"
#include "gzip.h"
#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */
#include <pmon/dev/loopdev.h>

static struct device *get_device(dev_t dev);

int loopdevmatch( struct device *parent, void *match, void *aux);
void loopdevattach(struct device *parent, struct device *self, void *aux);

struct cfattach loopdev_ca = {
        sizeof(struct loopdev_softc), loopdevmatch, loopdevattach,
};

struct cfdriver loopdev_cd = {
    NULL, "loopdev", DV_DISK
};

void loopdevstrategy(struct buf *bp)
{
	struct loopdev_softc *priv;
	unsigned int blkno, blkcnt;
	int ret ;
			
	priv=get_device(bp->b_dev);

	blkno = bp->b_blkno;

	blkno = blkno /(priv->bs/DEV_BSIZE);
	blkcnt = howmany(bp->b_bcount, priv->bs); 


	/* Valid request?  */
	if (bp->b_blkno < 0 ||
	    (bp->b_bcount % priv->bs) != 0 ||
	    (bp->b_bcount / priv->bs) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		printf("Invalid request \n");
		goto bad;
	}
	
	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;
	
	if(bp->b_flags & B_READ){
#if NGZIP > 0
	if(priv->unzip){
	 gz_lseek(priv->fd,blkno*priv->bs,SEEK_SET);
	 ret=gz_read(priv->fd,(unsigned long *)bp->b_data,bp->b_bcount);
	 }
	else 
#endif
	{
	lseek(priv->fd,blkno*priv->bs,SEEK_SET);
	ret = read(priv->fd,(unsigned long *)bp->b_data,bp->b_bcount);
	}
	if(ret != bp->b_bcount)
		bp->b_flags |= B_ERROR;	
	dotik(30000, 0);
	}
	else
	{
	lseek(priv->fd,blkno*priv->bs,SEEK_SET);
	ret = write(priv->fd,(unsigned long *)bp->b_data,bp->b_bcount);
	if(ret != bp->b_bcount)
		bp->b_flags |= B_ERROR;	
	dotik(30000, 0);
	}
done:
	biodone(bp);
	return;
bad:
	bp->b_flags |= B_ERROR;	
	biodone(bp);
}




static int losetup(int argc,char **argv)
{
int i;
struct loopdev_softc *priv;
int dev;
if(argc<3)return -1;
dev=find_device(&argv[1]);
priv=get_device(dev);
if(!priv)return -1;
strncpy(priv->dev,argv[2],63);
priv->bs=DEV_BSIZE;
priv->seek=0;
priv->count=-1;
for(i=3;i<argc;i++)
{
if(!strncmp(argv[i],"bs=",3))
 priv->bs=strtoul(&argv[i][3],0,0);
else if(!strncmp(argv[i],"count=",6))
 priv->count=strtoul(&argv[i][6],0,0);
else if(!strncmp(argv[i],"seek=",5))
 priv->seek=strtoul(&argv[i][5],0,0);
else if(!strncmp(argv[i],"access=",7))
 priv->access=strtoul(&argv[i][7],0,0);
#if NGZIP > 0
else if(!strcmp(argv[i],"unzip=1"))
 priv->unzip=1;
#endif
}
return 0;
}

int
loopdevopen(
    dev_t dev,
    int flag, int fmt,
    struct proc *p)
{
char loopdevcmd[0x200];
char *loopdevenv;
struct loopdev_softc *priv=get_device(dev);
if(!priv)return -1;

if((loopdevenv=getenv(priv->sc_dev.dv_xname)))
{
sprintf(loopdevcmd,"losetup %s %s",priv->sc_dev.dv_xname,loopdevenv);
do_cmd(loopdevcmd);
}

priv->fd=open(priv->dev,priv->access);
if(priv->fd==-1)return -1;
#if NGZIP > 0
if(priv->unzip&&(gz_open(priv->fd)==-1))priv->unzip=0;
#endif
lseek(priv->fd,priv->seek*priv->bs,SEEK_SET);
return 0;
}


int
loopdevread(
    dev_t dev,
    struct uio *uio,
    int flags)
{
	return physio(loopdevstrategy, NULL, dev, B_READ, minphys, uio);
}

int
loopdevwrite(
    dev_t dev,
    struct uio *uio,
    int flags)
{
    return (physio(loopdevstrategy, NULL, dev, B_WRITE, minphys, uio));
}


int
loopdevclose( dev_t dev,
	int flag, int fmt,
	struct proc *p)
{
struct loopdev_softc *priv=get_device(dev);
#if NGZIP > 0
	if(priv->unzip)gz_close(priv->fd);
#endif
	close(priv->fd);
	return 0;
}

int
loopdevmatch(parent, match, aux)
    struct device *parent;
    void *match, *aux;
{
    struct confargs *ca = aux;
    if (!strncmp(ca->ca_name, loopdev_cd.cd_name,7))
    return 1;
    else
    return 0;
}


void
loopdevattach(parent, self, aux)
    struct device *parent, *self;
    void *aux;
{
struct loopdev_softc *priv = (void *)self;
strncpy(priv->dev,"/dev/mtd0",63);
priv->bs=DEV_BSIZE;
priv->seek=0;
priv->count=-1;
priv->access=O_RDWR;
#if NGZIP > 0
priv->unzip=0;
#endif
}


static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"losetup",	"loopdev0 devfile  [bs=0x20000] [count=-1] [seek=0]", 0, "losetup",losetup, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


