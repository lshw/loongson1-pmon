/*
 * pci_machdep_cs5536.c  
 * 	the Virtual Support Module(VSM) for virtulize the PCI configure  
 * 	space. so user can access the PCI configure space directly as
 *	a normal multi-function PCI device which following the PCI-2.2 spec.
 *
 * Author : jlliu <liujl@lemote.com>
 * Date : 07-07-05
 *
 */
#include <sys/linux/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/nppbreg.h>

#include <machine/bus.h>

#include <include/bonito.h>
#include <include/cs5536.h>
#include <include/cs5536_pci.h>
#include <pmon.h>
#include "cs5536_io.h"

/* FOR INCLUDING THE RX REGISTERS */
#define	SB_RX
// NOTE THE IDE DMA OPERATION, BASE ADDR CONFIG RIGHT OR WRONG???

/************************************************************************/

/*
 * divil_lbar_enable_disable : enable/disable the divil module bar space.
 */
static void divil_lbar_enable_disable(int enable)
{
	u32 hi, lo;
	
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_IRQ), &hi, &lo);
	if(enable)
		hi |= 0x01;
	else
		hi &= ~0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_IRQ), hi, lo);

	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_SMB), &hi, &lo);
	if(enable)
		hi |= 0x01;
	else
		hi &= ~0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_SMB), hi, lo);

	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), &hi, &lo);
	if(enable)
		hi |= 0x01;
	else
		hi &= ~0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), hi, lo);

	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_MFGPT), &hi, &lo);
	if(enable)
		hi |= 0x01;
	else
		hi &= ~0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_MFGPT), hi, lo);

	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_PMS), &hi, &lo);
	if(enable)
		hi |= 0x01;
	else
		hi &= ~0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_PMS), hi, lo);

	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_ACPI), &hi, &lo);
	if(enable)
		hi |= 0x01;
	else
		hi &= ~0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_ACPI), hi, lo);

	return;
}

/******************************************************************************/

/*
 * The following functions are not implemented in  pmon.
 */
static void pci_flash_write_reg(int reg, pcireg_t value)
{
	return;
}

static pcireg_t pci_flash_read_reg(int reg)
{
	return 0xffffffff;
}

/*******************************************************************************/

/*
 * isa_write : isa write transfering.
 * WE assume that the ISA is not the BUS MASTER. 
 */
static void pci_isa_write_reg(int reg, pcireg_t value)
{
	u32 hi, lo;
	u32 temp;
	u32 softcom;

	_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
	softcom = lo;
	
	switch(reg){
		case PCI_COMMAND_STATUS_REG :
			// command
			if( value & PCI_COMMAND_IO_ENABLE ){
				divil_lbar_enable_disable(1);
			}else{
				divil_lbar_enable_disable(0);
			}
			/* BUS MASTER : is it 0 for SB???  yes...*/
			/* PER response enable or disable. */
			if( value & PCI_COMMAND_PARITY_ENABLE ){
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				lo |= SB_PARE_ERR_EN;
				_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);			
			}else{
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				lo &= ~SB_PARE_ERR_EN;
				_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);			
			}
			// status
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			temp = lo & 0x0000ffff;
			if( (value & PCI_STATUS_TARGET_TARGET_ABORT) && 
				(lo & SB_SYSE_ERR_EN) ){
				temp |= SB_SYSE_ERR_FLAG;
			}
			if( (value & PCI_STATUS_MASTER_TARGET_ABORT) &&
				(lo & SB_TAR_ERR_EN) ){
				temp |= SB_TAR_ERR_FLAG;
			}
			if( (value & PCI_STATUS_MASTER_ABORT) &&
				(lo & SB_MAR_ERR_EN) ){
				temp |= SB_MAR_ERR_FLAG;
			}
			if( (value & PCI_STATUS_PARITY_DETECT) &&
				(lo & SB_PARE_ERR_EN) ){
				temp |= SB_PARE_ERR_FLAG; 
			}
			lo = temp;
			_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
			break;
		case PCI_BHLC_REG :
			value &= 0x0000ff00;
			_rdmsr(SB_MSR_REG(SB_CTRL), &hi, &lo);
			hi &= 0xffffff00;
			hi |= (value >> 8);
			_wrmsr(SB_MSR_REG(SB_CTRL), hi, lo);
			break;
		case PCI_BAR0_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_SMB_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if(value & 0x01){
				hi = 0x0000f001;
				lo = value & 0x0000fff8;
				_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_SMB), hi, lo);
#ifdef	SB_RX
				hi = ((value & 0x000ffffc) << 12) | ((CS5536_SMB_LENGTH - 4) << 12) | 0x01;
				//hi = ((value & 0x000fffff) << 12) | ((CS5536_SMB_LENGTH) << 12) | 0x03;
				lo = ((value & 0x000ffffc) << 12) | 0x01;
				_wrmsr(SB_MSR_REG(SB_R0), hi, lo);
#endif				
			}
			break;
		case PCI_BAR1_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_GPIO_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if(value & 0x01){
				hi = 0x0000f001;
				lo = value & 0x0000ff00;
				_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), hi, lo);
#ifdef	SB_RX				
				hi = ((value & 0x000ffffc) << 12) | ((CS5536_GPIO_LENGTH - 4) << 12) | 0x01;
				//hi = ((value & 0x000fffff) << 12) | ((CS5536_GPIO_LENGTH) << 12) | 0x03;
				lo = ((value & 0x000ffffc) << 12) | 0x01;
				_wrmsr(SB_MSR_REG(SB_R1), hi, lo);
#endif				
			}
			break;
		case PCI_BAR2_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_MFGPT_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if(value & 0x01){
				hi = 0x0000f001;
				lo = value & 0x0000ffc0;
				_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_MFGPT), hi, lo);
#ifdef SB_RX				
				hi = ((value & 0x000ffffc) << 12) | ((CS5536_MFGPT_LENGTH - 4) << 12) | 0x01;
				//hi = ((value & 0x000fffff) << 12) | ((CS5536_MFGPT_LENGTH) << 12) | 0x03;
				lo = ((value & 0x000ffffc) << 12) | 0x01;
				_wrmsr(SB_MSR_REG(SB_R2), hi, lo);
#endif				
			}
			break;
		case PCI_BAR3_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_IRQ_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}
			if(value & 0x01){
				hi = 0x0000f001;
				lo = value & 0x0000ffe0;
				_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_IRQ), hi, lo);
#ifdef	SB_RX				
				hi = ((value & 0x000ffffc) << 12) | ((CS5536_IRQ_LENGTH - 4) << 12) | 0x01;
				//hi = ((value & 0x000fffff) << 12) | ((CS5536_IRQ_LENGTH) << 12) | 0x03;
				lo = ((value & 0x000ffffc) << 12) | 0x01;
				_wrmsr(SB_MSR_REG(SB_R3), hi, lo);
#endif				
			}
			break;
		case PCI_BAR4_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_PMS_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if(value & 0x01){
				hi = 0x0000f001;
				lo = value & 0x0000ff80;
				_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_PMS), hi, lo);
#ifdef 	SB_RX				
				hi = ((value & 0x000ffffc) << 12) | ((CS5536_PMS_LENGTH - 4) << 12) | 0x01;
				//hi = ((value & 0x000fffff) << 12) | ((CS5536_PMS_LENGTH) << 12) | 0x03;
				lo = ((value & 0x000ffffc) << 12) | 0x01;
				_wrmsr(SB_MSR_REG(SB_R4), hi, lo);
#endif				
			}
			break;
		case PCI_BAR5_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_ACPI_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if(value & 0x01){
				hi = 0x0000f001;
				lo = value & 0x0000ffe0;
				_wrmsr(DIVIL_MSR_REG(DIVIL_LBAR_ACPI), hi, lo);
#ifdef 	SB_RX				
				hi = ((value & 0x000ffffc) << 12) | ((CS5536_ACPI_LENGTH - 4) << 12) | 0x01;
				//hi = ((value & 0x000fffff) << 12) | ((CS5536_ACPI_LENGTH) << 12) | 0x03;
				lo = ((value & 0x000ffffc) << 12) | 0x01;
				_wrmsr(SB_MSR_REG(SB_R5), hi, lo);
#endif				
			}
			break;
		default :
			break;			
	}
	
	return;
}

