/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Code to handle x86 style IRQs plus some generic interrupt stuff.
 *
 * Copyright (C) 1992 Linus Torvalds
 * Copyright (C) 1994 - 2000 Ralf Baechle
 */


void enable_8259A_irq(unsigned int irq);
void disable_8259A_irq(unsigned int irq);

/*
 * This is the 'legacy' 8259A Programmable Interrupt Controller,
 * present in the majority of PC/AT boxes.
 * plus some generic x86 specific things if generic specifics makes
 * any sense at all.
 * this file should become arch/i386/kernel/irq.c when the old irq.c
 * moves to arch independent land
 */

static spinlock_t i8259A_lock = SPIN_LOCK_UNLOCKED;

static void end_8259A_irq (unsigned int irq)
{
		enable_8259A_irq(irq);
}

#define shutdown_8259A_irq	disable_8259A_irq

void mask_and_ack_8259A(unsigned int);

static unsigned int startup_8259A_irq(unsigned int irq)
{
	enable_8259A_irq(irq);
//	prom_printf("8259 irq %x enabled\n", irq);

	return 0; /* never anything pending */
}

static struct hw_interrupt_type i8259A_irq_type = {
	"XT-PIC",
	startup_8259A_irq,
	shutdown_8259A_irq,
	enable_8259A_irq,
	disable_8259A_irq,
	mask_and_ack_8259A,
	end_8259A_irq,
	NULL
};

/*
 * 8259A PIC functions to handle ISA devices:
 */

/*
 * This contains the irq mask for both 8259A irq controllers,
 */
static unsigned int cached_irq_mask = 0xffff;

#define cached_21	(cached_irq_mask)
#define cached_A1	(cached_irq_mask >> 8)

void disable_8259A_irq(unsigned int irq)
{
	unsigned int mask = 1 << irq;
	unsigned long flags;

	spin_lock_irqsave(&i8259A_lock, flags);
	cached_irq_mask |= mask;
	if (irq & 8)
		linux_outb(cached_A1,0xA1);
	else
		linux_outb(cached_21,0x21);
	spin_unlock_irqrestore(&i8259A_lock, flags);
}

void enable_8259A_irq(unsigned int irq)
{
	unsigned int mask = ~(1 << irq);
	unsigned long flags;

	spin_lock_irqsave(&i8259A_lock, flags);
	cached_irq_mask &= mask;
	if (irq & 8)
		linux_outb(cached_A1,0xA1);
	else
		linux_outb(cached_21,0x21);
	spin_unlock_irqrestore(&i8259A_lock, flags);
}

int i8259A_irq_pending(unsigned int irq)
{
	unsigned int mask = 1 << irq;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&i8259A_lock, flags);
	if (irq < 8)
		ret = linux_inb(0x20) & mask;
	else
		ret = linux_inb(0xA0) & (mask >> 8);
	spin_unlock_irqrestore(&i8259A_lock, flags);

	return ret;
}


/*
 * This function assumes to be called rarely. Switching between
 * 8259A registers is slow.
 * This has to be protected by the irq controller spinlock
 * before being called.
 */
static inline int i8259A_irq_real(unsigned int irq)
{
	int value;
	int irqmask = 1 << irq;

	if (irq < 8) {
		linux_outb(0x0B,0x20);		/* ISR register */
		value = linux_inb(0x20) & irqmask;
		linux_outb(0x0A,0x20);		/* back to the IRR register */
		return value;
	}
	linux_outb(0x0B,0xA0);		/* ISR register */
	value = linux_inb(0xA0) & (irqmask >> 8);
	linux_outb(0x0A,0xA0);		/* back to the IRR register */
	return value;
}

/*
 * Careful! The 8259A is a fragile beast, it pretty
 * much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI
 * to the two 8259s is important!
 */
