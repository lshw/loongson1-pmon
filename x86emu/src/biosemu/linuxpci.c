/* provide linux pci interface for pmon */

#include <stdio.h>
#include <dev/pci/pcivar.h>
#include <linux/types.h>

extern pcireg_t _pci_conf_readn(pcitag_t tag, int reg, int width);
extern void _pci_conf_writen(pcitag_t tag, int reg, pcireg_t data,int width);


int pci_read_config_dword(struct pci_device *linuxpd, int reg, u32 *val)
{
	if ((reg & 3) || reg < 0 || reg >= 0x100) {
        	printf ("pci_read_config_dword: bad reg %x\n", reg);
        	return -1;
    	}
	*val=_pci_conf_read(linuxpd->pa.pa_tag, reg);       
	return 0;
}

int pci_write_config_dword(struct pci_device *linuxpd, int reg, u32 val)
{
    if ((reg & 3) || reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_dword: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_write(linuxpd->pa.pa_tag,reg,val); 
   return 0;
}

int pci_read_config_word(struct pci_device *linuxpd, int reg, u16 *val)
{
	if ((reg & 1) || reg < 0 || reg >= 0x100) {
        	printf ("pci_read_config_word: bad reg %x\n", reg);
        	return -1;
    	}
	*val=_pci_conf_readn(linuxpd->pa.pa_tag,reg,2);       
	return 0;
}

int pci_write_config_word(struct pci_device *linuxpd, int reg, u16 val)
{
    if ((reg & 1) || reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_writen(linuxpd->pa.pa_tag,reg,val,2); 
   return 0;
}

int pci_read_config_byte(struct pci_device *linuxpd, int reg, u8 *val)
{
    if (reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
	*val=_pci_conf_readn(linuxpd->pa.pa_tag,reg,1);       
	return 0;
}

int pci_write_config_byte(struct pci_device *linuxpd, int reg, u8 val)
{
    if (reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_writen(linuxpd->pa.pa_tag,reg,val,1); 
   return 0;
}



