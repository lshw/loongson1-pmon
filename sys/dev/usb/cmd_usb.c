/*
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 *
 * Most of this source has been derived from the Linux USB
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
 */

#include <pmon.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "part.h"
#include "usb.h"
#include "mod_usb_ohci.h"


static int usb_stor_curr_dev=-1; /* current device */

/* some display routines (info command) */
char * usb_get_class_desc(unsigned char dclass)
{
	switch(dclass) {
		case USB_CLASS_PER_INTERFACE:
			return("See Interface");
		case USB_CLASS_AUDIO:
			return("Audio");
		case USB_CLASS_COMM:
			return("Communication");
		case USB_CLASS_HID:
			return("Human Interface");
		case USB_CLASS_PRINTER:
			return("Printer");
		case USB_CLASS_MASS_STORAGE:
			return("Mass Storage");
		case USB_CLASS_HUB:
			return("Hub");
		case USB_CLASS_DATA:
			return("CDC Data");
		case USB_CLASS_VENDOR_SPEC:
			return("Vendor specific");
		default :
			return("");
	}
}

void usb_display_class_sub(unsigned char dclass,unsigned char subclass,unsigned char proto)
{
	switch(dclass) {
		case USB_CLASS_PER_INTERFACE:
			printf("See Interface");
			break;
		case USB_CLASS_HID:
			printf("Human Interface, Subclass: ");
			switch(subclass) {
				case USB_SUB_HID_NONE:
					printf("None");
					break;
				case USB_SUB_HID_BOOT:
					printf("Boot ");
					switch(proto) {
						case USB_PROT_HID_NONE:
							printf("None");
							break;
						case USB_PROT_HID_KEYBOARD:
							printf("Keyboard");
							break;
						case USB_PROT_HID_MOUSE:
							printf("Mouse");
							break;
						default:
							printf("reserved");
					}
					break;
				default:
					printf("reserved");
			}
			break;
		case USB_CLASS_MASS_STORAGE:
			printf("Mass Storage, ");
			switch(subclass) {
				case US_SC_RBC:
					printf("RBC ");
					break;
				case US_SC_8020:
					printf("SFF-8020i (ATAPI)");
					break;
				case US_SC_QIC:
					printf("QIC-157 (Tape)");
					break;
				case US_SC_UFI:
					printf("UFI");
					break;
				case US_SC_8070:
					printf("SFF-8070");
					break;
				case US_SC_SCSI:
					printf("Transp. SCSI");
					break;
				default:
					printf("reserved");
					break;
			}
			printf(", ");
			switch(proto) {
				case US_PR_CB:
					printf("Command/Bulk");
					break;
				case US_PR_CBI:
					printf("Command/Bulk/Int");
					break;
				case US_PR_BULK:
					printf("Bulk only");
					break;
				default:
					printf("reserved");
			}
			break;
		default:
			printf("%s",usb_get_class_desc(dclass));
	}
}

void usb_display_string(struct usb_device *dev,int index)
{
	char buffer[256];
	if (index!=0) {
		if (usb_string(dev,index,&buffer[0],256)>0);
			printf("String: \"%s\"",buffer);
	}
}

void usb_display_desc(struct usb_device *dev)
{
	if (dev->descriptor.bDescriptorType==USB_DT_DEVICE) {
		printf("%d(%d): %s,  USB Revision %x.%x\n",dev->devnum,dev->port,usb_get_class_desc(dev->config.if_desc[0].bInterfaceClass),
			(dev->descriptor.bcdUSB>>8) & 0xff,dev->descriptor.bcdUSB & 0xff);
		if (strlen(dev->mf) || strlen(dev->prod) || strlen(dev->serial))
			printf(" - %s %s %s\n",dev->mf,dev->prod,dev->serial);
		if (dev->descriptor.bDeviceClass) {
			printf(" - Class: ");
			usb_display_class_sub(dev->descriptor.bDeviceClass,dev->descriptor.bDeviceSubClass,dev->descriptor.bDeviceProtocol);
			printf("\n");
		}
		else {
			printf(" - Class: (from Interface) %s\n",usb_get_class_desc(dev->config.if_desc[0].bInterfaceClass));
		}
		printf(" - PacketSize: %d  Configurations: %d\n",dev->descriptor.bMaxPacketSize0,dev->descriptor.bNumConfigurations);
		printf(" - Vendor: 0x%04x  Product 0x%04x Version %d.%d\n",dev->descriptor.idVendor,dev->descriptor.idProduct,(dev->descriptor.bcdDevice>>8) & 0xff,dev->descriptor.bcdDevice & 0xff);
	}

}

