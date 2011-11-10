#define GPIO_DIR_HIGH	(0xbfe00120)
#define GPIO_DATA_HIGH	(0xbfe0011c)
#define	G_OUTPUT	0
#define	G_INPUT		1

#define GPIO_SDA_DIR_SHIFT	2
#define	GPIO_SCL_DIR_SHIFT	3
#define GPIO_SDA_DATA_SHIFT	2
#define GPIO_SCL_DATA_SHIFT	3

/*
can't use t1,t5,t6

*/

LEAF(_i2c_sleep)
//
//	li 	t0,0x300
	li 	t0,0x3
	sll	a0,t0,a0
	
1:	nop
	subu 	a0,1
	bnez	a0,1b
	nop
	
	jr ra
	nop
	
END(_i2c_sleep)

LEAF(_sda_dir)
	li	t0,GPIO_DIR_HIGH
	lwu	t2,0(t0)
	nop
	
	beqz	a0,1f
	nop
	ori	t2,t2,0x1<<GPIO_SDA_DIR_SHIFT
	b	2f
	nop
	
1:	li	t3,~(0x1<<GPIO_SDA_DIR_SHIFT)
	and 	t2,t2,t3
2:	sw	t2,0(t0)

	nop
	jr	ra
	nop
	
END(_sda_dir)

LEAF(_scl_dir)
	li	t0,GPIO_DIR_HIGH
	lwu	t2,0(t0)
	nop

	beqz	a0,1f
	nop
	ori	t2,t2,0x1<<GPIO_SCL_DIR_SHIFT
	b	2f
	nop
	
1:	li	t3,~(0x1<<GPIO_SCL_DIR_SHIFT)
	and 	t2,t2,t3
2:	sw	t2,0(t0)

	nop
	jr	ra
	nop

END(_scl_dir)

LEAF(_sda_bit)
	li	t0,GPIO_DATA_HIGH
	lwu	t2,0(t0)
	nop
	
	beqz	a0,1f
	nop
	ori	t2,t2,0x1<<GPIO_SDA_DATA_SHIFT
	b	2f
	nop
	
1:	li	t3,~(0x1<<GPIO_SDA_DATA_SHIFT)
	and 	t2,t2,t3
2:	sw	t2,0(t0)

	nop
	jr	ra
	nop
END(_sda_bit)

LEAF(_scl_bit)
	li	t0,GPIO_DATA_HIGH
	lwu	t2,0(t0)
	nop
	
	beqz	a0,1f
	nop
	ori	t2,t2,0x1<<GPIO_SCL_DATA_SHIFT
	b	2f
	nop
	
1:	li	t3,~(0x1<<GPIO_SCL_DATA_SHIFT)
	and 	t2,t2,t3
2:	sw	t2,0(t0)

	nop
	jr	ra
	nop
END(_sda_bit)


LEAF(_i2c_start)
	move	t7,ra


	li 	a0,G_OUTPUT
	bal	_sda_dir
	nop
	li	a0,G_OUTPUT
	bal	_scl_dir
	nop
	li	a0,0
	bal	_scl_bit
	nop
	li	a0,1
	bal	_i2c_sleep
	nop
	li	a0,1
	bal	_sda_bit
	nop

	li	a0,1
	bal	_i2c_sleep
	nop
	li	a0,1
	bal	_scl_bit
	nop
	li	a0,5
	bal	_i2c_sleep
	nop
	li 	a0,0
	bal 	_sda_bit
	nop
	li	a0,5
	bal	_i2c_sleep
	nop
	li	a0,0
	bal	_scl_bit
	nop
	li	a0,2
	bal	_i2c_sleep
	nop

	jr 	t7
	nop
	
END(_i2c_start)

