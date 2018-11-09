/*
 * This is userland program. This opens the network interface and attaches a data gram socket.
 * The communication to the hardware is accomplished through the Ioctl's sent through the 
 * datagram socket.
 */

/*	sw
#include <stdio.h>
#define ENH_DESC_8W  //comment this if intended to use normal descriptor or 4 words
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <string.h>
#include <linux/netdevice.h>
*/

#include "GMAC_Pmon.h"

#define IOCTL_READ_REGISTER  SIOCDEVPRIVATE+1
#define IOCTL_WRITE_REGISTER SIOCDEVPRIVATE+2
#define IOCTL_READ_IPSTRUCT  SIOCDEVPRIVATE+3
#define IOCTL_READ_RXDESC    SIOCDEVPRIVATE+4
#define IOCTL_READ_TXDESC    SIOCDEVPRIVATE+5
#define IOCTL_POWER_DOWN     SIOCDEVPRIVATE+6

typedef unsigned long long u64;
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed long long s64;
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;
typedef int bool;
typedef unsigned long dma_addr_t;

struct ifr_data_struct
{
  u32 unit;
  u32 addr;
  u32 data;
};

struct reg
{
  u32    addr;
  char   * name;
};

enum WcsCmd
{
	wcs_write = 1,
	wcs_set,
	wcs_clr
};

/*
DMA Descriptor Structure
The structure is common for both receive and transmit descriptors
*/
#ifdef ENH_DESC_8W
typedef struct DmaDescStruct    
{                               
  u32   status;         /* Status 									*/
  u32   length;         /* Buffer 1  and Buffer 2 length 						*/
  u32   buffer1;        /* Network Buffer 1 pointer (Dma-able) 							*/
  u32   buffer2;        /* Network Buffer 2 pointer or next descriptor pointer (Dma-able)in chain structure 	*/
  			/* This data below is used only by driver					*/
  u32   extstatus;      /* Extended status of a Rx Descriptor                                           */
  u32   reserved1;      /* Reserved word                                                                */
  u32   timestamplow;   /* Lower 32 bits of the 64 bit timestamp value                                  */
  u32   timestamphigh;  /* Higher 32 bits of the 64 bit timestamp value                                  */
  u32   data1;          /* This holds virtual address of buffer1, not used by DMA  			*/
  u32   data2;          /* This holds virtual address of buffer2, not used by DMA  			*/
} DmaDesc;

#else
typedef struct DmaDescStruct    
{                               /* put it here to allow the caller to know its size */
  u32   status;         /* Status */
  u32   length;         /* Buffer length */
  u32   buffer1;        /* Buffer 1 pointer */
  u32   buffer2;        /* Buffer 2 pointer or next descriptor pointer in chain structure */

  u32   data1;          /* driver data, are not used by DMA engine,                       */
  u32   data2;          /* set DmaDescriptorSkip2 in DmaBusModeInit to skip these words */

} DmaDesc;
#endif

/* synopGMACdevice data */

typedef struct synopGMACDeviceStruct      
{
  u64 MacBase;          /* base address of MAC registers       */
  u64 DmaBase;          /* base address of DMA registers       */
  u32 PhyBase;          /* PHY device address on MII interface */
  u32 Version;		/* Gmac Revision version	       */		
	

  dma_addr_t TxDescDma;
  dma_addr_t RxDescDma;  
  DmaDesc *TxDesc;      /* start of TX descriptors ring/chain  */
  DmaDesc *RxDesc;      /* start of RX descriptors ring/chain  */  
 
  u32 BusyTxDesc;		 /* Number of Tx Descriptors owned by DMA at any given time*/  
  u32 BusyRxDesc;		 /* Number of Rx Descriptors owned by DMA at any given time*/  
  
  u32  RxDescCount;     /* number of rx descriptors */
  u32  TxDescCount;     /* number of tx descriptors */
  
  u32  TxBusy;          /* first descriptor owned by DMA engine, moved by DmaTxGet */
  u32  TxNext;          /* next available tx descriptor, moved by DmaTxSet */
  u32  RxBusy;          /* first descripror owned by DMA engine, moved by DmaRxGet */
  u32  RxNext;          /* next available rx descriptor, moved by DmaRxSet */

  DmaDesc * TxBusyDesc;    /* first descriptor owned by DMA engine, moved by DmaTxGet */
  DmaDesc * TxNextDesc;    /* next available tx descriptor, moved by DmaTxSet */
  DmaDesc * RxBusyDesc;    /* first descripror owned by DMA engine, moved by DmaRxGet */
  DmaDesc * RxNextDesc;    /* next available rx descriptor, moved by DmaRxSet */

  /*Phy related stuff*/
  u32 ClockDivMdc;
  /* The status of the link */
  u32 LinkState;
  u32 DuplexMode;
  u32 Speed;
  u32 LoopBackMode; 
  
} synopGMACdevice;



