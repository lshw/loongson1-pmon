#ifndef __MYDEBUG__
#define __MYDEBUG__
.set noreorder
#define CP0_PRID $15
#define BONITO_PCICFG_BASE 0x1fe00000
#define BONITO_PCIMAP_CFG 0x118

    PRINTSTR("press gb to boot\r\npresss h for help\r\n");
#if 0
	la  v0,cache_init
    addu v0,s0
    jal v0
    nop
#endif

bootkeytest:
	bal tgt_testchar
	nop
	beqz v0,bootnow
	nop
mycmd_loop:
	PRINTSTR("A:writeone-testone\r\nB:writesome+-testsome--\r\nC:writesome-testsome-rom\r\nD:writeall-testall\r\nE:writesome-testsome-ram\r\nF:writeall-testall-ram\r\nG:copytest\r\nH:writesomereverse-testsome-rom\r\nI:writesomereverse-testsome-ram\r\nz:read @addr\r\ns:write\r\nv:write then read @addr\r\ng:rwtest\r\nn:write 16M\r\nj:write 0 to a0000000\r\nd:debug\r\nb:boot\r\n");
	bal tgt_getchar
	nop
	li v1,'A'
	beq v0,v1,memtest
	nop
	li v1,'B'
	beq v0,v1,memtest1
	nop
	li v1,'C'
	beq v0,v1,memtest2
	nop
	li v1,'D'
	beq v0,v1,memtest3
	nop
	li v1,'E'
	beq v0,v1,memtest4
	nop
	li v1,'F'
	beq v0,v1,memtest5
	nop
	li v1,'G'
	beq v0,v1,memtest6
	nop
	li v1,'H'
	beq v0,v1,memtest7
	nop
	li v1,'I'
	beq v0,v1,memtest8
	nop
	li v1,'J'
	beq v0,v1,sumrom
	nop
	li v1,'z'
	beq v0,v1,testddrread
	nop
	li v1,'s'
	beq v0,v1,testddrwrite
	nop
	li v1,'v'
	beq v0,v1,testddrwrite1
	nop
	li v1,'g'
	beq v0,v1,rwtest
	nop
	li v1,'n'
	beq v0,v1,testddrwrite0
	nop
	li v1,'d'
	beq v0,v1,mydebug_start
	nop
	li v1,'r'
	beq v0,v1,dummyreadtest
	nop
	li v1,'w'
	beq v0,v1,dummywritetest
	nop
	li v1,'j'
	beq v0,v1,dummywritetest1
	nop
	b bootnow
	nop
dummyreadtest:
	li v0,0xa0000000
1:
	ld v1,(v0)
	addiu v0,8
	b 1b
	nop
dummywritetest:
	li v0,0xa0000000
1:
	sd v1,(v0)
	addiu v0,8
	b 1b
	nop
dummywritetest1:
	li v0,0xa0000000
1:
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	sd zero,(v0)
	b 1b
	nop
memtest:
#include "writeone-testone8.S"	
memtest1:
#include "writesome+-testsome--8.S"
memtest2:
#include "writesome-testsome-rom8.S"
memtest3:
#include "writeall-testall8.S"
memtest4:
#include "writesome-testsome-ram8.S"
memtest5:
#include "writeall-testall-ram8.S"
memtest6:
#include "copytest.S"
memtest7:
#include "writesome-testsome-rom8R.S"
memtest8:
#include "writesome-testsome-ram8R.S"
sumrom:
#include "sumrom.S"


testddrwrite0:
#if 1
	li t7,0xa0000000
	li t6,0xa1000000
0:
	sd t7,(t7)
	addiu t7,8
	bne t6,t7,0b
	nop
	b testddrread
	nop
#endif

testddrwrite:
#if 1
	li t7,0xa0000000
	li t6,0xa0100000
	li t8,1
0:
	move a0,t7
	bal hexserial
	nop
	PRINTSTR("\r\n");
	sb t8,(t7)
	addiu t7,1
	addiu t8,1
	bne t6,t7,0b
	nop
	b testddrread
	nop
#endif
testddrwrite1:
#if 1
	li t7,0xa0000000
	li t6,0xa0000100
	li t8,1
