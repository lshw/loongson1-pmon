/*
 *  linux/drivers/video/sm501hw.h -- Silicon Motion SM501 frame buffer device
 *
 *      Copyright (C) 2006 Silicon Motion, Inc.
 *      Ge Wang, gewang@siliconmotion.com
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */


#define SM501_VIDEOMEMORYSIZE    0x00800000  /*Assume SMTC graphics chip has 8MB VRAM */
#define SM502_REV_ID             0xC0

/*
 *
 * Definitions for the System Configuration registers.
 *
 */

#define SYSTEM_CTRL                                     0x000000
#define SYSTEM_CTRL_DPMS                                31:30
#define SYSTEM_CTRL_DPMS_VPHP                           0
#define SYSTEM_CTRL_DPMS_VPHN                           1
#define SYSTEM_CTRL_DPMS_VNHP                           2
#define SYSTEM_CTRL_DPMS_VNHN                           3
#define SYSTEM_CTRL_PCI_BURST                           29:29
#define SYSTEM_CTRL_PCI_BURST_DISABLE                   0
#define SYSTEM_CTRL_PCI_BURST_ENABLE                    1
#define SYSTEM_CTRL_CSC_STATUS                          28:28
#define SYSTEM_CTRL_CSC_STATUS_IDLE                     0
#define SYSTEM_CTRL_CSC_STATUS_BUSY                     1
#define SYSTEM_CTRL_PCI_MASTER                          25:25
#define SYSTEM_CTRL_PCI_MASTER_STOP                     0
#define SYSTEM_CTRL_PCI_MASTER_START                    1
#define SYSTEM_CTRL_LATENCY_TIMER                       24:24
#define SYSTEM_CTRL_LATENCY_TIMER_ENABLE                0
#define SYSTEM_CTRL_LATENCY_TIMER_DISABLE               1
#define SYSTEM_CTRL_PANEL_STATUS                        23:23
#define SYSTEM_CTRL_PANEL_STATUS_CURRENT                0
#define SYSTEM_CTRL_PANEL_STATUS_PENDING                1
#define SYSTEM_CTRL_VIDEO_STATUS                        22:22
#define SYSTEM_CTRL_VIDEO_STATUS_CURRENT                0
#define SYSTEM_CTRL_VIDEO_STATUS_PENDING                1
#define SYSTEM_CTRL_DE_FIFO                             20:20
#define SYSTEM_CTRL_DE_FIFO_NOT_EMPTY                   0
#define SYSTEM_CTRL_DE_FIFO_EMPTY                       1
#define SYSTEM_CTRL_DE_STATUS                           19:19
#define SYSTEM_CTRL_DE_STATUS_IDLE                      0
#define SYSTEM_CTRL_DE_STATUS_BUSY                      1
#define SYSTEM_CTRL_CRT_STATUS                          17:17
#define SYSTEM_CTRL_CRT_STATUS_CURRENT                  0
#define SYSTEM_CTRL_CRT_STATUS_PENDING                  1
#define SYSTEM_CTRL_ZVPORT                              16:16
#define SYSTEM_CTRL_ZVPORT_0                            0
#define SYSTEM_CTRL_ZVPORT_1                            1
#define SYSTEM_CTRL_PCI_BURST_READ                      15:15
#define SYSTEM_CTRL_PCI_BURST_READ_DISABLE              0
#define SYSTEM_CTRL_PCI_BURST_READ_ENABLE               1
#define SYSTEM_CTRL_DE_ABORT                            13:12
#define SYSTEM_CTRL_DE_ABORT_NORMAL                     0
#define SYSTEM_CTRL_DE_ABORT_2D_ABORT                   3
#define SYSTEM_CTRL_PCI_SUBSYS_LOCK                     11:11
#define SYSTEM_CTRL_PCI_SUBSYS_LOCK_DISABLE             0
#define SYSTEM_CTRL_PCI_SUBSYS_LOCK_ENABLE              1
#define SYSTEM_CTRL_PCI_RETRY                           7:7
#define SYSTEM_CTRL_PCI_RETRY_ENABLE                    0
#define SYSTEM_CTRL_PCI_RETRY_DISABLE                   1
#define SYSTEM_CTRL_PCI_CLOCK_RUN                       6:6
#define SYSTEM_CTRL_PCI_CLOCK_RUN_DISABLE               0
#define SYSTEM_CTRL_PCI_CLOCK_RUN_ENABLE                1
#define SYSTEM_CTRL_PCI_SLAVE_BURST_READ_SIZE           5:4
#define SYSTEM_CTRL_PCI_SLAVE_BURST_READ_SIZE_1         0
#define SYSTEM_CTRL_PCI_SLAVE_BURST_READ_SIZE_2         1
#define SYSTEM_CTRL_PCI_SLAVE_BURST_READ_SIZE_4         2
#define SYSTEM_CTRL_PCI_SLAVE_BURST_READ_SIZE_8         3
#define SYSTEM_CTRL_CRT_TRISTATE                        2:2
#define SYSTEM_CTRL_CRT_TRISTATE_DISABLE                0
#define SYSTEM_CTRL_CRT_TRISTATE_ENABLE                 1
#define SYSTEM_CTRL_INTMEM_TRISTATE                     1:1
#define SYSTEM_CTRL_INTMEM_TRISTATE_DISABLE             0
#define SYSTEM_CTRL_INTMEM_TRISTATE_ENABLE              1
#define SYSTEM_CTRL_PANEL_TRISTATE                      0:0
#define SYSTEM_CTRL_PANEL_TRISTATE_DISABLE              0
#define SYSTEM_CTRL_PANEL_TRISTATE_ENABLE               1

#define MISC_CTRL                                       0x000004
#define MISC_CTRL_PCI_PAD                               31:30
#define MISC_CTRL_PCI_PAD_24MA                          0
#define MISC_CTRL_PCI_PAD_12MA                          1
#define MISC_CTRL_PCI_PAD_8MA                           2
#define MISC_CTRL_48_SELECT                             29:28
#define MISC_CTRL_48_SELECT_CRYSTAL                     0
#define MISC_CTRL_48_SELECT_CPU_96                      2
#define MISC_CTRL_48_SELECT_CPU_48                      3
#define MISC_CTRL_UART1_SELECT                          27:27
#define MISC_CTRL_UART1_SELECT_UART                     0
#define MISC_CTRL_UART1_SELECT_SSP                      1
#define MISC_CTRL_8051_LATCH                            26:26
#define MISC_CTRL_8051_LATCH_DISABLE                    0
#define MISC_CTRL_8051_LATCH_ENABLE                     1
#define MISC_CTRL_FPDATA                                25:25
#define MISC_CTRL_FPDATA_18                             0
#define MISC_CTRL_FPDATA_24                             1
#define MISC_CTRL_CRYSTAL                               24:24
#define MISC_CTRL_CRYSTAL_24                            0
#define MISC_CTRL_CRYSTAL_12                            1
#define MISC_CTRL_DRAM_REFRESH                          22:21
#define MISC_CTRL_DRAM_REFRESH_8                        0
#define MISC_CTRL_DRAM_REFRESH_16                       1
#define MISC_CTRL_DRAM_REFRESH_32                       2
#define MISC_CTRL_DRAM_REFRESH_64                       3
#define MISC_CTRL_BUS_HOLD                              20:18
#define MISC_CTRL_BUS_HOLD_FIFO_EMPTY                   0
#define MISC_CTRL_BUS_HOLD_8                            1
#define MISC_CTRL_BUS_HOLD_16                           2
#define MISC_CTRL_BUS_HOLD_24                           3
#define MISC_CTRL_BUS_HOLD_32                           4
#define MISC_CTRL_HITACHI_READY                         17:17
#define MISC_CTRL_HITACHI_READY_NEGATIVE                0
#define MISC_CTRL_HITACHI_READY_POSITIVE                1
#define MISC_CTRL_INTERRUPT                             16:16
#define MISC_CTRL_INTERRUPT_NORMAL                      0
#define MISC_CTRL_INTERRUPT_INVERT                      1
#define MISC_CTRL_PLL_CLOCK_COUNT                       15:15
#define MISC_CTRL_PLL_CLOCK_COUNT_DISABLE               0
#define MISC_CTRL_PLL_CLOCK_COUNT_ENABLE                1
#define MISC_CTRL_DAC_BAND_GAP                          14:13
#define MISC_CTRL_DAC_POWER                             12:12
#define MISC_CTRL_DAC_POWER_ENABLE                      0
#define MISC_CTRL_DAC_POWER_DISABLE                     1
#define MISC_CTRL_USB_SLAVE_CONTROLLER                  11:11
#define MISC_CTRL_USB_SLAVE_CONTROLLER_CPU              0
#define MISC_CTRL_USB_SLAVE_CONTROLLER_8051             1
#define MISC_CTRL_BURST_LENGTH                          10:10
#define MISC_CTRL_BURST_LENGTH_8                        0
#define MISC_CTRL_BURST_LENGTH_1                        1
#define MISC_CTRL_USB_SELECT                            9:9
#define MISC_CTRL_USB_SELECT_MASTER                     0
#define MISC_CTRL_USB_SELECT_SLAVE                      1
#define MISC_CTRL_LOOPBACK                              8:8
#define MISC_CTRL_LOOPBACK_NORMAL                       0
#define MISC_CTRL_LOOPBACK_USB_HOST                     1
#define MISC_CTRL_CLOCK_DIVIDER_RESET                   7:7
#define MISC_CTRL_CLOCK_DIVIDER_RESET_ENABLE            0
#define MISC_CTRL_CLOCK_DIVIDER_RESET_DISABLE           1
#define MISC_CTRL_TEST_MODE                             6:5
#define MISC_CTRL_TEST_MODE_NORMAL                      0
#define MISC_CTRL_TEST_MODE_DEBUGGING                   1
#define MISC_CTRL_TEST_MODE_NAND                        2
#define MISC_CTRL_TEST_MODE_MEMORY                      3
#define MISC_CTRL_NEC_MMIO                              4:4
#define MISC_CTRL_NEC_MMIO_30                           0
#define MISC_CTRL_NEC_MMIO_62                           1
#define MISC_CTRL_CLOCK                                 3:3
#define MISC_CTRL_CLOCK_PLL                             0
#define MISC_CTRL_CLOCK_TEST                            1
#define MISC_CTRL_HOST_BUS                              2:0
#define MISC_CTRL_HOST_BUS_HITACHI                      0
#define MISC_CTRL_HOST_BUS_PCI                          1
#define MISC_CTRL_HOST_BUS_XSCALE                       2
#define MISC_CTRL_HOST_BUS_STRONGARM                    4
#define MISC_CTRL_HOST_BUS_NEC                          6

#define GPIO_MUX_LOW                                    0x000008
#define GPIO_MUX_LOW_31                                 31:31
#define GPIO_MUX_LOW_31_GPIO                            0
#define GPIO_MUX_LOW_31_PWM                             1
#define GPIO_MUX_LOW_30                                 30:30
#define GPIO_MUX_LOW_30_GPIO                            0
#define GPIO_MUX_LOW_30_PWM                             1
#define GPIO_MUX_LOW_29                                 29:29
#define GPIO_MUX_LOW_29_GPIO                            0
#define GPIO_MUX_LOW_29_PWM                             1
#define GPIO_MUX_LOW_28                                 28:28
#define GPIO_MUX_LOW_28_GPIO                            0
#define GPIO_MUX_LOW_28_AC97_I2S                        1
#define GPIO_MUX_LOW_27                                 27:27
#define GPIO_MUX_LOW_27_GPIO                            0
#define GPIO_MUX_LOW_27_AC97_I2S                        1
#define GPIO_MUX_LOW_26                                 26:26
#define GPIO_MUX_LOW_26_GPIO                            0
#define GPIO_MUX_LOW_26_AC97_I2S                        1
#define GPIO_MUX_LOW_25                                 25:25
#define GPIO_MUX_LOW_25_GPIO                            0
#define GPIO_MUX_LOW_25_AC97_I2S                        1
#define GPIO_MUX_LOW_24                                 24:24
#define GPIO_MUX_LOW_24_GPIO                            0
#define GPIO_MUX_LOW_24_AC97                            1
#define GPIO_MUX_LOW_23                                 23:23
#define GPIO_MUX_LOW_23_GPIO                            0
#define GPIO_MUX_LOW_23_ZVPORT                          1
#define GPIO_MUX_LOW_22                                 22:22
#define GPIO_MUX_LOW_22_GPIO                            0
#define GPIO_MUX_LOW_22_ZVPORT                          1
#define GPIO_MUX_LOW_21                                 21:21
#define GPIO_MUX_LOW_21_GPIO                            0
#define GPIO_MUX_LOW_21_ZVPORT                          1
#define GPIO_MUX_LOW_20                                 20:20
#define GPIO_MUX_LOW_20_GPIO                            0
#define GPIO_MUX_LOW_20_ZVPORT                          1
#define GPIO_MUX_LOW_19                                 19:19
#define GPIO_MUX_LOW_19_GPIO                            0
#define GPIO_MUX_LOW_19_ZVPORT                          1
#define GPIO_MUX_LOW_18                                 18:18
#define GPIO_MUX_LOW_18_GPIO                            0
#define GPIO_MUX_LOW_18_ZVPORT                          1
#define GPIO_MUX_LOW_17                                 17:17
#define GPIO_MUX_LOW_17_GPIO                            0
#define GPIO_MUX_LOW_17_ZVPORT                          1
#define GPIO_MUX_LOW_16                                 16:16
#define GPIO_MUX_LOW_16_GPIO                            0
#define GPIO_MUX_LOW_16_ZVPORT                          1
#define GPIO_MUX_LOW_15                                 15:15
#define GPIO_MUX_LOW_15_GPIO                            0
#define GPIO_MUX_LOW_15_8051                            1
#define GPIO_MUX_LOW_14                                 14:14
#define GPIO_MUX_LOW_14_GPIO                            0
#define GPIO_MUX_LOW_14_8051                            1
#define GPIO_MUX_LOW_13                                 13:13
#define GPIO_MUX_LOW_13_GPIO                            0
#define GPIO_MUX_LOW_13_8051                            1
#define GPIO_MUX_LOW_12                                 12:12
#define GPIO_MUX_LOW_12_GPIO                            0
#define GPIO_MUX_LOW_12_8051                            1
#define GPIO_MUX_LOW_11                                 11:11
#define GPIO_MUX_LOW_11_GPIO                            0
#define GPIO_MUX_LOW_11_8051                            1
#define GPIO_MUX_LOW_10                                 10:10
#define GPIO_MUX_LOW_10_GPIO                            0
#define GPIO_MUX_LOW_10_8051                            1
#define GPIO_MUX_LOW_9                                  9:9
#define GPIO_MUX_LOW_9_GPIO                             0
#define GPIO_MUX_LOW_9_8051                             1
#define GPIO_MUX_LOW_8                                  8:8
#define GPIO_MUX_LOW_8_GPIO                             0
#define GPIO_MUX_LOW_8_8051                             1
#define GPIO_MUX_LOW_7                                  7:7
#define GPIO_MUX_LOW_7_GPIO                             0
#define GPIO_MUX_LOW_7_8051                             1
#define GPIO_MUX_LOW_6                                  6:6
#define GPIO_MUX_LOW_6_GPIO                             0
#define GPIO_MUX_LOW_6_8051                             1
#define GPIO_MUX_LOW_5                                  5:5
#define GPIO_MUX_LOW_5_GPIO                             0
#define GPIO_MUX_LOW_5_8051                             1
#define GPIO_MUX_LOW_4                                  4:4
#define GPIO_MUX_LOW_4_GPIO                             0
#define GPIO_MUX_LOW_4_8051                             1
#define GPIO_MUX_LOW_3                                  3:3
#define GPIO_MUX_LOW_3_GPIO                             0
#define GPIO_MUX_LOW_3_8051                             1
#define GPIO_MUX_LOW_2                                  2:2
#define GPIO_MUX_LOW_2_GPIO                             0
#define GPIO_MUX_LOW_2_8051                             1
#define GPIO_MUX_LOW_1                                  1:1
#define GPIO_MUX_LOW_1_GPIO                             0
#define GPIO_MUX_LOW_1_8051                             1
#define GPIO_MUX_LOW_0                                  0:0
#define GPIO_MUX_LOW_0_GPIO                             0
#define GPIO_MUX_LOW_0_8051                             1

#define GPIO_MUX_HIGH                                   0x00000C
#define GPIO_MUX_HIGH_63                                31:31
#define GPIO_MUX_HIGH_63_GPIO                           0
#define GPIO_MUX_HIGH_63_CRT_ZVPORT_FPDATA              1
#define GPIO_MUX_HIGH_62                                30:30
#define GPIO_MUX_HIGH_62_GPIO                           0
#define GPIO_MUX_HIGH_62_CRT_ZVPORT_FPDATA              1
#define GPIO_MUX_HIGH_61                                29:29
#define GPIO_MUX_HIGH_61_GPIO                           0
#define GPIO_MUX_HIGH_61_CRT_ZVPORT_FPDATA              1
#define GPIO_MUX_HIGH_60                                28:28
#define GPIO_MUX_HIGH_60_GPIO                           0
#define GPIO_MUX_HIGH_60_CRT_ZVPORT_FPDATA              1
#define GPIO_MUX_HIGH_59                                27:27
#define GPIO_MUX_HIGH_59_GPIO                           0
#define GPIO_MUX_HIGH_59_CRT_ZVPORT_FPDATA              1
#define GPIO_MUX_HIGH_58                                26:26
#define GPIO_MUX_HIGH_58_GPIO                           0
#define GPIO_MUX_HIGH_58_CRT_ZVPORT_FPDATA              1
#define GPIO_MUX_HIGH_57                                25:25
#define GPIO_MUX_HIGH_57_GPIO                           0
#define GPIO_MUX_HIGH_57_CRT_ZVPORT                     1
#define GPIO_MUX_HIGH_56                                24:24
#define GPIO_MUX_HIGH_56_GPIO                           0
#define GPIO_MUX_HIGH_56_CRT_ZVPORT                     1
#define GPIO_MUX_HIGH_55                                23:23
#define GPIO_MUX_HIGH_55_GPIO                           0
#define GPIO_MUX_HIGH_55_CRT                            1
#define GPIO_MUX_HIGH_47                                15:15
#define GPIO_MUX_HIGH_47_GPIO                           0
#define GPIO_MUX_HIGH_47_I2C                            1
#define GPIO_MUX_HIGH_46                                14:14
#define GPIO_MUX_HIGH_46_GPIO                           0
#define GPIO_MUX_HIGH_46_I2C                            1
#define GPIO_MUX_HIGH_45                                13:13
#define GPIO_MUX_HIGH_45_GPIO                           0
#define GPIO_MUX_HIGH_45_SSP1                           1
#define GPIO_MUX_HIGH_44                                12:12
#define GPIO_MUX_HIGH_44_GPIO                           0
#define GPIO_MUX_HIGH_44_UART1_SSP1                     1
#define GPIO_MUX_HIGH_43                                11:11
#define GPIO_MUX_HIGH_43_GPIO                           0
#define GPIO_MUX_HIGH_43_UART1_SSP1                     1
#define GPIO_MUX_HIGH_42                                10:10
#define GPIO_MUX_HIGH_42_GPIO                           0
#define GPIO_MUX_HIGH_42_UART1_SSP1                     1
#define GPIO_MUX_HIGH_41                                9:9
#define GPIO_MUX_HIGH_41_GPIO                           0
#define GPIO_MUX_HIGH_41_UART1_SSP1                     1
#define GPIO_MUX_HIGH_40                                8:8
#define GPIO_MUX_HIGH_40_GPIO                           0
#define GPIO_MUX_HIGH_40_UART0                          1
#define GPIO_MUX_HIGH_39                                7:7
#define GPIO_MUX_HIGH_39_GPIO                           0
#define GPIO_MUX_HIGH_39_UART0                          1
#define GPIO_MUX_HIGH_38                                6:6
#define GPIO_MUX_HIGH_38_GPIO                           0
#define GPIO_MUX_HIGH_38_UART0                          1
#define GPIO_MUX_HIGH_37                                5:5
#define GPIO_MUX_HIGH_37_GPIO                           0
#define GPIO_MUX_HIGH_37_UART0                          1
#define GPIO_MUX_HIGH_36                                4:4
#define GPIO_MUX_HIGH_36_GPIO                           0
#define GPIO_MUX_HIGH_36_SSP0                           1
#define GPIO_MUX_HIGH_35                                3:3
#define GPIO_MUX_HIGH_35_GPIO                           0
#define GPIO_MUX_HIGH_35_SSP0                           1
#define GPIO_MUX_HIGH_34                                2:2
#define GPIO_MUX_HIGH_34_GPIO                           0
#define GPIO_MUX_HIGH_34_SSP0                           1
#define GPIO_MUX_HIGH_33                                1:1
#define GPIO_MUX_HIGH_33_GPIO                           0
#define GPIO_MUX_HIGH_33_SSP0                           1
#define GPIO_MUX_HIGH_32                                0:0
#define GPIO_MUX_HIGH_32_GPIO                           0
#define GPIO_MUX_HIGH_32_SSP0                           1

#define DRAM_CTRL                                       0x000010
#define DRAM_CTRL_EMBEDDED                              31:31
#define DRAM_CTRL_EMBEDDED_ENABLE                       0
#define DRAM_CTRL_EMBEDDED_DISABLE                      1
#define DRAM_CTRL_CPU_BURST                             30:28
#define DRAM_CTRL_CPU_BURST_1                           0
#define DRAM_CTRL_CPU_BURST_2                           1
#define DRAM_CTRL_CPU_BURST_4                           2
#define DRAM_CTRL_CPU_BURST_8                           3
#define DRAM_CTRL_CPU_CAS_LATENCY                       27:27
#define DRAM_CTRL_CPU_CAS_LATENCY_2                     0
#define DRAM_CTRL_CPU_CAS_LATENCY_3                     1
#define DRAM_CTRL_CPU_SIZE                              26:24
#define DRAM_CTRL_CPU_SIZE_2                            0
#define DRAM_CTRL_CPU_SIZE_4                            1
#define DRAM_CTRL_CPU_SIZE_64                           4
#define DRAM_CTRL_CPU_SIZE_32                           5
#define DRAM_CTRL_CPU_SIZE_16                           6
#define DRAM_CTRL_CPU_SIZE_8                            7
#define DRAM_CTRL_CPU_COLUMN_SIZE                       23:22
#define DRAM_CTRL_CPU_COLUMN_SIZE_1024                  0
#define DRAM_CTRL_CPU_COLUMN_SIZE_512                   2
#define DRAM_CTRL_CPU_COLUMN_SIZE_256                   3
#define DRAM_CTRL_CPU_ACTIVE_PRECHARGE                  21:21
#define DRAM_CTRL_CPU_ACTIVE_PRECHARGE_6                0
#define DRAM_CTRL_CPU_ACTIVE_PRECHARGE_7                1
#define DRAM_CTRL_CPU_RESET                             20:20
#define DRAM_CTRL_CPU_RESET_ENABLE                      0
#define DRAM_CTRL_CPU_RESET_DISABLE                     1
#define DRAM_CTRL_CPU_BANKS                             19:19
#define DRAM_CTRL_CPU_BANKS_2                           0
#define DRAM_CTRL_CPU_BANKS_4                           1
#define DRAM_CTRL_CPU_WRITE_PRECHARGE                   18:18
#define DRAM_CTRL_CPU_WRITE_PRECHARGE_2                 0
#define DRAM_CTRL_CPU_WRITE_PRECHARGE_1                 1
#define DRAM_CTRL_BLOCK_WRITE                           17:17
#define DRAM_CTRL_BLOCK_WRITE_DISABLE                   0
#define DRAM_CTRL_BLOCK_WRITE_ENABLE                    1
#define DRAM_CTRL_REFRESH_COMMAND                       16:16
#define DRAM_CTRL_REFRESH_COMMAND_10                    0
#define DRAM_CTRL_REFRESH_COMMAND_12                    1
#define DRAM_CTRL_SIZE                                  15:13
#define DRAM_CTRL_SIZE_4                                0
#define DRAM_CTRL_SIZE_8                                1
#define DRAM_CTRL_SIZE_16                               2
#define DRAM_CTRL_SIZE_32                               3
#define DRAM_CTRL_SIZE_64                               4
#define DRAM_CTRL_SIZE_2                                5
#define DRAM_CTRL_COLUMN_SIZE                           12:11
#define DRAM_CTRL_COLUMN_SIZE_256                       0
#define DRAM_CTRL_COLUMN_SIZE_512                       2
#define DRAM_CTRL_COLUMN_SIZE_1024                      3
#define DRAM_CTRL_BLOCK_WRITE_TIME                      10:10
#define DRAM_CTRL_BLOCK_WRITE_TIME_1                    0
#define DRAM_CTRL_BLOCK_WRITE_TIME_2                    1
#define DRAM_CTRL_BLOCK_WRITE_PRECHARGE                 9:9
#define DRAM_CTRL_BLOCK_WRITE_PRECHARGE_4               0
#define DRAM_CTRL_BLOCK_WRITE_PRECHARGE_1               1
#define DRAM_CTRL_ACTIVE_PRECHARGE                      8:8
#define DRAM_CTRL_ACTIVE_PRECHARGE_6                    0
#define DRAM_CTRL_ACTIVE_PRECHARGE_7                    1
#define DRAM_CTRL_RESET                                 7:7
#define DRAM_CTRL_RESET_ENABLE                          0
#define DRAM_CTRL_RESET_DISABLE                         1
#define DRAM_CTRL_REMAIN_ACTIVE                         6:6
#define DRAM_CTRL_REMAIN_ACTIVE_ENABLE                  0
#define DRAM_CTRL_REMAIN_ACTIVE_DISABLE                 1
#define DRAM_CTRL_BANKS                                 1:1
#define DRAM_CTRL_BANKS_2                               0
#define DRAM_CTRL_BANKS_4                               1
#define DRAM_CTRL_WRITE_PRECHARGE                       0:0
#define DRAM_CTRL_WRITE_PRECHARGE_2                     0
#define DRAM_CTRL_WRITE_PRECHARGE_1                     1

#define ARBITRATION_CTRL                                0x000014
#define ARBITRATION_CTRL_CPUMEM                         29:29
#define ARBITRATION_CTRL_CPUMEM_FIXED                   0
#define ARBITRATION_CTRL_CPUMEM_ROTATE                  1
#define ARBITRATION_CTRL_INTMEM                         28:28
#define ARBITRATION_CTRL_INTMEM_FIXED                   0
#define ARBITRATION_CTRL_INTMEM_ROTATE                  1
#define ARBITRATION_CTRL_USB                            27:24
#define ARBITRATION_CTRL_USB_OFF                        0
#define ARBITRATION_CTRL_USB_PRIORITY_1                 1
#define ARBITRATION_CTRL_USB_PRIORITY_2                 2
#define ARBITRATION_CTRL_USB_PRIORITY_3                 3
#define ARBITRATION_CTRL_USB_PRIORITY_4                 4
#define ARBITRATION_CTRL_USB_PRIORITY_5                 5
#define ARBITRATION_CTRL_USB_PRIORITY_6                 6
#define ARBITRATION_CTRL_USB_PRIORITY_7                 7
#define ARBITRATION_CTRL_PANEL                          23:20
#define ARBITRATION_CTRL_PANEL_OFF                      0
#define ARBITRATION_CTRL_PANEL_PRIORITY_1               1
#define ARBITRATION_CTRL_PANEL_PRIORITY_2               2
#define ARBITRATION_CTRL_PANEL_PRIORITY_3               3
#define ARBITRATION_CTRL_PANEL_PRIORITY_4               4
#define ARBITRATION_CTRL_PANEL_PRIORITY_5               5
#define ARBITRATION_CTRL_PANEL_PRIORITY_6               6
#define ARBITRATION_CTRL_PANEL_PRIORITY_7               7
#define ARBITRATION_CTRL_ZVPORT                         19:16
#define ARBITRATION_CTRL_ZVPORT_OFF                     0
#define ARBITRATION_CTRL_ZVPORT_PRIORITY_1              1
#define ARBITRATION_CTRL_ZVPORT_PRIORITY_2              2
#define ARBITRATION_CTRL_ZVPORT_PRIORITY_3              3
#define ARBITRATION_CTRL_ZVPORT_PRIORITY_4              4
#define ARBITRATION_CTRL_ZVPORT_PRIORITY_5              5
#define ARBITRATION_CTRL_ZVPORT_PRIORITY_6              6
#define ARBITRATION_CTRL_ZVPORT_PRIORITY_7              7
#define ARBITRATION_CTRL_CMD_INTPR                      15:12
#define ARBITRATION_CTRL_CMD_INTPR_OFF                  0
#define ARBITRATION_CTRL_CMD_INTPR_PRIORITY_1           1
#define ARBITRATION_CTRL_CMD_INTPR_PRIORITY_2           2
#define ARBITRATION_CTRL_CMD_INTPR_PRIORITY_3           3
#define ARBITRATION_CTRL_CMD_INTPR_PRIORITY_4           4
#define ARBITRATION_CTRL_CMD_INTPR_PRIORITY_5           5
#define ARBITRATION_CTRL_CMD_INTPR_PRIORITY_6           6
#define ARBITRATION_CTRL_CMD_INTPR_PRIORITY_7           7
#define ARBITRATION_CTRL_DMA                            11:8
#define ARBITRATION_CTRL_DMA_OFF                        0
#define ARBITRATION_CTRL_DMA_PRIORITY_1                 1
#define ARBITRATION_CTRL_DMA_PRIORITY_2                 2
#define ARBITRATION_CTRL_DMA_PRIORITY_3                 3
#define ARBITRATION_CTRL_DMA_PRIORITY_4                 4
#define ARBITRATION_CTRL_DMA_PRIORITY_5                 5
#define ARBITRATION_CTRL_DMA_PRIORITY_6                 6
#define ARBITRATION_CTRL_DMA_PRIORITY_7                 7
#define ARBITRATION_CTRL_VIDEO                          7:4
#define ARBITRATION_CTRL_VIDEO_OFF                      0
#define ARBITRATION_CTRL_VIDEO_PRIORITY_1               1
#define ARBITRATION_CTRL_VIDEO_PRIORITY_2               2
#define ARBITRATION_CTRL_VIDEO_PRIORITY_3               3
#define ARBITRATION_CTRL_VIDEO_PRIORITY_4               4
#define ARBITRATION_CTRL_VIDEO_PRIORITY_5               5
#define ARBITRATION_CTRL_VIDEO_PRIORITY_6               6
#define ARBITRATION_CTRL_VIDEO_PRIORITY_7               7
#define ARBITRATION_CTRL_CRT                            3:0
#define ARBITRATION_CTRL_CRT_OFF                        0
#define ARBITRATION_CTRL_CRT_PRIORITY_1                 1
#define ARBITRATION_CTRL_CRT_PRIORITY_2                 2
#define ARBITRATION_CTRL_CRT_PRIORITY_3                 3
#define ARBITRATION_CTRL_CRT_PRIORITY_4                 4
#define ARBITRATION_CTRL_CRT_PRIORITY_5                 5
#define ARBITRATION_CTRL_CRT_PRIORITY_6                 6
#define ARBITRATION_CTRL_CRT_PRIORITY_7                 7

#define CMD_INTPR_CTRL                                  0x000018
#define CMD_INTPR_CTRL_STATUS                           31:31
#define CMD_INTPR_CTRL_STATUS_STOPPED                   0
#define CMD_INTPR_CTRL_STATUS_RUNNING                   1
#define CMD_INTPR_CTRL_EXT                              27:27
#define CMD_INTPR_CTRL_EXT_LOCAL                        0
#define CMD_INTPR_CTRL_EXT_EXTERNAL                     1
#define CMD_INTPR_CTRL_CS                               26:26
#define CMD_INTPR_CTRL_CS_0                             0
#define CMD_INTPR_CTRL_CS_1                             1
#define CMD_INTPR_CTRL_ADDRESS                          25:0