/*
 * isa_read : isa read transfering.
 * we assume that the ISA is not the BUS MASTER. 
 */
static pcireg_t pci_isa_read_reg(int reg)
{
	pcireg_t conf_data;
	u32 hi, lo;
	
	switch(reg){
		case PCI_ID_REG :
			conf_data = 0;
			_rdmsr(SB_MSR_REG(SB_CAP), &hi, &lo);	/* jlliu : should get the correct value. */
			if( (lo != 0x0) && (lo != 0xffffffff) ){
				conf_data = (CS5536_ISA_DEVICE_ID << 16 | CS5536_VENDOR_ID);
			}
			break;
		case PCI_COMMAND_STATUS_REG :
			conf_data = 0;
			// COMMAND			
			_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_SMB), &hi, &lo);
			if(hi & 0x01){
				conf_data |= PCI_COMMAND_IO_ENABLE;
			}
			conf_data |= PCI_COMMAND_SPECIAL_ENABLE;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_PARE_ERR_EN){
				conf_data |= PCI_COMMAND_PARITY_ENABLE;
			}else{
				conf_data &= ~PCI_COMMAND_PARITY_ENABLE;
			}
			// STATUS	
			conf_data |= PCI_STATUS_66MHZ_SUPPORT;
			conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
			conf_data |= PCI_STATUS_BACKTOBACK_SUPPORT;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_SYSE_ERR_FLAG)
				conf_data |= PCI_STATUS_TARGET_TARGET_ABORT;
			if(lo & SB_TAR_ERR_FLAG)
				conf_data |= PCI_STATUS_MASTER_TARGET_ABORT;
			if(lo & SB_MAR_ERR_FLAG)
				conf_data |= PCI_STATUS_MASTER_ABORT;
			if(lo & SB_PARE_ERR_FLAG)
				conf_data |= PCI_STATUS_PARITY_DETECT;
			break;
		case PCI_CLASS_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_CHIP_REV_ID), &hi, &lo);
			conf_data = lo & 0x000000ff;
			conf_data |= (CS5536_ISA_CLASS_CODE << 8);
			break;
		case PCI_BHLC_REG :
			_rdmsr(SB_MSR_REG(SB_CTRL), &hi, &lo);
			hi &= 0x000000f8;
			conf_data = (PCI_NONE_BIST << 24) | (PCI_BRIDGE_HEADER_TYPE << 16) |
				(hi << 8) | PCI_NORMAL_CACHE_LINE_SIZE;
			break;
		/*
		 * we only use the LBAR of DIVIL, no RCONF used. 
		 * all of them are IO space.
		 */
		case PCI_BAR0_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_SMB_FLAG){
				conf_data = CS5536_SMB_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_SMB_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_SMB), &hi, &lo);
				conf_data = lo & 0x0000ffff;
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;
		case PCI_BAR1_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_GPIO_FLAG){
				conf_data = CS5536_GPIO_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_GPIO_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_GPIO), &hi, &lo);
				conf_data = lo & 0x0000ffff;
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;
		case PCI_BAR2_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_MFGPT_FLAG){
				conf_data = CS5536_MFGPT_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_MFGPT_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_MFGPT), &hi, &lo);
				conf_data = lo & 0x0000ffff;
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;
		case PCI_BAR3_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_IRQ_FLAG){
				conf_data = CS5536_IRQ_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_IRQ_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_IRQ), &hi, &lo);
				conf_data = lo & 0x0000ffff;
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;	
		case PCI_BAR4_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_PMS_FLAG){
				conf_data = CS5536_PMS_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_PMS_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_PMS), &hi, &lo);
				conf_data = lo & 0x0000ffff;
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;
		case PCI_BAR5_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_ACPI_FLAG){
				conf_data = CS5536_ACPI_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_ACPI_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_ACPI), &hi, &lo);
				conf_data = lo & 0x0000ffff;
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;
		case PCI_CARDBUS_CIS_REG :
			conf_data = PCI_CARDBUS_CIS_POINTER;
			break;
		case PCI_SUBSYS_ID_REG :
			conf_data = (CS5536_ISA_SUB_ID << 16) | CS5536_SUB_VENDOR_ID;
			break;
		case PCI_MAPREG_ROM :
			conf_data = PCI_EXPANSION_ROM_BAR;
			break;
		case PCI_CAPLISTPTR_REG :
			conf_data = PCI_CAPLIST_POINTER;
			break;
		case PCI_INTERRUPT_REG :
			conf_data = (PCI_MAX_LATENCY << 24) | (PCI_MIN_GRANT << 16) | 
				(PCI_INTERRUPT_PIN_NONE << 8) | 0x00;
			break;
		default :
			conf_data = 0;
			break;
	}

	return conf_data;
}

/*
 * ide_write : ide write transfering
 */
static void pci_ide_write_reg(int reg, pcireg_t value)
{
	u32 hi, lo;
	
	switch(reg){
		case PCI_COMMAND_STATUS_REG :
			// COMMAND
			if(value & PCI_COMMAND_MASTER_ENABLE){
				_rdmsr(GLIU_MSR_REG(GLIU_PAE), &hi, &lo);
				lo |= (0x03 << 4);
				_wrmsr(GLIU_MSR_REG(GLIU_PAE), hi, lo);			
			}else{
				_rdmsr(GLIU_MSR_REG(GLIU_PAE), &hi, &lo);
				lo &= ~(0x03 << 4);
				_wrmsr(GLIU_MSR_REG(GLIU_PAE), hi, lo);	
			}
			// STATUS
			if(value & PCI_STATUS_PARITY_ERROR){
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				if(lo & SB_PARE_ERR_FLAG){
					lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
					_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
				}				
			}
			break;
		case PCI_BHLC_REG :
			value &= 0x0000ff00;
			_rdmsr(SB_MSR_REG(SB_CTRL), &hi, &lo);
			hi &= 0xffffff00;
			hi |= (value >> 8);
			_wrmsr(SB_MSR_REG(SB_CTRL), hi, lo);
			break;
		case PCI_BAR4_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_IDE_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if(value & 0x01){
				hi = 0x00000000;
				//lo = ((value & 0x0fffffff) << 4) | 0x001;
				lo = (value & 0xfffffff0) | 0x1;
				_wrmsr(IDE_MSR_REG(IDE_IO_BAR), hi, lo);
#ifdef SB_RX				
				hi = 0x60000000 | ((value & 0x000ff000) >> 12);
				lo = 0x000ffff0 | ((value & 0x00000ffc) << 20);
				_wrmsr(GLIU_MSR_REG(GLIU_IOD_BM2), hi, lo);
#endif				
			}
			break;
		case PCI_IDE_CFG_REG :
			if(value == CS5536_IDE_FLASH_SIGNATURE){
				_rdmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), &hi, &lo);
				lo |= 0x01;
				_wrmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), hi, lo);
			}else{
				hi = 0;
				lo = value;
				_wrmsr(IDE_MSR_REG(IDE_CFG), hi, lo);			
			}
			break;
		case PCI_IDE_DTC_REG :
			hi = 0;
			lo = value;
			_wrmsr(IDE_MSR_REG(IDE_DTC), hi, lo);
			break;
		case PCI_IDE_CAST_REG :
			hi = 0;
			lo = value;
			_wrmsr(IDE_MSR_REG(IDE_CAST), hi, lo);
			break;
		case PCI_IDE_ETC_REG :
			hi = 0;
			lo = value;
			_wrmsr(IDE_MSR_REG(IDE_ETC), hi, lo);
			break;
		case PCI_IDE_PM_REG :
			hi = 0;
			lo = value;
			_wrmsr(IDE_MSR_REG(IDE_INTERNAL_PM), hi, lo);
			break;
		default :
			break;			
	}
	
	return;
}

/*
 * ide_read : ide read tranfering.
 */
