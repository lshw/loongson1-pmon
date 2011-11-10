
//extern onintr(int a,int *b);
//#include "mips/cpu.h"
#include <prid.h>
#include <stdio.h>	
#include <sys/sys/signal.h>
#ifdef R3081
#include "r3081.h"
#endif
#ifdef R3041
#include "r3041.h"
#endif

#include "ri.h"

#ifdef	GODSONEV1
int global_div_num=0;
#endif

extern int do_ri (struct pt_regs *xcp);
int __compute_return_epc(struct pt_regs *regs);
static unsigned long 
mips_get_word_l(struct pt_regs *xcp, void *va, int *perr)
{
		*perr = 0;
		return(*(unsigned long *)va);
}

static int 
mips_put_word_l(struct pt_regs *xcp, void *va, unsigned long val)
{
		*(unsigned long *)va = val;
		return 0;
}

static int emu_lwl(struct pt_regs * regs,mips_instruction ir,vaddr_t_l emulpc)
{ 	int err = 0;
	/*the "ir" is the instruction causing the exception*/
	/*get the real address,perhaps the address is not word aligned*/
	void *va = REG_TO_VA_l (regs->regs[MIPSInst_RS(ir)])+ MIPSInst_SIMM(ir);
	
	unsigned long addr = 0;
	unsigned long emul_pc = (unsigned long)emulpc;
	
	unsigned long little_three_bits;
	unsigned long value,value_tmp;

//	printf("emu_lwl\r\n");

	/*compute the correct position in the RT*/
	/*note !!!!: we have supposed the CPU is little_Endianness and status regiester's RE bit =0 */
	/*little Endianness*/
	little_three_bits = (unsigned long)va&(0x7);
	value_tmp = regs->regs[MIPSInst_RT(ir)];
	switch(little_three_bits) {
		case 0:
		case 4:
		 /*must check lwl valid*/
		 addr = (unsigned long) va;
		 check_axs(emul_pc,addr,4);
		 value = mips_get_word_l(regs,va,&err);
		 if(err){
			 return SIGBUS;
		 }
		 value<<=24;
		 value_tmp &= 0xffffff;
	         regs->regs[MIPSInst_RT(ir)] =value_tmp|value;
			break;
			
		case 1:
		case 5:
		addr = (unsigned long)va -1;
		check_axs(emul_pc,addr,4);
		value = mips_get_word_l(regs,(void *)((unsigned long) va-1),&err);
		if(err){
			return SIGBUS;
		}
		value<<=16;
		value_tmp&=0xffff;
		regs->regs[MIPSInst_RT(ir)] =value_tmp|value;
			break;

		case 2:
		case 6:
		addr = (unsigned long)va - 2;
		check_axs(emul_pc,addr,4);
		value = mips_get_word_l(regs,(void *)((unsigned long)va-2),&err);
		if(err){
			return SIGBUS;

		}
		value<<=8;
		value_tmp &= 0xff;
		regs->regs[MIPSInst_RT(ir)] =value_tmp|value;
			break;
			
		case 3:
		case 7:
		addr = (unsigned long)va - 3;
		check_axs(emul_pc,addr,4);
		value = mips_get_word_l(regs,(void *)((unsigned long)va-3),&err);
		if(err){
			return SIGBUS;
		};
		regs->regs[MIPSInst_RT(ir)] = value;
			break;
	} /*swith ended*/
	return 0;

}

