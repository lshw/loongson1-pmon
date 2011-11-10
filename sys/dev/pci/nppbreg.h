/*	$Id: nppbreg.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Per Fogelstrom, Opsycon AB  (www.opsycon.se)
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
 *	Per Fogelstrom, Opsycon AB, Sweden.
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

#ifndef	_NPPBREG_H_
#define	_NPPBREG_H_

#define	BAR_ENABLE	0x80000000	/* BAR Enable bit */
#define PRIM_LOCKOUT	(1 << 10)	/* Primary bus lockout */

#if 0
#define LOOKUP_TBL_SIZE	64
#define BAR_MASK		(0xFFFFFFC0)
#define CSR_SIZE		0x1000

/* For use with dec21554BusToLocal, dec21554LocalToBus translation calls */
#define CPCI_MEM_SPACE		0	/* PCI Memory Space */
#define CPCI_IO_SPACE		1	/* PCI I/O Space  */
#define CPCI_PF_MEM_SPACE	8	/* PCI Prefetchable Memory Space */
#define CPCI_CNFG_SPACE		3	/* PCI Configuration Space */

/* Compact PCI interrupt level definitions */
#define CPCI_INTA		0	/* INTA interrupt */
#define CPCI_INTB		1	/* INTB interrupt */
#define CPCI_INTC		2	/* INTC interrupt */
#define CPCI_INTD		3	/* INTD interrupt */

/* Primary/Secondary IRQ Registers */
#define IRQ_CLR_BASE		0x98
#define IRQ_SET_BASE		0x9C
#define IRQ_CLR_MASK_BASE	0xA0
#define IRQ_SET_MASK_BASE	0xA4

#define IRQ_CLR_PRIMARY1	(IRQ_CLR_BASE)
#define IRQ_CLR_PRIMARY2	(IRQ_CLR_BASE + 1)
#define IRQ_CLR_SECONDARY1	(IRQ_CLR_BASE + 2)
#define IRQ_CLR_SECONDARY2	(IRQ_CLR_BASE + 3)

#define IRQ_SET_PRIMARY1	(IRQ_SET_BASE)
#define IRQ_SET_PRIMARY2	(IRQ_SET_BASE + 1)
#define IRQ_SET_SECONDARY1	(IRQ_SET_BASE + 2)
#define IRQ_SET_SECONDARY2	(IRQ_SET_BASE + 3)

#define IRQ_CLR_MASK_PRIMARY1	(IRQ_CLR_MASK_BASE)
#define IRQ_CLR_MASK_PRIMARY2	(IRQ_CLR_MASK_BASE + 1)
#define IRQ_CLR_MASK_SECONDARY1	(IRQ_CLR_MASK_BASE + 2)
#define IRQ_CLR_MASK_SECONDARY2	(IRQ_CLR_MASK_BASE + 3)

#define IRQ_SET_MASK_PRIMARY1	(IRQ_SET_MASK_BASE)
#define IRQ_SET_MASK_PRIMARY2	(IRQ_SET_MASK_BASE + 1)
#define IRQ_SET_MASK_SECONDARY1	(IRQ_SET_MASK_BASE + 2)
#define IRQ_SET_MASK_SECONDARY2	(IRQ_SET_MASK_BASE + 3)
#endif



/*
 * DEC21554 Register definitions
 */