static pcireg_t pci_ide_read_reg(int reg)
{
	pcireg_t conf_data;
	u32 hi, lo;
	
	switch(reg){
		case PCI_ID_REG :
			conf_data = 0;
			_rdmsr(IDE_MSR_REG(IDE_CAP), &hi, &lo);
			if( (lo != 0x0) && (lo != 0xffffffff) ){
				conf_data = (CS5536_IDE_DEVICE_ID << 16 | CS5536_VENDOR_ID);
			}
			break;
		case PCI_COMMAND_STATUS_REG :
			conf_data = 0;
			// COMMAND
			_rdmsr(IDE_MSR_REG(IDE_IO_BAR), &hi, &lo);
			if(lo & 0xfffffff0)
				conf_data |= PCI_COMMAND_IO_ENABLE;
			_rdmsr(GLIU_MSR_REG(GLIU_PAE), &hi, &lo);
			if( (lo & 0x30) == 0x30 )
				conf_data |= PCI_COMMAND_MASTER_ENABLE;
			/* conf_data |= PCI_COMMAND_BACKTOBACK_ENABLE??? HOW TO GET..*/
			//STATUS
			conf_data |= PCI_STATUS_66MHZ_SUPPORT;
			conf_data |= PCI_STATUS_BACKTOBACK_SUPPORT;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_PARE_ERR_FLAG)
				conf_data |= PCI_STATUS_PARITY_ERROR;
			conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
			break;
		case PCI_CLASS_REG :
			_rdmsr(IDE_MSR_REG(IDE_CAP), &hi, &lo);
			conf_data = lo & 0x000000ff;
			conf_data |= (CS5536_IDE_CLASS_CODE << 8);
			break;
		case PCI_BHLC_REG :
			_rdmsr(SB_MSR_REG(SB_CTRL), &hi, &lo);
			hi &= 0x000000f8;
			conf_data = (PCI_NONE_BIST << 24) | (PCI_NORMAL_HEADER_TYPE << 16) |
				(hi << 8) | PCI_NORMAL_CACHE_LINE_SIZE;
			break;
		case PCI_BAR0_REG :
			conf_data = 0x00000000;
			break;
		case PCI_BAR1_REG :
			conf_data = 0x00000000;
			break;
		case PCI_BAR2_REG :
			conf_data = 0x00000000;
			break;
		case PCI_BAR3_REG :
			conf_data = 0x00000000;
			break;
		case PCI_BAR4_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_IDE_FLAG){
				conf_data = CS5536_IDE_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_IDE_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(IDE_MSR_REG(IDE_IO_BAR), &hi, &lo);
				//conf_data = lo >> 4;
				conf_data = lo & 0xfffffff0;
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;
		case PCI_BAR5_REG :
			conf_data = 0x00000000;
			break;
		case PCI_CARDBUS_CIS_REG :
			conf_data = PCI_CARDBUS_CIS_POINTER;
			break;
		case PCI_SUBSYS_ID_REG :
			conf_data = (CS5536_IDE_SUB_ID << 16) | CS5536_SUB_VENDOR_ID;
			break;
		case PCI_MAPREG_ROM :
			conf_data = PCI_EXPANSION_ROM_BAR;
			break;
		case PCI_CAPLISTPTR_REG :
			conf_data = PCI_CAPLIST_POINTER;
			break;
		case PCI_INTERRUPT_REG :
			conf_data = (PCI_MAX_LATENCY << 24) | (PCI_MIN_GRANT << 16) | 
				(PCI_INTERRUPT_PIN_NONE << 8) | 0x00;
			break;
		case PCI_IDE_CFG_REG :
			_rdmsr(IDE_MSR_REG(IDE_CFG), &hi, &lo);
			conf_data = lo;
			break;
		case PCI_IDE_DTC_REG :
			_rdmsr(IDE_MSR_REG(IDE_DTC), &hi, &lo);
			conf_data = lo;
			break;
		case PCI_IDE_CAST_REG :
			_rdmsr(IDE_MSR_REG(IDE_CAST), &hi, &lo);
			conf_data = lo;
			break;
		case PCI_IDE_ETC_REG :
			_rdmsr(IDE_MSR_REG(IDE_ETC), &hi, &lo);
			conf_data = lo;
		case PCI_IDE_PM_REG :
			_rdmsr(IDE_MSR_REG(IDE_INTERNAL_PM), &hi, &lo);
			conf_data = lo;
			break;
			
		default :
			conf_data = 0;
			break;
	}

	return conf_data;
}

static void pci_acc_write_reg(int reg, u32 value)
{
	u32 hi, lo;

	switch(reg){
		case PCI_COMMAND_STATUS_REG :
			// COMMAND
			if(value & PCI_COMMAND_MASTER_ENABLE){
				_rdmsr(GLIU_MSR_REG(GLIU_PAE), &hi, &lo);
				lo |= (0x03 << 8);
				_wrmsr(GLIU_MSR_REG(GLIU_PAE), hi, lo);			
			}else{
				_rdmsr(GLIU_MSR_REG(GLIU_PAE), &hi, &lo);
				lo &= ~(0x03 << 8);
				_wrmsr(GLIU_MSR_REG(GLIU_PAE), hi, lo);	
			}
			// STATUS
			if(value & PCI_STATUS_PARITY_ERROR){
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				if(lo & SB_PARE_ERR_FLAG){
					lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
					_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
				}				
			}
			break;
		case PCI_BAR0_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_ACC_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if( value & 0x01 ){
				value &= 0xfffffffc;
				hi = 0xA0000000 | ((value & 0x000ff000) >> 12);
				lo = 0x000fff80 | ((value & 0x00000fff) << 20);
				_wrmsr(GLIU_MSR_REG(GLIU_IOD_BM1), hi, lo);
			}
			break;
		default :
			break;			
	}

	return;
}

static u32 pci_acc_read_reg(int reg)
{
	u32 hi, lo;
	u32 conf_data;

	switch(reg){
		case PCI_ID_REG :
			conf_data = (CS5536_ACC_DEVICE_ID << 16 | CS5536_VENDOR_ID);
			break;
		case PCI_COMMAND_STATUS_REG :
			
			conf_data = 0;
			// COMMAND
			_rdmsr(GLIU_MSR_REG(GLIU_IOD_BM1), &hi, &lo);
			if( ( (lo & 0xfff00000) || (hi & 0x000000ff) ) 
					&& ((hi & 0xf0000000) == 0xa0000000) )
				conf_data |= PCI_COMMAND_IO_ENABLE;
			_rdmsr(GLIU_MSR_REG(GLIU_PAE), &hi, &lo);
			if( (lo & 0x300) == 0x300 )
				conf_data |= PCI_COMMAND_MASTER_ENABLE;
			/* conf_data |= PCI_COMMAND_BACKTOBACK_ENABLE??? HOW TO GET..*/
			//STATUS
			conf_data |= PCI_STATUS_66MHZ_SUPPORT;
			conf_data |= PCI_STATUS_BACKTOBACK_SUPPORT;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_PARE_ERR_FLAG)
				conf_data |= PCI_STATUS_PARITY_ERROR;
			conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
			break;
		case PCI_CLASS_REG :
			_rdmsr(ACC_MSR_REG(ACC_CAP), &hi, &lo);
			conf_data = lo & 0x000000ff;
			conf_data |= (CS5536_ACC_CLASS_CODE << 8);
			break;
		case PCI_BHLC_REG :
			conf_data = (PCI_NONE_BIST << 24) | (PCI_NORMAL_HEADER_TYPE << 16) |
				(PCI_NORMAL_LATENCY_TIMER << 8) | PCI_NORMAL_CACHE_LINE_SIZE;
			break;
		case PCI_BAR0_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_ACC_FLAG){
				conf_data = CS5536_ACC_RANGE | PCI_MAPREG_TYPE_IO;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_ACC_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(GLIU_MSR_REG(GLIU_IOD_BM1), &hi, &lo);
				conf_data = (hi & 0x000000ff) << 12;
				conf_data |= (lo & 0xfff00000) >> 20; 
				conf_data |= 0x01;
				conf_data &= ~0x02;
			}
			break;
		case PCI_BAR1_REG :
			conf_data = 0x000000;
			break;		
		case PCI_BAR2_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR3_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR4_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR5_REG :
			conf_data = 0x000000;
			break;
		case PCI_CARDBUS_CIS_REG :
			conf_data = PCI_CARDBUS_CIS_POINTER;
			break;
		case PCI_SUBSYS_ID_REG :
			conf_data = (CS5536_ACC_SUB_ID << 16) | CS5536_SUB_VENDOR_ID;
			break;
		case PCI_MAPREG_ROM :
			conf_data = PCI_EXPANSION_ROM_BAR;
			break;
		case PCI_CAPLISTPTR_REG :
			conf_data = PCI_CAPLIST_USB_POINTER;
			break;
		case PCI_INTERRUPT_REG :
			conf_data = (PCI_MAX_LATENCY << 24) | (PCI_MIN_GRANT << 16) | 
				(0x01 << 8) | (0x00);

			break;
		default :
			conf_data = 0;
			break;
	}

	return conf_data;
}