static int emu_lwr(struct pt_regs *regs,mips_instruction ir,vaddr_t_l emulpc)
{ 	int err = 0;
	/*the "ir" is the instruction causing the exception*/
	/*get the real address,perhaps the address is not word aligned*/
	void *va = REG_TO_VA_l (regs->regs[MIPSInst_RS(ir)])
		+ MIPSInst_SIMM(ir);
	unsigned long addr;
	unsigned long emul_pc = (unsigned long)emulpc;
	unsigned long little_three_bits;
	unsigned long value,value_tmp;

//	printf("emu_lwr\r\n");

	/*compute the correct position in the RT*/
	/*note !!!!: we have supposed the CPU is little_Endianness and status regiester's RE bit =0 */
	little_three_bits = (unsigned long)va&(0x7);
	value_tmp = regs->regs[MIPSInst_RT(ir)];
	switch(little_three_bits) {
		case 0:
		case 4:
		 /*must check lwl valid*/
		addr = (unsigned long)va ;
		check_axs(emul_pc,addr,4);
		 value = mips_get_word_l(regs,va,&err);
		 if(err){
			 return SIGBUS;
		 }
	         regs->regs[MIPSInst_RT(ir)] =value;
			break;
			
		case 1:
		case 5:
		addr = (unsigned long)va -1;
		check_axs(emul_pc,addr,4);
		value = mips_get_word_l(regs,(void *)((unsigned long)va-1),&err);
		if(err){
			return SIGBUS;
		}
		value>>=8;
		value_tmp&=0xff000000;
		regs->regs[MIPSInst_RT(ir)] =value_tmp|value;
			break;

		case 2:
		case 6:
		addr = (unsigned long)va-2;
		check_axs(emul_pc,addr,4);
		value = mips_get_word_l(regs,(void *)((unsigned long)va-2),&err);
		if(err){
			return SIGBUS;

		}
		value>>=16;
		value_tmp &= 0xffff0000;
		regs->regs[MIPSInst_RT(ir)] =value_tmp|value;
			break;
			
		case 3:
		case 7:
		addr = (unsigned long)va -3;
		check_axs(emul_pc,addr,4);
		value = mips_get_word_l(regs,(void *)((unsigned long)va-3),&err);
		if(err){
			return SIGBUS;
		};
		value>>=24;
		value_tmp &= 0xffffff00;
		regs->regs[MIPSInst_RT(ir)] = value_tmp|value;
			break;
	} /*swith ended*/
	return 0;
}

static int emu_swl(struct pt_regs *regs,mips_instruction ir, vaddr_t_l emulpc)
{
	int err = 0;
	/*the "ir" is the instruction causing the exception*/
	/*get the real address,perhaps the address is not word aligned*/
	void *va = REG_TO_VA_l (regs->regs[MIPSInst_RS(ir)])
		+ MIPSInst_SIMM(ir);
	unsigned long addr;
	unsigned long emul_pc = (unsigned long)emulpc;
	unsigned long little_three_bits;
	unsigned long value,value_tmp;

//	printf("emu_swl\r\n");

	/*compute the correct position in the RT*/
	/*note !!!!: we have supposed the CPU is little_Endianness and status re
	* giester's RE bit =0 */
	little_three_bits = (unsigned long)va&(0x7);
	value_tmp = regs->regs[MIPSInst_RT(ir)];
	switch(little_three_bits) {
		case 0:
		case 4:
			addr = (unsigned long)va;
			check_axs(emul_pc,addr,4);
			value_tmp >>= 24;
			value = mips_get_word_l(regs,va,&err);
			if(err){
				return SIGBUS;
			}
			value &=0xffffff00;
			value |= value_tmp;
			if(mips_put_word_l(regs,va,value)){
				return SIGBUS;
			}
			break;
		case 1:
		case 5:
			addr = (unsigned long)va -1;
			check_axs(emul_pc,addr,4);
			value_tmp >>= 16;
			value = mips_get_word_l(regs,(void *)((unsigned long)va-1),&err);
			if(err){
				return SIGBUS;
			}
			value &=0xffff0000;
			value |= value_tmp;
			if(mips_put_word_l(regs,(void *)((unsigned long)va-1),value)){
				return SIGBUS;
			}
			break;
		case 2:
		case 6:
			addr = (unsigned long)va - 2;
			check_axs(emul_pc,addr,4);
			value_tmp >>= 8;
			value = mips_get_word_l(regs,(void *)((unsigned long)va-2),&err);
			if(err){
				return SIGBUS;
			}
			value &=0xff000000;
			value |= value_tmp;
			if(mips_put_word_l(regs,(void *)((unsigned long)va-2),value)){
				return SIGBUS;
			}
			break;
		case 3:
		case 7:
			addr = (unsigned long)va - 3;
			check_axs(emul_pc,addr,4);
			value = value_tmp;
						
			if(mips_put_word_l(regs,(void *)((unsigned long)va-3),value)){
				return SIGBUS;
			}
			break;
	}
	return 0;

}