0:
	move a0,t7
	bal hexserial
	nop
	PRINTSTR("\r\n");
	sb t8,(t7)
	addiu t7,1
	addiu t8,1
	bne t6,t7,0b
	nop
	b testddrread
	nop
#endif

testddrread:
1:
	PRINTSTR("press x to begin read ddr\r\n");
	nop
	bal tgt_getchar
	nop
	li v1,'x'
	bne v0,v1,1b
	nop
	PRINTSTR("32 bit address:");
	li a0,8
	bal inputhex
	nop
	move t7,v0
	PRINTSTR("\r\n");
#	li t7,0xa0000000
1:
	lw t8,(t7)
	move a0,t7
	bal hexserial
	nop
	PRINTSTR(":");
	move a0,t8
	bal hexserial
	nop
	PRINTSTR("\r\n");
	addiu t7,4
	b 1b
	nop

rwtest:
1:
	PRINTSTR("press x to begin rwtest ddr\r\n");
	nop
	bal tgt_getchar
	nop
	li v1,'x'
	bne v0,v1,1b
	nop
2:
	PRINTSTR("32 bit address:");
	li a0,8
	bal inputhex
	nop
	move t7,v0
	PRINTSTR("\r\n");
	PRINTSTR("32 bit data:");
	li a0,8
	bal inputhex
	nop
	move t6,v0
	PRINTSTR("\r\nbefore write\r\n");
	sw v0,(t7)
	PRINTSTR("before read\r\n");
	lw t8,(t7)
	PRINTSTR("after read\r\n");
	move a0,t6
	bal hexserial
	nop
	beq t8,t6,3f
	nop
	PRINTSTR("!");
3:
	PRINTSTR("=");
	move a0,t8
	bal hexserial
	nop
	PRINTSTR("@");
	move a0,t7
	bal hexserial
	nop
	PRINTSTR("\r\n");
	b 2b
	nop

mydebug_start:
1:
	PRINTSTR("press x to begin debug\r\n");
	nop
	bal tgt_getchar
	nop
	li v1,'x'
	bne v0,v1,1b
	nop
mydebug_restarth:
	  PRINTSTR("[m|M][1,2,4,8] addr #modify\r\n[d|D][1,2,4,8] addr data[1,2,4,8] #dump\r\n[s|S][1,2,4,8] addr data count inc #set\r\n[c|C][1,2,4,8] addr data count inc #compare\r\n[t|T][1,2,3,8] addr datat count inc #write one then test one\r\ng #go to cmdloop\r\nf:config sdcfg\r\nke [0|1]#cache enable\r\nkf [1|2|3]#cache1 or cache2 or godson1 flush\r\nb msize dmsize cacheon #boot msize(Mbyes)\r\np bus dev func reg#pci config\r\ni slot off count#i2cread\r\n");
mydebug_restart:
	  move t6,zero
	  PRINTSTR("#");
mydebug_restart_flags:
	  bal tgt_getchar
	  nop
	  li v1,'m'
	  beq v0,v1,func_m
	  nop
	  li v1,'d'
	  beq v0,v1,func_d
	  nop
	  li v1,'M'
	  beq v0,v1,func_m
	  nop
	  li v1,'D'
	  beq v0,v1,func_d
	  nop
	  li v1,'c'
	  beq v0,v1,func_s
	  nop
	  li v1,'s'
	  beq v0,v1,func_s
	  nop
	  li v1,'t'
	  beq v0,v1,func_s
	  nop
	  li v1,'C'
	  beq v0,v1,func_s
	  nop
	  li v1,'S'
	  beq v0,v1,func_s
	  nop
	  li v1,'T'
	  beq v0,v1,func_s
	  nop
	  li v1,'g'
	  beq v0,v1,func_g
	  nop
	  li v1,'f'
	  beq v0,v1,func_f
	  nop
	  li v1,0xd
	  beq v0,v1,func_q
	  nop
	  li v1,'-'
	  beq v0,v1,func_flags
	  nop
	  li v1,'k'
	  beq v0,v1,func_k
	  nop
	  li v1,'p'
	  beq v0,v1,func_p
	  nop
	  li v1,'b'
	  beq v0,v1,func_b
	  nop
	  li v1,'i'
	  beq v0,v1,func_i
	  nop
	  li v1,'Q'
	  beq v0,v1,func_Q
	  nop
	  b mydebug_restarth
	  nop
