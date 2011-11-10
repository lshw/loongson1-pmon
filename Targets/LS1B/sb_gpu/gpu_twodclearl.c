//Created by xiexin for Display Controller pmon test 
//Oct 6th,2009



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pmon.h>
#include <include/fcr.h>

#define SYNC_W 1

typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;
typedef int bool;
typedef unsigned long dma_addr_t;


#define writeb(val, addr) (*(volatile u8*)(addr) = (val))
#define writew(val, addr) (*(volatile u16*)(addr) = (val))
#define writel(val, addr) (*(volatile u32*)(addr) = (val))
#define readb(addr) (*(volatile u8*)(addr))
#define readw(addr) (*(volatile u16*)(addr))
#define readl(addr) (*(volatile u32*)(addr))

#define write_reg(addr,val) writel(val,addr)

#define DIS_WIDTH  640
#define DIS_HEIGHT 480 
#define ACTU_WIDTH  640
#define ACTU_HEIGHT 480 
#define EXTRA_PIXEL  0
#define DC_BASE_ADDR 0xbc301240
#define DC_BASE_ADDR_1 0xbc301250


void displayasc(char * a,int b,int c, int d,int e,int f);
void soft_display(int a,int b,int c);

int gpu_reg_wr()
{

	int base_reg_addr=0xbc200000;
	int temp;
	temp=base_reg_addr + 0x0008*4;
	printf("chipid:%x\n",readl(temp));
	temp=base_reg_addr + 0x0009*4;
	printf("chiprev:%x\n",readl(temp));
	temp=base_reg_addr + 0x000a*4;
	printf("chipdate:%x\n",readl(temp));
	temp=base_reg_addr + 0x000b*4;
	printf("chiptime:%x\n",readl(temp));
	temp=base_reg_addr + 0x000c*4;
	printf("chipcustomer:%x\n",readl(temp));

printf("read addr bc200014 ,val:%x\n",readl(0xbc200014));
writel(0xaaaaaaaa,0xbc200014);
printf("write addr bc200014 with val 0xaaaaaaaa\n");
printf("read addr bc200014 ,val:%x\n",readl(0xbc200014));


printf("read addr bc200000 ,val:%x\n",readl(0xbc200000));
printf("read addr bc200654 ,val:%x\n",readl(0xbc200654));
printf("read addr bc200658 ,val:%x\n",readl(0xbc200658));
printf("read addr bc20065c ,val:%x\n",readl(0xbc20065c));

return 0;
}


unsigned long GPU_fbaddr;
//test for gpu_twodclears
//int gpu_twodclearl(int framebuf_addr)
int VIR_CMDBUF_ADDR=0;
int gpu_twodclearl(void)
{
	int cmd_buf_size = 4096;
	int CMDBUF_ADDR,FRAMEBUF_ADDR;
	int i;
	char *cmd_ptr = (char *)malloc(cmd_buf_size);
	CMDBUF_ADDR = cmd_ptr;
	VIR_CMDBUF_ADDR = cmd_ptr;
	CMDBUF_ADDR = (CMDBUF_ADDR + 16) & 0x0ffffff0;
	FRAMEBUF_ADDR = GPU_fbaddr & 0x0fffffff;
	VIR_CMDBUF_ADDR = (VIR_CMDBUF_ADDR + 16) & 0xfffffff0;
        printf("cmd buffer addr: %x \n",(VIR_CMDBUF_ADDR+0x00));

        
        write_reg((VIR_CMDBUF_ADDR+0x00),0x08010e00);
        write_reg((VIR_CMDBUF_ADDR+0x04),0x00000001);
        write_reg((VIR_CMDBUF_ADDR+0x08),0x0801049a);
        write_reg((VIR_CMDBUF_ADDR+0x0c),0x000000ff);
        write_reg((VIR_CMDBUF_ADDR+0x10),0x0801049c);
        write_reg((VIR_CMDBUF_ADDR+0x14),0xdeadbeef);
        write_reg((VIR_CMDBUF_ADDR+0x18),0x0801049d);
        write_reg((VIR_CMDBUF_ADDR+0x1c),0xdeadbeef);
        write_reg((VIR_CMDBUF_ADDR+0x20),0x0801048a);
        write_reg((VIR_CMDBUF_ADDR+0x24),FRAMEBUF_ADDR);
        write_reg((VIR_CMDBUF_ADDR+0x28),0x0801048b);
        write_reg((VIR_CMDBUF_ADDR+0x2c),0x00000a00);
        write_reg((VIR_CMDBUF_ADDR+0x30),0x0801048c);
        write_reg((VIR_CMDBUF_ADDR+0x34),0x00000000);
        write_reg((VIR_CMDBUF_ADDR+0x38),0x0801048d);
        write_reg((VIR_CMDBUF_ADDR+0x3c),0x00000006);
        write_reg((VIR_CMDBUF_ADDR+0x40),0x08010483);
        write_reg((VIR_CMDBUF_ADDR+0x44),0x00000000);
        write_reg((VIR_CMDBUF_ADDR+0x48),0x08010498);
        write_reg((VIR_CMDBUF_ADDR+0x4c),0x00000000);
        write_reg((VIR_CMDBUF_ADDR+0x50),0x08010499);
        write_reg((VIR_CMDBUF_ADDR+0x54),0x01e00280);
        write_reg((VIR_CMDBUF_ADDR+0x58),0x08010497);
        write_reg((VIR_CMDBUF_ADDR+0x5c),0x00200000);
        write_reg((VIR_CMDBUF_ADDR+0x60),0x20000100);
        write_reg((VIR_CMDBUF_ADDR+0x64),0x00000000);
        write_reg((VIR_CMDBUF_ADDR+0x68),0x00000000);
        write_reg((VIR_CMDBUF_ADDR+0x6c),0x01e00280);
        write_reg((VIR_CMDBUF_ADDR+0x70),0x08010e03);
        write_reg((VIR_CMDBUF_ADDR+0x74),0x00000008);


        write_reg((VIR_CMDBUF_ADDR+0x78),0x08010e02);
        write_reg((VIR_CMDBUF_ADDR+0x7c),0x00000701);
        write_reg((VIR_CMDBUF_ADDR+0x80),0x48000000);
        write_reg((VIR_CMDBUF_ADDR+0x84),0x00000701);
        write_reg((VIR_CMDBUF_ADDR+0x88),0x10000000);
        write_reg((VIR_CMDBUF_ADDR+0x8c),0x00000000);

        write_reg((0xbc200654),CMDBUF_ADDR);
        write_reg((0xbc200658),0xffffffff);

        free(cmd_ptr);
        return 0;
}