static int emu_swr(struct pt_regs *regs,mips_instruction ir, vaddr_t_l emulpc)
{
	int err = 0;
	/*the "ir" is the instruction causing the exception*/
	/*get the real address,perhaps the address is not word aligned*/
	void *va = REG_TO_VA_l (regs->regs[MIPSInst_RS(ir)])
		+ MIPSInst_SIMM(ir);
	unsigned long addr;
	unsigned long emul_pc = (unsigned long)emulpc;

	
	unsigned long little_three_bits;
	unsigned long value,value_tmp;
	
//	printf("emu_swr\r\n");

	/*compute the correct position in the RT*/
	/*note !!!!: we have supposed the CPU is little_Endianness and status re
	* giester's RE bit =0 */
	little_three_bits = (unsigned long)va&(0x7);
	value_tmp = regs->regs[MIPSInst_RT(ir)];
	switch(little_three_bits) {
		case 0:
		case 4:
			addr = (unsigned long) va;
			check_axs(emul_pc,addr,4);
			value = value_tmp;
			if(mips_put_word_l(regs,va,value)){
				return SIGBUS;
			}
			break;
		case 1:
		case 5:
			addr = (unsigned long)va -1;
			check_axs(emul_pc,addr,4);
			value_tmp <<= 8;
			value = mips_get_word_l(regs,(void *)((unsigned long)va-1),&err);
			if(err){
				return SIGBUS;
			}
			value &=0xff;
			value |= value_tmp;
			if(mips_put_word_l(regs,(void *)((unsigned long)va-1),value)){
				return SIGBUS;
			}
			break;
		case 2:
		case 6:
			addr = (unsigned long)va - 2;
			check_axs(emul_pc,addr,4);
			value_tmp <<= 16;
			value = mips_get_word_l(regs,(void *)((unsigned long)va-2),&err);
			if(err){
				return SIGBUS;
			}
			value &=0xffff;
			value |= value_tmp;
			if(mips_put_word_l(regs,(void *)((unsigned long)va-2),value)){
				return SIGBUS;
			}
			break;
		case 3:
		case 7:
			addr = (unsigned long)va -3;
			check_axs(emul_pc,addr,4);
			value_tmp <<= 24;
			value = mips_get_word_l(regs,(void *)((unsigned long)va-3),&err);
			if(err){
				return SIGBUS;
			}

			value &= 0xffffff;
			value |= value_tmp;			
			if(mips_put_word_l(regs,(void *)((unsigned long)va-3),value)){
				return SIGBUS;
			}
			break;
	}
	return 0;

}



static int emu_div(struct pt_regs *regs,mips_instruction ir)
{
	int x,y;
	int flag = 0;
	int quotient = 0,remainder = 0;
	unsigned int absx,absy,absquotient = 0,absremainder = 0,bm = 1;
	/*the "ir" is the instruction causing the exception*/
	x = regs->regs[MIPSInst_RS(ir)];
	y = regs->regs[MIPSInst_RT(ir)];
	
#ifdef __test_ri__
	//printf("now in function:emu_div().\r\n");
#endif

	if( y == 0 ) {/*overflow*/
		return SIGABRT;
	}
	/*x and y 符号是否不同*/
	flag = (x&0x80000000)^(y&0x80000000);
	
	/*get the abs(x)*/
	if(x<0){
		absx = (unsigned int)-x;
	}else {
		absx = (unsigned int)x;
	}
	
	/*get the abs(y)*/
	if(y<0){
		absy = (unsigned int) -y;
	}else {
		absy = (unsigned int)y;
	}
	
	/*caculate the absx/absy*/		
	if(absx<absy) {/*don't need to calculate*/
		absquotient = 0;
		absremainder = absx;
		goto end;
	}
       
	while(!(absy&0x80000000))
	{	absy<<=1;
		if(absx<absy){
			absy>>= 1;
			break;
		}
		bm<<=1;
	}

	for(;bm;bm>>=1){
		if(absx>=absy){
			absx -= absy;
			absquotient |= bm;
			if(absx == 0)
				break;
		}
		absy >>= 1;
	}
	absremainder = absx;

end:	
	if( flag ){/*符号相异*/
		quotient = -absquotient;
		remainder = x-quotient*y;
	}else {
		quotient = absquotient;
		remainder = x - quotient*y;
	}

	regs->lo =(unsigned long)quotient;
	regs->hi = (unsigned long)remainder;
	
#ifdef __test_ri__
//	printf("x is: %d\r\n",x);
//	printf("y is: %d\r\n",y);
//	printf("result is: %d (:\r\n",quotient);
#endif

	return  0;
}

