/**************** STFL-I Flash Memory Driver ***********************************

   Filename:    c2311.c
   Description: Library routines for the M29W640F
                64Mb (8Mb x 8 or 4Mb x 16,Page,Boot Block) Flash Memory drivers
                in different configurations.

   Version:     $Id: c2311.c,v 1.0 2006/05/09 
   Author:     Ze-Yu He, MSAC,STMicroelectronics, Shanghai (China)
               Wiley Xu, MSAC,STMicroelectronics, Shanghai (China)
               Echo Chen,MSAC,STMicroelectronics, Beijing  (China)
   Copyright (c) 2006 STMicroelectronics.

   THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
   EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTY
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK
   AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE
   PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
   REPAIR OR CORRECTION.
********************************************************************************

   Version History.

   Ver.   Date        Comments

   0.0    2006/01/20  Initial Release of the software (Alpha)
   0.1    2006/02/28  Bug Fixed in Flash Function
   0.2    2006/03/08  Added Double Byte Program
   1.0    2006/05/09  Qualified Release of the software

********************************************************************************

   This source file provides library C code for using the M29W640F flash devices.
   The following device is supported in the code: M29W640FB and M29W640FT

   This file can be used to access the devices in 8bit and 16bit mode.

   The following functions are available in this library:

      Flash(BlockErase, ParameterType)            to erase one block
      Flash(CheckBlockProtection, ParameterType)  to check whether a given block is protected
      Flash(CheckCompatibility, ParameterType)    to check the compatibility of the flash 
      Flash(ChipErase, ParameterType)             to erase the whole chip
      Flash(ChipUnprotect, ParameterType)         to unprotect the whole chip
      Flash(GroupProtect, ParameterType)          to unprotect a blocks group
      Flash(Program, ParameterType)               to program an array of elements
      Flash(Read, ParameterType)                  to read from the flash device
      Flash(ReadCfi, ParameterType)               to read CFI information from the flash
      Flash(ReadDeviceId, ParameterType)          to get the Device ID from the device 
      Flash(ReadManufacturerCode, ParameterType)  to get the Manufacture Code from the device
      Flash(Reset, ParameterType)                 to reset the flash for normal memory access 
      Flash(Resume, ParameterType)                to resume a suspended erase
      Flash(SingleProgram, ParameterType)         to program a single element
      Flash(Suspend, ParameterType)               to suspend an erase
      Flash(Write, ParameterType)                 to write a value to the flash device

      FlashDoubleProgram()                        to program two elements
      FlashEnterExtendedBlock()                   to issue the enter extended mode command
      FlashErrorStr()                             to return an error description (define VERBOSE)
      FlashExitExtendedBlock()                    to issue the exit extended mode command
      FlashMultipleBlockErase()                   to erase some blocks in same bank
      FlashPause()                                for timing short pauses (in micro seconds) 
      FlashQuadProgram()                          to program four elements
      FlashOctProgram()                           to program eight elements
      FlashReadExtendedBlockVerifyCode()    to read the lock/unlock status of the
                                                     Extended Memory Block
      FlashReadProtectionRegister()               to read a protection register location
      FlashReadStatusRegister()                   to read the Status Register
      FlashStatusPinConfig()                      to configure the Status/(Ready/Busy) pin
      FlashTimeOut()                              to return after function timeouts 
      FlashUnlockBypassProgram()                  to program in unlock bypass mode
      FlashUnlockBypassReset()                    to issue the unlock bypass reset command

   For further information consult the Data Sheet and the Application Note.
   The Application Note gives information about how to modify this code for
   a specific application.

   The hardware specific functions which may need to be modified by the user are:

      FlashWrite() for writing an element (uCPUBusType) to the flash
      FlashRead()  for reading an element (uCPUBusType) from the flash

   A list of the error conditions can be found at the end of the header file.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c2311.h" /* Header file with global prototypes */

#ifdef TIME_H_EXISTS
   #include <time.h>
#endif 

/******************************************************************************
    Global variables
*******************************************************************************/ 
ErrorInfoType eiErrorInfo;


/*******************************************************************************
Function:     ReturnType Flash( CommandType cmdCommand, ParameterType *fp )
Arguments:    cmdCommand is an enum which contains all the available function
   commands of the SW driver.
              fp is a (union) parameter struct for all flash command parameters
Return Value: The function returns the following conditions: 

   Flash_AddressInvalid 
   Flash_BlockEraseFailed 
   Flash_BlockNrInvalid 
   Flash_BlockProtected 
   Flash_BlockProtectFailed 
   Flash_BlockProtectionUnclear 
   Flash_BlockUnprotected 
   Flash_CfiFailed 
   Flash_ChipEraseFailed 
   Flash_ChipUnprotectFailed 
   Flash_FunctionNotSupported
   Flash_GroupProtectFailed 
   Flash_NoInformationAvailable
   Flash_OperationOngoing 
   Flash_OperationTimeOut 
   Flash_ProgramFailed 
   Flash_ResponseUnclear 
   Flash_SpecificError
   Flash_Success 
   Flash_WrongType

Description:  This function is used to access all functions provided with the
   current flash chip.

Pseudo Code:
   Step 1: Select the right action using the cmdCommand parameter
   Step 2: Execute the Flash Function
   Step 3: Return the Error Code
*******************************************************************************/
ReturnType Flash( CommandType cmdCommand, ParameterType *fp ) { 
   ReturnType  rRetVal;
   uCPUBusType  ucDeviceId, ucManufacturerCode;

   switch (cmdCommand) {
      case BlockErase:
         rRetVal = FlashBlockErase( (*fp).BlockErase.ublBlockNr );
         break;

      case CheckBlockProtection: 
         rRetVal = FlashCheckBlockProtection( (*fp).CheckBlockProtection.ublBlockNr );  
         break; 

      case CheckCompatibility:
         rRetVal = FlashCheckCompatibility();
         break;

      case ChipErase:
         rRetVal = FlashChipErase( (*fp).ChipErase.rpResults );
         break;

      case ChipUnprotect: 
         rRetVal = FlashChipUnprotect((*fp).ChipUnprotect.rpResults ); 
         break; 

      case GroupProtect: 
         rRetVal = FlashGroupProtect( (*fp).GroupProtect.ublBlockNr ); 
         break; 

      case Program:
         rRetVal = FlashProgram( (*fp).Program.udMode,
                                 (*fp).Program.udAddrOff,
                                 (*fp).Program.udNrOfElementsInArray,
                                 (*fp).Program.pArray );                 

         break;

      case Read:
         (*fp).Read.ucValue = FlashRead( (*fp).Read.udAddrOff );
         rRetVal = Flash_Success;
         break;

      case ReadCfi:  
         rRetVal = FlashReadCfi( (*fp).ReadCfi.uwCfiFunc, &((*fp).ReadCfi.ucCfiValue) ); 
         break; 

      case ReadDeviceId:
         rRetVal = FlashReadDeviceId(&ucDeviceId);
         (*fp).ReadDeviceId.ucDeviceId = ucDeviceId;
         break;

      case ReadManufacturerCode:
         rRetVal = FlashReadManufacturerCode(&ucManufacturerCode);
         (*fp).ReadManufacturerCode.ucManufacturerCode = ucManufacturerCode;
         break;

      case Reset:
         rRetVal = FlashReset();
         break;

      case Resume:
         rRetVal = FlashResume();
         break;

      case SingleProgram:
         rRetVal = FlashSingleProgram( (*fp).SingleProgram.udAddrOff, (*fp).SingleProgram.ucValue );
         break;

      case Suspend:
         rRetVal = FlashSuspend();
         break;

      case Write:
         FlashWrite( (*fp).Write.udAddrOff, (*fp).Write.ucValue ); 
         rRetVal = Flash_Success;
         break;

      default:
         rRetVal = Flash_FunctionNotSupported;
         break;

   } /* EndSwitch */
   return rRetVal;
} /* EndFunction Flash */