/*
 * ohci_write : ohci write tranfering.
 */
static void pci_ohci_write_reg(int reg, pcireg_t value)
{
	u32 hi, lo;
	
	switch(reg){
		case PCI_COMMAND_STATUS_REG :
			// COMMAND
			if(value & PCI_COMMAND_MASTER_ENABLE){
				_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
				hi |= (1 << 2);
				_wrmsr(USB_MSR_REG(USB_OHCI), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
				hi &= ~(1 << 2);
				_wrmsr(USB_MSR_REG(USB_OHCI), hi, lo);
			}
			if(value & PCI_COMMAND_MEM_ENABLE){
				_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
				hi |= (1 << 1);
				_wrmsr(USB_MSR_REG(USB_OHCI), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
				hi &= ~(1 << 1);
				_wrmsr(USB_MSR_REG(USB_OHCI), hi, lo);				
			}
			// STATUS
			if(value & PCI_STATUS_PARITY_ERROR){
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				if(lo & SB_PARE_ERR_FLAG){
					lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
					_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
				}				
			}
			break;
		case PCI_BAR0_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_OHCI_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if( (value & 0x01) == 0x00 ){
				_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
				//lo = (value & 0xffffff00) << 8;
				lo = value;
				_wrmsr(USB_MSR_REG(USB_OHCI), hi, lo);
				
				hi = 0x40000000 | (value&0xff000000) >> 24;
				lo = 0x000fffff | (value&0x00fff000) << 8;
				_wrmsr(GLIU_MSR_REG(GLIU_P2D_BM3), hi, lo);
			}
			break;
		case PCI_INTERRUPT_REG :
			value &= 0x000000ff;
			break;
		case PCI_USB_PM_REG :
			break;
		default :
			break;			
	}
	
	return;
}

/*
 * ohci_read : ohci read transfering.
 */
static pcireg_t pci_ohci_read_reg(int reg)
{
	pcireg_t conf_data;
	u32 hi, lo;
	
	switch(reg){
		case PCI_ID_REG :
			conf_data = 0;
			_rdmsr(USB_MSR_REG(USB_CAP), &hi, &lo);
			if( (lo != 0x0) && (lo != 0xffffffff) ){
				conf_data = (CS5536_OHCI_DEVICE_ID << 16 | CS5536_VENDOR_ID);
			}
			break;
		case PCI_COMMAND_STATUS_REG :
			conf_data = 0;
			// COMMAND
			_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
			if(hi & 0x04)
				conf_data |= PCI_COMMAND_MASTER_ENABLE;
			if(hi & 0x02)
				conf_data |= PCI_COMMAND_MEM_ENABLE;
			// STATUS
			conf_data |= PCI_STATUS_66MHZ_SUPPORT;
			conf_data |= PCI_STATUS_BACKTOBACK_SUPPORT;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_PARE_ERR_FLAG)
				conf_data |= PCI_STATUS_PARITY_ERROR;
			conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
			break;
		case PCI_CLASS_REG :
			_rdmsr(USB_MSR_REG(USB_CAP), &hi, &lo);
			conf_data = lo & 0x000000ff;
			conf_data |= (CS5536_OHCI_CLASS_CODE << 8);
			break;
		case PCI_BHLC_REG :
			conf_data = (PCI_NONE_BIST << 24) | (PCI_NORMAL_HEADER_TYPE << 16) |
				(0x00 << 8) | PCI_NORMAL_CACHE_LINE_SIZE;
			break;
		case PCI_BAR0_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_OHCI_FLAG){
				conf_data = CS5536_OHCI_RANGE | PCI_MAPREG_TYPE_MEM;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_OHCI_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
				//conf_data = lo >> 8;
				conf_data = lo & 0xffffff00;
				conf_data &= ~0x0000000f; // 32bit mem
			}
			break;
		case PCI_BAR1_REG :
			conf_data = 0x000000;
			break;		
		case PCI_BAR2_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR3_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR4_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR5_REG :
			conf_data = 0x000000;
			break;
		case PCI_CARDBUS_CIS_REG :
			conf_data = PCI_CARDBUS_CIS_POINTER;
			break;
		case PCI_SUBSYS_ID_REG :
			conf_data = (CS5536_OHCI_SUB_ID << 16) | CS5536_SUB_VENDOR_ID;
			break;
		case PCI_MAPREG_ROM :
			conf_data = PCI_EXPANSION_ROM_BAR;
			break;
		case PCI_CAPLISTPTR_REG :
			conf_data = PCI_CAPLIST_USB_POINTER;
			break;
		case PCI_INTERRUPT_REG :
			conf_data = (PCI_MAX_LATENCY << 24) | (PCI_MIN_GRANT << 16) | 
				(PCI_INTERRUPT_PIN_D << 8) | 0x00;
			break;
		case PCI_USB_PM_REG :
			conf_data = 0;
			break;
		case 0x50 :
			_rdmsr(GLIU_MSR_REG(GLIU_P2D_BM1), &hi, &lo);
			//printf("p2d bm1 : hi = 0x%x, lo = 0x%x\n", hi, lo);
			_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
			//printf("usb ohci : hi = 0x%x, lo = 0x%x\n", hi, lo);
			conf_data = 0;
			break;		
		default :
			conf_data = 0;
			break;
	}

	return conf_data;
}