static int emu_divu(struct pt_regs *regs,mips_instruction ir)
{ 
	unsigned int x,y,bm=1;
	unsigned int quotient = 0,remainder = 0;

	/*the "ir" is the instruction causing the exception*/
	x = regs->regs[MIPSInst_RS(ir)];
	y = regs->regs[MIPSInst_RT(ir)];

	if( y == 0 ) {/*overflow*/
		return SIGABRT;
	}
	
	if(x<y) {/*don't need to calculate*/
		quotient = 0;
		remainder = x;
		goto end;
	}
		
	while(!(y&0x80000000))
	{	y<<=1;
		if(x<y){
			y>>= 1;
			break;
		}
		bm<<=1;
	}
	
	for(;bm;bm>>=1){
		if(x>=y){
			x -= y;
			quotient |= bm;
			if(x == 0)
				break;
		}
		y >>= 1;

	}
	remainder = x;
end: 
	regs->lo = quotient;
	regs->hi = remainder;
	return  0;
	
}

/*
 * Compute the return address and do emulate branch simulation, if required.
 */
#define EFAULT 1
int __compute_return_epc(struct pt_regs *regs)
{
	unsigned int *addr, bit, fcr31;
	long epc;
	mips_instruction insn;

	epc = regs->cp0_epc;
	if (epc & 3) {
		printf("%s: unaligned epc - sending SIGBUS.\n");
//		force_sig(SIGBUS, current);
		return -EFAULT;
	}

	/*
	 * Read the instruction
	 */
	addr = (unsigned int *) (unsigned long) epc;
#if 0
	if (__get_user(insn, addr)) {
		printf("%s: bad epc value - sending SIGSEGV.\n");
//		force_sig(SIGSEGV, current);
		return -EFAULT;
	}
#endif
//bjzheng add __get_user is prevent page_fault exception,if this occurs,load from disk,but now my whole code is in ram.
	insn=*addr;
//	printf("instruction is %x",insn);
	regs->regs[0] = 0;
	switch (MIPSInst_OPCODE(insn)) {
	/*
	 * jr and jalr are in r_format format.
	 */
	case spec_op:
		switch (MIPSInst_FUNC(insn)) {
		case jalr_op:
			regs->regs[MIPSInst_RD(insn)] = epc + 8;
			/* Fall through */
		case jr_op:
			regs->cp0_epc = regs->regs[MIPSInst_RS(insn)];
			break;
		}
		break;

	/*
	 * This group contains:
	 * bltz_op, bgez_op, bltzl_op, bgezl_op,
	 * bltzal_op, bgezal_op, bltzall_op, bgezall_op.
	 */
	case bcond_op:
		switch (MIPSInst_RT(insn)) {
	 	case bltz_op:
		case bltzl_op:
			if ((long)regs->regs[MIPSInst_RS(insn)] < 0)
				epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
			else
				epc += 8;
			regs->cp0_epc = epc;
			break;

		case bgez_op:
		case bgezl_op:
			if ((long)regs->regs[MIPSInst_RS(insn)] >= 0)
				epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
			else
				epc += 8;
			regs->cp0_epc = epc;
			break;

		case bltzal_op:
		case bltzall_op:
			regs->regs[31] = epc + 8;
			if ((long)regs->regs[MIPSInst_RS(insn)] < 0)
				epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
			else
				epc += 8;
			regs->cp0_epc = epc;
			break;

		case bgezal_op:
		case bgezall_op:
			regs->regs[31] = epc + 8;
			if ((long)regs->regs[MIPSInst_RS(insn)] >= 0)
				epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
			else
				epc += 8;
			regs->cp0_epc = epc;
			break;
		}
		break;

	/*
	 * These are unconditional and in j_format.
	 */
	case jal_op:
		regs->regs[31] = regs->cp0_epc + 8;
	case j_op:
		epc += 4;
		epc >>= 28;
		epc <<= 28;
		epc |= (MIPSInst_JTARGET(insn) << 2);
		regs->cp0_epc = epc;
		break;

	/*
	 * These are conditional and in i_format.
	 */
	case beq_op:
	case beql_op:
		if (regs->regs[MIPSInst_RS(insn)] ==
		    regs->regs[MIPSInst_RT(insn)])
			epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
		else
			epc += 8;
		regs->cp0_epc = epc;
		break;

	case bne_op:
	case bnel_op:
		if (regs->regs[MIPSInst_RS(insn)] !=
		    regs->regs[MIPSInst_RT(insn)])
			epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
		else
			epc += 8;
		regs->cp0_epc = epc;
		break;

	case blez_op: /* not really i_format */
	case blezl_op:
		/* rt field assumed to be zero */
		if ((long)regs->regs[MIPSInst_RS(insn)] <= 0)
			epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
		else
			epc += 8;
		regs->cp0_epc = epc;
		break;

	case bgtz_op:
	case bgtzl_op:
		/* rt field assumed to be zero */
		if ((long)regs->regs[MIPSInst_RS(insn)] > 0)
			epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
		else
			epc += 8;
		regs->cp0_epc = epc;
		break;

	/*
	 * And now the FPA/cp1 branch instructions.
	 */
	case cop1_op:
#ifdef CONFIG_MIPS_FPU_EMULATOR
		if(!(mips_cpu.options & MIPS_CPU_FPU)) 
			fcr31 = current->tss.fpu.soft.sr;
		else
#endif
		asm ("cfc1\t%0,$31":"=r" (fcr31));
		bit = (MIPSInst_RT(insn) >> 2);
		bit += (bit != 0);
		bit += 23;
		switch (MIPSInst_RT(insn)) {
		case 0:	/* bc1f */
		case 2:	/* bc1fl */
			if (~fcr31 & (1 << bit))
				epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
			else
				epc += 8;
			regs->cp0_epc = epc;
			break;

		case 1:	/* bc1t */
		case 3:	/* bc1tl */
			if (fcr31 & (1 << bit))
				epc = epc + 4 + (MIPSInst_SIMM(insn) << 2);
			else
				epc += 8;
			regs->cp0_epc = epc;
			break;
		}
		break;
	}

	return 0;
}

