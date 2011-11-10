////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Internal macros                                                            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define _F_START(f)             (0 ? f)
#define _F_END(f)               (1 ? f)
#define _F_SIZE(f)              (1 + _F_END(f) - _F_START(f))
#define _F_MASK(f)              (((1 << _F_SIZE(f)) - 1) << _F_START(f))
#define _F_NORMALIZE(v, f)      (((v) & _F_MASK(f)) >> _F_START(f))
#define _F_DENORMALIZE(v, f)    (((v) << _F_START(f)) & _F_MASK(f))

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Global macros                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define FIELD_GET(x, reg, field) \
( \
    _F_NORMALIZE((x), reg ## _ ## field) \
)

#define FIELD_SET(x, reg, field, value) \
( \
    (x & ~_F_MASK(reg ## _ ## field)) \
    | _F_DENORMALIZE(reg ## _ ## field ## _ ## value, reg ## _ ## field) \
)

#define FIELD_VALUE(x, reg, field, value) \
( \
    (x & ~_F_MASK(reg ## _ ## field)) \
    | _F_DENORMALIZE(value, reg ## _ ## field) \
)

#define FIELD_CLEAR(reg, field) \
( \
    ~ _F_MASK(reg ## _ ## field) \
)

static int smtc_de_busy;

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Field Macros                                                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define FIELD_START(field)              (0 ? field)
#define FIELD_END(field)                (1 ? field)
#define FIELD_SIZE(field)               (1 + FIELD_END(field) - FIELD_START(field))
#define FIELD_MASK(field)               (((1 << (FIELD_SIZE(field)-1)) | ((1 << (FIELD_SIZE(field)-1)) - 1)) << FIELD_START(field))
#define FIELD_NORMALIZE(reg, field)     (((reg) & FIELD_MASK(field)) >> FIELD_START(field))
#define FIELD_DENORMALIZE(field, value) (((value) << FIELD_START(field)) & FIELD_MASK(field))

void deWaitForNotBusy(void)
{
	unsigned long i = 0x1000000;
//	return;
	while (i--)
	{
#ifdef CONFIG_FB_SM501
        unsigned long dwVal = regRead32(CMD_INTPR_STATUS);
        if ((FIELD_GET(dwVal, CMD_INTPR_STATUS, 2D_ENGINE)      == CMD_INTPR_STATUS_2D_ENGINE_IDLE) &&
            (FIELD_GET(dwVal, CMD_INTPR_STATUS, 2D_FIFO)        == CMD_INTPR_STATUS_2D_FIFO_EMPTY) &&
            (FIELD_GET(dwVal, CMD_INTPR_STATUS, 2D_SETUP)       == CMD_INTPR_STATUS_2D_SETUP_IDLE) &&
            (FIELD_GET(dwVal, CMD_INTPR_STATUS, CSC_STATUS)     == CMD_INTPR_STATUS_CSC_STATUS_IDLE) &&
            (FIELD_GET(dwVal, CMD_INTPR_STATUS, 2D_MEMORY_FIFO) == CMD_INTPR_STATUS_2D_MEMORY_FIFO_EMPTY) &&
            (FIELD_GET(dwVal, CMD_INTPR_STATUS, COMMAND_FIFO)   == CMD_INTPR_STATUS_COMMAND_FIFO_EMPTY))
            break;
#endif
#ifdef CONFIG_FB_SM7XX
        if ((smtc_seqr(0x16) & 0x18) == 0x10)
            break;
#endif

	}
    	smtc_de_busy = 0;
}

void setPower(unsigned long nGates, unsigned long Clock);

/**********************************************************************
 *
 * deInit
 *
 * Purpose
 *    Drawing engine initialization.
 *
 **********************************************************************/
void deInit(unsigned int nModeWidth, unsigned int nModeHeight, unsigned int bpp)
{
#ifdef CONFIG_FB_SM501
	// Get current power configuration.
	unsigned int gate, clock;

	gate  = regRead32(CURRENT_POWER_GATE);
	clock = regRead32(CURRENT_POWER_CLOCK);

	// Enable 2D Drawing Engine
	gate = FIELD_SET(gate, CURRENT_POWER_GATE, 2D, ENABLE);
	setPower(gate, clock);
#endif
#ifdef CONFIG_FB_SM7XX
{
	// Get current power configuration.
	unsigned char gate, clock;
	clock = smtc_seqr(0x21);
	// Enable 2D Drawing Engine
	smtc_seqw(0x21,clock& 0xF8);
}
#endif
	SMTC_write2Dreg(DE_CLIP_TL,
		FIELD_VALUE(0, DE_CLIP_TL, TOP,     0)       |
		FIELD_SET  (0, DE_CLIP_TL, STATUS,  DISABLE) |
		FIELD_SET  (0, DE_CLIP_TL, INHIBIT, OUTSIDE) |
		FIELD_VALUE(0, DE_CLIP_TL, LEFT,    0));

    SMTC_write2Dreg(DE_PITCH,
		FIELD_VALUE(0, DE_PITCH, DESTINATION, nModeWidth) |
		FIELD_VALUE(0, DE_PITCH, SOURCE,      nModeWidth));

    SMTC_write2Dreg(DE_WINDOW_WIDTH,
		FIELD_VALUE(0, DE_WINDOW_WIDTH, DESTINATION, nModeWidth) |
		FIELD_VALUE(0, DE_WINDOW_WIDTH, SOURCE,      nModeWidth));

    switch (bpp)
    {
    case 8:
        SMTC_write2Dreg(DE_STRETCH_FORMAT,
            FIELD_SET  (0, DE_STRETCH_FORMAT, PATTERN_XY,    NORMAL) |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, PATTERN_Y,     0)      |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, PATTERN_X,     0)      |
            FIELD_SET  (0, DE_STRETCH_FORMAT, PIXEL_FORMAT,  8)      |
            FIELD_SET  (0, DE_STRETCH_FORMAT, ADDRESSING,	 XY)     |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, SOURCE_HEIGHT, 3));
        break;
    case 32:
        SMTC_write2Dreg(DE_STRETCH_FORMAT,
            FIELD_SET  (0, DE_STRETCH_FORMAT, PATTERN_XY,    NORMAL) |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, PATTERN_Y,     0)      |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, PATTERN_X,     0)      |
            FIELD_SET  (0, DE_STRETCH_FORMAT, PIXEL_FORMAT,  32)     |
            FIELD_SET  (0, DE_STRETCH_FORMAT, ADDRESSING,	 XY)     |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, SOURCE_HEIGHT, 3));
        break;
    case 16:
    default:
        SMTC_write2Dreg(DE_STRETCH_FORMAT,
            FIELD_SET  (0, DE_STRETCH_FORMAT, PATTERN_XY,    NORMAL) |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, PATTERN_Y,     0)      |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, PATTERN_X,     0)      |
            FIELD_SET  (0, DE_STRETCH_FORMAT, PIXEL_FORMAT,  16)     |
            FIELD_SET  (0, DE_STRETCH_FORMAT, ADDRESSING,	 XY)     |
            FIELD_VALUE(0, DE_STRETCH_FORMAT, SOURCE_HEIGHT, 3));
        break;
    }

	SMTC_write2Dreg(DE_MASKS,
		FIELD_VALUE(0, DE_MASKS, BYTE_MASK, 0xFFFF) |
		FIELD_VALUE(0, DE_MASKS, BIT_MASK,  0xFFFF));
	SMTC_write2Dreg(DE_COLOR_COMPARE_MASK,
		FIELD_VALUE(0, DE_COLOR_COMPARE_MASK, MASKS, 0xFFFFFF));
	SMTC_write2Dreg(DE_COLOR_COMPARE,
		FIELD_VALUE(0, DE_COLOR_COMPARE, COLOR, 0xFFFFFF));
}
/**********************************************************************
 *
 * deCopy
 *
 * Purpose
 *    Copy a rectangular area of the source surface to a destination surface
 *
 * Remarks
 *       Source bitmap must have the same color depth (BPP) as the destination bitmap.
 *
**********************************************************************/
void deCopy(unsigned long dst_base,
            unsigned long dst_pitch,  
            unsigned long dst_BPP,  
            unsigned long dst_X, 
            unsigned long dst_Y, 
            unsigned long dst_width,
            unsigned long dst_height,
            unsigned long src_base, 
            unsigned long src_pitch,  
            unsigned long src_X, 
            unsigned long src_Y, 
            pTransparent pTransp,
            unsigned char nROP2)
{
    unsigned long nDirection = 0;
    unsigned long nTransparent = 0;
    unsigned long opSign = 1;    // Direction of ROP2 operation: 1 = Left to Right, (-1) = Right to Left
    unsigned long xWidth = 192 / (dst_BPP / 8); // xWidth is in pixels
    unsigned long de_ctrl = 0;
    
    deWaitForNotBusy();

    SMTC_write2Dreg(DE_WINDOW_DESTINATION_BASE, FIELD_VALUE(0, DE_WINDOW_DESTINATION_BASE, ADDRESS, dst_base));

    SMTC_write2Dreg(DE_WINDOW_SOURCE_BASE, FIELD_VALUE(0, DE_WINDOW_SOURCE_BASE, ADDRESS, src_base));

    if (dst_pitch && src_pitch)
    {
        SMTC_write2Dreg(DE_PITCH,
            FIELD_VALUE(0, DE_PITCH, DESTINATION, dst_pitch) |
            FIELD_VALUE(0, DE_PITCH, SOURCE,      src_pitch));
        
        SMTC_write2Dreg(DE_WINDOW_WIDTH,
            FIELD_VALUE(0, DE_WINDOW_WIDTH, DESTINATION, dst_pitch) |
            FIELD_VALUE(0, DE_WINDOW_WIDTH, SOURCE,      src_pitch));
    }
    
    /* Set transparent bits if necessary */
    if (pTransp != NULL)
    {
        nTransparent = pTransp->match | pTransp->select | pTransp->control;
        
        /* Set color compare register */
        SMTC_write2Dreg(DE_COLOR_COMPARE,
            FIELD_VALUE(0, DE_COLOR_COMPARE, COLOR, pTransp->color));
    }
    
    /* Determine direction of operation */
    if (src_Y < dst_Y)
    {
    /* +----------+
    |S         |
    |   +----------+
    |   |      |   |
    |   |      |   |
    +---|------+   |
    |         D|
        +----------+ */
        
        nDirection = BOTTOM_TO_TOP;
    }
    else if (src_Y > dst_Y)
    {
    /* +----------+
    |D         |
    |   +----------+
    |   |      |   |
    |   |      |   |
    +---|------+   |
    |         S|
        +----------+ */
        
        nDirection = TOP_TO_BOTTOM;
    }
    else
    {
        /* src_Y == dst_Y */
        
        if (src_X <= dst_X)
        {
        /* +------+---+------+
        |S     |   |     D|
        |      |   |      |
        |      |   |      |
        |      |   |      |
            +------+---+------+ */
            
            nDirection = RIGHT_TO_LEFT;
        }
        else
        {
            /* src_X > dst_X */
            
            /* +------+---+------+
            |D     |   |     S|
            |      |   |      |
            |      |   |      |
            |      |   |      |
            +------+---+------+ */
            
            nDirection = LEFT_TO_RIGHT;
        }
    }
    
    if ((nDirection == BOTTOM_TO_TOP) || (nDirection == RIGHT_TO_LEFT))
    {
        src_X += dst_width - 1;
        src_Y += dst_height - 1;
        dst_X += dst_width - 1;
        dst_Y += dst_height - 1;
        opSign = (-1);
    }
    
    /* Workaround for 192 byte hw bug */
    if ((nROP2 != 0x0C) && ((dst_width * (dst_BPP / 8)) >= 192))
    {
        /* Perform the ROP2 operation in chunks of (xWidth * dst_height) */
        while (1)
        {
            deWaitForNotBusy();
            SMTC_write2Dreg(DE_SOURCE,
                FIELD_SET  (0, DE_SOURCE, WRAP, DISABLE) |
                FIELD_VALUE(0, DE_SOURCE, X_K1, src_X)   |
                FIELD_VALUE(0, DE_SOURCE, Y_K2, src_Y));
            SMTC_write2Dreg(DE_DESTINATION,
                FIELD_SET  (0, DE_DESTINATION, WRAP, DISABLE) |
                FIELD_VALUE(0, DE_DESTINATION, X,    dst_X)  |
                FIELD_VALUE(0, DE_DESTINATION, Y,    dst_Y));
            SMTC_write2Dreg(DE_DIMENSION,
                FIELD_VALUE(0, DE_DIMENSION, X,    xWidth) |
                FIELD_VALUE(0, DE_DIMENSION, Y_ET, dst_height));
            de_ctrl = FIELD_VALUE(0, DE_CONTROL, ROP, nROP2) |
                nTransparent |
                FIELD_SET(0, DE_CONTROL, ROP_SELECT, ROP2) |
                FIELD_SET(0, DE_CONTROL, COMMAND, BITBLT) |
                ((nDirection == 1) ? FIELD_SET(0, DE_CONTROL, DIRECTION, RIGHT_TO_LEFT)
                : FIELD_SET(0, DE_CONTROL, DIRECTION, LEFT_TO_RIGHT)) |
                FIELD_SET(0, DE_CONTROL, STATUS, START);
            SMTC_write2Dreg(DE_CONTROL, de_ctrl);
            
            src_X += (opSign * xWidth);
            dst_X += (opSign * xWidth);
            dst_width -= xWidth;
            
            if (dst_width <= 0)
            {
                /* ROP2 operation is complete */
                break;
            }
            
            if (xWidth > dst_width)
            {
                xWidth = dst_width;
            }
        }
    }
    else
    {
        deWaitForNotBusy();
        SMTC_write2Dreg(DE_SOURCE,
            FIELD_SET  (0, DE_SOURCE, WRAP, DISABLE) |
            FIELD_VALUE(0, DE_SOURCE, X_K1, src_X)   |
            FIELD_VALUE(0, DE_SOURCE, Y_K2, src_Y));
        SMTC_write2Dreg(DE_DESTINATION,
            FIELD_SET  (0, DE_DESTINATION, WRAP, DISABLE) |
            FIELD_VALUE(0, DE_DESTINATION, X,    dst_X)  |
            FIELD_VALUE(0, DE_DESTINATION, Y,    dst_Y));
        SMTC_write2Dreg(DE_DIMENSION,
            FIELD_VALUE(0, DE_DIMENSION, X,    dst_width) |
            FIELD_VALUE(0, DE_DIMENSION, Y_ET, dst_height));
        de_ctrl = FIELD_VALUE(0, DE_CONTROL, ROP, nROP2) |
            nTransparent |
            FIELD_SET(0, DE_CONTROL, ROP_SELECT, ROP2) |
            FIELD_SET(0, DE_CONTROL, COMMAND, BITBLT) |
            ((nDirection == 1) ? FIELD_SET(0, DE_CONTROL, DIRECTION, RIGHT_TO_LEFT)
            : FIELD_SET(0, DE_CONTROL, DIRECTION, LEFT_TO_RIGHT)) |
            FIELD_SET(0, DE_CONTROL, STATUS, START);
        SMTC_write2Dreg(DE_CONTROL, de_ctrl);
    }

    smtc_de_busy = 1;
}


