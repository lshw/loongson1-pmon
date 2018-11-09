/*
 * cs5536.h
 * 
 * The include file of cs5536 sourthbridge define which is used in the pmon only.
 * you can modify it or change it, please set the modify time and steps.
 *
 * Author : jlliu <liujl@lemote.com>
 * Data : 07-6-27
 */

#ifndef	_CS5536_H
#define	_CS5536_H

/*************************************************************************/

/*
 * basic define
 */
#define	PCI_IO_BASE		0x1fd00000	//( < 0x1fe00000)
#define	PCI_IO_BASE_VA		0xbfd00000
#define	PCI_MEM_BASE		0x10000000	//( < 0x1c000000 )
#define	PCI_MEM_BASE_VA		0xb0000000

/*
 * MSR module base
 */ 
#define GET_MSR_ADDR(x) (((x<<9)&0xff100000)|(x&0x3fff))
#define	CS5536_SB_MSR_BASE		(0x00000000)
#define	CS5536_GLIU_MSR_BASE		(0x10000000)
#define	CS5536_ILLEGAL_MSR_BASE		(0x20000000)
#define	CS5536_USB_MSR_BASE		(0x40000000)
#define	CS5536_IDE_MSR_BASE		(0x60000000)
#define	CS5536_DIVIL_MSR_BASE		(0x80000000)
#define	CS5536_ACC_MSR_BASE		(0xa0000000)
#define	CS5536_UNUSED_MSR_BASE		(0xc0000000)
#define	CS5536_GLCP_MSR_BASE		(0xe0000000)

#define	SB_MSR_REG(offset)	(CS5536_SB_MSR_BASE	| offset)
#define	GLIU_MSR_REG(offset)	(CS5536_GLIU_MSR_BASE	| offset)
#define	ILLEGAL_MSR_REG(offset)	(CS5536_ILLEGAL_MSR_BASE| offset)
#define	USB_MSR_REG(offset)	(CS5536_USB_MSR_BASE	| offset)
#define	IDE_MSR_REG(offset)	(CS5536_IDE_MSR_BASE	| offset)
#define	DIVIL_MSR_REG(offset)	(CS5536_DIVIL_MSR_BASE	| offset)
#define	ACC_MSR_REG(offset)	(CS5536_ACC_MSR_BASE	| offset)
#define	UNUSED_MSR_REG(offset)	(CS5536_UNUSED_MSR_BASE	| offset)
#define	GLCP_MSR_REG(offset)	(CS5536_GLCP_MSR_BASE	| offset)
/*
 * PCI MSR ACCESS
 */
#define	PCI_MSR_CTRL		0xF0
#define	PCI_MSR_ADDR		0xF4
#define	PCI_MSR_DATA_LO		0xF8
#define	PCI_MSR_DATA_HI		0xFC

/******************************* MSR *****************************************/

/*
 * GLIU STANDARD MSR
 */
#define	GLIU_CAP		0x00
#define	GLIU_CONFIG		0x01
#define	GLIU_SMI		0x02
#define	GLIU_ERROR		0x03
#define	GLIU_PM			0x04
#define	GLIU_DIAG		0x05

/*
 * GLIU SPEC. MSR
 */
#define	GLIU_P2D_BM0		0x20 
#define	GLIU_P2D_BM1		0x21 
#define	GLIU_P2D_BM2		0x22 
#define	GLIU_P2D_BMK0		0x23
#define	GLIU_P2D_BMK1		0x24
#define	GLIU_P2D_BM3		0x25 
#define	GLIU_P2D_BM4		0x26 
#define	GLIU_COH		0x80
#define	GLIU_PAE		0x81	
#define	GLIU_ARB		0x82
#define	GLIU_ASMI		0x83
#define	GLIU_AERR		0x84
#define	GLIU_DEBUG		0x85
#define	GLIU_PHY_CAP		0x86
#define	GLIU_NOUT_RESP		0x87
#define	GLIU_NOUT_WDATA		0x88
#define	GLIU_WHOAMI		0x8B
#define	GLIU_SLV_DIS		0x8C
#define	GLIU_IOD_BM0		0xE0
#define	GLIU_IOD_BM1		0xE1
#define	GLIU_IOD_BM2		0xE2
#define	GLIU_IOD_BM3		0xE3
#define	GLIU_IOD_BM4		0xE4
#define	GLIU_IOD_BM5		0xE5
#define	GLIU_IOD_BM6		0xE6
#define	GLIU_IOD_BM7		0xE7
#define	GLIU_IOD_BM8		0xE8
#define	GLIU_IOD_BM9		0xE9
#define	GLIU_IOD_SC0		0xEA
#define	GLIU_IOD_SC1		0xEB
#define	GLIU_IOD_SC2		0xEC
#define	GLIU_IOD_SC3		0xED
#define	GLIU_IOD_SC4		0xEE
#define	GLIU_IOD_SC5		0xEF
#define	GLIU_IOD_SC6		0xF0
#define	GLIU_IOD_SC7		0xF1

