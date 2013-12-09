#include <machine/pio.h>
#include <stdio.h>
#include <pmon.h>
#include <cpu.h>

void mycacheflush(long long addrin, unsigned int size, unsigned int rw)
{
	unsigned int status;
	unsigned long long addr;
	addr = addrin & ~0x1fULL;
	size = (addrin-addr+size+0x1f) & ~0x1fUL;

#if __mips >= 3
	asm(" #define COP_0_STATUS_REG	$12 \n"
	"	#define SR_DIAG_DE		0x00010000\n"
	"	mfc0	%0, $12		# Save the status register.\n"
	"	li	$2, 0x00010000\n"
	"	mtc0	$2, $12		# Disable interrupts\n"
	:"=r"(status)
	::"$2");
	if (rw) {
		asm("#define HitWBInvalidate_S   0x17 \n"
		"#define HitWBInvalidate_D   0x15 \n"
		".set noreorder\n"
		"1:	\n"
		"sync \n"
		"cache   0x17, 0(%0) \n"
		"daddiu %0,32 \n"
		"addiu %1,-32 \n"
		"bnez %1,1b \n"
		"nop \n"
		".set reorder \n"
		::"r"(addr),"r"(size));
	}
	else {
		asm("#define HitInvalidate_S     0x13 \n"
		"#define HitInvalidate_D     0x11\n"
		"	.set noreorder\n"
		"	1:	\n"
		"	sync\n"
		"	cache	0x13, 0(%0)\n"
		"	daddiu %0,32\n"
		"	addiu %1,-32\n"
		"	bnez %1,1b\n"
		"	nop\n"
		"	.set reorder\n"
		::"r"(addr),"r"(size));
	}

	asm("\n"
	"	#define COP_0_STATUS_REG	$12\n"
	"	mtc0	%0, $12		# Restore the status register.\n"
	::"r"(status));

#else
	CPU_IOFlushDCache(addr, size, rw);
#endif
}

int highmemcpy(long long dst,long long src,long long count)
{
#if __mips >= 3
	asm(
	".set noreorder\n"
	"1:\n"
	"beqz %2,2f\n"
	"nop\n"
	"lb $2,(%0)\n"
	"sb $2,(%1)\n"
	"daddiu %0,1\n"
	"daddiu %1,1\n"
	"b 1b\n"
	"daddiu %2,-1\n"
	"2:\n"
	".set reorder\n"
	::"r"(src),"r"(dst),"r"(count)
	:"$2"
	);
#else
	 memcpy(dst,src,count);
#endif
	return 0;
}

int highmemset(long long addr,char c,long long count)
{
#if __mips >= 3
	asm(
	".set noreorder\n"
	"1:\n"
	"beqz %2,2f\n"
	"nop\n"
	"sb %1,(%0)\n"
	"daddiu %0,1\n"
	"b 1b\n"
	"daddiu %2,-1\n"
	"2:\n"
	".set reorder\n"
	::"r"(addr),"r"(c),"r"(count)
	:"$2"
	);
#else
	memset(addr,c,count);
#endif
	return 0;
}

