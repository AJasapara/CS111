#NAME: Arpit Jasapara
#EMAIL: ajasapara@ucla.edu
#ID: XXXXXXXXX

.SILENT:

default:
	gcc -Wall -Wextra -g -lmraa -pthread -o lab4c_tcp lab4c_tcp.c -lm
	gcc -Wall -Wextra -g -lmraa -lssl -pthread -o lab4c_tls lab4c_tls.c -lm -lcrypto
clean:
	rm -f lab4c-XXXXXXXXX.tar.gz lab4c_tcp lab4c_tls
dist:
	tar -czf lab4c-XXXXXXXXX.tar.gz README Makefile lab4c_tcp.c lab4c_tls.c