/*
 * SB STANDARD
 */
#define	SB_CAP		0x00
#define	SB_CONFIG	0x01
#define	SB_SMI		0x02
#define	SB_ERROR	0x03
#define	SB_MAR_ERR_EN		0x00000001
#define	SB_TAR_ERR_EN		0x00000002
#define	SB_RSVD_BIT1		0x00000004
#define	SB_EXCEP_ERR_EN		0x00000008
#define	SB_SYSE_ERR_EN		0x00000010
#define	SB_PARE_ERR_EN		0x00000020
#define	SB_TAS_ERR_EN		0x00000040
#define	SB_MAR_ERR_FLAG		0x00010000
#define	SB_TAR_ERR_FLAG		0x00020000
#define	SB_RSVD_BIT2		0x00040000
#define	SB_EXCEP_ERR_FLAG	0x00080000
#define	SB_SYSE_ERR_FLAG	0x00100000
#define	SB_PARE_ERR_FLAG	0x00200000
#define	SB_TAS_ERR_FLAG		0x00400000
#define	SB_PM		0x04
#define	SB_DIAG		0x05

/*
 * SB SPEC.
 */
#define	SB_CTRL		0x10
#define	SB_R0		0x20
#define	SB_R1		0x21
#define	SB_R2		0x22
#define	SB_R3		0x23
#define	SB_R4		0x24
#define	SB_R5		0x25
#define	SB_R6		0x26
#define	SB_R7		0x27
#define	SB_R8		0x28
#define	SB_R9		0x29
#define	SB_R10		0x2A
#define	SB_R11		0x2B
#define	SB_R12		0x2C
#define	SB_R13		0x2D
#define	SB_R14		0x2E
#define	SB_R15		0x2F

/*
 * GLCP STANDARD
 */
#define	GLCP_CAP		0x00
#define	GLCP_CONFIG		0x01
#define	GLCP_SMI		0x02
#define	GLCP_ERROR		0x03
#define	GLCP_PM			0x04
#define	GLCP_DIAG		0x05

/*
 * GLCP SPEC. 
 */
#define	GLCP_CLK_DIS_DELAY	0x08
#define	GLCP_PM_CLK_DISABLE	0x09
#define	GLCP_GLB_PM		0x0B
#define	GLCP_DBG_OUT		0x0C
#define	GLCP_RSVD1		0x0D
#define	GLCP_SOFT_COM		0x0E
#define	SOFT_BAR_SMB_FLAG		0x00000001
#define	SOFT_BAR_GPIO_FLAG		0x00000002
#define	SOFT_BAR_MFGPT_FLAG		0x00000004
#define	SOFT_BAR_IRQ_FLAG		0x00000008
#define	SOFT_BAR_PMS_FLAG		0x00000010
#define	SOFT_BAR_ACPI_FLAG		0x00000020
#define	SOFT_BAR_FLSH0_FLAG		0x00000040
#define	SOFT_BAR_FLSH1_FLAG		0x00000080
#define	SOFT_BAR_FLSH2_FLAG		0x00000100
#define	SOFT_BAR_FLSH3_FLAG		0x00000200
#define	SOFT_BAR_IDE_FLAG		0x00000400
#define	SOFT_BAR_ACC_FLAG		0x00000800
#define	SOFT_BAR_OHCI_FLAG		0x00001000
#define	SOFT_BAR_EHCI_FLAG		0x00002000
#define	SOFT_BAR_UDC_FLAG		0x00004000
#define	SOFT_BAR_OTG_FLAG		0x00008000
#define	GLCP_RSVD2		0x0F
#define	GLCP_CLK_OFF		0x10
#define	GLCP_CLK_ACTIVE		0x11
#define	GLCP_CLK_DISABLE	0x12
#define	GLCP_CLK4ACK		0x13
#define	GLCP_SYS_RST		0x14
#define	GLCP_RSVD3		0x15
#define	GLCP_DBG_CLK_CTRL	0x16
#define	GLCP_CHIP_REV_ID	0x17

