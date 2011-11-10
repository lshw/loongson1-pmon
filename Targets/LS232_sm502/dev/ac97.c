
/***************************************************
 *
 *
 ***************************************************
 */
#include <pmon.h>
#include <linux/types.h>
#define udelay delay
#define ac97_base  0xbf004200
#define dma_base  0xbf004280
#define ac97_reg_write(addr ,val) do{ *(volatile u32 *)(ac97_base+(addr))=val; }while(0)
#define ac97_reg_read(addr) *(volatile u32 * )(ac97_base+(addr))
//#define udelay(n)   
#define dma_reg_write(addr ,val) do{ *(volatile u32 *)(dma_base+(addr))=val; }while(0)
#define dma_reg_read(addr) *(volatile u32 * )(dma_base+(addr))

#define codec_wait(n) do{ int __i=n;\
        while (__i-->0){ \
            if (ac97_reg_read(0x5c)&0x3!=0) break;\
            udelay(100); }\
            if (__i>0){ \
                ac97_reg_read(0x6c);\
                ac97_reg_read(0x70);\
            }\
        }while (0)
        
#define DMA_BUF 0x00300000
#define  BUF_SIZE 0x200000
//#define  BUF_SIZE 0x7000

#define CPU2FIFO 1

#define AC97_RECORD 0
#define AC97_PLAY   1 

#define REC_DMA_BUF   (DMA_BUF+ BUF_SIZE)//0x00500000
#define  REC_BUF_SIZE  (BUF_SIZE>>1)

static unsigned short sample_rate=0xac44;

static int ac97_rw=0;

 void  init_audio_data(void )
 {
 #if 0
   volatile unsigned int *data= (volatile unsigned int*)(DMA_BUF|0x80000000);
   
   int i;
   
   for (i=0;i<((BUF_SIZE)>>3);i++)
   {     
        data[i<<1]=0x7fff0000;
        data[i<<1+1]=0x7fff;
   }
 #endif 
 }
 
int ac97_config()
 {
    printf("ac97 config enter\n");
    /*
    ac97_reg_write(0x58,0x0000036f);
    ac97_reg_write(0x4,0x6b6b6b6b);
    ac97_reg_write(0x10,0x006b6b6b);
    */
    
    ac97_reg_write(0x4,0x6969|0x202); //OCCR0   L&& R enable ; 3/4 empty; dma enabled;8 bits;var rate(0x202);
    //ac97_reg_write(0x4,0x6565); //OCCR0   L&& R enable ; 3/4 empty; dma enabled;16 bits;fix rate;
    
    ac97_reg_write(0x10,0x690000|0x20000);//ICCR
    
    ac97_reg_write(0x58,0x3); //INTM
    
    //codec
    ac97_reg_write(0x18,0|(0<<16)|(0<<31)); //codec reset
    
    codec_wait(10);

    
    ac97_reg_write(0x18,0|(0x7c<<16)|(1<<31));
    
    codec_wait(10);

    printf("codec id %x \n",ac97_reg_read(0x18)&0xffff); //read ID 
    
    
    ac97_reg_write(0x18,0x0808|(0x2<<16)|(0<<31));      //Master Vol.
    
    codec_wait(10);
    ac97_reg_write(0x18,0x0808|(0x4<<16)|(0<<31));      //headphone Vol.
    
    codec_wait(10);

    ac97_reg_write(0x18,0x0008|(0x6<<16)|(0<<31));      //headphone Vol.
    
    codec_wait(10);

    ac97_reg_write(0x18,0x0008|(0xc<<16)|(0<<31));      //phone Vol.
    
    codec_wait(10);
     
    ac97_reg_write(0x18,0x0808|(0x18<<16)|(0<<31));     //PCM Out Vol.
    
    codec_wait(10);
    
    ac97_reg_write(0x18,0x1|(0x2A<<16)|(0<<31));        //Extended Audio Status  and control
    
    codec_wait(10);
    
    ac97_reg_write(0x18,sample_rate|(0x2c<<16)|(0<<31));     //PCM Out Vol. FIXME:22k can play 44k wav data?
    
    codec_wait(10);


    if (AC97_RECORD==ac97_rw) //play
    {
            ac97_reg_write(0x18,sample_rate|(0x32<<16)|(0<<31));     //ADC rate .
    
            codec_wait(10);

            ac97_reg_write(0x18,0x035f|(0x0E<<16)|(0<<31));     //Mic vol .
    
            codec_wait(10);

            ac97_reg_write(0x18,0x0f0f|(0x1E<<16)|(0<<31));     //MIC Gain ADC.
    
            codec_wait(10);

            ac97_reg_write(0x18,sample_rate|(0x34<<16)|(0<<31));     //MIC rate.
    
            codec_wait(10);
        //0x1a  sele
    }

   printf("exit\n"); 
   return 0;
 }

