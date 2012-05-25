/**********************************
    Author: chenxinke
    Date:   20120425
    v1.0
        support 2F MC
**********************************/
#define CONFIG_BASE 0xaffffe00

        .global ddr2_config
        .ent    ddr2_config
        .set    noreorder
#        .set    mips3
ddr2_config:
    la      t0, ddr2_reg_data
    addu    t0, t0, s0
    li      t1, 0x1d
    li      t2, CONFIG_BASE
1:
    lw      a1, 0x0(t0)
    sw      a1, 0x0(t2)
    lw      a1, 0x4(t0)
    sw      a1, 0x4(t2)
    subu    t1, t1, 0x1
    addiu   t0, t0, 0x8
    bne     t1, $0, 1b
    addiu   t2, t2, 0x10
    sync

#ifdef DEBUG_DDR_PARAM   //debug
//input once, change all byte lanes parameters.
    /* store the ra */
    move	t1, ra

    li      t2, CONFIG_BASE
10:
    PRINTSTR("\r\nChange parameters:\r\n0--skip;1--rdlvl_dqs_delay;2--wrlvl_delay;3--wrlvl_dq_delay;\r\n");
    li      t6, 0x00
    bal     inputaddress
    nop
    beqz    v0, 88f;
    nop
    move    t5, v0
    PRINTSTR("\r\nPlease input the data-hex: ");
    li 	    t6, 0x00
    bal	    inputaddress
    nop
    move    a2, t5
#ifndef DLL_BYPASS_MODE
    and     t5, v0, 0x7f
#else
    and     t5, v0, 0x1ff
#endif
/*****************
a2: change select
t5: value
*****************/

//!!!!!note: don't change the switch order of the code bellow, because we use
//add instr to change a1 instead of li  instr to reduce code size.
    li      a1, 0x1
    beq     a2, a1, 1f;
    nop
    addu    a1, a1, 0x1
    beq     a2, a1, 2f;
    nop
    addu    a1, a1, 0x1
    beq     a2, a1, 3f;
    nop
    PRINTSTR("\r\n--------Wrong selection: no parameter will be changed.");
    b       40f
    nop
1:
    lw      a1, RDLVL_DELAY_8_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_8_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_8_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_8_ADDR(t2)

    lw      a1, RDLVL_DELAY_7_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_7_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_7_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_7_ADDR(t2)

    lw      a1, RDLVL_DELAY_6_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_6_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_6_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_6_ADDR(t2)

    lw      a1, RDLVL_DELAY_5_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_5_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_5_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_5_ADDR(t2)

    lw      a1, RDLVL_DELAY_4_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_4_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_4_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_4_ADDR(t2)

    lw      a1, RDLVL_DELAY_3_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_3_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_3_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_3_ADDR(t2)

    lw      a1, RDLVL_DELAY_2_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_2_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_2_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_2_ADDR(t2)

    lw      a1, RDLVL_DELAY_1_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_1_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_1_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_1_ADDR(t2)

    lw      a1, RDLVL_DELAY_0_ADDR(t2)
    li      a2, RDLVL_DELAY_MASK
    sll     a2, a2, RDLVL_DELAY_0_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, RDLVL_DELAY_0_OFFSET
    or      a1, a1, a2
    sw      a1, RDLVL_DELAY_0_ADDR(t2)
    b       40f
    nop
2:
    lw      a1, WRLVL_DELAY_0_ADDR(t2)
    li      a2, WRLVL_DELAY_MASK
    sll     a2, a2, WRLVL_DELAY_0_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, WRLVL_DELAY_0_OFFSET
    or      a1, a1, a2
    sw      a1, WRLVL_DELAY_0_ADDR(t2)
    b       40f
    nop
3:
    lw      a1, WRLVL_DQ_DELAY_0_ADDR(t2)
    li      a2, WRLVL_DQ_DELAY_MASK
    sll     a2, a2, WRLVL_DQ_DELAY_0_OFFSET
    not     a2, a2
    and     a1, a1, a2
    sll     a2, t5, WRLVL_DQ_DELAY_0_OFFSET
    or      a1, a1, a2
    sw      a1, WRLVL_DQ_DELAY_0_ADDR(t2)
    b       40f
    nop
40:
    sync
    b       10b
    nop
88:

    PRINTSTR("\r\nChange single parameters:");
1:
    PRINTSTR("\r\nPlease input the register number you want to change!!!(0xfff:jump out.): ");
    li 	    t6, 0x00
    bal	    inputaddress	#input value stored in v0
    nop
    move    t5, v0	#store v0 to t5
    
    li      a1, 0x1c0
    bgt     t5, a1, 2f	#if input address exceed range,jump out
    nop
    addu    t5, t5, t2

    PRINTSTR("\r\nPlease input the data-hex: ");
    li  	t6, 0x00
    bal	    inputaddress	#input value stored in v0
    nop
    sw	    v0, 0x0(t5)	#v0 is the input value

    //print the new register value
    move    t6, t5
    PRINTSTR("\r\nRegister 0x")
    subu    a0, t5, t2
    bal     hexserial
    nop
    PRINTSTR(": ")
    lw      a0, 0x0(t6)
    bal     hexserial
    nop

    b	    1b
    nop
2:	
    /* recover the ra */
    move	ra, t1
#endif
    ############start##########
    li      t2, CONFIG_BASE
    lw      a1, 0x34(t2)
    li 		a2, 0x00000100
    or		a1, a1, a2
    sw		a1, 0x34(t2)
    sync

    //wait initialization complete 
#if 1 
 //delay
    li      v0, 0x100
1:
    bnez    v0, 1b
    addi    v0, v0, -1
    nop
#endif

    //wait dlllockreg
    li      t2, CONFIG_BASE
1:
    lw	    a1, 0x10(t2)
    and     a1, 0x1
    beqz    a1, 1b
    nop

    //wait int_status
1:
    lw	    a1, 0x104(t2)
    srl     a1, a1, 16
    andi    a1, a1, 0x100
    beqz    a1, 1b
    nop

    jr      ra
    nop
    .end    ddr2_config