int do_ri (struct pt_regs *xcp)
{
	mips_instruction ir;
	vaddr_t_l emulpc;
	vaddr_t_l contpc;
	int err = 0;
	int sig;

	ir = mips_get_word_l(xcp, REG_TO_VA_l xcp->cp0_epc, &err);
	if (err) {
       		return SIGBUS;
	}
         
  /* XXX NEC Vr54xx bug workaround */
/*  if ((xcp->cp0_cause & CAUSEF_BD) && !isBranchInstr (&ir))
	xcp->cp0_cause &= ~CAUSEF_BD;*/
	
	if (xcp->cp0_cause & CAUSEF_BD) {

       /* The instruction to be emulated is in a branch delay slot
       * which means that we have to  emulate the branch instruction
       * BEFORE we do the emulating instruction.
       * This branch could be a COP1 branch
       */

       		emulpc = REG_TO_VA_l(xcp->cp0_epc + 4); /* Snapshot emulation target */

#ifndef __bjzheng__
       if( __compute_return_epc(xcp)) {/*compute the return address*/
#ifdef DBG
		_mon_printf ("failed to emulate branch at %p\n",
				REG_TO_VA_l (xcp->cp0_epc));
#endif
		return -1;;
       }
#endif

		ir = mips_get_word_l(xcp, emulpc, &err);
		if (err) {
			return SIGBUS;
		}
	        contpc = REG_TO_VA_l xcp->cp0_epc;
	
	} else { /* not in the Branch delay slot*/
		emulpc = REG_TO_VA_l xcp->cp0_epc;
		contpc = REG_TO_VA_l xcp->cp0_epc + 4;
		}

     	switch(MIPSInst_OPCODE(ir)) {
		case lwl_op: /*lwl instruction*/
			sig = emu_lwl(xcp,ir,emulpc);
			if( sig!=0) { /*emul has failed*/
				return sig;
			}
			break;
		  
		case lwr_op:/*lwr instruction*/
			sig = emu_lwr(xcp,ir,emulpc);
			if ( sig != 0){
			/*emulate has failed!\n");*/
				return sig;
		  	}
			break;
		  
		case swl_op:
			sig = emu_swl(xcp,ir,emulpc);
			if( sig!=0 ) { /*emul has failed!*/
				printf("emu_swl error\r\n");
				return sig;
			}
			break;
		
		case swr_op:
			sig = emu_swr(xcp,ir,emulpc);
			if( sig!=0 ) { /*emul has failed!*/
				printf("emu_swr error\r\n");
				return sig;
			}
			break;
		case spec_op:
			switch (MIPSInst_FUNC(ir)){
				case div_op:/*div_op*/
#ifdef	GODSONEV1
					global_div_num++;
#endif
					sig = emu_div(xcp,ir);
					if(sig) {
					return sig;
					}
					break;
				case divu_op:/* divu_op:*/
#ifdef	GODSONEV1
					global_div_num++;
#endif
					sig = emu_divu(xcp,ir);
					if(sig) {
					return sig;
					}
					break;
				default:;
				
			}
	
		default:;
	}
	
 
  /*we do it*/
 xcp->cp0_epc = VA_TO_REG_l(contpc);
 xcp->cp0_cause &= ~CAUSEF_BD;
 return sig;

}