void usb_display_conf_desc(struct usb_config_descriptor *config,struct usb_device *dev)
{
	printf("   Configuration: %d\n",config->bConfigurationValue);
	printf("   - Interfaces: %d %s%s%dmA\n",config->bNumInterfaces,(config->bmAttributes & 0x40) ? "Self Powered " : "Bus Powered ",
	(config->bmAttributes & 0x20) ? "Remote Wakeup " : "",config->MaxPower*2);
	if (config->iConfiguration) {
		printf("   - ");
		usb_display_string(dev,config->iConfiguration);
		printf("\n");
	}
}

void usb_display_if_desc(struct usb_interface_descriptor *ifdesc,struct usb_device *dev)
{
	printf("     Interface: %d\n",ifdesc->bInterfaceNumber);
	printf("     - Alternate Settings %d, Endpoints: %d\n",ifdesc->bAlternateSetting,ifdesc->bNumEndpoints);
	printf("     - Class ");
	usb_display_class_sub(ifdesc->bInterfaceClass,ifdesc->bInterfaceSubClass,ifdesc->bInterfaceProtocol);
	printf("\n");
	if (ifdesc->iInterface) {
		printf("     - ");
		usb_display_string(dev,ifdesc->iInterface);
		printf("\n");
	}
}

void usb_display_ep_desc(struct usb_endpoint_descriptor *epdesc)
{
	printf("     - Endpoint %d %s ",epdesc->bEndpointAddress & 0xf,(epdesc->bEndpointAddress & 0x80) ? "In" : "Out");
	switch((epdesc->bmAttributes & 0x03))
	{
		case 0: printf("Control"); break;
		case 1: printf("Isochronous"); break;
		case 2: printf("Bulk"); break;
		case 3: printf("Interrupt"); break;
	}
	printf(" MaxPacket %d",epdesc->wMaxPacketSize);
	if ((epdesc->bmAttributes & 0x03)==0x3)
		printf(" Interval %dms",epdesc->bInterval);
	printf("\n");
}

/* main routine to diasplay the configs, interfaces and endpoints */
void usb_display_config(struct usb_device *dev)
{
	struct usb_config_descriptor *config;
	struct usb_interface_descriptor *ifdesc;
	struct usb_endpoint_descriptor *epdesc;
	int i,ii;

	config= &dev->config;
	usb_display_conf_desc(config,dev);
	for(i=0;i<config->no_of_if;i++) {
		ifdesc= &config->if_desc[i];
		usb_display_if_desc(ifdesc,dev);
		for(ii=0;ii<ifdesc->no_of_ep;ii++) {
			epdesc= &ifdesc->ep_desc[ii];
			usb_display_ep_desc(epdesc);
		}
	}
	printf("\n");
}