func_flags:
	  move a0,v0
	  bal tgt_putchar
	  nop
	  bal tgt_getchar
	  nop
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
	  li a0,0x20
	  bal tgt_putchar
	  nop
	  b mydebug_restart_flags
	  nop
func_m:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
1:
	  bal tgt_getchar
	  nop
	  li v1,'1'
	  beq v0,v1,1f
	  nop
	  li v1,'2'
	  beq v0,v1,1f
	  nop
	  li v1,'4'
	  beq v0,v1,1f
	  nop
	  li v1,'8'
	  beq v0,v1,1f
	  nop
	  b func_q
	  nop
1:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
	  li a0,0x20
	  bal tgt_putchar
	  nop
	  bal inputaddress
	  nop

m_loop:
	  PRINTSTR("\r\n")
	  move a0,t7
	  bal hexserial
	  nop
	  PRINTSTR(": ")
	  
	  and a0,t6,0xf
	  sll a0,1
	  bal Inputhex
	  nop
	  move t8,v0
	  li v0,'q'
	  beq v0,v1,m_end
	  nop



	  and v0,t6,0xf
	  li v1,2
	  beq v0,v1,2f
	  nop
	  li v1,4
	  beq v0,v1,4f
	  nop
	  li v1,8
	  beq v0,v1,8f
	  nop

1:
	  sb t8,(t7)
	  addiu t7,1
	  b m_loop
	  nop
2:
	  sh t8,(t7)
	  addiu t7,2
	  b m_loop
	  nop
4:
	  sw t8,(t7)
	  addiu t7,4
	  b m_loop
	  nop
8:
	  sd t8,(t7)
	  addiu t7,8
	  b m_loop
	  nop
m_end:
	  PRINTSTR("\r\n")
	  b mydebug_restart
	  nop
	
func_d:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
1:
	  bal tgt_getchar
	  nop
	  li v1,'1'
	  beq v0,v1,1f
	  nop
	  li v1,'2'
	  beq v0,v1,1f
	  nop
	  li v1,'4'
	  beq v0,v1,1f
	  nop
	  li v1,'8'
	  beq v0,v1,1f
	  nop
	  b func_q
	  nop
1:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
	  li a0,0x20
	  bal tgt_putchar
	  nop
	  bal inputaddress
	  nop

	  li a0,0x20
	  bal tgt_putchar
	  nop
	  PRINTSTR("8'h");
	  li a0,8
	  bal inputhex
	  nop
	  move t5,v0
	  and v0,t6,0xf
	  mul  t5,v0

	  move t4,zero

	  beq t5,t4,d_end
	  nop
d_loop:
	  PRINTSTR("\r\n");
	  move a0,t7
	  bal hexserial
	  nop
	  PRINTSTR(": ")
d_loop1: 
	  and v0,t6,0xf
	  li v1,2
	  beq v0,v1,2f
	  nop
	  li v1,4
	  beq v0,v1,4f
	  nop
	  li v1,8
	  beq v0,v1,8f
	  nop

1:
	  lbu a0,(t7)
	  li  a1,2
	  addiu t7,1
	  addiu t4,1
	  b d_disp
	  nop
2:
	  lhu a0,(t7)
	  li  a1,4
	  addiu t7,2
	  addiu t4,2
	  b d_disp
	  nop
4:
	  lw a0,(t7)
	  li  a1,8
	  addiu t7,4
	  addiu t4,4
	  b d_disp
	  nop
8:
	  ld a0,(t7)
	  li  a1,16
	  addiu t7,8
	  addiu t4,8
	  b d_disp
	  nop

d_disp:
	  bal Hexserial
	  nop
	  li a0,0x20
	  bal tgt_putchar
	  nop
	  beq t5,t4,d_end
	  nop
	  andi v0,t4,0xf
	  bnez v0,d_loop1
	  nop
	  b d_loop
	  nop
d_end:
func_q:
	  PRINTSTR("\r\n")
	  b mydebug_restart
	  nop

func_qh:
	  PRINTSTR("\r\n")
	  b mydebug_restarth
	  nop

