
all:  lwdhcp

lwdhcp: lwdhcp.o packet.o options.o
	cc -o lwdhcp packet.o options.o lwdhcp.o 

lwdhcp.o: lwdhcp.c lwdhcp.h
	cc -c lwdhcp.c

packet.o: packet.c lwdhcp.h packet.h
	cc -c packet.c

options.o: options.c options.h
	cc -c options.c

clean: 
	-rm -f lwdhcp *.o