void mask_and_ack_8259A(unsigned int irq)
{
	unsigned int irqmask = 1 << irq;
	unsigned long flags;

	spin_lock_irqsave(&i8259A_lock, flags);
	/*
	 * Lightweight spurious IRQ detection. We do not want to overdo
	 * spurious IRQ handling - it's usually a sign of hardware problems, so
	 * we only do the checks we can do without slowing down good hardware
	 * nnecesserily.
	 *
	 * Note that IRQ7 and IRQ15 (the two spurious IRQs usually resulting
	 * rom the 8259A-1|2 PICs) occur even if the IRQ is masked in the 8259A.
	 * Thus we can check spurious 8259A IRQs without doing the quite slow
	 * i8259A_irq_real() call for every IRQ.  This does not cover 100% of
	 * spurious interrupts, but should be enough to warn the user that
	 * there is something bad going on ...
	 */
	if (cached_irq_mask & irqmask)
		goto spurious_8259A_irq;
	cached_irq_mask |= irqmask;

handle_real_irq:
	if (irq & 8) {
		linux_inb(0xA1);		/* DUMMY - (do we need this?) */
		linux_outb(cached_A1,0xA1);
		linux_outb(0x60+(irq&7),0xA0);/* 'Specific EOI' to slave */
		linux_outb(0x62,0x20);	/* 'Specific EOI' to master-IRQ2 */
	} else {
		linux_inb(0x21);		/* DUMMY - (do we need this?) */
		linux_outb(cached_21,0x21);
		linux_outb(0x60+irq,0x20);	/* 'Specific EOI' to master */
	}
	spin_unlock_irqrestore(&i8259A_lock, flags);
	return;

spurious_8259A_irq:
	/*
	 * this is the slow path - should happen rarely.
	 */
	if (i8259A_irq_real(irq))
		/*
		 * oops, the IRQ _is_ in service according to the
		 * 8259A - not spurious, go handle it.
		 */
		goto handle_real_irq;

	{
		static int spurious_irq_mask = 0;
		/*
		 * At this point we can be sure the IRQ is spurious,
		 * lets ACK and report it. [once per IRQ]
		 */
		if (!(spurious_irq_mask & irqmask)) {
			printk("spurious 8259A interrupt: IRQ%d.\n", irq);
			spurious_irq_mask |= irqmask;
		}
		/*
		 * Theoretically we do not have to handle this IRQ,
		 * but in Linux this does not cause problems and is
		 * simpler for us.
		 */
		goto handle_real_irq;
	}
}

void  init_8259A(int auto_eoi)
{
	unsigned long flags;

	spin_lock_irqsave(&i8259A_lock, flags);

	linux_outb(0xff, 0x21);	/* mask all of 8259A-1 */
	linux_outb(0xff, 0xA1);	/* mask all of 8259A-2 */

	/*
	 * linux_outb_p - this has to work on a wide range of PC hardware.
	 */
	linux_outb_p(0x11, 0x20);	/* ICW1: select 8259A-1 init */
	linux_outb_p(0x00, 0x21);	/* ICW2: 8259A-1 IR0-7 mapped to 0x00-0x07 */
	linux_outb_p(0x04, 0x21);	/* 8259A-1 (the master) has a slave on IR2 */
	if (auto_eoi)
		linux_outb_p(0x03, 0x21);	/* master does Auto EOI */
	else
		linux_outb_p(0x01, 0x21);	/* master expects normal EOI */

	linux_outb_p(0x11, 0xA0);	/* ICW1: select 8259A-2 init */
	linux_outb_p(0x08, 0xA1);	/* ICW2: 8259A-2 IR0-7 mapped to 0x08-0x0f */
	linux_outb_p(0x02, 0xA1);	/* 8259A-2 is a slave on master's IR2 */
	linux_outb_p(0x01, 0xA1);	/* (slave's support for AEOI in flat mode
				    is to be investigated) */

	if (auto_eoi)
		/*
		 * in AEOI mode we just have to mask the interrupt
		 * when acking.
		 */
		i8259A_irq_type.ack = disable_8259A_irq;
	else
		i8259A_irq_type.ack = mask_and_ack_8259A;

	udelay(100);		/* wait for 8259A to initialize */

	linux_outb(cached_21, 0x21);	/* restore master IRQ mask */
	linux_outb(cached_A1, 0xA1);	/* restore slave IRQ mask */

	spin_unlock_irqrestore(&i8259A_lock, flags);
}



/*
 * On systems with i8259-style interrupt controllers we assume for
 * driver compatibility reasons interrupts 0 - 15 to be the i8295
 * interrupts even if the hardware uses a different interrupt numbering.
 */
void  init_i8259_irqs (void)
{
	int i;


	init_8259A(0);

	i8259A_irq_type.startup(2);
}