/* shows the device tree recursively */
void usb_show_tree_graph(struct usb_device *dev,char *pre)
{
	int i,index;
	int has_child,last_child,port;

	index=strlen(pre);
	printf(" %s",pre);
	/* check if the device has connected children */
	has_child=0;
	for(i=0;i<dev->maxchild;i++) {
		if (dev->children[i]!=NULL)
			has_child=1;
	}
	/* check if we are the last one */
	last_child=1;
	if (dev->parent!=NULL) {
		for(i=0;i<dev->parent->maxchild;i++) {
			/* search for children */
			if (dev->parent->children[i]==dev) {
				/* found our pointer, see if we have a little sister */
				port=i;
				while(i++<dev->parent->maxchild) {
					if (dev->parent->children[i]!=NULL) {
						/* found a sister */
						last_child=0;
						break;
					} /* if */
				} /* while */
			} /* device found */
		} /* for all children of the parent */
		printf("\b+-");
		/* correct last child */
		if (last_child) {
			pre[index-1]=' ';
		}
	} /* if not root hub */
	else
		printf(" ");
	printf("%d ",dev->devnum);
	pre[index++]=' ';
	pre[index++]= has_child ? '|' : ' ';
	pre[index]=0;
	printf(" %s (%s, %dmA)\n",usb_get_class_desc(dev->config.if_desc[0].bInterfaceClass),
		dev->slow ? "1.5MBit/s" : "12MBit/s",dev->config.MaxPower * 2);
	if (strlen(dev->mf) ||
	   strlen(dev->prod) ||
	   strlen(dev->serial))
		printf(" %s  %s %s %s\n",pre,dev->mf,dev->prod,dev->serial);
	printf(" %s\n",pre);
	if (dev->maxchild>0) {
		for(i=0;i<dev->maxchild;i++) {
			if (dev->children[i]!=NULL) {
				usb_show_tree_graph(dev->children[i],pre);
				pre[index]=0;
			}
		}
	}
}

/* main routine for the tree command */
void usb_show_tree(struct usb_device *dev)
{
	char preamble[32];

	if(dev == NULL)
		return;	
	memset(preamble,0,32);
	usb_show_tree_graph(dev,&preamble[0]);
}



/*********************************************************************************
 * usb command intepreter
 */
static char *USB_usage[] = {
	"usb     - USB sub-system\n",
#ifdef __not_used__
	"usb stor  - scanning usb storage device\n",
#endif
	"usb tree  - show USB device tree\n",
	"usb info [dev] - show available USB devices\n",
#ifdef __not_used__
	"usb storage  - show details of USB storage devices\n",
#endif
	"usb dev [dev] - show or set current USB storage device\n",
	"usb read addr blk# cnt - read `cnt' blocks starting at block `blk#'\n"
	"    to memory address `addr'\n"
};

static void usage(void)
{
	int i, size = sizeof(USB_usage)/sizeof(USB_usage[0]);	

	for(i=0; i<size; i++)
		printf(" %s", USB_usage[i]);
}

extern struct hostcontroller host_controller;

#if NMOD_USB_OHCI > 0
void do_scan_usb_bus(struct hostcontroller *controllers)
{
	struct usb_hc *hc, *next_hc;	

	for(hc = TAILQ_FIRST(controllers); hc != NULL; hc = next_hc){
		next_hc = TAILQ_NEXT(hc, hc_list);
		reset_controller(hc);
		usb_scan_devices(hc);
	}
}
#endif