#define CMD_INTPR_CONDITIONS                            0x00001C

#define CMD_INTPR_RETURN                                0x000020
#define CMD_INTPR_RETURN_EXT                            27:27
#define CMD_INTPR_RETURN_EXT_LOCAL                      0
#define CMD_INTPR_RETURN_EXT_EXTERNAL                   1
#define CMD_INTPR_RETURN_CS                             26:26
#define CMD_INTPR_RETURN_CS_0                           0
#define CMD_INTPR_RETURN_CS_1                           1
#define CMD_INTPR_RETURN_ADDRESS                        25:0

#define CMD_INTPR_STATUS                                0x000024
#define CMD_INTPR_STATUS_2D_MEMORY_FIFO                 20:20
#define CMD_INTPR_STATUS_2D_MEMORY_FIFO_NOT_EMPTY       0
#define CMD_INTPR_STATUS_2D_MEMORY_FIFO_EMPTY           1
#define CMD_INTPR_STATUS_COMMAND_FIFO                   19:19
#define CMD_INTPR_STATUS_COMMAND_FIFO_NOT_EMPTY         0
#define CMD_INTPR_STATUS_COMMAND_FIFO_EMPTY             1
#define CMD_INTPR_STATUS_CSC_STATUS                     18:18
#define CMD_INTPR_STATUS_CSC_STATUS_IDLE                0
#define CMD_INTPR_STATUS_CSC_STATUS_BUSY                1
#define CMD_INTPR_STATUS_MEMORY_DMA                     17:17
#define CMD_INTPR_STATUS_MEMORY_DMA_IDLE                0
#define CMD_INTPR_STATUS_MEMORY_DMA_BUSY                1
#define CMD_INTPR_STATUS_CRT_STATUS                     16:16
#define CMD_INTPR_STATUS_CRT_STATUS_CURRENT             0
#define CMD_INTPR_STATUS_CRT_STATUS_PENDING             1
#define CMD_INTPR_STATUS_CURRENT_FIELD                  15:15
#define CMD_INTPR_STATUS_CURRENT_FIELD_ODD              0
#define CMD_INTPR_STATUS_CURRENT_FIELD_EVEN             1
#define CMD_INTPR_STATUS_VIDEO_STATUS                   14:14
#define CMD_INTPR_STATUS_VIDEO_STATUS_CURRENT           0
#define CMD_INTPR_STATUS_VIDEO_STATUS_PENDING           1
#define CMD_INTPR_STATUS_PANEL_STATUS                   13:13
#define CMD_INTPR_STATUS_PANEL_STATUS_CURRENT           0
#define CMD_INTPR_STATUS_PANEL_STATUS_PENDING           1
#define CMD_INTPR_STATUS_CRT_SYNC                       12:12
#define CMD_INTPR_STATUS_CRT_SYNC_INACTIVE              0
#define CMD_INTPR_STATUS_CRT_SYNC_ACTIVE                1
#define CMD_INTPR_STATUS_PANEL_SYNC                     11:11
#define CMD_INTPR_STATUS_PANEL_SYNC_INACTIVE            0
#define CMD_INTPR_STATUS_PANEL_SYNC_ACTIVE              1
#define CMD_INTPR_STATUS_2D_SETUP                       2:2
#define CMD_INTPR_STATUS_2D_SETUP_IDLE                  0
#define CMD_INTPR_STATUS_2D_SETUP_BUSY                  1
#define CMD_INTPR_STATUS_2D_FIFO                        1:1
#define CMD_INTPR_STATUS_2D_FIFO_NOT_EMPTY              0
#define CMD_INTPR_STATUS_2D_FIFO_EMPTY                  1
#define CMD_INTPR_STATUS_2D_ENGINE                      0:0
#define CMD_INTPR_STATUS_2D_ENGINE_IDLE                 0
#define CMD_INTPR_STATUS_2D_ENGINE_BUSY                 1

#define RAW_INT_STATUS                                  0x000028
#define RAW_INT_STATUS_USB_SLAVE_PLUG_IN                5:5
#define RAW_INT_STATUS_USB_SLAVE_PLUG_IN_INACTIVE       0
#define RAW_INT_STATUS_USB_SLAVE_PLUG_IN_ACTIVE         1
#define RAW_INT_STATUS_USB_SLAVE_PLUG_IN_CLEAR          1
#define RAW_INT_STATUS_ZVPORT                           4:4
#define RAW_INT_STATUS_ZVPORT_INACTIVE                  0
#define RAW_INT_STATUS_ZVPORT_ACTIVE                    1
#define RAW_INT_STATUS_ZVPORT_CLEAR                     1
#define RAW_INT_STATUS_CRT_VSYNC                        3:3
#define RAW_INT_STATUS_CRT_VSYNC_INACTIVE               0
#define RAW_INT_STATUS_CRT_VSYNC_ACTIVE                 1
#define RAW_INT_STATUS_CRT_VSYNC_CLEAR                  1
#define RAW_INT_STATUS_USB_SLAVE                        2:2
#define RAW_INT_STATUS_USB_SLAVE_INACTIVE               0
#define RAW_INT_STATUS_USB_SLAVE_ACTIVE                 1
#define RAW_INT_STATUS_USB_SLAVE_CLEAR                  1
#define RAW_INT_STATUS_PANEL_VSYNC                      1:1
#define RAW_INT_STATUS_PANEL_VSYNC_INACTIVE             0
#define RAW_INT_STATUS_PANEL_VSYNC_ACTIVE               1
#define RAW_INT_STATUS_PANEL_VSYNC_CLEAR                1
#define RAW_INT_STATUS_CMD_INTPR                        0:0
#define RAW_INT_STATUS_CMD_INTPR_INACTIVE               0
#define RAW_INT_STATUS_CMD_INTPR_ACTIVE                 1
#define RAW_INT_STATUS_CMD_INTPR_CLEAR                  1

#define INT_STATUS                                      0x00002C
#define INT_STATUS_USB_SLAVE_PLUG_IN                    31:31
#define INT_STATUS_USB_SLAVE_PLUG_IN_INACTIVE           0
#define INT_STATUS_USB_SLAVE_PLUG_IN_ACTIVE             1
#define INT_STATUS_GPIO54                               30:30
#define INT_STATUS_GPIO54_INACTIVE                      0
#define INT_STATUS_GPIO54_ACTIVE                        1
#define INT_STATUS_GPIO53                               29:29
#define INT_STATUS_GPIO53_INACTIVE                      0
#define INT_STATUS_GPIO53_ACTIVE                        1
#define INT_STATUS_GPIO52                               28:28
#define INT_STATUS_GPIO52_INACTIVE                      0
#define INT_STATUS_GPIO52_ACTIVE                        1
#define INT_STATUS_GPIO51                               27:27
#define INT_STATUS_GPIO51_INACTIVE                      0
#define INT_STATUS_GPIO51_ACTIVE                        1
#define INT_STATUS_GPIO50                               26:26
#define INT_STATUS_GPIO50_INACTIVE                      0
#define INT_STATUS_GPIO50_ACTIVE                        1
#define INT_STATUS_GPIO49                               25:25
#define INT_STATUS_GPIO49_INACTIVE                      0
#define INT_STATUS_GPIO49_ACTIVE                        1
#define INT_STATUS_GPIO48                               24:24
#define INT_STATUS_GPIO48_INACTIVE                      0
#define INT_STATUS_GPIO48_ACTIVE                        1
#define INT_STATUS_I2C                                  23:23
#define INT_STATUS_I2C_INACTIVE                         0
#define INT_STATUS_I2C_ACTIVE                           1
#define INT_STATUS_PWM                                  22:22
#define INT_STATUS_PWM_INACTIVE                         0
#define INT_STATUS_PWM_ACTIVE                           1
#define INT_STATUS_DMA                                  20:20
#define INT_STATUS_DMA_INACTIVE                         0
#define INT_STATUS_DMA_ACTIVE                           1
#define INT_STATUS_PCI                                  19:19
#define INT_STATUS_PCI_INACTIVE                         0
#define INT_STATUS_PCI_ACTIVE                           1
#define INT_STATUS_I2S                                  18:18
#define INT_STATUS_I2S_INACTIVE                         0
#define INT_STATUS_I2S_ACTIVE                           1
#define INT_STATUS_AC97                                 17:17
#define INT_STATUS_AC97_INACTIVE                        0
#define INT_STATUS_AC97_ACTIVE                          1
#define INT_STATUS_USB_SLAVE                            16:16
#define INT_STATUS_USB_SLAVE_INACTIVE                   0
#define INT_STATUS_USB_SLAVE_ACTIVE                     1
#define INT_STATUS_UART1                                13:13
#define INT_STATUS_UART1_INACTIVE                       0
#define INT_STATUS_UART1_ACTIVE                         1
#define INT_STATUS_UART0                                12:12
#define INT_STATUS_UART0_INACTIVE                       0
#define INT_STATUS_UART0_ACTIVE                         1
#define INT_STATUS_CRT_VSYNC                            11:11
#define INT_STATUS_CRT_VSYNC_INACTIVE                   0
#define INT_STATUS_CRT_VSYNC_ACTIVE                     1
#define INT_STATUS_8051                                 10:10
#define INT_STATUS_8051_INACTIVE                        0
#define INT_STATUS_8051_ACTIVE                          1
#define INT_STATUS_SSP1                                 9:9
#define INT_STATUS_SSP1_INACTIVE                        0
#define INT_STATUS_SSP1_ACTIVE                          1
#define INT_STATUS_SSP0                                 8:8
#define INT_STATUS_SSP0_INACTIVE                        0
#define INT_STATUS_SSP0_ACTIVE                          1
#define INT_STATUS_USB_HOST                             6:6
#define INT_STATUS_USB_HOST_INACTIVE                    0
#define INT_STATUS_USB_HOST_ACTIVE                      1
#define INT_STATUS_2D                                   3:3
#define INT_STATUS_2D_INACTIVE                          0
#define INT_STATUS_2D_ACTIVE                            1
#define INT_STATUS_ZVPORT                               2:2
#define INT_STATUS_ZVPORT_INACTIVE                      0
#define INT_STATUS_ZVPORT_ACTIVE                        1
#define INT_STATUS_PANEL_VSYNC                          1:1
#define INT_STATUS_PANEL_VSYNC_INACTIVE                 0
#define INT_STATUS_PANEL_VSYNC_ACTIVE                   1
#define INT_STATUS_CMD_INTPR                            0:0
#define INT_STATUS_CMD_INTPR_INACTIVE                   0
#define INT_STATUS_CMD_INTPR_ACTIVE                     1

#define INT_MASK                                        0x000030
#define INT_MASK_USB_SLAVE_PLUG_IN                      31:31
#define INT_MASK_USB_SLAVE_PLUG_IN_DISABLE              0
#define INT_MASK_USB_SLAVE_PLUG_IN_ENABLE               1
#define INT_MASK_GPIO54                                 30:30
#define INT_MASK_GPIO54_DISABLE                         0
#define INT_MASK_GPIO54_ENABLE                          1
#define INT_MASK_GPIO53                                 29:29
#define INT_MASK_GPIO53_DISABLE                         0
#define INT_MASK_GPIO53_ENABLE                          1
#define INT_MASK_GPIO52                                 28:28
#define INT_MASK_GPIO52_DISABLE                         0
#define INT_MASK_GPIO52_ENABLE                          1
#define INT_MASK_GPIO51                                 27:27
#define INT_MASK_GPIO51_DISABLE                         0
#define INT_MASK_GPIO51_ENABLE                          1
#define INT_MASK_GPIO50                                 26:26
#define INT_MASK_GPIO50_DISABLE                         0
#define INT_MASK_GPIO50_ENABLE                          1
#define INT_MASK_GPIO49                                 25:25
#define INT_MASK_GPIO49_DISABLE                         0
#define INT_MASK_GPIO49_ENABLE                          1
#define INT_MASK_GPIO48                                 24:24
#define INT_MASK_GPIO48_DISABLE                         0
#define INT_MASK_GPIO48_ENABLE                          1
#define INT_MASK_I2C                                    23:23
#define INT_MASK_I2C_DISABLE                            0
#define INT_MASK_I2C_ENABLE                             1
#define INT_MASK_PWM                                    22:22
#define INT_MASK_PWM_DISABLE                            0
#define INT_MASK_PWM_ENABLE                             1
#define INT_MASK_DMA                                    20:20
#define INT_MASK_DMA_DISABLE                            0
#define INT_MASK_DMA_ENABLE                             1
#define INT_MASK_PCI                                    19:19
#define INT_MASK_PCI_DISABLE                            0
#define INT_MASK_PCI_ENABLE                             1
#define INT_MASK_I2S                                    18:18
#define INT_MASK_I2S_DISABLE                            0
#define INT_MASK_I2S_ENABLE                             1
#define INT_MASK_AC97                                   17:17
#define INT_MASK_AC97_DISABLE                           0
#define INT_MASK_AC97_ENABLE                            1
#define INT_MASK_USB_SLAVE                              16:16
#define INT_MASK_USB_SLAVE_DISABLE                      0
#define INT_MASK_USB_SLAVE_ENABLE                       1
#define INT_MASK_UART1                                  13:13
#define INT_MASK_UART1_DISABLE                          0
#define INT_MASK_UART1_ENABLE                           1
#define INT_MASK_UART0                                  12:12
#define INT_MASK_UART0_DISABLE                          0
#define INT_MASK_UART0_ENABLE                           1
#define INT_MASK_CRT_VSYNC                              11:11
#define INT_MASK_CRT_VSYNC_DISABLE                      0
#define INT_MASK_CRT_VSYNC_ENABLE                       1
#define INT_MASK_8051                                   10:10
#define INT_MASK_8051_DISABLE                           0
#define INT_MASK_8051_ENABLE                            1
#define INT_MASK_SSP1                                   9:9
#define INT_MASK_SSP1_DISABLE                           0
#define INT_MASK_SSP1_ENABLE                            1
#define INT_MASK_SSP0                                   8:8
#define INT_MASK_SSP0_DISABLE                           0
#define INT_MASK_SSP0_ENABLE                            1
#define INT_MASK_USB_HOST                               6:6
#define INT_MASK_USB_HOST_DISABLE                       0
#define INT_MASK_USB_HOST_ENABLE                        1
#define INT_MASK_2D                                     3:3
#define INT_MASK_2D_DISABLE                             0
#define INT_MASK_2D_ENABLE                              1
#define INT_MASK_ZVPORT                                 2:2
#define INT_MASK_ZVPORT_DISABLE                         0
#define INT_MASK_ZVPORT_ENABLE                          1
#define INT_MASK_PANEL_VSYNC                            1:1
#define INT_MASK_PANEL_VSYNC_DISABLE                    0
#define INT_MASK_PANEL_VSYNC_ENABLE                     1
#define INT_MASK_CMD_INTPR                              0:0
#define INT_MASK_CMD_INTPR_DISABLE                      0
#define INT_MASK_CMD_INTPR_ENABLE                       1

