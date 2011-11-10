#include <pmon.h>
#include "gcSdk.h"
#include "common.h"

UINT32 gcREG_BASE=SYS_BASE;

UINT32 gcReadReg(
	UINT32 Address
	)
{
	return *(PUINT32) (gcREG_BASE + (Address << 2));
}

void gcWriteReg(
	UINT32 Address,
	UINT32 Data
	)
{
	*(PUINT32) (gcREG_BASE + (Address << 2)) = Data;
}

UINT32 gcReportIdle(
	char* Message
	)
{
	UINT32 idle = gcReadReg(AQHiIdleRegAddrs);

	if (Message != NULL)
	{
		printf(Message, idle);
	}

	return idle;
}

UINT32 gcReportRegs(void)
{
	UINT32 ClockControl 	= gcReadReg(AQHiClockControlRegAddrs);
	UINT32 HiIdleReg 		= gcReadReg(AQHiIdleRegAddrs);
	UINT32 AxiConfigReg 	= gcReadReg(AQAxiConfigRegAddrs);
	UINT32 AxiStatusReg 	= gcReadReg(AQAxiStatusRegAddrs);
	UINT32 IntrAcknowledge 	= gcReadReg(AQIntrAcknowledgeRegAddrs);
	UINT32 IntrEnblReg 		= gcReadReg(AQIntrEnblRegAddrs);
	UINT32 IdentReg			= gcReadReg(AQIdentRegAddrs);
	UINT32 FeaturesReg		= gcReadReg(GCFeaturesRegAddrs);
	
	UINT32 StreamBaseAddr	= gcReadReg(AQIndexStreamBaseAddrRegAddrs);
	UINT32 IndexStreamCtrl  = gcReadReg(AQIndexStreamCtrlRegAddrs);
	UINT32 CmdBufferAddr 	= gcReadReg(AQCmdBufferAddrRegAddrs);
	UINT32 CmdBufferCtrl 	= gcReadReg(AQCmdBufferCtrlRegAddrs);
	UINT32 FEStatusReg 		= gcReadReg(AQFEStatusRegAddrs);
	UINT32 FEDebugState 	= gcReadReg(AQFEDebugStateRegAddrs);
	UINT32 FEDebugCurCmd 	= gcReadReg(AQFEDebugCurCmdAdrRegAddrs);
	UINT32 FEDebugCmdLow 	= gcReadReg(AQFEDebugCmdLowRegRegAddrs);


	printf("ClockControl 	= %x\n", ClockControl	);
	printf("HiIdleReg    	= %x\n", HiIdleReg   	);
	printf("AxiConfigReg 	= %x\n", AxiConfigReg	);
	printf("AxiStatusReg 	= %x\n", AxiStatusReg	);
	printf("IntrAcknowledge = %x\n", IntrAcknowledge);
	printf("IntrEnblReg 	= %x\n", IntrEnblReg	);	
	printf("IdentReg    	= %x\n", IdentReg   	);	
	printf("FeaturesReg 	= %x\n", FeaturesReg	);
	
	printf("registers in FE module!\n");
	printf("StreamBaseAddr 	= %x\n", StreamBaseAddr	);
	printf("IndexStreamCtrl = %x\n", IndexStreamCtrl);
	printf("CmdBufferAddr 	= %x\n", CmdBufferAddr	);
	printf("CmdBufferCtrl	= %x\n", CmdBufferCtrl	);
	printf("FEStatusReg 	= %x\n", FEStatusReg	);
	printf("FEDebugState 	= %x\n", FEDebugState	);
	printf("FEDebugCurCmd 	= %x\n", FEDebugCurCmd	);	
	printf("FEDebugCmdLow	= %x\n", FEDebugCmdLow	);

	
	return 1;
}

//static const TestStretch[] =
static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"dump_gpu", "", 0, "report GPU regs", gcReportRegs, 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}

