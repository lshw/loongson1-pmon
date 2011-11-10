// This is the extra stuff added to the memtest+ from memtest.org
// Code from Eric Nelson and Wee 
// (Checked without vendor-specific optimization before adding)
/* extra.c - 
 *
 * Released under version 2 of the Gnu Public License.
 * 
 */

#include "test.h"
#include "screen_buffer.h"
//#include "pci.h"
#include "extra.h"
#define PCI_VENDOR_ID       0x00    /* 16 bits */
#define PCI_DEVICE_ID       0x02    /* 16 bits */

int Pci_conf_read(int bus,int dev,int func,int reg,int width,ulong *paddr)
{
#ifndef NOPCI
	pcitag_t tag;
	tag=_pci_make_tag(bus,dev,func);
	*paddr=_pci_conf_readn(tag, reg, width); 
#endif
	return 0;
}

void  Pci_conf_write(int bus,int dev,int func,int reg,int width,int data)
{
#ifndef NOPCI
pcitag_t tag;
tag=_pci_make_tag(bus,dev,func);
_pci_conf_writen(tag,reg,data,width);
#endif
}

void support();

static int ctrl = -1;

struct memory_controller {
	unsigned vendor;
	unsigned device;
	int worked;
	void (*change_timing)(int cas, int rcd, int rp, int ras);
};

static struct memory_controller mem_ctr[] = {
	
	/* AMD 64*/
	{ 0x1022, 0x1100,  1, change_timing_amd64}, //AMD64 hypertransport link
	
	/* nVidia */
	{ 0x10de, 0x01E0,  0, change_timing_nf2},  // nforce2
	
	/* Intel */
	{ 0x8086, 0x2570,  0, change_timing_i875}, //Intel i848/i865
	{ 0x8086, 0x2578,  0, change_timing_i875}, //Intel i875P
	{ 0x8086, 0x2580,  0, change_timing_i925}, //Intel i915P/G
	{ 0x8086, 0x2584,  0, change_timing_i925}, //Intel i925X
	{ 0x8086, 0x3580,  0, change_timing_i852}, //Intel i852GM - i855GM/GME (But not i855PM)
};

struct drc {
	unsigned t_rwt;
	unsigned t_wrt;
	unsigned t_ref;
	unsigned t_en2t;
	unsigned t_rwqb;
	unsigned t_rct;
	unsigned t_rrd;
	unsigned t_wr;
};

static struct drc a64;

void find_memctr(void)  // Basically copy from the find_controller function
{
	unsigned long vendor;
	unsigned long device;
	unsigned long a64;
	int i= 0;
	int result;
	
	result = Pci_conf_read(0, 0, 0, PCI_VENDOR_ID, 2, &vendor);
	result = Pci_conf_read(0, 0, 0, PCI_DEVICE_ID, 2, &device);
	
	Pci_conf_read(0, 24, 0, 0x00, 4, &a64);	
	
	if( a64 == 0x11001022) 	{
	ctrl = 0;
	return;
	}
	
	if (result == 0) {
		for(i = 1; i < sizeof(mem_ctr)/sizeof(mem_ctr[0]); i++) {
			if ((mem_ctr[i].vendor == vendor) &&
				(mem_ctr[i].device == device)) 
				{
				ctrl = i;
				return;
				}
		}
	}
  	ctrl = -1;
}

void a64_parameter(void)
{

	ulong dramtlr, temp, l, result; 
	
	result = Pci_conf_read(0, 24, 2, 0x88, 4, &dramtlr);
	if (result == 0)
	{
	temp = dramtlr;
	
	l = ((temp>>4) & 0x0F);
	switch(l){
		case 0x0 : a64.t_rct = 7; break;
		case 0x1 : a64.t_rct = 8; break;
		case 0x2 : a64.t_rct = 9; break;
		case 0x3 : a64.t_rct = 10; break;
		case 0x4 : a64.t_rct = 11; break;
		case 0x5 : a64.t_rct = 12; break;
		case 0x6 : a64.t_rct = 13; break;
		case 0x7 : a64.t_rct = 14; break;
		case 0x8 : a64.t_rct = 15; break;
		case 0x9 : a64.t_rct = 16; break;
		case 0xA : a64.t_rct = 17; break;
		case 0xB : a64.t_rct = 18; break;
		case 0xC : a64.t_rct = 19; break;
		case 0xD : a64.t_rct = 20; break;
		}
	
	l = ((temp>>16) & 0x7);
	a64.t_rrd = l;
	
	l = ((temp>>28) & 0x1);
	switch(l){
		case 0x0 : a64.t_wr = 2; break;
		case 0x1 : a64.t_wr = 3; break;
		}
	}
		
	result = Pci_conf_read(0, 24, 2, 0x8C, 4, &dramtlr);
	if (result == 0)
	{
	temp = dramtlr;
	
	l = ((temp>>4) & 0x07);
	switch(l){
		case 0x0 : a64.t_rwt = 1; break;
		case 0x1 : a64.t_rwt = 2; break;
		case 0x2 : a64.t_rwt = 3; break;
		case 0x3 : a64.t_rwt = 4; break;
		case 0x4 : a64.t_rwt = 5; break;
		case 0x5 : a64.t_rwt = 6; break;
		}
		
	l = (temp & 0x1);
	a64.t_wrt = l + 1;
		
	l = ((temp>>11) & 0x3);
	switch(l){
		case 0x0 : a64.t_ref = 1; break;
		case 0x1 : a64.t_ref = 2; break;
		case 0x2 : a64.t_ref = 3; break;
		}
	}
	
	result = Pci_conf_read(0, 24, 2, 0x90, 4, &dramtlr);
	if (result == 0 )
	{
	temp = dramtlr;
	
	l = ((temp>>28) & 0x1);
	switch(l){
		case 0x0 : a64.t_en2t = 1; break;
		case 0x1 : a64.t_en2t = 2; break;
		}
						
	l = ((temp>>14) & 0x3);
	switch(l){
		case 0x0 : a64.t_rwqb = 2; break;
		case 0x1 : a64.t_rwqb = 4; break;
		case 0x2 : a64.t_rwqb = 8; break;
		case 0x3 : a64.t_rwqb = 16; break;
		}
	}
}