int dump_gpu_cmd_buf(void)
{
	//print cmd buf
    int i=0;
	printf("print GPU cmd buffer:\n");
	for(i=VIR_CMDBUF_ADDR;i<VIR_CMDBUF_ADDR+0x8c;i+=4)
		printf("%p : %x\n",i,readl(i));


	return 0;
}
#if 0
int dc_init()
{
   int MEM_ADDR =0;
   int print_count;
   int i;
   int PIXEL_COUNT = DIS_WIDTH * DIS_HEIGHT +  EXTRA_PIXEL; 
   int MEM_SIZE = PIXEL_COUNT * 4;
   int init_R = 0;
   int init_G = 0;
   int init_B = 0;
   int j,k,l,va,va_mod;
   int MEM_SIZE_6 = MEM_SIZE /6; 
   char *p="AAAAA\nBBBBBBB\nCCCCCCC\nDDDDDDD\nEEEEEEEEEEEEE\nFFFFFFFFFFFFFF\n";
   char *q = "";
   char convert[3] = {0};
   char temp[3]  = {0};
   
   //debug for dc malloc
  // char *MEM_ptr =(volatile u8*)(MEM_ADDR);  
   int  print_addr;
   int print_data;
  //////////////////////////////////////////////////////////////
  // char *MEM_ptr = (char *)malloc(MEM_SIZE,M_MBUF, 0x1);
   char *MEM_ptr = (char *)malloc(MEM_SIZE);
   MEM_ADDR = MEM_ptr;
  ///////////////////////////////////////////////////////////////
   printf("enter dc_init...\n");
   if(MEM_ptr == NULL)
   {
       printf("frame buffer memory malloc failed!\n ");
       exit(0);
   }
   else
   {

        for (i=0;i<MEM_SIZE;i+=4)
        {
           //paint ARGB 0 255 0 0 high <- low
           *(MEM_ptr+i) = 0 ;
           *(MEM_ptr+i+1) = 0  ;
           *(MEM_ptr+i+2) = 255  ;
           *(MEM_ptr+i+3) = 23  ;
        }

#ifdef __mips__
	printf("==sync cache\n");
		pci_sync_cache(0, (vm_offset_t)MEM_ptr, MEM_SIZE, SYNC_W);
#endif


	
   }

   printf("frame buffer addr: %x \n",(MEM_ADDR+0x00));
  //write_reg(0x501030,0x23456789);
  //int *ptr = (int *)malloc(sizeof(int));
  //*ptr = (int *)0x501030;
  ////ptr = 0x501030;
  //*ptr = 0x23456789;
  //printf("addr:%x  data:%x",ptr,(*ptr));

   printf("frame buffer data: %x \n",readl((MEM_ADDR+0x00)));
  
  //config display controller panel 0 reg 
  write_reg((DC_BASE_ADDR+0x00),0x00000000);
  write_reg((DC_BASE_ADDR+0x00),0x00000004);
  write_reg((DC_BASE_ADDR+0x20),MEM_ADDR  );
  write_reg((DC_BASE_ADDR+0x40),0x00000500);
  write_reg((0xbc301360  +0x00),0x00000000);
  write_reg((0xbc301360  +0x20),0x00000000);
  write_reg((0xbc301360  +0x40),0x00000000);
  write_reg((0xbc301360  +0x60),0x80001111);
  write_reg((0xbc301360  +0x80),0x33333333);
  write_reg((0xbc301400  +0x00),0x014f0140);
  write_reg((0xbc301400  +0x20),0x414a0145);
  write_reg((0xbc301400  +0x80),0x00fa00f0);
  write_reg((0xbc301400  +0xa0),0x40f700f5);
  write_reg((0xbc301520  +0x00),0x00020202);
  write_reg((0xbc301530  +0x00),MEM_ADDR  );
  write_reg((0xbc301540  +0x00),0x00060122);
  write_reg((0xbc301550  +0x00),0x00eeeeee);
  write_reg((0xbc301560  +0x00),0x00aaaaaa);
 //config display controller panel 1 reg
  write_reg((DC_BASE_ADDR_1+0x00),0x00000000);
  write_reg((DC_BASE_ADDR_1+0x00),0x00000004);
  write_reg((DC_BASE_ADDR_1+0x20),MEM_ADDR  );
//800x600@60
//write_reg((DC_BASE_ADDR_1+0x40),0x00000C80);
  write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); //640
