/*
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Note: Part of this code has been derived from linux
 *
 */
#ifndef _USB_H_
#define _USB_H_

#define assert(expression) \
		do {	\
				if(!(expression)){ \
					(void)printf("Assertion %s failed: FILE %s LINE %d\n", \
					#expression, __FILE__, __LINE__); \
				} \
		}while(0)

#include <sys/types.h>
#include <linux/types.h>
#include <sys/queue.h>
#include <sys/device.h>
#include "usb_defs.h"

/* Everything is aribtrary */
#define USB_ALTSETTINGALLOC		4
#define USB_MAXALTSETTING		128	/* Hard limit */

#define USB_MAX_DEVICE			32
#define USB_MAXCONFIG			8
#define USB_MAXINTERFACES		8
#define USB_MAXENDPOINTS		16
#define USB_MAXCHILDREN			8	/* This is arbitrary */
#define USB_MAX_HUB			16

#define USB_CNTL_TIMEOUT 100 /* 100ms timeout */

enum usb_device_speed {
	USB_SPEED_UNKNOWN = 0,          /* enumerating */
	USB_SPEED_LOW, USB_SPEED_FULL,      /* usb 1.1 */
	USB_SPEED_HIGH,             /* usb 2.0 */
	USB_SPEED_VARIABLE,         /* wireless (usb 2.5) */
};

#define BW_HOST_DELAY   1000L       /* nanoseconds */
#define BW_HUB_LS_SETUP 333L        /* nanoseconds */

#define BitTime(bytecount)  (7 * 8 * bytecount / 6)  /* with integer truncation */
    

/* String descriptor */
struct usb_string_descriptor {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned short wData[1];
} __attribute__ ((packed));

/* device request (setup) */
struct devrequest {
	unsigned char requesttype;
	unsigned char request;
	unsigned short value;
	unsigned short index;
	unsigned short length;
} __attribute__ ((packed));


/* All standard descriptors have these 2 fields in common */
struct usb_descriptor_header {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
} __attribute__ ((packed));

/* Device descriptor */
struct usb_device_descriptor {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned short bcdUSB;
	unsigned char  bDeviceClass;
	unsigned char  bDeviceSubClass;
	unsigned char  bDeviceProtocol;
	unsigned char  bMaxPacketSize0;
	unsigned short idVendor;
	unsigned short idProduct;
	unsigned short bcdDevice;
	unsigned char  iManufacturer;
	unsigned char  iProduct;
	unsigned char  iSerialNumber;
	unsigned char  bNumConfigurations;
} __attribute__ ((packed));


/* Endpoint descriptor */
struct usb_endpoint_descriptor {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  bEndpointAddress;
	unsigned char  bmAttributes;
	unsigned short wMaxPacketSize;
	unsigned char  bInterval;
	unsigned char  bRefresh;
	unsigned char  bSynchAddress;

} __attribute__ ((packed));
/* Interface descriptor */
struct usb_interface_descriptor {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  bInterfaceNumber;
	unsigned char  bAlternateSetting;
	unsigned char  bNumEndpoints;
	unsigned char  bInterfaceClass;
	unsigned char  bInterfaceSubClass;
	unsigned char  bInterfaceProtocol;
	unsigned char  iInterface;

	unsigned char  no_of_ep;
	unsigned char  act_altsetting;
	struct usb_endpoint_descriptor ep_desc[USB_MAXENDPOINTS];
} __attribute__ ((packed));


/* Configuration descriptor information.. */
struct usb_config_descriptor {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned short wTotalLength;
	unsigned char  bNumInterfaces;
	unsigned char  bConfigurationValue;
	unsigned char  iConfiguration;
	unsigned char  bmAttributes;
	unsigned char  MaxPower;

	unsigned char  no_of_if;		/* number of interfaces */
	struct usb_interface_descriptor if_desc[USB_MAXINTERFACES];
} __attribute__ ((packed));


struct usb_device {
	int devnum;			/* Device number on USB bus */
	int slow;			/* Slow device? */
	char mf[32];			/* manufacturer */
	char prod[32];			/* product */
	char serial[32];		/* serial number */

	int maxpacketsize;		/* Maximum packet size; encoded as 0,1,2,3 = 8,16,32,64 */
	unsigned int toggle[2];		/* one bit for each endpoint ([0] = IN, [1] = OUT) */
	unsigned int halted[2];		/* endpoint halts; one bit per endpoint # & direction; */
			    /* [0] = IN, [1] = OUT */
	int epmaxpacketin[16];		/* INput endpoint specific maximums */
	int epmaxpacketout[16];		/* OUTput endpoint specific maximums */

	int configno;			/* selected config number */
	struct usb_device_descriptor descriptor; /* Device Descriptor */
	struct usb_config_descriptor config; /* config descriptor */

	int have_langid;		/* whether string_langid is valid yet */
	int string_langid;		/* language ID for strings */
	int (*irq_handle)(struct usb_device *dev);
	unsigned long irq_status;
	int irq_act_len;		/* transfered bytes */
	void *privptr;
	void *qpriv;
	/*
	 * Child devices -  if this is a hub device
	 * Each instance needs its own set of data structures.
	 */
	volatile unsigned long status;
	int act_len;			/* transfered bytes */
	int maxchild;			/* Number of ports if hub */
	struct usb_device *parent;
	struct usb_device *children[USB_MAXCHILDREN];
	void *hc_private;
	void *match;
	void (*destruct)(struct usb_device *);

	/* Rename support*/
	int port;
};

struct usb_driver{
	int (*probe)(struct usb_device *dev, unsigned int ifnum);
	SLIST_ENTRY(usb_driver) d_list;
};

TAILQ_HEAD(hostcontroller, usb_hc);
/**********************************************************************
 * this is how the lowlevel part communicate with the outer world
 */

//#if defined(CONFIG_USB_UHCI) || defined(CONFIG_USB_OHCI) || defined (CONFIG_USB_SL811HS)
#if 1
//int usb_lowlevel_init(void);
//int usb_lowlevel_stop(void);
int submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,int transfer_len);
int submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
			int transfer_len,struct devrequest *setup);
int submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
			int transfer_len, int interval);

/* Defines */
#define USB_UHCI_VEND_ID 0x8086
#define USB_UHCI_DEV_ID	 0x7112

#else
#error USB Lowlevel not defined
#endif

#define USB_MAX_STOR_DEV 5

typedef struct block_dev_desc {
	int		if_type;	/* type of the interface */
	int	        dev;	  	/* device number */
	unsigned char	part_type;  	/* partition type */
	unsigned char	target;		/* target SCSI ID */
	unsigned char	lun;		/* target LUN */
	unsigned char	type;		/* device type */
	unsigned char	removable;	/* removable device */
#ifdef CONFIG_LBA48
	unsigned char	lba48;		/* device can use 48bit addr (ATA/ATAPI v7) */
#endif
	unsigned int	lba;	  	/* number of blocks */
	unsigned long	blksz;		/* block size */
	unsigned char	vendor [40+1]; 	/* IDE model, SCSI Vendor */
	unsigned char	product[20+1];	/* IDE Serial no, SCSI product */
	unsigned char	revision[8+1];	/* firmware revision */
	unsigned long	(*block_read)(int dev,
				      unsigned long start,
				      unsigned long blkcnt,
				      unsigned long *buffer);
}block_dev_desc_t;

block_dev_desc_t *usb_stor_get_dev(int index);
int usb_stor_scan(int mode);
void usb_stor_info(void);

#ifdef CONFIG_USB_KEYBOARD

int drv_usb_kbd_init(void);
int usb_kbd_deregister(void);

#endif
/* routines */
int usb_init(void); /* initialize the USB Controller */
int usb_stop(void); /* stop the USB Controller */


int usb_set_protocol(struct usb_device *dev, int ifnum, int protocol);
int usb_set_idle(struct usb_device *dev, int ifnum, int duration, int report_id);
struct usb_device * usb_get_dev_index(int index);
int usb_control_msg(struct usb_device *dev, unsigned int pipe,
			unsigned char request, unsigned char requesttype,
			unsigned short value, unsigned short index,
			void *data, unsigned short size, int timeout);
int usb_bulk_msg(struct usb_device *dev, unsigned int pipe,
			void *data, int len, int *actual_length, int timeout);
int usb_submit_int_msg(struct usb_device *dev, unsigned long pipe,
			void *buffer,int transfer_len, int interval);
