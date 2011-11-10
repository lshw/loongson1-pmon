/*
 * cs5536.c :
 * 	for init the cs5536 sourth bridge on the godson based mainboard.
 * Author : kkmao <tiansm@lemote.com>
 * Date : 07-06-28
 *
 * 1, Modified by liujl<liujl@lemote.com> on 07-06-28
 *	collect the code to the cs5536 based macro file.
 *		
 */
#include <stdio.h>
#include <stdarg.h>
#include <sys/linux/types.h>
#include <pci/cs5536_io.h>
#include <include/bonito.h>
#include <include/cs5536.h>
#include <machine/pio.h> 

/******************************************************************************/

extern void delay(int microseconds);

static int cs5536_ide_wait(unsigned char sts_bit, int timeout)
{
	unsigned char status;
	int time = 0;

	for(;;){
		status = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_STATUS));
		if( ((status & IDE_LEGACY_STATUS_BUSY) == 0)
	         && ((status & sts_bit) == sts_bit ) 
		  )
			break;
		if(++time > timeout){
			printf("cs5536 ide wait timeout.\n");
			return (-1);
		}
		delay(1000);
	}
	
	if(status & IDE_LEGACY_STATUS_ERR){
		printf("cs5536 ide wait error...\n");
		return inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_ERROR));
	}

	return 0;
}

/*
 *  ide test...
 */
