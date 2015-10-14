/* camera test for loongson 1c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pmon.h>
#include <machine/pio.h>
#include <target/i2c-ls1x.h>

//#include <sys/malloc.h>

#include <target/gc0307.h>

#define CAMERA_BASE 0xbc280000
#define DMA0_CONFIG 0x0
#define DMA1_CONFIG 0x8
#define DMA2_CONFIG 0x10
#define DMA3_CONFIG 0x18
#define PIX_CONFIG 0x20
#define UOFF_CONFIG 0x28
#define VOFF_CONFIG 0x30
#define CAMIF_CONFIG 0x38

#define GC0307_ADDRESS 0x21

static unsigned int cam_xsize = 640;
static unsigned int cam_ysize = 480;

static char *cam_mem_ptr0 = NULL;
static char *cam_mem_ptr1 = NULL;
static char *cam_mem_ptr2 = NULL;
static char *cam_mem_ptr3 = NULL;

extern unsigned long fb_addr;	/* lcd显示的内存缓存地址，在/Targets/LS1X/dev/lcd.c中定义 */
unsigned long cam_mem_base0;
unsigned long cam_mem_base1;
unsigned long cam_mem_base2;
unsigned long cam_mem_base3;

static void gc0307_write(u8 addr, u8 val)
{
	struct i2c_client gc0307;
	u8 buf[2];

	gc0307.addr = GC0307_ADDRESS;

	buf[0] = addr & 0xff;
	buf[1] = val & 0xff;
	i2c_master_send(&gc0307, (const char *)buf, 2);
}

static u8 gc0307_read(u8 addr)
{
	struct i2c_client gc0307;
	u8 buf[2];

	gc0307.addr = GC0307_ADDRESS;

	buf[0] = addr & 0xff;
	i2c_master_send(&gc0307, (const char *)buf, 1);
	i2c_master_recv(&gc0307, (char *)buf, 1);
	
	return buf[0];
}

static int set_gc0307_config(void)
{
	int i;
	
	i = gc0307_read(0x00);
	printf("sensor_id = 0x%x\n", i);
	
	for (i = 0; i < GC0307_INIT_REGS; i++) {
		gc0307_write(gc0307_YCbCr8bit[i][0], gc0307_YCbCr8bit[i][1]);
	}

	gc0307_write(0x09, (cam_ysize >> 8) & 0xff);
	gc0307_write(0x0a, cam_ysize & 0xff);
	gc0307_write(0x0b, (cam_xsize >> 8) & 0xff);
	gc0307_write(0x0c, cam_xsize & 0xff);

	/* 指纹头背光控制 */
	pca953x_gpio_direction_output(0x20, 13);
	pca953x_gpio_set_value(0x20, 13, 1);

	return 0;
}