#ifdef	TEST_CS5536_USE_EHCI
static void pci_ehci_write_reg(int reg, u32 value)
{
	u32 hi, lo;
	
	switch(reg){
		case PCI_COMMAND_STATUS_REG :
			// COMMAND
			if(value & PCI_COMMAND_MASTER_ENABLE){
				_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
				hi |= (1 << 2);
				_wrmsr(USB_MSR_REG(USB_EHCI), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
				hi &= ~(1 << 2);
				_wrmsr(USB_MSR_REG(USB_EHCI), hi, lo);
			}
			if(value & PCI_COMMAND_MEM_ENABLE){
				_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
				hi |= (1 << 1);
				_wrmsr(USB_MSR_REG(USB_EHCI), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
				hi &= ~(1 << 1);
				_wrmsr(USB_MSR_REG(USB_EHCI), hi, lo);				
			}
			// STATUS
			if(value & PCI_STATUS_PARITY_ERROR){
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				if(lo & SB_PARE_ERR_FLAG){
					lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
					_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
				}				
			}
			break;
		case PCI_BAR0_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_EHCI_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if( (value & 0x01) == 0x00 ){
				_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
				lo = value;
				_wrmsr(USB_MSR_REG(USB_EHCI), hi, lo);
				
				value &= 0xfffffff0;
				hi = 0x40000000 | ((value & 0xff000000) >> 24);
				lo = 0x000fffff | ((value & 0x00fff000) << 8);
				_wrmsr(GLIU_MSR_REG(GLIU_P2D_BM4), hi, lo);
			}
			break;
		case PCI_EHCI_LEGSMIEN_REG :
			_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
			hi &= 0x003f0000;
			hi |= (value & 0x3f) << 16;
			_wrmsr(USB_MSR_REG(USB_EHCI), hi, lo);
			break;
		case PCI_EHCI_FLADJ_REG :
			_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
			hi &= ~0x00003f00;
			hi |= value & 0x00003f00;
			_wrmsr(USB_MSR_REG(USB_EHCI), hi, lo);
			break;
		default :
			break;			
	}
	
	return;
}

static u32 pci_ehci_read_reg(int reg)
{
	u32 conf_data;
	u32 hi, lo;
	
	switch(reg){
		case PCI_ID_REG :
			conf_data = (CS5536_EHCI_DEVICE_ID << 16 | CS5536_VENDOR_ID);
			break;
		case PCI_COMMAND_STATUS_REG :
			conf_data = 0;
			// COMMAND
			_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
			if(hi & 0x04)
				conf_data |= PCI_COMMAND_MASTER_ENABLE;
			if(hi & 0x02)
				conf_data |= PCI_COMMAND_MEM_ENABLE;
			// STATUS
			conf_data |= PCI_STATUS_66MHZ_SUPPORT;
			conf_data |= PCI_STATUS_BACKTOBACK_SUPPORT;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_PARE_ERR_FLAG)
				conf_data |= PCI_STATUS_PARITY_ERROR;
			conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
			break;
		case PCI_CLASS_REG :
			_rdmsr(USB_MSR_REG(USB_CAP), &hi, &lo);
			conf_data = lo & 0x000000ff;
			conf_data |= (CS5536_EHCI_CLASS_CODE << 8);
			break;
		case PCI_BHLC_REG :
			conf_data = (PCI_NONE_BIST << 24) | (PCI_NORMAL_HEADER_TYPE << 16) |
				(PCI_NORMAL_LATENCY_TIMER << 8) | PCI_NORMAL_CACHE_LINE_SIZE;
			break;
		case PCI_BAR0_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_EHCI_FLAG){
				conf_data = CS5536_EHCI_RANGE | PCI_MAPREG_TYPE_MEM;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_EHCI_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
				conf_data = lo & 0xffffff00;
				conf_data &= ~0x0000000f; // 32bit mem
			}
			break;
		case PCI_BAR1_REG :
			conf_data = 0x000000;
			break;		
		case PCI_BAR2_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR3_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR4_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR5_REG :
			conf_data = 0x000000;
			break;
		case PCI_CARDBUS_CIS_REG :
			conf_data = PCI_CARDBUS_CIS_POINTER;
			break;
		case PCI_SUBSYS_ID_REG :
			conf_data = (CS5536_EHCI_SUB_ID << 16) | CS5536_SUB_VENDOR_ID;
			break;
		case PCI_MAPREG_ROM :
			conf_data = PCI_EXPANSION_ROM_BAR;
			break;
		case PCI_CAPLISTPTR_REG :
			conf_data = PCI_CAPLIST_USB_POINTER;
			break;
		case PCI_INTERRUPT_REG :
			conf_data = (PCI_MAX_LATENCY << 24) | (PCI_MIN_GRANT << 16) | 
				(0x01 << 8) | 0x00;
			break;
		case PCI_EHCI_LEGSMIEN_REG :
			_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
			conf_data = (hi & 0x003f0000) >> 16;
			break;
		case PCI_EHCI_LEGSMISTS_REG :
			_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
			conf_data = (hi & 0x3f000000) >> 24;
			break;
		case PCI_EHCI_FLADJ_REG :
			_rdmsr(USB_MSR_REG(USB_EHCI), &hi, &lo);
			conf_data = hi & 0x00003f00;
			break;
		default :
			conf_data = 0;
			break;
	}

	return conf_data;
}
#else
static void pci_ehci_write_reg(int reg, u32 value)
{
	return;
}

static u32 pci_ehci_read_reg(int reg)
{
	return  0xffffffff;
}


#endif

#ifdef	TEST_CS5536_USE_UDC
static void pci_udc_write_reg(int reg, u32 value)
{
	u32 hi, lo;
	
	switch(reg){
		case PCI_COMMAND_STATUS_REG :
			// COMMAND
			if(value & PCI_COMMAND_MASTER_ENABLE){
				_rdmsr(USB_MSR_REG(USB_UDC), &hi, &lo);
				hi |= (1 << 2);
				_wrmsr(USB_MSR_REG(USB_UDC), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_UDC), &hi, &lo);
				hi &= ~(1 << 2);
				_wrmsr(USB_MSR_REG(USB_UDC), hi, lo);
			}
			if(value & PCI_COMMAND_MEM_ENABLE){
				_rdmsr(USB_MSR_REG(USB_UDC), &hi, &lo);
				hi |= (1 << 1);
				_wrmsr(USB_MSR_REG(USB_UDC), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_UDC), &hi, &lo);
				hi &= ~(1 << 1);
				_wrmsr(USB_MSR_REG(USB_UDC), hi, lo);				
			}
			// STATUS
			if(value & PCI_STATUS_PARITY_ERROR){
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				if(lo & SB_PARE_ERR_FLAG){
					lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
					_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
				}				
			}
			break;
		case PCI_BAR0_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_UDC_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if( (value & 0x01) == 0x00 ){
				_rdmsr(USB_MSR_REG(USB_UDC), &hi, &lo);
				lo = value;
				_wrmsr(USB_MSR_REG(USB_UDC), hi, lo);
				
				value &= 0xfffffff0;
				hi = 0x40000000 | ((value & 0xff000000) >> 24);
				lo = 0x000fffff | ((value & 0x00fff000) << 8);
				_wrmsr(GLIU_MSR_REG(GLIU_P2D_BM0), hi, lo);
			}
			break;
		default :
			break;			
	}
	
	return;
}

static u32 pci_udc_read_reg(int reg)
{
	u32 conf_data;
	u32 hi, lo;
	
	switch(reg){
		case PCI_ID_REG :
			conf_data = (CS5536_UDC_DEVICE_ID << 16 | CS5536_VENDOR_ID);
			break;
		case PCI_COMMAND_STATUS_REG :
			conf_data = 0;
			// COMMAND
			_rdmsr(USB_MSR_REG(USB_UDC), &hi, &lo);
			if(hi & 0x04)
				conf_data |= PCI_COMMAND_MASTER_ENABLE;
			if(hi & 0x02)
				conf_data |= PCI_COMMAND_MEM_ENABLE;
			// STATUS
			conf_data |= PCI_STATUS_66MHZ_SUPPORT;
			conf_data |= PCI_STATUS_BACKTOBACK_SUPPORT;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_PARE_ERR_FLAG)
				conf_data |= PCI_STATUS_PARITY_ERROR;
			conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
			break;
		case PCI_CLASS_REG :
			_rdmsr(USB_MSR_REG(USB_CAP), &hi, &lo);
			conf_data = lo & 0x000000ff;
			conf_data |= (CS5536_UDC_CLASS_CODE << 8);
			break;
		case PCI_BHLC_REG :
			conf_data = (PCI_NONE_BIST << 24) | (PCI_NORMAL_HEADER_TYPE << 16) |
				(0x00 << 8) | PCI_NORMAL_CACHE_LINE_SIZE;
			break;
		case PCI_BAR0_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_UDC_FLAG){
				conf_data = CS5536_UDC_RANGE | PCI_MAPREG_TYPE_MEM;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_UDC_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_UDC), &hi, &lo);
				conf_data = lo & 0xfffff000;
				conf_data &= ~0x0000000f; // 32bit mem
			}
			break;
		case PCI_BAR1_REG :
			conf_data = 0x000000;
			break;		
		case PCI_BAR2_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR3_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR4_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR5_REG :
			conf_data = 0x000000;
			break;
		case PCI_CARDBUS_CIS_REG :
			conf_data = PCI_CARDBUS_CIS_POINTER;
			break;
		case PCI_SUBSYS_ID_REG :
			conf_data = (CS5536_UDC_SUB_ID << 16) | CS5536_SUB_VENDOR_ID;
			break;
		case PCI_MAPREG_ROM :
			conf_data = PCI_EXPANSION_ROM_BAR;
			break;
		case PCI_CAPLISTPTR_REG :
			conf_data = PCI_CAPLIST_USB_POINTER;
			break;
		case PCI_INTERRUPT_REG :
			conf_data = (PCI_MAX_LATENCY << 24) | (PCI_MIN_GRANT << 16) | 
				(0x01 << 8) | 0x00;
			break;
		default :
			conf_data = 0;
			break;
	}

	return conf_data;
}

