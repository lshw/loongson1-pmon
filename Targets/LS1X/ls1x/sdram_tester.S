
/*
 * Auto param test by flygoat
 *
 * initstate: store round
 * Register usage at size check:
 *	msize: holding test address
 *	t1: holding test magic
 *	t2: holding read back data
 *      t3: end of posible mem
 */

#define MSIZE_STEP 0x1000

#define SDRAM_INIT_FAULT_ROUND(round) \
	li   t1, (INIT_STATE_DDR + round) ; \
	beq  initstate, t1, sdram_init_next_##round ; \
	nop

#define SDRAM_INIT_ROUND(round, para0, para1, size) \
sdram_init_start_##round:                \
	li initstate, (INIT_STATE_DDR + round)       ; \
	li  	t1, 0xbfd00410                ; \
	li		a1, para0                     ; \
	sw		a1, 0x0(t1)                   ; \
	li		a1, para1                     ; \
	sw		a1, 0x4(t1)                   ; \
	li		a1, para0                     ; \
	sw		a1, 0x0(t1)                   ; \
	li		a1, para1                     ; \
	sw		a1, 0x4(t1)                   ; \
	li		a1, para0                     ; \
	sw		a1, 0x0(t1)                   ; \
	li		a1, (para1 | (CFG_VALID << 9)) ; \
	sw		a1, 0x4(t1)                   ; \
	DELAY(300)                            ; \
	li	msize, 0xa0000000                 ; \
	li	t1, 0xdeadbeef                    ; \
	li	t3, (0xa0000000 + size)           ; \
msizetest_continue_##round:                \
	add     msize, msize, MSIZE_STEP      ; \
	sw	t1, 0x0(msize)                    ; \
	lw	t2, 0x0(msize)                    ; \
	beq	msize, t3, msizetest_out_##round  ; \
	nop                                   ; \
	beq	t1, t2, msizetest_continue_##round ; \
	nop                                   ; \
msizetest_out_##round:                      \
	sub	msize, msize, 0xa0000000          ; \
	li	t1, size                          ; \
	beq t1, msize, sdram_init_done        ; \
	nop                                   ; \
	b  sdram_init_fault_handler           ; \
	nop                                   ; \
sdram_init_next_##round:
