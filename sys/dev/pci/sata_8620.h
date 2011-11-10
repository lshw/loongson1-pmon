/*
 * Copyright (C) 2007-2008 Freescale Semiconductor, Inc.
 *		Dave Liu <daveliu@freescale.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __8620_H__
#define __8620_H__

#define SATA_HC_MAX_NUM		4 /* Max host controller numbers */
#define SATA_HC_MAX_CMD		16 /* Max command queue depth per host controller */
#define SATA_HC_MAX_PORT	16 /* Max port number per host controller */
#define PCI_IO_BASE  		0xbfd00000
/* HStatus register
*/
#define HSTATUS_ONOFF			0x80000000 /* Online/offline status */
#define HSTATUS_FORCE_OFFLINE		0x40000000 /* In process going offline */
#define HSTATUS_BIST_ERR		0x20000000

/* Fatal error */
#define HSTATUS_MASTER_ERR		0x00004000
#define HSTATUS_DATA_UNDERRUN		0x00002000
#define HSTATUS_DATA_OVERRUN		0x00001000
#define HSTATUS_CRC_ERR_TX		0x00000800
#define HSTATUS_CRC_ERR_RX		0x00000400
#define HSTATUS_FIFO_OVERFLOW_TX	0x00000200
#define HSTATUS_FIFO_OVERFLOW_RX	0x00000100
#define HSTATUS_FATAL_ERR_ALL		(HSTATUS_MASTER_ERR | \
					HSTATUS_DATA_UNDERRUN | \
					HSTATUS_DATA_OVERRUN | \
					HSTATUS_CRC_ERR_TX | \
					HSTATUS_CRC_ERR_RX | \
					HSTATUS_FIFO_OVERFLOW_TX | \
					HSTATUS_FIFO_OVERFLOW_RX)
/* Interrupt status */
#define HSTATUS_FATAL_ERR		0x00000020
#define HSTATUS_PHY_RDY			0x00000010
#define HSTATUS_SIGNATURE		0x00000008
#define HSTATUS_SNOTIFY			0x00000004
#define HSTATUS_DEVICE_ERR		0x00000002
#define HSTATUS_CMD_COMPLETE		0x00000001

/* HControl register
*/
#define HCONTROL_ONOFF			0x80000000 /* Online or offline request */
#define HCONTROL_FORCE_OFFLINE		0x40000000 /* Force offline request */
#define HCONTROL_HDR_SNOOP		0x00000400 /* Command header snoop */
#define HCONTROL_PMP_ATTACHED		0x00000200 /* Port multiplier attached */

/* Interrupt enable */
#define HCONTROL_FATAL_ERR		0x00000020
#define HCONTROL_PHY_RDY		0x00000010
#define HCONTROL_SIGNATURE		0x00000008
#define HCONTROL_SNOTIFY		0x00000004
#define HCONTROL_DEVICE_ERR		0x00000002
#define HCONTROL_CMD_COMPLETE		0x00000001

#define HCONTROL_INT_EN_ALL		(HCONTROL_FATAL_ERR | \
					HCONTROL_PHY_RDY | \
					HCONTROL_SIGNATURE | \
					HCONTROL_SNOTIFY | \
					HCONTROL_DEVICE_ERR | \
					HCONTROL_CMD_COMPLETE)

/* SStatus register
*/
#define SSTATUS_IPM_MASK		0x00000780
#define SSTATUS_IPM_NOPRESENT		0x00000000
#define SSTATUS_IPM_ACTIVE		0x00000080
#define SSTATUS_IPM_PATIAL		0x00000100
#define SSTATUS_IPM_SLUMBER		0x00000300

#define SSTATUS_SPD_MASK		0x000000f0
#define SSTATUS_SPD_GEN1		0x00000010
#define SSTATUS_SPD_GEN2		0x00000020

#define SSTATUS_DET_MASK		0x0000000f
#define SSTATUS_DET_NODEVICE		0x00000000
#define SSTATUS_DET_DISCONNECT		0x00000001
#define SSTATUS_DET_CONNECT		0x00000003
#define SSTATUS_DET_PHY_OFFLINE		0x00000004