#define DEBUG_CTRL                                      0x000034
#define DEBUG_CTRL_MODULE                               7:5
#define DEBUG_CTRL_PARTITION                            4:0
#define DEBUG_CTRL_PARTITION_HIF                        0
#define DEBUG_CTRL_PARTITION_CPUMEM                     1
#define DEBUG_CTRL_PARTITION_PCI                        2
#define DEBUG_CTRL_PARTITION_CMD_INTPR                  3
#define DEBUG_CTRL_PARTITION_DISPLAY                    4
#define DEBUG_CTRL_PARTITION_ZVPORT                     5
#define DEBUG_CTRL_PARTITION_2D                         6
#define DEBUG_CTRL_PARTITION_MIF                        8
#define DEBUG_CTRL_PARTITION_USB_HOST                   10
#define DEBUG_CTRL_PARTITION_SSP0                       12
#define DEBUG_CTRL_PARTITION_SSP1                       13
#define DEBUG_CTRL_PARTITION_UART0                      19
#define DEBUG_CTRL_PARTITION_UART1                      20
#define DEBUG_CTRL_PARTITION_I2C                        21
#define DEBUG_CTRL_PARTITION_8051                       23
#define DEBUG_CTRL_PARTITION_AC97                       24
#define DEBUG_CTRL_PARTITION_I2S                        25
#define DEBUG_CTRL_PARTITION_INTMEM                     26
#define DEBUG_CTRL_PARTITION_DMA                        27
#define DEBUG_CTRL_PARTITION_SIMULATION                 28

#define CURRENT_POWER_GATE                              0x000038
#define CURRENT_POWER_GATE_AC97_I2S                     18:18
#define CURRENT_POWER_GATE_AC97_I2S_DISABLE             0
#define CURRENT_POWER_GATE_AC97_I2S_ENABLE              1
#define CURRENT_POWER_GATE_8051                         17:17
#define CURRENT_POWER_GATE_8051_DISABLE                 0
#define CURRENT_POWER_GATE_8051_ENABLE                  1
#define CURRENT_POWER_GATE_PLL                          16:16
#define CURRENT_POWER_GATE_PLL_DISABLE                  0
#define CURRENT_POWER_GATE_PLL_ENABLE                   1
#define CURRENT_POWER_GATE_OSCILLATOR                   15:15
#define CURRENT_POWER_GATE_OSCILLATOR_DISABLE           0
#define CURRENT_POWER_GATE_OSCILLATOR_ENABLE            1
#define CURRENT_POWER_GATE_PLL_RECOVERY                 14:13
#define CURRENT_POWER_GATE_PLL_RECOVERY_32              0
#define CURRENT_POWER_GATE_PLL_RECOVERY_64              1
#define CURRENT_POWER_GATE_PLL_RECOVERY_96              2
#define CURRENT_POWER_GATE_PLL_RECOVERY_128             3
#define CURRENT_POWER_GATE_USB_SLAVE                    12:12
#define CURRENT_POWER_GATE_USB_SLAVE_DISABLE            0
#define CURRENT_POWER_GATE_USB_SLAVE_ENABLE             1
#define CURRENT_POWER_GATE_USB_HOST                     11:11
#define CURRENT_POWER_GATE_USB_HOST_DISABLE             0
#define CURRENT_POWER_GATE_USB_HOST_ENABLE              1
#define CURRENT_POWER_GATE_SSP0_SSP1                    10:10
#define CURRENT_POWER_GATE_SSP0_SSP1_DISABLE            0
#define CURRENT_POWER_GATE_SSP0_SSP1_ENABLE             1
#define CURRENT_POWER_GATE_UART1                        8:8
#define CURRENT_POWER_GATE_UART1_DISABLE                0
#define CURRENT_POWER_GATE_UART1_ENABLE                 1
#define CURRENT_POWER_GATE_UART0                        7:7
#define CURRENT_POWER_GATE_UART0_DISABLE                0
#define CURRENT_POWER_GATE_UART0_ENABLE                 1
#define CURRENT_POWER_GATE_GPIO_PWM_I2C                 6:6
#define CURRENT_POWER_GATE_GPIO_PWM_I2C_DISABLE         0
#define CURRENT_POWER_GATE_GPIO_PWM_I2C_ENABLE          1
#define CURRENT_POWER_GATE_ZVPORT                       5:5
#define CURRENT_POWER_GATE_ZVPORT_DISABLE               0
#define CURRENT_POWER_GATE_ZVPORT_ENABLE                1
#define CURRENT_POWER_GATE_CSC                          4:4
#define CURRENT_POWER_GATE_CSC_DISABLE                  0
#define CURRENT_POWER_GATE_CSC_ENABLE                   1
#define CURRENT_POWER_GATE_2D                           3:3
#define CURRENT_POWER_GATE_2D_DISABLE                   0
#define CURRENT_POWER_GATE_2D_ENABLE                    1
#define CURRENT_POWER_GATE_DISPLAY                      2:2
#define CURRENT_POWER_GATE_DISPLAY_DISABLE              0
#define CURRENT_POWER_GATE_DISPLAY_ENABLE               1
#define CURRENT_POWER_GATE_INTMEM                       1:1
#define CURRENT_POWER_GATE_INTMEM_DISABLE               0
#define CURRENT_POWER_GATE_INTMEM_ENABLE                1
#define CURRENT_POWER_GATE_HOST                         0:0
#define CURRENT_POWER_GATE_HOST_DISABLE                 0
#define CURRENT_POWER_GATE_HOST_ENABLE                  1

#define CURRENT_POWER_CLOCK                             0x00003C
#define CURRENT_POWER_CLOCK_P1XCLK               31:31
#define CURRENT_POWER_CLOCK_P1XCLK_ENABLE               1
#define CURRENT_POWER_CLOCK_P1XCLK_DISABLE              0
#define CURRENT_POWER_CLOCK_PLLCLK_SELECT               30:30
#define CURRENT_POWER_CLOCK_PLLCLK_SELECT_ENABLE               1
#define CURRENT_POWER_CLOCK_PLLCLK_SELECT_DISABLE              0
#define CURRENT_POWER_CLOCK_P2XCLK_SELECT               29:29
#define CURRENT_POWER_CLOCK_P2XCLK_SELECT_288           0
#define CURRENT_POWER_CLOCK_P2XCLK_SELECT_336           1
#define CURRENT_POWER_CLOCK_P2XCLK_DIVIDER              28:27
#define CURRENT_POWER_CLOCK_P2XCLK_DIVIDER_1            0
#define CURRENT_POWER_CLOCK_P2XCLK_DIVIDER_3            1
#define CURRENT_POWER_CLOCK_P2XCLK_DIVIDER_5            2
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT                26:24
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_0              0
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_1              1
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_2              2
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_3              3
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_4              4
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_5              5
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_6              6
#define CURRENT_POWER_CLOCK_P2XCLK_SHIFT_7              7
#define CURRENT_POWER_CLOCK_V2XCLK_SELECT               20:20
#define CURRENT_POWER_CLOCK_V2XCLK_SELECT_288           0
#define CURRENT_POWER_CLOCK_V2XCLK_SELECT_336           1
#define CURRENT_POWER_CLOCK_V2XCLK_DIVIDER              19:19
#define CURRENT_POWER_CLOCK_V2XCLK_DIVIDER_1            0
#define CURRENT_POWER_CLOCK_V2XCLK_DIVIDER_3            1
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT                18:16
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_0              0
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_1              1
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_2              2
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_3              3
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_4              4
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_5              5
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_6              6
#define CURRENT_POWER_CLOCK_V2XCLK_SHIFT_7              7
#define CURRENT_POWER_CLOCK_MCLK_SELECT                 12:12
#define CURRENT_POWER_CLOCK_MCLK_SELECT_288             0
#define CURRENT_POWER_CLOCK_MCLK_SELECT_336             1
#define CURRENT_POWER_CLOCK_MCLK_DIVIDER                11:11
#define CURRENT_POWER_CLOCK_MCLK_DIVIDER_1              0
#define CURRENT_POWER_CLOCK_MCLK_DIVIDER_3              1
#define CURRENT_POWER_CLOCK_MCLK_SHIFT                  10:8
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_0                0
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_1                1
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_2                2
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_3                3
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_4                4
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_5                5
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_6                6
#define CURRENT_POWER_CLOCK_MCLK_SHIFT_7                7
#define CURRENT_POWER_CLOCK_M2XCLK_SELECT               4:4
#define CURRENT_POWER_CLOCK_M2XCLK_SELECT_288           0
#define CURRENT_POWER_CLOCK_M2XCLK_SELECT_336           1
#define CURRENT_POWER_CLOCK_M2XCLK_DIVIDER              3:3
#define CURRENT_POWER_CLOCK_M2XCLK_DIVIDER_1            0
#define CURRENT_POWER_CLOCK_M2XCLK_DIVIDER_3            1
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT                2:0
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_0              0
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_1              1
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_2              2
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_3              3
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_4              4
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_5              5
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_6              6
#define CURRENT_POWER_CLOCK_M2XCLK_SHIFT_7              7

#define POWER_MODE0_GATE                                0x000040
#define POWER_MODE0_GATE_AC97_I2S                       18:18
#define POWER_MODE0_GATE_AC97_I2S_DISABLE               0
#define POWER_MODE0_GATE_AC97_I2S_ENABLE                1
#define POWER_MODE0_GATE_8051                           17:17
#define POWER_MODE0_GATE_8051_DISABLE                   0
#define POWER_MODE0_GATE_8051_ENABLE                    1
#define POWER_MODE0_GATE_USB_SLAVE                      12:12
#define POWER_MODE0_GATE_USB_SLAVE_DISABLE              0
#define POWER_MODE0_GATE_USB_SLAVE_ENABLE               1
#define POWER_MODE0_GATE_USB_HOST                       11:11
#define POWER_MODE0_GATE_USB_HOST_DISABLE               0
#define POWER_MODE0_GATE_USB_HOST_ENABLE                1
#define POWER_MODE0_GATE_SSP0_SSP1                      10:10
#define POWER_MODE0_GATE_SSP0_SSP1_DISABLE              0
#define POWER_MODE0_GATE_SSP0_SSP1_ENABLE               1
#define POWER_MODE0_GATE_UART1                          8:8
#define POWER_MODE0_GATE_UART1_DISABLE                  0
#define POWER_MODE0_GATE_UART1_ENABLE                   1
#define POWER_MODE0_GATE_UART0                          7:7
#define POWER_MODE0_GATE_UART0_DISABLE                  0
#define POWER_MODE0_GATE_UART0_ENABLE                   1
#define POWER_MODE0_GATE_GPIO_PWM_I2C                   6:6
#define POWER_MODE0_GATE_GPIO_PWM_I2C_DISABLE           0
#define POWER_MODE0_GATE_GPIO_PWM_I2C_ENABLE            1
#define POWER_MODE0_GATE_ZVPORT                         5:5
#define POWER_MODE0_GATE_ZVPORT_DISABLE                 0
#define POWER_MODE0_GATE_ZVPORT_ENABLE                  1
#define POWER_MODE0_GATE_CSC                            4:4
#define POWER_MODE0_GATE_CSC_DISABLE                    0
#define POWER_MODE0_GATE_CSC_ENABLE                     1
#define POWER_MODE0_GATE_2D                             3:3
#define POWER_MODE0_GATE_2D_DISABLE                     0
#define POWER_MODE0_GATE_2D_ENABLE                      1
#define POWER_MODE0_GATE_DISPLAY                        2:2
#define POWER_MODE0_GATE_DISPLAY_DISABLE                0
#define POWER_MODE0_GATE_DISPLAY_ENABLE                 1
#define POWER_MODE0_GATE_INTMEM                         1:1
#define POWER_MODE0_GATE_INTMEM_DISABLE                 0
#define POWER_MODE0_GATE_INTMEM_ENABLE                  1
#define POWER_MODE0_GATE_HOST                           0:0
#define POWER_MODE0_GATE_HOST_DISABLE                   0
#define POWER_MODE0_GATE_HOST_ENABLE                    1

#define POWER_MODE0_CLOCK                               0x000044
#define POWER_MODE0_CLOCK_PLL3_P1XCLK					31:31
#define POWER_MODE0_CLOCK_PLL3_P1XCLK_ENABLE			1
#define POWER_MODE0_CLOCK_PLL3_P1XCLK_DISABLE			0
#define POWER_MODE0_CLOCK_PLL3							30:30
#define POWER_MODE0_CLOCK_PLL3_ENABLE					1
#define POWER_MODE0_CLOCK_PLL3_DISABLE					0							
#define POWER_MODE0_CLOCK_P2XCLK_SELECT                 29:29
#define POWER_MODE0_CLOCK_P2XCLK_SELECT_288             0
#define POWER_MODE0_CLOCK_P2XCLK_SELECT_336             1
#define POWER_MODE0_CLOCK_P2XCLK_DIVIDER                28:27
#define POWER_MODE0_CLOCK_P2XCLK_DIVIDER_1              0
#define POWER_MODE0_CLOCK_P2XCLK_DIVIDER_3              1
#define POWER_MODE0_CLOCK_P2XCLK_DIVIDER_5              2
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT                  26:24
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_0                0
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_1                1
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_2                2
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_3                3
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_4                4
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_5                5
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_6                6
#define POWER_MODE0_CLOCK_P2XCLK_SHIFT_7                7
#define POWER_MODE0_CLOCK_V2XCLK_SELECT                 20:20
#define POWER_MODE0_CLOCK_V2XCLK_SELECT_288             0
#define POWER_MODE0_CLOCK_V2XCLK_SELECT_336             1
#define POWER_MODE0_CLOCK_V2XCLK_DIVIDER                19:19
#define POWER_MODE0_CLOCK_V2XCLK_DIVIDER_1              0
#define POWER_MODE0_CLOCK_V2XCLK_DIVIDER_3              1
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT                  18:16
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_0                0
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_1                1
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_2                2
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_3                3
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_4                4
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_5                5
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_6                6
#define POWER_MODE0_CLOCK_V2XCLK_SHIFT_7                7
#define POWER_MODE0_CLOCK_MCLK_SELECT                   12:12
#define POWER_MODE0_CLOCK_MCLK_SELECT_288               0
#define POWER_MODE0_CLOCK_MCLK_SELECT_336               1
#define POWER_MODE0_CLOCK_MCLK_DIVIDER                  11:11
#define POWER_MODE0_CLOCK_MCLK_DIVIDER_1                0
#define POWER_MODE0_CLOCK_MCLK_DIVIDER_3                1
#define POWER_MODE0_CLOCK_MCLK_SHIFT                    10:8
#define POWER_MODE0_CLOCK_MCLK_SHIFT_0                  0
#define POWER_MODE0_CLOCK_MCLK_SHIFT_1                  1
#define POWER_MODE0_CLOCK_MCLK_SHIFT_2                  2
#define POWER_MODE0_CLOCK_MCLK_SHIFT_3                  3
#define POWER_MODE0_CLOCK_MCLK_SHIFT_4                  4
#define POWER_MODE0_CLOCK_MCLK_SHIFT_5                  5
#define POWER_MODE0_CLOCK_MCLK_SHIFT_6                  6
#define POWER_MODE0_CLOCK_MCLK_SHIFT_7                  7
#define POWER_MODE0_CLOCK_M2XCLK_SELECT                 4:4
#define POWER_MODE0_CLOCK_M2XCLK_SELECT_288             0
#define POWER_MODE0_CLOCK_M2XCLK_SELECT_336             1
#define POWER_MODE0_CLOCK_M2XCLK_DIVIDER                3:3
#define POWER_MODE0_CLOCK_M2XCLK_DIVIDER_1              0
#define POWER_MODE0_CLOCK_M2XCLK_DIVIDER_3              1
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT                  2:0
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_0                0
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_1                1
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_2                2
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_3                3
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_4                4
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_5                5
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_6                6
#define POWER_MODE0_CLOCK_M2XCLK_SHIFT_7                7

