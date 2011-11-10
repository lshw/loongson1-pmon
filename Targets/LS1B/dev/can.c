#define CAN0_BASE 0xbfe50000
#define CAN1_BASE 0xbfe54000



/********************** debug ********************/
void dumpcanregs(unsigned char* base)
{
	int i;
	printf("====dump addr: 0x%x regs\n",base);
		
	for(i = 0;i < 30;i++)
	{	
		printf("==addr:0x%x  val: 0x%x\n",i,*(base+i));
	}
	
	
}

/*********************** init *********************/

/* set can-bus timer reg 6,7 */
void bustimer_init(unsigned char* base)
{
	*(base+6) = 0x43;
	*(base+7) = 0x45;
}

void desc_init(unsigned char* base)
{
//filer
	*(base+16) = 0x0;
	*(base+17) = 0x0;
	*(base+18) = 0x0;
	*(base+19) = 0x0;
//filer mask	
	*(base+20) = 0xff;
	*(base+21) = 0xff;
	*(base+22) = 0xff;
	*(base+23) = 0xff;
}

void set_mode(unsigned char* base)
{
	
	*(base+0) = 0x0;
	*(base+4) = 0xff;
	
}

/* init can-descriptor and mode */
void can_init(unsigned char* base)
{
	bustimer_init(base);
	
//externed mode
//	*(base+1) = 0x81;
	*(base+1) = 0x80;

	desc_init(base);
	set_mode(base);
	
}


/***********************send frame*********************/
void send_frame(unsigned char* base)
{
//tx frame & id
	int i;
	
	*(base+16) = 0x83;
	*(base+17) = 0x12;
	*(base+18) = 0x34;
	*(base+19) = 0x45;
	*(base+20) = 0x56;
//tx data
	*(base+21) = 0xde;
	*(base+22) = 0xad;
	*(base+23) = 0xbe;
	
	*(base+1) = 0x1;	
}

/***********************receive frame*********************/
void receive_frame(unsigned char* base)
{
//poll rx intr
	printf("====pre to receive...\n");
	
	dumpcanregs(base);
	
	while(!(*(base+3) & 0x1))
		;
		
	printf("===rx a frame!\n");	
	printf("===desc addr: %x val: %02x\n",base+16,*(base+16));	
	printf("===desc addr: %x val: %02x\n",base+17,*(base+17));	
	printf("===desc addr: %x val: %02x\n",base+18,*(base+18));	
	printf("===desc addr: %x val: %02x\n",base+19,*(base+19));	
	printf("===desc addr: %x val: %02x\n",base+20,*(base+20));	
	printf("===desc addr: %x val: %02x\n",base+21,*(base+21));	
	printf("===desc addr: %x val: %02x\n",base+22,*(base+22));	
	printf("===desc addr: %x val: %02x\n",base+23,*(base+23));	
       	
}







void can_test(void)
{
	int* base0 = CAN0_BASE;
	int* base1 = CAN1_BASE;
		
printf("==========111111\n");
	dumpcanregs(base0);
	dumpcanregs(base1);

	can_init(base0);	
	can_init(base1);	

printf("==========222222\n");
	dumpcanregs(base0);
	dumpcanregs(base1);
	
	send_frame(base1);

printf("==========333333\n");
	dumpcanregs(base0);
	dumpcanregs(base1);

	receive_frame(base0);
}	