static struct reg mac[] =
{
  { 0x0000, "          Config" },
  { 0x0004, "    Frame Filter" },
  { 0x0008, "     MAC HT High" },
  { 0x000C, "      MAC HT Low" },
  { 0x0010, "       GMII Addr" },
  { 0x0014, "       GMII Data" },
  { 0x0018, "    Flow Control" },
  { 0x001C, "        VLAN Tag" },
  { 0x0020, "    GMAC Version" },
  { 0x0024, "    GMAC Debug  " },
  { 0x0040, "  MAC Addr0 High" },
  { 0x0044, "   MAC Addr0 Low" },
  { 0x0048, "  MAC Addr1 High" },
  { 0x004c, "   MAC Addr1 Low" },
  { 0x0100, "   MMC Ctrl Reg " },
  { 0x010c, "MMC Intr Reg(rx)" },
  { 0x0110, "MMC Intr Reg(tx)" },
  { 0x0200, "MMC Intr Reg(rx ipc)" },
  { 0, 0 }
};
static struct reg ts[] =
{
  { 0x0700, "     TS Ctrl Reg" },
  { 0x0704, "Sub Sec Incr Reg" },
  { 0x0708, "     TS High Reg" },
  { 0x070C, "     TS Low  Reg" },
  { 0x0710, "TS High Updt Reg" },
  { 0x0714, "TS Low  Updt Reg" },
  { 0x0718, "   TS Addend Reg" },
  { 0x071C, "Tgt Time (H) Reg" },
  { 0x0720, "Tgt Time (L) Reg" },
  { 0x0724, "TS High Word Reg" },
  { 0x0728, "   TS Status Reg" },
  { 0x072C, " TS PPS Ctrl Reg" },
  { 0, 0 }
};

static struct reg dma[] =
{
  { 0x0000, "CSR0   Bus Mode" },
  { 0x0004, "CSR1   TxPlDmnd" },
  { 0x0008, "CSR2   RxPlDmnd" },
  { 0x000C, "CSR3    Rx Base" },
  { 0x0010, "CSR4    Tx Base" },
  { 0x0014, "CSR5     Status" },
  { 0x0018, "CSR6    Control" },
  { 0x001C, "CSR7 Int Enable" },
  { 0x0020, "CSR8 Missed Fr." },
  { 0x0048, "CSR18 Tx Desc  " },
  { 0x004C, "CSR19 Rx Desc  " },
  { 0x0050, "CSR20 Tx Buffer" },
  { 0x0054, "CSR21 Rx Buffer" },
  { 0x0058, "CSR22 HWCFG    " },
  { 0, 0 }
};

static struct reg mii[] =
{
  { 0x0000, "Phy Control Reg" },
  { 0x0001, "Phy Status  Reg" },
  { 0x0002, "Phy Id (02)    " },
  { 0x0003, "Phy Id (03)    " },
  { 0x0004, "Auto-nego   Adv" },
  { 0x0005, "Auto-nego   Lpa" },
  { 0x0006, "Auto-nego   Exp" },
  { 0x0007, "Auto-nego    Np" },
  { 0x0009, "1000 Ctl    Reg" },
  { 0x000a, "1000 Sts    Reg" },
  { 0x0010, "PHY  Ctl    Reg" },
  { 0x0011, "PHY  Sts    Reg" },
  { 0, 0 }
};

static struct reg mmc[] = 
{