#define POWER_MODE1_GATE                                0x000048
#define POWER_MODE1_GATE_AC97_I2S                       18:18
#define POWER_MODE1_GATE_AC97_I2S_DISABLE               0
#define POWER_MODE1_GATE_AC97_I2S_ENABLE                1
#define POWER_MODE1_GATE_8051                           17:17
#define POWER_MODE1_GATE_8051_DISABLE                   0
#define POWER_MODE1_GATE_8051_ENABLE                    1
#define POWER_MODE1_GATE_USB_SLAVE                      12:12
#define POWER_MODE1_GATE_USB_SLAVE_DISABLE              0
#define POWER_MODE1_GATE_USB_SLAVE_ENABLE               1
#define POWER_MODE1_GATE_USB_HOST                       11:11
#define POWER_MODE1_GATE_USB_HOST_DISABLE               0
#define POWER_MODE1_GATE_USB_HOST_ENABLE                1
#define POWER_MODE1_GATE_SSP0_SSP1                      10:10
#define POWER_MODE1_GATE_SSP0_SSP1_DISABLE              0
#define POWER_MODE1_GATE_SSP0_SSP1_ENABLE               1
#define POWER_MODE1_GATE_UART1                          8:8
#define POWER_MODE1_GATE_UART1_DISABLE                  0
#define POWER_MODE1_GATE_UART1_ENABLE                   1
#define POWER_MODE1_GATE_UART0                          7:7
#define POWER_MODE1_GATE_UART0_DISABLE                  0
#define POWER_MODE1_GATE_UART0_ENABLE                   1
#define POWER_MODE1_GATE_GPIO_PWM_I2C                   6:6
#define POWER_MODE1_GATE_GPIO_PWM_I2C_DISABLE           0
#define POWER_MODE1_GATE_GPIO_PWM_I2C_ENABLE            1
#define POWER_MODE1_GATE_ZVPORT                         5:5
#define POWER_MODE1_GATE_ZVPORT_DISABLE                 0
#define POWER_MODE1_GATE_ZVPORT_ENABLE                  1
#define POWER_MODE1_GATE_CSC                            4:4
#define POWER_MODE1_GATE_CSC_DISABLE                    0
#define POWER_MODE1_GATE_CSC_ENABLE                     1
#define POWER_MODE1_GATE_2D                             3:3
#define POWER_MODE1_GATE_2D_DISABLE                     0
#define POWER_MODE1_GATE_2D_ENABLE                      1
#define POWER_MODE1_GATE_DISPLAY                        2:2
#define POWER_MODE1_GATE_DISPLAY_DISABLE                0
#define POWER_MODE1_GATE_DISPLAY_ENABLE                 1
#define POWER_MODE1_GATE_INTMEM                         1:1
#define POWER_MODE1_GATE_INTMEM_DISABLE                 0
#define POWER_MODE1_GATE_INTMEM_ENABLE                  1
#define POWER_MODE1_GATE_HOST                           0:0
#define POWER_MODE1_GATE_HOST_DISABLE                   0
#define POWER_MODE1_GATE_HOST_ENABLE                    1

#define POWER_MODE1_CLOCK                               0x00004C
#define POWER_MODE1_CLOCK_PLL3_P1XCLK					31:31
#define POWER_MODE1_CLOCK_PLL3_P1XCLK_ENABLE			1
#define POWER_MODE1_CLOCK_PLL3_P1XCLK_DISABLE			0
#define POWER_MODE1_CLOCK_PLL3							30:30
#define POWER_MODE1_CLOCK_PLL3_ENABLE					1
#define POWER_MODE1_CLOCK_PLL3_DISABLE					0							
#define POWER_MODE1_CLOCK_P2XCLK_SELECT                 29:29
#define POWER_MODE1_CLOCK_P2XCLK_SELECT_288             0
#define POWER_MODE1_CLOCK_P2XCLK_SELECT_336             1
#define POWER_MODE1_CLOCK_P2XCLK_DIVIDER                28:27
#define POWER_MODE1_CLOCK_P2XCLK_DIVIDER_1              0
#define POWER_MODE1_CLOCK_P2XCLK_DIVIDER_3              1
#define POWER_MODE1_CLOCK_P2XCLK_DIVIDER_5              2
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT                  26:24
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_0                0
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_1                1
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_2                2
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_3                3
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_4                4
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_5                5
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_6                6
#define POWER_MODE1_CLOCK_P2XCLK_SHIFT_7                7
#define POWER_MODE1_CLOCK_V2XCLK_SELECT                 20:20
#define POWER_MODE1_CLOCK_V2XCLK_SELECT_288             0
#define POWER_MODE1_CLOCK_V2XCLK_SELECT_336             1
#define POWER_MODE1_CLOCK_V2XCLK_DIVIDER                19:19
#define POWER_MODE1_CLOCK_V2XCLK_DIVIDER_1              0
#define POWER_MODE1_CLOCK_V2XCLK_DIVIDER_3              1
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT                  18:16
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_0                0
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_1                1
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_2                2
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_3                3
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_4                4
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_5                5
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_6                6
#define POWER_MODE1_CLOCK_V2XCLK_SHIFT_7                7
#define POWER_MODE1_CLOCK_MCLK_SELECT                   12:12
#define POWER_MODE1_CLOCK_MCLK_SELECT_288               0
#define POWER_MODE1_CLOCK_MCLK_SELECT_336               1
#define POWER_MODE1_CLOCK_MCLK_DIVIDER                  11:11
#define POWER_MODE1_CLOCK_MCLK_DIVIDER_1                0
#define POWER_MODE1_CLOCK_MCLK_DIVIDER_3                1
#define POWER_MODE1_CLOCK_MCLK_SHIFT                    10:8
#define POWER_MODE1_CLOCK_MCLK_SHIFT_0                  0
#define POWER_MODE1_CLOCK_MCLK_SHIFT_1                  1
#define POWER_MODE1_CLOCK_MCLK_SHIFT_2                  2
#define POWER_MODE1_CLOCK_MCLK_SHIFT_3                  3
#define POWER_MODE1_CLOCK_MCLK_SHIFT_4                  4
#define POWER_MODE1_CLOCK_MCLK_SHIFT_5                  5
#define POWER_MODE1_CLOCK_MCLK_SHIFT_6                  6
#define POWER_MODE1_CLOCK_MCLK_SHIFT_7                  7
#define POWER_MODE1_CLOCK_M2XCLK_SELECT                 4:4
#define POWER_MODE1_CLOCK_M2XCLK_SELECT_288             0
#define POWER_MODE1_CLOCK_M2XCLK_SELECT_336             1
#define POWER_MODE1_CLOCK_M2XCLK_DIVIDER                3:3
#define POWER_MODE1_CLOCK_M2XCLK_DIVIDER_1              0
#define POWER_MODE1_CLOCK_M2XCLK_DIVIDER_3              1
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT                  2:0
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_0                0
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_1                1
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_2                2
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_3                3
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_4                4
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_5                5
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_6                6
#define POWER_MODE1_CLOCK_M2XCLK_SHIFT_7                7

#define POWER_SLEEP_GATE                                0x000050
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK             22:19
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_4096        0
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_2048        1
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_1024        2
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_512         3
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_256         4
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_128         5
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_64          6
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_32          7
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_16          8
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_8           9
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_4           10
#define POWER_SLEEP_GATE_PLL_RECOVERY_CLOCK_2           11
#define POWER_SLEEP_GATE_PLL_RECOVERY                   14:13
#define POWER_SLEEP_GATE_PLL_RECOVERY_32                0
#define POWER_SLEEP_GATE_PLL_RECOVERY_64                1
#define POWER_SLEEP_GATE_PLL_RECOVERY_96                2
#define POWER_SLEEP_GATE_PLL_RECOVERY_128               3

#define POWER_MODE_CTRL                                 0x000054
#define POWER_MODE_CTRL_SLEEP_STATUS                    2:2
#define POWER_MODE_CTRL_SLEEP_STATUS_INACTIVE           0
#define POWER_MODE_CTRL_SLEEP_STATUS_ACTIVE             1
#define POWER_MODE_CTRL_MODE                            1:0
#define POWER_MODE_CTRL_MODE_MODE0                      0
#define POWER_MODE_CTRL_MODE_MODE1                      1
#define POWER_MODE_CTRL_MODE_SLEEP                      2

#define PCI_MASTER_BASE                                 0x000058
#define PCI_MASTER_BASE_ADDRESS                         31:20

#define ENDIAN_CTRL                                     0x00005C
#define ENDIAN_CTRL_ENDIAN                              0:0
#define ENDIAN_CTRL_ENDIAN_LITTLE                       0
#define ENDIAN_CTRL_ENDIAN_BIG                          1

#define DEVICE_ID                                       0x000060
#define DEVICE_ID_DEVICE_ID                             31:16
#define DEVICE_ID_REVISION_ID                           7:0

#define PLL_CLOCK_COUNT                                 0x000064
#define PLL_CLOCK_COUNT_COUNTER                         15:0

#define SYSTEM_DRAM_CTRL                                0x000068
#define SYSTEM_DRAM_CTRL_READ_DELAY                     2:0
#define SYSTEM_DRAM_CTRL_READ_DELAY_OFF                 0
#define SYSTEM_DRAM_CTRL_READ_DELAY_0_5NS               1
#define SYSTEM_DRAM_CTRL_READ_DELAY_1NS                 2
#define SYSTEM_DRAM_CTRL_READ_DELAY_1_5NS               3
#define SYSTEM_DRAM_CTRL_READ_DELAY_2NS                 4
#define SYSTEM_DRAM_CTRL_READ_DELAY_2_5NS               5

#define SYSTEM_PLL3_CLOCK								0x000074
#define SYSTEM_PLL3_CLOCK_M								7:0
#define SYSTEM_PLL3_CLOCK_N								14:8
#define SYSTEM_PLL3_CLOCK_DIVIDE						15:15
#define SYSTEM_PLL3_CLOCK_DIVIDE_1						0
#define SYSTEM_PLL3_CLOCK_DIVIDE_2						1
#define SYSTEM_PLL3_CLOCK_INPUT							16:16
#define SYSTEM_PLL3_CLOCK_INPUT_CRYSTAL					0
#define SYSTEM_PLL3_CLOCK_INPUT_TEST					1
#define SYSTEM_PLL3_CLOCK_POWER							17:17
#define SYSTEM_PLL3_CLOCK_POWER_OFF						0
#define SYSTEM_PLL3_CLOCK_POWER_ON						1						


#define CURRENT_POWER_PLLCLOCK                    0x000074
#define CURRENT_POWER_PLLCLOCK_TEST_OUTPUT            	20:20
#define CURRENT_POWER_PLLCLOCK_TEST_OUTPUT_ENABLE		1
#define CURRENT_POWER_PLLCLOCK_TEST_OUTPUT_DISABLE      	0
#define CURRENT_POWER_PLLCLOCK_TESTMODE              	19:18
#define CURRENT_POWER_PLLCLOCK_TESTMODE_ENABLE              	1
#define CURRENT_POWER_PLLCLOCK_TESTMODE_DISABLE            	0
#define CURRENT_POWER_PLLCLOCK_POWER              	17:17
#define CURRENT_POWER_PLLCLOCK_POWER_DOWN              	0
#define CURRENT_POWER_PLLCLOCK_POWER_ON              	1
#define CURRENT_POWER_PLLCLOCK_INPUT_SELECT              16:16
#define CURRENT_POWER_PLLCLOCK_INPUT_SELECT_TEST        1
#define CURRENT_POWER_PLLCLOCK_INPUT_SELECT_CRYSTAL    0
#define CURRENT_POWER_PLLCLOCK_DIVIDEBY2                    15:15
#define CURRENT_POWER_PLLCLOCK_DIVIDE_N                     14:8
#define CURRENT_POWER_PLLCLOCK_MULTIPLE_M                     7:0

// Panel Graphics Control

#define PANEL_DISPLAY_CTRL                              0x080000
#define PANEL_DISPLAY_CTRL_FPEN                         27:27
#define PANEL_DISPLAY_CTRL_FPEN_LOW                     0
#define PANEL_DISPLAY_CTRL_FPEN_HIGH                    1
#define PANEL_DISPLAY_CTRL_VBIASEN                      26:26
#define PANEL_DISPLAY_CTRL_VBIASEN_LOW                  0
#define PANEL_DISPLAY_CTRL_VBIASEN_HIGH                 1
#define PANEL_DISPLAY_CTRL_DATA                         25:25
#define PANEL_DISPLAY_CTRL_DATA_DISABLE                 0
#define PANEL_DISPLAY_CTRL_DATA_ENABLE                  1
#define PANEL_DISPLAY_CTRL_FPVDDEN                      24:24
#define PANEL_DISPLAY_CTRL_FPVDDEN_LOW                  0
#define PANEL_DISPLAY_CTRL_FPVDDEN_HIGH                 1
#define PANEL_DISPLAY_CTRL_PATTERN                      23:23
#define PANEL_DISPLAY_CTRL_PATTERN_4                    0
#define PANEL_DISPLAY_CTRL_PATTERN_8                    1
#define PANEL_DISPLAY_CTRL_TFT                          22:21
#define PANEL_DISPLAY_CTRL_TFT_24                       0
#define PANEL_DISPLAY_CTRL_TFT_9                        1
#define PANEL_DISPLAY_CTRL_TFT_12                       2
#define PANEL_DISPLAY_CTRL_DITHER                       20:20
#define PANEL_DISPLAY_CTRL_DITHER_DISABLE               0
#define PANEL_DISPLAY_CTRL_DITHER_ENABLE                1
#define PANEL_DISPLAY_CTRL_LCD                          19:18
#define PANEL_DISPLAY_CTRL_LCD_TFT                      0
#define PANEL_DISPLAY_CTRL_LCD_STN_8                    2
#define PANEL_DISPLAY_CTRL_LCD_STN_12                   3
#define PANEL_DISPLAY_CTRL_FIFO                         17:16
#define PANEL_DISPLAY_CTRL_FIFO_1                       0
#define PANEL_DISPLAY_CTRL_FIFO_3                       1
#define PANEL_DISPLAY_CTRL_FIFO_7                       2
#define PANEL_DISPLAY_CTRL_FIFO_11                      3
#define PANEL_DISPLAY_CTRL_CLOCK_PHASE                  14:14
#define PANEL_DISPLAY_CTRL_CLOCK_PHASE_ACTIVE_HIGH      0
#define PANEL_DISPLAY_CTRL_CLOCK_PHASE_ACTIVE_LOW       1
#define PANEL_DISPLAY_CTRL_VSYNC_PHASE                  13:13
#define PANEL_DISPLAY_CTRL_VSYNC_PHASE_ACTIVE_HIGH      0
#define PANEL_DISPLAY_CTRL_VSYNC_PHASE_ACTIVE_LOW       1
#define PANEL_DISPLAY_CTRL_HSYNC_PHASE                  12:12
#define PANEL_DISPLAY_CTRL_HSYNC_PHASE_ACTIVE_HIGH      0
#define PANEL_DISPLAY_CTRL_HSYNC_PHASE_ACTIVE_LOW       1
#define PANEL_DISPLAY_CTRL_COLOR_KEY                    9:9
#define PANEL_DISPLAY_CTRL_COLOR_KEY_DISABLE            0
#define PANEL_DISPLAY_CTRL_COLOR_KEY_ENABLE             1
#define PANEL_DISPLAY_CTRL_TIMING                       8:8
#define PANEL_DISPLAY_CTRL_TIMING_DISABLE               0
#define PANEL_DISPLAY_CTRL_TIMING_ENABLE                1
#define PANEL_DISPLAY_CTRL_VERTICAL_PAN_DIR             7:7
#define PANEL_DISPLAY_CTRL_VERTICAL_PAN_DIR_DOWN        0
#define PANEL_DISPLAY_CTRL_VERTICAL_PAN_DIR_UP          1
#define PANEL_DISPLAY_CTRL_VERTICAL_PAN                 6:6
#define PANEL_DISPLAY_CTRL_VERTICAL_PAN_DISABLE         0
#define PANEL_DISPLAY_CTRL_VERTICAL_PAN_ENABLE          1
#define PANEL_DISPLAY_CTRL_HORIZONTAL_PAN_DIR           5:5
#define PANEL_DISPLAY_CTRL_HORIZONTAL_PAN_DIR_RIGHT     0
#define PANEL_DISPLAY_CTRL_HORIZONTAL_PAN_DIR_LEFT      1
#define PANEL_DISPLAY_CTRL_HORIZONTAL_PAN               4:4
#define PANEL_DISPLAY_CTRL_HORIZONTAL_PAN_DISABLE       0
#define PANEL_DISPLAY_CTRL_HORIZONTAL_PAN_ENABLE        1
#define PANEL_DISPLAY_CTRL_GAMMA                        3:3
#define PANEL_DISPLAY_CTRL_GAMMA_DISABLE                0
#define PANEL_DISPLAY_CTRL_GAMMA_ENABLE                 1
#define PANEL_DISPLAY_CTRL_PLANE                        2:2
#define PANEL_DISPLAY_CTRL_PLANE_DISABLE                0
#define PANEL_DISPLAY_CTRL_PLANE_ENABLE                 1
#define PANEL_DISPLAY_CTRL_FORMAT                       1:0
#define PANEL_DISPLAY_CTRL_FORMAT_8                     0
#define PANEL_DISPLAY_CTRL_FORMAT_16                    1
#define PANEL_DISPLAY_CTRL_FORMAT_32                    2

