/** \file
 * This is the network dependent layer to handle network related functionality.
 * This file is tightly coupled to neworking frame work of linux 2.6.xx kernel.
 * The functionality carried out in this file should be treated as an example only
 * if the underlying operating system is not Linux. 
 * 
 * \note Many of the functions other than the device specific functions
 *  changes for operating system other than Linux 2.6.xx
 * \internal 
 *-----------------------------REVISION HISTORY-----------------------------------
 * Synopsys			01/Aug/2007				Created
 */

#include "synopGMAC_Host.h"
#include "synopGMAC_plat.h"
#include "synopGMAC_network_interface.h"
#include "synopGMAC_Dev.h"

//static struct timer_list synopGMAC_cable_unplug_timer;
static u32 GMAC_Power_down; // This global variable is used to indicate the ISR whether the interrupts occured in the process of powering down the mac or not


static int rtl88e1111_config_init(synopGMACdevice *gmacdev)
{
	int retval, err;
	u16 data;

	synopGMAC_read_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,&data);
	data = data | 0x82;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,data);
	synopGMAC_read_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,&data);
	data = data | 0x8000;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,data);
#if SYNOP_PHY_LOOPBACK
	synopGMAC_read_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,&data);
	data = data | 0x70;
	data = data & 0xffdf;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,data);
	data = 0x8000;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,data);
	data = 0x5140;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,data);
#endif
	if (err < 0)
		return err;
	return 0;
}
/*
#include "mii.h"
#include "mii.c"*/

s32 synopGMAC_check_phy_init (synopGMACPciNetworkAdapter *adapter);


static void synopGMAC_linux_cable_unplug_function(synopGMACPciNetworkAdapter *adapter)
{
	s32 data;
	synopGMACdevice *gmacdev = adapter->synopGMACdev;

/*	if (!mii_link_ok(&adapter->mii)) {
		if(gmacdev->LinkState)
			TR("No Link\n");
		gmacdev->DuplexMode = 0;
		gmacdev->Speed = 0;
		gmacdev->LoopBackMode = 0;
		gmacdev->LinkState = 0;
	} else {
		data = synopGMAC_check_phy_init(adapter);

		if(gmacdev->LinkState != data) {
			gmacdev->LinkState = data;
			synopGMAC_mac_init(gmacdev);
			TR("Link UP data=%08x\n",data);
			TR("Link is up in %s mode\n",(gmacdev->DuplexMode == FULLDUPLEX) ? "FULL DUPLEX": "HALF DUPLEX");
			if(gmacdev->Speed == SPEED1000)	
				TR("Link is with 1000M Speed \n");
			if(gmacdev->Speed == SPEED100)	
				TR("Link is with 100M Speed \n");
			if(gmacdev->Speed == SPEED10)	
				TR("Link is with 10M Speed \n");
		}
	}*/
	gmacdev->DuplexMode = FULLDUPLEX;
	gmacdev->Speed = SPEED100;
	gmacdev->LoopBackMode = 0;
	gmacdev->LinkState = LINKUP;
}

s32 synopGMAC_check_phy_init (synopGMACPciNetworkAdapter *adapter)
{	
//	struct ethtool_cmd cmd;
	synopGMACdevice *gmacdev = adapter->synopGMACdev;

/*	if(!mii_link_ok(&adapter->mii)) {
		gmacdev->DuplexMode = FULLDUPLEX;
		gmacdev->Speed      = SPEED100;
		return 0;
	} else {
		mii_ethtool_gset(&adapter->mii, &cmd);

		gmacdev->DuplexMode = (cmd.duplex == DUPLEX_FULL)  ? FULLDUPLEX: HALFDUPLEX;
		if(cmd.speed == SPEED_1000)
			gmacdev->Speed = SPEED1000;
		else if(cmd.speed == SPEED_100)
			gmacdev->Speed = SPEED100;
		else
			gmacdev->Speed = SPEED10;
	}*/
	gmacdev->DuplexMode = FULLDUPLEX;
	gmacdev->Speed      = SPEED100;

	return gmacdev->Speed|(gmacdev->DuplexMode<<4);
}

static void synopGMAC_linux_powerup_mac(synopGMACdevice *gmacdev)
{
	GMAC_Power_down = 0;	// Let ISR know that MAC is out of power down now
	if( synopGMAC_is_magic_packet_received(gmacdev))
		TR("GMAC wokeup due to Magic Pkt Received\n");
	if(synopGMAC_is_wakeup_frame_received(gmacdev))
		TR("GMAC wokeup due to Wakeup Frame Received\n");
	//Disable the assertion of PMT interrupt
	synopGMAC_pmt_int_disable(gmacdev);
	//Enable the mac and Dma rx and tx paths
	synopGMAC_rx_enable(gmacdev);
	synopGMAC_enable_dma_rx(gmacdev);

	synopGMAC_tx_enable(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);
}