/*******************************************************************************
Function:     ReturnType FlashBlockErase( uBlockType ublBlockNr )
Arguments:    ublBlockNr is the number of the Block to be erased.
Return Value: The function returns the following conditions:
   Flash_Success
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_BlockProtected
   Flash_OperationTimeOut

Description:  This function can be used to erase the Block specified in ublBlockNr.
   The function checks that the block nr is within the valid range and not protected
   before issuing the erase command, otherwise the block will not be erased and an
   error code will be returned.
   The function returns only when the block is erased. During the Erase Cycle the
   Data Toggle Flow Chart of the Datasheet is followed. The polling bit, DQ7, is not
   used.
   
Pseudo Code:
   Step 1:  Check that the block number exists
   Step 2:  Check if the block is protected
   Step 3:  Write Block Erase command
   Step 4:  Wait for the timer bit to be set
   Step 5:  Follow Data Toggle Flow Chart until the Program/Erase Controller is
            finished
   Step 6:  Return to Read mode (if an error occurred)
*******************************************************************************/
ReturnType FlashBlockErase( uBlockType ublBlockNr) {

   ReturnType rRetVal = Flash_Success; /* Holds return value: optimistic initially! */

   /* Step 1: Check for invalid block. */
   if( ublBlockNr >= NUM_BLOCKS ) /* Check specified blocks <= NUM_BLOCKS */
      return Flash_BlockNrInvalid;

   /* Step 2: Check if the block is protected */
   if ( FlashCheckBlockProtection(ublBlockNr) == Flash_BlockProtected)
      return Flash_BlockProtected;

   /* Step 3: Write Block Erase command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0080) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( BlockOffset[ublBlockNr], (uCPUBusType)CMD(0x0030) );

   /* Step 4: Wait for the Erase Timer Bit (DQ3) to be set */
   FlashTimeOut(0); /* Initialize TimeOut Counter */
   while( !(FlashRead( BlockOffset[ublBlockNr] ) & CMD(0x0008) ) ){
      if (FlashTimeOut(5) == Flash_OperationTimeOut) {
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         return Flash_OperationTimeOut;
      } /* EndIf */
   } /* EndWhile */

   /* Step 5: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle() !=  Flash_Success ) {
      /* Step 6: Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_BlockEraseFailed;
   } /* EndIf */
   return rRetVal;
} /* EndFunction FlashBlockErase */





/*******************************************************************************
Function:      ReturnType FlashCheckBlockProtection( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be checked
Note: the first block is Block 0

Return Values: The function returns the following conditions: 
   Flash_BlockNrInvalid
   Flash_BlockUnprotected
   Flash_BlockProtected
   Flash_BlockProtectionUnclear

Description:   This function reads the protection status of a block.
Pseudo Code:
   Step 1:  Check that the block number exists
   Step 2:  Send the AutoSelect command
   Step 3:  Read Protection Status
   Step 4:  Return the device to Read Array mode
*******************************************************************************/
ReturnType FlashCheckBlockProtection( uBlockType ublBlockNr ) {
   ReturnType  rRetVal; /* Holds the return value */
   uCPUBusType ucProtStatus; /* Holds the protection status */

   /* Step 1: Check that the block number exists */
   if ( ublBlockNr >= NUM_BLOCKS )
      return Flash_BlockNrInvalid;

   /* Step 2: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 3: Read Protection Status */
   ucProtStatus=FlashRead( BlockOffset[ublBlockNr] + ShAddr(0x02));
   if ( (ucProtStatus & CMD(0x00ff)) == 0)
      rRetVal = Flash_BlockUnprotected;
   else if ( (ucProtStatus & CMD(0x00ff)) == CMD(0x0001) )
      rRetVal = Flash_BlockProtected;
      else
         rRetVal = Flash_BlockProtectionUnclear;

   /* Step 4: Return to Read mode */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
   return rRetVal;

} /* EndFunction FlashCheckBlockProtection */





/******************************************************************************* 
Function:      ReturnType FlashCheckCompatibility( void ) 
Arguments:     None
Return Values: The function returns the following conditions:  
   Flash_Success
   Flash_WrongType
Description:   This function checks the compatibility of the device with
   the SW driver.
 
Pseudo Code: 
   Step 1:  Read the Device Id 
   Step 2:  Read the Manufacturer Code
   Step 3:  Check the results
*******************************************************************************/ 
ReturnType FlashCheckCompatibility( void ) { 
   ReturnType  rRetVal, rCheck1, rCheck2; /* Holds the results of the Read operations */ 
   uCPUBusType ucDeviceId, ucManufacturerCode; /* Holds the values read */ 

   rRetVal =  Flash_WrongType;

   /* Step 1:  Read the Device Id */
   rCheck1 =  FlashReadDeviceId( &ucDeviceId ); 

   /* Step 2:  Read the ManufactureCode */
   rCheck2 =  FlashReadManufacturerCode( &ucManufacturerCode ); 

   /* Step 3:  Check the results */
   if (    (rCheck1 == Flash_Success) && (rCheck2 == Flash_Success)   
      && (ucDeviceId == EXPECTED_DEVICE)  && (ucManufacturerCode == MANUFACTURER_ST)  )  
      rRetVal = Flash_Success; 
   return rRetVal; 
} /* EndFunction FlashCheckCompatibility */ 





/*******************************************************************************
Function:     ReturnType FlashCheckBlockEraseError( uBlockType  ublBlock )
Arguments:    ublBlock specifies the block to be checked
Return Value: 
         Flash_Success
         FlashBlockEraseFailed

Description:  This function can only be called after an erase operation which
   has failed the FlashDataPoll() function. It must be called before the reset
   is made. The function reads bit 2 of the Status Register to check if the block
   has erased successfully or not. Successfully erased blocks should have DQ2
   set to 1 following the erase. Failed blocks will have DQ2 toggle.

Pseudo Code:
   Step 1: Read DQ2 in the block twice
   Step 2: If they are both the same then return Flash_Success
   Step 3: Else return Flash_BlockEraseFailed
*******************************************************************************/
static ReturnType FlashCheckBlockEraseError( uBlockType ublBlock ){

   uCPUBusType ucFirstRead, ucSecondRead; /* Two variables used for clarity*/
   
   /* Step 1: Read DQ2 in the block twice */
   ucFirstRead  = FlashRead( BlockOffset[ublBlock] ) & CMD(0x0004);
   ucSecondRead = FlashRead( BlockOffset[ublBlock] ) & CMD(0x0004);
   /* Step 2: If they are the same return Flash_Success */
   if( ucFirstRead == ucSecondRead )
      return Flash_Success;
   /* Step 3: Else return Flash_BlockEraseFailed */
   return Flash_BlockEraseFailed;
} /*EndFunction FlashCheckBlockEraseError*/





/*******************************************************************************
Function:     ReturnType FlashChipErase( ReturnType *rpResults )
Arguments:    rpResults is a pointer to an array where the results will be 
   stored. If rpResults == NULL then no results have been stored.
Return Value: The function returns the following conditions:
   Flash_Success              
   Flash_ChipEraseFailed

Description: The function can be used to erase the whole flash chip. Each Block
   is erased in turn. The function only returns when all of the Blocks have
   been erased. If rpResults is not NULL, it will be filled with the error
   conditions for each block.

Pseudo Code:
   Step 1: Check if some blocks are protected
   Step 2: Send Chip Erase Command
   Step 3: Check for blocks erased correctly
   Step 4: Return to Read mode (if an error occurred)
*******************************************************************************/
ReturnType FlashChipErase( ReturnType *rpResults ) {

   ReturnType rRetVal = Flash_Success, /* Holds return value: optimistic initially! */
              rProtStatus; /* Holds the protection status of each block */
   uBlockType ublCurBlock; /* Used to tack current block in a range */

   /* Step 1: Check if some blocks are protected */
   for (ublCurBlock=0; ublCurBlock < NUM_BLOCKS;ublCurBlock++) {
      if (FlashCheckBlockProtection(ublCurBlock)==Flash_BlockProtected) {
         rProtStatus = Flash_BlockProtected;
	 rRetVal = Flash_ChipEraseFailed;
      } else
         rProtStatus =Flash_Success;
      if (rpResults != NULL)
         rpResults[ublCurBlock] = rProtStatus;
   } /* Next ublCurBlock */

   /* Step 2: Send Chip Erase Command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0080) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0010) );

   /* Step 3: Check for blocks erased correctly */
   if( FlashDataToggle()!=Flash_Success){
      rRetVal= Flash_ChipEraseFailed;
      if (rpResults != NULL){
         for (ublCurBlock=0;ublCurBlock < NUM_BLOCKS;ublCurBlock++)
            if (rpResults[ublCurBlock]==Flash_Success)
               rpResults[ublCurBlock] = FlashCheckBlockEraseError(ublCurBlock);
      } /* EndIf */
         /* Step 4: Return to Read mode (if an error occurred) */
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
   } /* EndIf */
   return rRetVal;

} /* EndFunction FlashChipErase */





