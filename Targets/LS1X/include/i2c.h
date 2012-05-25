#define I2C_START 0x80
#define I2C_STOP 0x40
#define I2C_READ 0x20
#define I2C_WRITE 0x10
#define I2C_WACK 0x8
#define I2C_IACK 0x1
#define I2C_RUN 0x2
#define I2C_BUSY 0x40
#define I2C_RACK 0x80
#define GS_SOC_I2C_BASE    0xbfe58000
#define GS_SOC_I2C_PRER_LO (volatile unsigned char *)(GS_SOC_I2C_BASE + 0x0)
#define GS_SOC_I2C_PRER_HI (volatile unsigned char *)(GS_SOC_I2C_BASE + 0x1)
#define GS_SOC_I2C_CTR     (volatile unsigned char *)(GS_SOC_I2C_BASE + 0x2)
#define GS_SOC_I2C_TXR     (volatile unsigned char *)(GS_SOC_I2C_BASE + 0x3)
#define GS_SOC_I2C_RXR     (volatile unsigned char *)(GS_SOC_I2C_BASE + 0x3)
#define GS_SOC_I2C_CR      (volatile unsigned char *)(GS_SOC_I2C_BASE + 0x4)
#define GS_SOC_I2C_SR      (volatile unsigned char *)(GS_SOC_I2C_BASE + 0x4)


#define SR_NOACK 0x80
#define CR_START 0x80
#define CR_WRITE 0x10
#define CR_READ  0x20
#define SR_BUSY  0x40 
#define SR_TIP   0x2
#define CR_STOP  0x40
#define LS232_I2C_BASE  0xbf0040d0
#define LS232_I2C_PRER_LO ((volatile unsigned char *)(LS232_I2C_BASE + 0x0))
#define LS232_I2C_PRER_HI ((volatile unsigned char *)(LS232_I2C_BASE + 0x1))
#define LS232_I2C_CTR     ((volatile unsigned char *)(LS232_I2C_BASE + 0x2))
#define LS232_I2C_TXR     ((volatile unsigned char *)(LS232_I2C_BASE + 0x3))
#define LS232_I2C_RXR     ((volatile unsigned char *)(LS232_I2C_BASE + 0x3))
#define LS232_I2C_CR      ((volatile unsigned char *)(LS232_I2C_BASE + 0x4))
#define LS232_I2C_SR      ((volatile unsigned char *)(LS232_I2C_BASE + 0x4))
void i2c_send(char ctrl,char addr);
char i2c_stat();
char i2c_recv();
