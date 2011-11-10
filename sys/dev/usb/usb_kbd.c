/*
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 *
 * (C) Copyright 2006 
 * Adapted to godson pmon by yanhua
 * yanhua ict.ac.cn
 *
 * Part of this source has been derived from the Linux USB
 * project.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 *
 */

#include <pmon.h>
#include <stdlib.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <vm/vm.h>		/* for vtophys */

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>


#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include "usb.h"
#include "devices.h"

/*#define USB_KBD_DEBUG to turn on debug*/
#undef USB_KBD_DEBUG
/*
 * if overwrite_console returns 1, the stdin, stderr and stdout
 * are switched to the serial port, else the settings in the
 * environment are used
 */

#define MAX_USB_KBD 4
struct usb_device *kbd_dev[MAX_USB_KBD+1];
static int kbd_index=0;

static inline int usbdev_to_index(struct usb_device *dev)
{
	int i;

	for(i=0; i<MAX_USB_KBD; i++)
	{
		if(dev == kbd_dev[i])
			return i;
	}
	return i;
}

static void * memscan(void * addr, int c, size_t size)
{
	unsigned char * p = (unsigned char *) addr;

	while (size) {
		if (*p == c)
			return (void *) p;
			p++;
			size--;
	}
	return (void *) p;
}

#ifdef CFG_CONSOLE_OVERWRITE_ROUTINE
extern int overwrite_console (void);
#else
int overwrite_console (void)
{
	return (0);
}
#endif


#ifdef	USB_KBD_DEBUG
#define	USB_KBD_PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define USB_KBD_PRINTF(fmt,args...)
#endif


#define REPEAT_RATE  40/4 /* 40msec -> 25cps */
#define REPEAT_DELAY 10 /* 10 x REAPEAT_RATE = 400msec */

#define NUM_LOCK	0x53
#define CAPS_LOCK 0x39
#define SCROLL_LOCK 0x47


/* Modifier bits */
#define LEFT_CNTR		0
#define LEFT_SHIFT	1
#define LEFT_ALT		2
#define LEFT_GUI		3
#define RIGHT_CNTR	4
#define RIGHT_SHIFT	5
#define RIGHT_ALT		6
#define RIGHT_GUI		7

#define USB_KBD_BUFFER_LEN 0x20  /* size of the keyboardbuffer */

static volatile char usb_kbd_buffer[USB_KBD_BUFFER_LEN];
static volatile int usb_in_pointer = 0;
static volatile int usb_out_pointer = 0;

extern int usb_kbd_available;

static volatile unsigned char _new[MAX_USB_KBD][32] __attribute__((section(".bss"),aligned(128)));
volatile unsigned char *new[MAX_USB_KBD];
unsigned char old[MAX_USB_KBD][8];
int repeat_delay;
#define DEVNAME "usbkbd"
static unsigned char num_lock = 0;
static unsigned char caps_lock = 0;
static unsigned char scroll_lock = 0;

unsigned char usb_kbd_code;
static unsigned char esc_code;
static unsigned char esc_seq;

static unsigned char leds __attribute__ ((aligned (0x4)));

static unsigned char usb_kbd_numkey[] = {
	 '1', '2', '3', '4', '5', '6', '7', '8', '9', '0','\r',0x1b,'\b','\t',' ', '-',
	 '=', '[', ']','\\', '#', ';', '\'', '`', ',', '.', '/'
};
static unsigned char usb_kbd_numkey_shifted[] = {
	 '!', '@', '#', '$', '%', '^', '&', '*', '(', ')','\r',0x1b,'\b','\t',' ', '_',
	 '+', '{', '}', '|', '~', ':', '"', '~', '<', '>', '?'
};

/******************************************************************
 * Queue handling
 ******************************************************************/