#define	IDE_IDENTIFY_DATA_LEN	512
static void cs5536_ide_test(void)
{
	unsigned char identify[IDE_IDENTIFY_DATA_LEN];
	unsigned char control;
	unsigned char head;
	unsigned int  length;
	unsigned short *ptr;
	int i;
	int rst_cycle;
	unsigned char status;
	unsigned char sc, sn, cl, ch;
	int is_atapi;
	unsigned char command;
	unsigned char fir_status;
	unsigned char sec_status;
	int identify_timeout;
	
	printf("cs5536_ide : begin test............................\n");
	
	for(i = 0; i < IDE_IDENTIFY_DATA_LEN; i++)
		identify[i] = 0;
	
	printf("before ide reset :");
	head = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD));
	head |= IDE_LEGACY_HEAD_IBM;
	head &= ~IDE_LEGACY_HEAD_DRV;
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), head);
	delay(10);
	status = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_STATUS));
	printf("st0 = 0x%x", status);
	head |= IDE_LEGACY_HEAD_IBM | IDE_LEGACY_HEAD_DRV;
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), head);
	delay(10);
	status = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_STATUS));
	printf("st1 = 0x%x\n", status);
	
	/* wait for ide software reset */
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), IDE_LEGACY_HEAD_IBM);
	delay(10);
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_CTRL), IDE_LEGACY_CTRL_RST|IDE_LEGACY_CTRL_IDS);
	delay(1000);
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_CTRL), IDE_LEGACY_CTRL_IDS);
	delay(5000);
	inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_ERROR));
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_CTRL), IDE_LEGACY_CTRL_4BIT);
	delay(500000);
	for(rst_cycle = 0; rst_cycle < 31000/2; rst_cycle++){	// 31000 maybe
		outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), IDE_LEGACY_HEAD_IBM);
		delay(10);
		status = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_STATUS));
		delay(10);
		//printf("cs5536 ide status value = 0x%x\n", status);
		if( (status & IDE_LEGACY_STATUS_BUSY) == 0 )
			break;
		delay(1000);
	}
	if(rst_cycle >= 31000 / 2){
		printf("cs5536 ide software reset timeout\n");
		return;
	}else if(rst_cycle < 31000/2){
		printf("cycle time is : %d\n", rst_cycle);
		printf("cs5536 ide software reset ok.\n");
	}

	printf("after ide reset :");
	
	head = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD));
	head |= IDE_LEGACY_HEAD_IBM;
	head &= ~IDE_LEGACY_HEAD_DRV;
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), head);
	delay(10);
	status = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_STATUS));
	printf("st0 = 0x%x", status);
	head |= IDE_LEGACY_HEAD_IBM | IDE_LEGACY_HEAD_DRV;
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), head);
	delay(10);
	status = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_STATUS));
	printf("st1 = 0x%x\n", status);
	
	/* detect the type */
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), IDE_LEGACY_HEAD_IBM | (0 << 4));
	delay(10);
	sc = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_SECTOR_COUNT));
	sn = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_SECTOR_NUM));
	cl = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_CYL_LO));
	ch = inb(PRI_IDE_LEGACY_REG(IDE_LEGACY_CYL_HI));
	printf("sector_count = 0x%x, sector_number = 0x%x, cyl low = 0x%x, cyl high = 0x%x\n", sc, sn, cl, ch);
	if( (cl == 0x14) && (ch == 0xeb) ){
		is_atapi = 1;
		printf("cs5536 ide atapi\n");
	}else{
		is_atapi = 0;
		printf("cs5536 ide ata\n");
	}
	
	/* identify command to get parameters */
	if(is_atapi){
		command = 0xa1;
		fir_status = 0;
		sec_status = IDE_LEGACY_STATUS_DRQ;
		identify_timeout = 10000;
	}else{
		command = 0xec;
		fir_status = IDE_LEGACY_STATUS_DRDY;
		//fir_status = 0x0;
		sec_status = IDE_LEGACY_STATUS_DRQ;
		identify_timeout = 1000;
	}
	// disable interrupt, read return the aux status register
	control = IDE_LEGACY_CTRL_IDS;
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_CTRL), control);
	
	// IBM 512MB and master driver
	head = IDE_LEGACY_HEAD_IBM;
	outb(PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), head);

	// wait for drive ready status
	if(cs5536_ide_wait(fir_status, identify_timeout) != 0){
		printf("cs5536 wait for driver ready err.\n");
		return;
	}
	
	// exec command
	outb( PRI_IDE_LEGACY_REG(IDE_LEGACY_HEAD), (0xa0 | (0 << 4) | 0) );
	outb( PRI_IDE_LEGACY_REG(IDE_LEGACY_FEATURE), 0x00 );
	outb( PRI_IDE_LEGACY_REG(IDE_LEGACY_CYL_LO), 0x00 );
	outb( PRI_IDE_LEGACY_REG(IDE_LEGACY_CYL_HI), 0x00 );
	outb( PRI_IDE_LEGACY_REG(IDE_LEGACY_SECTOR_NUM), 0x00 );
	outb( PRI_IDE_LEGACY_REG(IDE_LEGACY_SECTOR_COUNT), 0x00 );
	outb( PRI_IDE_LEGACY_REG(IDE_LEGACY_COMMAND), command );
	delay(10);
	
	if(cs5536_ide_wait(sec_status, identify_timeout) != 0){
		printf("cs5536 wait for data request err.\n");
		return;
	}
	
	// get data
	length = IDE_IDENTIFY_DATA_LEN;
	ptr = (unsigned short *)(&identify[0]);
	while(length > 0){
		*ptr = inw( PRI_IDE_LEGACY_REG(IDE_LEGACY_DATA) );
		ptr++;
		length -= 2;
	}
	// print data
	printf("cs5536 get identify data as following: \n");
	for(i = 0; i < IDE_IDENTIFY_DATA_LEN; i++){
		printf("0x%x", identify[i]);
		if((i % 5) == 0)
			printf("\n");
	}
	
	printf("cs5536_ide: end test...................................\n");
	
	return;	
}

/********************************************************************************/

/*
 * cs5536_ide_init : the ide MSR register configuration to setup the primary HD
 */
static void cs5536_ide_init(void)
{
	u32	hi, lo;
	u32	tag, reg;
	u32	barbase;
	u32	val;
	
	/* 
	 * multiplexed pins associated with IDE Controller 
	 * Flash controller reside in DD module
	 */
	_rdmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), &hi, &lo);
	lo |= 0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), hi, lo);
	/*
	 * IDE_IRQ0/GPIO2 :
	 * 	IDE Interrupt. Indicates the external
	 * 	IDE device has completed the DMA operation.
	 *	Pull-Down enable for irq is high effective default.
	 */