/*
 * DIVIL STANDARD
 */
#define	DIVIL_CAP		0x00
#define	DIVIL_CONFIG		0x01
#define	DIVIL_SMI		0x02
#define	DIVIL_ERROR		0x03
#define	DIVIL_PM		0x04
#define	DIVIL_DIAG		0x05

/*
 * DIVIL SPEC. 
 */
#define	DIVIL_LBAR_IRQ		0x08
#define	DIVIL_LBAR_KEL		0x09
#define	DIVIL_LBAR_SMB		0x0B
#define	DIVIL_LBAR_GPIO		0x0C
#define	DIVIL_LBAR_MFGPT	0x0D
#define	DIVIL_LBAR_ACPI		0x0E
#define	DIVIL_LBAR_PMS		0x0F
#define	DIVIL_LBAR_FLSH0	0x10
#define	DIVIL_LBAR_FLSH1	0x11
#define	DIVIL_LBAR_FLSH2	0x12
#define	DIVIL_LBAR_FLSH3	0x13
#define	DIVIL_LEG_IO		0x14
#define	DIVIL_BALL_OPTS		0x15
#define	DIVIL_SOFT_IRQ		0x16
#define	DIVIL_SOFT_RESET	0x17
// NOR FLASH
#define	NORF_CTRL		0x18
#define	NORF_T01		0x19
#define	NORF_T23		0x1A
// NAND FLASH
#define	NANDF_DATA		0x1B
#define	NANDF_CTRL		0x1C
#define	NANDF_RSVD		0x1D
// KEL Keyboard Emulation Logic
#define	KEL_CTRL		0x1F
// PIC
#define	PIC_YZSEL_LOW		0x20
#define	PIC_YSEL_HIGH		0x21
#define	PIC_ZSEL_LOW		0x22
#define	PIC_ZSEL_HIGH		0x23
#define	PIC_IRQM_PRIM		0x24
#define	PIC_IRQM_LPC		0x25
#define	PIC_XIRR_STS_LOW	0x26
#define	PIC_XIRR_STS_HIGH	0x27
#define	PCI_SHDW		0x34
// MFGPT
#define	MFGPT_IRQ		0x28
#define	MFGPT_NR		0x29
#define	MFGPT_RSVD		0x2A
#define	MFGPT_SETUP		0x2B
// FLOPPY
#define	FLPY_3F2_SHDW		0x30
#define	FLPY_3F7_SHDW		0x31
#define	FLPY_372_SHDW		0x32
#define	FLPY_377_SHDW		0x33
// PIT
#define	PIT_SHDW		0x36
#define	PIT_CNTRL		0x37
// UART
#define	UART1_MOD		0x38
#define	UART1_DONG		0x39
#define	UART1_CONF		0x3A
#define	UART1_RSVD		0x3B
#define	UART2_MOD		0x3C
#define	UART2_DONG		0x3D
#define	UART2_CONF		0x3E
#define	UART2_RSVD		0x3F
// DMA
#define	DIVIL_AC_DMA		0x1E
#define	DMA_MAP			0x40
#define	DMA_SHDW_CH0		0x41
#define	DMA_SHDW_CH1		0x42
#define	DMA_SHDW_CH2		0x43
#define	DMA_SHDW_CH3		0x44
#define	DMA_SHDW_CH4		0x45
#define	DMA_SHDW_CH5		0x46
#define	DMA_SHDW_CH6		0x47
#define	DMA_SHDW_CH7		0x48
#define	DMA_MSK_SHDW		0x49
// LPC
#define	LPC_EADDR		0x4C
#define	LPC_ESTAT		0x4D
#define	LPC_SIRQ		0x4E
#define	LPC_RSVD		0x4F
// PMC
#define	PMC_LTMR		0x50
#define	PMC_RSVD		0x51
// RTC
#define	RTC_RAM_LOCK		0x54
#define	RTC_DOMA_OFFSET		0x55
#define	RTC_MONA_OFFSET		0x56
#define	RTC_CEN_OFFSET		0x57

/*
 * IDE STANDARD 
 */
