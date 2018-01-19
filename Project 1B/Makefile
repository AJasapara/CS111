#NAME: Arpit Jasapara
#EMAIL: ajasapara@ucla.edu
#ID: XXXXXXXXX

.SILENT:

default: 
	gcc --save-temps -g -Wall -Wextra -o lab1b-client lab1b-client.c -lmcrypt
	gcc --save-temps -g -Wall -Wextra -o lab1b-server lab1b-server.c -lmcrypt
	rm -f *.i *.o *.s
client:
	gcc --save-temps -g -Wall -Wextra -o lab1b-client lab1b-client.c -lmcrypt
server:
	gcc --save-temps -g -Wall -Wextra -o lab1b-server lab1b-server.c -lmcrypt
clean:
	rm -f lab1b-server lab1b-client lab1b-XXXXXXXXX.tar.gz
dist:
	tar -czf lab1b-XXXXXXXXX.tar.gz lab1b-client.c lab1b-server.c Makefile README my.key