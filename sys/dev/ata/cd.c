#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/scsiio.h>
#include <sys/vnode.h>

#include <vm/vm.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ata/wdvar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>
#include <sys/ataio.h>
#define	CDRETRIES	4
#define CDBLKSIZE	2048	/* XXX hardwired in */

//#define DEBUG_CD
struct ide_cd_softc {
	struct device sc_dev;
	struct disk sc_dk;

	int sc_flags;
#define	IDE_CDF_LOCKED	0x01
#define	IDE_CDF_WANTED	0x02
#define	IDE_CDF_WLABEL	0x04		/* label is writable */
#define	IDE_CDF_LABELLING	0x08		/* writing label */

#define IDE_CDF_LOADED	  	0x10 /* parameters loaded */
#define IDE_CDF_WAIT		0x20 /* waiting for resources */
#define IDE_CDF_LBA	 		0x40 /* using LBA mode */
	struct ata_atapi_attach *aa_link;	/* contains our targ, lun, etc. */
	struct ata_drive_datas *drvp;
	int openings;	/*max request count */
	struct buf *sc_bp; /*buf being transfered*/ 
	struct buf sc_q;
	char name[16]; /* product name, for default disklabel */
};

struct packet_cmd{
	int use_dma;
	int type;
#define T_READ  	0x01
#define T_WRITE 	0x02
#define T_REQUEST_SENSE  0x03
#define T_PACKET_COMMAND 0x04
	int timeout;
	int error;
	int success;
	int blkno;
	int nblks;
	unsigned char *buffer;
	int  buflen;
	char c[12];
};

#define PACKET_CMD 		0xA0

#define NO_SENSE                0x00
#define RECOVERED_ERROR         0x01
#define NOT_READY               0x02
#define MEDIUM_ERROR            0x03
#define HARDWARE_ERROR          0x04
#define ILLEGAL_REQUEST         0x05
#define UNIT_ATTENTION          0x06
#define DATA_PROTECT            0x07
#define ABORTED_COMMAND         0x0b
#define MISCOMPARE              0x0e

#define GPCMD_READ_10		0x28 
#define GPCMD_WRITE_10		0x2a
#define GPCMD_START_STOP 	0x1b
#define GPCMD_REQUEST_SENSE 	0x03
#define GPCMD_TEST_UNIT_READY 	0x00
#define GPCMD_START_STOP_UNIT	0x1b
#define GPCMD_PREVENT_ALLOW_REMOVAL  0x1e

#define	IDE_CDOUTSTANDING	4


#define CD_ALLOW   0x00	
#define CD_PREVENT 0x01

#define	IDE_CDUNIT(z)			DISKUNIT(z)
#define	IDE_CDMINOR(unit, part)	       	DISKMINOR(unit, part)

#define	IDE_CDPART(z)			RAW_PART

#define	MAKECDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	IDE_CDLABELDEV(dev)	(MAKECDDEV(major(dev), IDE_CDUNIT(dev), RAW_PART))

int	ide_cdmatch __P((struct device *, void *, void *));
void	ide_cdattach __P((struct device *, struct device *, void *));
int	ide_cdopen(dev_t dev, int flag, int fmt, struct proc *p);
int	ide_cdclose(dev_t dev, int flag, int fmt, struct proc *p);
void	ide_cdstrategy(struct buf *bp);
int	ide_cdread(dev_t dev, struct uio *uio, int ioflag);
int	ide_cdwrite(dev_t dev, struct uio *uio, int ioflag);

void make_read_command_packet __P((struct ide_cd_softc *, struct packet_cmd *));
int ide_transfer_command_packet __P((struct ata_drive_datas *, struct packet_cmd*));
void ide_select_drive __P((struct ata_drive_datas *drvp));
int ide_test_unit_ready __P((struct ide_cd_softc *cd));
int ide_cd_prevent __P((struct ide_cd_softc *, int));
int ide_cd_prepare __P((struct ide_cd_softc *, int));
int	ide_cdwrite_intr(struct ide_cd_softc *cd, struct packet_cmd *pc);

void dump_reg(struct ide_cd_softc * cd, char *msg);

void	ide_cdstart __P((void *));
void	ide_cddone __P((struct ide_cd_softc *));