static int cameraif_init(void)
{
	unsigned int value;
	unsigned int cam_config;
	/* lcd显存地址，一般使用rgb565格式 16bit存放，所以使用unsigned short类型 */
	unsigned short *lcd_addr = (unsigned short *)fb_addr;
	unsigned short *mem_addr0 = (unsigned short *)cam_mem_ptr0;
	unsigned short *mem_addr1 = (unsigned short *)cam_mem_ptr1;
	unsigned short *mem_addr2 = (unsigned short *)cam_mem_ptr2;
	unsigned short *mem_addr3 = (unsigned short *)cam_mem_ptr3;
	unsigned int i, j;

	cam_mem_base0 = (unsigned long)cam_mem_ptr0;
	cam_mem_base1 = (unsigned long)cam_mem_ptr1;
	cam_mem_base2 = (unsigned long)cam_mem_ptr2;
	cam_mem_base3 = (unsigned long)cam_mem_ptr3;
	printf("cam_mem_base:%x\n", cam_mem_base0);

	writel(0, CAMERA_BASE + CAMIF_CONFIG);
	cam_config = readl(CAMERA_BASE + CAMIF_CONFIG);
	cam_config = cam_config & 0x40000000;
	if (cam_config == 0x40000000) {
		writel(cam_config | 0x3fffffff, CAMERA_BASE + CAMIF_CONFIG);
	}
	cam_config = readl(CAMERA_BASE + CAMIF_CONFIG);
	cam_config = cam_config & 0x40000000;
	if (cam_config == 0x0)
		printf("camera init is ready for use....\n");
	else {
		while (readl(CAMERA_BASE + CAMIF_CONFIG) != 0x40000000);
	}

	/* camera的4个帧缓存 */
	writel(cam_mem_base0 & 0x0fffffff, CAMERA_BASE + DMA0_CONFIG);
	writel(cam_mem_base1 & 0x0fffffff, CAMERA_BASE + DMA1_CONFIG);
	writel(cam_mem_base2 & 0x0fffffff, CAMERA_BASE + DMA2_CONFIG);
	writel(cam_mem_base3 & 0x0fffffff, CAMERA_BASE + DMA3_CONFIG);

	/* 摄像头一般使用yuv422输出格式，而ls1c的camera控制器可以把格式转换为rgb，方便在lcd上显示 */
//	value = 0x800f2000; /* input rgb -> output rgb */
//	gc0307_write(0x44, 0xe6);
	value = 0x800f2800; /* input yuv -> output rgb */
	gc0307_write(0x44, 0xe2);
	if ((cam_xsize == 640) && (cam_ysize == 480)) {
		value |= 0x80;
	}
	writel(value, CAMERA_BASE + CAMIF_CONFIG);
	printf("CAM config register 0x%x\n", value);

	/* 等待4帧图像，填充完毕 */
	while (1) {
		cam_config = readl(CAMERA_BASE + CAMIF_CONFIG) & 0x0f000000;
		if (cam_config == 0x03000000) {
			while (1) {
				cam_config = readl(CAMERA_BASE + CAMIF_CONFIG) & 0x0f000000;
				if (cam_config == 0x00000000) {
					break;
				}
			}
			break;
		}
	}
	writel(0, CAMERA_BASE + CAMIF_CONFIG);
	printf("Pics capture ok\n");

	/* 第一帧显示到lcd */
	for (i=0; i<cam_ysize; i++) {
		lcd_addr = (unsigned short *)(fb_addr + i* 800 * 2); /* 800*2表示lcd显示器的x像素为800  使用rgb565格式（16bit） */
		for (j=0; j<cam_xsize; j++) {
			*lcd_addr = *mem_addr0;
			lcd_addr += 1;
			mem_addr0 += 1;
		}
	}

	/* 第二帧显示到lcd */
	for (i=0; i<cam_ysize; i++) {
		lcd_addr = (unsigned short *)(fb_addr + i* 800 * 2);
		for (j=0; j<cam_xsize; j++) {
			*lcd_addr = *mem_addr1;
			lcd_addr += 1;
			mem_addr1 += 1;
		}
	}

	/* 第三帧显示到lcd */
	for (i=0; i<cam_ysize; i++) {
		lcd_addr = (unsigned short *)(fb_addr + i* 800 * 2);
		for (j=0; j<cam_xsize; j++) {
			*lcd_addr = *mem_addr2;
			lcd_addr += 1;
			mem_addr2 += 1;
		}
	}

	/* 第四帧显示到lcd */
	for (i=0; i<cam_ysize; i++) {
		lcd_addr = (unsigned short *)(fb_addr + i* 800 * 2);
		for (j=0; j<cam_xsize; j++) {
			*lcd_addr = *mem_addr3;
			lcd_addr += 1;
			mem_addr3 += 1;
		}
	}

	return 0;
}

/* 目前只适用于gc0307的指纹头，使用其他sensor的要另外初始化 */
static int camera_test(int argc, char **argv)
{
#if 0
	unsigned int cam_mem_len;

	cam_mem_len = PAGE_ALIGN(cam_xsize * cam_ysize * 4);
//	cam_mem_ptr = (char *)malloc(cam_mem_len, M_DEVBUF, M_WAITOK); /* 高位地在开始分配内存 */
	cam_mem_ptr = (char *)malloc(cam_mem_len);
	cam_mem_ptr = (unsigned int)ALIGN((unsigned int)cam_mem_ptr, 4);	/* 地址4字节对齐 */
	if (!cam_mem_ptr) {
		printf("Unable to allocate memory for cam.\n");
		return -ENOMEM;
	}
	memset(cam_mem_ptr, 0, cam_mem_len);
	cam_mem_ptr = (char *)((unsigned int)cam_mem_ptr | 0xa0000000); /* 需要转换为umap ucache的地址 */
#else
	/* camera控制器的图像采集缓存地址，这里使用一个unmap uncache的内存地址，
	      用于存放4帧图像 */
	cam_mem_ptr0 = (char *)0xa0400100;
	cam_mem_ptr1 = (char *)(0xa0400100 + 2 * 1024 * 1024);
	cam_mem_ptr2 = (char *)(0xa0400100 + 4 * 1024 * 1024);
	cam_mem_ptr3 = (char *)(0xa0400100 + 8 * 1024 * 1024);
#endif

	set_gc0307_config();
	cameraif_init();

//	free(cam_mem_ptr);

	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"cam_test", "[data]     [yuv] ", 0, "camera_test", camera_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}