#if	0
	set_gpio_reg(GPIO_REG(GPIOL_IN_EN), 2);
	set_gpio_reg(GPIO_REG(GPIOL_IN_AUX1_SEL), 2);
	set_gpio_reg(GPIO_REG(GPIOL_PU_EN), 2);
#endif
	/* 
	 * enable base addr 
	 * Reject Primary & Secondary IDE Port
	 */
	_rdmsr(SB_MSR_REG(SB_CTRL), &hi, &lo);
	lo &= ~( (1<<13) | (1 << 14) );
	_wrmsr(SB_MSR_REG(SB_CTRL), hi, lo);
	
	/*
	 * IDECFG  msr:
	 * 1: channel port enable
	 * 14: enalbe primary port posted-write buffer for PIO modes
	 * 16(master)/17(slave): cable type, 1 represent high speed 80-pin cable
	 */
	_rdmsr(IDE_MSR_REG(IDE_CFG), &hi, &lo);
	lo |= (1 << 1);
	_wrmsr(IDE_MSR_REG(IDE_CFG), hi, lo);
	
	
	/*
	 * pid mode timing setting
	 * IDE_DTC : drive 0/1 data io-r/w active pluse width and min recovery time
	 * IDE_CAST : drive 0/1 control io-r/w active pluse width, 
	 * 	min recovery time and address setup time
	 */
	_rdmsr(IDE_MSR_REG(IDE_DTC), &hi, &lo);
	lo &= 0x0;
	lo |= 0x98980000;	//PIO-0 mode for 300ns IO-R/W
//	lo |= 0x20200000;	//PIO-4 mode for 90ns IO-R/W
	_wrmsr(IDE_MSR_REG(IDE_DTC), hi, lo);

	_rdmsr(IDE_MSR_REG(IDE_CAST), &hi, &lo);
	lo &= 0x0;
//	lo |= 0x20000000;	//PIO-4 mode
	lo |= 0x990000a0;	//PIO-0 mode 
	_wrmsr(IDE_MSR_REG(IDE_CAST), hi, lo);

	/*
	 * IDE legacy I/O space 
	 * enable the 1f0~1f7/3f6 io space access.
	 */
	hi = 0x60000000;
	lo = 0x1f0ffff8;
	_wrmsr(GLIU_MSR_REG(GLIU_IOD_BM0), hi, lo);
	hi = 0x60000000;
	lo = 0x3f6ffffe;
	_wrmsr(GLIU_MSR_REG(GLIU_IOD_BM1), hi, lo);

	/*
	 * IDE Controller UDMA Extended Timing Control Register
	 * 23: 
	 * 	0: Enable UDMA by detecting the “Set Feature” ATA command.
	 *	1: Enable UDMA by setting D1UDMAEN.
	 */
	//_rdmsr(IDE_MSR_REG(IDE_ETC), &hi, &lo);
	//lo &= ~(1<<23);
	//_wrmsr(IDE_MSR_REG(IDE_ETC), hi, lo);

	// delete it if the ide access is ok.
//	cs5536_ide_test();
	return;
}

/*
 * init : initialize the necessary MSRs for VSM.
 */
void cs5536_vsm_init(void)
{
	u32 hi, lo;

	/*
	 * enable the sourth bridge error flag which enable the pci transation
	 * error detection. 
	 */
	_rdmsr(SB_MSR_REG(SB_ERROR), &hi, &lo);
	//lo |= SB_MAR_ERR_EN | SB_TAR_ERR_EN | SB_SYSE_ERR_EN | SB_PARE_ERR_EN;
	lo |= SB_MAR_ERR_EN | SB_TAR_ERR_EN | SB_TAS_ERR_EN;
	_wrmsr(SB_MSR_REG(SB_ERROR), hi, lo);
	
	/* setting the special cycle way */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LEG_IO), &hi, &lo);
	lo |= (1 << 28);
	_wrmsr(DIVIL_MSR_REG(DIVIL_LEG_IO), hi, lo);