static s32 synopGMAC_setup_tx_desc_queue(synopGMACdevice *gmacdev, void *dev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->TxDescCount = 0;




	TR("Total size of memory required for Tx Descriptors in Ring Mode = 0x%08x\n",
		((sizeof(DmaDesc) * no_of_desc)));
	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory(gmacdev, sizeof(DmaDesc) * no_of_desc, &dma_addr);
	if(first_desc == NULL){
		TR("Error in Tx Descriptors memory allocation\n");
		return -ESYNOPGMACNOMEM;
	}

	gmacdev->TxDescCount = no_of_desc;
	gmacdev->TxDesc      = first_desc;
	gmacdev->TxDescDma   = dma_addr;
	TR("Tx Descriptors in Ring Mode: No. of descriptors = %d base = 0x%08x dma = 0x%08x\n",
		no_of_desc, (u32)first_desc, dma_addr);

	for(i =0; i < gmacdev -> TxDescCount; i++){
		synopGMAC_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount-1);
		TR("%02d %08x \n",i, (unsigned int)(gmacdev->TxDesc + i));
	}

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->TxNextDesc = gmacdev->TxDesc;
	gmacdev->TxBusyDesc = gmacdev->TxDesc;
	gmacdev->BusyTxDesc  = 0; 

	return -ESYNOPGMACNOERR;
}

static s32 synopGMAC_setup_rx_desc_queue(synopGMACdevice *gmacdev, void *dev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->RxDescCount = 0;




	TR("total size of memory required for Rx Descriptors in Ring Mode = 0x%08x\n",
		((sizeof(DmaDesc) * no_of_desc)));
	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory(gmacdev, sizeof(DmaDesc) * no_of_desc, &dma_addr);
	if(first_desc == NULL){
		TR("Error in Rx Descriptor Memory allocation in Ring mode\n");
		return -ESYNOPGMACNOMEM;
	}

	gmacdev->RxDescCount = no_of_desc;
	gmacdev->RxDesc      = first_desc;
	gmacdev->RxDescDma   = dma_addr;
	TR("Rx Descriptors in Ring Mode: No. of descriptors = %d base = 0x%08x dma = 0x%08x\n",
		no_of_desc, (u32)first_desc, dma_addr);

	for(i =0; i < gmacdev -> RxDescCount; i++){
		synopGMAC_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount-1);
		TR("%02d %08x \n",i, (unsigned int)(gmacdev->RxDesc + i));
	}

	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;
	gmacdev->RxNextDesc = gmacdev->RxDesc;
	gmacdev->RxBusyDesc = gmacdev->RxDesc;
	gmacdev->BusyRxDesc   = 0; 

	return -ESYNOPGMACNOERR;
}

void synop_handle_transmit_over(struct synopGMACNetworkAdapter * tp)
{
	synopGMACdevice * gmacdev;
	s32 desc_index;
	u32 data1, data2;
	u32 status;
	u32 length1, length2;
	u64 dma_addr1, dma_addr2;
#ifdef ENH_DESC_8W
	u32 ext_status;
	u16 time_stamp_higher;
	u32 time_stamp_high;
	u32 time_stamp_low;
#endif
	
	gmacdev = tp->synopGMACdev;

	/*Handle the transmit Descriptors*/
	do {
#ifdef ENH_DESC_8W
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2,&ext_status,&time_stamp_high,&time_stamp_low);
        synopGMAC_TS_read_timestamp_higher_val(gmacdev, &time_stamp_higher);
#else
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
#endif
//		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr, &length, &data1);
		if(desc_index >= 0 && data1 != 0){
		#ifdef	IPC_OFFLOAD
			if(synopGMAC_is_tx_ipv4header_checksum_error(gmacdev, status)){
				TR("Harware Failed to Insert IPV4 Header Checksum\n");
			}
			if(synopGMAC_is_tx_payload_checksum_error(gmacdev, status)){
				TR("Harware Failed to Insert Payload Checksum\n");
			}
		#endif
		
			plat_free_memory((void *)(data1));	//sw:	data1 = buffer1
			
			if(synopGMAC_is_desc_valid(status)){
				tp->synopGMACNetStats.tx_bytes += length1;
				tp->synopGMACNetStats.tx_packets++;
			}
			else {
				TR("Error in Status %08x\n",status);
				tp->synopGMACNetStats.tx_errors++;
				tp->synopGMACNetStats.tx_aborted_errors += synopGMAC_is_tx_aborted(status);
				tp->synopGMACNetStats.tx_carrier_errors += synopGMAC_is_tx_carrier_error(status);
			}
		}
		tp->synopGMACNetStats.collisions += synopGMAC_get_tx_collision_count(status);
	} while(desc_index >= 0);
}