#define	IDE_CAP		0x00
#define	IDE_CONFIG	0x01
#define	IDE_SMI		0x02
#define	IDE_ERROR	0x03
#define	IDE_PM		0x04
#define	IDE_DIAG	0x05

/*
 * ACC STANDARD
 */
#define	ACC_CAP		0x00
#define	ACC_CONFIG	0x01
#define	ACC_SMI		0x02
#define	ACC_ERROR	0x03
#define	ACC_PM		0x04
#define	ACC_DIAG	0x05

/*
 * IDE SPEC. 
 */
#define	IDE_IO_BAR	0x08
#define	IDE_CFG		0x10
#define	IDE_DTC		0x12
#define	IDE_CAST	0x13
#define	IDE_ETC		0x14
#define	IDE_INTERNAL_PM	0x15

/*
 * USB STANDARD
 */
#define	USB_CAP		0x00
#define	USB_CONFIG	0x01
#define	USB_SMI		0x02
#define	USB_ERROR	0x03
#define	USB_PM		0x04
#define	USB_DIAG	0x05

/*
 * USB SPEC.
 */
#define	USB_OHCI	0x08
#define	USB_EHCI	0x09
#define	USB_UDC		0x0A
#define	USB_OTG		0x0B

/********************************** NATIVE ************************************/

#define	CS5536_IDE_RANGE	0xfffffff0
#define	CS5536_IDE_LENGTH	0x10

/*
 * IDE NATIVE : I/O SPACE
 * REG : 8BITS WIDTH
 * BASE : DETERMINED BY MSR
 */
#define	IDE_BM_CMD	0x00
#define	IDE_BM_STS	0x02
#define	IDE_BM_PRD	0x04

/*
 * ACC
 */
#define	CS5536_ACC_RANGE	0xffffff80
#define	CS5536_ACC_LENGTH	0x80

/*
 * USB NATIVE : MEM SPACE
 * REG : 32BITS WIDTH
 */
#define	CS5536_OHCI_RANGE	0xfffff000
#define	CS5536_OHCI_LENGTH	0x1000

#define	CS5536_EHCI_RANGE	0xfffff000
#define	CS5536_EHCI_LENGTH	0x1000

#define	CS5536_UDC_RANGE	0xffffe000
#define	CS5536_UDC_LENGTH	0x2000

#define	CS5536_OTG_RANGE	0xfffff000
#define	CS5536_OTG_LENGTH	0x1000

// OHCI NATIVE
#define	OHCI_REVISION		0x00
#define	OHCI_CONTROL		0x04
#define	OHCI_COMMAND_STATUS	0x08
#define	OHCI_INT_STATUS		0x0C
#define	OHCI_INT_ENABLE		0x10
#define	OHCI_INT_DISABLE	0x14
#define	OHCI_HCCA		0x18
#define	OHCI_PERI_CUR_ED	0x1C
#define	OHCI_CTRL_HEAD_ED	0x20
#define	OHCI_CTRL_CUR_ED	0x24
#define	OHCI_BULK_HEAD_ED	0x28
#define	OHCI_BULK_CUR_ED	0x2C
#define	OHCI_DONE_HEAD		0x30
#define	OHCI_FM_INTERVAL	0x34
#define	OHCI_FM_REMAINING	0x38
#define	OHCI_FM_NUMBER		0x3C
#define	OHCI_PERI_START		0x40
#define	OHCI_LS_THRESHOLD	0x44
#define	OHCI_RH_DESCRIPTORA	0x48
#define	OHCI_RH_DESCRIPTORB	0x4C
#define	OHCI_RH_STATUS		0x50
#define	OHCI_RH_PORT_STATUS1	0x54
#define	OHCI_RH_PORT_STATUS2	0x58
#define	OHCI_RH_PORT_STATUS3	0x5C
#define	OHCI_RH_PORT_STATUS4	0x60

/*
 * DIVIL NATIVE
 */
#define	CS5536_IRQ_RANGE		0xffffffe0	// USERD FOR PCI PROBE
#define	CS5536_IRQ_LENGTH		0x20		// THE REGS ACTUAL LENGTH

#define	CS5536_SMB_RANGE		0xfffffff8
#define	CS5536_SMB_LENGTH		0x08

#define	CS5536_GPIO_RANGE		0xffffff00
#define	CS5536_GPIO_LENGTH		0x100

#define	CS5536_MFGPT_RANGE		0xffffffc0
#define	CS5536_MFGPT_LENGTH		0x40