  { 0x0114, "No. Of Bytes  Transmitted (Good/Bad)                            " },
  { 0x0164, "No. Of Bytes  Transmitted (Good)                                " },
  { 0x0118, "No. Of Frames Trsnsmitted (Good/Bad)                            " },
  { 0x0168, "No. Of Frames Trsnsmitted (Good)                                " },
  
  { 0x013C, "No. Of Unicast Frames Transmitted (Good/Bad)                    " },
  { 0x011C, "No. Of Broadcast Frames Transmitted (Good)                      " },
  { 0x0140, "No. Of Broadcast Frames Transmitted (Good/Bad)                  " },
  { 0x0120, "No. Of Multicast Frames Transmitted (Good)                      " },
  { 0x0144, "No. Of Multicast Frames Transmitted (Good/Bad)                  " },
  
  { 0x0124, "No. Of Tx frames with Length 64 bytes (Good/Bad)                " },
  { 0x0128, "No. Of Tx frames with Length <064,128> bytes (Good/Bad)         " },
  { 0x012C, "No. Of Tx frames with Length <128,256> bytes (Good/Bad)         " },
  { 0x0130, "No. Of Tx frames with Length <256,512> bytes (Good/Bad)         " },
  { 0x0134, "No. Of Tx frames with Length <512,1024> bytes (Good/Bad)        " },
  { 0x0138, "No. Of Tx frames with Length <1024,MaxSize> bytes (Good/Bad)    " },
  
  { 0x0148, "No. Of Tx aborted due to Underflow error                        " },
  { 0x0158, "No. Of Tx aborted due to Late Collision                         " },
  { 0x015C, "No. Of Tx aborted due to Excessive Collision                    " },
  { 0x0160, "No. Of Tx aborted due to Carrier Sense error                    " },
  { 0x016C, "No. Of Tx aborted due to Excessive Defferal error               " },
  
  { 0x014C, "No. Of Tx frames after single collision in Half Duplex mode     " },
  { 0x0150, "No. Of Tx frames after multiple collision in Half Duplex mode   " },
  { 0x0154, "No. Of Tx frames after a deferral                               " },

  { 0x0170, "No. Of Pause frames Transmitted   (Good)                        " },
  { 0x0174, "No. Of VLAM tagged frames Transmitted  (Good)                   " },

 

  { 0x0184, "No. Of Bytes  Received    (Good/Bad)                            " },
  { 0x0188, "No. Of Bytes  Received    (Good)                                " },
  { 0x0180, "No. Of Frames Received    (Good/Bad)                            " },
  
  { 0x01C4, "No. Of Unicast Frames Received (Good)                           " },
  { 0x018C, "No. Of Broadcast Frames Transmitted (Good)                      " },
  { 0x0190, "No. Of Multicast Frames Transmitted (Good)                      " },

  { 0x01AC, "No. Of Rx frames with Length 64 bytes (Good/Bad)                " },
  { 0x01B0, "No. Of Rx frames with Length <064,128> bytes (Good/Bad)         " },
  { 0x01B4, "No. Of Rx frames with Length <128,256> bytes (Good/Bad)         " },
  { 0x01B8, "No. Of Rx frames with Length <256,512> bytes (Good/Bad)         " },
  { 0x01BC, "No. Of Rx frames with Length <512,1024> bytes (Good/Bad)        " },
  { 0x01C0, "No. Of Rx frames with Length <1024,MaxSize> bytes (Good/Bad)    " },

  { 0x01A4, "No. Of Runt Rx frames with frames (<64)                         " },
  { 0x01A8, "No. Of Jabber Rx frames with frames (>1518)                     " },
  { 0x0194, "No. Of Rx frames with CRC error                                 " },
  { 0x0198, "No. Of Rx frames with dribble error (only in 10/100)            " },
  { 0x019C, "No. Of Rx frames with runt error (<64 and CRC error)            " },
  { 0x01A0, "No. Of Rx frames with jabber error (>1518 and CRC)              " },
  { 0x01D4, "No. Of Rx frames missed due to FIFO overflow                    " },
  { 0x01DC, "No. Of Rx frames received with watchdog timeout error           " },
  { 0x01C8, "No. Of Rx frames with Length != Frame Size                      " },
  { 0x01CC, "No. Of Rx frames with Length != Valid Frame Size                " },