void change_timing(int cas, int rcd, int rp, int ras)
{ 
	find_memctr();	
	if ((ctrl == -1) || ( ctrl > sizeof(mem_ctr)/sizeof(mem_ctr[0])))
	{
	return;
	}
	else
	{
	mem_ctr[ctrl].change_timing(cas, rcd, rp, ras);
	restart();
	}
}

void amd64_option()
{
	int rwt=0, wrt=0, ref=0, en2t=0, rct=0, rrd=0, rwqb=0, wr = 0, flag=0;
	
	if ((ctrl == -1) || ( ctrl > sizeof(mem_ctr)/sizeof(mem_ctr[0])))
	{
	return;
	}
		
	if (mem_ctr[ctrl].worked)
	{
	a64_parameter();
	cprint(POP_Y+1, POP_X+4, "AMD64 options");
	
	cprint(POP_Y+3, POP_X+4, "(1) Rd-Wr Delay   : ");
	dprint(POP_Y+3, POP_X+24, a64.t_rwt, 2, 0);
	
	cprint(POP_Y+4, POP_X+4, "(2) Wr-Rd Delay   : ");
	dprint(POP_Y+4, POP_X+24, a64.t_wrt, 2, 0);
	
	cprint(POP_Y+5, POP_X+4, "(3) Rd/Wr Bypass  : ");
	dprint(POP_Y+5, POP_X+24, a64.t_rwqb, 2, 0);
		
	cprint(POP_Y+6, POP_X+4, "(4) Refresh Rate  : ");
	switch ( a64.t_ref)
	{
	case 1 : cprint(POP_Y+6, POP_X+23, "15.6us"); break;
	case 2 : cprint(POP_Y+6, POP_X+23, " 7.8us"); break;
	case 3 : cprint(POP_Y+6, POP_X+23, " 3.9us"); break;
	}
	cprint(POP_Y+7, POP_X+4,  "(5) Command Rate  :");
	dprint(POP_Y+7, POP_X+24, a64.t_en2t, 2, 0);
	cprint(POP_Y+7, POP_X+26, "T ");
	
	cprint(POP_Y+8, POP_X+4,  "(6) Row Cycle Time: ");
	dprint(POP_Y+8, POP_X+24, a64.t_rct, 2, 0);
	
	cprint(POP_Y+9, POP_X+4, "(7) RAS-RAS Delay : ");
	dprint(POP_Y+9, POP_X+24, a64.t_rrd, 2, 0);
	
	cprint(POP_Y+10, POP_X+4, "(8) Write Recovery: ");
	dprint(POP_Y+10, POP_X+24, a64.t_wr, 2, 0);
			
	cprint(POP_Y+11, POP_X+4,"(0) Cancel   ");
	
	while(!flag)
		{	
		switch(get_key()) 
			{  				   
			case 2:
			popclear();
			// read-to-write delay
			cprint(POP_Y+3, POP_X+4, "Rd-Wr delay ");
			cprint(POP_Y+4, POP_X+4, " (2 - 6 cycles)");
			cprint(POP_Y+5, POP_X+4, "Current: ");
			dprint(POP_Y+5, POP_X+14, a64.t_rwt, 4, 0);
			cprint(POP_Y+7, POP_X+4, "New: ");
			rwt = getval(POP_Y+7, POP_X+12, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);      
			break;
			
			case 3:
			popclear();
			// read-to-write delay
			cprint(POP_Y+3, POP_X+4, "Wr-Rd delay ");
			cprint(POP_Y+4, POP_X+4, " (1 - 2 cycles)");
			cprint(POP_Y+5, POP_X+4, "Current: ");
			dprint(POP_Y+5, POP_X+14, a64.t_wrt, 4, 0);
			cprint(POP_Y+7, POP_X+4, "New: ");
			wrt = getval(POP_Y+7, POP_X+12, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);       
			break;
				
			case 4:
			popclear();
			// Read write queue bypass count
			cprint(POP_Y+3, POP_X+4, "Rd/Wr bypass ");
			cprint(POP_Y+4, POP_X+4, " (2, 4 or 8 )");
			cprint(POP_Y+5, POP_X+4, "Current: ");
			dprint(POP_Y+5, POP_X+14, a64.t_rwqb, 2, 0);
			cprint(POP_Y+7, POP_X+4, "New: ");
			rwqb = getval(POP_Y+7, POP_X+11, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);      
			break;
			
			case 5:
			popclear();
			// refresh rate
			cprint(POP_Y+3, POP_X+4, "Refresh rate ");
			cprint(POP_Y+4, POP_X+4, "Current: ");
			switch ( a64.t_ref){
			case 1 : cprint(POP_Y+4, POP_X+14, "15.6us"); break;
			case 2 : cprint(POP_Y+4, POP_X+14, "7.8us "); break;
			case 3 : cprint(POP_Y+4, POP_X+14, "3.9us "); break;
			}
			cprint(POP_Y+6, POP_X+4, "New: ");
			cprint(POP_Y+7, POP_X+4, "(1) 15.6us");
			cprint(POP_Y+8, POP_X+4, "(2) 7.8us ");
			cprint(POP_Y+9, POP_X+4, "(3) 3.9us ");
			ref = getval(POP_Y+6, POP_X+11, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);      
			break;
												
			case 6:
			popclear();
			//Enable 2T command and addressing      
			cprint(POP_Y+3, POP_X+4, "Command rate:");
			cprint(POP_Y+5, POP_X+4, "(1) 1T "); //only supoprted by CG revision and later
			cprint(POP_Y+6, POP_X+4, "(2) 2T ");
			en2t = getval(POP_Y+3, POP_X+22, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);      
			break;
			
			case 7:
			popclear();
			//Row cycle time    
			cprint(POP_Y+3, POP_X+4, "Row cycle time: ");
			cprint(POP_Y+4, POP_X+4, " (7 - 20 cycles)");
			cprint(POP_Y+5, POP_X+4, "Current: ");
			dprint(POP_Y+5, POP_X+14, a64.t_rct, 4, 0);
			cprint(POP_Y+7, POP_X+4, "New: ");
			rct = getval(POP_Y+7, POP_X+12, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);      
			break; 
				
			case 8:
			popclear();
			//Active-to-Active RAS Delay   
			cprint(POP_Y+3, POP_X+4, "RAS-RAS Delay: ");
			cprint(POP_Y+4, POP_X+4, " (2 - 4 cycles)");
			cprint(POP_Y+5, POP_X+4, "Current: ");
			dprint(POP_Y+5, POP_X+14, a64.t_rrd, 2, 0);
			cprint(POP_Y+7, POP_X+4, "New: ");
			rrd = getval(POP_Y+7, POP_X+12, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);      
			break;
			
			case 9:
			popclear();
			//Active-to-Active RAS Delay   
			cprint(POP_Y+3, POP_X+4, "Write Recovery: ");
			cprint(POP_Y+4, POP_X+4, " (2 - 3 cycles)");
			cprint(POP_Y+5, POP_X+4, "Current: ");
			dprint(POP_Y+5, POP_X+14, a64.t_wr, 2, 0);
			cprint(POP_Y+7, POP_X+4, "New: ");
			wr = getval(POP_Y+7, POP_X+12, 0);
			amd64_tweak(rwt, wrt, ref,en2t, rct, rrd, rwqb, wr);      
			break;
			
			case 11:
			case 57:
			flag++;
		/* 0/CR - Cancel */
			break;
			}
		}		    
	}
}