#define	CS5536_ACPI_RANGE		0xffffffe0
#define	CS5536_ACPI_LENGTH		0x20

#define	CS5536_PMS_RANGE		0xffffff80
#define	CS5536_PMS_LENGTH		0x80


// KEL : MEM SPACE; REG :32BITS WIDTH
#define	KEL_HCE_CTRL	0x100
#define	KEL_HCE_IN	0x104
#define	KEL_HCE_OUT	0x108
#define	KEL_HCE_STS	0x10C
#define	KEL_PORTA	0x92	//8bits
// PIC : I/O SPACE; REG : 8BITS
#define	PIC_ICW1_MASTER	0x20
#define	PIC_ICW1_SLAVE	0xA0
#define	PIC_ICW2_MASTER	0x21
#define	PIC_ICW2_SLAVE	0xA1
#define	PIC_ICW3_MASTER	0x21
#define	PIC_ICW3_SLAVE	0xA1
#define	PIC_ICW4_MASTER	0x21
#define	PIC_ICW4_SLAVE	0xA1
#define	PIC_OCW1_MASTER	0x21
#define	PIC_OCW1_SLAVE	0xA1
#define	PIC_OCW2_MASTER	0x20
#define	PIC_OCW2_SLAVE	0xA0
#define	PIC_OCW3_MASTER	0x20
#define	PIC_OCW3_SLAVE	0xA0
#define	PIC_IRR_MASTER	0x20
#define	PIC_IRR_SLAVE	0xA0
#define	PIC_ISR_MASTER	0x20
#define	PIC_ISR_SLAVE	0xA0
#define	PIC_INT_SEL1	0x4D0
#define	PIC_INT_SEL2	0x4D1
// GPIO : I/O SPACE; REG : 32BITS
#define	GPIOL_OUT_VAL		0x00
#define	GPIOL_OUT_EN		0x04
#define	GPIOL_OUT_OD_EN		0x08
#define	GPIOL_OUT_INVRT_EN	0x0c
#define	GPIOL_OUT_AUX1_SEL	0x10
#define	GPIOL_OUT_AUX2_SEL	0x14
#define	GPIOL_PU_EN		0x18
#define	GPIOL_PD_EN		0x1c
#define	GPIOL_IN_EN		0x20
#define	GPIOL_IN_INVRT_EN	0x24
#define	GPIOL_IN_FLTR_EN	0x28
#define	GPIOL_IN_EVNTCNT_EN	0x2c
#define	GPIOL_IN_READBACK	0x30
#define	GPIOL_IN_AUX1_SEL	0x34
#define	GPIOL_EVNT_EN		0x38
#define	GPIOL_LOCK_EN		0x3c
#define	GPIOL_IN_POSEDGE_EN	0x40
#define	GPIOL_IN_NEGEDGE_EN	0x44
#define	GPIOL_IN_POSEDGE_STS	0x48
#define	GPIOL_IN_NEGEDGE_STS	0x4c
#define	GPIOH_OUT_VAL		0x80
#define	GPIOH_OUT_EN		0x84
#define	GPIOH_OUT_OD_EN		0x88
#define	GPIOH_OUT_INVRT_EN	0x8c
#define	GPIOH_OUT_AUX1_SEL	0x90
#define	GPIOH_OUT_AUX2_SEL	0x94
#define	GPIOH_PU_EN		0x98
#define	GPIOH_PD_EN		0x9c
#define	GPIOH_IN_EN		0xA0
#define	GPIOH_IN_INVRT_EN	0xA4
#define	GPIOH_IN_FLTR_EN	0xA8
#define	GPIOH_IN_EVNTCNT_EN	0xAc
#define	GPIOH_IN_READBACK	0xB0
#define	GPIOH_IN_AUX1_SEL	0xB4
#define	GPIOH_EVNT_EN		0xB8
#define	GPIOH_LOCK_EN		0xBc
#define	GPIOH_IN_POSEDGE_EN	0xC0
#define	GPIOH_IN_NEGEDGE_EN	0xC4
#define	GPIOH_IN_POSEDGE_STS	0xC8
#define	GPIOH_IN_NEGEDGE_STS	0xCC
// SMB : I/O SPACE, REG : 8BITS WIDTH
#define	SMB_SDA				0x00
#define	SMB_STS				0x01
#define	SMB_STS_SLVSTP		(1 << 7)
#define	SMB_STS_SDAST		(1 << 6)
#define	SMB_STS_BER		(1 << 5)
#define	SMB_STS_NEGACK		(1 << 4)
#define	SMB_STS_STASTR		(1 << 3)
#define	SMB_STS_NMATCH		(1 << 2)
#define	SMB_STS_MASTER		(1 << 1)
#define	SMB_STS_XMIT		(1 << 0)
#define	SMB_CTRL_STS			0x02
#define	SMB_CSTS_TGSTL		(1 << 5)
#define	SMB_CSTS_TSDA		(1 << 4)
#define	SMB_CSTS_GCMTCH		(1 << 3)
#define	SMB_CSTS_MATCH		(1 << 2)
#define	SMB_CSTS_BB		(1 << 1)
#define	SMB_CSTS_BUSY		(1 << 0)
#define	SMB_CTRL1			0x03
#define	SMB_CTRL1_STASTRE	(1 << 7)
#define	SMB_CTRL1_NMINTE	(1 << 6)
#define	SMB_CTRL1_GCMEN		(1 << 5)
#define	SMB_CTRL1_ACK		(1 << 4)
#define	SMB_CTRL1_RSVD		(1 << 3)
#define	SMB_CTRL1_INTEN		(1 << 2)
#define	SMB_CTRL1_STOP		(1 << 1)
#define	SMB_CTRL1_START		(1 << 0)
#define	SMB_ADDR			0x04
#define	SMB_ADDR_SAEN		(1 << 7)
#define	SMB_CTRL2			0x05
#define	SMB_ENABLE		(1 << 0)
#define	SMB_CTRL3			0x06

