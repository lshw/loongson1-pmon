/*
 * via686b.h: VIA 686B southbridge
 *
 * Copyright (c) 2006, Lemote Ltd.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GPL License Agreement. 
 *
 * You may not, however, modify or remove any part of this copyright 
 * message if this program is redistributed or reused in whole or in
 * part.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GPL License for more details.  
 */

/*** CONFIG REGISTERS AND VALUES */
#define SMBUS_IO_BASE_ADDR  0x90  
#define SMBUS_IO_BASE_VALUE	0xeee1

#define SMBUS_HOST_CONFIG_ADDR  0xd2
#define SMBUS_HOST_CONFIG_ENABLE_BIT 0x1

#define SMBUS_HOST_SLAVE_COMMAND     0xd3

/*** SMBUS IO REGISTERS AND VALUES */
#define SMBUS_HOST_STATUS   ((SMBUS_IO_BASE_VALUE & 0xfff0) + 0x0)
#define SMBUS_HOST_STATUS_BUSY   0x1
#define SMBUS_HOST_STATUS_INT	 0x2
#define SMBUS_HOST_STATUS_DEVERR 0x4
#define SMBUS_HOST_STATUS_COLLISION 0x8
#define SMBUS_HOST_STATUS_FAIL   0x10

#define SMBUS_HOST_CONTROL ((SMBUS_IO_BASE_VALUE & 0xfff0) + 0x2)
#define SMBUS_HOST_CONTROL_START   0x40
#define SMBUS_HOST_CONTROL_KILL    0x2
#define SMBUS_HOST_CONTROL_INTEN   0x1

#define SMBUS_HOST_COMMAND ((SMBUS_IO_BASE_VALUE & 0xfff0) + 0x3)

#define SMBUS_HOST_ADDRESS ((SMBUS_IO_BASE_VALUE & 0xfff0) + 0x4)
#define SMBUS_HOST_ADDRESS_READOP   0x1
#define SMBUS_HOST_ADDRESS_WRITEOP  0x0

#define SMBUS_HOST_DATA0   ((SMBUS_IO_BASE_VALUE & 0xfff0) + 0x5)
#define SMBUS_HOST_DATA1   ((SMBUS_IO_BASE_VALUE & 0xfff0) + 0x6)