/* puts character in the queue and sets up the in and out pointer */
static void usb_kbd_put_queue(char data)
{
	if((usb_in_pointer+1)==USB_KBD_BUFFER_LEN) {
		if(usb_out_pointer==0) {
			return; /* buffer full */
		} else{
			usb_in_pointer=0;
		}
	} else {
		if((usb_in_pointer+1)==usb_out_pointer)
			return; /* buffer full */
		usb_in_pointer++;
	}
	usb_kbd_buffer[usb_in_pointer]=data;
	return;
}

/* test if a character is in the queue */
static int usb_kbd_testc(void)
{
	if(usb_in_pointer==usb_out_pointer)
		return(0); /* no data */
	else
		return(1);
}

/* gets the character from the queue */
static int usb_kbd_getc(void)
{
	char c;
	while(usb_in_pointer==usb_out_pointer);
	if((usb_out_pointer+1)==USB_KBD_BUFFER_LEN)
		usb_out_pointer=0;
	else
		usb_out_pointer++;
	c=usb_kbd_buffer[usb_out_pointer];
	return (int)c;

}

void usb_kbd_poll(void)
{
	if(esc_seq){
		switch(esc_seq){
			case 1:
				usb_kbd_code = 0x1b;
				esc_seq++;
				break;
			case 2:
				usb_kbd_code = '[';
				esc_seq++;
				break;
			case 3:
				usb_kbd_code = esc_code;
				esc_seq = 0;
		}	
	} else {
		if(usb_in_pointer == usb_out_pointer)
			usb_kbd_code = 0;	
		else{
			if(usb_out_pointer +1 == USB_KBD_BUFFER_LEN)
				usb_out_pointer = 0;
			else
				usb_out_pointer ++;	
			usb_kbd_code = usb_kbd_buffer[usb_out_pointer];	
		}
	}
}

/* forward decleration */
static int usb_kbd_probe(struct usb_device *dev, unsigned int ifnum);
static int usb_mice_probe(struct usb_device *dev, unsigned int ifnum);

struct usb_driver usb_kbd_driver = {
	.probe = usb_kbd_probe,
};

struct usb_driver usb_mice_driver = {
	.probe = usb_mice_probe,
};

/* search for keyboard and register it if found */
int drv_usb_kbd_init(void)
{
	int error,i;
	device_t usb_kbd_dev;
	struct usb_device *dev;

	usb_in_pointer=0;
	usb_out_pointer=0;
	/* scan all USB Devices */
	for(i=0;i<USB_MAX_DEVICE;i++) {
		dev=usb_get_dev_index(i); /* get device */
		if(dev->devnum!=-1) {
			if(usb_kbd_probe(dev,0)==1) { /* Ok, we found a keyboard */
				USB_KBD_PRINTF("USB KBD register.\n");
				memset (&usb_kbd_dev, 0, sizeof(device_t));
				strcpy(usb_kbd_dev.name, DEVNAME);
				usb_kbd_dev.flags =  DEV_FLAGS_INPUT | DEV_FLAGS_SYSTEM;
				usb_kbd_dev.putc = NULL;
				usb_kbd_dev.puts = NULL;
				usb_kbd_dev.getc = usb_kbd_getc;
				usb_kbd_dev.tstc = usb_kbd_testc;
				return error;
			}
		}
	}
	/* no USB Keyboard found */
	return -1;
}


/* deregistering the keyboard */
int usb_kbd_deregister(void)
{
	//return device_deregister(DEVNAME);
	return 0;
}

/* set the LEDs. Since this is used in the irq routine, the control job
   is issued with a timeout of 0. This means, that the job is queued without
   waiting for job completion */

static void usb_kbd_setled(struct usb_device *dev)
{
	struct usb_interface_descriptor *iface;
	iface = &dev->config.if_desc[0];
	leds=0;
	if(scroll_lock!=0)
		leds|=1;
	leds<<=1;
	if(caps_lock!=0)
		leds|=1;
	leds<<=1;
	if(num_lock!=0)
		leds|=1;
	usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_SET_REPORT, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
 		0x200, iface->bInterfaceNumber,(void *)&leds, 1, 0);

}