/*******************************************************************************
Function:     ReturnType FlashChipUnprotect( ReturnType *rpResults )
Arguments:    rpResults, if not NULL, it contains the status of every block.
              This device does not support this functionality. If rpResults == NULL
              then no results are stored. Otherwise the Flash_NoInformationAvailable
              is written to the array.

Return Value: The function returns the following conditions:  
   Flash_Success
   Flash_ChipUnprotectFailed

Description:  This function unprotects the whole flash chip implementing the
              In-System Unprotection procedure (see the device datasheet).

Pseudo Code:
   Step 1: filling of rpResults
   Step 2: protect all blocks
   Step 3: setup phase
   Step 4: unprotect phase
   Step 5: verify phase
   Step 6: if verified and the current block isn't last block, increment current
           block number and repeat from step 3; else return Flash_Success
   Step 7: if not verified and if attempts number is < 1000, repeat from step 2,
   	   else return Flash_ChipUnprotectFailed

*******************************************************************************/
ReturnType FlashChipUnprotect( ReturnType *rpResults )
{
   uBlockType ublCurBlockNr;
   uCPUBusType ucReadData;
   word wAttempt; 
   udword udAddrOff,i;

   /* Step 1: filling of rpResults */
   if (rpResults !=NULL) {
      for (ublCurBlockNr=0; ublCurBlockNr<NUM_BLOCKS; ublCurBlockNr++)
         rpResults[ublCurBlockNr]=Flash_NoInformationAvailable;
   } /* EndIf */
   
   /* Step 2: protect all blocks */
   i = 0;
   for (ublCurBlockNr=0; ublCurBlockNr<NUM_BLOCKS; ublCurBlockNr+=BlockGroupOffset[i++]){
      if (FlashGroupProtect(ublCurBlockNr) == Flash_GroupProtectFailed)
         return  Flash_ChipUnprotectFailed;
   } /* Next ublCurBlockNr */

   /* Reset command */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
   wAttempt = 0;
   ublCurBlockNr = 0;
   udAddrOff = ANY_ADDR & 0x11111100;
   i = 0;

   /* Step 3: setup phase */
   FlashWrite( (udAddrOff | ShAddr(0x00000002)), CMD(0x0060) );
   do {
      FlashWrite( (udAddrOff | ShAddr(0x00000042)), CMD(0x0060) );
      FlashPause(10000);
      do {      
         /* Step 4: unprotect phase */
	 FlashWrite( (BlockOffset[ublCurBlockNr] | ShAddr(0x00000042)), CMD(0x0040) );
	 FlashPause(4);
	 ucReadData = FlashRead( BlockOffset[ublCurBlockNr] | ShAddr(0x00000042) );  
	 /* Step 5: verify phase */
	 if ( ucReadData == CMD(0x0000) ){
	    /* Step 6: if verified, if the current block isn't last block increment current block 
   	       number and repeat step from 3 else return Flash_Success */
            ublCurBlockNr += BlockGroupOffset[i++]; /* Next group */
            if ( ublCurBlockNr < NUM_BLOCKS ) {	
                continue;
            }
            else {
               /* Reset command  */
	           FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
	           return Flash_Success;
            } 
	    } /* EndIf */
         else
            break;
      } while (1);
      /* Step 7: if not verified and if attempts number is < 1000, repeat step from 2,
                 else return Flash_ChipUnprotectFailed */
   } while (++wAttempt < 1000 );  

   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
   return Flash_ChipUnprotectFailed;

} /* EndFunction FlashChipUnprotect */





/*******************************************************************************
Function:     ReturnType FlashDataToggle( void )
Arguments:    none
Return Value: The function returns Flash_Success if the Program/Erase Controller
   is successful or Flash_SpecificError if there is a problem.In this case
   the field eiErrorInfo.sprRetVal will be filled with FlashSpec_ToggleFailed value.
   If the Program/Erase Controller do not finish before time-out expired
   the function return Flash_OperationTimeout.

Description:  The function is used to monitor the Program/Erase Controller during
   erase or program operations. It returns when the Program/Erase Controller has
   completed. In the Data Sheets, the Data Toggle Flow Chart shows the operation
   of the function.

Pseudo Code:
   Step 1: Read DQ5 and DQ6 (into word)
   Step 2: Read DQ6 (into  another a word)
   Step 3: If DQ6 did not toggle between the two reads then return Flash_Success
   Step 4: Else if DQ5 is zero then operation is not yet complete, goto 1
   Step 5: Else (DQ5 != 0), read DQ6 again
   Step 6: If DQ6 did not toggle between the last two reads then return 
            Flash_Success
   Step 7: Else return Flash_ToggleFail
*******************************************************************************/
static ReturnType FlashDataToggle( void ){

   uCPUBusType ucVal1, ucVal2; /* hold values read from any address offset within 
                                  the Flash Memory */

   FlashTimeOut(0); /* Initialize TimeOut Counter */     
   while(FlashTimeOut(120) != Flash_OperationTimeOut) {   
      /* TimeOut: If, for some reason, the hardware fails then this
         loop exit and the function return flash_OperationTimeOut.  */
            
      /* Step 1: Read DQ5 and DQ6 (into  word) */
      ucVal2 = FlashRead( ANY_ADDR ); /* Read DQ5 and DQ6 from the Flash (any 
                                         address) */

      /* Step 2: Read DQ6 (into another a word) */
      ucVal1 = FlashRead( ANY_ADDR ); /* Read DQ6 from the Flash (any address) */


      /* Step 3: If DQ6 did not toggle between the two reads then return 
                 Flash_Success */
      if( (ucVal1&CMD(0x0040)) == (ucVal2&CMD(0x0040)) ) /* DQ6 == NO Toggle */
         return Flash_Success;

      /* Step 4: Else if DQ5 is zero then operation is not yet complete */
      if( (ucVal2&CMD(0x0020)) != CMD(0x0020) )
         continue;

      /* Step 5: Else (DQ5 == 1), read DQ6 twice */
      ucVal1 = FlashRead( ANY_ADDR ); /* Read DQ6 from the Flash (any address) */
      ucVal2 = FlashRead( ANY_ADDR );
      /* Step 6: If DQ6 did not toggle between the last two reads then
                 return Flash_Success */
      if( (ucVal2&CMD(0x0040)) == (ucVal1&CMD(0x0040)) ) /* DQ6 == NO Toggle  */
         return Flash_Success;

      /* Step 7: Else return Flash_ToggleFailed */
      else {
         /* DQ6 == Toggle here means fail */
         eiErrorInfo.sprRetVal=FlashSpec_ToggleFailed;
         return Flash_SpecificError;
      } /* EndInf */
   } /* EndWhile */    
   return Flash_OperationTimeOut; /*if exit from while loop then time out exceeded */
} /* EndFunction FlashDataToggle */





/*****************************************************************************************
Function:   ReturnType FlashDoubleProgram( udword udAddrOff, uCPUBusType ucVal1, 
                                           uCPUBusType ucVal2 )
Arguments:
   udAddrOff is an address offset into pair to be programmed 
   ucValue1 is the value to be programmed at the first address offset of pair
   ucValue2 is the value to be programmed the second address offset of pair
Return Value: 
   The function returns the following conditions: 
      Flash_Success                  
      Flash_AddressInvalid           
      Flash_BlockProtected           
      Flash_ProgramFailed

Description: 
   This function is used to program two uCPUBusType value into two addresses which differ
   only for the bit A0 (LSB). It uses double-word program command. It does not
   erase the flash first and no error is returned in case of double programming. 
   Once the program command has completed the function checks the Status 
   Register for errors. The function returns Flash_Success if the addresses
   have successfully been programmed. 
   Note: 1) VPP must be set to VPPH. (Other limitations are indicated in the data sheet).
         2) This procedure is available both in 16-bit and 8-bit mode. 
     
Pseudo Code:
   Step 1: Check the offset range is valid
   Step 2: Check that the block(s) to be programmed are not protected
   Step 3: Program the data
   Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller has 
           completed
   Step 5: Return to Read Mode (if an error occurred)
*******************************************************************************/
ReturnType FlashDoubleProgram( udword udAddrOff, uCPUBusType ucVal1, uCPUBusType ucVal2 ) {   
   uBlockType ublCurBlock;
   ReturnType rRetVal = Flash_Success; /* Return Value: Initially optimistic */ 
   udword udFirstAddrOff; /* first address offset */


   /* Step 1: Check that the offset and range are valid */   
   if( udAddrOff >= FLASH_SIZE )
      return Flash_AddressInvalid;

   /* compute the start block */
   for (ublCurBlock=0; ublCurBlock < NUM_BLOCKS-1;ublCurBlock++)
      if (udAddrOff < BlockOffset[ublCurBlock+1])
         break;

   /* Step 2: Check if the start block is protected */
   if (FlashCheckBlockProtection(ublCurBlock)== Flash_BlockProtected) {
      return Flash_BlockProtected;
   } /* EndIf */         


   udFirstAddrOff = udAddrOff & (~0x1); /* calculate first address offset*/

   /* Step 3: Program the data */ 
   FlashWrite( ConvAddr(0x555), (uCPUBusType)CMD(0x50) );/* Program Command */
   FlashWrite( udFirstAddrOff, ucVal1 );
   FlashWrite( udFirstAddrOff + 1, ucVal2 );


   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller
              has completed */
   /* See Data Toggle Flow Chart of the Data Sheet */
   if( FlashDataToggle() != Flash_Success) {

      /* Step 5: Return to Read Mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      return Flash_ProgramFailed ;
   } /* EndIf */    
   
   return Flash_Success;

} /* EndFunction FlashDoubleProgram */





/*******************************************************************************
Function:     void FlashEnterExtendedBlock (void);
Arguments:    None
Return Value: None

Description:  This function is used to send the Enter Extended block command to
   the device
Pseudo Code:
   Step 1:  Send the Enter Extended Block command to the device
*******************************************************************************/
void FlashEnterExtendedBlock( void ) {
   /* Step 1: Send the Unlock Bypass command */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x02AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x0088) ); /* 3rd Cycle */
} /* EndFunction FlashEnterExtendedBlock */





