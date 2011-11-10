void
tgt_reboot()
{

	void (*longreach) (void);
	
	longreach = (void *)0xbfc00000;
	(*longreach)();

	while(1);

}
