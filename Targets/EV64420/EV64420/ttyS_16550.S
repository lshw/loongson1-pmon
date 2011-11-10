
LEAF(initserial)
	la	v0, COM1_BASE_ADDR
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
 	li	v1, NS16550HZ/(16*CONS_BAUD)
	sb	v1, NSREG(NS16550_DATA)(v0)
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
#if 0
	li	v1, MCR_DTR|MCR_RTS
#endif
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
#if 1
	move	v1, v0
	la	v0, COM2_BASE_ADDR
	bne	v0, v1, 1b
	nop
#endif
	j	ra
	nop
END(initserial)


LEAF(tgt_putchar)
	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)
#if 1
	move	v1, v0
	la	v0, COM2_BASE_ADDR
	bne	v0, v1, 1b
	nop
#endif
	j	ra
	nop	
END(tgt_putchar)