void synop_handle_received_data(struct synopGMACNetworkAdapter* tp)
{
	synopGMACdevice * gmacdev;
	struct PmonInet * pinetdev;
	s32 desc_index;
	struct ifnet* ifp;
	struct ether_header * eh;
	u32 data1;
	u32 data2;
	u32 len;
	u32 status;
	u64 dma_addr1;
	u64 dma_addr2;
	struct mbuf *skb; //This is the pointer to hold the received data

	gmacdev = tp->synopGMACdev;

	pinetdev = tp->PInetdev;

	ifp = &(pinetdev->arpcom.ac_if);

	/*Handle the Receive Descriptors*/
	do{
		desc_index = synopGMAC_get_rx_qptr(gmacdev, &status, &dma_addr1, NULL, &data1, &dma_addr2, NULL, &data2);

		if(desc_index >= 0 && data1 != 0){
			if(synopGMAC_is_rx_desc_valid(status)||SYNOP_PHY_LOOPBACK){
				skb = getmbuf(tp);

				dma_addr1 =  plat_dma_map_single(gmacdev,data1,RX_BUF_SIZE,SYNC_R);
				len =  synopGMAC_get_rx_desc_frame_length(status) - 4; //Not interested in Ethernet CRC bytes
				bcopy((char *)data1, mtod(skb, char *), len);

				skb->m_pkthdr.rcvif = ifp;
				skb->m_pkthdr.len = skb->m_len = len - sizeof(struct ether_header);

				eh = mtod(skb, struct ether_header *);
				skb->m_data += sizeof(struct ether_header);

				ether_input(ifp, eh, skb);
				tp->synopGMACNetStats.rx_packets++;
				tp->synopGMACNetStats.rx_bytes += len;
			}
			else{
				printf("s: %08x\n",status);
				tp->synopGMACNetStats.rx_errors++;
				tp->synopGMACNetStats.collisions       += synopGMAC_is_rx_frame_collision(status);
				tp->synopGMACNetStats.rx_crc_errors    += synopGMAC_is_rx_crc(status);
				tp->synopGMACNetStats.rx_frame_errors  += synopGMAC_is_frame_dribbling_errors(status);
				tp->synopGMACNetStats.rx_length_errors += synopGMAC_is_rx_frame_length_errors(status);
			}

			desc_index = synopGMAC_set_rx_qptr(gmacdev,dma_addr1, RX_BUF_SIZE, (u32)data1,0,0,0);

			if(desc_index < 0){
				plat_free_memory((void *)data1);
			}
		}
	}while(desc_index >= 0);
}