void usb_disable_asynch(int disable);
int usb_maxpacket(struct usb_device *dev,unsigned long pipe);
void __inline__ wait_ms(unsigned long ms);
int usb_get_configuration_no(struct usb_device *dev,unsigned char *buffer,int cfgno);
int usb_get_report(struct usb_device *dev, int ifnum, unsigned char type, unsigned char id, void *buf, int size);
int usb_get_class_descriptor(struct usb_device *dev, int ifnum,
		unsigned char type, unsigned char id, void *buf, int size);
int usb_clear_halt(struct usb_device *dev, int pipe);
int usb_string(struct usb_device *dev, int index, char *buf, size_t size);
int usb_set_interface(struct usb_device *dev, int interface, int alternate);

void usb_scan_devices(void *);

int usb_new_device(struct usb_device *dev);

/* big endian -> little endian conversion */
/* some CPUs are already little endian e.g. the ARM920T */
#define LITTLEENDIAN
#ifdef LITTLEENDIAN
#define swap_16(x) ((unsigned short)(x))
#define swap_32(x) ((unsigned long)(x))
#else
#define swap_16(x) \
	({ unsigned short x_ = (unsigned short)x; \
	 (unsigned short)( \
		((x_ & 0x00FFU) << 8) | ((x_ & 0xFF00U) >> 8) ); \
	})
#define swap_32(x) \
	({ unsigned long x_ = (unsigned long)x; \
	 (unsigned long)( \
		((x_ & 0x000000FFUL) << 24) | \
		((x_ & 0x0000FF00UL) <<	 8) | \
		((x_ & 0x00FF0000UL) >>	 8) | \
		((x_ & 0xFF000000UL) >> 24) ); \
	})
#endif /* LITTLEENDIAN */

/*
 * Calling this entity a "pipe" is glorifying it. A USB pipe
 * is something embarrassingly simple: it basically consists
 * of the following information:
 *  - device number (7 bits)
 *  - endpoint number (4 bits)
 *  - current Data0/1 state (1 bit)
 *  - direction (1 bit)
 *  - speed (1 bit)
 *  - max packet size (2 bits: 8, 16, 32 or 64)
 *  - pipe type (2 bits: control, interrupt, bulk, isochronous)
 *
 * That's 18 bits. Really. Nothing more. And the USB people have
 * documented these eighteen bits as some kind of glorious
 * virtual data structure.
 *
 * Let's not fall in that trap. We'll just encode it as a simple
 * unsigned int. The encoding is:
 *
 *  - max size:		bits 0-1	(00 = 8, 01 = 16, 10 = 32, 11 = 64)
 *  - direction:	bit 7		(0 = Host-to-Device [Out], 1 = Device-to-Host [In])
 *  - device:		bits 8-14
 *  - endpoint:		bits 15-18
 *  - Data0/1:		bit 19
 *  - speed:		bit 26		(0 = Full, 1 = Low Speed)
 *  - pipe type:	bits 30-31	(00 = isochronous, 01 = interrupt, 10 = control, 11 = bulk)
 *
 * Why? Because it's arbitrary, and whatever encoding we select is really
 * up to us. This one happens to share a lot of bit positions with the UHCI
 * specification, so that much of the uhci driver can just mask the bits
 * appropriately.
 */
/* Create various pipes... */
#define create_pipe(dev,endpoint) \
		(((dev)->devnum << 8) | (endpoint << 15) | ((dev)->slow << 26) | (dev)->maxpacketsize)
#define default_pipe(dev) ((dev)->slow <<26)

#define usb_sndctrlpipe(dev,endpoint)	((PIPE_CONTROL << 30) | create_pipe(dev,endpoint))
#define usb_rcvctrlpipe(dev,endpoint)	((PIPE_CONTROL << 30) | create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndisocpipe(dev,endpoint)	((PIPE_ISOCHRONOUS << 30) | create_pipe(dev,endpoint))
#define usb_rcvisocpipe(dev,endpoint)	((PIPE_ISOCHRONOUS << 30) | create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndbulkpipe(dev,endpoint)	((PIPE_BULK << 30) | create_pipe(dev,endpoint))
#define usb_rcvbulkpipe(dev,endpoint)	((PIPE_BULK << 30) | create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_sndintpipe(dev,endpoint)	((PIPE_INTERRUPT << 30) | create_pipe(dev,endpoint))
#define usb_rcvintpipe(dev,endpoint)	((PIPE_INTERRUPT << 30) | create_pipe(dev,endpoint) | USB_DIR_IN)
#define usb_snddefctrl(dev)		((PIPE_CONTROL << 30) | default_pipe(dev))
#define usb_rcvdefctrl(dev)		((PIPE_CONTROL << 30) | default_pipe(dev) | USB_DIR_IN)