func_g:
	 b mycmd_loop
	 nop

func_s:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
1:
	  bal tgt_getchar
	  nop
	  li v1,'1'
	  beq v0,v1,1f
	  nop
	  li v1,'2'
	  beq v0,v1,1f
	  nop
	  li v1,'4'
	  beq v0,v1,1f
	  nop
	  li v1,'8'
	  beq v0,v1,1f
	  nop
	  b func_q
	  nop
1:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
	  li a0,0x20
	  bal tgt_putchar
	  nop
	  bal inputaddress
	  nop

	  li a0,0x20
	  bal tgt_putchar
	  nop

	  bal inputdata
	  nop
	  move t5,t8

	  PRINTSTR(" 8'h"); 
	  li a0,8
	  bal Inputhex
	  nop
	  move t4,v0

	  li a0,0x20
	  bal tgt_putchar
	  nop

	  bal inputdata
	  nop
	  PRINTSTR("\r\n");
//t7: addr,t6: type,t5: data, t4: count,t8: datainc
	  beqz t4,func_q
	  nop
sc_loop:
	  srl v0,t6,16
	  and v0,v0,0xff
	  li v1,'v'
	  bne v0,v1,0f
	  nop
	  move a0,t7
	  bal hexserial
	  nop
	  PRINTSTR("\r\n")
0:
      andi v0,t6,0x1000 #s,t or c
	  bnez v0,s_loop
	  nop
	  b c_loop
	  nop
s_loop:
	  and v0,t6,0xf
	  li v1,2
	  beq v0,v1,2f
	  nop
	  li v1,4
	  beq v0,v1,4f
	  nop
	  li v1,8
	  beq v0,v1,8f
	  nop

1:
	  sb t5,(t7)
	  b s_next
	  nop
2:
	  sh t5,(t7)
	  b s_next
	  nop
4:
	  sw t5,(t7)
	  b s_next
	  nop
8:
	  sd t5,(t7)
s_next:
	  andi a0,t6,0x0100 #s or t
	  beqz a0,c_loop
	  nop
	  and a0,t6,0xf
	  addu t7,a0
	  srl a1,t6,16
	  beqz a1,1f
	  nop
	  subu t7,a0
	  subu t7,a0
1:
	  addiu t4,-1
	  addu t5,t8
	  bnez t4,sc_loop
	  nop
	  b func_q
	  nop

c_loop:
	  and v0,t6,0xf
	  li v1,2
	  beq v0,v1,2f
	  nop
	  li v1,4
	  beq v0,v1,4f
	  nop
	  li v1,8
	  beq v0,v1,8f
	  nop

1:
	  lbu t3,(t7)
	  bne t3,t5,c_disp
	  nop
	  b c_next
	  nop
2:
	  lhu t3,(t7)
	  bne t3,t5,c_disp
	  nop
	  b c_next
	  nop
4:
	  lw t3,(t7)
	  bne t3,t5,c_disp
	  nop
	  b c_next
	  nop
8:
	  ld t3,(t7)
	  bne t3,t5,c_disp
	  nop
	  b c_next
	  nop
c_disp:
	  move a0,t3
	  and a1,t6,0xf
	  sll a1,1
	  bal Hexserial
	  nop
	  PRINTSTR("!=")
	  move a0,t5
	  and a1,t6,0xf
	  sll a1,1
	  bal Hexserial
	  nop
	  PRINTSTR(" @ ")
	  move a0,t7
	  bal hexserial
	  nop
	  PRINTSTR("\r\n")
c_next:
	  addu t5,t8
	  and a0,t6,0xf
	  addu t7,a0
	  srl a1,t6,16
	  beqz a1,1f
	  nop
	  subu t7,a0
	  subu t7,a0
1:
	  addiu t4,-1
	  beqz t4,func_q
	  nop
	  b sc_loop
	  nop