#ifdef	PCI_SPECIAL_SHUTDOWN
	_rdmsr(DIVIL_MSR_REG(DIVIL_LEG_IO), &hi, &lo);
	lo |= (1 << 31);
	_wrmsr(DIVIL_MSR_REG(DIVIL_LEG_IO), hi, lo);
#elif	PCI_SPECIAL_ERROR
	_rdmsr(DIVIL_MSR_REG(DIVIL_ERROR), &hi, &lo);
	lo |= (1 << 15);
	_wrmsr(DIVIL_MSR_REG(DIVIL_ERROR), hi, lo);
#elif	PCI_SPECIAL_ASMI
	_rdmsr(DIVIL_MSR_REG(DIVIL_SMI), &hi, &lo);
	lo |= (1 << 1);
	_wrmsr(DIVIL_MSR_REG(DIVIL_SMI), hi, lo);
#endif
	/* 
	 * multiplexed pins associated with IDE Controller 
	 * Flash controller reside in DD module
	 */
	_rdmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), &hi, &lo);
	lo |= 0x01;
	_wrmsr(DIVIL_MSR_REG(DIVIL_BALL_OPTS), hi, lo);

	return;
}

/*
 * cs5536_ohci_init : init the usb of ohci
 */
void cs5536_ohci_init(void)
{
	u32 hi, lo;

	/* setting the USB memory map space. */
	hi = 0x40000006;
	lo = 0x050fffff;
	_wrmsr(GLIU_MSR_REG(GLIU_P2D_BM1), hi, lo);

	/* enable the usb bus master and memory enable. */
	_rdmsr(USB_MSR_REG(USB_OHCI), &hi, &lo);
	hi = (1 << 1) | (0 << 2); // BUS MASTER ENABLE AND MEM ENABLE
	lo = 0x06050000;
	_wrmsr(USB_MSR_REG(USB_OHCI), hi, lo);
	
	return;
}

void cs5536_i8259_init(void)
{
	int hi,lo;
	outb(CS5536_LEGACY_BASE_ADDR | 0x20, 0x11); /* Initialization sequence (8259A-1). */
	outb(CS5536_LEGACY_BASE_ADDR | 0xA0, 0x11); /* Initialization sequence (8259A-2). */
	outb(CS5536_LEGACY_BASE_ADDR | 0x21, 0x0);  /* Start of hardware INTs (0x20). */
	outb(CS5536_LEGACY_BASE_ADDR | 0xA1, 0x8);  /* Start of hardware INTs 2 (0x28). */

	outb(CS5536_LEGACY_BASE_ADDR | 0x21, 0x04); /* 8259-1 is master. */
	outb(CS5536_LEGACY_BASE_ADDR | 0xA1, 0x02); /* 8259-2 is slave. */
	outb(CS5536_LEGACY_BASE_ADDR | 0x21, 0x01); /* 8086 mode for master. */
	outb(CS5536_LEGACY_BASE_ADDR | 0xA1, 0x01); /* 8086 mode for slave */
	outb(CS5536_LEGACY_BASE_ADDR | 0xA1, 0xFF); /* Mask off all interrupts for now. */
	outb(CS5536_LEGACY_BASE_ADDR | 0x21, 0xFB); /* Mask all IRQs but IRQ2 is cascaded.*/
//	outb(CS5536_LEGACY_BASE_ADDR | 0xA1, 0x00); /* Mask off all interrupts for now. */
//	outb(CS5536_LEGACY_BASE_ADDR | 0x21, 0x00); /* Mask all IRQs but IRQ2 is cascaded.*/				
	_rdmsr(DIVIL_MSR_REG(PIC_IRQM_LPC), &hi, &lo);
	lo |= 0x1002;
	_wrmsr(DIVIL_MSR_REG(PIC_IRQM_LPC), hi, lo);
	_rdmsr(DIVIL_MSR_REG(PIC_IRQM_PRIM), &hi, &lo);
	lo &=~(0x1002) ;
	_wrmsr(DIVIL_MSR_REG(PIC_IRQM_PRIM), hi, lo);
	_rdmsr(DIVIL_MSR_REG(LPC_SIRQ), &hi, &lo);
	lo |=0xc0 ;
	_wrmsr(DIVIL_MSR_REG(LPC_SIRQ), hi, lo);			
	return;
}