#ifdef VERBOSE
/******************************************************************************* 
Function:     byte *FlashErrorStr( ReturnType rErrNum ); 
Arguments:    rErrNum is the error number returned from other Flash Routines 
Return Value: A pointer to a string with the error message 
Description:  This function is used to generate a text string describing the 
   error from the flash. Call with the return value from other flash routines. 
 
Pseudo Code: 
   Step 1: Return the correct string. 
*******************************************************************************/ 
byte *FlashErrorStr( ReturnType rErrNum ) { 

   switch(rErrNum) {
      case Flash_Success: 
         return "Flash - Success"; 
      case Flash_FunctionNotSupported: 
         return "Flash - Function not supported"; 
      case Flash_AddressInvalid: 
         return "Flash - Address is out of Range"; 
      case Flash_BlockEraseFailed: 
         return "Flash - Block Erase failed"; 
      case Flash_BlockNrInvalid: 
         return "Flash - Block Number is out of Range"; 
      case Flash_BlockProtected: 
         return "Flash - Block is protected"; 
      case Flash_BlockProtectFailed: 
         return "Flash - Block Protection failed"; 
      case Flash_BlockProtectionUnclear: 
         return "Flash - Block Protection Status is unclear"; 
      case Flash_BlockUnprotected: 
         return "Flash - Block is unprotected"; 
      case Flash_CfiFailed: 
         return "Flash - CFI Interface failed"; 
      case Flash_ChipEraseFailed: 
         return "Flash - Chip Erase failed"; 
      case Flash_ChipUnprotectFailed: 
         return "Flash - Chip Unprotect failed"; 
      case Flash_GroupProtectFailed: 
         return "Flash - Group Protect Failed"; 
      case Flash_NoInformationAvailable: 
         return "Flash - No Information Available"; 
      case Flash_OperationOngoing: 
         return "Flash - Operation ongoing"; 
      case Flash_OperationTimeOut: 
         return "Flash - Operation TimeOut"; 
      case Flash_ProgramFailed: 
         return "Flash - Program failed";
      case Flash_ResponseUnclear:
         return "Flash - Response unclear"; 
      case Flash_SpecificError:

         switch (eiErrorInfo.sprRetVal) {
            case FlashSpec_MpuTooSlow:
               return "Flash - Flash MPU too slow";
            case FlashSpec_TooManyBlocks:
               return "Flash - Too many Blocks";
            case FlashSpec_ToggleFailed:
               return "Flash - Toggle failed";
            default:
               return "Flash - Undefined Specific Error";
         } /* EndSwitch eiErrorInfo */

      case Flash_WrongType: 
         return "Flash - Wrong Type"; 
      default: 
         return "Flash - Undefined Error Value"; 
   } /* EndSwitch */ 
} /* EndFunction FlashErrorString */
#endif /* VERBOSE Definition */





/*******************************************************************************
Function:     void FlashExitExtendedBlock (void);
Arguments:    None
Return Value: None
Description:  This function is used to send the Exit Extended Block to the device
Pseudo Code:
   Step 1:  Send the Exit Extended Block command to the device
*******************************************************************************/
void FlashExitExtendedBlock( void ){
   /* Step 1: Send the Exit Extended Block command to the device */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x02AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0000) );/* 4th Cycle */
} /* EndFunction FlashExitExtendedBlock */  





/******************************************************************************* 
Function: ReturnType FlashGroupProtect( uBlockType ublBlockNr ) 
Arguments:
   ublBlockNr holds a block number in the group to protect 
Description:
   This function protects a group in the flash chip using
   the In-System Protection procedure as described in the data sheet.

NOTE : This procedure required a high voltage  level on Reset/Blocks Temporary 
       Unprotect pin !RP, else the function will return Flash_BlockProtectFailed.
       For more datails see flow-chart in the Datasheet.

Return Value: The function returns the following conditions: 
   Flash_Success
   Flash_BlockNrInvalid
   Flash_BlockProtectFailed

Pseudo Code:
   Step 1: Check for invalid block
   Step 2: Set-up phase
   Step 3: Protect phase
   Step 4: Verify phase
   Step 5: if verified return Flash_Success
   Step 6:if not verified and if attempts number is < 25, repeat from step 2,
   	  else return Flash_GroupProtectFailed 
*******************************************************************************/
ReturnType  FlashGroupProtect( uBlockType ublBlockNr) {
   word wAttempt= 0;
   uCPUBusType ucReadData;

   /* Step 1: Check for invalid block. */
   if( ublBlockNr >= NUM_BLOCKS ) /* Check specified blocks <= NUM_BLOCKS */
      return Flash_BlockNrInvalid;   

   /* Step 2: set-up phase */
   FlashWrite( (BlockOffset[ublBlockNr] | ShAddr(0x00000002)), CMD(0x0060) );

   do {
      /* Step 3: protect phase */
      FlashWrite( (BlockOffset[ublBlockNr] | ShAddr(0x00000002)), CMD(0x0060) );
      FlashPause(100);

      /* Step 4: verify phase */
      FlashWrite( (BlockOffset[ublBlockNr] | ShAddr(0x00000002)), CMD(0x0040) );
      FlashPause(4);
      ucReadData = FlashRead( BlockOffset[ublBlockNr] | ShAddr(0x00000002) );

      /* Step 5: if verified return Flash_Success */
      if ( ucReadData == CMD(0x0001) ){
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         return Flash_Success;
      } /* EndIf */
   } while (++wAttempt < 25);

   /* Step 6: if not verified and if attempts number is < 25, repeat step from step 2,
              else return Flash_GroupProtectFailed  */  
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */  
   return Flash_GroupProtectFailed;

} /* EndFunction FlashGroupProtect */