void deFillRect(unsigned long dst_base,
                unsigned long dst_pitch,  
                unsigned long dst_X, 
                unsigned long dst_Y, 
                unsigned long dst_width, 
                unsigned long dst_height, 
                unsigned long nColor)
{
    deWaitForNotBusy();

    SMTC_write2Dreg(DE_WINDOW_DESTINATION_BASE, FIELD_VALUE(0, DE_WINDOW_DESTINATION_BASE, ADDRESS, dst_base));
    
    if (dst_pitch)
    {
        SMTC_write2Dreg(DE_PITCH,
            FIELD_VALUE(0, DE_PITCH, DESTINATION, dst_pitch) |
            FIELD_VALUE(0, DE_PITCH, SOURCE,      dst_pitch));
        
        SMTC_write2Dreg(DE_WINDOW_WIDTH,
            FIELD_VALUE(0, DE_WINDOW_WIDTH, DESTINATION, dst_pitch) |
            FIELD_VALUE(0, DE_WINDOW_WIDTH, SOURCE,      dst_pitch));
    }

    SMTC_write2Dreg(DE_FOREGROUND,
        FIELD_VALUE(0, DE_FOREGROUND, COLOR, nColor));

    SMTC_write2Dreg(DE_DESTINATION,
        FIELD_SET  (0, DE_DESTINATION, WRAP, DISABLE) |
        FIELD_VALUE(0, DE_DESTINATION, X,    dst_X)     |
        FIELD_VALUE(0, DE_DESTINATION, Y,    dst_Y));

    SMTC_write2Dreg(DE_DIMENSION,
        FIELD_VALUE(0, DE_DIMENSION, X,    dst_width) |
        FIELD_VALUE(0, DE_DIMENSION, Y_ET, dst_height));

    SMTC_write2Dreg(DE_CONTROL,
        FIELD_SET  (0, DE_CONTROL, STATUS,     START)          |
        FIELD_SET  (0, DE_CONTROL, DIRECTION,  LEFT_TO_RIGHT)  |
        FIELD_SET  (0, DE_CONTROL, LAST_PIXEL, OFF)            |
        FIELD_SET  (0, DE_CONTROL, COMMAND,    RECTANGLE_FILL) |
        FIELD_SET  (0, DE_CONTROL, ROP_SELECT, ROP2)           |
        FIELD_VALUE(0, DE_CONTROL, ROP,        0x0C));

    smtc_de_busy = 1;
}

