/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

/*
 *	Structure returned from eth_probe and passed to other driver
 *	functions.
 */

#ifndef USE_FLUSH_IO
#if defined(__NetBSD__) || defined(__OpenBSD__)
 #define	RTL_READ_1(sc, reg)						\
	bus_space_read_1((sc)->sc_st, (sc)->sc_sh, (reg))
 #define	RTL_READ_2(sc, reg)						\
	bus_space_read_2((sc)->sc_st, (sc)->sc_sh, (reg))
 #define	RTL_READ_4(sc, reg)						\
	bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (reg))
 #define	RTL_WRITE_1(sc, reg, val)					\
	bus_space_write_1((sc)->sc_st, (sc)->sc_sh, (reg), (val))
 #define	RTL_WRITE_2(sc, reg, val)					\
	bus_space_write_2((sc)->sc_st, (sc)->sc_sh, (reg), (val))
 #define	RTL_WRITE_4(sc, reg, val)					\
	bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (reg), (val))
#else
 #define	RTL_READ_1(sc, reg)						\
	(*((u_int8_t *)((sc)->csr + (reg))))
 #define	RTL_READ_2(sc, reg)						\
	(*((u_int16_t *)((sc)->csr + (reg))))
 #define	RTL_READ_4(sc, reg)						\
	(*((u_int32_t *)((sc)->csr + (reg))))
 #define	RTL_WRITE_1(sc, reg, val)					\
	(*((u_int8_t *)((sc)->csr + (reg)))) = (val)
 #define	RTL_WRITE_2(sc, reg, val)					\
	(*((u_int16_t *)((sc)->csr + (reg)))) = (val)
 #define	RTL_WRITE_4(sc, reg, val)					\
	(*((u_int32_t *)((sc)->csr + (reg)))) = (val)
#endif /* __NetBSD__ || __OpenBSD__ */

#else

#if defined(__NetBSD__) || defined(__OpenBSD__)
 #define	RTL_READ_1(sc, reg)						\
	bus_space_read_1((sc)->sc_st, (sc)->sc_sh, (reg))
 #define	RTL_READ_2(sc, reg)						\
	bus_space_read_2((sc)->sc_st, (sc)->sc_sh, (reg))
 #define	RTL_READ_4(sc, reg)						\
	bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (reg))
 #define	RTL_WRITE_1(sc, reg, val)					\
	do {  bus_space_write_1((sc)->sc_st, (sc)->sc_sh, (reg), (val))         \
	      bus_space_read_1((sc)->sc_st, (sc)->sc_sh, (reg), (val))	
 #define	RTL_WRITE_2(sc, reg, val)					\
	bus_space_write_2((sc)->sc_st, (sc)->sc_sh, (reg), (val))
 #define	RTL_WRITE_4(sc, reg, val)					\
	bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (reg), (val))
#else
 #define	RTL_READ_1(sc, reg)						\
	(*((u_int8_t *)((sc)->csr + (reg))))
 #define	RTL_READ_2(sc, reg)						\
	(*((u_int16_t *)((sc)->csr + (reg))))
 #define	RTL_READ_4(sc, reg)						\
	(*((u_int32_t *)((sc)->csr + (reg))))
 #define	RTL_WRITE_1(sc, reg, val)					\
	(*((u_int8_t *)((sc)->csr + (reg)))) = (val)
 #define	RTL_WRITE_2(sc, reg, val)					\
	(*((u_int16_t *)((sc)->csr + (reg)))) = (val)
 #define	RTL_WRITE_4(sc, reg, val)					\
	(*((u_int32_t *)((sc)->csr + (reg)))) = (val)
#endif /* __NetBSD__ || __OpenBSD__ */

#endif












struct nic
{
//most of the fields come from struct fxp_softc
#if defined(__NetBSD__) || defined(__OpenBSD__)
	struct device sc_dev;		/* generic device structures */
	void *sc_ih;			/* interrupt handler cookie */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	pci_chipset_tag_t sc_pc;	/* chipset handle needed by mips */
#else
	struct caddr_t csr;		/* control/status registers */
#endif /* __NetBSD__ || __OpenBSD__ */
#if defined(__OpenBSD__) || defined(__FreeBSD__)
	struct arpcom arpcom;		/* per-interface network data !!we use this*/
#endif
#if defined(__NetBSD__)
	struct ethercom sc_ethercom;	/* ethernet common part */
#endif
	struct mii_data sc_mii;		/* MII media information */
	char		node_addr[6]; //the net interface's address

	unsigned char *tx_ad[4];   // Transmit buffer
	unsigned char *tx_dma;     // used by dma
	unsigned char *tx_buffer;  // Transmit buffer
	unsigned char *tx_buf[4];  // used by driver coping data

	unsigned char *rx_dma;     // Receive buffer, used by dma	
	unsigned char *rx_buffer;     // used by driver 
	//We use mbuf here

	int		packetlen;
	void		*priv_data;	/* driver can hang private data here */
};

