#include "SPCE061A.H"
extern void Delay5us();
extern void PutPortraitChar(unsigned int ScreenX,unsigned int Line,unsigned char *StringPointer,unsigned int Overwrite_Back_Ground);

unsigned int ReadBuffer[256];
unsigned int WriteBuffer[256];

unsigned int BlockSize;

//*P_Watchdog_Clear=0x0001

//CLK_High()
//*P_IOB_Data|=0x0100;

//CLK_Low()
//*P_IOB_Data&=0xfeff;

//CS_High()
//*P_IOB_Data|=0x0800;

//CS_Low()
//*P_IOB_Data&=0xf7ff;

//DO_High()
//*P_IOB_Data|=0x4000;

//DO_Low()
//*P_IOB_Data&=0xbfff;
//********************************************
void SD_2Byte_Write(unsigned int IOData)//Simulating SPI IO
{
	unsigned int BitCounter;
	flash_writeb_cmd((IOData>>8)&0xff);
	flash_writeb_cmd(IOData&0xff);
}
//********************************************
void SD_Write(unsigned int IOData)//Simulating SPI IO
{
	flash_writeb_cmd(IOData&0xff);
}
//********************************************
unsigned int SD_2Byte_Read()//Simulating SPI IO
{
	unsigned int BitCounter,Buffer;
	Buffer=flash_read_data();
	Buffer=(Buffer<<8)|flash_read_data();
	
	return Buffer;
}
//********************************************
unsigned int SD_Read()//Simulating SPI IO
{
	
	return flash_read_data();
}
//********************************************
unsigned int SD_CMD_Write(unsigned int CMDIndex,unsigned long CMDArg,unsigned int ResType,unsigned int CSLowRSV)//ResType:Response Type, send 1 for R1; send 2 for R1b; send 3 for R2.
{	//There are 7 steps need to do.(marked by [1]-[7])
	unsigned int temp,Response,Response2,CRC,MaximumTimes;
	Response2=0;
	MaximumTimes=10;
	CRC=0x0095;//0x0095 is only valid for CMD0
	if (CMDIndex!=0) CRC=0x00ff;
	
//	*P_IOB_Data&=0xf7ff;//[1] CS Low
	
	SD_2Byte_Write(((CMDIndex|0x0040)<<8)+(CMDArg>>24));//[2] Transmit Command_Index & 1st Byte of Command_Argument.
	SD_2Byte_Write((CMDArg&0x00ffff00)>>8);				//[2] 2nd & 3rd Byte of Command_Argument
	SD_2Byte_Write(((CMDArg&0x000000ff)<<8)+CRC);		//[2] 4th Byte of Command_Argument & CRC only for CMD0
	
#if 0
	*P_IOB_Data|=0x4000;//[3] Do High
						//[3] Restore Do to High Level
	
	 for (temp=0;temp<8;temp++)//[4] Provide 8 extra clock after CMD
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
#else
	flash_writeb_cmd(0xff);
#endif
	
	switch (ResType)//[5] wait response
	{
		case 1://R1
				{
					do
						Response=SD_Read();
					while (Response==0xffff);
					break;
				}
		case 2://R1b
				{
					do
						Response=SD_Read();
					while (Response==0xffff);//Read R1 firstly
					
					do
						Response2=SD_Read()-0xff00;
					while (Response2!=0);//Wait until the Busy_Signal_Token is non-zero
					break;	
				}
		case 3: Response=SD_2Byte_Read();break;//R2
	}

#if 0
	if (CSLowRSV==0) *P_IOB_Data|=0x0800;//[6] CS High (if the CMD has data block response CS should be kept low)
	 
	 for (temp=0;temp<8;temp++)//[7] Provide 8 extra clock after card response
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
#else
flash_writeb_cmd(0xff);
#endif
	return Response;
}
//********************************************
unsigned int SD_Reset_Card()
{
	unsigned int i,Response;
	
	for (i=0;i<80;i++)//Send 74+ Clocks
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	for(i=0;i<10;i++)//The max initiation times is 10.
	{
		Response=SD_CMD_Write(0x0000,0x00000000,1,0);//Send CMD0
		if (Response==0xff01) i=10;
	}

	return Response;
}
//********************************************
unsigned int SD_Initiate_Card()//Polling the card after reset
{
	unsigned int i,Response;
	
	for(i=0;i<30;i++)//The max initiation times is 10.
	{
		Response=SD_CMD_Write(0x0037,0x00000000,1,0);//Send CMD55
		Response=SD_CMD_Write(0x0029,0x00000000,1,0);//Send ACMD41
		if (Response==0xff00) i=30;
	}

	return Response;
}
//********************************************
unsigned int SD_Get_CardInfo()//Read CSD register
{
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
	{
		Response=SD_CMD_Write(9,0x00000000,3,1);//Send CMD9
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	 for (temp=0;temp<8;temp++)//Provide 8 clock to romove the first byte of data response (0x00fe)
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	for (temp=0;temp<8;temp++) ReadBuffer[temp]=SD_2Byte_Read();//Get the CSD data
	
	for (temp=0;temp<16;temp++)//Provide 16 clock to remove the last 2 bytes of data response (CRC)
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	*P_IOB_Data|=0x0800;//CS_High()
	
	for (temp=0;temp<8;temp++)//Provide 8 extra clock after data response
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}

	return Response;
}
//********************************************
unsigned int SD_Overall_Initiation()
{
	unsigned int Response,Response_2;
	Response=0x0000;
	Response_2=0xff00;
	
	*P_IOB_Data|=0x4000;//[1] Do High
						//[1] Do must be High when there is no transmition

	Response=SD_Reset_Card();//[2] Send CMD0
	if (Response!=0xff01)
	{
		PutPortraitChar(0,15,"No SD Card..",1);//Print MSG
		Response_2+=8;
		return Response_2;
	}
		
	Response=SD_Initiate_Card();//[3] Send CMD55+ACMD41 
	if (Response==0xff00)
		PutPortraitChar(0,15,"Init Success",1);//Print MSG
	else
		{
		Response_2+=4;
		PutPortraitChar(0,15,"No SD Card..",1);//Print MSG
		}
	
	return Response_2;
//	1111|1111||0000|0000 Response_2
//                  ||
//                  ||__CMD55+ACMD41 Fail
//                  |___CMD0 Fail
}
//********************************************
unsigned int SD_Get_CardID()//Read CID register
{
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
	{
		Response=SD_CMD_Write(10,0x00000000,1,1);//Send CMD9
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	 for (temp=0;temp<8;temp++)//Provide 8 clock to romove the first byte of data response (0x00fe)
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	for (temp=0;temp<8;temp++) ReadBuffer[temp]=SD_2Byte_Read();//Get the CID data
	
	for (temp=0;temp<16;temp++)//Provide 16 clock to remove the last 2 bytes of data response (CRC)
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	*P_IOB_Data|=0x0800;//CS_High()
	
	for (temp=0;temp<8;temp++)//Provide 8 extra clock after data response
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	return Response;
}
//********************************************
unsigned int Read_Single_Block(unsigned long int ByteAddress)
{
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
	{
		Response=SD_CMD_Write(17,ByteAddress,1,1);//Send CMD17
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	if (Response!=0xff00) return Response;
		
	while (SD_Read()!=0xfffe) {;}
	
	for (temp=0;temp<256;temp++) ReadBuffer[temp]=SD_2Byte_Read();//Get the readed data
	
	for (temp=0;temp<16;temp++)//Provide 16 clock to remove the last 2 bytes of data response (CRC)
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	*P_IOB_Data|=0x0800;//CS_High()
	
	for (temp=0;temp<8;temp++)//Provide 8 extra clock after data response
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	return Response;
}
//********************************************
unsigned int Write_Single_Block(unsigned long int ByteAddress)
{
	unsigned int temp,Response,MaximumTimes;
	MaximumTimes=10;
	
	for(temp=0;temp<MaximumTimes;temp++)
	{
		Response=SD_CMD_Write(24,ByteAddress,1,1);//Send CMD24
		if (Response==0xff00)
			temp=MaximumTimes;
	}
	
	for (temp=0;temp<8;temp++)//Provide 8 extra clock after CMD response
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	SD_Write(0x00fe);//Send Start Block Token
	for (temp=0;temp<256;temp++) SD_2Byte_Write(WriteBuffer[temp]);//Data Block
	SD_2Byte_Write(0xffff);//Send 2 Bytes CRC
	
	Response=SD_Read();
	while (SD_Read()!=0xffff) {;}
	
	*P_IOB_Data|=0x0800;//CS_High()
	
	for (temp=0;temp<8;temp++)//Provide 8 extra clock after data response
	{
		*P_IOB_Data&=0xfeff;//CLK Low
		Delay5us();
		*P_IOB_Data|=0x0100;//CLK High
		Delay5us();
	}
	
	return Response;
}
