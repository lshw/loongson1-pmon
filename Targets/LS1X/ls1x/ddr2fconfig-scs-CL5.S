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
DDR2_CTL_05_DATA_LO: .word 0x04050201 #CL=5	//CL=5
DDR2_CTL_05_DATA_HI: .word 0x07000000
DDR2_CTL_06_DATA_LO: .word 0x03040203 #800	//tRTP=3 tRRD=4 tEMRS=2 tCKE=3
//DDR2_CTL_06_DATA_LO: .word 0x07070202 #800	//
DDR2_CTL_06_DATA_HI: .word 0x0a040306 #800	//WRLAT=CL-1=4 tWTR=3 tWR=6
//DDR2_CTL_06_DATA_HI: .word 0x0a060707 #800	//
DDR2_CTL_07_DATA_LO: .word 0x00010809 #CL=7 //CASLAT_LIN_GATE=CASLAT_LIN-1=9-1=8  CASLAT_LIN=CLx2-1=2x5-1=9
DDR2_CTL_07_DATA_HI: .word 0x00000400 #800
DDR2_CTL_08_DATA_LO: .word 0x08040201	//片选
DDR2_CTL_08_DATA_HI: .word 0x08040201	//片选
DDR2_CTL_09_DATA_LO: .word 0x00000000
DDR2_CTL_09_DATA_HI: .word 0x0000050b #800 //tRP=tRCD=5 tDAL=tWR+tRP=6+5=11(min 2)
DDR2_CTL_10_DATA_LO: .word 0x3f150212 #800 //tRC=tRAS+tRP=16+5=21 tMRD=min2 tFAW=18
//DDR2_CTL_10_DATA_LO: .word 0x3f1f0214 #800 //tRC=tRAS+tRP
DDR2_CTL_10_DATA_HI: .word 0x0000003f //yanhua //DQS延迟百分比
DDR2_CTL_11_DATA_LO: .word 0x00000000 //yanhua
DDR2_CTL_11_DATA_HI: .word 0x2c4c0000 //yanhua  0x4060 -- 0x2f4f-ok
DDR2_CTL_12_DATA_LO: .word 0x10000000 #800 //tRAS = 16
//DDR2_CTL_12_DATA_LO: .word 0x18000000 #800 //tRAS
DDR2_CTL_12_DATA_HI: .word 0x002a2a05 #800 //tRFC=42 tRCD=tRP=5
DDR2_CTL_13_DATA_LO: .word 0x002a002a
DDR2_CTL_13_DATA_HI: .word 0x002a002a
DDR2_CTL_14_DATA_LO: .word 0x002a002a
DDR2_CTL_14_DATA_HI: .word 0x002a002a //
DDR2_CTL_15_DATA_LO: .word 0x00000002 //
DDR2_CTL_15_DATA_HI: .word 0x00b40020 //
DDR2_CTL_16_DATA_LO: .word 0x00000087
DDR2_CTL_16_DATA_HI: .word 0x000007ff #no_interrupt
DDR2_CTL_17_DATA_LO: .word 0x44240c30 //EMRS(1)  tREF=3120/1560 
//DDR2_CTL_17_DATA_LO: .word 0x47a40c30
//DDR2_CTL_17_DATA_LO: .word 0x44260c30
DDR2_CTL_17_DATA_HI: .word 0x80808080
DDR2_CTL_18_DATA_LO: .word 0x80808080
DDR2_CTL_18_DATA_HI: .word 0x001c8080
DDR2_CTL_19_DATA_LO: .word 0x00c8006b
DDR2_CTL_19_DATA_HI: .word 0x6d600002 #800	//tRAS_MAX=28000 TPDEX=min2
DDR2_CTL_20_DATA_LO: .word 0x00c8002e #800	//tXSR=min200 tXSNR=tRFC+10(ns)=42+4=46
DDR2_CTL_20_DATA_HI: .word 0x00000000
DDR2_CTL_21_DATA_LO: .word 0x00013880 //tINIT=200(us)x0.4=80000tCK
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
