#define I2C_START 0x80
#define I2C_STOP 0x40
#define I2C_READ 0x20
#define I2C_WRITE 0x10
#define I2C_WACK 0x8
#define I2C_IACK 0x1
#define I2C_RUN 0x2
#define I2C_BUSY 0x40
#define I2C_RACK 0x80
#define FCR_SOC_I2C_BASE  0xbf0040d0
#define FCR_SOC_I2C_PRER_LO (volatile unsigned char *)(FCR_SOC_I2C_BASE + 0x0)
#define FCR_SOC_I2C_PRER_HI (volatile unsigned char *)(FCR_SOC_I2C_BASE + 0x1)
#define FCR_SOC_I2C_CTR     (volatile unsigned char *)(FCR_SOC_I2C_BASE + 0x2)
#define FCR_SOC_I2C_TXR     (volatile unsigned char *)(FCR_SOC_I2C_BASE + 0x3)
#define FCR_SOC_I2C_RXR     (volatile unsigned char *)(FCR_SOC_I2C_BASE + 0x3)
#define FCR_SOC_I2C_CR      (volatile unsigned char *)(FCR_SOC_I2C_BASE + 0x4)
#define FCR_SOC_I2C_SR      (volatile unsigned char *)(FCR_SOC_I2C_BASE + 0x4)
void i2c_send(char ctrl,char addr);
char i2c_stat();
char i2c_recv();