/*******************************************************************************
Function:ReturnType FlashMultipleBlockErase(uBlockType ublNumBlocks,uBlockType 
                    *ublpBlock,ReturnType *rpResults)
Arguments:   ublNumBlocks holds the number of blocks in the array ubBlock
   ublpBlocks is an array containing the blocks to be erased.
   rpResults is an array that it holds the results of every single block 
   erase.
   Every elements of rpResults will be filled with below values:
      Flash_Success
      Flash_BlockEraseFailed               
      Flash_BlockProtected
   If a time-out occurs because the MPU is too slow then the function returns 
   Flash_MpuTooSlow 

Return Value: The function returns the following conditions:
   Flash_Success            
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_OperationTimeOut
   Flash_SpecificError      : if a no standard error occour.In this case the 
      field sprRetVal of the global variable eiErrorInfo will be filled 
      with Flash_MpuTooSlow when any blocks are not erased because DQ3 
      the MPU is too slow.


Description: This function erases up to ublNumBlocks in the flash. The blocks
   can be listed in any order. The function does not return until the blocks are
   erased. If any blocks are protected or invalid none of the blocks are erased, 
   in this casse the function return Flash_BlockEraseFailed. 
   During the Erase Cycle the Data Toggle Flow Chart of the Data Sheet is
   followed. The polling bit, DQ7, is not used.

Pseudo Code:
   Step 1:  Check for invalid block  
   Step 2:  Check if some blocks are protected
   Step 3:  Write Block Erase command
   Step 4:  Check for time-out blocks 
   Step 5:  Wait for the timer bit to be set.
   Step 6:  Follow Data Toggle Flow Chart until Program/Erase Controller has 
            completed
   Step 7:  Return to Read mode (if an error occurred)

*******************************************************************************/
ReturnType FlashMultipleBlockErase(uBlockType ublNumBlocks,uBlockType *ublpBlock,ReturnType *rpResults) {

   ReturnType rRetVal = Flash_Success, /* Holds return value: optimistic initially! */
              rProtStatus; /* Holds the protection status of each block */
   uBlockType ublCurBlock; /* Range Variable to track current block */
   uCPUBusType ucFirstRead, ucSecondRead; /* used to check toggle bit DQ2 */

   /* Step 1: Check for invalid block. */
   if( ublNumBlocks > NUM_BLOCKS ){ /* Check specified blocks <= NUM_BLOCKS */
      eiErrorInfo.sprRetVal = FlashSpec_TooManyBlocks; 
      return Flash_SpecificError;
   } /* EndIf */

   /* Step 2: Check if some blocks are protected */ 
   for (ublCurBlock=0; ublCurBlock < ublNumBlocks;ublCurBlock++) {
      if (FlashCheckBlockProtection(ublCurBlock)==Flash_BlockProtected) {
         rProtStatus = Flash_BlockProtected;  
	 rRetVal = Flash_BlockEraseFailed;
      } else 
         rProtStatus =Flash_Success;
         if (rpResults != NULL)
            rpResults[ublCurBlock] = rProtStatus;
   } /* Next ublCurBlock */

   /* Step 3: Write Block Erase command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0080) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );

   /* DSI!: Disable Interrupt, Time critical section. Additional blocks must be added 
            every 50us */

   for( ublCurBlock = 0; ublCurBlock < ublNumBlocks; ublCurBlock++ ) {
      FlashWrite( BlockOffset[ublpBlock[ublCurBlock]], (uCPUBusType)CMD(0x0030) );
      /* Check for Erase Timeout Period (is bit DQ3 set?)*/ 
      if( (FlashRead( BlockOffset[ublpBlock[0]] ) & CMD(0x0008)) != 0 )
         break; /* Cannot set any more blocks due to timeout */
   } /* Next ublCurBlock */

   /* ENI!: Enable Interrupt */

   /* Step 4: Check for time-out blocks */
   /* if timeout occurred then check if current block is erasing or not */
   /* Use DQ2 of status register, toggle implies block is erasing */
   if ( ublCurBlock < ublNumBlocks ) {
      ucFirstRead = FlashRead( BlockOffset[ublpBlock[ublCurBlock]] ) & CMD(0x0004);
      ucSecondRead = FlashRead( BlockOffset[ublpBlock[ublCurBlock]] ) & CMD(0x0004);
      if( ucFirstRead != ucSecondRead )
         ublCurBlock++; /* Point to the next block */

      if( ublCurBlock < ublNumBlocks ){
         /* Indicate that some blocks have been timed out of the erase list */
         rRetVal = Flash_SpecificError;
         eiErrorInfo.sprRetVal = FlashSpec_MpuTooSlow; 
      } /* EndIf */

      /* Now specify all other blocks as not being erased */
      while( ublCurBlock < ublNumBlocks ) {
         rpResults[ublCurBlock++] = Flash_BlockEraseFailed;
      } /* EndWhile */     
   } /* EndIf ( ublCurBlock < ublNumBlocks ) */


   /* Step 5: Wait for the Erase Timer Bit (DQ3) to be set */
   FlashTimeOut(0); /* Initialize TimeOut Counter */
   while( !(FlashRead( BlockOffset[ublpBlock[0]] ) & CMD(0x0008) ) ){ 
      if (FlashTimeOut(5) == Flash_OperationTimeOut) {
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction 
                                                              cycle method */
         return Flash_OperationTimeOut; 
      } /* EndIf */
   } /* EndWhile */

   /* Step 6: Follow Data Toggle Flow Chart until Program/Erase Controlle completes */
   if( FlashDataToggle() !=  Flash_Success ) {
      if (rpResults != NULL) {
         for (ublCurBlock=0;ublCurBlock < ublNumBlocks;ublCurBlock++)
            if (rpResults[ublCurBlock]==Flash_Success)
               rpResults[ublCurBlock] = FlashCheckBlockEraseError(ublCurBlock);  
      } /* EndIf */

      /* Step 7: Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_BlockEraseFailed;
   } /* EndIf */
   return rRetVal;

} /* EndFunction FlashMultipleBlockErase */





/*******************************************************************************
Function:    void FlashPause( udword udMicroSeconds )
Arguments:   udMicroSeconds is the length of the pause in microseconds
Returns:     None
Description: This routine returns after udMicroSeconds have elapsed. It is used
   in several parts of the code to generate a pause required for correct
   operation of the flash part.

Pseudo Code:
   Step 1: Initilize clkReset variable.
   Step 2: Count to the required size.
*******************************************************************************/

#ifdef TIME_H_EXISTS
/*-----------------------------------------------------------------------------
 Note:The Routine uses the function clock() inside of the ANSI C library "time.h". 
-----------------------------------------------------------------------------*/

static void FlashPause(udword udMicroSeconds){
   clock_t clkReset,clkCounts;

   /* Step 1: Initialize clkReset variable */
   clkReset=clock();

   /* Step 2: Count to the required size */
   do
      clkCounts = clock()-clkReset;
   while (clkCounts < ((CLOCKS_PER_SEC/1e6L)*udMicroSeconds));

} /* EndFunction FlashPause */
#else

/*-----------------------------------------------------------------------------
Note: The routine here works by counting. The user may already have a more suitable
      routine for timing which can be used.
-----------------------------------------------------------------------------*/

static void FlashPause(udword udMicroSeconds) {
   static udword udCounter;

   /* Step 1: Compute the count size */
   udCounter = udMicroSeconds * COUNT_FOR_A_MICROSECOND;

   /* Step 2: Count to the required size */
   while( udCounter > 0 ) /* Test to see if finished */
      udCounter--; /* and count down */
} /* EndFunction FlashPause */

#endif





/*******************************************************************************
Function:     ReturnType FlashProgram( udword udMode, udword udAddrOff,
                                       udword udNrOfElementsInArray, void *pArray )
Arguments:    udMode changes between programming modes
   udAddrOff is the address offset into the flash to be programmed
   udNrOfElementsInArray holds the number of elements (uCPUBusType) in the array.
   pArray is a void pointer to the array with the contents to be programmed.

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_AddressInvalid
   Flash_ProgramFailed

Description: This function is used to program an array into the flash. It does
   not erase the flash first and will not produce proper results, if the block(s)
   are not erased first.
   Any errors are returned without any further attempts to program other addresses
   of the device. The function returns Flash_Success when all addresses have
   successfully been programmed.

   Note: Two program modes are available:
   - udMode = 0, Normal Program Mode
   The number of elements (udNumberOfElementsInArray) contained in pArray
   are programmed directly to the flash starting with udAddrOff.    
   - udMode = 1, Single Value Program Mode
¸  Only the first value of the pArray will be programmed to the flash
   starting from udAddrOff.
   .
Pseudo Code:
   Step  1:  Check whether the data to be programmed are are within the
             Flash memory 
   Step  2:  Determine first and last block to program
   Step  3:  Check protection status for the blocks to be programmed
   Step  4:  Issue the Unlock Bypass command
   Step  5:  Unlock Bypass Program command
   Step  6:  Wait until Program/Erase Controller has completed
   Step  7:  Return to Read Mode
   Step  8:  Decision between direct and single value programming
   Step  9:  Unlock Bypass Reset
*******************************************************************************/
ReturnType FlashProgram(udword udMode, udword udAddrOff, udword udNrOfElementsInArray, void *pArray ) {
   ReturnType rRetVal = Flash_Success; /* Return Value: Initially optimistic */ 
   ReturnType rProtStatus; /* Protection Status of a block */
   uCPUBusType *ucpArrayPointer; /* Use an uCPUBusType to access the array */ 
   udword udLastOff; /* Holds the last offset to be programmed */ 
   uBlockType ublFirstBlock; /* The block where start to program */
   uBlockType ublLastBlock; /* The last block to be programmed */
   uBlockType ublCurBlock; /* Current block */

   if (udMode > 1)
      return Flash_FunctionNotSupported;

   /* Step 1: Check if the data to be programmed are within the Flash memory space */
   udLastOff = udAddrOff + udNrOfElementsInArray - 1; 
   if( udLastOff >= FLASH_SIZE ) 
      return Flash_AddressInvalid; 
   
   /* Step 2: Determine first and last block to program */
   for (ublFirstBlock=0; ublFirstBlock < NUM_BLOCKS-1;ublFirstBlock++)
      if (udAddrOff < BlockOffset[ublFirstBlock+1]) 
         break;
   
   for (ublLastBlock=ublFirstBlock; ublLastBlock < NUM_BLOCKS-1;ublLastBlock++)
      if (udLastOff < BlockOffset[ublLastBlock+1]) 
         break;
   
   /* Step 3: Check protection status for the blocks to be programmed */
   for (ublCurBlock = ublFirstBlock; ublCurBlock <= ublLastBlock; ublCurBlock++){
      if ( (rProtStatus = FlashCheckBlockProtection(ublCurBlock)) != Flash_BlockUnprotected ){
         rRetVal = Flash_BlockProtected;
         if (ublCurBlock == ublFirstBlock){
            eiErrorInfo.udGeneralInfo[0] = udAddrOff;
            return rRetVal; 
         } else {
            eiErrorInfo.udGeneralInfo[0] = BlockOffset[ublCurBlock];
            udLastOff = BlockOffset[ublCurBlock]-1;
         } /* EndIf ublCurBlock */
      } /* EndIf rProtStatus */
   } /* Next ublCurBlock */
   
   /* Step 4: Issue the Unlock Bypass command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0020) ); /* 3nd cycle */
   
   ucpArrayPointer = (uCPUBusType *)pArray; 
   
   /* Step 5: Unlock Bypass Program command */ 
   while( udAddrOff <= udLastOff ){
      FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st cycle */
      FlashWrite( udAddrOff, *ucpArrayPointer ); /* 2nd Cycle */  
   
      /* Step 6: Wait until Program/Erase Controller has completed */
      if( FlashDataToggle() != Flash_Success){
         /* Step 7: Return to Read Mode */
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         rRetVal=Flash_ProgramFailed;
         eiErrorInfo.udGeneralInfo[0] = udAddrOff;
         break; /* exit while cycle */
      } /* EndIf */

      /* Step 8: Decision between direct and single value programming */
      if (udMode == 0) /* Decision between direct and single value programming */
         ucpArrayPointer++;
   
      udAddrOff++;
   } /* EndWhile */
   
   /* Step 9: Unlock Bypass Reset */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0090) ); /* 1st cycle */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0000) ); /* 2st cycle */
   
   return rRetVal;

} /* EndFunction FlashProgram */ 