void test_gpio_set(u32 base, int reg, int port)
{
	int val;
	
	val = *(volatile u32 *)(base + reg);
	val |= (1 << port);
	val &= ~(1 << (port + 16));
	*(volatile u32 *)(base + reg) = val;
}

void test_gpio_clr(u32 base, int reg, int port)
{
	int val;
	
	val = *(volatile u32 *)(base + reg);
	val |= (1 << (port + 16));
	val &= ~(1 << port);
	*(volatile u32 *)(base + reg) = val;
}

#if	1	
void test_gpio_function(void)
{
	u32 tag;
	u32 base;
	u32 value;
	int i;

	tag = _pci_make_tag(0, 14, 0);
	base = _pci_conf_read(tag, 0x14);
	base |= 0xbfd00000;
	base &= ~0x3;
	printf("test gpio base = 0x%x\n", base);

	test_gpio_set(base, GPIOL_OUT_EN, 2);
	value = *(volatile u32 *)(base + GPIOL_OUT_EN);
	printf("GPIOL_OUT_EN addr = 0x%x, value = 0x%x\n", (base + GPIOL_OUT_EN), value);
	
	test_gpio_set(base, GPIOL_PD_EN, 2);
	value = *(volatile u32 *)(base + GPIOL_PD_EN);
	printf("GPIOL_PD_EN addr = 0x%x, value = 0x%x\n", (base + GPIOL_PD_EN), value);
	
	test_gpio_clr(base, GPIOL_IN_EN, 2);	
	value = *(volatile u32 *)(base + GPIOL_IN_EN);
	printf("GPIOL_IN_EN addr = 0x%x, value = 0x%x\n", (base + GPIOL_IN_EN), value);
	
	test_gpio_clr(base, GPIOL_IN_AUX1_SEL, 2);
	value = *(volatile u32 *)(base + GPIOL_IN_AUX1_SEL);
	printf("GPIOL_IN_AUX1_EN addr = 0x%x, value = 0x%x\n", (base + GPIOL_IN_AUX1_SEL), value);
	
	test_gpio_clr(base, GPIOL_OUT_AUX1_SEL, 2);
	value = *(volatile u32 *)(base + GPIOL_OUT_AUX1_SEL);
	printf("GPIOL_OUT_AUX1_EN addr = 0x%x, value = 0x%x\n", (base + GPIOL_OUT_AUX1_SEL), value);
	
	test_gpio_clr(base, GPIOL_OUT_AUX2_SEL, 2);	
	value = *(volatile u32 *)(base + GPIOL_OUT_AUX2_SEL);
	printf("GPIOL_OUT_AUX2_EN addr = 0x%x, value = 0x%x\n", (base + GPIOL_OUT_AUX2_SEL), value);

	value = *(volatile u32 *)(base + GPIOL_OUT_VAL);
	printf("the init value of out addr : 0x%x, val : 0x%x\n", (base + GPIOL_OUT_VAL), value);
	
	i = 0x100000;
	while(i--){
		test_gpio_set(base, GPIOL_OUT_VAL, 2);
		test_gpio_clr(base, GPIOL_OUT_VAL, 2);
	}
	printf("out of gpio test.\n");
	return;
}
#endif