#define SDRCFG_INPUT(STR,BIT,WID) \
	  PRINTSTR(STR); \
	  srl a0,t5,BIT; \
	  andi a0,(1<<WID)-1; \
	  li  a1,(WID+3)/4; \
	  bal Hexserial; \
	  nop; \
	  li a0,0x20; \
	  bal tgt_putchar; \
	  nop; \
	  li a0,(WID+3)/4; \
	  bal inputhex; \
	  nop; \
	  li a0,0xd; \
	  beq a0,v1,991f; \
	  nop; \
	  andi v0,(1<<WID)-1; \
	  li v1,(1<<WID)-1; \
	  sll v0,BIT; \
	  sll v1,BIT; \
	  not v1; \
	  and t5,v1; \
	  or t5,v0; \
991:\
	  PRINTSTR("\r\n");

func_f:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop

	  PRINTSTR("\r\n");
	 
	  li a0,0xbff00008
	  lw t5,(a0)

	  SDRCFG_INPUT("0:no;1:yes\r\nDIMM_slot0 have memory?",29,1);
	  SDRCFG_INPUT( \
	  "2'b00£ºDIMM1: 1; DIMM0:1\r\n2'b01£ºDIMM1: 1; DIMM0:2\r\n2'b10£ºDIMM1: 2; DIMM0:1\r\n2'b11£ºDIMM1: 2; DIMM0:2\r\nDIMM0/DIMM1 MOUDLE nums?" \
	  ,27,2);
	  SDRCFG_INPUT("0:sequence 1:interleave\r\ninterleave?",26,1);

	  SDRCFG_INPUT(\
		"BITS Density Org.  Row Addr.  Col Addr.\r\n0000 64Mb 16Mb X 4 DA[11:0] DA[9:0]\r\n     128Mb 16Mb X 8\r\n0001 64Mb 8Mb X 8 DA[11:0] DA[8:0]\r\n     128Mb 8Mb X 16 \r\n0010 64Mb 4Mb X 16 DA[11:0] DA[7:0]\r\n0011 128Mb 32Mb X 4 DA[11:0] DA[11],DA[9:0]\r\n0100 256Mb 64Mb X 4 DA[12:0] DA[11],DA[9:0]\r\n     512Mb 64Mb X 8 \r\n0101 256Mb 32Mb X 8 DA[12:0] DA[9:0]\r\n     512Mb 32Mb X 16\r\n0110 256Mb 16Mb X 16 DA[12:0] DA[8:0]\r\n0111 512Mb 128Mb X 4 DA[12:0] DA[12:11],DA[9:0]\r\n1000 1Gb 256Mb X 4 DA[13:0] DA[12:11],DA[9:0]\r\n1001 1Gb 128Mb X 8 DA[13:0] DA[11],DA[9:0]\r\n1010 1Gb 64Mb X 16 DA[13:0] DA[9:0]\r\nDDR type?\r\n" \
		,22,4);

	  SDRCFG_INPUT(\
		"SDRAM refresh counter\r\n@100MHz:\r\n12'd780    7.8us\r\n12'd1560    15.6us\r\n@133MHz:\r\n12'd1040    7.8us\r\n12'd2080    15.6us\r\n@166MHz:\r\n12'd1300    7.8us\r\n12'd2600    15.6us\r\nTREF:SDRAM refresh counter?" \
		,10,12);

	  SDRCFG_INPUT(\
		"1'b0   2 cycle @DDR100\r\n1'b1   3 cycle @DDR266,DDR33\r\nTRCD:time form ras to cas?" \
		,9,1);

	  SDRCFG_INPUT(\
		"2'b00  Null \r\n2'b01  8 cylces (DDR100)\r\n2'b10  10 cycles(DDR266)\r\n2'b11  12 cycles(DDR333)\r\nTRPC:time from AUTO_REFRESH to ACTIVE?" \
		,7,2);

	  SDRCFG_INPUT(\
		"1'b0   5 cycles(DDR100)\r\n1'b1   7 cycles(DDR266¡¢DDR333)\r\nTRAS:time from ACTIVE to PRECHARGE?" \
		,6,1);

	  SDRCFG_INPUT(\
		"2'b00  1.5 cycles\r\n2'b01  2 cycles\r\n2'b10  2.5 cycles\r\n2'b11  3 cycles\r\nTCAS:time form read to data?" \
		,4,2);

	  SDRCFG_INPUT(\
		"1'b0   2 cycles(DDR100)\r\n1'b1   3 cycles(DDR266¡¢DDR333)\r\nTWR:time from last write data to PRECHARGE?" \
		,3,1);

	  SDRCFG_INPUT(\
		"1'b0   2 cycles(DDR100)\r\n1'b1   3 cycles(DDR266¡¢DDR333)\r\nTRP:PRECHARGE time counter?" \
		,2,1);

	  SDRCFG_INPUT(\
		"2'b00  Null\r\n2'b01  7 cycles(DDR100)\r\n2'b10  9 cycles(DDR266)\r\n2'b11  10cycles(DDR333)\r\nTRC:time from ACTIVE toACTIVE/AUTO_REFRESH?" \
		,0,2);
		move a0,t5
		bal hexserial
		nop
		li a0,0xbff00008
		sw t5,(a0)
		b func_q
		nop