  { 0x0170, "No. Of Pause frames Received (Good)                             " },
  { 0x0174, "No. Of VLAM tagged frames Received (Good)                       " },



  { 0x0210, "No. Of IPV4 Packets  Received (Good)                            " },
  { 0x021C, "No. Of IPV4 Packets with fragmentation Received                 " },
  { 0x0220, "No. Of IPV4 Packets with UDP checksum disabled                  " },

  { 0x0214, "No. Of IPV4 Packets with Header Error Received                  " },
  { 0x0218, "No. Of IPV4 Packets with No Payload Error Received              " },

  { 0x0224, "No. Of IPV6 Packets  Received (Good)                            " },

  { 0x0228, "No. Of IPV4 Packets with Header Error Received                  " },
  { 0x022C, "No. Of IPV6 Packets with No Payload Error Received              " },

  { 0x0240, "No. Of ICMP Frames Received (Good)                              " },
  { 0x0244, "No. Of ICMP Frames Received with Error                          " },

  { 0x0230, "No. Of UDP Frames Received (Good)                               " },
  { 0x0234, "No. Of UDP Frames Received with Error                           " },

  { 0x0238, "No. Of TCP Frames Received (Good)                               " },
  { 0x023C, "No. Of TCP Frames Received with Error                           " },

  { 0, 0 }
};


static char copyright[] =
    "\n"
    " ********************************************\n"
    " *  synopGMAC debug Utility for Linux       *\n"
    " *  Copyright(c) 2007-2007 Synopsys, Inc    *\n"
    " ********************************************\n\n"
    ;


static char usage[] =
    "  Usage:\n"
    "    synopGMAC_Debug  <interface> <command> {<parameters>}\n"
    "\n"
    "  Commands:\n"
    "    dump          <unit>  - dump registers,     unit ::= {mac|ts|dma|mii|mmc}\n"
    "    read          <unit> <reg>                  unit ::= {mac|ts|dma|mii}\n"
    "    write/set/clr <unit> <reg> <value>          unit ::= {mac|ts|dma|mii}\n"
    "                 - write/set/clr bits\n"
    "    status       - print driver's internal data and dump DMA descriptors\n"
    "    powerdown  <on/off>\n"
    "\n"
    ;


static int powerdown(char *ifname, int argc, char *argv[] )
{

  int socket_desc;
  int retval = 0;
  struct ifreq ifr;  //defined in if.h

  struct ifr_data_struct data;
  struct reg *regs;

  if(argc != 1) return 1;
  
  if     ( strcmp( argv[0], "on"  ) == 0 ) { data.unit = 1;}
  else if( strcmp( argv[0], "off" ) == 0 ) { data.unit = 2;}
  else return 1;

  socket_desc = socket( PF_INET, SOCK_DGRAM, 0 );
  if( socket_desc < 0 )
  {
  	perror("Socket error");
	return -1;
  }

  strcpy( ifr.ifr_name, ifname );

  ifr.ifr_data = (void *) &data;

  retval = ioctl( socket_desc, IOCTL_POWER_DOWN, &ifr );
  if( retval < 0 )
   {
      printf("IOCTL Error");
      goto close_socket;
   }

  printf("The GMAC is Successfully powered %s\n", (( data.unit == 1 ) ? "Down":"Up" ) );
  
 close_socket:  close(socket_desc);
  return retval;
}

