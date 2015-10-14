#define REG_ADDRESS 0x0
#define CONFIG_BASE 0xaffffe00

	.global ddr2_config
	.ent    ddr2_config
	.set    noreorder
#	.set    mips32
ddr2_config:
    la      t0, ddr2_reg_data
    addu    t0, t0, s0
    li      t1, 0x1d
    li      t2, CONFIG_BASE

reg_write:
    ld      a1, 0x0(t0)
    sd      a1, REG_ADDRESS(t2)
    subu    t1, t1, 0x1
    addiu   t0, t0, 0x8
    bne     t1, $0, reg_write
    addiu   t2, t2, 0x10

    ############start##########
ddr2_config_start:
    li      t2, CONFIG_BASE
    la      t0, DDR2_CTL_03_DATA_HI
    addu    t0, t0, s0
10:    
    lw      a1, 0x0(t0)
    sw      a1, 0x34(t2)
    lw      a1, 0x0(t0)
    ori     a1, 0x0100
    sw      a1, 0x34(t2)

9: //not_locked
	ld      a1, 0x10(t2)
	andi    a1, 0x01
	beqz    a1, 9b
	nop
	lh      a1, 0xf2(t2)
	sltiu 	a1, 5
	beqz 	a1, 1f
	nop
	lw      a1, 0xf4(t2)
	addiu   a1, 4
	sw      a1, 0xf4(t2)
	b       10b
	nop
1:
	jr      ra
	nop
	.end    ddr2_config

	.rdata
	.align 5

ddr2_reg_data:
DDR2_CTL_00_DATA_LO: .word 0x00000101
DDR2_CTL_00_DATA_HI: .word 0x01000100 #no_concurrentap
DDR2_CTL_01_DATA_LO: .word 0x00000000
#ifdef EIGHT_BANK_MODE
DDR2_CTL_01_DATA_HI: .word 0x01000001 //内存颗粒有8bank和4bank的区分，请认真看清使用的内存颗粒的bank数
#else
DDR2_CTL_01_DATA_HI: .word 0x01000000
#endif
DDR2_CTL_02_DATA_LO: .word 0x00000000
DDR2_CTL_02_DATA_HI: .word 0x01000101
DDR2_CTL_03_DATA_LO: .word 0x01000100
DDR2_CTL_03_DATA_HI: .word 0x01010000
DDR2_CTL_04_DATA_LO: .word 0x01010101
DDR2_CTL_04_DATA_HI: .word 0x01000202
DDR2_CTL_05_DATA_LO: .word 0x04030201 #CL=3	//CL=3
DDR2_CTL_05_DATA_HI: .word 0x07000000
DDR2_CTL_06_DATA_LO: .word 0x02020203 #400	//tRTP=2(1.5) tRRD=2 tEMRS=2 tCKE=3
//DDR2_CTL_06_DATA_LO: .word 0x07070202 #800	//
DDR2_CTL_06_DATA_HI: .word 0x0a020203 #400	//WRLAT=CL-1=2  tWTR=2  tWR=3
//DDR2_CTL_06_DATA_HI: .word 0x0a060707 #800	//
DDR2_CTL_07_DATA_LO: .word 0x00010506 #CL=3 //CASLAT_LIN_GATE=CASLAT_LIN-1=5-1=4  CASLAT_LIN=CASLATx2-1=3x2-1=5
DDR2_CTL_07_DATA_HI: .word 0x00000400 #400
DDR2_CTL_08_DATA_LO: .word 0x08040201	//片选
DDR2_CTL_08_DATA_HI: .word 0x08040201	//片选
DDR2_CTL_09_DATA_LO: .word 0x00000000
DDR2_CTL_09_DATA_HI: .word 0x00000306 #400 //tRP=tRCD=3 tDAL=tWR+tRP=3+3=6(min 2)
DDR2_CTL_10_DATA_LO: .word 0x3f0b020a #400 //tRC=tRAS+tRP=8+3=11 tMRD=min 2 tFAW=10
//DDR2_CTL_10_DATA_LO: .word 0x3f1f0214 #800 //tRC=tRAS+tRP
DDR2_CTL_10_DATA_HI: .word 0x0000003f //yanhua //DQS延迟百分比
DDR2_CTL_11_DATA_LO: .word 0x00000000 //yanhua
DDR2_CTL_11_DATA_HI: .word 0x37570000 //yanhua  0x4060 -- 0x2d4d-取中
DDR2_CTL_12_DATA_LO: .word 0x08000000 #400 //tRAS = 8
//DDR2_CTL_12_DATA_LO: .word 0x18000000 #800 //tRAS
DDR2_CTL_12_DATA_HI: .word 0x002a1503 #400 //tRFC=21 tRCD=tRP=3
DDR2_CTL_13_DATA_LO: .word 0x002a002a
DDR2_CTL_13_DATA_HI: .word 0x002a002a
DDR2_CTL_14_DATA_LO: .word 0x002a002a
DDR2_CTL_14_DATA_HI: .word 0x002a002a //
DDR2_CTL_15_DATA_LO: .word 0x00000002 //
DDR2_CTL_15_DATA_HI: .word 0x00b40020 //
DDR2_CTL_16_DATA_LO: .word 0x00000087
DDR2_CTL_16_DATA_HI: .word 0x000007ff #no_interrupt
DDR2_CTL_17_DATA_LO: .word 0x44240618 //EMRS(1)  tREF=1560/780 设置为0x00010c30时 CPU频率高了跑不起来 使能DLL
//DDR2_CTL_17_DATA_LO: .word 0x47a40618
//DDR2_CTL_17_DATA_LO: .word 0x44260618
DDR2_CTL_17_DATA_HI: .word 0x80808080
DDR2_CTL_18_DATA_LO: .word 0x80808080
DDR2_CTL_18_DATA_HI: .word 0x001c8080
DDR2_CTL_19_DATA_LO: .word 0x00c8006b
DDR2_CTL_19_DATA_HI: .word 0x36b00002 #400	//tRAS_MAX=14000 TPDEX=min2
DDR2_CTL_20_DATA_LO: .word 0x00c80017 #400	//tXSR=min200 tXSNR=tRFC+10(ns)=23
DDR2_CTL_20_DATA_HI: .word 0x00000000
DDR2_CTL_21_DATA_LO: .word 0x00009c40 //tINIT=200(us)x0.2=40000tCK
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