int synopGMAC_intr_handler(struct synopGMACNetworkAdapter * tp)
{
	/*Kernels passes the netdev structure in the dev_id. So grab it*/
	synopGMACdevice * gmacdev;
	u32 dma_status_reg;

	s32 status;
	u64 dma_addr;
	struct ifnet * ifp;

//	adapter  = tp;
	gmacdev = tp->synopGMACdev;
	ifp = &(tp->PInetdev->arpcom.ac_if);

	if(gmacdev->LinkState == LINKUP)
		ifp->if_flags = ifp->if_flags | IFF_RUNNING;


	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synopGMACReadReg(gmacdev->DmaBase, DmaStatus);
//	synopGMAC_disable_interrupt_all(gmacdev);

	/* TX/RX NORMAL interrupts正常中断 */
//	if (dma_status_reg & DmaIntNormal) {
		/* 接收中断或发送中断 */
		if ((dma_status_reg & DmaIntRxCompleted) ||
			 (dma_status_reg & (DmaIntTxCompleted))) {
//				if(dma_status_reg & DmaIntRxCompleted){
					synop_handle_received_data(tp);
//				}
//				if(dma_status_reg & DmaIntTxCompleted){
					synop_handle_transmit_over(tp);	//Do whatever you want after the transmission is over
//				}
		}
//	}
	
	/* ABNORMAL interrupts异常中断 */
	if (dma_status_reg & DmaIntAbnormal) {
		/*总线错误(异常)*/
		if(dma_status_reg & DmaIntBusError){
			u8 mac_addr[6] = DEFAULT_MAC_ADDRESS;//after soft reset, configure the MAC address to default value
			TR("%s::Fatal Bus Error Inetrrupt Seen\n",__FUNCTION__);
			printf("====DMA error!!!\n");

			synopGMAC_disable_dma_tx(gmacdev);
			synopGMAC_disable_dma_rx(gmacdev);

			synopGMAC_take_desc_ownership_tx(gmacdev);
			synopGMAC_take_desc_ownership_rx(gmacdev);

			synopGMAC_init_tx_rx_desc_queue(gmacdev);

			synopGMAC_reset(gmacdev);//reset the DMA engine and the GMAC ip

			synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr); 
			synopGMAC_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 );
			synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward);	
			synopGMAC_init_rx_desc_base(gmacdev);
			synopGMAC_init_tx_desc_base(gmacdev);
			synopGMAC_mac_init(gmacdev);
			synopGMAC_enable_dma_rx(gmacdev);
			synopGMAC_enable_dma_tx(gmacdev);
		}

		/*接收缓存不可用(异常)*/
		if(dma_status_reg & DmaIntRxNoBuffer){
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				tp->synopGMACNetStats.rx_over_errors++;
				/*Now Descriptors have been created in synop_handle_received_data(). Just issue a poll demand to resume DMA operation*/
				synopGMACWriteReg(gmacdev->DmaBase, DmaStatus ,0x80); 	//sw: clear the rxb ua bit
				synopGMAC_resume_dma_rx(gmacdev);//To handle GBPS with 12 descriptors
			}
		}

		/*接收过程停止(异常)*/
		if(dma_status_reg & DmaIntRxStopped){
			TR("%s::Receiver stopped seeing Rx interrupts\n",__FUNCTION__); //Receiver gone in to stopped state
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				tp->synopGMACNetStats.rx_over_errors++;
				do{
					u32 skb = (u32)plat_alloc_memory(RX_BUF_SIZE);		//should skb aligned here?
					if(skb == NULL){
						TR0("ERROR in skb buffer allocation\n");
						break;
						//return -ESYNOPGMACNOMEM;
					}
					//dma_addr = (u32)skb;
					dma_addr = plat_dma_map_single(gmacdev,skb,RX_BUF_SIZE,SYNC_R);
					status = synopGMAC_set_rx_qptr(gmacdev,dma_addr,RX_BUF_SIZE, (u32)skb,0,0,0);
					TR("%s::Set Rx Descriptor no %08x for skb %08x \n",__FUNCTION__,status,(u32)skb);
					if(status < 0){
						printf("==%s:no free\n",__FUNCTION__);
						plat_free_memory((void *)skb);
					}
				}while(status >= 0);

				synopGMAC_enable_dma_rx(gmacdev);
			}
		}

		/*传输缓存下溢(异常)*/
		if(dma_status_reg & DmaIntTxUnderflow){
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				synop_handle_transmit_over(tp);
			}
		}

		/*传输过程停止(异常)*/
		if(dma_status_reg & DmaIntTxStopped){
			TR("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				synopGMAC_disable_dma_tx(gmacdev);
				synopGMAC_take_desc_ownership_tx(gmacdev);
				synopGMAC_enable_dma_tx(gmacdev);
				//netif_wake_queue(netdev);
				TR("%s::Transmission Resumed\n",__FUNCTION__);
			}
		}
	}
	
	/* Optional hardware blocks, interrupts should be disabled */
	if (dma_status_reg &
		     (GmacPmtIntr | GmacMmcIntr | GmacLineIntfIntr)) {
		TR("%s: unexpected status %08x\n", __func__, intr_status);
		if(dma_status_reg & GmacPmtIntr){
			synopGMAC_linux_powerup_mac(gmacdev);
		}
	
		if(dma_status_reg & GmacMmcIntr){
			TR("%s:: Interrupt due to MMC module\n",__FUNCTION__);
			TR("%s:: synopGMAC_rx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_rx_int_status(gmacdev));
			TR("%s:: synopGMAC_tx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_tx_int_status(gmacdev));
		}

		/* thf PMON中执行synopGMAC_linux_cable_unplug_function()函数来判断网络是否有连接,
		   并获取连接信息,由于PMON使用轮询的方式来执行,所以占用很多资源，导致网络延时比较长,
		   目前暂时屏蔽该函数的执行来提高网络速度。屏蔽该函数没有发现会影响使用.
		*/
		if(dma_status_reg & GmacLineIntfIntr){
			/* 配置成千兆模式时GmacLineIntfIntr会被置1, 
			导致synopGMAC_linux_cable_unplug_function不断被执行,影响系统性能 
			GmacLineIntfIntr会被置1 还不清楚原因，这里暂时屏蔽该函数 */
			#ifdef CONFIG_PHY100M
			synopGMAC_linux_cable_unplug_function(tp);
			#endif
		}
	}

	/* Enable the interrrupt before returning from ISR*/
//	synopGMAC_clear_interrupt(gmacdev);
//	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	return 0;
}

static s32 synopGMAC_linux_open(struct synopGMACNetworkAdapter *tp)
{
	s32 status = 0;
	s32 retval = 0;

	u64 dma_addr;
	u32 skb;	//sw	we just use the name skb in pomn

	struct synopGMACNetworkAdapter *adapter = tp;
	synopGMACdevice *gmacdev;
	struct PmonInet *PInetdev;
	TR0("%s called \n",__FUNCTION__);
	adapter = tp;
	gmacdev = (synopGMACdevice *)adapter->synopGMACdev;
	PInetdev = (struct PmonInet *)adapter->PInetdev;

	/*Now platform dependent initialization.*/
//	synopGMAC_disable_interrupt_all(gmacdev);

	/*Lets reset the IP*/
	synopGMAC_reset(gmacdev);

	/* we do not process interrupts */
	synopGMAC_disable_interrupt_all(gmacdev);

	/*Attach the device to MAC struct This will configure all the required base addresses
	  such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
	synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, PInetdev->dev_addr);

	/*Lets read the version of ip in to device structure*/
	synopGMAC_read_version(gmacdev);

	synopGMAC_get_mac_addr(adapter->synopGMACdev, GmacAddr0High, GmacAddr0Low, PInetdev->dev_addr);
	
	/*Check for Phy initialization*/
	synopGMAC_set_mdc_clk_div(gmacdev, GmiiCsrClk2);	//thf
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

	/*Set up the tx and rx descriptor queue/ring*/
	synopGMAC_setup_tx_desc_queue(gmacdev, NULL, TRANSMIT_DESC_SIZE, RINGMODE);
	synopGMAC_init_tx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr
	
	synopGMAC_setup_rx_desc_queue(gmacdev, NULL, RECEIVE_DESC_SIZE, RINGMODE);
	synopGMAC_init_rx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

#ifdef ENH_DESC_8W
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2 | DmaDescriptor8Words); //pbl32 incr with rxthreshold 128 and Desc is 8 Words
#else
#if defined(LS1CSOC)
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength4 | DmaDescriptorSkip2);
#else
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength4 | DmaDescriptorSkip1);	//pbl4 incr with rxthreshold 128
#endif
#endif
	synopGMAC_dma_control_init(gmacdev, DmaStoreAndForward|DmaTxSecondFrame|DmaRxThreshCtrl128);

	/*Initialize the mac interface*/
	synopGMAC_check_phy_init(adapter);
	synopGMAC_mac_init(gmacdev);
	synopGMAC_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

	do {
		skb = (u32)plat_alloc_memory(RX_BUF_SIZE);		//should skb aligned here?
		if (skb == NULL) {
			TR0("ERROR in skb buffer allocation\n");
			break;
//			return -ESYNOPGMACNOMEM;
		}
		
		dma_addr = plat_dma_map_single(gmacdev,skb,RX_BUF_SIZE,SYNC_R);

		status = synopGMAC_set_rx_qptr(gmacdev, dma_addr, RX_BUF_SIZE, (u32)skb, 0, 0, 0);
		if (status < 0) {
			plat_free_memory((void *)skb);
		}
	} while(status >= 0 && status < RECEIVE_DESC_SIZE-1);

	synopGMAC_clear_interrupt(gmacdev);
	/*
	Disable the interrupts generated by MMC and IPC counters.
	If these are not disabled ISR should be modified accordingly to handle these interrupts.
	*/	
	synopGMAC_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);

	/* no interrupts in pmon */
