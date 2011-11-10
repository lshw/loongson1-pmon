#define SET_LED(x) \
	lw	v0, GT_BASE_ADDR+(GPP_VALUE);\
	or v0,(7<<4); \
	and v0,~(x<<4); \
	sw	v0, GT_BASE_ADDR+(GPP_VALUE); \
		li v0, 0x40000;	\
99:			\
	bnez	v0, 99b;\
	addiu	v0, -1

#define HALT \
1:\
	b 1b;\
	nop



#define SUPERIO_RD(idx,reg) \
	li v0,PCI_IO_SPACE_BASE+0x3f0; \
	or v1,zero,idx; \
	sb	v1,(v0); \
	lbu	reg,1(v0);


#define SUPERIO_WR(idx,data) \
	li v0,PCI_IO_SPACE_BASE+0x3f0; \
	or	v1,zero,idx; \
	sb	v1,(v0); \
	or v1,zero,data; \
	sb	v1,1(v0);
	

//////////////////////////////////////////////////
#define PCICONF_READB(dev,func,reg,data) \
	or v0,zero,dev; \
	sll v0,3; \
	or v0,func; \
	sll v0,8; \
	or v0,reg; \
	and v0,~3; \
	or v0,HTOLE32(0x80000000|(0<<11)|(0<<8)|0); \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	or v0,zero,reg ;\
	and v0,3; \
	addu v0,s2; \
	lbu data, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(v0);


#define PCICONF_WRITEB(dev,func,reg,data) \
	or v0,zero,dev; \
	sll v0,3; \
	or v0,func; \
	sll v0,8; \
	or v0,reg; \
	or v0,3; \
	xor v0,3; \
	or v0,HTOLE32(0x80000000|(0<<11)|(0<<8)|0); \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	or v1,zero,data; \
	or v0,zero,reg ;\
	and v0,3; \
	addu v0,s2; \
	sb v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(v0); 

#define PCICONF_ORB(dev,func,reg,data) \
	or v0,zero,dev; \
	sll v0,3; \
	or v0,func; \
	sll v0,8; \
	or v0,reg; \
	or v0,3; \
	xor v0,3; \
	or v0,HTOLE32(0x80000000|(0<<11)|(0<<8)|0); \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	or v1,zero,reg ;\
	and v1,3; \
	addu v1,s2; \
	lbu v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(v1); \
	or v1,data; \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	or v0,zero,reg ;\
	and v0,3; \
	addu v0,s2; \
	sb v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(v0); 

#define PCICONF_ANDB(dev,func,reg,data) \
	or v0,zero,dev; \
	sll v0,3; \
	or v0,func; \
	sll v0,8; \
	or v0,reg; \
	or v0,3; \
	xor v0,3; \
	or v0,HTOLE32(0x80000000|(0<<11)|(0<<8)|0); \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	or v1,zero,reg ;\
	and v1,3; \
	addu v1,s2; \
	lbu v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(v1); \
	and v1,data; \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	or v0,zero,reg ;\
	and v0,3; \
	addu v0,s2; \
	sb v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(v0); 


#define PCICONF_WRITEW(dev,func,reg,data) \
	or v0,zero,dev; \
	sll v0,3; \
	or v0,func; \
	sll v0,8; \
	or v0,reg; \
	or v0,HTOLE32(0x80000000|(0<<11)|(0<<8)|0); \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	or v1,zero,data;\
	sw v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(s2);

#define PCICONF_WRITEW_NAND_OR(func,reg,nanddata,ordata) \
	or v0,zero,dev; \
	sll v0,3; \
	or v0,func; \
	sll v0,8; \
	or v0,reg; \
	or v0,HTOLE32(0x80000000|(0<<11)|(0<<8)|0); \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
    lw v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(s2);\
	not v1; \
	or v1,nanddata; \
	not v1; \
	or v1,ordata; \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	sw	v1, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(s2);
	

#define PCICONF_READW(dev,func,reg,data) \
	or v0,zero,dev; \
	sll v0,3; \
	or v0,func; \
	sll v0,8; \
	or v0,reg; \
	or v0,HTOLE32(0x80000000|(0<<11)|(0<<8)|0); \
	sw v0, PCI_CONFIGURATION_ADDRESS(s2); \
	lw data, PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER(s2);


#define E2_EPP 2
#define E2_S1 (1<<2)
#define E2_S2 (1<<3)
#define E2_FLOPPY (1<<4)

#define MV_REGRD(offs)			\
	addu v0,zero,offs; \
	addu v0,GT_BASE_ADDR; \
	lw	v0, (v0);

#define MV_REGWR(offs, reg)		\
	addu v0,zero,offs; \
	addu v0,GT_BASE_ADDR; \
	addu v1,zero,reg ; \
	sw	v1, (v0)