#else	/* TEST_CS5536_USE_UDC */

static void pci_udc_write_reg(int reg, u32 value)
{
	return;
}

static u32 pci_udc_read_reg(int reg)
{
	return  0xffffffff;
}

#endif	/* TEST_CS5536_USE_UDC */


#ifdef	TEST_CS5536_USE_OTG
static void pci_otg_write_reg(int reg, u32 value)
{
	u32 hi, lo;
	
	switch(reg){
		case PCI_COMMAND_STATUS_REG :
			// COMMAND
			if(value & PCI_COMMAND_MEM_ENABLE){
				_rdmsr(USB_MSR_REG(USB_OTG), &hi, &lo);
				hi |= (1 << 1);
				_wrmsr(USB_MSR_REG(USB_OTG), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_OTG), &hi, &lo);
				hi &= ~(1 << 1);
				_wrmsr(USB_MSR_REG(USB_OTG), hi, lo);				
			}
			// STATUS
			if(value & PCI_STATUS_PARITY_ERROR){
				_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
				if(lo & SB_PARE_ERR_FLAG){
					lo = (lo & 0x0000ffff) | SB_PARE_ERR_FLAG;
					_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
				}				
			}
			break;
		case PCI_BAR0_REG :
			if(value == PCI_BAR_RANGE_MASK){
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo |= SOFT_BAR_OTG_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else if( (value & 0x01) == 0x00 ){
				_rdmsr(USB_MSR_REG(USB_OTG), &hi, &lo);
				lo = value & 0xffffff00;
				_wrmsr(USB_MSR_REG(USB_OTG), hi, lo);
				
				value &= 0xfffffff0;
				hi = 0x40000000 | ((value & 0xff000000) >> 24);
				lo = 0x000fffff | ((value & 0x00fff000) << 8);
				_wrmsr(GLIU_MSR_REG(GLIU_P2D_BM1), hi, lo);
			}
			break;
		default :
			break;			
	}
	
	return;
}

static u32 pci_otg_read_reg(int reg)
{
	u32 conf_data;
	u32 hi, lo;
	
	switch(reg){
		case PCI_ID_REG :
			conf_data = (CS5536_OTG_DEVICE_ID << 16 | CS5536_VENDOR_ID);
			break;
		case PCI_COMMAND_STATUS_REG :
			conf_data = 0;
			// COMMAND
			_rdmsr(USB_MSR_REG(USB_OTG), &hi, &lo);
			if(hi & 0x02)
				conf_data |= PCI_COMMAND_MEM_ENABLE;
			// STATUS
			conf_data |= PCI_STATUS_66MHZ_SUPPORT;
			conf_data |= PCI_STATUS_BACKTOBACK_SUPPORT;
			_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
			if(lo & SB_PARE_ERR_FLAG)
				conf_data |= PCI_STATUS_PARITY_ERROR;
			conf_data |= PCI_STATUS_DEVSEL_MEDIUM;
			break;
		case PCI_CLASS_REG :
			_rdmsr(USB_MSR_REG(USB_CAP), &hi, &lo);
			conf_data = lo & 0x000000ff;
			conf_data |= (CS5536_OTG_CLASS_CODE << 8);
			break;
		case PCI_BHLC_REG :
			conf_data = (PCI_NONE_BIST << 24) | (PCI_NORMAL_HEADER_TYPE << 16) |
				(0x00 << 8) | PCI_NORMAL_CACHE_LINE_SIZE;
			break;
		case PCI_BAR0_REG :
			_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
			if(lo & SOFT_BAR_OTG_FLAG){
				conf_data = CS5536_OTG_RANGE | PCI_MAPREG_TYPE_MEM;
				_rdmsr(GLCP_MSR_REG(GLCP_SOFT_COM), &hi, &lo);
				lo &= ~SOFT_BAR_OTG_FLAG;
				_wrmsr(GLCP_MSR_REG(GLCP_SOFT_COM), hi, lo);
			}else{
				_rdmsr(USB_MSR_REG(USB_OTG), &hi, &lo);
				conf_data = lo & 0xffffff00;
				conf_data &= ~0x0000000f;
			}
			break;
		case PCI_BAR1_REG :
			conf_data = 0x000000;
			break;		
		case PCI_BAR2_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR3_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR4_REG :
			conf_data = 0x000000;
			break;
		case PCI_BAR5_REG :
			conf_data = 0x000000;
			break;
		case PCI_CARDBUS_CIS_REG :
			conf_data = PCI_CARDBUS_CIS_POINTER;
			break;
		case PCI_SUBSYS_ID_REG :
			conf_data = (CS5536_OTG_SUB_ID << 16) | CS5536_SUB_VENDOR_ID;
			break;
		case PCI_MAPREG_ROM :
			conf_data = PCI_EXPANSION_ROM_BAR;
			break;
		case PCI_CAPLISTPTR_REG :
			conf_data = PCI_CAPLIST_USB_POINTER;
			break;
		case PCI_INTERRUPT_REG :
			conf_data = (PCI_MAX_LATENCY << 24) | (PCI_MIN_GRANT << 16) | 
				(0x01 << 8) | 0x00;
			break;
		default :
			conf_data = 0;
			break;
	}

	return conf_data;
}

#else	/* TEST_CS5536_USE_OTG */

static void pci_otg_write_reg(int reg, u32 value)
{
	return;
}

static u32 pci_otg_read_reg(int reg)
{
	return 0xffffffff;
}

#endif	/* TEST_CS5536_USE_OTG */

/*******************************************************************************/

/*
 * writen : write to PCI config space and transfer it to MSR write.
 */
int cs5536_pci_conf_writen(int function, int reg, int width, pcireg_t value)
{
	unsigned short temp_reg;

	/* some basic checking. */
	if( (function < CS5536_FUNC_START) || (function > CS5536_FUNC_END) ){
		printf("cs5536 pci conf read : function range error.\n");
		return (-1);
	}
	if( (reg < 0) || (reg > 0x100) ){
		printf("cs5536 pci conf read : register range error.\n");
		return (-1);
	}
	if( width != 4 ){
		printf("cs5536 pci conf read : width error.\n");
		return (-1);
	}
	
	temp_reg = reg;
	reg = temp_reg & 0xfc;
	switch(function){
		case CS5536_ISA_FUNC :
			pci_isa_write_reg(reg, value);		
			break;

		case CS5536_FLASH_FUNC :
			pci_flash_write_reg(reg, value);
			break;
		
		case CS5536_IDE_FUNC :
			pci_ide_write_reg(reg, value);
			break;

		case CS5536_ACC_FUNC :
			pci_acc_write_reg(reg, value);
			break;

		case CS5536_OHCI_FUNC :
			pci_ohci_write_reg(reg, value);
			break;

		case CS5536_EHCI_FUNC :
			pci_ehci_write_reg(reg, value);
			break;

		case CS5536_UDC_FUNC :
			pci_udc_write_reg(reg, value);
			break;

		case CS5536_OTG_FUNC :
			pci_otg_write_reg(reg, value);
			break;
		
		default :
			printf("cs5536 not supported function.\n");
			break;
	
	}
	
	return 0;
}


/*
 * readn : read PCI config space and transfer it to MSR access.
 */