//	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);

	synopGMAC_enable_dma_rx(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);

#if defined(LS1ASOC) || defined(LS1BSOC) || defined(LS1CSOC)
	synopGMAC_mac_init(gmacdev);
#endif

	PInetdev->sc_ih = pci_intr_establish(0, 0, IPL_NET, synopGMAC_intr_handler, adapter, 0);
	TR("register poll interrupt: gmac 0\n");

	return retval;
}

s32 synopGMAC_linux_xmit_frames(struct ifnet* ifp)
{
	s32 status = 0;
	u64 dma_addr;
	u32 offload_needed = 0;
	u32 skb;
	int len;
	struct mbuf *mb_head;	//sw	we just use the name skb

	struct synopGMACNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	
	adapter = (struct synopGMACNetworkAdapter *) ifp->if_softc;
	if(adapter == NULL)
		return -1;

	gmacdev = (synopGMACdevice *) adapter->synopGMACdev;
	if(gmacdev == NULL)
		return -1;

	while(ifp->if_snd.ifq_head != NULL){
		if(!synopGMAC_is_desc_owned_by_dma(gmacdev->TxNextDesc)) {

			skb = (u32)plat_alloc_memory(TX_BUF_SIZE);
			if(skb == 0)
				return -1;

			IF_DEQUEUE(&ifp->if_snd, mb_head);

			/*Now we have skb ready and OS invoked this function. Lets make our DMA know about this*/
			len = mb_head->m_pkthdr.len;

			//sw: i don't know weather it's right
			m_copydata(mb_head, 0, len, (char *)skb);
			dma_addr = plat_dma_map_single(gmacdev,skb,len,SYNC_W);

			m_freem(mb_head);

			status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, len, skb, 0, 0, 0, offload_needed);

			if(status < 0){
				TR("%s No More Free Tx Descriptors\n",__FUNCTION__);
				return -EBUSY;
			}
		}
#if SYNOP_TX_DEBUG
		else
			printf("===%x: next txDesc belongs to DMA don't set it\n",gmacdev->TxNextDesc);
#endif
	}
	
	/*Now force the DMA to start transmission*/
	synopGMAC_resume_dma_tx(gmacdev);
	return -ESYNOPGMACNOERR;
}