LEAF(_i2c_stop)
	move 	t7,ra

	li	a0,G_OUTPUT
	bal	_sda_dir
	nop
	li	a0,G_OUTPUT
	bal	_scl_dir
	nop
	li	a0,0
	bal	_scl_bit
	nop
	li	a0,1
	bal	_i2c_sleep
	nop
	li	a0,0
	bal	_sda_bit
	nop
	li 	a0,1
	bal	_i2c_sleep
	nop
	li	a0,1
	bal 	_scl_bit
	nop
	li	a0,5
	bal	_i2c_sleep
	nop
	li	a0,1
	bal	_sda_bit
	nop
	li	a0,5
	bal	_i2c_sleep
	nop
	li 	a0,0
	bal	_scl_bit
	nop
	li	a0,2
	bal	_i2c_sleep
	nop
	
	jr	t7
	nop
END(_i2c_stop)
LEAF(_i2c_send_ack)

	move	t7,ra
	move 	t4,a0

	li	a0,G_OUTPUT
	bal	_sda_dir
	nop
	move	a0,t4
	bal	_sda_bit
	nop
	li	a0,3
	bal	_i2c_sleep
	nop
	li 	a0,1
	bal	_scl_bit
	nop
	li	a0,5
	bal	_i2c_sleep
	nop
	li	a0,0
	bal	_scl_bit
	nop
	li	a0,2
	bal	_i2c_sleep
	nop
	
	jr	t7
	nop
END(_i2c_send_ack)

LEAF(_i2c_rec_ack)
	move 	t7,ra
	li	v0,1
	li	t4,10

	li	a0,G_INPUT
	bal	_sda_dir
	nop
	li	a0,3
	bal	_i2c_sleep
	nop
	li	a0,1
	bal	_scl_bit
	nop
	li	a0,5
	bal	_i2c_sleep
	nop
#ifdef DEVBD2F_SM502
	li	t9,GPIO_DATA_HIGH
	lwu	t9,0(t9)
	nop
	andi	t9,t9,0x1<<GPIO_SDA_DATA_SHIFT
#elif DEVBD2F_FIREWALL
	li	t9,GPIO_DATA_HIGH
	lwu	t9,0(t9)
	nop
	srl 	t9,t9,16
	andi	t9,t9,0x1<<(GPIO_SDA_DATA_SHIFT)

#endif

2:	beqz	t9,1f
	nop
	li	a0,1
	bal	_i2c_sleep
	nop
	subu	t4,t4,1
	
	bnez	t4,3f
	nop
	li	v0,0
	b	1f
	nop

#ifdef DEVBD2F_SM502
3:	li	t9,GPIO_DATA_HIGH
	lwu	t9,0(t9)
	nop
	andi	t9,t9,0x1<<GPIO_SDA_DATA_SHIFT
#elif DEVBD2F_FIREWALL
3:	li	t9,GPIO_DATA_HIGH
	lwu	t9,0(t9)
	nop
	srl 	t9,t9,16
	andi	t9,t9,0x1<<(GPIO_SDA_DATA_SHIFT)
#endif
	b	2b
	nop
	

1:	li	a0,0
	bal	_scl_bit
	nop
	li	a0,3
	bal	_i2c_sleep
	nop
	
	jr	t7
	nop	

END(_i2c_rec_ack)


LEAF(_i2c_rec)
	move 	t7,ra
	li	t9,0x7
	li	v0,0
	li	a0,G_INPUT
	bal	_sda_dir
	nop

2:	bltz	t9,1f
	nop



	li	a0,5
	bal	_i2c_sleep
	nop
	li	a0,1
	bal	_scl_bit
	nop
	li	a0,3
	bal	_i2c_sleep
	nop
#ifdef DEVBD2F_SM502
	li	t4,GPIO_DATA_HIGH
	lwu	t4,0(t4)
	nop
	andi	t4,t4,0x1<<GPIO_SDA_DATA_SHIFT
#elif DEVBD2F_FIREWALL
	li	t4,GPIO_DATA_HIGH
	lwu	t4,0(t4)
	nop
	srl 	t4,t4,16
	andi	t4,t4,0x1<<(GPIO_SDA_DATA_SHIFT)

#endif
	beqz 	t4,3f
	nop
	li	t4,1