void dma_config(void)
{
    if (AC97_PLAY==ac97_rw) //play
        dma_reg_write(0x20,0x10006); // 16bits X 6 entry 
     else       //record
        dma_reg_write(0x24,0x4); //  6 entry 
}


 void dma_setup_trans(u32 * src_addr,u32 size)
 {
    
    if (AC97_PLAY==ac97_rw) //play
    {
            dma_reg_write(0x0,src_addr);
            dma_reg_write(0x4,size>>2);
    }
    else //record
    {
        dma_reg_write(0x10,src_addr);
        dma_reg_write(0x14,size>>2);
    }
 }

 int ac97_test(int argc,char **argv)
 {
    
	char cmdbuf[100];
    int i;
	if(argc!=2 && argc!=1)return -1;
	if(argc==2){
	sprintf(cmdbuf,"load -o 0x%x -r %s",DMA_BUF|0xa0000000,argv[1]);
	do_cmd(cmdbuf);
	}
   
    ac97_rw=AC97_PLAY;
   
    init_audio_data();
    ac97_config();
    dma_config();
    
    printf("%d\n",dma_reg_read(0x8));
    printf("%d\n",dma_reg_read(0xc));
   
    dma_setup_trans(DMA_BUF,BUF_SIZE);
 
        
    printf("%d\n",dma_reg_read(0x8));
    printf("%d\n",dma_reg_read(0xc));

    printf("play data on 0x%x,sz=0x%x\n",(DMA_BUF|0xa0000000),BUF_SIZE);

        //1.wait a trans complete
        while(((dma_reg_read(0x2c))&0x1)==0)
		idle();
        
        //2.clear int status  bit.
        dma_reg_write(0x2c,0x1);

return 0;
 }

int ac97_read(int argc,char **argv)
{
    int i,l;
    unsigned int j;
    unsigned int k;
    unsigned int m;

    volatile unsigned short * rec_buff;
    volatile unsigned int  *  ply_buff;
    ac97_rw=AC97_RECORD;
    
    /*1.dma_config read*/
   
     dma_config();
    
    /*2.ac97 config read*/
    
     ac97_config();

    /*3.set dma desc*/
    printf("wait 5s ");
    for(i=0;i<5;i++)
    {
        udelay(1000000);
        printf("."); 
    }
    printf("\n");
    
    dma_setup_trans(REC_DMA_BUF,REC_BUF_SIZE);
    
    /*4.wait dma done,return*/ 
    while(((dma_reg_read(0x2c))&0x8)==0)
         udelay(1000);//1 ms 

    dma_reg_write(0x2c,0x8);
    /*5.transform single channel to double channel*/
    l = 0;
    printf("record complete\n");
    
   // printf("%d\n",sizeof(short));
   
    rec_buff=(volatile unsigned short * )(REC_DMA_BUF|0xa0000000);
    ply_buff=(volatile unsigned int * )(DMA_BUF|0xa0000000);
    
    
    for(i=0;i<(REC_BUF_SIZE<<1);i++)
    {
        j = 0x0000ffff & (unsigned int) rec_buff[i];
        
        //ply_buff[i]=(rec_buff[i]<<16)|(rec_buff[i]);
        ply_buff[i]=(j<<16)|(j);
    }
    
    /*
    for(i=0;i<REC_BUF_SIZE;i++)
    {
         j = *((int *)((i<<2)+(0xa0000000|REC_DMA_BUF)));
         
         k = j<<16;
         m = j>>16;
         
         *((volatile unsigned int *)((l<<2)+(0xa0000000|DMA_BUF))) = (k & 0xffff0000)|(j & 0xffff);
         *((volatile unsigned int *)((l<<2)+(0xa0000004|DMA_BUF))) = (j & 0xffff0000)|(m & 0xffff);
         l = l+2;
     }
     */

return 0;
}



static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"ac97_test","file",0,"ac97_test file",ac97_test,0,99,CMD_REPEAT},
	{"ac97_read","",0,"ac97_read",ac97_read,0,99,CMD_REPEAT},
	{"ac97_config","",0,"ac97_config",ac97_config,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