#define CAPITAL_MASK 0x20

#define SEND_ESC_SEQ(c)	\
	esc_code = c;	\
	esc_seq = 1;

/* Translate the scancode in ASCII */
static int usb_kbd_translate(unsigned char scancode,unsigned char modifier,int pressed)
{
	unsigned char keycode;

	if(pressed==0) {
		/* key released */
 		repeat_delay=0;
		return 0;
	}
	if(pressed==2) {
		repeat_delay++;
		if(repeat_delay<REPEAT_DELAY)
			return 0;
		repeat_delay=REPEAT_DELAY;
	}
	keycode=0;
	if((scancode>3) && (scancode<=0x1d)) { /* alpha numeric values */
		keycode=scancode-4 + 0x61;
		if(caps_lock)
			keycode&=~CAPITAL_MASK; /* switch to capital Letters */
		if(((modifier&(1<<LEFT_SHIFT))!=0)||((modifier&(1<<RIGHT_SHIFT))!=0)) {
			if(keycode & CAPITAL_MASK)
				keycode&=~CAPITAL_MASK; /* switch to capital Letters */
			else
				keycode|=CAPITAL_MASK; /* switch to non capital Letters */
		}
		if((modifier &(1<<LEFT_CNTR)) !=0 ||(modifier & (1<<RIGHT_CNTR)) !=0)
			keycode  &= 0x1f;
	}
	if((scancode>0x1d) && (scancode<=0x3A)) {
		if(((modifier&(1<<LEFT_SHIFT))!=0)||((modifier&(1<<RIGHT_SHIFT))!=0))  /* shifted */
			keycode=usb_kbd_numkey_shifted[scancode-0x1e];
		else /* non shifted */
			keycode=usb_kbd_numkey[scancode-0x1e];
	}

	if((scancode>=0x59) && (scancode<=0x62)) {
		if(num_lock)
			keycode=usb_kbd_numkey[scancode-0x59];
		else {
			switch(scancode) {
			case 0x60:
				scancode = 0x52;
				break;
			case 0x5a:
				scancode = 0x51;
				break;
			case 0x5c:
				scancode = 0x50;
				break;
			case 0x5e:
				scancode = 0x4f;
				break;
			default:
				break;
			}
		}
	}

	switch(scancode){
		case 0x52:
			SEND_ESC_SEQ('A'); /*Direction key, UP*/
			break;
		case 0x51:             /*Down*/
			SEND_ESC_SEQ('B');
			break;
		case 0x50:		       /*Left*/
			SEND_ESC_SEQ('D');
			break;
		case 0x4f:             /*Right*/
			SEND_ESC_SEQ('C');
			break;
		case 0x4c: /*Fall through*/
		case 0x63: /*small key*/
			SEND_ESC_SEQ('G'); /*Delete key*/
			break;
		case 0x54: /*small '/' */
			keycode = '/';
			break;
		case 0x55:
			keycode = '*';
			break;
		case 0x56:
			keycode = '-';
			break;
		case 0x57:
			keycode = '+';
			break;
		case 0x58:
			keycode = '\r';
			break;
		default:
			break;
	}		

	if(pressed==1) {
		if(scancode==NUM_LOCK) {
			num_lock=~num_lock;
			return 1;
		}
		if(scancode==CAPS_LOCK) {
			caps_lock=~caps_lock;
			return 1;
		}
		if(scancode==SCROLL_LOCK) {
			scroll_lock=~scroll_lock;
			return 1;
		}
	}
	if(keycode!=0) {
		//USB_KBD_PRINTF("%c",keycode);
		usb_kbd_put_queue(keycode);
	}
	return 0;
}