/* CSR Address Map */
struct dec21554csr {
    u_int32_t down_addr;	/* 00 Downstream Configuration Address */
    u_int32_t down_data;	/* 04 Downstream Configuration Data */
    u_int32_t up_addr;		/* 08 Upstream Configuration Address */
    u_int32_t up_data;		/* 0c Upstream Configuration Data */
    u_int32_t cnfg_own;		/* 10 Configuration Control/Own Bits */
    u_int32_t down_io_addr;	/* 14 Downstream I/O Address */
    u_int32_t down_io_data;	/* 18 Downstream I/O Data */
    u_int32_t up_io_addr;	/* 1c Downstream I/O Address */
    u_int32_t up_io_data;	/* 20 Downstream I/O Data */
    u_int32_t io_own;		/* 24 I/O Control/Status/Own Bits */
    u_int32_t tbl_offset;	/* 28 Lookup Table Offset */
    u_int32_t tbl_data;		/* 2c Lookup Table Data */
    u_int32_t i2o_ops;		/* 30 I2O Outbound Post_List Status */
    u_int32_t i2o_opim;		/* 34 I2O Outbound Post_List Interrupt Mask */
    u_int32_t i2o_ips;		/* 38 I2O Inbound Post_List Status */
    u_int32_t i2o_ipim;		/* 3c I2O Inbound Post_List Interrupt Mask */
    u_int32_t i2o_iq;		/* 40 I2O Inbound Queue */
    u_int32_t i2o_oq;		/* 44 I2O Outbound Queue */
    u_int32_t i2o_ifhp;		/* 48 I2O Inbound Free_List Head Pointer */
    u_int32_t i2o_iphp;		/* 4c I2O Inbound Post_List Head Pointer */
    u_int32_t i2o_oftp;		/* 50 I2O Outbound Free_List Tail Pointer */
    u_int32_t i2o_ophp;		/* 54 I2O Outbound Post_List Head Pointer */
    u_int32_t i2o_ipc;		/* 58 I2O Inbound Post_List Counter */
    u_int32_t i2o_ifc;		/* 5c I2O Inbound Free_List Counter */
    u_int32_t i2o_opc;		/* 60 I2O Outbound Post_List Counter */
    u_int32_t i2o_ofc;		/* 64 I2O Outbound Free_List Counter */
    u_int32_t down_mem0_base;	/* 68 Downstream Memory 0 Translated Base */
    u_int32_t down_mem1_base;	/* 6c Downstream IO/Memory 1 Translated Base */
    u_int32_t down_mem2_base;	/* 70 Downstream Memory 2 Translated Base */
    u_int32_t down_mem3_base;	/* 74 Downstream Memory 3 Translated Base */
    u_int32_t up_mem0_base;	/* 78 Upstream IO/Memory 0 Translated Base */
    u_int32_t up_mem1_base;	/* 7c Upstream Memory 1 Translated Base */
    u_int32_t chip_status;	/* 80 Chip Status CSR */
    u_int32_t chip_irq_msk;	/* 84 Chip Clear/Set IRQ Mask */
    u_int32_t up_pb_irq0;	/* 88 Upstream Page Boundary IRQ 0 */
    u_int32_t up_pb_irq1;	/* 8c Upstream Page Boundary IRQ 1 */
    u_int32_t up_pb_mask0;	/* 90 Upstream Page Boundary IRQ Mask 0 */
    u_int32_t up_pb_mask1;	/* 94 Upstream Page Boundary IRQ Mask 1 */
    u_int32_t clear_irq;	/* 98 Primary/Secondary Clear IRQ */
    u_int32_t set_irq;		/* 9c Primary/Secondary Set IRQ */
    u_int32_t clear_mask;	/* a0 Primary/Secondary Clear IRQ mask */
    u_int32_t set_mask;		/* a4 Primary/Secondary Set IRQ mask */
    u_int32_t scratchpad0;	/* a8 Scratchpad 0 */
    u_int32_t scratchpad1;	/* ac Scratchpad 1 */
    u_int32_t scratchpad2;	/* b0 Scratchpad 2 */
    u_int32_t scratchpad3;	/* b4 Scratchpad 3 */
    u_int32_t scratchpad4;	/* b8 Scratchpad 4 */
    u_int32_t scratchpad5;	/* bc Scratchpad 5 */
    u_int32_t scratchpad6;	/* c0 Scratchpad 6 */
    u_int32_t scratchpad7;	/* c4 Scratchpad 7 */
    u_int32_t rom_setup;	/* c8 ROM Setup */
    u_int32_t rom_address;	/* cc ROM Address */
    u_int32_t reserved1;	/* d0 Reserved */
    u_int32_t up_tbl[64];	/* d4 Upstream Memory 2 Lookup Table */
    u_int32_t reserved2;	/* d8 Reserved */
};