/* The D0/D1 toggle bits */
#define usb_gettoggle(dev, ep, out) (((dev)->toggle[out] >> ep) & 1)
#define usb_dotoggle(dev, ep, out)  ((dev)->toggle[out] ^= (1 << ep))
#define usb_settoggle(dev, ep, out, bit) ((dev)->toggle[out] = ((dev)->toggle[out] & ~(1 << ep)) | ((bit) << ep))

/* Endpoint halt control/status */
#define usb_endpoint_out(ep_dir)	(((ep_dir >> 7) & 1) ^ 1)
#define usb_endpoint_halt(dev, ep, out) ((dev)->halted[out] |= (1 << (ep)))
#define usb_endpoint_running(dev, ep, out) ((dev)->halted[out] &= ~(1 << (ep)))
#define usb_endpoint_halted(dev, ep, out) ((dev)->halted[out] & (1 << (ep)))

#define usb_packetid(pipe)	(((pipe) & USB_DIR_IN) ? USB_PID_IN : USB_PID_OUT)

#define usb_pipeout(pipe)	((((pipe) >> 7) & 1) ^ 1)
#define usb_pipein(pipe)	(((pipe) >> 7) & 1)
#define usb_pipedevice(pipe)	(((pipe) >> 8) & 0x7f)
#define usb_pipe_endpdev(pipe)	(((pipe) >> 8) & 0x7ff)
#define usb_pipeendpoint(pipe)	(((pipe) >> 15) & 0xf)
#define usb_pipedata(pipe)	(((pipe) >> 19) & 1)
#define usb_pipeslow(pipe)	(((pipe) >> 26) & 1)
#define usb_pipetype(pipe)	(((pipe) >> 30) & 3)
#define usb_pipeisoc(pipe)	(usb_pipetype((pipe)) == PIPE_ISOCHRONOUS)
#define usb_pipeint(pipe)	(usb_pipetype((pipe)) == PIPE_INTERRUPT)
#define usb_pipecontrol(pipe)	(usb_pipetype((pipe)) == PIPE_CONTROL)
#define usb_pipebulk(pipe)	(usb_pipetype((pipe)) == PIPE_BULK)


/*************************************************************************
 * Hub Stuff
 */
struct usb_port_status {
	unsigned short wPortStatus;
	unsigned short wPortChange;
} __attribute__ ((packed));

struct usb_hub_status {
	unsigned short wHubStatus;
	unsigned short wHubChange;
} __attribute__ ((packed));


/* Hub descriptor */
struct usb_hub_descriptor {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  bNbrPorts;
	unsigned short wHubCharacteristics;
	unsigned char  bPwrOn2PwrGood;
	unsigned char  bHubContrCurrent;
	unsigned char  DeviceRemovable[(USB_MAXCHILDREN+1+7)/8];
	unsigned char  PortPowerCtrlMask[(USB_MAXCHILDREN+1+7)/8];
		/* DeviceRemovable and PortPwrCtrlMask want to be variable-length
	   bitmaps that hold max 255 entries. (bit0 is ignored) */
} __attribute__ ((packed));


struct usb_hub_device {
	struct usb_device *pusb_dev;
	struct usb_hub_descriptor desc;
};

struct usb_ops {
	int (*submit_bulk_msg)(struct usb_device *dev, unsigned long pipe, void *buffer, int transfer_len);
	int (*submit_control_msg)(struct usb_device *dev, unsigned long pipe, void *buffer,
					int transfer_len, struct devrequest *setup);
	int (*submit_int_msg)(struct usb_device *dev, unsigned long pipe, void *buffer,
					        int transfer_len, int interval);
};

struct usb_hc {
	struct device self;
	struct usb_ops *uop;
	void (*notify)(struct usb_device *dev, int port);
	TAILQ_ENTRY(usb_hc) hc_list;
	unsigned int port_mask; /*To mask some port when scanning*/
};

extern void delay(int ms);

static inline void udelay(unsigned long us)
{
	delay(us);	
}

void reset_controller(void *data);
#endif /*_USB_H_ */