/*********************************** LEGACY I/O *******************************/

/*
 * LEGACY I/O SPACE BASE
 */
#define	CS5536_LEGACY_BASE_ADDR		(PCI_IO_BASE_VA | 0x0000)

/*
 * IDE LEGACY REG : legacy IO address is 0x170~0x177 and 0x376 (0x1f0~0x1f7 and 0x3f6)
 * all registers are 16bits except the IDE_LEGACY_DATA reg
 * some registers are read only and the 
 */
#define	PRI_IDE_LEGACY_REG(offset) 	(CS5536_LEGACY_BASE_ADDR | 0x1f0 | offset)
#define	SEC_IDE_LEGACY_REG(offset)	(CS5536_LEGACY_BASE_ADDR | 0x170 | offset)

#define	IDE_LEGACY_DATA		0x00 // RW
#define	IDE_LEGACY_ERROR	0x01 // RO
#define	IDE_LEGACY_FEATURE	0x01 // WO
#define	IDE_LEGACY_SECTOR_COUNT	0x02 // RW
#define	IDE_LEGACY_SECTOR_NUM	0x03 // RW
#define	IDE_LEGACY_CYL_LO	0x04 // RW
#define	IDE_LEGACY_CYL_HI	0x05 // RW
#define	IDE_LEGACY_HEAD		0x06 // RW
#define	IDE_LEGACY_HEAD_DRV		(1 << 4)
#define	IDE_LEGACY_HEAD_LBA		(1 << 6)
#define	IDE_LEGACY_HEAD_IBM		(1 << 7 | 1 << 5)
#define	IDE_LEGACY_STATUS	0x07 // RO
#define IDE_LEGACY_STATUS_ERR		(1 << 0)
#define	IDE_LEGACY_STATUS_IDX		(1 << 1)
#define IDE_LEGACY_STATUS_CORR		(1 << 2)
#define	IDE_LEGACY_STATUS_DRQ		(1 << 3)
#define	IDE_LEGACY_STATUS_DSC 		(1 << 4)
#define	IDE_LEGACY_STATUS_DWF		(1 << 5)
#define	IDE_LEGACY_STATUS_DRDY		(1 << 6)
#define	IDE_LEGACY_STATUS_BUSY		(1 << 7)
#define	IDE_LEGACY_COMMAND	0x07 // WO
#define	IDE_LEGACY_ASTATUS	0x206 // RO
#define	IDE_LEGACY_CTRL		0x206 // WO
#define	IDE_LEGACY_CTRL_IDS	0x02
#define	IDE_LEGACY_CTRL_RST	0x04
#define	IDE_LEGACY_CTRL_4BIT	0x08

/**********************************************************************************/

#endif	/* _CS5536_H */