static int dump( char *ifname, int argc, char *argv[] )
{
  int socket_desc;
  int retval = 0;
  struct ifreq ifr;  //defined in if.h

  struct ifr_data_struct data;
  struct reg *regs;
  
  if(argc!=1) return 1;
  if     ( strcmp( argv[0], "mac" ) == 0 ) { data.unit = 0; regs = mac; }
  else if( strcmp( argv[0], "ts" ) == 0 ) { data.unit = 0; regs = ts; }
  else if( strcmp( argv[0], "dma" ) == 0 ) { data.unit = 1; regs = dma; }
  else if( strcmp( argv[0], "mii" ) == 0 ) { data.unit = 2; regs = mii; }
  else if( strcmp( argv[0], "mmc" ) == 0 ) { data.unit = 0; regs = mmc; }
  else return 1;

  printf("%s Dump %s Registers: \n", ifname, argv[0]);

  socket_desc = socket( PF_INET, SOCK_DGRAM, 0 );
  if( socket_desc < 0 )
  {
  	perror("Socket error");
	return -1;
  }

  strcpy( ifr.ifr_name, ifname );

  ifr.ifr_data = (void *) &data;

  while( regs->name != 0 )
  {
    data.addr = regs->addr;
    retval = ioctl( socket_desc, IOCTL_READ_REGISTER, &ifr );
  
    if( retval < 0 )
    {
      printf("IOCTL Error");
      break;
    }
    printf( "%s (0x%04x) = 0x%08x\n", regs->name, regs->addr, data.data );

    regs++;
  }
  close(socket_desc);
  return retval;
}

static int read(char *ifname, int argc, char *argv[])
{
  int socket_desc;
  int retval = 0;
  struct ifreq ifr;  //defined in if.h

  struct ifr_data_struct data;
  struct reg *regs;

  if(argc != 2) return 1;
  if     ( strcmp( argv[0], "mac" ) == 0 ) { data.unit = 0; regs = mac; }
  else if( strcmp( argv[0], "ts" ) == 0 ) { data.unit = 0; regs = ts; }
  else if( strcmp( argv[0], "dma" ) == 0 ) { data.unit = 1; regs = dma; }
  else if( strcmp( argv[0], "mii" ) == 0 ) { data.unit = 2; regs = mii; }
  else return 1;
  
  strcpy( ifr.ifr_name, ifname );

  ifr.ifr_data = (void *) &data;

 
   data.addr = strtol(argv[1],NULL,16); 		//convert the register address argument from string to long
   if(((data.unit == 0) && (data.addr > 0x000002FC)) || ((data.addr & 0x00000003) != 0))
	return 1;
   if(((data.unit == 1) && (data.addr > 0x00000058)) || ((data.addr & 0x00000003) != 0))
	return 1;
   if((data.unit == 2) && (data.addr > 0x0000001c))
	return 1;

  socket_desc = socket( PF_INET, SOCK_DGRAM, 0 );
  if( socket_desc < 0 )
  {
  	perror("Socket error");
	return -1;
  }
   retval = ioctl( socket_desc, IOCTL_READ_REGISTER, &ifr );
  
    if(retval < 0)
    {
	printf("IOCTL Error");
 	goto close_socket;		
    }
   printf( "[%s](0x%04x) = 0x%08x\n ",argv[0],data.addr, data.data);
  
close_socket:  close(socket_desc);
	  return retval;
}




static int wcs(int cmd, char * ifname, char * cmdname, int argc,  char *argv[])
{
int socket_desc;
int retval = 0;
struct ifreq ifr; //defined in if.h

struct ifr_data_struct data;
struct reg *regs;
u32 data_to_write;

if (argc != 3) return -1;

if     (strcmp (argv[0],"mac") == 0){
	data.unit = 0;
	regs = mac;
}
else if (strcmp (argv[0],"ts") == 0){
	data.unit = 0;
	regs = ts;
}
else if (strcmp (argv[0],"dma") == 0){
	data.unit = 1;
	regs = dma;
}
else if (strcmp (argv[0],"mii") == 0){
	data.unit = 2;
	regs = mii;
}
else return -1;

sscanf(argv[1], "%x", &data.addr);
while(regs->name != 0){
	if(regs->addr == data.addr) break;
	regs++;
}
if(regs->name == 0){
	printf("Invalid %s Register   - 0x%02x\n",argv[0],data.addr);
	return -1;
}
sscanf(argv[2],"%x", &data_to_write);
printf("%s %s 0x%08x to %s Register 0x%02x\n",ifname,cmdname,data_to_write,argv[0],data.addr);

socket_desc = socket( PF_INET, SOCK_DGRAM, 0 );
if( socket_desc < 0 )
{
	perror("Socket error");
	return -1;
}

strcpy( ifr.ifr_name, ifname );
ifr.ifr_data = (void *) &data;

if(cmd == wcs_set || cmd == wcs_clr){
	retval = ioctl(socket_desc,IOCTL_READ_REGISTER, &ifr);
	if(retval < 0){
		perror("IOCTL Error");
	}
}

if(retval == 0){
	if(cmd == wcs_set  ) data.data |= data_to_write;
	if(cmd == wcs_clr) data.data &= (~data_to_write);
	if(cmd == wcs_write) data.data  = data_to_write;
}

retval = ioctl(socket_desc,IOCTL_WRITE_REGISTER, &ifr);
if(retval < 0)
	perror("IOCTL Error");

close(socket_desc);
return retval;


}



