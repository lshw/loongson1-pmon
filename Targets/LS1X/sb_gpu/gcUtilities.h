#ifndef _gcUTILITIES_H_
#define _gcUTILITIES_H_

/******************************************************************************\
********************************** Bit Macros *********************************
\******************************************************************************/

int _MIN(
	const int n1,
	const int n2
	);

int _MAX(
	const int n1,
	const int n2
	);

// Extract bits from an 8-bit value.
UINT8 GETBITS8(
	const UINT8 Data,
	const int Start,
	const int End
	);

// Extract bits from a 16-bit value.
UINT16 GETBITS16(
	const UINT16 Data,
	const int Start,
	const int End
	);

// Extract bits from a 32-bit value.
UINT32 GETBITS32(
	const UINT32 Data,
	const int Start,
	const int End
	);

// Extract bits from a 64-bit value.
UINT64 GETBITS64(
	const UINT64 Data,
	const int Start,
	const int End
	);

// Set bits in an 8-bit value.
UINT8 SETBITS8(
	UINT8* Data,
	const int Start,
	const int End,
	const UINT8 Value
	);

// Set bits in a 16-bit value.
UINT16 SETBITS16(
	UINT16* Data,
	const int Start,
	const int End,
	const UINT16 Value
	);

// Set bits in a 32-bit value.
UINT32 SETBITS32(
	UINT32* Data,
	const int Start,
	const int End,
	const UINT32 Value
	);

// Set bits in a 64-bit value.
UINT64 SETBITS64(
	UINT64* Data,
	const int Start,
	const int End,
	const UINT64 Value
	);


/******************************************************************************\
********************************* Field Macros *********************************
\******************************************************************************/

#define __START(reg_field)			(0 ? reg_field)
#define __END(reg_field)			(1 ? reg_field)
#define __GETSIZE(reg_field)		(__END(reg_field) - __START(reg_field) + 1)
#define __ALIGN(data, reg_field)	(((UINT32) (data)) << __START(reg_field))
#define __MASK(reg_field)			((UINT32) ((__GETSIZE(reg_field) == 32) \
										?  ~0 \
										: (~(~0 << __GETSIZE(reg_field)))))

/*******************************************************************************
**
**	FIELDMASK
**
**		Get aligned field mask.
**
**	ARGUMENTS:
**
**		reg		Name of register.
**		field	Name of field within register.
*/
#define FIELDMASK(reg, field) \
( \
	__ALIGN(__MASK(reg##_##field), reg##_##field) \
)

/*******************************************************************************
**
**	GETFIELD
**
**		Extract the value of a field from specified data.
**
**	ARGUMENTS:
**
**		data	Data value.
**		reg		Name of register.
**		field	Name of field within register.
*/
#define GETFIELD(data, reg, field) \
( \
	((((UINT32) (data)) >> __START(reg##_##field)) & __MASK(reg##_##field)) \
)

/*******************************************************************************
**
**	SETFIELD
**
**		Set the value of a field within specified data.
**
**	ARGUMENTS:
**
**		data	Data value.
**		reg		Name of register.
**		field	Name of field within register.
**		value	Value for field.
*/
#define SETFIELD(data, reg, field, value)	\
( \
	((UINT32) (data) & ~__ALIGN(__MASK(reg##_##field), reg##_##field)) \
		| \
	__ALIGN((UINT32) (value) & __MASK(reg##_##field), reg##_##field) \
)

/*******************************************************************************
**
**	SETFIELDVALUE
**
**		Set the value of a field within specified data with a
**		predefined value.
**
**	ARGUMENTS:
**
**		data	Data value.
**		reg		Name of register.
**		field	Name of field within register.
**		value	Name of the value within the field.
*/
#define SETFIELDVALUE(data, reg, field, value) \
( \
	((UINT32) (data) & ~__ALIGN(__MASK(reg##_##field), reg##_##field)) \
		| \
	__ALIGN(reg##_##field##_##value & __MASK(reg##_##field), reg##_##field) \
)

/*******************************************************************************
**
**	VERIFYFIELDVALUE
**
**		Verify if the value of a field within specified data equals a
**		predefined value.
**
**	ARGUMENTS:
**
**		data	Data value.
**		reg		Name of register.
**		field	Name of field within register.
**		value	Name of the value within the field.
*/
#define VERIFYFIELDVALUE(data, reg, field, value) \
( \
	(((UINT32) (data)) >> __START(reg##_##field) & __MASK(reg##_##field)) \
		== \
	(reg##_##field##_##value & __MASK(reg##_##field)) \
)


/******************************************************************************\
****************************** Utility Functions ******************************
\******************************************************************************/

void gcAppInit(
	void
	);

UINT32 gcReadReg(
	UINT32 Address
	);

void gcWriteReg(
	UINT32 Address,
	UINT32 Data
	);

void gcPoke(
	UINT32 Address,
	UINT32 Data
	);

UINT32 gcPeek(
	UINT32 Address
	);

UINT32 gcReportIdle(
	char* Message
	);

UINT32 gcReportRegs(void);

UINT32 gcGetNextColor16(
	UINT32 Color
	);

UINT32 gcRand(
	UINT32 Range
	);

UINT32 gcRandExclusive(
	UINT32 Range,
	UINT32 ExcludeValue
	);

void gcNormalizeRect(
	gcRECT* Rect,
	gcRECT* BoundingRect
	);

void gcGenerateRect(
	gcRECT* Rect,
	gcRECT* BoundingRect
	);

void gcPrintRect(
	char* Message,
	gcRECT* Rect
	);

UINT32 gcSurfaceWidth(
	gcSURFACEINFO* Surface
	);

UINT32 gcSurfaceHeight(
	gcSURFACEINFO* Surface
	);

void gcAlignSurface(
	PUINT8* Surface,
	UINT32 Size,
	UINT32 Alignment
	);
	
int GalSaveDIBitmap(const char *filename,
					 unsigned char *bits, 
					 int alignedWidth, 
					 int alignedHeight);

UINT32 printSelectMenu(void);

void gcGenerateRect_Stretch(
	gcRECT* Rect,
	gcRECT* BoundingRect
	);
#endif
