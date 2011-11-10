/** \file
 * Header file for the nework dependent functionality.
 * The function prototype listed here are linux dependent.
 *   
 * \internal
 * ---------------------------REVISION HISTORY-------------------
 * Synopsys 			01/Aug/2007		Created
 */
 
#ifndef SYNOP_GMAC_NETWORK_INTERFACE_H
#define SYNOP_GMAC_NETWORK_INTERFACE_H 1

#include "synopGMAC_plat.h"
#include "synopGMAC_Host.h"


#define NET_IF_TIMEOUT (10*HZ)
#define CHECK_TIME (HZ)

s32  synopGMAC_init_network_interface(char* xname,u64 synopGMACMappedAddr);
void  synopGMAC_exit_network_interface(void);

//s32 synopGMAC_linux_open(struct synopGMACNetworkAdapter *);
unsigned long synopGMAC_linux_open(struct synopGMACNetworkAdapter *);
//s32 synopGMAC_linux_close(struct net_device *);
s32 synopGMAC_linux_xmit_frames(struct ifnet *);
struct net_device_stats * synopGMAC_linux_get_stats(struct synopGMACNetworkAdapter *);
//void synopGMAC_linux_set_multicast_list(struct net_device *);
//s32 synopGMAC_linux_set_mac_address(struct synopGMACNetwokrAdapter*,void *);
//s32 synopGMAC_linux_change_mtu(struct net_device *,s32);
s32 synopGMAC_linux_do_ioctl(struct ifnet *,struct ifreq *,s32);
//void synopGMAC_linux_tx_timeout(struct net_device *);

s32 synopGMAC_test(synopGMACdevice * gmacdev_0,synopGMACdevice * gmacdev_1);

void dumpreg(u64 );
void dumpphyreg();

#endif /* End of file */