/******************************************************************************* 
Function:     ReturnType FlashQuadProgram( udword udAddrOff, uCPUBusType ucValue1, 
                   uCPUBusType ucValue2,uCPUBusType ucValue3, uCPUBusType ucValue4 ) 
Arguments:    udAddrOff is the address to program.
              ucValue1, ucValue2, ucValue3 and ucValue4 are the values to program 
              on the chip
Return Value: The function returns the following conditions:   
               Flash_Success       
               Flash_ProgramFailed 
   
Note:         
1)    This procedure is available both in 8-bit/16-bit mode. 
2)    This procedure automatically put the 2 least significati bits of udAddrOff 
      to zero, to align the address with a quad-bytes/words boundary.

Description: This function programs four consecutive byte/words, starting from the 
             quadbytes/quadwords boundary.

Pseudo Code: 
   Step 1: Align address to quadbytes/quadwords boundary
   Step 2: Program
   Step 3: Wait until the Program/Erase Controller has completed
   Step 4: Return to read Array mode
********************************************************************************/ 
ReturnType FlashQuadProgram( udword udAddrOff, uCPUBusType ucValue1, uCPUBusType ucValue2, 
                             uCPUBusType ucValue3, uCPUBusType ucValue4  ) {
   ReturnType rRetVal = Flash_Success; /* Return value */

   /* Step 1: Align address to QUAD-BYTE/Word boundary */
   udAddrOff = udAddrOff  & (0xFFFFFFFC);

   /* Step 2: Program  */
   /* Note: the command is expressed with ConvAddr, because it is possible both 8bit and 16bit mode*/
   FlashWrite( ConvAddr(0x0555), CMD(0x0056) ); /* Quadruple Byte Command */
   FlashWrite( udAddrOff,   ucValue1 );
   FlashWrite( udAddrOff+1, ucValue2 );
   FlashWrite( udAddrOff+2, ucValue3 );
   FlashWrite( udAddrOff+3, ucValue4 );
   
   /* Step 3: Wait until Program/Erase Controller has completed */
   if (FlashDataToggle() != Flash_Success)
      rRetVal = Flash_ProgramFailed; 

   /* Step 4: Return to read Array mode */ 
   FlashReset();
   return rRetVal;
} /* EndFunction FlashQuadProgram */


#if defined(USE_M29W640FB_8) || defined(USE_M29W640FT_8) /* In 8 bit Mode */
/******************************************************************************* 
Function:     ReturnType FlashOctProgram( udword udAddrOff, uCPUBusType *pArray ) 
Arguments:    udAddrOff is the address to program.
              pArray is the pointer to the content to be programmed
Return Value: The function returns the following conditions:   
               Flash_Success       
               Flash_ProgramFailed 
   
Note:         
1)    This procedure is available only in 8-bit mode. For the 16-bit mode, see the 
      FlashDoubleProgram/FlashQuadProgram procedure
2)    This procedure automatically put the two least significati bits of udAddrOff 
      to zero, to align the address with an oct-bytes boundary.

Description: This function programs eight consecutive byte, starting from the 
             oct-byte boundary.

Pseudo Code: 
   Step 1: Align address to oct-bytes boundary
   Step 2: Program
   Step 3: Wait until the Program/Erase Controller has completed
   Step 4: Return to read Array mode
********************************************************************************/ 
ReturnType FlashOctProgram( udword udAddrOff, uCPUBusType *pArray  )
{
   ReturnType rRetVal = Flash_Success; /* Return value */
   /* Step 1: Align address to oct-BYTE boundary */
   udAddrOff = udAddrOff  & (0xFFFFFFF8);

   /* Step 2: Program  */
   /* Note: the command is expressed without ConvAddr, because it is possible 
   only in 8bit mode*/
   FlashWrite( 0x0AAA, CMD(0x008B) ); /* Oct Byte Command */
   FlashWrite( udAddrOff, 	*pArray++);
   FlashWrite( udAddrOff+1, *pArray++);
   FlashWrite( udAddrOff+2, *pArray++);
   FlashWrite( udAddrOff+3, *pArray++);
   FlashWrite( udAddrOff+4, *pArray++);
   FlashWrite( udAddrOff+5, *pArray++);
   FlashWrite( udAddrOff+6, *pArray++);
   FlashWrite( udAddrOff+7, *pArray++);

   /* Step 3: Wait until Program/Erase Controller has completed */
   if (FlashDataToggle() != Flash_Success)
      rRetVal = Flash_ProgramFailed; 

   /* Step 4: Return to read Array mode */ 
   FlashReset();
   return rRetVal;

}/* End of  FlashOctProgram*/
#endif /*defined(USE_M29W640FB_8) | defined(USE_M29W640FT_8) */



/*******************************************************************************
Function:     uCPUBusType FlashRead( udword udAddrOff )
Arguments:    udAddrOff is the offset into the flash to read from.
Return Value: The uCPUBusType content at the address offset.
Description: This function is used to read a uCPUBusType from the flash.
   On many microprocessor systems a macro can be used instead, increasing the
   speed of the flash routines. For example:

   #define FlashRead( udAddrOff ) ( BASE_ADDR[udAddrOff] )

   A function is used here instead to allow the user to expand it if necessary.

Pseudo Code:
   Step 1: Return the value at double-word offset udAddrOff
*******************************************************************************/
uCPUBusType FlashRead( udword udAddrOff ) {
   /* Step 1 Return the value at double-word offset udAddrOff */
   return BASE_ADDR[udAddrOff];
} /* EndFunction FlashRead */





/*******************************************************************************
Function:     ReturnType FlashReadCfi( uword uwCfiFunc, uCPUBusType *ucpCfiValue )
Arguments:    uwCfiFunc is set to the offset of the CFI parameter to be read.
   The CFI value read from offset uwCfiFunc is passed back to the calling 
   function by *ucpCfiValue.

Return Value: The function returns the following conditions: 
   Flash_Success
   Flash_CfiFailed

Description: This function checks whether the flash CFI is present and operable, 
   then reads the CFI value at the specified offset. The CFI value requested is
   then passed back to the calling function.   

Pseudo Code:
   Step 1: Send the Read CFI Instruction  
   Step 2: Check that the CFI interface is operable     
   Step 3: If CFI is operable read the required CFI value
   Step 4: Return the flash to Read Array mode 
*******************************************************************************/
ReturnType FlashReadCfi( uword uwCfiFunc, uCPUBusType *ucpCfiValue ) {
   ReturnType rRetVal = Flash_Success; /* Holds the return value */
   udword udCfiAddr; /* Holds CFI address */

   /* Step 1: Send the Read CFI Instruction */
   FlashWrite( ConvAddr(0x55), (uCPUBusType)CMD(0x0098) ); 

   /* Step 2: Check that the CFI interface is operable */
   if( ((FlashRead( ShAddr(0x00000010) ) & CMD(0x00FF) ) != CMD(0x0051)) ||
       ((FlashRead( ShAddr(0x00000011) ) & CMD(0x00FF) ) != CMD(0x0052)) ||
       ((FlashRead( ShAddr(0x00000012) ) & CMD(0x00FF) ) != CMD(0x0059)) ) 
      rRetVal = Flash_CfiFailed;
   else {
      /* Step 3: Read the required CFI Info */
      udCfiAddr = (udword)uwCfiFunc;
      *ucpCfiValue = FlashRead( ShAddr((udCfiAddr & 0x000000FF)) );  
   } /* EndIf */
   
   FlashReset(); /* Step 4: Return to Read Array mode */
   return rRetVal;
} /* EndFunction FlashReadCfi */