/* SControl register
*/
#define SCONTROL_SPM_MASK		0x0000f000
#define SCONTROL_SPM_GO_PARTIAL		0x00001000
#define SCONTROL_SPM_GO_SLUMBER		0x00002000
#define SCONTROL_SPM_GO_ACTIVE		0x00004000

#define SCONTROL_IPM_MASK		0x00000f00
#define SCONTROL_IPM_NO_RESTRICT	0x00000000
#define SCONTROL_IPM_PARTIAL		0x00000100
#define SCONTROL_IPM_SLUMBER		0x00000200
#define SCONTROL_IPM_PART_SLUM		0x00000300

#define SCONTROL_SPD_MASK		0x000000f0
#define SCONTROL_SPD_NO_RESTRICT	0x00000000
#define SCONTROL_SPD_GEN1		0x00000010
#define SCONTROL_SPD_GEN2		0x00000020

#define SCONTROL_DET_MASK		0x0000000f
#define SCONTROL_DET_HRESET		0x00000001
#define SCONTROL_DET_DISABLE		0x00000004

/* TransCfg register
*/
#define TRANSCFG_DFIS_SIZE_SHIFT	16
#define TRANSCFG_RX_WATER_MARK_MASK	0x0000001f

/* PhyCtrlCfg register
*/
#define PHYCTRLCFG_FPRFTI_MASK		0x00000018
#define PHYCTRLCFG_LOOPBACK_MASK	0x0000000e

/*
* Command Header Entry
*/
typedef struct cmd_hdr_entry {
	u8  		CommandFISLen:5;
        u8     	        AtapiPioFIS:1;
        u8              ReadWrite:1;
        u8              PreFetchAble:1;
        u8              Reset:1;
        u8              Bist:1;
        u8              ClearBusy:1;
        u8              Reserved0       :1;
        u8              PortMultiplier:4;
	u16		prdtl;		
	u32 		prdbc;		/* physical region descriptor byte count */
	u32 		ctba;		/* Command Table Descriptor Base Address, 128 bytes aligned */
	u32 		ctba_u;		/* Upper CTBA */
} __attribute__ ((packed)) cmd_hdr_entry_t;

#define SATA_HC_CMD_HDR_ENTRY_SIZE	sizeof(struct cmd_hdr_entry)

/* cda
*/
#define CMD_HDR_CDA_ALIGN	4

/* prde_fis_len
*/
#define CMD_HDR_PRD_ENTRY_SHIFT	16
#define CMD_HDR_PRD_ENTRY_MASK	0x003f0000
#define CMD_HDR_FIS_LEN_SHIFT	2

/* attribute
*/
#define CMD_HDR_ATTR_RES	0x00000800 /* Reserved bit, should be 1 */
#define CMD_HDR_ATTR_VBIST	0x00000400 /* Vendor BIST */
#define CMD_HDR_ATTR_SNOOP	0x00000200 /* Snoop enable for all descriptor */
#define CMD_HDR_ATTR_FPDMA	0x00000100 /* FPDMA queued command */
#define CMD_HDR_ATTR_RESET	0x00000080 /* Reset - a SRST or device reset */
#define CMD_HDR_ATTR_BIST	0x00000040 /* BIST - require the host to enter BIST mode */
#define CMD_HDR_ATTR_ATAPI	0x00000020 /* ATAPI command */
#define CMD_HDR_ATTR_TAG	0x0000001f /* TAG mask */

/* command type
*/
enum cmd_type {
	CMD_VENDOR_BIST,
	CMD_BIST,
	CMD_RESET,	/* SRST or device reset */
	CMD_ATAPI,
	CMD_NCQ,
	CMD_ATA,	/* None of all above */
};

/*
* Command Header Table
*/
typedef struct cmd_hdr_tbl {
	cmd_hdr_entry_t cmd_slot[SATA_HC_MAX_CMD];
} __attribute__ ((packed)) cmd_hdr_tbl_t;

#define SATA_HC_CMD_HDR_TBL_SIZE	sizeof(struct cmd_hdr_tbl)
#define SATA_HC_CMD_HDR_TBL_ALIGN	1024	

/*
* PRD entry - Physical Region Descriptor entry
*/
typedef struct prd_entry {
	u32 dba;	/* Data base address, 4 bytes aligned */
	u32 dba_u;
	u32 reserved;
	u32 dbc;	/* Indirect PRD flags, snoop and data word count */
} __attribute__ ((packed)) prd_entry_t;