void get_option()
{
	int cas =0, rp=0, rcd=0, ras=0, sflag = 0 ;
		
	while(!sflag)
	{	
		switch(get_key()) 
                {  				   
			case 2:
			    popclear();
			    cas = get_cas();
			    popclear();
                         	    
			    cprint(POP_Y+3, POP_X+8, "tRCD: ");
			    rcd = getval(POP_Y+3, POP_X+15, 0);
			    popclear();
			      
			    cprint(POP_Y+3, POP_X+8, "tRP: ");
			    rp = getval(POP_Y+3, POP_X+15, 0);
			    popclear();
				       
			    cprint(POP_Y+3, POP_X+8, "tRAS: ");
			    ras = getval(POP_Y+3, POP_X+15, 0);
			    popclear();
			    change_timing(cas, rcd, rp, ras);	
			    break;
			   
			 case 3:
			    popclear();
			    cas = get_cas();
			    change_timing(cas, 0, 0, 0);
			    sflag++;				
			   break;
			    
			    
			  case 4:
			    popclear();
			    cprint(POP_Y+3, POP_X+8, "tRCD: ");
			    rcd =getval(POP_Y+3, POP_X+15, 0);
			    change_timing(0, rcd, 0, 0);	
			    sflag++;		    	 
			    break;
					   
			  case 5:
			    popclear();
			    cprint(POP_Y+3, POP_X+8, "tRP: ");
			    rp =getval(POP_Y+3, POP_X+15, 0);
			    change_timing(0, 0, rp, 0);
			    sflag++;
			   break; 
			   
			   case 6:
			    popclear();
			    cprint(POP_Y+3, POP_X+8, "tRAS: ");
			    ras =getval(POP_Y+3, POP_X+15, 0);
			    change_timing(0, 0, 0, ras);
			    sflag++;			   
			   break;
				   
			   case 7:     
			    popclear();
			    amd64_option();
			    sflag++;
			    popclear();
			    break;
			   
			   case 8:
			    break;
			    
			   case 11:
			   case 57:
			    sflag++;
			/* 0/CR - Cancel */
			    break;
		}  
	}
}