pcireg_t cs5536_pci_conf_readn(int function, int reg, int width)
{
	pcireg_t data;
	int temp_reg;

	/* some basic checking. */
	if( (function < CS5536_FUNC_START) || (function > CS5536_FUNC_END) ){
		printf("cs5536 pci conf read : function range error.\n");
		return (-1);
	}
	if( (reg < 0) || (reg > 0x100) ){
		printf("cs5536 pci conf read : register range error.\n");
		return (-1);
	}
	if( (width != 1) && (width != 2) && (width != 4) ){
		printf("cs5536 pci conf read : width error.\n");
		return (-1);
	}
	
	temp_reg = reg;
	reg = temp_reg & 0xfc;
	switch(function){
		case CS5536_ISA_FUNC :
			data = pci_isa_read_reg(reg);		
			break;

		case CS5536_FLASH_FUNC :
			data = pci_flash_read_reg(reg);
			break;
		
		case CS5536_IDE_FUNC :
			data = pci_ide_read_reg(reg);
			break;

		case CS5536_ACC_FUNC :
			data = pci_acc_read_reg(reg);
			break;

		case CS5536_OHCI_FUNC :
			data = pci_ohci_read_reg(reg);
			break;

		case CS5536_EHCI_FUNC :
			data = pci_ehci_read_reg(reg);
			break;

		case CS5536_UDC_FUNC :
			data = pci_udc_read_reg(reg);
			break;

		case CS5536_OTG_FUNC :
			data = pci_otg_read_reg(reg);
			break;
		
		default :
			printf("cs5536 not supported function.\n");
			break;
	
	}
	
	data =  data >> ( (temp_reg & 0x03) << 3 );
	return data;
}

void cs5536_pci_conf_write4(int function, int reg, pcireg_t value){
	cs5536_pci_conf_writen(function, reg, 4, value);
	return;
}

pcireg_t cs5536_pci_conf_read4(int function, int reg){
	return cs5536_pci_conf_readn(function, reg, 4);
}

/***************************************************************************/

#if	1
static int cmd_rdecreg(int ac, char *av[])
{
	u32 start, size, reg;
	u8 value;

	size = 0;
	if(!get_rsa(&start, av[1])) {
		printf("ecreg : access error!\n");
		return -1;
	}

	if(!get_rsa(&size, av[2])){
		printf("ecreg : size error\n");
		return -1;
	}

	if((start > 0x10000) || (size > 0x10000)){
		printf("ecreg : start not available.\n");
	}

	printf("ecreg : \n");
	reg = start;
	while(size > 0){
		*( (volatile unsigned char *)(0xbfd00381) ) = (reg & 0xff00) >> 8;
		*( (volatile unsigned char *)(0xbfd00382) ) = (reg & 0x00ff);
		value = *((volatile unsigned char *)(0xbfd00383));
		printf("reg address : 0x%x,  value : 0x%x\n", reg, value);
		reg++;
		size--;
	}

	return 0;
}

static int cmd_rdmsr(int ac, char *av[])
{
	u32 msr, hi, lo;

	if(!get_rsa(&msr, av[1])) {
		printf("msr : access error!\n");
		return -1;
	}

	_rdmsr(msr, &hi, &lo);
	
	printf("msr : address  %x    hi  %x   lo  %x\n", msr, hi, lo);
	
	return 0;
}

static int cmd_wrmsr(int ac, char *av[])
{
	u32 msr, hi, lo;

	if(!get_rsa(&msr, av[1])){
		printf("msr : access error!\n");
		return -1;
	}else if(!get_rsa(&hi, av[2])){
		printf("hi : access error!\n");
		return -1;
	}else if(!get_rsa(&lo, av[3])){
		printf("lo : access error!\n");
		return -1;
	}

	_wrmsr(msr, hi, lo);
	
	return 0;
}

static inline wrec(u16 reg, u8 val)
{
	*( (volatile unsigned char *)(0xbfd00381) ) = (reg & 0xff00) >> 8;
	*( (volatile unsigned char *)(0xbfd00382) ) = (reg & 0x00ff);
	*( (volatile unsigned char *)(0xbfd00383) ) = val;
}

static inline rdec(u16 reg)
{
	*( (volatile unsigned char *)(0xbfd00381) ) = (reg & 0xff00) >> 8;
	*( (volatile unsigned char *)(0xbfd00382) ) = (reg & 0x00ff);
	return (*( (volatile unsigned char *)(0xbfd00383) ));
}

/* SMBUS relative register block according to the EC datasheet. */
#define	REG_SMBTCRC		0xff92
#define	REG_SMBPIN		0xff93
#define	REG_SMBCFG		0xff94
#define	REG_SMBEN		0xff95
#define	REG_SMBPF		0xff96
#define	REG_SMBRCRC		0xff97
#define	REG_SMBPRTCL	0xff98
#define	REG_SMBSTS		0xff99
#define	REG_SMBADR		0xff9a
#define	REG_SMBCMD		0xff9b
#define	REG_SMBDAT_START		0xff9c
#define	REG_SMBDAT_END			0xffa3
#define	SMBDAT_SIZE				8
#define	REG_SMBRSA		0xffa4
#define	REG_SMBCNT		0xffbc
#define	REG_SMBAADR		0xffbd
#define	REG_SMBADAT0	0xffbe
#define	REG_SMBADAT1	0xffbf


static int cmd_rdbat(int ac, char *av[])
{
	u8 index;
	u8 val;

	if(ac < 2){
		printf("usage : rdbat %index\n");
		return -1;
	}

	if(!get_rsa(&index, av[1])) {
		printf("rdbat : access error!\n");
		return -1;
	}

	printf("battery : index 0x%x \t", index);
	wrec(REG_SMBSTS, 0x00);
	wrec(REG_SMBCNT, 0x01);
	val = rdec(REG_SMBPIN);
	val = (val & 0xfc) | (1 << 0);
	wrec(REG_SMBPIN, val);
	wrec(REG_SMBADR, 0x6c|0x01);
	wrec(REG_SMBCMD, index);
	wrec(REG_SMBPRTCL, 0x07);
	while(!(rdec(REG_SMBSTS) & (1 << 7)));

	val = rdec(REG_SMBDAT_START);
	printf("value 0x%x\n", val);

	return 0;
}

static int cmd_rdfan(int ac, char *av[])
{
	u8 index;
	u8 val;

	if(ac < 2){
		printf("usage : rdfan %index\n");
		return -1;
	}

	if(!get_rsa(&index, av[1])) {
		printf("rdfan : access error!\n");
		return -1;
	}

	printf("fan : index 0x%x \t", index);
	wrec(REG_SMBSTS, 0x00);
	wrec(REG_SMBCNT, 0x02);
	val = rdec(REG_SMBPIN);
	val = (val & 0xfc) | (1 << 1);
	wrec(REG_SMBPIN, val);
	wrec(REG_SMBADR, 0x90|0x01);
	wrec(REG_SMBCMD, index);
	wrec(REG_SMBPRTCL, 0x09);
	while(!(rdec(REG_SMBSTS) & (1 << 7)));

	val = rdec(REG_SMBDAT_START);
	printf("value 0x%x\t", val);
	val = rdec(REG_SMBDAT_START + 1);
	printf("value2 0x%x\n", val);

	return 0;
}

#define	DISPLAY_MODE_NORMAL		0	// Screen(on);	Hsync(on);	Vsync(on)
#define	DISPLAY_MODE_STANDBY	1	// Screen(off);	Hsync(off);	Vsync(on)
#define	DISPLAY_MODE_SUSPEND	2	// Screen(off);	Hsync(on);	Vsync(off)
#define	DISPLAY_MODE_OFF		3	// Screen(off);	Hsync(off);	Vsync(off)
#define	SEQ_INDEX	0x3c4
#define	SEQ_DATA	0x3c5
static inline unsigned char read_seq(int reg)
{
	*((volatile unsigned char *)(0xbfd00000 | SEQ_INDEX)) = reg;
	return (*(volatile unsigned char *)(0xbfd00000 | SEQ_DATA));
}

static inline void write_seq(int reg, unsigned char data)
{
	*((volatile unsigned char *)(0xbfd00000 | SEQ_INDEX)) = reg;
	*((volatile unsigned char *)(0xbfd00000 | SEQ_DATA)) = data;

	return;
}

