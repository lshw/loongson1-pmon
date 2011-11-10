
static struct pci_driver e1000_driver = {
	.name     = e1000_driver_name,
	.id_table = e1000_pci_tbl,
	.probe    = e1000_probe,
//	.remove   = __devexit_p(e1000_remove),
	/* Power Managment Hooks */
#ifdef CONFIG_PM
	.suspend  = e1000_suspend,
	.resume   = e1000_resume
#endif
};

#if 0
struct notifier_block e1000_notifier_reboot = {
	.notifier_call	= e1000_notify_reboot,
	.next		= NULL,
	.priority	= 0
};
static int
e1000_notify_reboot(struct notifier_block *nb, unsigned long event, void *p)
{
	struct pci_dev *pdev = NULL;

	switch(event) {
	case SYS_DOWN:
	case SYS_HALT:
	case SYS_POWER_OFF:
		pci_for_each_dev(pdev) {
			if(pci_dev_driver(pdev) == &e1000_driver)
				e1000_suspend(pdev, 3);
		}
	}
	return NOTIFY_DONE;
}
#endif

static int __devinit
e1000_probe(struct pci_dev *pdev,
            const struct pci_device_id *ent)
{
	struct net_device *netdev;
	struct e1000_adapter *adapter;
	static int cards_found = 0;
	unsigned long mmio_start;
	int mmio_len;
	int pci_using_dac;
	int i;
	uint16_t eeprom_data;

	if((i = pci_enable_device(pdev)))
		return i;

	if(!(i = pci_set_dma_mask(pdev, PCI_DMA_64BIT))) {
		pci_using_dac = 1;
	} else {
		if((i = pci_set_dma_mask(pdev, PCI_DMA_32BIT))) {
			E1000_ERR("No usable DMA configuration, aborting\n");
			return i;
		}
		pci_using_dac = 0;
	}

	if((i = pci_request_regions(pdev, e1000_driver_name)))
		return i;

	pci_set_master(pdev);

	netdev = alloc_etherdev(sizeof(struct e1000_adapter));
	if(!netdev)
		goto err_alloc_etherdev;

	SET_MODULE_OWNER(netdev);

	pci_set_drvdata(pdev, netdev);
	if(em_probe(netdev,ent,pdev)<0)goto err_alloc_etherdev;

	netdev->open = &e1000_open;
	netdev->stop = &e1000_close;
	netdev->hard_start_xmit = &e1000_xmit_frame;
	netdev->get_stats = &e1000_get_stats;
	netdev->set_multicast_list = &e1000_set_multi;
	netdev->set_mac_address = &e1000_set_mac;
	netdev->change_mtu = &e1000_change_mtu;
	netdev->do_ioctl = &e1000_ioctl;
	netdev->tx_timeout = &e1000_tx_timeout;
	netdev->watchdog_timeo = 5 * HZ;

	register_netdev(netdev);
	return 0;
err_alloc_etherdev:
	pci_release_regions(pdev);
	return -ENOMEM;
}


MODULE_AUTHOR("Intel Corporation, <linux.nics@intel.com>");
MODULE_DESCRIPTION("Intel(R) PRO/1000 Network Driver");
MODULE_LICENSE("GPL");

/**
 * e1000_init_module - Driver Registration Routine
 *
 * e1000_init_module is the first routine called when the driver is
 * loaded. All it does is register with the PCI subsystem.
 **/

static int __init
e1000_init_module(void)
{
	int ret;
	printk(KERN_INFO "%s - version %s\n",
	       e1000_driver_string, e1000_driver_version);

	printk(KERN_INFO "%s\n", e1000_copyright);

	ret = pci_module_init(&e1000_driver);
	if(ret >= 0) {
	//	register_reboot_notifier(&e1000_notifier_reboot);
	}
	return ret;
}

module_init(e1000_init_module);

/**
 * e1000_exit_module - Driver Exit Cleanup Routine
 *
 * e1000_exit_module is called just before the driver is removed
 * from memory.
 **/

static void __exit
e1000_exit_module(void)
{
	//unregister_reboot_notifier(&e1000_notifier_reboot);
	pci_unregister_driver(&e1000_driver);
}

module_exit(e1000_exit_module);