#define SATA_HC_CMD_DESC_PRD_SIZE	sizeof(struct prd_entry)

/* dba
*/
#define PRD_ENTRY_DBA_ALIGN	4

/* dbc
*/
#define PRD_ENTRY_EXT		0x80000000 /* extension flag or called indirect descriptor flag */
#define PRD_ENTRY_DATA_SNOOP	0x00400000 /* Snoop enable for all data associated with the PRD entry */
#define PRD_ENTRY_LEN_MASK	0x003fffff /* Data word count */

#define PRD_ENTRY_MAX_XFER_SZ	0x10000

/*
 * This SATA host controller supports a max of 16 direct PRD entries, but if use
 * chained indirect PRD entries, then the contollers supports upto a max of 63
 * entries including direct and indirect PRD entries.
 * The PRDT is an array of 63 PRD entries contigiously, but the PRD entries#15
 * will be setup as an indirect descriptor, pointing to it's next (contigious)
 * PRD entries#16.
 */
#define SATA_HC_MAX_PRD		63 /* Max PRD entry numbers per command */
#define SATA_HC_MAX_PRD_DIRECT	16 /* Direct PRDT entries */
#define SATA_HC_MAX_PRD_USABLE	(SATA_HC_MAX_PRD - 1)
#define SATA_HC_MAX_XFER_LEN	0x4000000

/*
* Command Descriptor
*/

typedef struct cmd_desc {
	u8 cfis[64];
	u8 AtapiCommand[32];
	u8 Reserved[32];
	prd_entry_t prdt[24];
} __attribute__ ((packed)) cmd_desc_t;

#define SATA_HC_CMD_DESC_SIZE		sizeof(struct cmd_desc)
#define SATA_HC_CMD_DESC_ALIGN		128	

/*
* CFIS - Command FIS, which is H2D register FIS, the struct defination
* of Non-Queued command is different than NCQ command. see them is sata2.h
*/
typedef struct cfis {
	u8 fis_type;
	u8 pm_port_c:4;
	u8 FG_Reserved:3;
	u8 FIS_Flag:1;
	u8 command;
	u8 features;
	u8 lba_low;
	u8 lba_mid;
	u8 lba_high;
	u8 device;
	u8 lba_low_exp;
	u8 lba_mid_exp;
	u8 lba_high_exp;
	u8 features_exp;
	u8 sector_count;
	u8 sector_count_exp;
	u8 res1;
	u8 control;
	u8 res2[4];
} __attribute__ ((packed)) cfis_t;

typedef struct _DmaSetupFIS
{
	u8	FIS_Type;
	u8	FIS_Flag;
	u16	Reserved0;
	u32	DmaBufferIdentifierL;
	u32	DmaBufferIdentifierH;
	u32	Reserved1;
	u32	DmaBufferOffset;
	u32	DmaTransferCount;
	u32	Reserved2;
} T_DmaSetupFIS,*P_DmaSetupFIS;
//===========================================================================
typedef struct _PioSetupFIS
{
	u8	FIS_Type;
	u8	FIS_Flag;
	u8	Status;
	u8	Error;
	u8	P1x3;
	u8	P1x4;
	u8	P1x5;
	u8	P1x6;
	u8	P1x3Exp;
	u8	P1x4Exp;
	u8	P1x5Exp;
	u8	Reserved0;
	u8	P1x2;
	u8	P1x2Exp;
	u8	Reserved1;
	u8	E_Status;
	u16	TransferCount;
	u16	Reserved2;
} T_PioSetupFIS,*P_PioSetupFIS;
//===========================================================================
typedef struct _Device2Host
{
	u8	FIS_Type;
	u8	FIS_Flag;
	u8	Status;
	u8	Error;
	u8	P1x3;
	u8	P1x4;
	u8	P1x5;
	u8	P1x6;
	u8	P1x3Exp;
	u8	P1x4Exp;
	u8	P1x5Exp;
	u8	Reserved0;
	u8	P1x2;
	u8	P1x2Exp;
	u16	Reserved1;
	u32	Reserved2;
}T_Device2Host,*P_Device2Host;
//===========================================================================
typedef struct _SetDeviceBitsFIS
{
	u8	FIS_Type;
	u8	PortMultiplier:4;
	u8	FG_Interrupt:4;
	u8	Status;
	u8	Error;
	u32	CompleteMap;
}T_SetDeviceBitsFIS,*P_SetDeviceBitsFIS;
//===========================================================================
typedef struct _ReceiveFIS
{
	T_DmaSetupFIS	DmaSetupFIS;		//  28 bytes
	u32	Reserved0;		//   4 bytes
	T_PioSetupFIS	PioSetupFIS;		//  20 bytes
	u32	Reserved1[3];		//  12 bytes
	T_Device2Host	Device2HostFIS;		//  20bytes
	u32	Reserved2;		//   4 bytes
	T_SetDeviceBitsFIS SetDeviceBitsFIS;	//   8 bytes
	u8	Reserved3[32];		//  32 bytes
} __attribute__ ((packed)) T_ReceiveFIS;
/*
 * SATA device driver info
 */