#define PANEL_PAN_CTRL                                  0x080004
#define PANEL_PAN_CTRL_VERTICAL_PAN                     31:24
#define PANEL_PAN_CTRL_VERTICAL_VSYNC                   21:16
#define PANEL_PAN_CTRL_HORIZONTAL_PAN                   15:8
#define PANEL_PAN_CTRL_HORIZONTAL_VSYNC                 5:0

#define PANEL_COLOR_KEY                                 0x080008
#define PANEL_COLOR_KEY_MASK                            31:16
#define PANEL_COLOR_KEY_VALUE                           15:0

#define PANEL_FB_ADDRESS                                0x08000C
#define PANEL_FB_ADDRESS_STATUS                         31:31
#define PANEL_FB_ADDRESS_STATUS_CURRENT                 0
#define PANEL_FB_ADDRESS_STATUS_PENDING                 1
#define PANEL_FB_ADDRESS_EXT                            27:27
#define PANEL_FB_ADDRESS_EXT_LOCAL                      0
#define PANEL_FB_ADDRESS_EXT_EXTERNAL                   1
#define PANEL_FB_ADDRESS_CS                             26:26
#define PANEL_FB_ADDRESS_CS_0                           0
#define PANEL_FB_ADDRESS_CS_1                           1
#define PANEL_FB_ADDRESS_ADDRESS                        25:0

#define PANEL_FB_WIDTH                                  0x080010
#define PANEL_FB_WIDTH_WIDTH                            29:16
#define PANEL_FB_WIDTH_OFFSET                           13:0

#define PANEL_WINDOW_WIDTH                              0x080014
#define PANEL_WINDOW_WIDTH_WIDTH                        27:16
#define PANEL_WINDOW_WIDTH_X                            11:0

#define PANEL_WINDOW_HEIGHT                             0x080018
#define PANEL_WINDOW_HEIGHT_HEIGHT                      27:16
#define PANEL_WINDOW_HEIGHT_Y                           11:0

#define PANEL_PLANE_TL                                  0x08001C
#define PANEL_PLANE_TL_TOP                              26:16
#define PANEL_PLANE_TL_LEFT                             10:0

#define PANEL_PLANE_BR                                  0x080020
#define PANEL_PLANE_BR_BOTTOM                           26:16
#define PANEL_PLANE_BR_RIGHT                            10:0

#define PANEL_HORIZONTAL_TOTAL                          0x080024
#define PANEL_HORIZONTAL_TOTAL_TOTAL                    27:16
#define PANEL_HORIZONTAL_TOTAL_DISPLAY_END              11:0

#define PANEL_HORIZONTAL_SYNC                           0x080028
#define PANEL_HORIZONTAL_SYNC_WIDTH                     23:16
#define PANEL_HORIZONTAL_SYNC_START                     11:0

#define PANEL_VERTICAL_TOTAL                            0x08002C
#define PANEL_VERTICAL_TOTAL_TOTAL                      26:16
#define PANEL_VERTICAL_TOTAL_DISPLAY_END                10:0

#define PANEL_VERTICAL_SYNC                             0x080030
#define PANEL_VERTICAL_SYNC_HEIGHT                      21:16
#define PANEL_VERTICAL_SYNC_START                       10:0

#define PANEL_CURRENT_LINE                              0x080034
#define PANEL_CURRENT_LINE_LINE                         10:0

// Video Control

#define VIDEO_DISPLAY_CTRL                              0x080040
#define VIDEO_DISPLAY_CTRL_FIFO                         17:16
#define VIDEO_DISPLAY_CTRL_FIFO_1                       0
#define VIDEO_DISPLAY_CTRL_FIFO_3                       1
#define VIDEO_DISPLAY_CTRL_FIFO_7                       2
#define VIDEO_DISPLAY_CTRL_FIFO_11                      3
#define VIDEO_DISPLAY_CTRL_BUFFER                       15:15
#define VIDEO_DISPLAY_CTRL_BUFFER_0                     0
#define VIDEO_DISPLAY_CTRL_BUFFER_1                     1
#define VIDEO_DISPLAY_CTRL_CAPTURE                      14:14
#define VIDEO_DISPLAY_CTRL_CAPTURE_DISABLE              0
#define VIDEO_DISPLAY_CTRL_CAPTURE_ENABLE               1
#define VIDEO_DISPLAY_CTRL_DOUBLE_BUFFER                13:13
#define VIDEO_DISPLAY_CTRL_DOUBLE_BUFFER_DISABLE        0
#define VIDEO_DISPLAY_CTRL_DOUBLE_BUFFER_ENABLE         1
#define VIDEO_DISPLAY_CTRL_BYTE_SWAP                    12:12
#define VIDEO_DISPLAY_CTRL_BYTE_SWAP_DISABLE            0
#define VIDEO_DISPLAY_CTRL_BYTE_SWAP_ENABLE             1
#define VIDEO_DISPLAY_CTRL_VERTICAL_SCALE               11:11
#define VIDEO_DISPLAY_CTRL_VERTICAL_SCALE_NORMAL        0
#define VIDEO_DISPLAY_CTRL_VERTICAL_SCALE_HALF          1
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_SCALE             10:10
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_SCALE_NORMAL      0
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_SCALE_HALF        1
#define VIDEO_DISPLAY_CTRL_VERTICAL_MODE                9:9
#define VIDEO_DISPLAY_CTRL_VERTICAL_MODE_REPLICATE      0
#define VIDEO_DISPLAY_CTRL_VERTICAL_MODE_INTERPOLATE    1
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_MODE              8:8
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_MODE_REPLICATE    0
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_MODE_INTERPOLATE  1
#define VIDEO_DISPLAY_CTRL_PIXEL                        7:4
#define VIDEO_DISPLAY_CTRL_GAMMA                        3:3
#define VIDEO_DISPLAY_CTRL_GAMMA_DISABLE                0
#define VIDEO_DISPLAY_CTRL_GAMMA_ENABLE                 1
#define VIDEO_DISPLAY_CTRL_PLANE                        2:2
#define VIDEO_DISPLAY_CTRL_PLANE_DISABLE                0
#define VIDEO_DISPLAY_CTRL_PLANE_ENABLE                 1
#define VIDEO_DISPLAY_CTRL_FORMAT                       1:0
#define VIDEO_DISPLAY_CTRL_FORMAT_8                     0
#define VIDEO_DISPLAY_CTRL_FORMAT_16                    1
#define VIDEO_DISPLAY_CTRL_FORMAT_32                    2
#define VIDEO_DISPLAY_CTRL_FORMAT_YUV                   3

#define VIDEO_FB_0_ADDRESS                              0x080044
#define VIDEO_FB_0_ADDRESS_STATUS                       31:31
#define VIDEO_FB_0_ADDRESS_STATUS_CURRENT               0
#define VIDEO_FB_0_ADDRESS_STATUS_PENDING               1
#define VIDEO_FB_0_ADDRESS_EXT                          27:27
#define VIDEO_FB_0_ADDRESS_EXT_LOCAL                    0
#define VIDEO_FB_0_ADDRESS_EXT_EXTERNAL                 1
#define VIDEO_FB_0_ADDRESS_CS                           26:26
#define VIDEO_FB_0_ADDRESS_CS_0                         0
#define VIDEO_FB_0_ADDRESS_CS_1                         1
#define VIDEO_FB_0_ADDRESS_ADDRESS                      25:0

#define VIDEO_FB_WIDTH                                  0x080048
#define VIDEO_FB_WIDTH_WIDTH                            29:16
#define VIDEO_FB_WIDTH_OFFSET                           13:0

#define VIDEO_FB_0_LAST_ADDRESS                         0x08004C
#define VIDEO_FB_0_LAST_ADDRESS_EXT                     27:27
#define VIDEO_FB_0_LAST_ADDRESS_EXT_LOCAL               0
#define VIDEO_FB_0_LAST_ADDRESS_EXT_EXTERNAL            1
#define VIDEO_FB_0_LAST_ADDRESS_CS                      26:26
#define VIDEO_FB_0_LAST_ADDRESS_CS_0                    0
#define VIDEO_FB_0_LAST_ADDRESS_CS_1                    1
#define VIDEO_FB_0_LAST_ADDRESS_ADDRESS                 25:0

#define VIDEO_PLANE_TL                                  0x080050
#define VIDEO_PLANE_TL_TOP                              26:16
#define VIDEO_PLANE_TL_LEFT                             13:0

#define VIDEO_PLANE_BR                                  0x080054
#define VIDEO_PLANE_BR_BOTTOM                           26:16
#define VIDEO_PLANE_BR_RIGHT                            13:0

#define VIDEO_SCALE                                     0x080058
#define VIDEO_SCALE_VERTICAL_MODE                       31:31
#define VIDEO_SCALE_VERTICAL_MODE_EXPAND                0
#define VIDEO_SCALE_VERTICAL_MODE_SHRINK                1
#define VIDEO_SCALE_VERTICAL_SCALE                      27:16
#define VIDEO_SCALE_HORIZONTAL_MODE                     15:15
#define VIDEO_SCALE_HORIZONTAL_MODE_EXPAND              0
#define VIDEO_SCALE_HORIZONTAL_MODE_SHRINK              1
#define VIDEO_SCALE_HORIZONTAL_SCALE                    11:0

#define VIDEO_INITIAL_SCALE                             0x08005C
#define VIDEO_INITIAL_SCALE_FB_1                        27:16
#define VIDEO_INITIAL_SCALE_FB_0                        11:0

#define VIDEO_YUV_CONSTANTS                             0x080060
#define VIDEO_YUV_CONSTANTS_Y                           31:24
#define VIDEO_YUV_CONSTANTS_R                           23:16
#define VIDEO_YUV_CONSTANTS_G                           15:8
#define VIDEO_YUV_CONSTANTS_B                           7:0

#define VIDEO_FB_1_ADDRESS                              0x080064
#define VIDEO_FB_1_ADDRESS_STATUS                       31:31
#define VIDEO_FB_1_ADDRESS_STATUS_CURRENT               0
#define VIDEO_FB_1_ADDRESS_STATUS_PENDING               1
#define VIDEO_FB_1_ADDRESS_EXT                          27:27
#define VIDEO_FB_1_ADDRESS_EXT_LOCAL                    0
#define VIDEO_FB_1_ADDRESS_EXT_EXTERNAL                 1
#define VIDEO_FB_1_ADDRESS_CS                           26:26
#define VIDEO_FB_1_ADDRESS_CS_0                         0
#define VIDEO_FB_1_ADDRESS_CS_1                         1
#define VIDEO_FB_1_ADDRESS_ADDRESS                      25:0

#define VIDEO_FB_1_LAST_ADDRESS                         0x080068
#define VIDEO_FB_1_LAST_ADDRESS_EXT                     27:27
#define VIDEO_FB_1_LAST_ADDRESS_EXT_LOCAL               0
#define VIDEO_FB_1_LAST_ADDRESS_EXT_EXTERNAL            1
#define VIDEO_FB_1_LAST_ADDRESS_CS                      26:26
#define VIDEO_FB_1_LAST_ADDRESS_CS_0                    0
#define VIDEO_FB_1_LAST_ADDRESS_CS_1                    1
#define VIDEO_FB_1_LAST_ADDRESS_ADDRESS                 25:0

// Video Alpha Control

#define VIDEO_ALPHA_DISPLAY_CTRL                        0x080080
#define VIDEO_ALPHA_DISPLAY_CTRL_SELECT                 28:28
#define VIDEO_ALPHA_DISPLAY_CTRL_SELECT_PER_PIXEL       0
#define VIDEO_ALPHA_DISPLAY_CTRL_SELECT_ALPHA           1
#define VIDEO_ALPHA_DISPLAY_CTRL_ALPHA                  27:24
#define VIDEO_ALPHA_DISPLAY_CTRL_FIFO                   17:16
#define VIDEO_ALPHA_DISPLAY_CTRL_FIFO_1                 0
#define VIDEO_ALPHA_DISPLAY_CTRL_FIFO_3                 1
#define VIDEO_ALPHA_DISPLAY_CTRL_FIFO_7                 2
#define VIDEO_ALPHA_DISPLAY_CTRL_FIFO_11                3
#define VIDEO_ALPHA_DISPLAY_CTRL_VERT_SCALE             11:11
#define VIDEO_ALPHA_DISPLAY_CTRL_VERT_SCALE_NORMAL      0
#define VIDEO_ALPHA_DISPLAY_CTRL_VERT_SCALE_HALF        1
#define VIDEO_ALPHA_DISPLAY_CTRL_HORZ_SCALE             10:10
#define VIDEO_ALPHA_DISPLAY_CTRL_HORZ_SCALE_NORMAL      0
#define VIDEO_ALPHA_DISPLAY_CTRL_HORZ_SCALE_HALF        1
#define VIDEO_ALPHA_DISPLAY_CTRL_VERT_MODE              9:9
#define VIDEO_ALPHA_DISPLAY_CTRL_VERT_MODE_REPLICATE    0
#define VIDEO_ALPHA_DISPLAY_CTRL_VERT_MODE_INTERPOLATE  1
#define VIDEO_ALPHA_DISPLAY_CTRL_HORZ_MODE              8:8
#define VIDEO_ALPHA_DISPLAY_CTRL_HORZ_MODE_REPLICATE    0
#define VIDEO_ALPHA_DISPLAY_CTRL_HORZ_MODE_INTERPOLATE  1
#define VIDEO_ALPHA_DISPLAY_CTRL_PIXEL                  7:4
#define VIDEO_ALPHA_DISPLAY_CTRL_CHROMA_KEY             3:3
#define VIDEO_ALPHA_DISPLAY_CTRL_CHROMA_KEY_DISABLE     0
#define VIDEO_ALPHA_DISPLAY_CTRL_CHROMA_KEY_ENABLE      1
#define VIDEO_ALPHA_DISPLAY_CTRL_PLANE                  2:2
#define VIDEO_ALPHA_DISPLAY_CTRL_PLANE_DISABLE          0
#define VIDEO_ALPHA_DISPLAY_CTRL_PLANE_ENABLE           1
#define VIDEO_ALPHA_DISPLAY_CTRL_FORMAT                 1:0
#define VIDEO_ALPHA_DISPLAY_CTRL_FORMAT_8               0
#define VIDEO_ALPHA_DISPLAY_CTRL_FORMAT_16              1
#define VIDEO_ALPHA_DISPLAY_CTRL_FORMAT_ALPHA_4_4       2
#define VIDEO_ALPHA_DISPLAY_CTRL_FORMAT_ALPHA_4_4_4_4   3

#define VIDEO_ALPHA_FB_ADDRESS                          0x080084
#define VIDEO_ALPHA_FB_ADDRESS_STATUS                   31:31
#define VIDEO_ALPHA_FB_ADDRESS_STATUS_CURRENT           0
#define VIDEO_ALPHA_FB_ADDRESS_STATUS_PENDING           1
#define VIDEO_ALPHA_FB_ADDRESS_EXT                      27:27
#define VIDEO_ALPHA_FB_ADDRESS_EXT_LOCAL                0
#define VIDEO_ALPHA_FB_ADDRESS_EXT_EXTERNAL             1
#define VIDEO_ALPHA_FB_ADDRESS_CS                       26:26
#define VIDEO_ALPHA_FB_ADDRESS_CS_0                     0
#define VIDEO_ALPHA_FB_ADDRESS_CS_1                     1
#define VIDEO_ALPHA_FB_ADDRESS_ADDRESS                  25:0