func_k:
	  move a0,v0
	  bal tgt_putchar
	  nop
	  bal tgt_getchar
	  nop
	  li v1,'e'
	  beq v0,v1,func_ke
	  nop
	  li v1,'f'
	  beq v0,v1,func_kf
	  nop
	  b func_q
	  nop
func_ke:
	  move a0,v0
	  bal tgt_putchar
	  nop
	  li a0,0x20
	  bal tgt_putchar
	  nop
	  bal tgt_getchar
	  nop
      mfc0   a0,CP0_CONFIG
      and    a0,a0,0xfffffff8
	  li v1,'0'
	  beq v0,v1,1f
	  nop
	  li v1,'1'
	  beq v0,v1,2f
	  nop
	  b func_qh
	  nop
1:
		move   a0,v0
		bal tgt_putchar
		nop
        or     a0,a0,0x2
        mtc0   a0,CP0_CONFIG
		b func_q
		nop
2:
		move   a0,v0
		bal tgt_putchar
		nop
        or     a0,a0,0x3
        mtc0   a0,CP0_CONFIG
		b func_q
		nop
func_kf:
	  move a0,v0
	  bal tgt_putchar
	  nop

	  li a0,0x20
	  bal tgt_putchar
	  nop
	  bal tgt_getchar
	  nop
	  li v1,'1'
	  beq v0,v1,1f
	  nop
	  li v1,'2'
	  beq v0,v1,2f
	  nop
	  li v1,'3'
	  beq v0,v1,3f
	  nop
	  b func_qh
	  nop
1:
		move   a0,v0
		bal tgt_putchar
		nop
        li    a0,0x80000000
        addu  a1,a0,16384
1:
        .set mips3
        cache  1,0(a0)
        cache  1,1(a0)
        cache  1,2(a0)
        cache  1,3(a0)
        cache  0,(a0)
        .set mips0
        add    a0,a0,32
        bne    a0,a1,1b
        nop
		b func_q
		nop
2:
		move   a0,v0
		bal tgt_putchar
		nop

		.set mips3
		mfc0	a0, CP0_PRID			# read processor ID register
		li		a1, 0x6302				#godson2e prid
		bne	a1, a0,11f 
		nop
		.set mips0
	# godson2e
		li	  a1, 0x80000000
		addu  a0,a1,512*1024
10:
        .set mips3
		cache	3, 0(a1)
		cache	3, 1(a1)
		cache	3, 2(a1)
		cache	3, 3(a1)
        .set mips0
		addu	a1, 32
		bne	    a1,a0, 10b
		nop
11:
		b func_q
		nop
3:
        la  v0,cache_init
        addu v0,s0
        jal v0
        nop
        b func_q
        nop

func_p:
	  sll t6,8
	  or t6,v0
	  move a0,v0
	  bal tgt_putchar
	  nop
#bus,dev,func,reg:t0,t1,t2,t3
	  li a0,0x20
	  bal tgt_putchar
	  nop
	  PRINTSTR("1'h"); #bus
	  li a0,1
	  bal inputhex
	  nop
	  move t0,v0

	  li a0,0x20
	  bal tgt_putchar
	  nop
	  PRINTSTR("2'h"); #dev
	  li a0,2
	  bal inputhex
	  nop
	  move t1,v0

	  li a0,0x20
	  bal tgt_putchar
	  nop
	  PRINTSTR("1'h"); #func
	  li a0,1
	  bal inputhex
	  nop
	  move t2,v0

	  li a0,0x20
	  bal tgt_putchar
	  nop
	  PRINTSTR("2'h"); #reg
	  li a0,2
	  bal inputhex
	  nop
	  move t3,v0
	  
