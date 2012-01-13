static int cputest(void)
{
//	static volatile float x=1.2342374;	//THF
//	static volatile float y=9.153784;	//THF
//	static volatile float z;				//THF
	char cmd[100];
#ifdef CONFIG_CHINESE
	printf("CP0 寄存器读测试\n");
#else
	printf("CP0 register read test\n");
#endif

	asm(\
	"mfc0 $0,$0;\n" \
	"mfc0 $0,$1;\n" \
	"mfc0 $0,$2;\n" \
	"mfc0 $0,$3;\n" \
	"mfc0 $0,$4;\n" \
	"mfc0 $0,$5;\n" \
	"mfc0 $0,$6;\n" \
	"mfc0 $0,$7;\n" \
	"mfc0 $0,$8;\n" \
	"mfc0 $0,$9;\n" \
	"mfc0 $0,$10;\n" \
	"mfc0 $0,$11;\n" \
	"mfc0 $0,$12;\n" \
	"mfc0 $0,$13;\n" \
	"mfc0 $0,$14;\n" \
	"mfc0 $0,$15;\n" \
	"mfc0 $0,$16;\n" \
	"mfc0 $0,$17;\n" \
	"mfc0 $0,$18;\n" \
	"mfc0 $0,$19;\n" \
	"mfc0 $0,$20;\n" \
	"mfc0 $0,$21;\n" \
	"mfc0 $0,$22;\n" \
	"mfc0 $0,$23;\n" \
	"mfc0 $0,$24;\n" \
	"mfc0 $0,$25;\n" \
	"mfc0 $0,$26;\n" \
	"mfc0 $0,$27;\n" \
	"mfc0 $0,$28;\n" \
	"mfc0 $0,$29;\n" \
	"mfc0 $0,$30;\n" \
	"mfc0 $0,$31;\n" \
	:::"$2","$3"
	);
#ifdef CONFIG_CHINESE
	printf("CP0 寄存器读测试通过\n");
//	sprintf(cmd, "set cputest \"%s\"", "测试通过");
#else
	printf("CP0 register read test pass.\n");
//	sprintf(cmd, "set cputest \"%s\"", "pass");
#endif
//	do_cmd(cmd);
//	printf("cpu float calculation test\n");	//THF
//	z=x*y*10000;	//THF
//	if((int)z==112979) printf("cpu float calculation test ok\n");	//THF
//	else printf("cpu float calculation test error\n");	//THF
	return 0;
}