3:	sll	t4,t4,t9
	or	v0,v0,t4
	li	a0,3
	bal	_i2c_sleep
	nop
	li	a0,0
	bal	_scl_bit
	nop

	sub	t9,t9,1
	b	2b
	nop
	
1:	jr t7
	nop
	
END(_i2c_rec)

LEAF(_i2c_send)
	move	t7,ra
	move	t4,a0
	li	t9,0x7
	
	li	a0,G_OUTPUT
	bal	_sda_dir
	nop
	
2:	bltz	t9,1f
	nop

	move	a0,t4
	srl	a0,a0,t9
	andi	a0,a0,1
	bal	_sda_bit
	nop
	
	li	a0,1
	bal	_i2c_sleep
	nop
	li	a0,1
	bal	_scl_bit
	nop
	li	a0,5
	bal	_i2c_sleep
	nop
	li	a0,0
	bal	_scl_bit
	nop
	li	a0,1
	bal	_i2c_sleep
	nop
	
	sub	t9,t9,1
	b	2b
	nop
1:	li	a0,1
	bal	_sda_bit
	nop

	jr	t7
	nop

END(_i2c_send)

/*
a0,a2:slave device addr
a1,a3:sub addr
v0:recieve data
v1:show if sucess,0:sucess,1:failure
*/
LEAF(i2cread)
	move 	t8,ra
	nop
	move 	a2,a0
	move	a3,a1
	li	v0,0
	li	v1,0
	

	bal	_i2c_start
	nop

	move	a0,a2
	bal	_i2c_send
	nop

	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop
#if 0
	
	li	a0,0
	bal	_i2c_send
	nop


	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop
#endif

	move	a0,a3
	bal	_i2c_send
	nop


	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop


	bal	_i2c_start
	nop

	move	a0,a2
	addu	a0,a0,1
	bal	_i2c_send
	nop


	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop

	bal	_i2c_rec
	nop
/*	
	move	k0,v0
	
	li	a0,1
	bal	_i2c_send_ack
	nop
*/
	bal	_i2c_stop
	nop

	li	v1,0
//	move	v0,k0
	b	2f
	nop

1:	li	v1,1
2:	jr	t8 
	nop


END(i2cread)


/*
 *a0: slave_addr
 *a1: sub_addr
 *a2: value
 */
LEAF(i2cwrite)
	move 	t8,ra
	nop
	move	a3,a0

	bal	_i2c_start
	nop

	move	a0,a3
	bal	_i2c_send
	nop

	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop
//for write isl12027 watchdog
#if 1
	
	li	a0,0
	bal	_i2c_send
	nop


	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop
#endif


	move	a0,a1
	bal	_i2c_send
	nop


	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop
	
	move	a0,a2
	bal	_i2c_send
	nop

	bal	_i2c_rec_ack
	nop
	beqz	v0,1f
	nop

	bal	_i2c_stop
	nop

	b	2f
	nop

1:	li	v1,1
2:	jr	t8
	nop

END(i2cwrite)

LEAF(isl12027_watchdog)
	move	t1,ra	

	li	a0,0xde
	li	a1,0x3f
	li	a2,0x2
	bal	i2cwrite
	nop

//PRINTSTR("setp1\r\n")
	li	a0,0xde
	li	a1,0x3f
	li	a2,0x6
	bal	i2cwrite
	nop

//PRINTSTR("setp2\r\n")
	li	a0,0xde
	li	a1,0x14
	li	a2,0x84
	bal	i2cwrite
	nop

//PRINTSTR("setp3\r\n")


	li	a0,0xde
	li	a1,0x3f
	li	a2,0x2
	bal	i2cwrite
	nop

//PRINTSTR("setp1\r\n")
	li	a0,0xde
	li	a1,0x3f
	li	a2,0x6
	bal	i2cwrite
	nop

//PRINTSTR("setp2\r\n")
	li	a0,0xde
	li	a1,0x10
	li	a2,0x18
	bal	i2cwrite
	nop

//PRINTSTR("setp3\r\n")
	jr	t1
	nop
	
END(isl12027_watchdog)
