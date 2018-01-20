#NAME: Arpit Jasapara
#EMAIL: ajasapara@ucla.edu
#ID: XXXXXXXXX

.SILENT:

default:
	gcc -Wall -Wextra -g -lmraa -pthread -o lab4b lab4b.c -lm
check: default
	./lab4b --log=tempFile.txt
	sleep 5
	OFF
	rm -f tempFile.txt
clean:
	rm -f lab4b-XXXXXXXXX.tar.gz lab4b
dist:
	tar -czf lab4b-XXXXXXXXX.tar.gz README Makefile lab4b.c