static int init_phy(synopGMACdevice *gmacdev)
{
	u16 data, data1;

	synopGMAC_read_phy_reg(gmacdev->MacBase, gmacdev->PhyBase, 2, &data);
	synopGMAC_read_phy_reg(gmacdev->MacBase, gmacdev->PhyBase, 3, &data1);
//	printf("PHY ID %x %x\n", data, data1);
#ifdef RMII
	/* RTL8201EL */
	if ((data == 0x001c) && (data1 == 0xc815)) {
		/*  设置寄存器25，使能RMII模式 */
		synopGMAC_write_phy_reg(gmacdev->MacBase, gmacdev->PhyBase, 25, 0x400);
	}
#endif
	/*set 88e1111 clock phase delay*/
	if (data == 0x141)
		rtl88e1111_config_init(gmacdev);
	return 0;
}

s32 synopGMAC_dummy_reset(struct ifnet *ifp)
{
	struct synopGMACNetworkAdapter * adapter; 
	synopGMACdevice	* gmacdev;

	adapter = (struct synopGMACNetworkAdapter *)ifp->if_softc;
	gmacdev = adapter->synopGMACdev;

	return synopGMAC_reset(gmacdev);
}

s32 synopGMAC_dummy_ioctl(struct ifnet *ifp)
{
//	printf("==dummy ioctl\n");
	return 0;
}

/* just copy this function from rtl8169.c */
static int gmac_ether_ioctl(struct ifnet *ifp, unsigned long cmd, caddr_t data)
{
	struct ifaddr *ifa;
	struct synopGMACNetworkAdapter *adapter;
	int error = 0;
	int s;

	adapter = ifp->if_softc;
	ifa = (struct ifaddr *) data;

	s = splimp();
	switch (cmd) {
#ifdef PMON
	case SIOCPOLL:
	{
		break;
	}
#endif
	case SIOCSIFADDR:
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if (!(ifp->if_flags & IFF_UP)) {	
				error = synopGMAC_linux_open(adapter);
			}

			if(error == -1){
				return(error);
			}	
			ifp->if_flags |= IFF_UP;
#ifdef __OpenBSD__
			arp_ifinit(&(adapter->PInetdev->arpcom), ifa);
			TR("==arp_ifinit done\n");
#else
			arp_ifinit(ifp, ifa);
#endif
			break;
#endif
		default:
			synopGMAC_linux_open(adapter);
			ifp->if_flags |= IFF_UP;
			break;
		}
		break;
	case SIOCSIFFLAGS:
		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */

		TR("===ioctl sifflags\n");
		if(ifp->if_flags & IFF_UP){
			synopGMAC_linux_open(adapter);
		}
		break;
/*
	case SIOCETHTOOL:
		{
		long *p=data;
		myRTL = sc;
		cmd_setmac(p[0],p[1]);
		}
		break;
*/
	case SIOCWRPHY:
	case SIOCWREEPROM:
		{
		long *p=data;
		int ac;
		char **av;
		int phybase;
		synopGMACdevice * gmacdev;
		ac = p[0];
		av = p[1];
		gmacdev = (synopGMACdevice *)adapter->synopGMACdev;
		phybase = gmacdev->PhyBase;
		if(ac>1) {
			int i;
			int data;
			for(i=1;i<ac;i++) {
				char *p=av[i];
				char *nextp;
				int offset=strtoul(p,&nextp,0);
				while(*nextp && nextp!=p) {
					p=++nextp;
					data=strtoul(p,&nextp,0);
					if(nextp==p)break;
					synopGMAC_write_phy_reg(gmacdev->MacBase,phybase,offset, data);
				}
			}
		}
		}
		break;
	case SIOCRDPHY:
	case SIOCRDEEPROM:
		{
		long *p=data;
		int ac;
		char **av;
		int i;
		int phybase;
		unsigned data;
		synopGMACdevice * gmacdev;
		ac = p[0];
		av = p[1];
		gmacdev = (synopGMACdevice *)adapter->synopGMACdev;
		if(ac<2)
			phybase = gmacdev->PhyBase;
		else
			phybase = strtoul(av[1],0,0);
		for(i=0;i<32;i++) {
			data = 0;
			synopGMAC_read_phy_reg(gmacdev->MacBase,phybase,i, &data);
			if((i&0xf)==0)printf("\n%02x: ",i);
			printf("%04x ",data);
		}
		printf("\n");
		}
		break;
	default:
		printf("===ioctl default\n");
		error = EINVAL;
	}

	splx(s);

	if(error)
		printf("===ioctl error: %d\n",error);
	return (error);
}

