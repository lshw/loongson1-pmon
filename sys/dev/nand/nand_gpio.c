#define NAND_CMD_ID     0x90

#define NAND_GPIO_MUX  0x0a000000
#define GPIO_MUX_CTRL  0xbfd00420

#define GPIO_CONF3      0xbfd010c8
#define GPIO_IEN3        0xbfd010d8
#define GPIO_IDATA3      0xbfd010e8
#define GPIO_ODATA3      0xbfd010f8

#define NAND_D0         6  //70 - 64
#define NAND_D1         7  //71 - 64
#define NAND_D2         8  //72 - 64
#define NAND_D3         9  //73 - 64
#define NAND_D4         10 //74 - 64
#define NAND_D5         11 //75 - 64
#define NAND_CLE        12 //76 - 64
#define NAND_ALE        13 //77 - 64
#define NAND_RE         14 //78 - 64
#define NAND_WE         15 //79 - 64
#define NAND_CE         16 //80 - 64
#define NAND_RDY        17 //81 - 64
#define NAND_D6         18 //82 - 64 
#define NAND_D7         19 //83 - 64

#define _EN_PIN(x)      (*((volatile unsigned int*)(GPIO_CONF3)) = *((volatile unsigned int *)GPIO_CONF3) | (0x1 << (x)))          
#define _DIS_PIN(x)      (*((volatile unsigned int*)(GPIO_CONF3)) = *((volatile unsigned int *)GPIO_CONF3) & (~(0x1 << (x))))          
#define _EN_IN(x)      (*((volatile unsigned int*)(GPIO_IEN3)) = *((volatile unsigned int *)GPIO_IEN3) | (0x1 << (x)))          
#define _DIS_IN(x)      (*((volatile unsigned int*)(GPIO_IEN3)) = *((volatile unsigned int *)GPIO_IEN3) & (~(0x1 << (x))))          
#define _SET_ONE(x)      (*((volatile unsigned int*)(GPIO_ODATA3)) = *((volatile unsigned int *)GPIO_ODATA3) | (0x1 << (x)))          
#define _SET_ZERO(x)      (*((volatile unsigned int*)(GPIO_ODATA3)) = *((volatile unsigned int *)GPIO_ODATA3) & (~(0x1 << (x))))          
#define  _READ_GPIO3     (*(volatile unsigned int *)(GPIO_ODATA3))

#define _EN_OUT(x)      _DIS_IN(x)
#define _DIS_CTRL_IO            do{     \
            _DIS_PIN(NAND_CLE);         \
            _DIS_PIN(NAND_ALE);         \
            _DIS_PIN(NAND_RE);          \
            _DIS_PIN(NAND_WE);          \
            _DIS_PIN(NAND_CE);          \
            _DIS_PIN(NAND_RDY);         \
            }while(0)


#define _DIS_DATA_IO            do{     \
            _DIS_PIN(NAND_D0);           \
            _DIS_PIN(NAND_D1);           \
            _DIS_PIN(NAND_D2);           \
            _DIS_PIN(NAND_D3);           \
            _DIS_PIN(NAND_D4);           \
            _DIS_PIN(NAND_D5);           \
            _DIS_PIN(NAND_D6);           \
            _DIS_PIN(NAND_D7);           \
        }while(0)
#define DIS_GPIO_READ_ID    do{         \
            _DIS_CTRL_IO;               \
            _DIS_DATA_IO;               \
        }while(0)

#define _EN_DATA_IO             do{     \
            _EN_PIN(NAND_D0) ;           \
            _EN_PIN(NAND_D1) ;           \
            _EN_PIN(NAND_D2) ;           \
            _EN_PIN(NAND_D3) ;           \
            _EN_PIN(NAND_D4) ;           \
            _EN_PIN(NAND_D5) ;           \
            _EN_PIN(NAND_D6) ;           \
            _EN_PIN(NAND_D7) ;           \
        }while(0)

#define _EN_IN_DATA_IO          do{     \
            _EN_IN(NAND_D0) ;            \
            _EN_IN(NAND_D1) ;            \
            _EN_IN(NAND_D2) ;            \
            _EN_IN(NAND_D3) ;            \
            _EN_IN(NAND_D4) ;            \
            _EN_IN(NAND_D5) ;            \
            _EN_IN(NAND_D6) ;            \
            _EN_IN(NAND_D7) ;            \
        }while(0)
           