static int status(char * ifname, int argc, char *argv[])
{
  int socket_desc;
  int retval = 0;
  int count = 0;
  struct ifreq ifr;  //defined in if.h
  
  DmaDesc    dmadesc;
  synopGMACdevice  gmacdev;
       
  if(argc!=0) return 1;

  printf("%s Dump Status: \n", ifname);

  socket_desc = socket( PF_INET, SOCK_DGRAM, 0 );
  if( socket_desc < 0 )
  {
  	perror("Socket error");
	return -1;
  }

  strcpy( ifr.ifr_name, ifname );

  ifr.ifr_data = (void *) &gmacdev;
  
  retval = ioctl( socket_desc, IOCTL_READ_IPSTRUCT, &ifr );

  if( retval < 0 )
  {
  	printf("IOCTL Error");
  } 

  else
  {
    printf("GMAC ip Version                              = 0x%08x\n", gmacdev.Version );	
    printf("GMAC   Register Base Address                 = 0x%08x\n", gmacdev.MacBase );
    printf("DMA    Register Base Address                 = 0x%08x\n", gmacdev.DmaBase );
    printf("PHY device Base Address(32 phys possible)    = 0x%08x\n", gmacdev.PhyBase );
    printf("Start of Rx Desc Ring                        = %p\n", gmacdev.RxDesc );
    printf("Start of Tx Desc Ring                        = %p\n", gmacdev.TxDesc );
    
    printf("Busy Rx Desc Count                           = %d\n", gmacdev.BusyRxDesc );
    printf("Busy Tx Desc Count                           = %d\n", gmacdev.BusyTxDesc );

    printf("Start of Rx Desc Ring(DMA)                   = %p\n", gmacdev.RxDescDma );
    printf("Start of Tx Desc Ring(DMA)                   = %p\n", gmacdev.TxDescDma );
    printf("Number of Rx Descriptors                     = %d\n", gmacdev.RxDescCount );
    printf("Number of Tx Descriptors                     = %d\n", gmacdev.TxDescCount );
    printf("First Rx Descriptor Owned by DMA Engine      = %d\n", gmacdev.RxBusy );
    printf("Next Available Rx Descriptor                 = %d\n", gmacdev.RxNext );
    printf("First Tx Descriptor Owned by DMA Engine      = %d\n", gmacdev.TxBusy );
    printf("Next Available Tx Descriptor                 = %d\n", gmacdev.TxNext );
    printf("MDC clock division number                    = 0x%08x\n", gmacdev.ClockDivMdc);	
    printf("Link State information                       = 0x%08x\n", gmacdev.LinkState );
    printf("Duplex Mode of the Interface                 = 0x%08x\n", gmacdev.DuplexMode );
    printf("Speed of the Interface                       = 0x%08x\n", gmacdev.Speed );
    printf("LoopBack status of the Interface             = 0x%08x\n", gmacdev.LoopBackMode);

    printf("\n");
#ifdef ENH_DESC
  printf("RxD Idx:   Status      length      buffer1     buffer2     data1       data2     Ext_status  reserved       TSL         TSH    \n");
    printf("---------------------------------------------------------------------------------------------------------------------------- \n");
	    for(count = 0; count < gmacdev.RxDescCount; count++ ){
		dmadesc.data1 = count;
		ifr.ifr_data = (void *) &dmadesc;
		retval = ioctl(socket_desc, IOCTL_READ_RXDESC,&ifr);
		if(retval < 0)
			break;
		printf("RxD  %02x: 0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x\n",
                       count,dmadesc.status, dmadesc.length,dmadesc.buffer1,dmadesc.buffer2,
			dmadesc.data1,dmadesc.data2,dmadesc.extstatus,dmadesc.reserved1,dmadesc.timestamplow,dmadesc.timestamphigh);
	    }
	    if(retval < 0)
		perror("IOCTL Error");
	    
            printf("\n");

    printf("TxD Idx:   Status      length      buffer1     buffer2     data1       data2   reserved      reserved      TSL           TSH \n");
    printf("---------------------------------------------------------------------------------------------------------------------------- \n");
	    for(count = 0; count < gmacdev.TxDescCount; count++ ){
		dmadesc.data1 = count;
		ifr.ifr_data = (void *) &dmadesc;
		retval = ioctl(socket_desc, IOCTL_READ_TXDESC,&ifr);
		if(retval < 0)
			break;
		printf("TxD  %02x: 0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x   0x%08x  0x%08x  0x%08x  0x%08x\n",
                       count,dmadesc.status, dmadesc.length,dmadesc.buffer1,dmadesc.buffer2,
			dmadesc.data1,dmadesc.data2,dmadesc.extstatus,dmadesc.reserved1,dmadesc.timestamplow,dmadesc.timestamphigh);
	    }
	    if(retval < 0)
		perror("IOCTL Error");
#else
  printf("RxD Idx:   Status      length      buffer1     buffer2     data1       data2 \n");
    printf("---------------------------------------------------------------------------\n");
	    for(count = 0; count < gmacdev.RxDescCount; count++ ){
		dmadesc.data1 = count;
		ifr.ifr_data = (void *) &dmadesc;
		retval = ioctl(socket_desc, IOCTL_READ_RXDESC,&ifr);
		if(retval < 0)
			break;
		printf("RxD  %02x: 0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x\n",
                       count,dmadesc.status, dmadesc.length,dmadesc.buffer1,dmadesc.buffer2,
			dmadesc.data1,dmadesc.data2);
	    }
	    if(retval < 0)
		perror("IOCTL Error");
	    
            printf("\n");

    printf("TxD Idx:   Status      length      buffer1     buffer2     data1       data2 \n");
    printf("-----------------------------------------------------------------------------\n");
	    for(count = 0; count < gmacdev.TxDescCount; count++ ){
		dmadesc.data1 = count;
		ifr.ifr_data = (void *) &dmadesc;
		retval = ioctl(socket_desc, IOCTL_READ_TXDESC,&ifr);
		if(retval < 0)
			break;
		printf("TxD  %02x: 0x%08x  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x \n",
                       count,dmadesc.status, dmadesc.length,dmadesc.buffer1,dmadesc.buffer2,
			dmadesc.data1,dmadesc.data2);
	    }
	    if(retval < 0)
		perror("IOCTL Error");
#endif  
  }
   close(socket_desc);
   return 0;

}

int main( int argc, char *argv[] )
{
  int retval = 1;
  
  printf(copyright);
  
  if( argc>=3 )
  {
//    if( strcmp( argv[2], "debug" ) == 0 )    retval = debug( argv[1], argc-3, argv+3 );
    if( strcmp( argv[2], "dump" ) == 0 )     retval = dump( argv[1], argc-3, argv+3 );
    if( strcmp( argv[2], "write" ) == 0 )    retval = wcs( wcs_write, argv[1], argv[2], argc-3, argv+3 );
    if( strcmp( argv[2], "read" ) == 0 )    retval = read(  argv[1], argc-3, argv+3 );
    if( strcmp( argv[2], "set" ) == 0 )      retval = wcs( wcs_set, argv[1], argv[2], argc-3, argv+3 );
    if( strcmp( argv[2], "clr" ) == 0 )    retval = wcs( wcs_clr, argv[1], argv[2], argc-3, argv+3 );
    if( strcmp( argv[2], "status" ) == 0 )   retval = status( argv[1], argc-3, argv+3 );
    if( strcmp( argv[2], "powerdown") == 0)  retval = powerdown(argv[1],argc-3,argv+3);
  }

  if( retval==1 ) printf( usage );
  return retval;
}