//  write_reg((DC_BASE_ADDR_1+0x40),0x00001558); //1366
  write_reg((0xbc301370  +0x00),0x00000000);
  write_reg((0xbc301370  +0x20),0x00000000);
  write_reg((0xbc301370  +0x40),0x00000000);
  write_reg((0xbc301370  +0x60),0x80001111);
  write_reg((0xbc301370  +0x80),0x33333333);

 //write_reg((0xbc301410  +0x00),0x038a0320);
 //write_reg((0xbc301410  +0x20),0x43610324);
 //write_reg((0xbc301410  +0x80),0x02940240);
 //write_reg((0xbc301410  +0xa0),0x426c0264);

//800x600@60
// write_reg((0xbc301410  +0x00),0x04000320);
// write_reg((0xbc301410  +0x20),0x43900340);
// write_reg((0xbc301410  +0x80),0x026E0258);
// write_reg((0xbc301410  +0xa0),0x425C0259);
/*704x598  
  write_reg((0xbc301410  +0x00),0x038002C0);
  write_reg((0xbc301410  +0x20),0x432002D8);
  write_reg((0xbc301410  +0x80),0x026B0256);
  write_reg((0xbc301410  +0xa0),0x425A0257);
*/
// 640x480@60 frequecy:23.86hz
// write_reg((0xbc301410  +0x00),0x03200280);
// write_reg((0xbc301410  +0x20),0x42D00290);
// write_reg((0xbc301410  +0x80),0x01F101E0);
// write_reg((0xbc301410  +0xa0),0x41E401E1);

// 640x480@60 frequecy:25.18hz
 write_reg((0xbc301410  +0x00),0x03200280);
 write_reg((0xbc301410  +0x20),0x42F00290);
 write_reg((0xbc301410  +0x80),0x020D01E0);
 write_reg((0xbc301410  +0xa0),0x41EC01EA);

//1366*768@20 23.78Hz
//write_reg((0xbc301410  +0x00),0x07080556);
//write_reg((0xbc301410  +0x20),0x463005A0);
//write_reg((0xbc301410  +0x80),0x031B0300);
//write_reg((0xbc301410  +0xa0),0x43040301);
  
// 640x480@60 frequecy:23.86hz  another set
//write_reg((0xbc301410  +0x00),0x03200280);
//write_reg((0xbc301410  +0x20),0x42F00290);
//write_reg((0xbc301410  +0x80),0x020D01E0);
//write_reg((0xbc301410  +0xa0),0x41EC01EA);