void deCopyModify(unsigned long nbpp,unsigned long nSrcBase,unsigned long nSrcWidth, unsigned long nSrcX, unsigned long nSrcY,
                        unsigned long nDestBase,unsigned long nDestWidth, unsigned long nDestX,unsigned long nDestY,
                        unsigned long nWidth, unsigned long nHeight,  
                        unsigned char nROP2)
{
deCopy(nDestBase,
            0,  
            nbpp,  
            nDestX, 
            nDestY, 
            nWidth,
            nHeight,
            nSrcBase, 
            0,  
            nSrcX, 
            nSrcY, 
            0,
            nROP2);
}


int deFillRectModify( unsigned long nDestBase,unsigned long nX1, unsigned long nY1, unsigned long nX2, unsigned long nY2,
unsigned long nColor)

{
        unsigned long DeltaX;
        unsigned long DeltaY;

   
            /* Determine delta X */
            if (nX2 < nX1)
            {
                    DeltaX = nX1 - nX2 + 1;
                    nX1 = nX2;
            }
            else
            {
                    DeltaX = nX2 - nX1 + 1;
            }

            /* Determine delta Y */
            if (nY2 < nY1)
            {
                    DeltaY = nY1 - nY2 + 1;
                    nY1 = nY2;
            }
            else
            {
                    DeltaY = nY2 - nY1 + 1;
            }

deFillRect(nDestBase,0,
                nX1,  
                nY1, 
                DeltaX, 
                DeltaY, 
                nColor);
}

void AutodeInit()
{
	deInit(FB_XSIZE,FB_YSIZE,FB_COLOR_BITS);
}

void AutodeFillRectModify(int color)
{
	deFillRectModify(0,0,FB_YSIZE-16,FB_XSIZE,FB_YSIZE,color);
}

void AutodeCopyModify(int bpp)
{
	deCopyModify(bpp,0,FB_XSIZE,0,16,0,FB_XSIZE,0,0,FB_XSIZE,FB_YSIZE-16,0x0c);
}