/* Interrupt service routine */
static int usb_kbd_irq(struct usb_device *dev)
{
	int i,res;
	int k_index=0;

	if((dev->irq_status!=0)||(dev->irq_act_len!=8))
	{
		USB_KBD_PRINTF("usb_keyboard Error %lX, len %d\n",dev->irq_status,dev->irq_act_len);
		return 1;
	}

	k_index = usbdev_to_index(dev);
	if(k_index >= MAX_USB_KBD)
		return 1;
	res=0;
	for (i = 2; i < 8; i++) {
		if (old[i] > 3 && memscan(&new[k_index][2], old[k_index][i], 6) == &new[k_index][8]) {
			res|=usb_kbd_translate(old[k_index][i],new[k_index][0],0);
		}
		if (new[k_index][i] > 3 && memscan(&old[k_index][2], new[k_index][i], 6) == &old[k_index][8]) {
			res|=usb_kbd_translate(new[k_index][i],new[k_index][0],1);
		}
	}
	if((new[k_index][2]>3) && (old[k_index][2]==new[k_index][2])) /* still pressed */
		res|=usb_kbd_translate(new[k_index][2],new[k_index][0],2);
#if 0
	if(res==1)
		usb_kbd_setled(dev);
#endif
	memcpy(&old[k_index][0],&new[k_index][0], 8);

	return 1; /* install IRQ Handler again */
}

/* probes the USB device dev for keyboard type */
static int usb_kbd_probe(struct usb_device *dev, unsigned int ifnum)
{
	struct usb_interface_descriptor *iface;
	struct usb_endpoint_descriptor *ep;
	int pipe,maxp;
	int k_index;

	if (dev->descriptor.bNumConfigurations != 1) return 0;
	iface = &dev->config.if_desc[ifnum];

	if (iface->bInterfaceClass != 3) return 0;
	if (iface->bInterfaceSubClass != 1) return 0;
	if (iface->bInterfaceProtocol != 1) return 0;
	if (iface->bNumEndpoints != 1) return 0;

	/* Now probed a usb kbd */
	if(kbd_index >= MAX_USB_KBD -1) {
		printf("Waring!! too much kbd attached\n");
		return 0;
	}
	kbd_dev[kbd_index] = dev;
	memset(_new[kbd_index], 0, sizeof(_new[kbd_index]));
	pci_sync_cache(0, (vm_offset_t)_new[kbd_index], sizeof(_new[kbd_index]), SYNC_W);
	new[kbd_index] = (volatile unsigned char *)CACHED_TO_UNCACHED(&_new[kbd_index]);

	k_index = kbd_index;
	kbd_index ++;
	
	ep = &iface->ep_desc[0];

	if (!(ep->bEndpointAddress & 0x80)) return 0;
	if ((ep->bmAttributes & 3) != 3) return 0;
	USB_KBD_PRINTF("USB KBD found set protocol...\n");
	/* ok, we found a USB Keyboard, install it */
	/* usb_kbd_get_hid_desc(dev); */
	usb_set_protocol(dev, iface->bInterfaceNumber, 0);
	USB_KBD_PRINTF("USB KBD found set idle...\n");
	usb_set_idle(dev, iface->bInterfaceNumber, REPEAT_RATE, 0);
	memset(&new[k_index][0], 0, 8);
	memset(&old[k_index][0], 0, 8);
	repeat_delay=0;
	pipe = usb_rcvintpipe(dev, ep->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe);
	dev->irq_handle= usb_kbd_irq;
	USB_KBD_PRINTF("USB KBD enable interrupt pipe...\n");
	usb_submit_int_msg(dev,pipe,&new[k_index][0], maxp > 8 ? 8 : maxp,ep->bInterval);
	usb_kbd_available  = 1;
	return 1;
}

