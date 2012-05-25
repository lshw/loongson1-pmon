#ifndef __UTILITIES__
#define __UTILITIES__

#include "./Reg/AQ.h"
#include <stdio.h>
#include "display.h" 
#define SAVEFRAMEBUFFER

#define REG_BASE 0x80000000
#define DEBUG_REG_BASE 0x80040000

#define IDX_BUFFER_ADDR    0x03000000
#define VERTEX_BUFFER_ADDR 0x04000000						   
#define TEX_BUFFER_ADDR    0x05000000
#define DEPTH_BUFFER_ADDR  0x06000000
#define FRAME_BUFFER_ADDR  0x08000000

#define TILE_STATUS_BUFFERC_ADDR 0x07000000
#define TILE_STATUS_BUFFERZ_ADDR 0x07500000



void show(void);

void DoResolve(
	unsigned long phys,
	unsigned long SrcAddress,
	unsigned long DestAddress,
	unsigned long DepthAddress,
	int Width,
	int Height, 
	int SrcColorBpp, 
	int DestColorBpp,
	int DepthBpp, 
	int SrcTiled,
	int DepthTiled,
	int DestTiled,
	int DetectThresh,
	int FilterOffFrontZ,
	int EdgeDetect,
	int SuperSample
	);

void DoResolve_new(
	unsigned long phys,
	unsigned long SrcAddress,
	unsigned long DestAddress,
	unsigned long DepthAddress,
	int Width,
	int Height, 
	int SrcColorBpp, 
	int DestColorBpp,
	int DepthBpp, 
	int SrcTiled,
	int DepthTiled,
	int DestTiled,
	int DetectThresh,
	int FilterOffFrontZ,
	int EdgeDetect,
	int SuperSample
	);	

void CallResolve(unsigned long phys, int EdgeDetect);
void CallResolve1(unsigned long phys, int SuperSample);
void CallResolve1_new(unsigned long phys, int EdgeDetect);
//TWOD Clear command
void clearTWOD(unsigned long phys, unsigned long target_address, unsigned long target_data);


#endif