void get_option_1()
{
	int rp=0, rcd=0, ras=0, sflag = 0 ;
	
	while(!sflag)
	{	
		switch(get_key()) 
                {  				   
			case 2:
			    popclear();
			    cprint(POP_Y+3, POP_X+8, "tRCD: ");
			    rcd = getval(POP_Y+3, POP_X+15, 0);
			    popclear();
			      
			    cprint(POP_Y+3, POP_X+8, "tRP: ");
			    rp = getval(POP_Y+3, POP_X+15, 0);
			    popclear();
				       
			    cprint(POP_Y+3, POP_X+8, "tRAS: ");
			    ras = getval(POP_Y+3, POP_X+15, 0);
			    popclear();
			    change_timing(0, rcd, rp, ras);
			    break;
			   
			 case 3:
			    popclear();
			    cprint(POP_Y+3, POP_X+8, "tRCD: ");
			    rcd =getval(POP_Y+3, POP_X+15, 0);
			    change_timing(0, rcd, 0, 0);				    	 
			    break;
					   
			  case 4:
			    popclear();
			    cprint(POP_Y+3, POP_X+8, "tRP: ");
			    rp =getval(POP_Y+3, POP_X+15, 0);
			    change_timing(0, 0, rp, 0);
			   break; 
			   
			   case 5:
			    popclear();
			    cprint(POP_Y+3, POP_X+8, "tRAS: ");
			    ras =getval(POP_Y+3, POP_X+15, 0);
			    change_timing(0, 0, 0, ras);				   
			   break;
				   
			   case 6:     
			    popclear();
			    amd64_option();
			    sflag++;
			    popclear();
			    break;
			   
			   case 7:
			    break;
			    
			   case 11:
			   case 57:
			    sflag++;
			/* 0/CR - Cancel */
			    break;
		}  
	}
}


void get_menu(void)
{ 
	int menu ;
	
	find_memctr();
	support();
	
	switch(ctrl)
	{
	case 0: menu = 2;	break;
	case 1:
	case 2:
	case 3:	
	case 4:	menu = 0;	break;
	case 5: menu = 1;	break;
	case 6: menu = 0;	break;
	default: return;	break;
	}
	popclear();
	cprint(POP_Y+10, POP_X+3, "Modifying timing may ");
	cprint(POP_Y+11, POP_X+3, "cause system instability");
	cprint(POP_Y+12, POP_X+3, "proceed at your own risk");
	
	if (menu == 0)
	{
		cprint(POP_Y+1, POP_X+2, "Modify Timing:");
		cprint(POP_Y+3, POP_X+5, "(1) Modify All   ");
		cprint(POP_Y+4, POP_X+5, "(2) Modify tCAS  ");
		cprint(POP_Y+5, POP_X+5, "(3) Modify tRCD  ");
		cprint(POP_Y+6, POP_X+5, "(4) Modify tRP  ");
		cprint(POP_Y+7, POP_X+5, "(5) Modify tRAS   ");
		cprint(POP_Y+8, POP_X+5, "(0) Cancel");
		wait_keyup();
	 	get_option();	
	}
	else if (menu == 1)
	{
		cprint(POP_Y+1, POP_X+2, "Modify Timing:");
		cprint(POP_Y+3, POP_X+5, "(1) Modify All   ");
		cprint(POP_Y+4, POP_X+5, "(2) Modify tRCD  ");
		cprint(POP_Y+5, POP_X+5, "(3) Modify tRP   ");
		cprint(POP_Y+6, POP_X+5, "(4) Modify tRAS  ");
		cprint(POP_Y+7, POP_X+5, "(0) Cancel");
		wait_keyup();
	 	get_option();	
	}
	else  // AMD64 special menu
	{ 
		cprint(POP_Y+1, POP_X+2, "Modify Timing:");
		cprint(POP_Y+3, POP_X+5, "(1) Modify All   ");
		cprint(POP_Y+4, POP_X+5, "(2) Modify tRCD  ");
		cprint(POP_Y+5, POP_X+5, "(3) Modify tRP   ");
		cprint(POP_Y+6, POP_X+5, "(4) Modify tRAS  ");
		cprint(POP_Y+7, POP_X+5, "(5) AMD64 Options");
		cprint(POP_Y+8, POP_X+5, "(0) Cancel");
		wait_keyup();
	 	get_option_1();	
	}
}				