#define	PCI_STATUS_MASTER_ABORT			0x20000000
#define	PCI_STATUS_MASTER_TARGET_ABORT		0x10000000
	  li a0,0xbfe00000
	  li v0,PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT
	  lw v1,4(a0)
	  or v1,v0
	  sw v1,4(a0)
	
	  move t7,zero

	  li v0,1
	  addu v1,t1,11
	  sll v0,v1

	  beqz t0,1f
	  nop
	  sll t7,t0,16
	  sll v0,t1,11
1:
	  or t7,v0
	  sll v0,t2,8
	  or t7,v0
	  or t7,t3

	  srl v0,t7,16
	  beqz t0,1f
	  nop
	  or v0,0x1000
1:
	  li a0,0xbfe00000
	  sw v0,BONITO_PCIMAP_CFG(a0)

	  PRINTSTR("\r\nmap address=");

	  li v0,BONITO_PCICFG_BASE|0xa0000000
	  and v1,t7,0xfffc
	  or a0,v0,v1
	  bal hexserial
	  nop
	  b func_q
	  nop
	  

func_b:
	move a0,v0
	bal tgt_putchar
	nop
	PRINTSTR(" 3'h");
	li a0,3
	bal inputhex
	nop
	move t5,v0  #memory size in bytes
	
	PRINTSTR(" 3'h");
	li a0,3
	bal inputhex
	nop
	move t6,v0  #memory size in bytes

	PRINTSTR(" 1'h");
	li a0,1
	bal inputhex
	nop
	move t7,v0    #cacheon
	

	TTYDBG("\r\nCopy PMON to execute location...\r\n")
	la	a0, start
	li	a1, 0xbfc00000
	la	a2, _edata
        or      a0, 0xa0000000
        or      a2, 0xa0000000

	/* copy text section */
1:	lw	a3, 0(a1)
	nop
	sw	a3, 0(a0)
	addu	a0, 4
	addu	a1, 4
	bne	a2, a0, 1b
	nop

	PRINTSTR("\ncopy text section done.\r\n")
	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
	or      a0, 0xa0000000
    or      a2, 0xa0000000
2:	sw	zero, 0(a0)
	bne	a2, a0, 2b
	addu	a0, 4

	TTYDBG("Copy PMON to execute location done.\r\n")
	li	a0, 0
	move	a0,t5
	move	a1,t6
	move 	a2,t7

	la	v0, initmips
	jalr	v0
	nop
	

func_i:
	move a0,v0
	bal tgt_putchar
	nop
	PRINTSTR(" 1'h");
	li a0,1
	bal inputhex
	nop
	move t5,v0  #chip
	
	PRINTSTR(" 2'h");
	li a0,2
	bal inputhex
	nop
	move t6,v0  #offs

	PRINTSTR(" 3'h");
	li a0,3
	bal inputhex
	nop
	move t7,v0  #count
	
	sll t5,1
	or t5,0xa0

	li t4,0
1:
	and v0,t4,0xf
	bnez v0,2f
	nop
	PRINTSTR("\r\n");
	move a0,t6
	li a1,2
	bal Hexserial
	nop
	PRINTSTR(": ");
2:
	move a0,t5
	move a1,t6
#	bal i2cread
	nop
	move a0,v0
	li a1,2
	bal Hexserial
	nop
	li a0,0x20
	bal tgt_putchar
	nop
	addi t6,1
	addi t4,1
	blt t4,t7,1b
	nop
	PRINTSTR("\r\n")
	b mydebug_restart
	nop

func_Q:
 jr t9
 nop




//---------------------------------------------------------------------------------------------------------


inputaddress:
	  move s7,ra
	  andi v0,t6,0x2000
	  beqz v0,1f
	  nop
	  PRINTSTR("8'h");
	  li a0,8
	  bal inputhex
	  nop
	  b 2f
	  nop
1:
	  PRINTSTR("16'h");
	  li a0,16
	  bal Inputhex
	  nop