struct cfattach ide_cd_ca = {
	sizeof(struct ide_cd_softc), ide_cdmatch, ide_cdattach
};

struct cfdriver ide_cd_cd = {
	NULL, "cd", DV_DISK
};



#define ide_cdlookup(unit) (struct ide_cd_softc *)device_lookup(&ide_cd_cd, (unit))

int ide_cdmatch(struct device *parent,	void *match_, void *aux)
{

	struct ata_atapi_attach *aa_link = aux;
	struct cfdata *match = match_;

	if (aa_link == NULL)
		return 0;
	if (aa_link->aa_type != T_ATAPI)
		return 0;

	if (match->cf_loc[0] != -1 &&
	    match->cf_loc[0] != aa_link->aa_channel)
		return 0;

	if (match->cf_loc[1] != -1 &&
	    match->cf_loc[1] != aa_link->aa_drv_data->drive)
		return 0;

	return 1;
}

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
#ifdef DEBUG_CD
struct ide_cd_softc *mycd;
#endif
void ide_cdattach(struct device *parent, struct device *self,	void *aux)
{
	
	struct ide_cd_softc *cd = (void *)self;
	struct ata_atapi_attach *aa_link= aux;
	struct ata_drive_datas *drvp = aa_link->aa_drv_data;	
	
	printf("cd attach drive=%d\n", drvp->drive);

	cd->openings = aa_link->aa_openings;
	cd->drvp = aa_link->aa_drv_data; //set by wdcattach and wdcprobe

	printf("dv_xname %s\n", cd->sc_dev.dv_xname);
	strncpy(cd->drvp->drive_name, cd->sc_dev.dv_xname, 
		sizeof(cd->drvp->drive_name) - 1);
	cd->drvp->cf_flags = cd->sc_dev.dv_cfdata->cf_flags;

}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int ide_cdopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct ide_cd_softc *cd;
	int unit, part;
	int error;
	int i;

	unit = IDE_CDUNIT(dev);
	cd = ide_cdlookup(unit);
	if (cd == NULL)
		return ENXIO;

	if (cd->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((cd->sc_flags & IDE_CDF_LOADED) == 0) {
			error = EIO;
			goto bad3;
		}
	} else {
		/* Check that it is still responding and ok. */
		
		if(cd->sc_dk.dk_label==NULL)
			cd->sc_dk.dk_label = (struct disklabel *)
					malloc(sizeof(struct disklabel),M_DEVBUF,M_NOWAIT);
			
		for(i=0; i<10; i++){
			if((error = ide_test_unit_ready(cd)) == 0)
				break;
			else
				ide_cd_prepare(cd, 1); //1: load the disk
		}
		if (error)
			goto bad3;


		cd->sc_flags |= IDE_CDF_LOADED; //CD is loaded

	}

	part = IDE_CDPART(dev);

	/*  Fake some label info that we are going to use. If we
         *  want to be more advanced in the future, eg not ony RAW
	 *  this label must probably be filled in some more...
	 */
	cd->sc_dk.dk_label->d_secsize = 2048; /* XXX Yeah... */

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= cd->sc_dk.dk_label->d_npartitions ||
	     cd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	cd->sc_dk.dk_openmask = cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	return 0;

bad:
	if (cd->sc_dk.dk_openmask == 0) 
		ide_cd_prevent(cd, CD_ALLOW);

bad3:
	cd->sc_flags &= ~ IDE_CDF_LOADED;
	return error;
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int  ide_cdclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct ide_cd_softc *cd;
	int part = IDE_CDPART(dev);

	cd = ide_cdlookup(IDE_CDUNIT(dev));
	if (cd == NULL)
		return ENXIO;

	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	cd->sc_dk.dk_openmask = cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	if (cd->sc_dk.dk_openmask == 0) {
		/* XXXX Must wait for I/O to complete! */

		//ide_cd_prevent(cd, CD_ALLOW);
		//cd->sc_flags &= ~SDEV_OPEN;
		cd->sc_flags &= ~IDE_CDF_LOADED;

	}

	return 0;
}

/*
 * Actually translate the requested transfer into one the physical driver can
 * understand.  The transfer is described by a buf and will include only one
 * physical transfer.
 */