static int do_usb (int argc, char *argv[])
{

	int i;
	struct usb_device *dev = NULL;
	block_dev_desc_t *stor_dev;
	
	if(argc < 2){
		printf("Generic usb commands, type usb help for all usage\n");
		return 0;
	}

#ifdef __not_used__
	if (strncmp(argv[1], "stor", 5) == 0 ){
		printf("scanning USB storage device...\n");
		/* try to recognize storage devices immediately */
	 	usb_stor_curr_dev = usb_stor_scan(1);
		return 0;
	}
#endif
#if 0
	if (strncmp(argv[1],"stop",4) == 0) {
		if (argc==2) {
			if (usb_kbd_deregister()!=0) {
				printf("USB not stopped: usbkbd still using USB\n");
				return 1;
			}
		}
		else { /* forced stop, switch console in to serial */
			console_assign(stdin,"serial");
			usb_kbd_deregister();
		}
		printf("stopping USB..\n");
		usb_stop();
		return 0;
	}
#endif
	if (strncmp(argv[1],"tree",4) == 0) {
		printf("Device Tree:\n");
		usb_show_tree(usb_get_dev_index(0));
		return 0;
	}
	if (strncmp(argv[1],"inf",3) == 0) {
		int d;
		if (argc==2) {
			for(d=0;d<USB_MAX_DEVICE;d++) {
				dev=usb_get_dev_index(d);
				if (dev==NULL)
					break;
				usb_display_desc(dev);
				usb_display_config(dev);
			}
			return 0;
		}
		else {
			int d;

			i=strtoul(argv[2], NULL, 16);
			printf("config for device %d\n",i);
			for(d=0;d<USB_MAX_DEVICE;d++) {
				dev=usb_get_dev_index(d);
				if (dev==NULL)
					break;
				if (dev->devnum==i)
					break;
			}
			if (dev==NULL) {
				printf("*** NO Device avaiable ***\n");
				return 0;
			}
			else {
				usb_display_desc(dev);
				usb_display_config(dev);
			}
		}
		return 0;
	}
#if NMOD_USB_OHCI > 0 && defined(__not_used__)
	if (strncmp(argv[1], "scan", 4) == 0) {
		do_scan_usb_bus(&host_controller);
		return 0;
	}
#endif

#ifdef __not_used__
	if (strncmp(argv[1], "stor", 4) == 0) {
		usb_stor_info();
		return 0;
	}
#endif

	if (strcmp(argv[1],"read") == 0) {
		if (usb_stor_curr_dev<0) {
			printf("no current device selected\n");
			return 1;
		}
		if (argc==5) {
			unsigned long addr = strtoul(argv[2], NULL, 16);
			unsigned long blk  = strtoul(argv[3], NULL, 16);
			unsigned long cnt  = strtoul(argv[4], NULL, 16);
			unsigned long n;
			printf ("\nUSB read: device %d block # %ld, count %ld ... ",
					usb_stor_curr_dev, blk, cnt);
			stor_dev=usb_stor_get_dev(usb_stor_curr_dev);
			n = stor_dev->block_read(usb_stor_curr_dev, blk, cnt, (ulong *)addr);
			printf ("%ld blocks read: %s\n",n,(n==cnt) ? "OK" : "ERROR");
			if (n==cnt)
				return 0;
			return 1;
		}
	}
	if (strncmp(argv[1], "dev", 3) == 0) {
		if (argc == 3) {
			int dev = (int)strtoul(argv[2], NULL, 10);
			printf ("\nUSB device %d: ", dev);
			if (dev >= USB_MAX_STOR_DEV) {
				printf("unknown device\n");
				return 1;
			}
			printf ("\n    Device %d: ", dev);
			stor_dev=usb_stor_get_dev(dev);
			dev_print(stor_dev);
			if (stor_dev->type == DEV_TYPE_UNKNOWN) {
				return 1;
			}
			usb_stor_curr_dev = dev;
			printf("... is now current device\n");
			return 0;
		}
		else {
			printf ("USB device %d: \n", usb_stor_curr_dev);
			stor_dev=usb_stor_get_dev(usb_stor_curr_dev);
			dev_print(stor_dev);
			if (stor_dev->type == DEV_TYPE_UNKNOWN) {
				return 1;
			}
			return 0;
		}
		return 0;
	}

	if(strncmp("help", argv[1], 4) == 0){
		usage();
		return 0;
	}
	
	printf("Generic usb commands, type usb help for all usage\n");
	return 0;
}

static const Cmd Cmds[] = {
	{ "USB commands"},
	{ "usb", " ", NULL, "general usb commands", do_usb, 1, 5, 0},
	{ 0, 0}
};		

static void init_cmd(void) __attribute__((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
