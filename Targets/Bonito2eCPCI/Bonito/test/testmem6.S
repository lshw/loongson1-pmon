//#include "test/testmem.S"
//#include "test/testmem5.S"

//#define TESTCACHE
#define TREAD
#define TWRITE
#ifdef TESTCACHE
        mfc0   a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x3
        mtc0   a0,COP_0_CONFIG
#endif
PRINTSTR("begin test\r\n");
	dli  t5, 0x5555555555555555
	dli  t7, 0x3333333333333333
	dli  t6, 0xaaaaaaaaaaaaaaaa
#ifdef TESTCACHE
	li   t2, 0x80000000
	li   t3, 0xa1000000
#else
	li   t2, 0xa0000000
	li   t3, 0xa1000000
#endif

#if 0
/* test continous write */
	move t0, t2
1:
	sd   t7,0(t0)
	addiu t0,t0,8
	blt   t0,t3,1b
	nop

	move t0, t2
1:
	ld   t4,0(t0)
	beq  t4, t7, 2f
	nop
	addiu t0,t0,8
	blt   t0,t3,1b
	nop

	PRINTSTR("block write test failed!\r\n");
	b  3f;
	nop
2:
	PRINTSTR("block write test ok!addr=\r\n");
	move a0,t0
	bal hexserial
	nop
3:
#endif

1:
#ifdef TWRITE
	#sd t5,-8(t2)
	sd t7,(t2)
	#sd t6,8(t2)
	PRINTSTR("\r\nwrite="); 
	move a0,t7;
	bal  hexserial
	nop
	dsrl a0,t7,32;
	bal  hexserial
	nop
#endif
	SET_LED(7)
#ifdef TESTCACHE
	cache 1,(t2)
	cache 1,1(t2)
	cache 1,2(t2)
	cache 1,3(t2)
#endif
#ifdef TREAD
	ld t8,(t2)
	PRINTSTR("\r\nread="); 
	move a0,t8;
	bal  hexserial
	nop
	dsrl a0,t8,32;
	bal  hexserial
	nop

	ld t8,8(t2)
	PRINTSTR("\r\nread="); 
	move a0,t8;
	bal  hexserial
	nop
	dsrl a0,t8,32;
	bal  hexserial
	nop
#endif

#ifdef TESTCACHE
	cache 1,(t2)
	cache 1,1(t2)
	cache 1,(t2)
	cache 1,1(t2)
	cache 1,2(t2)
	cache 1,3(t2)
#endif
	SET_LED(0)
	nop

#if 0
222:
	li  t3, 0x10000
1:
	sd t0,(t2)
	addu t3,-1
	bnez t3,1b
	nop
	li  t3, 0x10000
1:
	ld v0,(t2)
	addu t3,-1
	bnez t3,1b
	nop
	b 222b
	nop

1:
	sd  t0,0(t1)
	not t0
	addiu t1,t1,8
	addiu t2,t2,-1
	bnez  t2,1b
	nop

	PRINTSTR("Start reading memory 0xa0000000\r\n");
1:
	li  t1, 0xa0000000
	ld  t0, 0(t1);
	ld  t0, 40(t1);
	b  1b
	nop

#endif
