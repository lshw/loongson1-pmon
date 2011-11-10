#ifndef	MPC7400_REGS_H
#define	MPC7400_REGS_H

.set	rtoc,2;		

.set	r0,0;	.set	r1,1;	.set	r2,2;	.set	r3,3;		
.set	r4,4;	.set	r5,5;	.set	r6,6;	.set	r7,7;		
.set	r8,8;	.set	r9,9;	.set	r10,10;	.set	r11,11;
.set	r12,12;	.set	r13,13;	.set	r14,14;	.set	r15,15;		
.set	r16,16;	.set	r17,17;	.set	r18,18;	.set	r19,19;		
.set	r20,20;	.set	r21,21;	.set	r22,22;	.set	r23,23;
.set	r24,24;	.set	r25,25;	.set	r26,26;	.set	r27,27;		
.set	r28,28;	.set	r29,29;	.set	r30,30;	.set	r31,31;

.set	f0,0;	.set	f1,1;	.set	f2,2;	.set	f3,3;		
.set	f4,4;	.set	f5,5;	.set	f6,6;	.set	f7,7;		
.set	f8,8;	.set	f9,9;	.set	f10,10;	.set	f11,11;
.set	f12,12;	.set	f13,13;	.set	f14,14;	.set	f15,15;		
.set	f16,16;	.set	f17,17;	.set	f18,18;	.set	f19,19;		
.set	f20,20;	.set	f21,21;	.set	f22,22;	.set	f23,23;
.set	f24,24;	.set	f25,25;	.set	f26,26;	.set	f27,27;		
.set	f28,28;	.set	f29,29;	.set	f30,30;	.set	f31,31;

.set	v0,0;	.set	v1,1;	.set	v2,2;	.set	v3,3;		
.set	v4,4;	.set	v5,5;	.set	v6,6;	.set	v7,7;		
.set	v8,8;	.set	v9,9;	.set	v10,10;	.set	v11,11;
.set	v12,12;	.set	v13,13;	.set	v14,14;	.set	v15,15;		
.set	v16,16;	.set	v17,17;	.set	v18,18;	.set	v19,19;		
.set	v20,20;	.set	v21,21;	.set	v22,22;	.set	v23,23;
.set	v24,24;	.set	v25,25;	.set	v26,26;	.set	v27,27;		
.set	v28,28;	.set	v29,29;	.set	v30,30;	.set	v31,31;

.set	xer,1;		.set	lr,8;		.set	ctr,9;		.set	dsisr,18;	
.set	dar,19;		.set	dec_r,22; 	.set	sdr1,25;	.set	srr0,26;
.set	srr1,27;	.set	ear,282;	.set	pvr,287;	

.set	sprg0,272;	.set	sprg1,273;	
.set	sprg2,274;	.set	sprg3,275;	

.set	cr2,2;

.set	sr0,0;	.set	sr1,1;	.set	sr2,2;	.set	sr3,3;		
.set	sr4,4;	.set	sr5,5;	.set	sr6,6;	.set	sr7,7;		
.set	sr8,8;	.set	sr9,9;	.set	sr10,10;.set	sr11,11;
.set	sr12,12;.set	sr13,13;.set	sr14,14;.set	sr15,15;


.set	dmiss,976;	.set	dcmp,977;
.set	hash1,978;	.set	hash2,979;
.set	imiss,980;	.set	icmp,981;
.set	rpa,982;

.set	mmcr1,956;
.set	pmc3,957;	.set	pmc4,958;
.set	thrm1,1020;	.set	thrm2,1021;
.set	thrm3,1022;	.set	ictc,1019;
.set	l2cr,1017;	.set	upmc1,937;
.set	upmc2,938;	.set	upmc3,941;
.set	upmc4,942;	.set	usia,939;
.set	ummcr0,936;	.set	ummcr1,940;


.set	vrsave,256;	.set 	ubamr, 935;
.set	ummcr2,928;	.set 	bamr, 951;
.set	mmcr2,944;	.set	msscr1, 1015;
.set	msscr0,1014;	
.set	dL1HWf,0x0080   
.set	C_dL1HWf,0xff7f   

.set	tbl,284;	.set	tbu,285;		// For Writing
.set	tbl_r,268;	.set	tbu_r,269;		// For Reading

.set	ibat0u,528;	.set	ibat0l,529;	
.set	ibat1u,530;	.set	ibat1l,531;
.set	ibat2u,532;	.set	ibat2l,533;	
.set	ibat3u,534;	.set	ibat3l,535;	
.set	dbat0u,536;	.set	dbat0l,537;	
.set	dbat1u,538;	.set	dbat1l,539;
.set	dbat2u,540;	.set	dbat2l,541;	
.set	dbat3u,542;	.set	dbat3l,543;	

.set	mmcr0,952;
.set	pmc1,953;	.set	pmc2,954;
.set	sia,955;	.set	sda,959;

.set	hid0,1008;	.set	hid1,1009;	
.set	dabr,1013;	.set	iabr,1010;
.set	pir,1023;

#endif	// MPC7400_REGS_H