void ide_cdstrategy(struct buf *bp)
{
	struct ide_cd_softc *cd;
	int opri;

	if ((cd = ide_cdlookup(IDE_CDUNIT(bp->b_dev))) == NULL) {
		bp->b_error = ENXIO;
		goto bad;
	}

	/*
	 * The transfer must be a whole number of blocks.
	 */
	if ((bp->b_bcount % cd->sc_dk.dk_label->d_secsize) != 0) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * If the device has been made invalid, error out
	 * maybe the media changed
	 */
	if ((cd->sc_flags & IDE_CDF_LOADED) == 0) {
		bp->b_error = EIO;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediately
	 */
	if (bp->b_bcount == 0)
		goto done;

	opri = splbio();

	/*
	 * Place it in the queue of disk activities for this disk
	 */
	disksort(&cd->sc_q, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	ide_cdstart(cd);
	
	splx(opri);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}


int ide_cdread_intr(struct ide_cd_softc *cd, struct packet_cmd *pc)
{
	struct ata_drive_datas *drvp = cd->drvp;
	struct channel_softc *chp = drvp->chnl_softc;
	int ireason, lowcyl, hicyl, stat;
	int len, i;
	int timeout = 5000;

#ifdef DEBUG_CD	
	if(pc->use_dma)
		printf("Currently Dma not implemented\n");	
#endif
	for(i =0; i<10; i++){
		if(wdcwait(chp, WDCS_DRQ, WDCS_DRQ, timeout)){
			printf("WDCS_DRQ not asserted before read intr\n");	
			pc->timeout = 1;
			return -1;
		}
	
		stat = chp->ch_status; 
		ireason = CHP_READ_REG(chp, wdr_ireason);
		lowcyl = CHP_READ_REG(chp, wdr_cyl_lo);
		hicyl  = CHP_READ_REG(chp, wdr_cyl_hi);

		if(stat & WDCS_ERR){
				printf("Error do read write\n");
				pc->error = 1;
				return -2;
		}
		len = lowcyl + (hicyl << 8);
			
		if((stat & WDCS_DRQ) == 0){
				printf("Read command complete abnormally\n");
				return 1;
		}	
		
		if((ireason & 3) == 2)
			break;
		if((ireason & 3) ==1)
			(void)CHP_READ_REG(chp, wdr_status);
	}

	if(i == 10){
		printf("wrong  intr ireason\n");
		return -1;
	}
	wdc_input_bytes(drvp, pc->buffer, pc->buflen);
	return 0;
}

int 
ide_do_read_write(struct ide_cd_softc *cd, struct packet_cmd *pc)
{	

	struct ata_drive_datas *drvp = cd->drvp;
	struct channel_softc *chp =  drvp->chnl_softc;
	int dma, timeout = 500;
	int xlen = pc->buflen; //unit is byte
		
	dma = pc->use_dma;

#ifdef DEBUG_CD
	if(dma){
		printf("You request dma transfer, Currently it is not implemented\n");
	}	
#endif

	if(wdcwait(chp, WDCS_DRDY, WDCS_DRDY, timeout)){
		printf("Wait time out in ide_do_read_write\n");
		return -1;
	}

	ide_select_drive(drvp);
	delay(10);

	CHP_WRITE_REG(chp, wdr_features, dma);
	CHP_WRITE_REG(chp, wdr_seccnt, 0);
	CHP_WRITE_REG(chp, wdr_sector, 0);
	CHP_WRITE_REG(chp, wdr_cyl_lo, xlen & 0xff); //PIO data
	CHP_WRITE_REG(chp, wdr_cyl_hi, xlen >> 8);
	CHP_WRITE_REG(chp, wdr_command, PACKET_CMD);
	
	ide_transfer_command_packet(drvp, pc);
	/*If command for read*/

	if(pc->type == T_READ)
		return ide_cdread_intr(cd, pc);
	else {
		return ide_cdwrite_intr(cd, pc);	
	}
	
	return 0;
}

void  ide_cdstart(void *v)
{
	struct ide_cd_softc *cd = v;
	struct buf *bp = 0;
	struct buf *dp;
	int blkno, nblks;
	struct packet_cmd pc;
	struct partition *p;

	/*
	 * Check if the device has room for another command
	 */
	while (cd->openings > 0) {

		/*
		 * See if there is a buf with work for us to do..
		 */
		dp = &cd->sc_q;
		if ((bp = dp->b_actf) == NULL)	/* yes, an assign */
			return;
		dp->b_actf = bp->b_actf;

		/*
		 * If the deivce has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if ((cd->sc_flags & IDE_CDF_LOADED) == 0) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			continue;
		}

		/*
		 * We have a buf, now we should make a command
		 *
		 * First, translate the block to absolute and put it in terms
		 * of the logical blocksize of the device.
		 */
		//Convert to physical block number
		blkno =
		    bp->b_blkno / (cd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
		if (IDE_CDPART(bp->b_dev) != RAW_PART) {
		      p = &cd->sc_dk.dk_label->d_partitions[IDE_CDPART(bp->b_dev)];
		      blkno += p->p_offset;
		}
		nblks = howmany(bp->b_bcount, cd->sc_dk.dk_label->d_secsize);

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		memset(&pc, 0, sizeof pc);
		pc.blkno = blkno;
		pc.nblks = nblks; 
		pc.buflen = bp->b_bcount;
		pc.buffer = bp->b_data;
		if((bp->b_flags & B_READ) != 0){
			make_read_command_packet(cd, &pc);
			ide_do_read_write(cd, &pc);
			if(pc.timeout || pc.error){
				bp->b_flags |= B_ERROR;
			}
			biodone(bp);
		}
	}
}

void make_read_command_packet(struct ide_cd_softc *cd, struct packet_cmd *pc)
{
	int nframes, sector ;
	
	nframes = pc->nblks; 
	sector = pc->blkno;
	
	pc->type =  T_READ; 

	pc->c[0] = GPCMD_READ_10;
	pc->c[7] = (nframes>>8) & 0xff; //How many blocks
	pc->c[8] = nframes & 0xff;
	pc->c[2] = (sector >> 24) & 0xff; //Block start address
	pc->c[3] = (sector >> 16) & 0xff;
	pc->c[4] = (sector >>  8) & 0xff;
	pc->c[5] = (sector >>  0) & 0xff;
}

void ide_cddone(struct ide_cd_softc *cd)
{
}

int
ide_cdread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(ide_cdstrategy, NULL, dev, B_READ, NULL, uio));
}

int
ide_cdwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(ide_cdstrategy, NULL, dev, B_WRITE, NULL, uio));
}