/**
 * Function to initialize the Linux network interface.
 * 
 * Linux dependent Network interface is setup here. This provides 
 * an example to handle the network dependent functionality.
 *
 * \return Returns 0 on success and Error code on failure.
 */
s32  synopGMAC_init_network_interface(char* xname, u64 synopGMACMappedAddr)
{
	struct ifnet *ifp;

	static u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;
	static int inited = 0;
	int i, ret;
	struct synopGMACNetworkAdapter *synopGMACadapter;

	if(!inited) {
		int32_t v;
		char *s = getenv("ethaddr");
		if (s) {
			int allz, allf;
			u8 macaddr[6];

			for (i=0, allz=1, allf=1; i<6; i++) {
				gethex(&v, s, 2);
				macaddr[i] = (u8)v;
				s += 3;         /* Don't get to fancy here :-) */
				if(v != 0) allz = 0;
				if(v != 0xff) allf = 0;
			}
			if (!allz && !allf)
				memcpy(mac_addr0, macaddr, 6);
		}
		inited = 1;
	}
#if defined(LS1ASOC)
	*((volatile unsigned int*)0xbfd00420) &= ~0x00800000;	/* 使能GMAC0 */
	#ifdef CONFIG_GMAC0_100M
	*((volatile unsigned int*)0xbfd00420) |= 0x500;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00420) &= ~0x500;		/* 否则配置成千兆模式 */
	#endif
	if (synopGMACMappedAddr == 0xbfe20000) {
		*((volatile unsigned int*)0xbfd00420) &= ~0x01000000;	/* 使能GMAC1 */
		#ifdef CONFIG_GMAC1_100M
		*((volatile unsigned int*)0xbfd00420) |= 0xa00;		/* 配置成百兆模式 */
		#else
		*((volatile unsigned int*)0xbfd00420) &= ~0xa00;		/* 否则配置成千兆模式 */
		#endif
		#ifdef GMAC1_USE_UART01
		*((volatile unsigned int*)0xbfd00420) |= 0xc0;
		#else
		*((volatile unsigned int*)0xbfd00420) &= ~0xc0;
		#endif
	}
#elif defined(LS1BSOC)
	/* 寄存器0xbfd00424有GMAC的使能开关 */
	*((volatile unsigned int*)0xbfd00424) &= ~0x1000;	/* 使能GMAC0 */
	#ifdef CONFIG_GMAC0_100M
	*((volatile unsigned int*)0xbfd00424) |= 0x5;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00424) &= ~0x5;	/* 否则配置成千兆模式 */
	#endif
	/* GMAC1初始化 使能GMAC1 和UART0复用，导致UART0不能使用 */
	if (synopGMACMappedAddr == 0xbfe20000) {
		*((volatile unsigned int*)0xbfd00420) |= 0x18;
		*((volatile unsigned int*)0xbfd00424) &= ~0x2000;	/* 使能GMAC1 */
		#ifdef CONFIG_GMAC1_100M
		*((volatile unsigned int*)0xbfd00424) |= 0xa;		/* 配置成百兆模式 */
		#else
		*((volatile unsigned int*)0xbfd00424) &= ~0xa;	/* 否则配置成千兆模式 */
		#endif
	}