/*******************************************************************************
Function:     ReturnType FlashReadDeviceId( uCPUBusType *ucpDeviceId )
Arguments:    - *ucpDeviceId = <return value> The function returns the Device Code.
   The device code for the part is:
   M29W640FB   0x22FD
   M29W640FT   0x22ED

Note:         In case a common response of more flash chips is not identical the real
   read value will be given (Flash_ResponseUnclear)

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear

Description: This function can be used to read the device code of the flash.

Pseudo Code:
   Step 1:  Send the Auto Select instruction
   Step 2:  Read the DeviceId
   Step 3:  Return the device to Read Array mode
   Step 4:  Check flash response (more flashes could give different results)
*******************************************************************************/
ReturnType FlashReadDeviceId( uCPUBusType *ucpDeviceId ) {

   /* Step 1: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 2: Read the DeviceId */
   *ucpDeviceId = FlashRead(ShAddr(0x1)); /* A0 = 1, A1 = 0 */
   
   /* Step 3: Return to Read Array Mode */
   FlashReset();

   /* Step 4: Check flash response (more flashes could give different results) */
   return FlashResponseIntegrityCheck( ucpDeviceId );    

} /* EndFunction FlashReadDeviceId */





/*******************************************************************************
Function: ReturnType FlashReadExtendedBlockVerifyCode( uCPUBusType *ucpVerifyCode )
Arguments:    - *ucpVerifyCode = <return value>
                 The function returns the Extended Memory Block Verify Code.

   The Extended Memory Block Verify Code for the part are:

   M29W640FB/T   
   		0x80 (factory locked)
            0x00 (not factory locked)

Note:         In case a common response of more flash chips is not identical the real
   read value will be given (Flash_ResponseUnclear)

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear

Description: This function can be used to read the Extended Memory Block Verify Code.
   The verify code is used to specify if the Extended Memory Block was locked/not locked
   by the manufacturer.

Pseudo Code:
   Step 1:  Send the Auto Select instruction
   Step 2:  Read the Extended Memory Block Verify Code
   Step 3:  Return the device to Read Array mode
   Step 4:  Check flash response (more flashes could give different results)
*******************************************************************************/
ReturnType FlashReadExtendedBlockVerifyCode( uCPUBusType *ucpVerifyCode ) {

   /* Step 1: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 2: Read the Extended Memory Block Verify Code */
   *ucpVerifyCode = FlashRead(ShAddr(0x3)); /* A0 = 1, A1 = 1 */
   
   /* Step 3: Return to Read Array Mode */
   FlashReset();

   /* Step 4: Check flash response (more flashes could give different results) */
   return FlashResponseIntegrityCheck( ucpVerifyCode );    

} /* EndFunction FlashReadExtendedBlockVerifyCode */





/*******************************************************************************
Function:     ReturnType FlashReadManufacturerCode( uCPUBusType *ucpManufacturerCode )
Arguments:    - *ucpManufacturerCode = <return value> The function returns 
   the manufacture code (for ST = 0x0020). 
   In case a common response of more flash chips is not identical the real
   read value will be given (Flash_ResponseUnclear)

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear

Description:   This function can be used to read the manufacture code of the flash.

Pseudo Code:
   Step 1:  Send the Auto Select instruction
   Step 2:  Read the Manufacturer Code
   Step 3:  Return the device to Read Array mode
   Step 4:  Check flash response (more flashes could give different results)
*******************************************************************************/
ReturnType FlashReadManufacturerCode( uCPUBusType *ucpManufacturerCode ) {

   /* Step 1: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 2: Read the DeviceId */
   *ucpManufacturerCode = FlashRead( ShAddr(0x0) ); /* A0 = 0, A1 = 0 */
   
   /* Step 3: Return to Read Array Mode */
   FlashReset();

   /* Step 4: Check flash response (more flashes could give different results) */
   return FlashResponseIntegrityCheck( ucpManufacturerCode );    

} /* EndFunction FlashReadManufacturerCode */





/*******************************************************************************
Function:      void FlashReset( void )
Arguments:     none
Return Value:  Flash_Success
Description:   This function places the flash in the Read Array mode described
   in the Data Sheet. In this mode the flash can be read as normal memory.

   All of the other functions leave the flash in the Read Array mode so this is
   not strictly necessary. It is provided for completeness and in case of
   problems.

Pseudo Code:
   Step 1: write command sequence (see Instructions Table of the Data Sheet)
*******************************************************************************/
ReturnType FlashReset( void ) {

   /* Step 1: write command sequence */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* 3rd Cycle: write 0x00F0 to ANY address */
   return Flash_Success;

} /* EndFunction FlashReset */





/*******************************************************************************
Function:     ReturnType FlashResponseIntegrityCheck(uCPUBusType *ucpFlashResponse)
Arguments:    - ucpFlashResponse <parameter> + <return value> 
   The function returns a unique value in case one flash or an
   array of flashes return all the same value (Consistent Response = Flash_Success).
   In case an array of flashes returns different values the function returns the
   received response without any changes (Inconsistent Response = Flash_ResponseUnclear).

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear
               
Description:   This function is used to create one response in multi flash
   environments, instead of giving multiple answers of the single flash
   devices. 

   For example: Using a 32bit CPU and two 16bit Flash devices, the device Id
   would be directly read: 00170017h, because each device gives an answer
   within the range of the databus. In order to give a simple response
   like 00000017h in all possible configurations, this subroutine is used. 

   In case the two devices give different results for the device Id, the
   answer would then be: 00150017h. This allows debugging and helps to
   discover multiple flash configuration problems.
       
Pseudo Code: 
   Step 1:  Extract the first single flash response 
   Step 2:  Compare all next possible flash responses with the first one
   Step 3a: Return all flash responses in case of different values
   Step 3b: Return only the first single flash response in case of matching values 
   
*******************************************************************************/
ReturnType FlashResponseIntegrityCheck(uCPUBusType *ucpFlashResponse) {
   ubyte a;
   union {
      uCPUBusType ucFlashResponse;
      ubyte       ubBytes[sizeof(uCPUBusType)];
   } FullResponse;

   union {
      uCPUBusType ucSingleResponse;
      ubyte       ubBytes[FLASH_BIT_DEPTH/8];
   } SingleResponse;

   SingleResponse.ucSingleResponse = 0;
   FullResponse.ucFlashResponse    = *ucpFlashResponse;

   /* Step 1: Extract the first single flash response */
   memcpy(SingleResponse.ubBytes, FullResponse.ubBytes, FLASH_BIT_DEPTH/8);

   /* Step 2: Compare all next possible flash responses with the first one */
   for (a = 0; a < sizeof(uCPUBusType); a += FLASH_BIT_DEPTH/8) {
      if (memcmp (&FullResponse.ubBytes[a], SingleResponse.ubBytes, FLASH_BIT_DEPTH/8) != 0)
         /* Step 3a: Return all flash responses in case of different values */
         return Flash_ResponseUnclear;
   } /* Next a */                                                                 
   
   /* Step 3b: Return only the first single flash response in case of matching values */
   *ucpFlashResponse = SingleResponse.ucSingleResponse;
   return Flash_Success;
} /* EndFunction FlashResponseIntegrityCheck */





/******************************************************************************* 
Function:      ReturnType FlashResume( void ) 
Arguments:     none 
Return Value:  The function returns the following conditions: 
   Flash_Success

Description:   This function resume a suspended operation.
 
Pseudo Code:
   Step 1:     Send the Erase resume command to the device
*******************************************************************************/ 
ReturnType FlashResume( void ) { 

   /* Step 1: Send the Erase Resume command */
   FlashWrite( ANY_ADDR,CMD(0x0030) );
   return Flash_Success;

} /* EndFunction FlashResume */ 