int get_cas(void)
{ 
	int i852=0, cas=0;
	ulong drc, ddr;
	long *ptr;
	
	switch(ctrl)
	{
	case 0: ddr = 1; break;
	case 1:
	case 2:
	case 3:	ddr = 1; break;
	case 4:	
	  Pci_conf_read( 0, 0, 0, 0x44, 4, &ddr);
		ptr=(long*)(ddr+0x120);
		drc = *ptr & 0xFFFFFFFF;
		
		if ((drc & 3) == 2) ddr = 2;
		else ddr = 1;
		break;
	case 5: ddr = 2; break;
	case 6: ddr = 1; i852 = 1; break;
	default: ddr = 1;  
	}
	
	if (ddr == 1)
	{
	cprint(POP_Y+3, POP_X+8, "tCAS:  ");
	cprint(POP_Y+5, POP_X+8, "(1) CAS 2.5 ");
	cprint(POP_Y+6, POP_X+8, "(2) CAS 2   ");
	if(!i852) {
	cprint(POP_Y+7, POP_X+8, "(3) CAS 3   ");
	}
	cas = getval(POP_Y+3, POP_X+15, 0);
	popclear();
	}
	else if (ddr == 2)
	{ 
	cprint(POP_Y+3, POP_X+8, "tCAS:  ");
	cprint(POP_Y+5, POP_X+8, "(1) CAS 4 ");
	cprint(POP_Y+6, POP_X+8, "(2) CAS 3 ");
	cprint(POP_Y+7, POP_X+8, "(3) CAS 5 ");
	cas = getval(POP_Y+3, POP_X+15, 0);
	popclear();
	}
	else 
	{
	popclear();
	return(-1);  
	}	
		
	return (cas);
}

void support(void)
{
	if ((ctrl == -1) || ( ctrl > sizeof(mem_ctr)/sizeof(mem_ctr[0])))
	{
		popclear();
		wait_keyup();
		cprint(POP_Y+2, POP_X+4, "   Chipset     ");
		cprint(POP_Y+3, POP_X+4, "Not supported !");
		cprint(POP_Y+5, POP_X+4, "Press any key ");
		cprint(POP_Y+6, POP_X+4, "to continue ");
		get_key();
		wait_keyup();
		popclear();
		ctrl = -1;
		return;
	}
}  

/////////////////////////////////////////////////////////  
// here we go for the exciting timing change part...   //
/////////////////////////////////////////////////////////

void change_timing_i852(int cas, int rcd, int rp, int ras) {

	ulong dramtlr, temp;
	ulong int1, int2;
	
	Pci_conf_read(0, 0, 1, 0x60, 4, &dramtlr);
		
	// CAS Latency (tCAS)
	int1 = dramtlr & 0xFF9F;
	if (cas == 2) { int2 = int1 ^ 0x20;	}
	else if (cas == 1) { int2 = int1; }
	else { int2 = dramtlr; }
	
	temp = int2;
	
	// RAS-To-CAS (tRCD)
	int1 = int2 & 0xFFF3;
	if (rcd == 2) { int2 = int1 ^ 0x8; }
	else if (rcd == 3) { int2 = int1 ^ 0x4; }
	else if (rcd == 4) { int2 = int1; }
	else { int2 = temp;}

	temp = int2;
		
	// RAS Precharge (tRP)
	int1 = int2 & 0xFFFC;
	if (rp == 2) { int2 = int1 ^ 0x2; }
	else if (rp == 3) { int2 = int1 ^ 0x1; }
	else if (rp == 4) { int2 = int1; }
	else { int2 = temp;}

	temp = int2;
	
	// RAS Active to precharge (tRAS)
	int1 = int2 & 0xF9FF;
	if (ras == 5)  { int2 = int1 ^ 0x0600; }
	else if (ras == 6)  { int2 = int1 ^ 0x0400; }
	else if (ras == 7)  { int2 = int1 ^ 0x0200; }
	else if (ras == 8)  { int2 = int1; }
	else { int2 = temp;}
	
	Pci_conf_write(0, 0, 1, 0x60, 4, int2);
	__delay(500);
}

