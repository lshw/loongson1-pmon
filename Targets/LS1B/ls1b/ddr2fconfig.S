#define REG_ADDRESS 0x0
#define CONFIG_BASE 0xaffffe00

	.global ddr2_config
	.ent    ddr2_config
	.set    noreorder
	.set    mips32
ddr2_config:
    la      t0, ddr2_reg_data
    addu    t0, t0, s0
    li      t1, 0x1d
    li      t2, CONFIG_BASE

reg_write:
#if 0
	move	a0,t0
	bal	hexserial
	nop
	move	a0,t2
	bal	hexserial
	nop

	move	a0,t1
	bal	hexserial
	nop
#endif
    ld		a1, 0x0(t0)
#if 0
	move	a0,a1
	bal	hexserial
	nop
#endif
	sd		a1, REG_ADDRESS(t2)	//*((long long *)(t2+REG_ADDRESS)) = a1;
	subu	t1, t1, 0x1			//t1 = t1(0x1d) - 0x1
	addiu	t0, t0, 0x8			//t0 = t0(ddr2_reg_data) + 0x8
	bne		t1, $0, reg_write		//if (t1 != $0) goto reg_write
	addiu	t2, t2, 0x10			//

############start##########
	li		t2, CONFIG_BASE
	la		t0, DDR2_CTL_03_DATA_HI
	addu	t0, t0, s0
10:    
	lw		a1, 0x0(t0)	//THF
	sw		a1, 0x34(t2)	//THF
	lw		a1, 0x0(t0)
	ori		a1, 0x0100
	sw		a1, 0x34(t2)

9: //not_locked
	ld		a1, 0x10(t2)
	andi	a1, 0x01
	beqz	a1, 9b
	nop
	lh		a1, 0xf2(t2)
	sltiu	a1, 5
	beqz	a1, 1f
	nop
	lw		a1, 0xf4(t2)
	addiu	a1, 4
	sw		a1, 0xf4(t2)
	b		10b
	nop
1:
	jr		ra
	nop
	
	.end	ddr2_config
	.rdata
	.align 5
	
ddr2_reg_data:
DDR2_CTL_00_DATA_LO: .word 0x00000101
DDR2_CTL_00_DATA_HI: .word 0x01000100 #no_concurrentap
DDR2_CTL_01_DATA_LO: .word 0x00000000
DDR2_CTL_01_DATA_HI: .word 0x00000000
DDR2_CTL_02_DATA_LO: .word 0x00000000
DDR2_CTL_02_DATA_HI: .word 0x01000101
DDR2_CTL_03_DATA_LO: .word 0x01000100
DDR2_CTL_03_DATA_HI: .word 0x01010000
DDR2_CTL_04_DATA_LO: .word 0x01010101
DDR2_CTL_04_DATA_HI: .word 0x01000202
DDR2_CTL_05_DATA_LO: .word 0x04070201 #CL=7	//04CASLAT
DDR2_CTL_05_DATA_HI: .word 0x00000000
DDR2_CTL_06_DATA_LO: .word 0x04040202 #800	//tRTP=4 tRRD=4
//DDR2_CTL_06_DATA_LO: .word 0x07070202 #800	//
DDR2_CTL_06_DATA_HI: .word 0x0a060407 #800	//tWR=7 tWTR=tRTP=4
//DDR2_CTL_06_DATA_HI: .word 0x0a060707 #800	//
DDR2_CTL_07_DATA_LO: .word 0x00010d0e #CL=7//cs_map to cs0
DDR2_CTL_07_DATA_HI: .word 0x00000400 #800
DDR2_CTL_08_DATA_LO: .word 0x08040201	//片选
DDR2_CTL_08_DATA_HI: .word 0x08040201	//片选
DDR2_CTL_09_DATA_LO: .word 0x00000000
DDR2_CTL_09_DATA_HI: .word 0x0000070c #800 //tRP=7
DDR2_CTL_10_DATA_LO: .word 0x3f1b0200 #800 //tRC=tRAS+tRP=20+7 
//DDR2_CTL_10_DATA_LO: .word 0x3f1f0214 #800 //tRC=tRAS+tRP
DDR2_CTL_10_DATA_HI: .word 0x0000003f //yanhua //把DQS延迟百分比减少可以提高频率
DDR2_CTL_11_DATA_LO: .word 0x00000000 //yanhua
DDR2_CTL_11_DATA_HI: .word 0x30500000 //yanhua  0x4060 -- 0x2f4f-ok
DDR2_CTL_12_DATA_LO: .word 0x14000000 #800 //tRAS = 20
//DDR2_CTL_12_DATA_LO: .word 0x18000000 #800 //tRAS
DDR2_CTL_12_DATA_HI: .word 0x002a5607 #800 //tRCD=7 tRFC=86
DDR2_CTL_13_DATA_LO: .word 0x002a002a
DDR2_CTL_13_DATA_HI: .word 0x002a002a
DDR2_CTL_14_DATA_LO: .word 0x002a002a
DDR2_CTL_14_DATA_HI: .word 0x002a002a //无关
DDR2_CTL_15_DATA_LO: .word 0x00000002 //无关
DDR2_CTL_15_DATA_HI: .word 0x00140008 //无关
DDR2_CTL_16_DATA_LO: .word 0x00000014
DDR2_CTL_16_DATA_HI: .word 0x000007ff #no_interrupt
DDR2_CTL_17_DATA_LO: .word 0x40041fff //注意0x1fff的值
//DDR2_CTL_17_DATA_LO: .word 0x0000101b //注意0x1fff的值
DDR2_CTL_17_DATA_HI: .word 0x00000000
DDR2_CTL_18_DATA_LO: .word 0x00000000
DDR2_CTL_18_DATA_HI: .word 0x001c0000
DDR2_CTL_19_DATA_LO: .word 0x00c8006b
DDR2_CTL_19_DATA_HI: .word 0x28e10002 #800
DDR2_CTL_20_DATA_LO: .word 0x00c8002f #800
DDR2_CTL_20_DATA_HI: .word 0x00000000
DDR2_CTL_21_DATA_LO: .word 0x00030d40 #real
DDR2_CTL_21_DATA_HI: .word 0x00000000
DDR2_CTL_22_DATA_LO: .word 0x00000000
DDR2_CTL_22_DATA_HI: .word 0x00000000
DDR2_CTL_23_DATA_LO: .word 0x00000000
DDR2_CTL_23_DATA_HI: .word 0x00000000
DDR2_CTL_24_DATA_LO: .word 0x00000000
DDR2_CTL_24_DATA_HI: .word 0x00000000
DDR2_CTL_25_DATA_LO: .word 0x00000000
DDR2_CTL_25_DATA_HI: .word 0x00000000
DDR2_CTL_26_DATA_LO: .word 0x00000000
DDR2_CTL_26_DATA_HI: .word 0x00000000
DDR2_CTL_27_DATA_LO: .word 0x00000000
DDR2_CTL_27_DATA_HI: .word 0x00000000
DDR2_CTL_28_DATA_LO: .word 0x00000000
DDR2_CTL_28_DATA_HI: .word 0x00000000
DDR2_CTL_start_DATA_LO: .word 0x01000000
DDR2_CTL_start_DATA_HI: .word 0x01010100