void ide_select_drive(struct ata_drive_datas *drvp)
{
	struct channel_softc *chp = drvp->chnl_softc ;
	int drive;
	
	drive = drvp->drive;

	CHP_WRITE_REG(chp, wdr_sdh, drive << 4);
	
}

int 
ide_transfer_command_packet(struct ata_drive_datas *drvp, struct packet_cmd *pc)
{
	
	struct channel_softc * chp = drvp->chnl_softc;
	int timeout = 5000;
	int cmd_len = sizeof(pc->c);
	char *cmd_buf = pc->c;
	int ireason;
	
#ifdef CD_DEBUG 	
	printf("%02d, transfer_packet: drvp %p\n", __LINE__, drvp);
#endif	

	if(wdcwait(chp, WDCS_DRQ, WDCS_DRQ, timeout)){
		printf("wait timeout in transfer packet \n");
		return -1;
	}

	ireason = CHP_READ_REG(chp, wdr_ireason);
	if((ireason & 3) !=1){
		printf("Wrong Intr reason in trans packet command\n");
	}

	wdc_output_bytes(drvp, cmd_buf, cmd_len);
	
	//splx();	

	return 0;
}
/*@flag 0: unload the media, 1: load the media 
 */

int ide_cd_prepare(struct ide_cd_softc *cd, int flag)
{
	struct packet_cmd pc;
	struct ata_drive_datas *drvp = cd->drvp;
	struct channel_softc *chp = drvp->chnl_softc;
	int timeout = 1000;
	int status;
			
	memset(&pc, 0, sizeof pc);
	pc.c[0] = GPCMD_START_STOP_UNIT;
	pc.c[4] = 0x02 + (flag !=0);
	
	ide_select_drive(drvp);
	
	CHP_WRITE_REG(chp, wdr_features, 0);
	CHP_WRITE_REG(chp, wdr_seccnt, 0);
	CHP_WRITE_REG(chp, wdr_sector, 0);
	CHP_WRITE_REG(chp, wdr_cyl_lo, 0);
	CHP_WRITE_REG(chp, wdr_cyl_hi, 0);

	CHP_WRITE_REG(chp, wdr_command, PACKET_CMD);	

	ide_transfer_command_packet(drvp, &pc);

	if(wdcwait(chp, WDCS_DRDY, WDCS_DRDY, timeout)){
		printf("timeout in ide_cd_prepare\n");
		return -1;
	}
	
	status = CHP_READ_REG(chp, wdr_status);
	if((status & WDCS_ERR)){
		printf("Errror in ide_cd_prepare\n");
		return -2;
	}
	
	return 0;
}