void change_timing_i925(int cas, int rcd, int rp, int ras)
{
	ulong int1, dev0, temp= 0x0;
	long *ptr;
	
	//read MMRBAR
	Pci_conf_read( 0, 0, 0, 0x44, 4, &dev0);
	
	ptr=(long*)(dev0+0x114);
	temp = *ptr;
	
	// RAS-To-CAS (tRCD)
	int1 = temp | 0x70;
	if (rcd == 2) { temp = int1 ^ 0x70; }
	else if (rcd == 3) { temp = int1 ^ 0x60; }
	else if (rcd == 4) { temp = int1 ^ 0x50; }
	else if (rcd == 5) { temp = int1 ^ 0x40; }
	else { temp = temp;}

	//RAS precharge (tRP)
	int1 = temp | 0x7;
	if (rp == 2){ temp = int1 ^ 0x7 ;}
	else if (rp == 3) { temp = int1 ^ 0x6; }
	else if (rp == 4) { temp = int1 ^ 0x5; }
	else if (rp == 5) { temp = int1 ^ 0x4; }
	else { temp = temp;}
	
	// RAS Active to precharge (tRAS)
	int1 = temp | 0xF00000;
	if (ras == 4)  { temp = int1 ^ 0xB00000; }
	else if (ras == 5)  { temp = int1 ^ 0xA00000; }
	else if (ras == 6)  { temp = int1 ^ 0x900000; }
	else if (ras == 7)  { temp = int1 ^ 0x800000; }
	else if (ras == 8)  { temp = int1 ^ 0x700000; }
	else if (ras == 9)  { temp = int1 ^ 0x600000; }
	else if (ras == 10) { temp = int1 ^ 0x500000; }
	else if (ras == 11) { temp = int1 ^ 0x400000; }
	else if (ras == 12) { temp = int1 ^ 0x300000; }
	else if (ras == 13) { temp = int1 ^ 0x200000; }
	else if (ras == 14) { temp = int1 ^ 0x100000; }
	else if (ras == 15) { temp = int1 ^ 0x000000; }
	else { temp = temp;}

	// CAS Latency (tCAS)
	int1 = temp | 0x0300;
	if (cas == 1)  {  temp = int1 ^ 0x200;  }   // cas 2.5
	else if (cas == 2) { temp = int1 ^ 0x100; }
	else if (cas == 3) { temp = int1 ^ 0x300; }
	else  temp = temp;
	
	*ptr = temp;
	__delay(500);
	return;
}

void change_timing_i875(int cas, int rcd, int rp, int ras){
	
	ulong int1, dev6, temp= 0x0;
	long *ptr;
	
	/* Read the MMR Base Address & Define the pointer from the BAR6 overflow register */
	Pci_conf_read( 0, 6, 0, 0x10, 4, &dev6);
	
	ptr=(long*)(dev6+0x60);
	
	temp = *ptr;
			
	// RAS-To-CAS (tRCD)
	int1 = temp | 0xC;
	if (rcd == 2) { temp = int1 ^ 0x4; }
	else if (rcd == 3) { temp = int1 ^ 0x8; }
	else if (rcd == 4) { temp = int1 ^ 0xC; }
	else if (rcd == 5) { temp = int1 ^ 0xC; }
	else { temp = temp;}

	
	//RAS precharge (tRP)
	int1 = temp | 0x3;
	if (rp == 2){ temp = int1 ^ 0x1 ;}
	else if (rp == 3) { temp = int1 ^ 0x2; }
	else if (rp == 4) { temp = int1 ^ 0x3; }
	else if (rp == 5) { temp = int1 ^ 0x3; }
	else { temp = temp;}

	
	// RAS Active to precharge (tRAS)
	int1 = temp | 0x380;
	if (ras == 5)  { temp = int1 ^ 0x100; }
	else if (ras == 6)  { temp = int1 ^ 0x180; }
	else if (ras == 7)  { temp = int1 ^ 0x200; }
	else if (ras == 8)  { temp = int1 ^ 0x280; }
	else if (ras == 9)  { temp = int1 ^ 0x300; }
	else if (ras == 10) { temp = int1 ^ 0x380; }
	else { temp = temp;}

	// CAS Latency (tCAS)
	int1 = temp | 0x60;
	if (cas == 1)  {  temp = int1 ^ 0x60;  }   // cas 2.5
	else if (cas == 2) { temp = int1 ^ 0x40; }
	else if (cas == 3) { temp = int1 ^ 0x20; }
	else { temp = temp; }		
	
	*ptr = temp;
	__delay(500); 
	return;
}