void cs5536_gpio_init(void)
{
	u32 tag;
	u32 base;

	tag = _pci_make_tag(0, 14, 0);
	base = _pci_conf_read(tag, 0x14);
	base |= 0xbfd00000;
	base &= ~0x3;
	printf("gpio base = 0x%x\n", base);

	/* set gpio2 as IDE IRQ */
	test_gpio_set(base, GPIOL_IN_EN, 2);
	test_gpio_set(base, GPIOL_IN_AUX1_SEL, 2);
	test_gpio_set(base, GPIOL_PU_EN, 2);
	test_gpio_clr(base, GPIOL_OUT_EN, 2);
	test_gpio_clr(base, GPIOL_OUT_AUX1_SEL, 2);
	test_gpio_clr(base, GPIOL_OUT_AUX2_SEL, 2);
	/* set gpio12 as I8259 IRQ */
	test_gpio_clr(base, GPIOL_IN_EN, 12);
	test_gpio_set(base, GPIOL_OUT_EN, 12);
	test_gpio_set(base, GPIOL_OUT_OD_EN, 12);
	test_gpio_set(base, GPIOL_PU_EN, 12);
	test_gpio_set(base, GPIOL_OUT_AUX1_SEL, 12);

	return;
}
/****************************************************************************/

/*
 * cs5536_init : init the cs5536 based functions
 */
void cs5536_init(void)
{
	printf("cs5536 : begin init...\n");

	cs5536_vsm_init();
	printf("cs5536 : vsm init ok\n");

	cs5536_i8259_init();
	printf("cs5536 : i8259 init ok\n");
	
	cs5536_ide_init();
	printf("cs5536 : ide init ok\n");

	cs5536_ohci_init();
	printf("cs5536 : ohci init ok\n");
	
	//cs5536_gpio_init();
	//printf("cs5536 : gpio init ok\n");
	
	return;
}

/******************************************************************************/

/*
 * cs5536_pci_otg_fixup : fixup the OTG configuration for HOST or DEVICE.
 */
void cs5536_pci_otg_fixup(void)
{
	u32 tag, base, val;
	u32 cmdsts;
	
	tag = _pci_make_tag(0, 14, 7);

	/* get the momery base address. */
	base = _pci_conf_read(tag, 0x10);
	base |= 0xb0000000;
	printf("otg base = 0x%x\n", base);

	/* enable the pci memory space  */
	cmdsts = _pci_conf_read(tag, 0x04);
	_pci_conf_write(tag, 0x04, (cmdsts | 0x02));

#ifdef	TEST_USB_DEVICE
	/* set the MUX pin as USB DEVICE */
	val = *(volatile u32 *)(base + 0x04);
	val &= ~(3 << 0);
	val |= (3 << 0);
	*(volatile u32 *)(base + 0x04) = val;
	/* use the auto pull-up for usb-D+ */
	val = *(volatile u32 *)(base + 0x00);
	val &= ~(1 << 15);
	val |= (1 << 15);
	*(volatile u32 *)(base + 0x00) = val;
#endif

#ifdef	TEST_USB_HOST
	/* set the MUX pin as HOST CONTROLLER */
	val = *(volatile u32 *)(base + 0x04);
	val &= ~(3 << 0);
	val |= 0x02;
	*(volatile u32 *)(base + 0x04) = val;
	val = *(volatile u32 *)(base + 0x04);
#endif
	
	val = *(volatile u32 *)(base + 0x0c);
	if( (val & 0x03) == 0x02 ){
		printf("cs5536 otg configured for USB HOST CONTROLLER...\n");
	}else if( (val & 0x03) == 0x03 ){
		printf("cs5536 otg configured for USB DEVICE...\n");
	}else{
		printf("cs5536 otg configured for NONE...\n");
	}

	/* restore the pci memory space bit. */
	_pci_conf_write(tag, 0x04, cmdsts);
	
	return;	
}

void cs5536_pci_fixup(void)
{
	printf("cs5536 pci fixup.\n");

#ifdef	TEST_CS5536_USE_OTG
	cs5536_pci_otg_fixup();
#endif
	return;
}