typedef struct atp_sata_info {
	struct ata_atapi_attach aa_link; //just for not match id
	u32	sata_reg_base;
	u32	flags;
} atp_sata_info_t;

#define FLAGS_DMA	0x00000000
#define FLAGS_FPDMA	0x00000001

/*
 * SATA device driver struct
 */
typedef struct atp_sata {
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */
	struct channel_softc *wdc_chanarray[1];
	struct channel_softc wdc_channel; /* generic part */
	char		name[12];
	u32		reg_base;		/* the base address of controller register */
	void		*cmd_hdr_tbl_offset;	/* alloc address of command header table */
	cmd_hdr_tbl_t	*cmd_hdr;		/* aligned address of command header table */
	void		*cmd_desc_offset;	/* alloc address of command descriptor */
	cmd_desc_t	*cmd_desc;		/* aligned address of command descriptor */
	void 		*receive_fis_offset;
	T_ReceiveFIS	*receive_fis;		
	int		link;			/* PHY link status */
	/* device attribute */
	int		ata_device_type;	/* device type */
	int		lba48;
	int		queue_depth;		/* Max NCQ queue depth */
	u16		pio;
	u16		mwdma;
	u16		udma;
	int		wcache;
	int		flush;
	int		flush_ext;
} atp_sata_t;

#define READ_CMD	0
#define WRITE_CMD	1

static int ata_wait_register(volatile unsigned *addr, u32 mask, u32 val, u32 timeout_msec);
void dump_print(u32 reg,u32 num);
int init_sata(int dev,u32 reg);
static int atp_ata_exec_ata_cmd(struct atp_sata *sata, struct cfis *cfis,int is_ncq, int tag, u8 *buffer, u32 len);
static int atp_ata_exec_reset_cmd(struct atp_sata *sata, struct cfis *cfis,int tag, u8 *buffer, u32 len);
static int atp_sata_exec_cmd(struct atp_sata *sata, struct cfis *cfis, enum cmd_type command_type, int tag, u8 *buffer, u32 len);
static void atp_sata_identify(int dev, u16 *id,int tag);
static void atp_sata_xfer_mode(int dev, u16 *id);
static void atp_sata_set_features(int dev);
static u32 atp_sata_rw_cmd(int dev, u32 start, u32 blkcnt, u8 *buffer, int is_write);
static u32 atp_sata_rw_cmd_ext(int dev, u32 start, u32 blkcnt, u8 *buffer, int is_write);
u32 atp_sata_rw_ncq_cmd(int dev, u32 start, u32 blkcnt, u8 *buffer, int is_write);
/* Software reset, set SRST of the Device Control register */
int atp_sata_software_reset(int dev,int tag);
static void atp_sata_init_wcache(int dev, u16 *id);
u32 ata_low_level_rw_lba48(int dev, u32 blknr, u32 blkcnt, void *buffer, int is_write);
u32 ata_low_level_rw_lba28(int dev, u32 blknr, u32 blkcnt, void *buffer, int is_write);
/*
 * SATA interface between low level driver and command layer
 */
ulong atp_sata_read(int dev, unsigned long blknr, lbaint_t blkcnt, void *buffer);
ulong atp_sata_write(int dev, unsigned long blknr, lbaint_t blkcnt, void *buffer);
int scan_sata(int dev);

#endif /* __8620_H__ */