/*******************************************************************************
Function:     ReturnType FlashSingleProgram( udword udAddrOff, uCPUBusType ucVal )
Arguments:    udAddrOff is the offset in the flash to write to.
              ucVal is the value to be written
Return Value: The function returns the following conditions: 
   Flash_Success
   Flash_AddressInvalid
   Flash_BlockProtected
   Flash_ProgramFailed

Description: This function is used to write a single element to the flash.

Pseudo Code:
   Step 1: Check the offset range is valid 
   Step 2: Check if the start block is protected
   Step 3: Program sequence command 
   Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller has 
           completed
   Step 5: Return to Read Mode (if an error occurred) 
*******************************************************************************/
ReturnType FlashSingleProgram( udword udAddrOff, uCPUBusType ucVal) { 
   uBlockType ublCurBlock;
   
   /* Step 1: Check the offset and range are valid */
   if( udAddrOff >= FLASH_SIZE )
      return Flash_AddressInvalid;
      
   /* compute the start block */
   for (ublCurBlock=0; ublCurBlock < NUM_BLOCKS-1;ublCurBlock++)
      if (udAddrOff < BlockOffset[ublCurBlock+1])
         break;

   /* Step 2: Check if the start block is protected */
   if (FlashCheckBlockProtection(ublCurBlock)== Flash_BlockProtected) {
      return Flash_BlockProtected;
   } /* EndIf */         

   /*Step 3: Program sequence command */

   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00A0) ); /* Program command */
   FlashWrite( udAddrOff,ucVal ); /* Program val */                              

   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller
              has completed */

   /* See Data Toggle Flow Chart of the Data Sheet */
   if( FlashDataToggle() != Flash_Success) {

      /* Step 5: Return to Read Mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      return Flash_ProgramFailed ;
   } /* EndIf */
   return Flash_Success;

} /* EndFunction FlashSingleProgram */





/******************************************************************************* 
Function:     ReturnType FlashSuspend( void ) 
Arguments:    none 
Return Value: The function returns the following conditions: 
   Flash_Success

Description:  This function suspends an operation.

Pseudo Code:
   Step 1:  Send the Erase suspend command to the device
*******************************************************************************/ 
ReturnType FlashSuspend( void ) { 

   /* Step 1: Send the Erase Suspend command */
   FlashWrite( ANY_ADDR,CMD(0x00B0) );   
   return Flash_Success;

} /* EndFunction FlashSuspend */ 






/*******************************************************************************
Function:     ReturnType FlashTimeOut(udword udSeconds)

Arguments:    fSeconds holds the number of seconds before giving a TimeOut 
Return Value: The function returns the following conditions:  
   Flash_OperationTimeOut   
   Flash_OperationOngoing 

Example:   FlashTimeOut(0)  // Initializes the Timer

           While(1) {
              ...
              If (FlashTimeOut(5) == Flash_OperationTimeOut) break;
              // The loop is executed for 5 Seconds before leaving it
           } EndWhile

*******************************************************************************/
#ifdef TIME_H_EXISTS
/*-----------------------------------------------------------------------------
Description:   This function realizes a timeout for flash polling actions or
   other operations which would otherwise never return.
   The Routine uses the function clock() inside ANSI C library "time.h". 
-----------------------------------------------------------------------------*/
ReturnType FlashTimeOut(udword udSeconds){
   static clock_t clkReset,clkCount;

   if (udSeconds == 0) { /* Set Timeout to 0 */
      clkReset=clock();
   } /* EndIf */

   clkCount = clock() - clkReset;

   if (clkCount<(CLOCKS_PER_SEC*(clock_t)udSeconds))
      return Flash_OperationOngoing;
   else
      return Flash_OperationTimeOut;
} /* EndFunction FlashTimeOut */

#else 
/*-----------------------------------------------------------------------------
Description:   This function realizes a timeout for flash polling actions or
   other operations which would otherwise never return.
   The Routine uses COUNT_FOR_A_SECOND which describes the performance of 
   the current Hardware. If I count in a loop to COUNT_FOR_A_SECOND
   I would reach 1 Second. Needs to be adopted to the current Hardware. 
-----------------------------------------------------------------------------*/
ReturnType FlashTimeOut(udword udSeconds) {

   static udword udCounter;

   if (udSeconds == 0) { /* Set Timeout to 0 */
      udCounter = 0;
   } /* EndIf */

   if (udCounter == (udSeconds * COUNT_FOR_A_SECOND)) {
      return Flash_OperationTimeOut;
   } else {
      udCounter++;
      return Flash_OperationOngoing;
   } /* EndIf */

} /* EndFunction FlashTimeOut */
#endif /* TIME_H_EXISTS */





/*******************************************************************************
Function:     void FlashWrite( udword udAddrOff, uCPUBusType ucVal )
Arguments:    udAddrOff is double-word offset in the flash to write to.
   ucVal is the value to be written
Return Value: None
Description:  This function is used to write a uCPUBusType to the flash.
*******************************************************************************/
void FlashWrite( udword udAddrOff, uCPUBusType ucVal ) {
   /* Write ucVal to the double-word offset in flash */
   BASE_ADDR[udAddrOff] = ucVal;
} /* EndFunction FlashWrite */




/*******************************************************************************
Function:     ReturnType  FlashUnlockBypassProgram (udword udAddrOff, udword NumWords, 
                          void *pArray)

Arguments:    udAddrOff is the word offset into the flash to be programmed. 
   NumWords holds the number of words in the array.
   pArray is a pointer to the array to be programmed.
             
Return Value: The function returns the following conditions: 
   Flash_Success
   Flash_ProgramFailed
   Flash_AddressInvalid

   When all addresses are successfully programmed the function returns Flash_Success.
   The function returns Flash_ProgramFail if a programming failure occurs:
   udFirstAddrOffProgramFailed will be filled with the first address on which
   the program operation has failed and the functions does not continue to program
   on the remaining addresses.
   If part of the address range to be programmed falls within a protected block,
   the function returns  nothing is programmed and the function no error return. 
   If the address range to be programmed exceeds the address range of the Flash
   Device the function returns Flash_AddressInvalid and nothing is programmed.

Description:  This function is used to program an array into the flash. It does
   not erase the flash first and may fail if the block(s) are not erased first.
   This function can be used only when the device is in unlock bypass mode. The memory 
   offers accellerated program operations through the Vpp pin. When the system asserts 
   Vpp on Vpp pin the memory enters the unlock bypass mode.

Pseudo Code:
   Step 1: Check the offset range is valid
   Step 2: Check that the block(s) to be programmed are not protected
   Step 3: Send the Unlock Bypass command
   Step 4: While there is more to be programmed
   Step 5: Program the next word
   Step 6: Follow Data Toggle Flow Chart until Program/Erase Controller has 
           completed
   Step 7: Return to Read Mode (if an error occurred)
   Step 8: Send the Unlock Bypass Reset command
*******************************************************************************/
ReturnType FlashUnlockBypassProgram (udword udAddrOff, udword NumWords, void *pArray){
   udword  udLastOff;
   uCPUBusType *ucpArrayPointer;  
   ReturnType rRetVal = Flash_Success; /* Return Value: Initially optimistic */ 

   udLastOff = udAddrOff + NumWords- 1;

   /* Step 1: Check that the offset and range are valid */   
   if( udLastOff >= FLASH_SIZE )
      return Flash_AddressInvalid;


   /* Step 4: While there is more to be programmed */
   ucpArrayPointer = (uCPUBusType *)pArray;
   while( udAddrOff <= udLastOff ){
      /* Step 5: Unlock Bypass Program the next word */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00A0) ); /* 1st cycle */
      FlashWrite( udAddrOff++,*ucpArrayPointer++ ); /* Program value */                              

      /* Step 6: Follow Data Toggle Flow Chart until Program/Erase Controller
                 has completed */

      /* See Data Toggle Flow Chart of the Data Sheet */

      if( FlashDataToggle() != Flash_Success){

         /* Step 7: Return to Read Mode (if an error occurred) */
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         rRetVal=Flash_ProgramFailed ;
	 eiErrorInfo.udGeneralInfo[0]=udAddrOff-1;
         break;
      } /* EndIf */

   } /* EndWhile */

   /* Step 8: Send the Unlock Bypass Reset command */
   FlashWrite( ANY_ADDR, CMD(0x0090) ); /* 1st Cycle */
   FlashWrite( ANY_ADDR, CMD(0x0000) ); /* 2nd Cycle */
   return rRetVal;

} /* EndFunction FlashUnlockBypassProgram */




/*******************************************************************************
Function:     void FlashUnlockBypassReset (void);
Arguments:    None
Return Value: None
Description:  This function is used to send the Unlock Bypass Reset command to the device
Pseudo Code:
   Step 1:  Send the Unlock Bypass Reset command to the device
*******************************************************************************/
void FlashUnlockBypassReset( void ) {

   /* Step 1: Send the Unlock Bypass Reset command */
   FlashWrite( ANY_ADDR, CMD(0x0090) ); /* 1st Cycle */
   FlashWrite( ANY_ADDR, CMD(0x0000) ); /* 2nd Cycle */

} /* EndFunction FlashUnlockBypassReset */


/*******************************************************************************
 End of c2311.c
*******************************************************************************/ 