static u8 cur_mode;
static u8 SavedSR01, SavedSR20, SavedSR21, SavedSR22, SavedSR23, SavedSR24, SavedSR31, SavedSR34;
static int cmd_powerdebug(int ac, char *av[])
{
	u8 index;
	u8 val;
	u8 SR01, SR20, SR21, SR22, SR23, SR24, SR31, SR34;

	if(ac < 2){
		printf("usage : powerdebug %index\n");
		return -1;
	}

	if(!get_rsa(&index, av[1])) {
		printf("powerdebug : access error!\n");
		return -1;
	}
	
	if(cur_mode == DISPLAY_MODE_NORMAL){
		SavedSR01 = read_seq(0x01);
		SavedSR20 = read_seq(0x20);
		SavedSR21 = read_seq(0x21);
		SavedSR22 = read_seq(0x22);
		SavedSR23 = read_seq(0x23);
		SavedSR24 = read_seq(0x24);
		SavedSR31 = read_seq(0x31);
		SavedSR34 = read_seq(0x34);
		printf("normal power register : \n");
		printf("SR01 0x%x   SR20 0x%x   SR21 0x%x   SR22 0x%x   SR23 0x%x\n", 
						SavedSR01, SavedSR20, SavedSR21, SavedSR22, SavedSR23, SavedSR24);
		printf("SR24 0x%x   SR31 0x%x   SR34 0x%x\n", SavedSR24, SavedSR31, SavedSR34);
	}
	SR01 = read_seq(0x01);
	SR20 = read_seq(0x20);
	SR21 = read_seq(0x21);
	SR22 = read_seq(0x22);
	SR23 = read_seq(0x23);
	SR24 = read_seq(0x24);
	SR31 = read_seq(0x31);
	SR34 = read_seq(0x34);

	printf("powerdebug : mode %d\n", index);
	switch(index){
		case	DISPLAY_MODE_NORMAL :
				SR01 &= ~0x20;
				SR20 = SavedSR20;
		        SR21 = SavedSR21;
				SR22 &= ~0x30; 
				SR23 &= ~0xC0;
				SR24 |= 0x01;
				SR31 = SavedSR31;
				SR34 = SavedSR34;
				break;
		case	DISPLAY_MODE_STANDBY :
				SR01 |= 0x20;
				SR20 = (SR20 & ~0xB0) | 0x10;
				SR21 |= 0x88;
				SR22 = (SR22 & ~0x30) | 0x10;
				SR23 = (SR23 & ~0x07) | 0xD8;
				SR24 &= ~0x01;
				SR31 = (SR31 & ~0x07) | 0x00;
				SR34 |= 0x80;
				break;
		case	DISPLAY_MODE_SUSPEND :
				SR01 |= 0x20;
				SR20 = (SR20 & ~0xB0) | 0x10;
				SR21 |= 0x88;
				SR22 = (SR22 & ~0x30) | 0x20;
				SR23 = (SR23 & ~0x07) | 0xD8;
				SR24 &= ~0x01;
				SR31 = (SR31 & ~0x07) | 0x00;
				SR34 |= 0x80;
				break;
		case	DISPLAY_MODE_OFF :
				SR01 |= 0x20;
			    SR20 = (SR20 & ~0xB0) | 0x10;
				SR21 |= 0x88;
				SR22 = (SR22 & ~0x30) | 0x30;
				SR23 = (SR23 & ~0x07) | 0xD8;
				SR24 &= ~0x01;
				SR31 = (SR31 & ~0x07) | 0x00;
				SR34 |= 0x80;
				break;
		default :
				printf("powerdebug : not supported mode setting for display.\n");
				break;
	}

	if( (*((volatile unsigned char *)(0xbfd00000 | 0x3cc))) & 0x01 ){
		while( (*((volatile unsigned char *)(0xbfd00000 | 0x3da))) & 0x08 );
		while( !((*((volatile unsigned char *)(0xbfd00000 | 0x3da))) & 0x08) );
	}else{
		while( (*((volatile unsigned char *)(0xbfd00000 | 0x3ba))) & 0x08 );
		while( !((*((volatile unsigned char *)(0xbfd00000 | 0x3ba))) & 0x08) );
	}

	write_seq(0x01, SR01);
	write_seq(0x20, SR20);
	write_seq(0x21, SR21);
	write_seq(0x22, SR22);
	write_seq(0x23, SR23);
	write_seq(0x24, SR24);
	write_seq(0x31, SR31);
	write_seq(0x34, SR34);

	cur_mode = index;

	return 0;
}

#define	XBI_BANK	0xFE00
#define	XBISEG0		0xA0
#define	XBISEG1		0xA1
#define	XBIRSV2		0xA2
#define	XBIRSV3		0xA3
#define	XBIRSV4		0xA4
#define	XBICFG		0xA5
#define	XBICS		0xA6
#define	XBIWE		0xA7
#define	XBISPIA0	0xA8
#define	XBISPIA1	0xA9
#define	XBISPIA2	0xAA
#define	XBISPIDAT	0xAB
#define	XBISPICMD	0xAC
#define	XBISPICFG	0xAD
#define	XBISPIDATR	0xAE
#define	XBISPICFG2	0xAF
#define	XBI_READ	0x03
#define	XBI_WRITE	0x02
#define	XBI_WRITE_ENABLE	0x06
static int cmd_xbi(int ac, char *av[])
{
	u8 val;
	u8 cmd;
	u32 addr;
	u8 data;
	int timeout = 0x100000;

	if(ac < 3){
		//printf("usage : xbi cmd start_address [val]\n");
		printf("usage : xbi start_address [val]\n");
		return -1;
	}

//	if(!get_rsa(&cmd, av[1])) {
//		printf("xbi : access error!\n");
//		return -1;
//	}
	if(!get_rsa(&addr, av[1])) {
		printf("xbi : access error!\n");
		return -1;
	}
//	if(ac == 4){
		if(!get_rsa(&data, av[2])) {
			printf("xbi : access error!\n");
			return -1;
		}
//	}

	/* disable 8051 fetching code. */
	val = rdec(XBI_BANK | XBICFG);
	wrec(XBI_BANK | XBICFG, val & ~(1 << 6));
	/* enable spicmd writing. */
	val = rdec(XBI_BANK | XBISPICFG);
	wrec(XBI_BANK | XBISPICFG, val | (1 << 3));
	/* enable write spi flash */
	wrec(XBI_BANK | XBISPICMD, XBI_WRITE_ENABLE);

	/* write the address */
	wrec(XBI_BANK | XBISPIA2, (addr & 0xff0000) >> 16);
	wrec(XBI_BANK | XBISPIA1, (addr & 0x00ff00) >> 8);
	wrec(XBI_BANK | XBISPIA0, (addr & 0x0000ff) >> 0);
//	if(cmd == XBI_WRITE)
			wrec(XBI_BANK | XBISPIDAT, data);
	/* start action */
	wrec(XBI_BANK | XBISPICMD, 2);
	while(timeout-- >= 0){
		if( !(rdec(XBI_BANK | XBISPICFG) & (1 << 1)) )
				break;
	}
	if(timeout <= 0){
		printf("xbi : read timeout.\n");
		return -1;
	}

//	if(cmd == XBI_READ){
//		/* get data */
//		data = rdec(XBI_BANK | XBISPIDAT);
//		printf("ec : addr 0x%x\t, data 0x%x\n", addr, data);
//	}

	return 0;
}

static const Cmd Cmds[] =
{
	{"cs5536 debug"},
	{"rdmsr", "reg", NULL, "msr read test", cmd_rdmsr, 2, 99, CMD_REPEAT},
	{"wrmsr", "reg", NULL, "msr write test", cmd_wrmsr, 2, 99, CMD_REPEAT},
	{"rdecreg", "reg", NULL, "KB3310 EC reg read test", cmd_rdecreg, 2, 99, CMD_REPEAT},
	{"rdbat", "reg", NULL, "KB3310 smbus battery reg read test", cmd_rdbat, 2, 99, CMD_REPEAT},
	{"rdfan", "reg", NULL, "KB3310 smbus fan reg read test", cmd_rdfan, 2, 99, CMD_REPEAT},
	{"powerdebug", "reg", NULL, "for debug the power state of sm712 graphic", cmd_powerdebug, 2, 99, CMD_REPEAT},
	{"xbi", "reg", NULL, "for debug write or read data from xbi interface of ec", cmd_xbi, 2, 99, CMD_REPEAT},
	{0},
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
#endif

/**************************************************************************/



