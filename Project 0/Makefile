#NAME: Arpit Jasapara
#EMAIL: ajasapara@ucla.edu
#ID: XXXXXXXXX

.SILENT:

default: 
	gcc -g -Wall -Wextra -o lab0 lab0.c
clean:
	rm -f lab0 lab0-504742401.tar.gz
check:
	echo "test" > testIn.txt
	./lab0 --input=testIn.txt --output=testOut.txt
	if [ $$? -ne 0 ] ; then \
		echo "Failed read/write test" ; \
		exit 1 ; \
	fi
	diff testIn.txt testOut.txt
	if [ $$? -ne 0 ] ; then \
		echo "Failed to copy exactly" ; \
		exit 1 ; \
	fi
	rm -f testIn.txt testOut.txt
	./lab0 --wrongArg &> /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		echo "Failed identifying wrong argument test" ; \
		exit 1 ; \
	fi
	./lab0 --input=fakeFile &> /dev/null ; \
	if [ $$? -ne 2 ] ; then \
		echo "Failed opening input file test" ; \
		exit 1 ; \
	fi
	touch rdOnly
	chmod 444 rdOnly
	./lab0 --output=rdOnly &> /dev/null ; \
	if [ $$? -ne 3 ] ; then \
		echo "Failed opening output file test" ; \
		exit 1 ; \
	fi
	rm -f rdOnly
	./lab0 --segfault --catch &> /dev/null ; \
	if [ $$? -ne 4 ] ; then \
		echo "Failed SIGSEGV catch test" ; \
		exit 1 ; \
	fi
	echo "All tests passed successfully"
dist:
	tar -czf lab0-504742401.tar.gz lab0.c backtrace.png breakpoint.png Makefile README