//640x480@50
 // write_reg((0xbc301410  +0x00),0x03100280);
 // write_reg((0xbc301410  +0x20),0x42C80288);
 // write_reg((0xbc301410  +0x80),0x01EF01E0);
 // write_reg((0xbc301410  +0xa0),0x41E401E1);
  
  write_reg((0xbc301520  +0x00),0x00020202);
  write_reg((0xbc301530  +0x00),MEM_ADDR  );
  write_reg((0xbc301540  +0x00),0x00020002);
  write_reg((0xbc301550  +0x00),0x00eeeeee);
  write_reg((0xbc301560  +0x00),0x00aaaaaa);


  write_reg((DC_BASE_ADDR+0x00),0x00100104);
  write_reg((DC_BASE_ADDR_1+0x00),0x00100104);
  printf("display controller reg config complete!\n");

  printf("read reg addr begin...\n");

  printf("read panel 0 reg:\n");
  print_data = readl((DC_BASE_ADDR+0x00));
  printf("read first data successfully!\n");
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x00),print_data);
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x00),(readl((DC_BASE_ADDR+0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x20),(readl((DC_BASE_ADDR+0x20))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x40),(readl((DC_BASE_ADDR+0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x00),(readl((0xbc301360  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x20),(readl((0xbc301360  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x40),(readl((0xbc301360  +0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x60),(readl((0xbc301360  +0x60))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x80),(readl((0xbc301360  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0x00),(readl((0xbc301400  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0x20),(readl((0xbc301400  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0x80),(readl((0xbc301400  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0xa0),(readl((0xbc301400  +0xa0))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301520  +0x00),(readl((0xbc301520  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301530  +0x00),(readl((0xbc301530  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301540  +0x00),(readl((0xbc301540  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301550  +0x00),(readl((0xbc301550  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301560  +0x00),(readl((0xbc301560  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x00),(readl((DC_BASE_ADDR+0x00))));

  printf("\n\nread panel 1 reg:\n");

  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x00),(readl((DC_BASE_ADDR_1+0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x20),(readl((DC_BASE_ADDR_1+0x20))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x40),(readl((DC_BASE_ADDR_1+0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x00),(readl((0xbc301370  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x20),(readl((0xbc301370  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x40),(readl((0xbc301370  +0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x60),(readl((0xbc301370  +0x60))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x80),(readl((0xbc301370  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0x00),(readl((0xbc301410  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0x20),(readl((0xbc301410  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0x80),(readl((0xbc301410  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0xa0),(readl((0xbc301410  +0xa0))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301520  +0x00),(readl((0xbc301520  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301530  +0x00),(readl((0xbc301530  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301540  +0x00),(readl((0xbc301540  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301550  +0x00),(readl((0xbc301550  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301560  +0x00),(readl((0xbc301560  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x00),(readl((DC_BASE_ADDR_1+0x00))));

  printf("read frame buffer begin...\n");
       for(j=0;j<DIS_HEIGHT;j+=100)
       {
  //          printf("line %d;\n",j);
            for (i=1;i<DIS_WIDTH*4;i+=4*10)
            {
              printf("%x %x %x %x \t",*(MEM_ptr+j*DIS_WIDTH*4+i-1),*(MEM_ptr+j*DIS_WIDTH*4+i),*(MEM_ptr+j*DIS_WIDTH*4+i+1),*(MEM_ptr+j*DIS_WIDTH*4+i+2)); 
            }
    //        printf("\n\n\n\n");
       }

  printf("\nGPU twodclearl test begin...\n");
  gpu_reg_wr();
  gpu_twodclearl(MEM_ptr);
  printf("write cmd buff end...\n");
  gpu_reg_wr();

  //wait
  for(j=0;j<500;j++)
  {
          printf("debug out reg:%x\n",readl(0xbc200004));
  }
  printf("wait end...\n");
  printf("read frame buffer begin...\n");
       for(j=0;j<DIS_HEIGHT;j+=100)
       {
  //          printf("line %d;\n",j);
            for (i=1;i<DIS_WIDTH*4;i+=4*10)
            {
              printf("%x %x %x %x \t",*(MEM_ptr+j*DIS_WIDTH*4+i-1),*(MEM_ptr+j*DIS_WIDTH*4+i),*(MEM_ptr+j*DIS_WIDTH*4+i+1),*(MEM_ptr+j*DIS_WIDTH*4+i+2)); 
            }
    //        printf("\n\n\n\n");
       }

  
#ifdef __mips__
	printf("==sync cache\n");
		pci_sync_cache(0, (vm_offset_t)MEM_ptr,MEM_SIZE, SYNC_W);
#endif

   return 0;
    
}

#endif


static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"gpu_clear", "", 0, "GPU clear SCREEN", gpu_twodclearl, 0, 99, CMD_REPEAT},
    {"gpu_dump_cmdbuf", "", 0, "DUMP GPU CMD BUF", dump_gpu_cmd_buf, 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}