2:
	  move t7,v0
	  jr s7
	  nop

inputdata:
	  move s7,ra
	  andi a0,t6,0x0f
	  li v0,8
	  beq a0,v0,1f
	  nop
	  sll a0,a0,1 
	  addiu a0,'0'
	  bal tgt_putchar
	  nop
	  PRINTSTR("'h");
	  andi a0,t6,0x0f
	  sll a0,a0,1 
	  b 2f
	  nop
1:
	  PRINTSTR("16'h");
	  li a0,16
2:
	  bal Inputhex
	  nop
	  move t8,v0
	  jr s7
	  nop

LEAF(inputhex)
.set noreorder
	move a3,ra
	move a1,a0
	move a2,zero
1:
	bal tgt_getchar
	nop
	li v1,'q'
	beq v0,v1,3f
	nop
	li v1,0x20
	beq v1,v0,3f
	nop
	li v1,0xd
	beq v1,v0,3f
	nop
	li v1,0x8 //backspace
	beq v1,v0,4f
	nop
	li v1,'x'
	beq v1,v0,4f
	nop

	slt v1,v0,'0'
	bnez v1,1b
	nop
	slt v1,v0,'9'+1
	bnez v1,2f
	move v1,v0
	slt v1,v0,'a'
	bnez v1,1b
	nop
	slt v1,v0,'f'+1
	beqz v1,1b
	move v1,v0
	addi v0,10-'a'+'0'
2:	
	addi v0,0-'0'
	sll a2,4
	or a2,v0,a2	
	move a0,v1
	bal tgt_putchar
	nop
	addi a1,-1
	bnez a1,1b
	nop
	li v1,0
3:
	move v0,a2
	jr a3
	nop
4:
	srl a2,4
	addu a1,1
	li a0,'\b'
	bal tgt_putchar
	nop
	li a0,0x20
	bal tgt_putchar
	nop
	li a0,'\b'
	bal tgt_putchar
	nop
	b 1b
	nop
END(inputhex)

LEAF(Inputhex)
	move a3,ra
	move a1,a0
	move a2,zero
1:
	bal tgt_getchar
	nop
	li v1,'q'
	beq v0,v1,3f
	nop
	li v1,0x20
	beq v1,v0,3f
	nop
	li v1,0xd
	beq v1,v0,3f
	nop
	li v1,0x8 //backspace
	beq v1,v0,4f
	nop
	li v1,'x'
	beq v1,v0,4f
	nop

	slt v1,v0,'0'
	bnez v1,1b
	nop
	slt v1,v0,'9'+1
	bnez v1,2f
	move v1,v0
	slt v1,v0,'a'
	bnez v1,1b
	nop
	slt v1,v0,'f'+1
	beqz v1,1b
	move v1,v0
	addi v0,10-'a'+'0'
2:	
	addi v0,0-'0'
	sll a2,4
	or a2,v0,a2	
	move a0,v1
	bal tgt_putchar
	nop
	addi a1,-1
	bnez a1,1b
	nop
	li v1,0
3:
	move v0,a2
	jr a3
	nop
4:
	srl a2,4
	addu a1,1
	li a0,'\b'
	bal tgt_putchar
	nop
	li a0,0x20
	bal tgt_putchar
	nop
	li a0,'\b'
	bal tgt_putchar
	nop
	b 1b
	nop
END(Inputhex)

LEAF(Hexserial)
	.set noat
	move 	a3,a1
	move	a2, ra
	move	a1, a0
	sll v0,a3,2
	li AT,32
	subu v0,AT,v0
	sllv a1,a1,v0

	addiu a3,-1
1:
	sll	AT, a1, 4
    srl a0,a1,28
	or      a0,a0,AT
	move	a1, a0
	and	a0, 0xf
	la	v0, hexchar
	addu	v0, s0
	addu	v0, a0
	lbu	a0, 0(v0)
	bal	tgt_putchar
    nop

	bnez	a3, 1b
	addu	a3, -1

	j	a2
	nop
	.set at
END(Hexserial)


LEAF(mydebug_main)
move t9,ra
b mydebug_restarth
nop
END(mydebug_main)


	.text
    .align 2
.set noreorder
#endif