/* Interface Configuration Space Address Map */
struct dec21554ic {
	u_int32_t devVenID;	/* Device/Vendor ID */
	u_int32_t command;	/* Command/Status */
	u_int32_t revision;	/* Revision/Primary Class Code */
	u_int32_t bist;		/* BiST/CLS/MLT/Headr Type */
	u_int32_t mem_csr;	/* Primary/Secondary CSR Memory BAR */
	u_int32_t io_csr;	/* Primary/Secondary CSR I/O BAR */
	u_int32_t down_mem1_bar;/* Downstream/Upstream I/O or Memory 1/0 BAR */
#define up_mem0_bar down_mem1_bar
	u_int32_t down_mem2_bar;/* Downstream Mem 2/Upstream Mem 1 BAR */
#define up_mem1_bar down_mem2_bar
	u_int32_t down_mem3_bar;/* Downstream Mem 3/Upstream Mem 2 BAR */
#define up_mem2_bar down_mem3_bar
	u_int32_t down_mem3_ubar;/* Downstream Mem 3 BAR upper 32 bits */
	u_int32_t reserved1;	/* Reserved */
	u_int32_t subsystemID;	/* Subsystem/Subsystem Vendor ID */
	u_int32_t rom_exp;	/* ROM expansion Base Address */
	u_int32_t capabilities;	/* Capabilities Pointer */
	u_int32_t reserved2;	/* Reserved */
	u_int32_t int_gnt;	/* Interrupt/Grant Control */
};

/* Device-Specific Configuration Address Map */
struct dec21554ds {
    u_int32_t down_addr;	/* 80 Downstream Configuration Address */
    u_int32_t down_data;	/* 84 Downstream Configuration Data */
    u_int32_t up_addr;		/* 88 Upstream Configuration Address */
    u_int32_t up_data;		/* 8c Upstream Configuration Data */
    u_int32_t cnfg_own;		/* 90 Configuration Control/Own Bits */
    u_int32_t down_mem0_tran;	/* 94 Downstream Memory 0 Translated Base */
    u_int32_t down_mem1_tran;	/* 98 Downstream IO/Memory 1 Translated Base */
    u_int32_t down_mem2_tran;	/* 9c Downstream Memory 2 Translated Base */
    u_int32_t down_mem3_tran;	/* a0 Downstream Memory 3 Translated Base */
    u_int32_t up_mem0_tran;	/* a4 Upstream IO/Memory 0 Translated Base */
    u_int32_t up_mem1_tran;	/* a8 Upstream Memory 1 Translated Base */
    u_int32_t down_mem0_setup;	/* ac Downstream Memory 0 Setup */
    u_int32_t down_mem1_setup;	/* b0 Downstream IO/Memory 1 Setup */
    u_int32_t down_mem2_setup;	/* b4 Downstream Memory 2 Setup */
    u_int32_t down_mem3_setup;	/* b8 Downstream Memory 3 Setup */
    u_int32_t down_upper32;	/* bc Downstream Upper 32 Bits Setup */
    u_int32_t pri_exp_rom;	/* c0 Primary Expansion ROM Setup */
    u_int32_t up_mem0_setup;	/* c4 Upstream IO/Memory 0 Setup */
    u_int32_t up_mem1_setup;	/* c8 Upstream Memory 1 Setup */
    u_int32_t chip_control;	/* cc Chip Control */
    u_int32_t chip_status;	/* d0 Chip Status/Arbiter Control */
    u_int32_t serr_disables;	/* d4 Primary/Secondary SERR# Disables */
    u_int32_t reset_control;	/* d8 Reset Control */
    u_int32_t power_cap;	/* dc Power Management Capabilities */
    u_int32_t power_csr;	/* e0 Power Management CSR */
    u_int32_t vpd_addr;		/* e4 VPD Address */
    u_int32_t vpd_data;		/* e8 VPD Data */
    u_int32_t hot_swap;		/* ec Hot Swap Control */
    u_int32_t reserved;		/* f0 Reserved */
};

struct nppb_cfg {
	volatile struct dec21554ic local;
	volatile struct dec21554ic sub;
	volatile struct dec21554ds ds;
};

#define cfgoffs(x) (u_int32_t)(&((struct nppb_cfg *)0)->x)
#define csroffs(x) (u_int32_t)(&((struct dec21554csr *)0)->x)


__BEGIN_DECLS
pcireg_t	nppb_config_read __P((pcitag_t, int));
int		nppb_config_write __P((pcitag_t, int, pcireg_t));
#ifdef PMON
/* Called to do low level initialisation */
void		_nppb_pmon_init __P((pcitag_t));
pcireg_t	_nppb_config_read __P((pcitag_t, pcitag_t, int));
int		_nppb_config_write __P((pcitag_t, pcitag_t, int, pcireg_t));
#endif
__END_DECLS

#endif /* _NPPBREG_H_ */
