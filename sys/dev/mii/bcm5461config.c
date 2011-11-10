/*
 * This file comes from linux 2.6.21.
 * This is a part of Broadcom.c
 *
 * /



#define MII_BCM54XX_ECR		0x10	/* BCM54xx extended control register */
#define MII_BCM54XX_ECR_IM	0x1000	/* Interrupt mask */
#define MII_BCM54XX_ECR_IF	0x0800	/* Interrupt force */

#define MII_BCM54XX_ESR		0x11	/* BCM54xx extended status register */
#define MII_BCM54XX_ESR_IS	0x1000	/* Interrupt status */

#define MII_BCM54XX_ISR		0x1a	/* BCM54xx interrupt status register */
#define MII_BCM54XX_IMR		0x1b	/* BCM54xx interrupt mask register */
#define MII_BCM54XX_INT_CRCERR	0x0001	/* CRC error */
#define MII_BCM54XX_INT_LINK	0x0002	/* Link status changed */
#define MII_BCM54XX_INT_SPEED	0x0004	/* Link speed change */
#define MII_BCM54XX_INT_DUPLEX	0x0008	/* Duplex mode changed */
#define MII_BCM54XX_INT_LRS	0x0010	/* Local receiver status changed */
#define MII_BCM54XX_INT_RRS	0x0020	/* Remote receiver status changed */
#define MII_BCM54XX_INT_SSERR	0x0040	/* Scrambler synchronization error */
#define MII_BCM54XX_INT_UHCD	0x0080	/* Unsupported HCD negotiated */
#define MII_BCM54XX_INT_NHCD	0x0100	/* No HCD */
#define MII_BCM54XX_INT_NHCDL	0x0200	/* No HCD link */
#define MII_BCM54XX_INT_ANPR	0x0400	/* Auto-negotiation page received */
#define MII_BCM54XX_INT_LC	0x0800	/* All counters below 128 */
#define MII_BCM54XX_INT_HC	0x1000	/* Counter above 32768 */
#define MII_BCM54XX_INT_MDIX	0x2000	/* MDIX status change */
#define MII_BCM54XX_INT_PSERR	0x4000	/* Pair swap error */


static int bcm54xx_config_init(struct synopGMACdevice *gmacdev)
{
	int retval, err;
	unsigned short data;

	retval = synopGMAC_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase,MII_BCM54XX_ECR,&data);
	if (retval < 0)
		return retval;

	/* Mask interrupts globally.  */
	data |= MII_BCM54XX_ECR_IM;
	err = synopGMAC_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase,MII_BCM54XX_ECR,data);
	if (err < 0)
		return err;

	/* Unmask events we are interested in.  */
	data = ~(MII_BCM54XX_INT_DUPLEX |
		MII_BCM54XX_INT_SPEED |
		MII_BCM54XX_INT_LINK);
	err = synopGMAC_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase,MII_BCM54XX_IMR,data);
	if (err < 0)
		return err;
	return 0;
}

static int bcm54xx_ack_interrupt(struct synopGMACdevice *gmacdev)
{
	int reg;
	unsigned short data;
	
	/* Clear pending interrupts.  */
	reg = synopGMAC_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase,MII_BCM54XX_ISR,&data);
	if (reg < 0)
		return reg;

	printf("===phy intr status: %04x\n",data);
	
	return 0;
}

static int bcm54xx_config_intr(struct synopGMACdevice *gmacdev)
{
	bcm54xx_config_init(gmacdev);
}