#define VIDEO_ALPHA_FB_WIDTH                            0x080088
#define VIDEO_ALPHA_FB_WIDTH_WIDTH                      29:16
#define VIDEO_ALPHA_FB_WIDTH_OFFSET                     13:0

#define VIDEO_ALPHA_FB_LAST_ADDRESS                     0x08008C
#define VIDEO_ALPHA_FB_LAST_ADDRESS_EXT                 27:27
#define VIDEO_ALPHA_FB_LAST_ADDRESS_EXT_LOCAL           0
#define VIDEO_ALPHA_FB_LAST_ADDRESS_EXT_EXTERNAL        1
#define VIDEO_ALPHA_FB_LAST_ADDRESS_CS                  26:26
#define VIDEO_ALPHA_FB_LAST_ADDRESS_CS_0                0
#define VIDEO_ALPHA_FB_LAST_ADDRESS_CS_1                1
#define VIDEO_ALPHA_FB_LAST_ADDRESS_ADDRESS             25:0

#define VIDEO_ALPHA_PLANE_TL                            0x080090
#define VIDEO_ALPHA_PLANE_TL_TOP                        26:16
#define VIDEO_ALPHA_PLANE_TL_LEFT                       10:0

#define VIDEO_ALPHA_PLANE_BR                            0x080094
#define VIDEO_ALPHA_PLANE_BR_BOTTOM                     26:16
#define VIDEO_ALPHA_PLANE_BR_RIGHT                      10:0

#define VIDEO_ALPHA_SCALE                               0x080098
#define VIDEO_ALPHA_SCALE_VERTICAL_MODE                 31:31
#define VIDEO_ALPHA_SCALE_VERTICAL_MODE_EXPAND          0
#define VIDEO_ALPHA_SCALE_VERTICAL_MODE_SHRINK          1
#define VIDEO_ALPHA_SCALE_VERTICAL_SCALE                27:16
#define VIDEO_ALPHA_SCALE_HORIZONTAL_MODE               15:15
#define VIDEO_ALPHA_SCALE_HORIZONTAL_MODE_EXPAND        0
#define VIDEO_ALPHA_SCALE_HORIZONTAL_MODE_SHRINK        1
#define VIDEO_ALPHA_SCALE_HORIZONTAL_SCALE              11:0

#define VIDEO_ALPHA_INITIAL_SCALE                       0x08009C
#define VIDEO_ALPHA_INITIAL_SCALE_FB                    11:0

#define VIDEO_ALPHA_CHROMA_KEY                          0x0800A0
#define VIDEO_ALPHA_CHROMA_KEY_MASK                     31:16
#define VIDEO_ALPHA_CHROMA_KEY_VALUE                    15:0

#define VIDEO_ALPHA_COLOR_LOOKUP_01                     0x0800A4
#define VIDEO_ALPHA_COLOR_LOOKUP_01_1_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_01_1_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_01_1_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_01_0_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_01_0_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_01_0_BLUE              4:0

#define VIDEO_ALPHA_COLOR_LOOKUP_23                     0x0800A8
#define VIDEO_ALPHA_COLOR_LOOKUP_23_3_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_23_3_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_23_3_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_23_2_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_23_2_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_23_2_BLUE              4:0

#define VIDEO_ALPHA_COLOR_LOOKUP_45                     0x0800AC
#define VIDEO_ALPHA_COLOR_LOOKUP_45_5_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_45_5_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_45_5_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_45_4_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_45_4_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_45_4_BLUE              4:0

#define VIDEO_ALPHA_COLOR_LOOKUP_67                     0x0800B0
#define VIDEO_ALPHA_COLOR_LOOKUP_67_7_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_67_7_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_67_7_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_67_6_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_67_6_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_67_6_BLUE              4:0

#define VIDEO_ALPHA_COLOR_LOOKUP_89                     0x0800B4
#define VIDEO_ALPHA_COLOR_LOOKUP_89_9_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_89_9_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_89_9_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_89_8_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_89_8_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_89_8_BLUE              4:0

#define VIDEO_ALPHA_COLOR_LOOKUP_AB                     0x0800B8
#define VIDEO_ALPHA_COLOR_LOOKUP_AB_B_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_AB_B_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_AB_B_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_AB_A_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_AB_A_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_AB_A_BLUE              4:0

#define VIDEO_ALPHA_COLOR_LOOKUP_CD                     0x0800BC
#define VIDEO_ALPHA_COLOR_LOOKUP_CD_D_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_CD_D_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_CD_D_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_CD_C_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_CD_C_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_CD_C_BLUE              4:0

#define VIDEO_ALPHA_COLOR_LOOKUP_EF                     0x0800C0
#define VIDEO_ALPHA_COLOR_LOOKUP_EF_F_RED               31:27
#define VIDEO_ALPHA_COLOR_LOOKUP_EF_F_GREEN             26:21
#define VIDEO_ALPHA_COLOR_LOOKUP_EF_F_BLUE              20:16
#define VIDEO_ALPHA_COLOR_LOOKUP_EF_E_RED               15:11
#define VIDEO_ALPHA_COLOR_LOOKUP_EF_E_GREEN             10:5
#define VIDEO_ALPHA_COLOR_LOOKUP_EF_E_BLUE              4:0

// Panel Cursor Control

#define PANEL_HWC_ADDRESS                               0x0800F0
#define PANEL_HWC_ADDRESS_ENABLE                        31:31
#define PANEL_HWC_ADDRESS_ENABLE_DISABLE                0
#define PANEL_HWC_ADDRESS_ENABLE_ENABLE                 1
#define PANEL_HWC_ADDRESS_EXT                           27:27
#define PANEL_HWC_ADDRESS_EXT_LOCAL                     0
#define PANEL_HWC_ADDRESS_EXT_EXTERNAL                  1
#define PANEL_HWC_ADDRESS_CS                            26:26
#define PANEL_HWC_ADDRESS_CS_0                          0
#define PANEL_HWC_ADDRESS_CS_1                          1
#define PANEL_HWC_ADDRESS_ADDRESS                       25:0

#define PANEL_HWC_LOCATION                              0x0800F4
#define PANEL_HWC_LOCATION_TOP                          27:27
#define PANEL_HWC_LOCATION_TOP_INSIDE                   0
#define PANEL_HWC_LOCATION_TOP_OUTSIDE                  1
#define PANEL_HWC_LOCATION_Y                            26:16
#define PANEL_HWC_LOCATION_LEFT                         11:11
#define PANEL_HWC_LOCATION_LEFT_INSIDE                  0
#define PANEL_HWC_LOCATION_LEFT_OUTSIDE                 1
#define PANEL_HWC_LOCATION_X                            10:0

#define PANEL_HWC_COLOR_12                              0x0800F8
#define PANEL_HWC_COLOR_12_2_RGB565                     31:16
#define PANEL_HWC_COLOR_12_1_RGB565                     15:0

#define PANEL_HWC_COLOR_3                               0x0800FC
#define PANEL_HWC_COLOR_3_RGB565                        15:0

// Old Definitions +++
#define PANEL_HWC_COLOR_01                              0x0800F8
#define PANEL_HWC_COLOR_01_1_RED                        31:27
#define PANEL_HWC_COLOR_01_1_GREEN                      26:21
#define PANEL_HWC_COLOR_01_1_BLUE                       20:16
#define PANEL_HWC_COLOR_01_0_RED                        15:11
#define PANEL_HWC_COLOR_01_0_GREEN                      10:5
#define PANEL_HWC_COLOR_01_0_BLUE                       4:0

#define PANEL_HWC_COLOR_2                               0x0800FC
#define PANEL_HWC_COLOR_2_RED                           15:11
#define PANEL_HWC_COLOR_2_GREEN                         10:5
#define PANEL_HWC_COLOR_2_BLUE                          4:0
// Old Definitions ---

// Alpha Control

#define ALPHA_DISPLAY_CTRL                              0x080100
#define ALPHA_DISPLAY_CTRL_SELECT                       28:28
#define ALPHA_DISPLAY_CTRL_SELECT_PER_PIXEL             0
#define ALPHA_DISPLAY_CTRL_SELECT_ALPHA                 1
#define ALPHA_DISPLAY_CTRL_ALPHA                        27:24
#define ALPHA_DISPLAY_CTRL_FIFO                         17:16
#define ALPHA_DISPLAY_CTRL_FIFO_1                       0
#define ALPHA_DISPLAY_CTRL_FIFO_3                       1
#define ALPHA_DISPLAY_CTRL_FIFO_7                       2
#define ALPHA_DISPLAY_CTRL_FIFO_11                      3
#define ALPHA_DISPLAY_CTRL_PIXEL                        7:4
#define ALPHA_DISPLAY_CTRL_CHROMA_KEY                   3:3
#define ALPHA_DISPLAY_CTRL_CHROMA_KEY_DISABLE           0
#define ALPHA_DISPLAY_CTRL_CHROMA_KEY_ENABLE            1
#define ALPHA_DISPLAY_CTRL_PLANE                        2:2
#define ALPHA_DISPLAY_CTRL_PLANE_DISABLE                0
#define ALPHA_DISPLAY_CTRL_PLANE_ENABLE                 1
#define ALPHA_DISPLAY_CTRL_FORMAT                       1:0
#define ALPHA_DISPLAY_CTRL_FORMAT_16                    1
#define ALPHA_DISPLAY_CTRL_FORMAT_ALPHA_4_4             2
#define ALPHA_DISPLAY_CTRL_FORMAT_ALPHA_4_4_4_4         3

#define ALPHA_FB_ADDRESS                                0x080104
#define ALPHA_FB_ADDRESS_STATUS                         31:31
#define ALPHA_FB_ADDRESS_STATUS_CURRENT                 0
#define ALPHA_FB_ADDRESS_STATUS_PENDING                 1
#define ALPHA_FB_ADDRESS_EXT                            27:27
#define ALPHA_FB_ADDRESS_EXT_LOCAL                      0
#define ALPHA_FB_ADDRESS_EXT_EXTERNAL                   1
#define ALPHA_FB_ADDRESS_CS                             26:26
#define ALPHA_FB_ADDRESS_CS_0                           0
#define ALPHA_FB_ADDRESS_CS_1                           1
#define ALPHA_FB_ADDRESS_ADDRESS                        25:0

#define ALPHA_FB_WIDTH                                  0x080108
#define ALPHA_FB_WIDTH_WIDTH                            29:16
#define ALPHA_FB_WIDTH_OFFSET                           13:0

#define ALPHA_PLANE_TL                                  0x08010C
#define ALPHA_PLANE_TL_TOP                              26:16
#define ALPHA_PLANE_TL_LEFT                             10:0

#define ALPHA_PLANE_BR                                  0x080110
#define ALPHA_PLANE_BR_BOTTOM                           26:16
#define ALPHA_PLANE_BR_RIGHT                            10:0

#define ALPHA_CHROMA_KEY                                0x080114
#define ALPHA_CHROMA_KEY_MASK                           31:16
#define ALPHA_CHROMA_KEY_VALUE                          15:0

#define ALPHA_COLOR_LOOKUP_01                           0x080118
#define ALPHA_COLOR_LOOKUP_01_1_RED                     31:27
#define ALPHA_COLOR_LOOKUP_01_1_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_01_1_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_01_0_RED                     15:11
#define ALPHA_COLOR_LOOKUP_01_0_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_01_0_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_23                           0x08011C
#define ALPHA_COLOR_LOOKUP_23_3_RED                     31:27
#define ALPHA_COLOR_LOOKUP_23_3_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_23_3_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_23_2_RED                     15:11
#define ALPHA_COLOR_LOOKUP_23_2_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_23_2_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_45                           0x080120
#define ALPHA_COLOR_LOOKUP_45_5_RED                     31:27
#define ALPHA_COLOR_LOOKUP_45_5_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_45_5_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_45_4_RED                     15:11
#define ALPHA_COLOR_LOOKUP_45_4_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_45_4_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_67                           0x080124
#define ALPHA_COLOR_LOOKUP_67_7_RED                     31:27
#define ALPHA_COLOR_LOOKUP_67_7_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_67_7_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_67_6_RED                     15:11
#define ALPHA_COLOR_LOOKUP_67_6_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_67_6_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_89                           0x080128
#define ALPHA_COLOR_LOOKUP_89_9_RED                     31:27
#define ALPHA_COLOR_LOOKUP_89_9_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_89_9_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_89_8_RED                     15:11
#define ALPHA_COLOR_LOOKUP_89_8_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_89_8_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_AB                           0x08012C
#define ALPHA_COLOR_LOOKUP_AB_B_RED                     31:27
#define ALPHA_COLOR_LOOKUP_AB_B_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_AB_B_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_AB_A_RED                     15:11
#define ALPHA_COLOR_LOOKUP_AB_A_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_AB_A_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_CD                           0x080130
#define ALPHA_COLOR_LOOKUP_CD_D_RED                     31:27
#define ALPHA_COLOR_LOOKUP_CD_D_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_CD_D_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_CD_C_RED                     15:11
#define ALPHA_COLOR_LOOKUP_CD_C_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_CD_C_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_EF                           0x080134
#define ALPHA_COLOR_LOOKUP_EF_F_RED                     31:27
#define ALPHA_COLOR_LOOKUP_EF_F_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_EF_F_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_EF_E_RED                     15:11
#define ALPHA_COLOR_LOOKUP_EF_E_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_EF_E_BLUE                    4:0

// CRT Graphics Control

#define CRT_DISPLAY_CTRL                                0x080200
#define CRT_DISPLAY_CTRL_FIFO                           17:16
#define CRT_DISPLAY_CTRL_FIFO_1                         0
#define CRT_DISPLAY_CTRL_FIFO_3                         1
#define CRT_DISPLAY_CTRL_FIFO_7                         2
#define CRT_DISPLAY_CTRL_FIFO_11                        3
#define CRT_DISPLAY_CTRL_TV_PHASE                       15:15
#define CRT_DISPLAY_CTRL_TV_PHASE_ACTIVE_HIGH           0
#define CRT_DISPLAY_CTRL_TV_PHASE_ACTIVE_LOW            1
#define CRT_DISPLAY_CTRL_CLOCK_PHASE                    14:14
#define CRT_DISPLAY_CTRL_CLOCK_PHASE_ACTIVE_HIGH        0
#define CRT_DISPLAY_CTRL_CLOCK_PHASE_ACTIVE_LOW         1
#define CRT_DISPLAY_CTRL_VSYNC_PHASE                    13:13
#define CRT_DISPLAY_CTRL_VSYNC_PHASE_ACTIVE_HIGH        0
#define CRT_DISPLAY_CTRL_VSYNC_PHASE_ACTIVE_LOW         1
#define CRT_DISPLAY_CTRL_HSYNC_PHASE                    12:12
#define CRT_DISPLAY_CTRL_HSYNC_PHASE_ACTIVE_HIGH        0
#define CRT_DISPLAY_CTRL_HSYNC_PHASE_ACTIVE_LOW         1
#define CRT_DISPLAY_CTRL_BLANK                          10:10
#define CRT_DISPLAY_CTRL_BLANK_OFF                      0
#define CRT_DISPLAY_CTRL_BLANK_ON                       1
#define CRT_DISPLAY_CTRL_SELECT                         9:9
#define CRT_DISPLAY_CTRL_SELECT_PANEL                   0
#define CRT_DISPLAY_CTRL_SELECT_CRT                     1
#define CRT_DISPLAY_CTRL_TIMING                         8:8
#define CRT_DISPLAY_CTRL_TIMING_DISABLE                 0
#define CRT_DISPLAY_CTRL_TIMING_ENABLE                  1
#define CRT_DISPLAY_CTRL_PIXEL                          7:4
#define CRT_DISPLAY_CTRL_GAMMA                          3:3
#define CRT_DISPLAY_CTRL_GAMMA_DISABLE                  0
#define CRT_DISPLAY_CTRL_GAMMA_ENABLE                   1
#define CRT_DISPLAY_CTRL_PLANE                          2:2
#define CRT_DISPLAY_CTRL_PLANE_DISABLE                  0
#define CRT_DISPLAY_CTRL_PLANE_ENABLE                   1
#define CRT_DISPLAY_CTRL_FORMAT                         1:0
#define CRT_DISPLAY_CTRL_FORMAT_8                       0
#define CRT_DISPLAY_CTRL_FORMAT_16                      1
#define CRT_DISPLAY_CTRL_FORMAT_32                      2