void change_timing_nf2(int cas, int rcd, int rp, int ras) {

	ulong dramtlr, dramtlr2, temp = 0x0;
	ulong int1, int2;
	
	Pci_conf_read(0, 0, 1, 0x90, 4, &dramtlr);
	Pci_conf_read(0, 0, 1, 0xA0, 4, &dramtlr2);
	
	
	// CAS Latency (tCAS)
	int1 = dramtlr2 | 0x0070;
	if (cas == 1)  // cas = 2.5
	{  int2 = int1 ^ 0x10;	}
	else if (cas == 2) { int2 = int1 ^ 0x50; }
	else if (cas == 3) { int2 = int1 ^ 0x40; }
	else { int2 = dramtlr2; }
	
	Pci_conf_write(0, 0, 1, 0xA0, 4, int2);
	
	// RAS-To-CAS (tRCD)
	
	int1 = dramtlr | 0x700000;
	if (rcd == 2) { int2 = int1 ^ 0x500000; }
	else if (rcd == 3) { int2 = int1 ^ 0x400000; }
	else if (rcd == 4) { int2 = int1 ^ 0x300000; }
	else if (rcd == 5) { int2 = int1 ^ 0x200000; }
	else if (rcd == 6) { int2 = int1 ^ 0x100000; }
	else { int2 = dramtlr;}
	
	temp = int2;
	
	
	// RAS Precharge (tRP)
	int1 = temp | 0x70000000;
	if (rp == 2) { int2 = int1 ^ 0x50000000; }
	else if (rp == 3) { int2 = int1 ^ 0x40000000; }
	else if (rp == 4) { int2 = int1 ^ 0x30000000; }
	else if (rp == 5) { int2 = int1 ^ 0x20000000; }
	else if (rp == 6) { int2 = int1 ^ 0x10000000; }
	else { int2 = temp;}
	
	temp = int2;
	
	// RAS Active to precharge (tRAS)

	int1 = temp | 0x78000;
	if (ras == 4)  { int2 = int1 ^ 0x58000; }
	else if (ras == 5)  { int2 = int1 ^ 0x50000; }
	else if (ras == 6)  { int2 = int1 ^ 0x48000; }
	else if (ras == 7)  { int2 = int1 ^ 0x40000; }
	else if (ras == 8)  { int2 = int1 ^ 0x38000; }
	else if (ras == 9)  { int2 = int1 ^ 0x30000; }
	else if (ras == 10) { int2 = int1 ^ 0x28000; }
	else if (ras == 11) { int2 = int1 ^ 0x20000; }
	else if (ras == 12) { int2 = int1 ^ 0x18000; }
	else if (ras == 13) { int2 = int1 ^ 0x10000; }
	else if (ras == 14) { int2 = int1 ^ 0x08000; }
	else { int2 = temp;}
	
	temp = int2;
	
	Pci_conf_write(0, 0, 1, 0x90, 4, temp);
	__delay(500);
}


void change_timing_amd64(int cas, int rcd, int rp, int ras) {

	ulong dramtlr, temp ;
	ulong int1= 0x0; 
	
	Pci_conf_read(0, 24, 2, 0x88, 4, &dramtlr);
		
	temp = dramtlr;
	
	// RAS-To-CAS (tRCD)
	int1 = temp | 0x7000;
	if (rcd == 2) { temp = int1 ^ 0x5000; }
	else if (rcd == 3) { temp = int1 ^ 0x4000; }
	else if (rcd == 4) { temp = int1 ^ 0x3000; }
	else if (rcd == 5) { temp = int1 ^ 0x2000; }
	else if (rcd == 6) { temp = int1 ^ 0x1000; } 
	else if (rcd == 1) { temp = int1 ^ 0x6000; }
	else { temp = temp;}
	
			
	//RAS precharge (tRP)
	int1 = temp | 0x7000000;
	if (rp == 2){ temp = int1 ^ 0x5000000; }
	else if (rp == 3) { temp = int1 ^ 0x4000000; }
	else if (rp == 1) { temp = int1 ^ 0x6000000; }
	else if (rp == 4) { temp = int1 ^ 0x3000000; }
	else if (rp == 5) { temp = int1 ^ 0x2000000; }
	else if (rp == 6) { temp = int1 ^ 0x1000000; } 
	else { temp = temp;}

	
	// RAS Active to precharge (tRAS)
	int1 = temp | 0xF00000;
	if (ras == 5)  { temp = int1 ^ 0xA00000; }
	else if (ras == 6)  { temp = int1 ^ 0x900000; }
	else if (ras == 7)  { temp = int1 ^ 0x800000; }
	else if (ras == 8)  { temp = int1 ^ 0x700000; }
	else if (ras == 9)  { temp = int1 ^ 0x600000; }
	else if (ras == 10) { temp = int1 ^ 0x500000; }
	else if (ras == 11) { temp = int1 ^ 0x400000; }
	else if (ras == 12) { temp = int1 ^ 0x300000; }
	else if (ras == 13) { temp = int1 ^ 0x200000; }
	else if (ras == 14) { temp = int1 ^ 0x100000; }
	else { temp = temp;}

	// CAS Latency (tCAS)
	int1 = temp | 0x7;	// some changes will cause the system hang, tried Draminit to no avail 
	if (cas == 1)  {  temp = int1 ^ 0x2;  }   // cas 2.5
	else if (cas == 2) { temp = int1 ^ 0x6; }
	else if (cas == 3) { temp = int1 ^ 0x5; }
	else if (cas == 4) { temp = int1 ^ 0x7; } //cas 1.5 on a64
	else { temp = temp; }	
	
//	Pci_conf_read(0, 24, 2, 0x90, 4, &dramcr);// use dram init
	Pci_conf_write(0, 24, 2, 0x88, 4, temp);
	__delay(500);
	
////////////////////////////////////////////////////////////////
// trying using the draminit, but do not work
}

