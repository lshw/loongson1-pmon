#ifndef __LINUXIO_INCLUDE_H_
#define __LINUXIO_INCLUDE_H_
int pci_read_config_dword(struct pci_device *linuxpd, int reg, u32 *val);
int pci_write_config_dword(struct pci_device *linuxpd, int reg, u32 val);
int pci_read_config_word(struct pci_device *linuxpd, int reg, u16 *val);
int pci_write_config_word(struct pci_device *linuxpd, int reg, u16 val);
int pci_read_config_byte(struct pci_device *linuxpd, int reg, u8 *val);
int pci_write_config_byte(struct pci_device *linuxpd, int reg, u8 val);
	
#endif