/* probes the USB mice device*/
static int usb_mice_probe(struct usb_device *dev, unsigned int ifnum)
{
	struct usb_interface_descriptor *iface;
	struct usb_endpoint_descriptor *ep;
	//int pipe,maxp;

	if (dev->descriptor.bNumConfigurations != 1) return 0;
	iface = &dev->config.if_desc[ifnum];

	if (iface->bInterfaceClass != 3) return 0;
	if (iface->bInterfaceSubClass != 1) return 0;
	if (iface->bInterfaceProtocol != 2) return 0;
	if (iface->bNumEndpoints != 1) return 0;

	return 0;
}

static void init_kbd_driver(void) __attribute__((constructor));

extern void usb_driver_register(struct usb_driver *driver);

static void init_kbd_driver(void)
{
	usb_driver_register(&usb_kbd_driver);
}

#ifdef __not_used__
struct usb_hid_descriptor {
	unsigned char  bLength;
	unsigned char  bDescriptorType; /* 0x21 for HID */
	unsigned short bcdHID; /* release number */
	unsigned char  bCountryCode;
	unsigned char  bNumDescriptors;
	unsigned char  bReportDescriptorType;
	unsigned short wDescriptorLength;
} __attribute__ ((packed));

/*
 * We parse each description item into this structure. Short items data
 * values are expanded to 32-bit signed int, long items contain a pointer
 * into the data area.
 */

struct hid_item {
	unsigned char format;
	unsigned char size;
	unsigned char type;
	unsigned char tag;
	union {
	    unsigned char   u8;
	    char            s8;
	    unsigned short  u16;
	    short           s16;
	    unsigned long   u32;
	    long            s32;
	    unsigned char  *longdata;
	} data;
};

/*
 * HID report item format
 */

#define HID_ITEM_FORMAT_SHORT	0
#define HID_ITEM_FORMAT_LONG	1

/*
 * Special tag indicating long items
 */

#define HID_ITEM_TAG_LONG	15


static struct usb_hid_descriptor usb_kbd_hid_desc;

void usb_kbd_display_hid(struct usb_hid_descriptor *hid)
{
	printf("USB_HID_DESC:\n");
	printf("  bLenght               0x%x\n",hid->bLength);
	printf("  bcdHID                0x%x\n",hid->bcdHID);
	printf("  bCountryCode          %d\n",hid->bCountryCode);
	printf("  bNumDescriptors       0x%x\n",hid->bNumDescriptors);
	printf("  bReportDescriptorType 0x%x\n",hid->bReportDescriptorType);
	printf("  wDescriptorLength     0x%x\n",hid->wDescriptorLength);
}


/*
 * Fetch a report description item from the data stream. We support long
 * items, though they are not used yet.
 */

static int fetch_item(unsigned char *start,unsigned char *end, struct hid_item *item)
{
	if((end - start) > 0) {
		unsigned char b = *start++;
		item->type = (b >> 2) & 3;
		item->tag  = (b >> 4) & 15;
		if (item->tag == HID_ITEM_TAG_LONG) {
			item->format = HID_ITEM_FORMAT_LONG;
			if ((end - start) >= 2) {
				item->size = *start++;
				item->tag  = *start++;
				if ((end - start) >= item->size) {
					item->data.longdata = start;
					start += item->size;
					return item->size;
				}
			}
		} else {
			item->format = HID_ITEM_FORMAT_SHORT;
			item->size = b & 3;
			switch (item->size) {
				case 0:
					return item->size;
				case 1:
					if ((end - start) >= 1) {
						item->data.u8 = *start++;
						return item->size;
					}
					break;
				case 2:
					if ((end - start) >= 2) {
						item->data.u16 = swap_16((unsigned short *)start);
						start+=2;
						return item->size;
					}
				case 3:
					item->size++;
					if ((end - start) >= 4) {
						item->data.u32 = swap_32((unsigned long *)start);
						start+=4;
						return item->size;
					}
			}
		}
	}
	return -1;
}

/*
 * HID report descriptor item type (prefix bit 2,3)
 */