static	volatile int d0;
// copy from lib.c code to add delay to chipset timing modification
void __delay(ulong loops)
{
	for(d0=loops;d0;d0--);
}

void amd64_tweak(int rwt, int wrt, int ref, int en2t, int rct, int rrd, int rwqb, int wr)
{
	ulong dramtlr, temp ;
	ulong int1= 0x0; 
	
	Pci_conf_read(0, 24, 2, 0x88, 4, &dramtlr);
		
	temp = dramtlr;
	
	// Row Cycle time
	int1 = temp | 0xF0;
	if (rct == 7) { temp = int1 ^ 0xF0; }
	else if (rct == 8) { temp = int1 ^ 0xE0; }
	else if (rct == 9) { temp = int1 ^ 0xD0; }
	else if (rct == 10) { temp = int1 ^ 0xC0; }
	else if (rct == 11) { temp = int1 ^ 0xB0; } 
	else if (rct == 12) { temp = int1 ^ 0xA0; }
	else if (rct == 13) { temp = int1 ^ 0x90; }
	else if (rct == 14) { temp = int1 ^ 0x80; } 
	else if (rct == 15) { temp = int1 ^ 0x70; }
	else if (rct == 16) { temp = int1 ^ 0x60; }
	else if (rct == 17) { temp = int1 ^ 0x50; } 
	else if (rct == 18) { temp = int1 ^ 0x40; }
	else if (rct == 19) { temp = int1 ^ 0x30; } 
	else if (rct == 20) { temp = int1 ^ 0x20; }
	else { temp = temp;}
	
	//Active-avtive ras-ras delay
	int1 = temp | 0x70000; 
	if (rrd == 2){ temp = int1 ^ 0x50000; } // 2 bus clocks
	else if (rrd == 3) { temp = int1 ^ 0x40000; } //3 bus clocks
	else if (rrd == 4) { temp = int1 ^ 0x30000; } //4 bus clocks
	else { temp = temp;}

	//Write recovery time
	int1 = temp | 0x10000000; 
	if (wr == 2){ temp = int1 ^ 0x10000000; } // 2 bus clocks
	else if (wr == 3) { temp = int1 ^ 0x00000000; } //3 bus clocks
	else { temp = temp;}
	
	Pci_conf_write(0, 24, 2, 0x88, 4, temp);
	__delay(500);
	//////////////////////////////////////////////	
	
	Pci_conf_read(0, 24, 2, 0x8C, 4, &dramtlr);
		
	temp = dramtlr;
	
	// Write-to read delay
	int1 = temp | 0x1;
	if (wrt == 2) { temp = int1 ^ 0x0; }
	else if (wrt == 1) { temp = int1 ^ 0x1; }
	else { temp = temp;}
	
	// Read-to Write delay
	int1 = temp | 0x70;
	if (rwt == 1) { temp = int1 ^ 0x70; }
	else if (rwt == 2) { temp = int1 ^ 0x60; }
	else if (rwt == 3) { temp = int1 ^ 0x50; }
	else if (rwt == 4) { temp = int1 ^ 0x40; }
	else if (rwt == 5) { temp = int1 ^ 0x30; } 
	else if (rwt == 6) { temp = int1 ^ 0x20; }
	else { temp = temp;}
	
	//Refresh Rate
	int1 = temp | 0x1800; 
	if (ref == 1){ temp = int1 ^ 0x1800; } // 15.6us
	else if (ref == 2) { temp = int1 ^ 0x1000; } //7.8us
	else if (ref == 3) { temp = int1 ^ 0x0800; } //3.9us
	else { temp = temp;}

	Pci_conf_write(0, 24, 2, 0x8c, 4, temp);
	__delay(500);
	/////////////////////////////////////
	
	Pci_conf_read(0, 24, 2, 0x90, 4, &dramtlr);
	temp = dramtlr;
	
	// Enable 2t command
	int1 = temp | 0x10000000;
	if (en2t == 2)  { temp = int1 ^ 0x00000000; } // 2T
	else if (en2t == 1) { temp = int1 ^ 0x10000000; } // 1T
	else { temp = temp;}

	// Read Write queue bypass count
	int1 = temp | 0xC000;
	if (rwqb == 2) { temp = int1 ^ 0xC000; }
	else if (rwqb == 4)  { temp = int1 ^ 0x8000; }
	else if (rwqb == 8)  { temp = int1 ^ 0x4000; }
	else if (rwqb == 16) { temp = int1 ^ 0x0000; }
	else { temp = temp;}
	
	Pci_conf_write(0, 24, 2, 0x90, 4, temp);
	__delay(500);
	restart();	
}