#define  _EN_OUT_DATA_IO            do{   \
            _EN_OUT(NAND_D0);            \
            _EN_OUT(NAND_D1);            \
            _EN_OUT(NAND_D2);            \
            _EN_OUT(NAND_D3);            \
            _EN_OUT(NAND_D4);            \
            _EN_OUT(NAND_D5);            \
            _EN_OUT(NAND_D6);            \
            _EN_OUT(NAND_D7);           \
        }while(0)    

#define DIS_DATA_IO    _DIS_DATA_IO

#define _DIS_GPIO3    *((volatile unsigned int *)GPIO_CONF3) = 0


#define _PUTODATA32(x)      *((volatile unsigned int *)GPIO_ODATA3) = (x)
#define _GETODATA32(x)      (x) = *((volatile unsigned int *)GPIO_ODATA3)  

#define _PUTIDATA32(x)      *((volatile unsigned int *)GPIO_IDATA3) = (x)
#define _GETIDATA32(x)      (x) = *((volatile unsigned int *)GPIO_IDATA3)  


#define SET_UP(x)    do{                \
                _EN_PIN((x));           \
                _EN_OUT((x));           \
                _SET_ONE((x));          \
                _READ_GPIO3;            \
            }while(0)

#define SET_DOWN(x)    do{                \
                _EN_PIN((x));           \
                _EN_OUT((x));           \
                _SET_ZERO((x));          \
                _READ_GPIO3;            \
            }while(0)
/*EN_SEND_DATA_IO*/
#define  EN_SEND_DATA_IO        do{      \
            _EN_DATA_IO     ;            \
            _EN_OUT_DATA_IO ;           \
            _READ_GPIO3;                \
        }while(0)
/*EN_GET_DATA_IO*/
#define  EN_GET_DATA_IO         do{     \
            _EN_DATA_IO;                 \
            _EN_IN_DATA_IO;              \
            _READ_GPIO3;               \
        }while(0)
 
static void nand_delay(unsigned int num)
{
    int u=280;
    while(num--){
        while(u--);
        u=280;
    }
}
static void nand_gpio_init(void)
{
    *((unsigned int *)GPIO_MUX_CTRL) = NAND_GPIO_MUX;
    *((unsigned int *)GPIO_MUX_CTRL) = NAND_GPIO_MUX;
}

static void gpio_senddata8(unsigned char val)
{
    unsigned int d32=0;
//    EN_SEND_DATA_IO;
    _GETODATA32(d32);
    d32 = (d32 & (~(0x3f << 6)) | ((val & 0x3f)<<6));
    d32 = (d32 & (~(0x3 << 18)) | (((val >> 6) & 0x3)<<18));
    SET_DOWN(NAND_WE);
    _PUTODATA32(d32);
    _READ_GPIO3;
    SET_UP(NAND_WE);
    //delay;

//   DIS_DATA_IO;     
}

static unsigned char gpio_getdata8(void)
{
    unsigned int d32=0;
    unsigned char val=0;
    //EN_GET_DATA_IO;
    //delay
    SET_DOWN(NAND_RE);
    _GETIDATA32(d32);
    SET_UP(NAND_RE);
 //   DIS_DATA_IO;
    val = (d32 >> 18) & 0x3;
    val = (val << 6) |(d32 >> 6) & 0x3f;
    return val;

}


static unsigned int nand_gpio_read_id(void)
{
    unsigned char val=0;
    nand_gpio_init();
    EN_SEND_DATA_IO;

    SET_DOWN(NAND_CE);

    /*send command*/
    SET_DOWN(NAND_ALE);
    SET_UP(NAND_RE);
    SET_UP(NAND_CLE);
    gpio_senddata8(NAND_CMD_ID);
    //delay
    SET_DOWN(NAND_CLE);

    /*send addr*/
    SET_UP(NAND_ALE);
    gpio_senddata8(0x00);
    SET_DOWN(NAND_ALE);
    //delay min 60ns
    /*read id*/
    EN_GET_DATA_IO;
    val = gpio_getdata8();
    printf("1th:%x\n",val);
    val = gpio_getdata8();
    printf("2th:%x\n",val);
    val = gpio_getdata8();
    printf("3th:%x\n",val);
    val = gpio_getdata8();
    printf("4th:%x\n",val);
    DIS_GPIO_READ_ID;
    return 0;
}