#define HID_ITEM_TYPE_MAIN		0
#define HID_ITEM_TYPE_GLOBAL		1
#define HID_ITEM_TYPE_LOCAL		2
#define HID_ITEM_TYPE_RESERVED		3
/*
 * HID report descriptor main item tags
 */

#define HID_MAIN_ITEM_TAG_INPUT			8
#define HID_MAIN_ITEM_TAG_OUTPUT		9
#define HID_MAIN_ITEM_TAG_FEATURE		11
#define HID_MAIN_ITEM_TAG_BEGIN_COLLECTION	10
#define HID_MAIN_ITEM_TAG_END_COLLECTION	12
/*
 * HID report descriptor main item contents
 */

#define HID_MAIN_ITEM_CONSTANT		0x001
#define HID_MAIN_ITEM_VARIABLE		0x002
#define HID_MAIN_ITEM_RELATIVE		0x004
#define HID_MAIN_ITEM_WRAP		0x008
#define HID_MAIN_ITEM_NONLINEAR		0x010
#define HID_MAIN_ITEM_NO_PREFERRED	0x020
#define HID_MAIN_ITEM_NULL_STATE	0x040
#define HID_MAIN_ITEM_VOLATILE		0x080
#define HID_MAIN_ITEM_BUFFERED_BYTE	0x100

/*
 * HID report descriptor collection item types
 */

#define HID_COLLECTION_PHYSICAL		0
#define HID_COLLECTION_APPLICATION	1
#define HID_COLLECTION_LOGICAL		2
/*
 * HID report descriptor global item tags
 */

#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE		0
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM	1
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM	2
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM	3
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM	4
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT	5
#define HID_GLOBAL_ITEM_TAG_UNIT		6
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE		7
#define HID_GLOBAL_ITEM_TAG_REPORT_ID		8
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT	9
#define HID_GLOBAL_ITEM_TAG_PUSH		10
#define HID_GLOBAL_ITEM_TAG_POP			11

/*
 * HID report descriptor local item tags
 */

#define HID_LOCAL_ITEM_TAG_USAGE		0
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM	1
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM	2
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX	3
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM	4
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM	5
#define HID_LOCAL_ITEM_TAG_STRING_INDEX		7
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM	8
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM	9
#define HID_LOCAL_ITEM_TAG_DELIMITER		10