#define CRT_FB_ADDRESS                                  0x080204
#define CRT_FB_ADDRESS_STATUS                           31:31
#define CRT_FB_ADDRESS_STATUS_CURRENT                   0
#define CRT_FB_ADDRESS_STATUS_PENDING                   1
#define CRT_FB_ADDRESS_EXT                              27:27
#define CRT_FB_ADDRESS_EXT_LOCAL                        0
#define CRT_FB_ADDRESS_EXT_EXTERNAL                     1
#define CRT_FB_ADDRESS_CS                               26:26
#define CRT_FB_ADDRESS_CS_0                             0
#define CRT_FB_ADDRESS_CS_1                             1
#define CRT_FB_ADDRESS_ADDRESS                          25:0

#define CRT_FB_WIDTH                                    0x080208
#define CRT_FB_WIDTH_WIDTH                              29:16
#define CRT_FB_WIDTH_OFFSET                             13:0

#define CRT_HORIZONTAL_TOTAL                            0x08020C
#define CRT_HORIZONTAL_TOTAL_TOTAL                      27:16
#define CRT_HORIZONTAL_TOTAL_DISPLAY_END                11:0

#define CRT_HORIZONTAL_SYNC                             0x080210
#define CRT_HORIZONTAL_SYNC_WIDTH                       23:16
#define CRT_HORIZONTAL_SYNC_START                       11:0

#define CRT_VERTICAL_TOTAL                              0x080214
#define CRT_VERTICAL_TOTAL_TOTAL                        26:16
#define CRT_VERTICAL_TOTAL_DISPLAY_END                  10:0

#define CRT_VERTICAL_SYNC                               0x080218
#define CRT_VERTICAL_SYNC_HEIGHT                        21:16
#define CRT_VERTICAL_SYNC_START                         10:0

#define CRT_SIGNATURE_ANALYZER                          0x08021C
#define CRT_SIGNATURE_ANALYZER_STATUS                   31:16
#define CRT_SIGNATURE_ANALYZER_ENABLE                   3:3
#define CRT_SIGNATURE_ANALYZER_ENABLE_DISABLE           0
#define CRT_SIGNATURE_ANALYZER_ENABLE_ENABLE            1
#define CRT_SIGNATURE_ANALYZER_RESET                    2:2
#define CRT_SIGNATURE_ANALYZER_RESET_NORMAL             0
#define CRT_SIGNATURE_ANALYZER_RESET_RESET              1
#define CRT_SIGNATURE_ANALYZER_SOURCE                   1:0
#define CRT_SIGNATURE_ANALYZER_SOURCE_RED               0
#define CRT_SIGNATURE_ANALYZER_SOURCE_GREEN             1
#define CRT_SIGNATURE_ANALYZER_SOURCE_BLUE              2

#define CRT_CURRENT_LINE                                0x080220
#define CRT_CURRENT_LINE_LINE                           10:0

#define CRT_MONITOR_DETECT                              0x080224
#define CRT_MONITOR_DETECT_ENABLE                       24:24
#define CRT_MONITOR_DETECT_ENABLE_DISABLE               0
#define CRT_MONITOR_DETECT_ENABLE_ENABLE                1
#define CRT_MONITOR_DETECT_RED                          23:16
#define CRT_MONITOR_DETECT_GREEN                        15:8
#define CRT_MONITOR_DETECT_BLUE                         7:0

// CRT Cursor Control

#define CRT_HWC_ADDRESS                                 0x080230
#define CRT_HWC_ADDRESS_ENABLE                          31:31
#define CRT_HWC_ADDRESS_ENABLE_DISABLE                  0
#define CRT_HWC_ADDRESS_ENABLE_ENABLE                   1
#define CRT_HWC_ADDRESS_EXT                             27:27
#define CRT_HWC_ADDRESS_EXT_LOCAL                       0
#define CRT_HWC_ADDRESS_EXT_EXTERNAL                    1
#define CRT_HWC_ADDRESS_CS                              26:26
#define CRT_HWC_ADDRESS_CS_0                            0
#define CRT_HWC_ADDRESS_CS_1                            1
#define CRT_HWC_ADDRESS_ADDRESS                         25:0

#define CRT_HWC_LOCATION                                0x080234
#define CRT_HWC_LOCATION_TOP                            27:27
#define CRT_HWC_LOCATION_TOP_INSIDE                     0
#define CRT_HWC_LOCATION_TOP_OUTSIDE                    1
#define CRT_HWC_LOCATION_Y                              26:16
#define CRT_HWC_LOCATION_LEFT                           11:11
#define CRT_HWC_LOCATION_LEFT_INSIDE                    0
#define CRT_HWC_LOCATION_LEFT_OUTSIDE                   1
#define CRT_HWC_LOCATION_X                              10:0

#define CRT_HWC_COLOR_12                                0x080238
#define CRT_HWC_COLOR_12_2_RGB565                       31:16
#define CRT_HWC_COLOR_12_1_RGB565                       15:0

#define CRT_HWC_COLOR_3                                 0x08023C
#define CRT_HWC_COLOR_3_RGB565                          15:0

// Old Definitions +++
#define CRT_HWC_COLOR_01                                0x080238
#define CRT_HWC_COLOR_01_1_RED                          31:27
#define CRT_HWC_COLOR_01_1_GREEN                        26:21
#define CRT_HWC_COLOR_01_1_BLUE                         20:16
#define CRT_HWC_COLOR_01_0_RED                          15:11
#define CRT_HWC_COLOR_01_0_GREEN                        10:5
#define CRT_HWC_COLOR_01_0_BLUE                         4:0

#define CRT_HWC_COLOR_2                                 0x08023C
#define CRT_HWC_COLOR_2_RED                             15:11
#define CRT_HWC_COLOR_2_GREEN                           10:5
#define CRT_HWC_COLOR_2_BLUE                            4:0
// Old Definitions ---

// Palette RAM

#define PANEL_PALETTE_RAM                               0x080400
#define VIDEO_PALETTE_RAM                               0x080800
#define CRT_PALETTE_RAM                                 0x080C00

// Power constants to use with setDPMS function.
typedef enum _DPMS_t
{
	DPMS_ON,
	DPMS_STANDBY,
	DPMS_SUSPEND,
	DPMS_OFF
}
DPMS_t;

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// D I S P L A Y   C O N T R O L L E R                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// Display type constants to use with setMode function and others.
typedef enum _display_t
{
	PANEL = 0,
	CRT = 1,
}
display_t;

// Type of LCD display
typedef enum _lcd_display_t
{
    LCD_TFT = 0,
    LCD_STN_8 = 2,
	LCD_STN_12 = 3
}
lcd_display_t;

// Polarity constants.
typedef enum _polarity_t
{
	POSITIVE,
	NEGATIVE,
}
polarity_t;


// Format of mode table record.
typedef struct _mode_table_t
{
	// Horizontal timing.
	int horizontal_total;
	int horizontal_display_end;
	int horizontal_sync_start;
	int horizontal_sync_width;
	polarity_t horizontal_sync_polarity;

	// Vertical timing.
	int vertical_total;
	int vertical_display_end;
	int vertical_sync_start;
	int vertical_sync_height;
	polarity_t vertical_sync_polarity;

	// Refresh timing.
	long pixel_clock;
	long horizontal_frequency;
	long vertical_frequency;

    //Programe PLL3 
	int M;
	int N;
	int bit15;
	int bit31;

}
mode_table_t, *pmode_table_t;

// Clock value structure.
typedef struct clock_select_t
{
	long mclk;
	long test_clock;
	int divider;
	int shift;

	long multipleM;
	int dividerN;
	short divby2;
}
clock_select_t, *pclock_select_t;

// Registers necessary to set mode.
typedef struct _reg_table_t
{
	unsigned long clock;
	unsigned long control;
	unsigned long fb_width;
	unsigned long horizontal_total;
	unsigned long horizontal_sync;
	unsigned long vertical_total;
	unsigned long vertical_sync;
	unsigned long width;
	unsigned long height;
	display_t display;
}
reg_table_t, *preg_table_t;

// Panel On/Off constants to use with panelPowerSequence.
typedef enum _panel_state_t
{
	PANEL_OFF,
	PANEL_ON,
}
panel_state_t;

// Structure used to initialize Panel hardware module
typedef struct
{
    unsigned long mask;       // Holds flags indicating which register bitfields to init
	unsigned long dp;         // TFT dithering pattern
	unsigned long tft;        // TFT panel interface
	unsigned long de;         // Enable/disable TFT dithering
	unsigned long lcd;        // LCD type
	unsigned long fifo_level; // FIFO request level
	unsigned long cp;         // Clock phase select
	unsigned long format;     // Panel graphics plane format
} init_panel, *pinit_panel;

// Structure used to initialize Panel cursor hardware module
typedef struct
{
    unsigned long mask;       // Holds flags indicating which register bitfields to init
} init_panel_hwc, *pinit_panel_hwc;

// Structure used to initialize Alpha hardware module
typedef struct
{
    unsigned long mask;       // Holds flags indicating which register bitfields to init
	unsigned long fifo_level; // FIFO request level
	unsigned long format;     // Alpha plane format
} init_alpha, *pinit_alpha;

// Structure used to initialize CRT hardware module
typedef struct
{
    unsigned long mask;       // Holds flags indicating which register bitfields to init
	unsigned long fifo_level; // FIFO request level
	unsigned long tvp;        // TV clock phase select
	unsigned long cp;         // CRT clock phase select
	unsigned long blank;      // CRT data blanking
	unsigned long format;     // CRT graphics plane format
} init_crt, *pinit_crt;

// Structure used to initialize CRT cursor hardware module
typedef struct
{
    unsigned long mask;       // Holds flags indicating which register bitfields to init
} init_crt_hwc, *pinit_crt_hwc;

// Init flags and values used in init_panel, init_alpha, and init_crt structures
#define DISP_FIFO_LEVEL                    0x00000001    // FIFO request level
#define DISP_FIFO_LEVEL_1                  0x00000000
#define DISP_FIFO_LEVEL_3                  0x00010000
#define DISP_FIFO_LEVEL_7                  0x00020000
#define DISP_FIFO_LEVEL_11                 0x00030000

// Init flags and values used in init_panel structure
#define DISP_PANEL_DP                      0x00000100    // TFT dithering pattern
#define DISP_PANEL_DP_4GRAY                0x00000000
#define DISP_PANEL_DP_8GRAY                0x00800000

#define DISP_PANEL_TFT                     0x00000200    // TFT panel interface
#define DISP_PANEL_TFT_24                  0x00000000
#define DISP_PANEL_TFT_9                   0x00200000
#define DISP_PANEL_TFT_12                  0x00400000

#define DISP_PANEL_DE                      0x00000400    // Enable/disable TFT dithering
#define DISP_PANEL_DE_DISABLE              0x00000000
#define DISP_PANEL_DE_ENABLE               0x00100000

#define DISP_PANEL_LCD                     0x00000800    // LCD type
#define DISP_PANEL_LCD_TFT                 0x00000000
#define DISP_PANEL_LCD_STN8                0x00080000
#define DISP_PANEL_LCD_STN12               0x000C0000

#define DISP_PANEL_CP                      0x00001000    // Clock phase select
#define DISP_PANEL_CP_HIGH                 0x00000000
#define DISP_PANEL_CP_LOW                  0x00004000

#define DISP_PANEL_FORMAT                  0x00002000    // Panel graphics plane format
#define DISP_PANEL_FORMAT_8                0x00000000
#define DISP_PANEL_FORMAT_16               0x00000001
#define DISP_PANEL_FORMAT_32               0x00000002

// Init flags and values used in init_alpha structure
#define DISP_ALPHA_FORMAT                  0x00000100    // Alpha plane format
#define DISP_ALPHA_FORMAT_RGB565           0x00000001
#define DISP_ALPHA_FORMAT_ALPHA44          0x00000002
#define DISP_ALPHA_FORMAT_ALPHA4444        0x00000003

// Init flags and values used in init_crt structure
#define DISP_CRT_TVP                       0x00000100    // TV clock phase select
#define DISP_CRT_TVP_HIGH                  0x00000000
#define DISP_CRT_TVP_LOW                   0x00008000

#define DISP_CRT_CP                        0x00000200    // CRT clock phase select
#define DISP_CRT_CP_HIGH                   0x00000000
#define DISP_CRT_CP_LOW                    0x00004000

#define DISP_CRT_BLANK                     0x00000400    // CRT data blanking
#define DISP_CRT_BLANK_OFF                 0x00000000
#define DISP_CRT_BLANK_ON                  0x00000400

#define DISP_CRT_FORMAT                    0x00000800    // CRT graphics plane format
#define DISP_CRT_FORMAT_8                  0x00000000
#define DISP_CRT_FORMAT_16                 0x00000001
#define DISP_CRT_FORMAT_32                 0x00000002

#define DISP_MODE_8_BPP			           0		     // 8 bits per pixel i8RGB
#define DISP_MODE_16_BPP		           1		     // 16 bits per pixel RGB565
#define DISP_MODE_32_BPP		           2		     // 32 bits per pixel RGB888
#define DISP_MODE_YUV			           3		     // 16 bits per pixel YUV422
#define DISP_MODE_ALPHA_8		           4		     // 8 bits per pixel a4i4RGB
#define DISP_MODE_ALPHA_16		           5		     // 16 bits per pixel a4RGB444

#define DISP_PAN_LEFT			           0		     // Pan left
#define DISP_PAN_RIGHT			           1		     // Pan right
#define DISP_PAN_UP				           2		     // Pan upwards
#define DISP_PAN_DOWN		               3		     // Pan downwards

#define DISP_DPMS_QUERY			           -1		     // Query DPMS value
#define DISP_DPMS_ON			           0		     // DPMS on
#define DISP_DPMS_STANDBY	               1		     // DPMS standby
#define DISP_DPMS_SUSPEND		           2		     // DPMS suspend
#define DISP_DPMS_OFF			           3		     // DPMS off

#define DISP_DELAY_DEFAULT		           0		     // Default delay

#define DISP_HVTOTAL_UNKNOWN               -1            // Used in panelSetTiming, crtSetTiming if
                                                         // nHTotal, nVTotal not specified by user
#define DISP_HVTOTAL_SCALEFACTOR           1.25          // Used in panelSetTiming, crtSetTiming if
                                                         // nHTotal, nVTotal not specified by user

#define VGX_SIGNAL_PANEL_VSYNC	           100		     // Panel VSYNC
#define VGX_SIGNAL_PANEL_PAN               101		     // Panel auto panning complete
#define VGX_SIGNAL_CRT_VSYNC               102		     // CRT VSYNC

#define VSYNCTIMEOUT                       10000

#define ALPHA_MODE_PER_PIXEL               0             // Use per-pixel alpha values
#define ALPHA_MODE_ALPHA                   1             // Use alpha value specified in Alpha bitfield
#define ALPHA_COLOR_LUT_SIZE               16            // Number of colors in alpha/video alpha palette

#define HWC_ON_SCREEN                      0             // Cursor is within screen top/left boundary
#define HWC_OFF_SCREEN                     1             // Cursor is outside screen top/left boundary
#define HWC_NUM_COLORS                     3             // Number of cursor colors