#elif defined(LS1CSOC)
	*((volatile unsigned int *)0xbfd00424) &= ~(7 << 28);
#ifdef RMII
    *((volatile unsigned int *)0xbfd00424) |= (1 << 30); //wl rmii
#endif
/*    *((volatile unsigned int *)0xbfd011c0) &= 0x000fffff; //gpio[37:21] used as mac
    *((volatile unsigned int *)0xbfd011c4) &= 0xffffffc0;
    *((volatile unsigned int *)0xbfd011d0) &= 0x000fffff;
    *((volatile unsigned int *)0xbfd011d4) &= 0xffffffc6;
    *((volatile unsigned int *)0xbfd011e0) &= 0x000fffff;
    *((volatile unsigned int *)0xbfd011e4) &= 0xffffffc0;
    *((volatile unsigned int *)0xbfd011f0) &= 0x000fffff;
    *((volatile unsigned int *)0xbfd011f4) &= 0xffffffc0;*/
#endif
	
	TR("Now Going to Call register_netdev to register the network interface for GMAC core\n");

	synopGMACadapter = (struct synopGMACNetworkAdapter * )plat_alloc_memory(sizeof(struct synopGMACNetworkAdapter)); 
	memset((char *)synopGMACadapter, 0, sizeof(struct synopGMACNetworkAdapter));

	synopGMACadapter->synopGMACdev = NULL;
	synopGMACadapter->PInetdev = NULL;

	/*Allocate Memory for the the GMACip structure*/
	synopGMACadapter->synopGMACdev = (synopGMACdevice *)plat_alloc_memory(sizeof(synopGMACdevice));
	memset((char *)synopGMACadapter->synopGMACdev, 0, sizeof(synopGMACdevice));
	if(!synopGMACadapter->synopGMACdev) {
		TR0("Error in Memory Allocataion \n");
	}

	/*Allocate Memory for the the GMAC-Pmon structure	sw*/
	synopGMACadapter->PInetdev = (struct PmonInet *)plat_alloc_memory(sizeof(struct PmonInet));
	memset((char *)synopGMACadapter->PInetdev, 0, sizeof(struct PmonInet));
	if(!synopGMACadapter->PInetdev){
		TR0("Error in Pdev-Memory Allocataion \n");
	}

	ret = synopGMAC_attach(synopGMACadapter->synopGMACdev, (u64)synopGMACMappedAddr + MACBASE, (u64)synopGMACMappedAddr + DMABASE, DEFAULT_PHY_BASE, mac_addr0);
	if (ret) {
		return 1;
	}
	init_phy(synopGMACadapter->synopGMACdev);

	ifp = &(synopGMACadapter->PInetdev->arpcom.ac_if);
	ifp->if_softc = synopGMACadapter;

	memcpy(synopGMACadapter->PInetdev->dev_addr, mac_addr0, 6);

	/* set mac addr manually */
	bcopy(synopGMACadapter->PInetdev->dev_addr, synopGMACadapter->PInetdev->arpcom.ac_enaddr, sizeof(synopGMACadapter->PInetdev->arpcom.ac_enaddr));
	bcopy(xname, ifp->if_xname, IFNAMSIZ);

	ifp->if_start = (void *)synopGMAC_linux_xmit_frames;
	ifp->if_ioctl = (int *)gmac_ether_ioctl;
//	ifp->if_ioctl = (int *)synopGMAC_dummy_ioctl;
	ifp->if_reset = (int *)synopGMAC_dummy_reset;
	ifp->if_snd.ifq_maxlen = TRANSMIT_DESC_SIZE - 1;	//defined in Dev.h value is 12, too small?

	/*Now start the network interface*/
	TR("\nNow Registering the netdevice\n");
	if_attach(ifp);
	ether_ifattach(ifp);
	ifp->if_flags = ifp->if_flags | IFF_RUNNING;

	mac_addr0[5]++;

	/* MII setup */
/*	synopGMACadapter->mii.phy_id_mask = 0x1F;
	synopGMACadapter->mii.reg_num_mask = 0x1F;
	synopGMACadapter->mii.dev = synopGMACadapter;
	synopGMACadapter->mii.mdio_read = mdio_read;
	synopGMACadapter->mii.mdio_write = mdio_write;
	synopGMACadapter->mii.phy_id = synopGMACadapter->synopGMACdev->PhyBase;
	synopGMACadapter->mii.supports_gmii = mii_check_gmii_support(&synopGMACadapter->mii);*/

	return 0;
}