static void usb_kbd_show_item(struct hid_item *item)
{
	switch(item->type) {
		case HID_ITEM_TYPE_MAIN:
			switch(item->tag) {
				case HID_MAIN_ITEM_TAG_INPUT:
					printf("Main Input");
					break;
				case HID_MAIN_ITEM_TAG_OUTPUT:
					printf("Main Output");
					break;
				case HID_MAIN_ITEM_TAG_FEATURE:
					printf("Main Feature");
					break;
				case HID_MAIN_ITEM_TAG_BEGIN_COLLECTION:
					printf("Main Begin Collection");
					break;
				case HID_MAIN_ITEM_TAG_END_COLLECTION:
					printf("Main End Collection");
					break;
				default:
					printf("Main reserved %d",item->tag);
					break;
			}
			break;
		case HID_ITEM_TYPE_GLOBAL:
			switch(item->tag) {
				case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
					printf("- Global Usage Page");
					break;
				case HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM:
					printf("- Global Logical Minimum");
					break;
				case HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
					printf("- Global Logical Maximum");
					break;
				case HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM:
					printf("- Global physical Minimum");
					break;
				case HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM:
					printf("- Global physical Maximum");
					break;
				case HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT:
					printf("- Global Unit Exponent");
					break;
				case HID_GLOBAL_ITEM_TAG_UNIT:
					printf("- Global Unit");
					break;
				case HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
					printf("- Global Report Size");
					break;
				case HID_GLOBAL_ITEM_TAG_REPORT_ID:
					printf("- Global Report ID");
					break;
				case HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
					printf("- Global Report Count");
					break;
				case HID_GLOBAL_ITEM_TAG_PUSH:
					printf("- Global Push");
					break;
				case HID_GLOBAL_ITEM_TAG_POP:
					printf("- Global Pop");
					break;
				default:
					printf("- Global reserved %d",item->tag);
					break;
			}
			break;
		case HID_ITEM_TYPE_LOCAL:
			switch(item->tag) {
				case HID_LOCAL_ITEM_TAG_USAGE:
					printf("-- Local Usage");
					break;
				case HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:
					printf("-- Local Usage Minimum");
					break;
				case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:
					printf("-- Local Usage Maximum");
					break;
				case HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX:
					printf("-- Local Designator Index");
					break;
				case HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM:
					printf("-- Local Designator Minimum");
					break;
				case HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM:
					printf("-- Local Designator Maximum");
					break;
				case HID_LOCAL_ITEM_TAG_STRING_INDEX:
					printf("-- Local String Index");
					break;
				case HID_LOCAL_ITEM_TAG_STRING_MINIMUM:
					printf("-- Local String Minimum");
					break;
				case HID_LOCAL_ITEM_TAG_STRING_MAXIMUM:
					printf("-- Local String Maximum");
					break;
				case HID_LOCAL_ITEM_TAG_DELIMITER:
					printf("-- Local Delimiter");
					break;
				default:
					printf("-- Local reserved %d",item->tag);
					break;
			}
			break;
		default:
			printf("--- reserved %d",item->type);
			break;
	}
	switch(item->size) {
		case 1:
			printf("  %d",item->data.u8);
			break;
		case 2:
			printf("  %d",item->data.u16);
			break;
		case 4:
			printf("  %ld",item->data.u32);
			break;
	}
	printf("\n");
}


static int usb_kbd_get_hid_desc(struct usb_device *dev)
{
	unsigned char buffer[256];
	struct usb_descriptor_header *head;
	struct usb_config_descriptor *config;
	int index,len,i;
	unsigned char *start, *end;
	struct hid_item item;

	if(usb_get_configuration_no(dev,&buffer[0],0)==-1)
		return -1;
	head =(struct usb_descriptor_header *)&buffer[0];
	if(head->bDescriptorType!=USB_DT_CONFIG) {
		printf(" ERROR: NOT USB_CONFIG_DESC %x\n",head->bDescriptorType);
		return -1;
	}
	index=head->bLength;
	config=(struct usb_config_descriptor *)&buffer[0];
	len=swap_16(config->wTotalLength);
	/* Ok the first entry must be a configuration entry, now process the others */
	head=(struct usb_descriptor_header *)&buffer[index];
	while(index+1 < len) {
		if(head->bDescriptorType==USB_DT_HID) {
			printf("HID desc found\n");
			memcpy(&usb_kbd_hid_desc,&buffer[index],buffer[index]);
			usb_kbd_hid_desc.bcdHID=swap_16(usb_kbd_hid_desc.bcdHID);
			usb_kbd_hid_desc.wDescriptorLength=swap_16(usb_kbd_hid_desc.wDescriptorLength);
			usb_kbd_display_hid(&usb_kbd_hid_desc);
			len=0;
			break;
		}
		index+=head->bLength;
		head=(struct usb_descriptor_header *)&buffer[index];
	}
	if(len>0)
		return -1;
	len=usb_kbd_hid_desc.wDescriptorLength;
	if((index = usb_get_class_descriptor(dev, 0, USB_DT_REPORT, 0, &buffer[0], len)) < 0) {
		printf("reading report descriptor failed\n");
		return -1;
	}
	printf(" report descriptor (size %u, read %d)\n", len, index);
	start=&buffer[0];
	end=&buffer[len];
	i=0;
	do {
		index=fetch_item(start,end,&item);
		i+=index;
		i++;
		if(index>=0)
			usb_kbd_show_item(&item);

		start+=index;
		start++;
	} while(index>=0);

}

#endif

