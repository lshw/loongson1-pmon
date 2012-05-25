/*****************************
    Macro defination for 2F DDR MC parameters
    Author: Chen Xinke
    v0.1    
*******************************/
#define DDR_MC_CONFIG_BASE      0xaffffe00

#define EIGHT_BANK_MODE_OFFSET  32
#define COLUMN_SIZE_OFFSET      24
#define ADDR_PINS_OFFSET        8
#define CS_MAP_OFFSET           16

#define START_ADDR              (0x30)
#define START_OFFSET            40
#define INT_STATUS_ADDR         (0x100)
#define INT_STATUS_OFFSET       48
#define INIT_COMPLETE_OFFSET    8

#ifndef  DLL_BYPASS_MODE

#define RDLVL_DELAY_MASK        (0xff)
#define WRLVL_DELAY_MASK        (0xff)
#define WRLVL_DQ_DELAY_MASK     (0xff)

#define WRLVL_DQ_DELAY_0_ADDR   (0xb4)
#define WRLVL_DQ_DELAY_0_OFFSET 16

#define WRLVL_DELAY_0_ADDR      (0xb4)
#define WRLVL_DELAY_0_OFFSET    24

#define RDLVL_DELAY_8_ADDR      (0xb4)
#define RDLVL_DELAY_7_ADDR      (0xb4)
#define RDLVL_DELAY_6_ADDR      (0xb0)
#define RDLVL_DELAY_5_ADDR      (0xb0)
#define RDLVL_DELAY_4_ADDR      (0xb0)
#define RDLVL_DELAY_3_ADDR      (0xb0)
#define RDLVL_DELAY_2_ADDR      (0xa4)
#define RDLVL_DELAY_1_ADDR      (0xa4)
#define RDLVL_DELAY_0_ADDR      (0xa4)
#define RDLVL_DELAY_8_OFFSET    8
#define RDLVL_DELAY_7_OFFSET    0
#define RDLVL_DELAY_6_OFFSET    24
#define RDLVL_DELAY_5_OFFSET    16
#define RDLVL_DELAY_4_OFFSET    8
#define RDLVL_DELAY_3_OFFSET    0
#define RDLVL_DELAY_2_OFFSET    24
#define RDLVL_DELAY_1_OFFSET    16
#define RDLVL_DELAY_0_OFFSET    8
#else

#define RDLVL_DELAY_MASK        (0x1ff)
#define WRLVL_DELAY_MASK        (0x1ff)
#define WRLVL_DQ_DELAY_MASK     (0x1ff)

#define WRLVL_DQ_DELAY_0_ADDR   (0x100)
#define WRLVL_DQ_DELAY_0_OFFSET 0
#define WRLVL_DELAY_0_ADDR      (0xf4)
#define WRLVL_DELAY_0_OFFSET    16

#define RDLVL_DELAY_8_ADDR      (0xe4)
#define RDLVL_DELAY_7_ADDR      (0xe4)
#define RDLVL_DELAY_6_ADDR      (0xe0)
#define RDLVL_DELAY_5_ADDR      (0xe0)
#define RDLVL_DELAY_4_ADDR      (0xd4)
#define RDLVL_DELAY_3_ADDR      (0xd4)
#define RDLVL_DELAY_2_ADDR      (0xd0)
#define RDLVL_DELAY_1_ADDR      (0xd0)
#define RDLVL_DELAY_0_ADDR      (0xc4)
#define RDLVL_DELAY_8_OFFSET    16
#define RDLVL_DELAY_7_OFFSET    0
#define RDLVL_DELAY_6_OFFSET    16
#define RDLVL_DELAY_5_OFFSET    0
#define RDLVL_DELAY_4_OFFSET    16
#define RDLVL_DELAY_3_OFFSET    0
#define RDLVL_DELAY_2_OFFSET    16
#define RDLVL_DELAY_1_OFFSET    0
#define RDLVL_DELAY_0_OFFSET    16
#endif