int ide_cd_prevent(struct ide_cd_softc *cd, int prevent)
{
	struct packet_cmd pc;
	struct ata_drive_datas *drvp = cd->drvp;
	struct channel_softc *chp = drvp->chnl_softc;
	int timeout =100;
	int status;
	
	memset(&pc, 0, sizeof pc);

	ide_select_drive(drvp);

	pc.c[0] = GPCMD_PREVENT_ALLOW_REMOVAL;
	pc.c[4] = prevent;
	
	
	CHP_WRITE_REG(chp, wdr_features, 0);
	CHP_WRITE_REG(chp, wdr_seccnt, 0);
	CHP_WRITE_REG(chp, wdr_sector, 0);
	CHP_WRITE_REG(chp, wdr_cyl_lo, 0);
	CHP_WRITE_REG(chp, wdr_cyl_hi, 0);

	CHP_WRITE_REG(chp, wdr_command, PACKET_CMD);

	ide_transfer_command_packet(drvp, &pc);

	if(wdcwait(chp, WDCS_DRDY, WDCS_DRDY, timeout)){
		printf("timeout in ide_cd_prevent\n");
		return -1;
	}
	
	status = CHP_READ_REG(chp, wdr_status);
	if((status & WDCS_ERR)){
		printf("Errror in ide_cd_prevent %x\n", status);
		return -2;
	}
	
	return 0;
}

int ide_test_unit_ready(struct ide_cd_softc *cd)
{
	struct packet_cmd pc;
	struct ata_drive_datas *drvp = cd->drvp;
	struct channel_softc *chp = drvp->chnl_softc;
	int status, err, sense_key;
	int timeout = 50000;
	
	ide_select_drive(drvp);	

	delay(10);

	if(wdcwait(chp, 0, 0, timeout)){
		printf("Check status timeout 1\n");
		return -1;
	}

	memset(&pc, 0, sizeof pc);
	pc.c[0] = GPCMD_TEST_UNIT_READY;

	CHP_WRITE_REG(chp, wdr_features, 0);
	CHP_WRITE_REG(chp, wdr_seccnt, 0);
	CHP_WRITE_REG(chp, wdr_sector, 0);
	CHP_WRITE_REG(chp, wdr_cyl_lo, 0);
	CHP_WRITE_REG(chp, wdr_cyl_hi, 0);
	CHP_WRITE_REG(chp, wdr_command, PACKET_CMD);
	
	ide_transfer_command_packet(drvp, &pc);
	
	if(wdcwait(chp, WDCS_DRDY, WDCS_DRDY, timeout)){
		printf("Check status timeout 2\n");
		return -1;
	}
	
	status = CHP_READ_REG(chp, wdr_status);
	err = CHP_READ_REG(chp, wdr_error);

	sense_key = err >> 4;
			
	if(status & WDCS_ERR){

		printf("Check status complete with check condition!!\n");		
		switch(sense_key){
		case NOT_READY:
			printf("check status not ready\n");
			break;
		case UNIT_ATTENTION:
			printf("check status unit attention\n");
			break;
		default:
			break;
		
		}
		printf("sense key =%x\n", sense_key);	
		return -2;
	}

	return 0;
}

int	ide_cdwrite_intr(struct ide_cd_softc *cd, struct packet_cmd *pc)
{
	(void)cd;

	pc->error = 1;
	printf("ide writer not implemented\n");
	
	return -1;		
}
