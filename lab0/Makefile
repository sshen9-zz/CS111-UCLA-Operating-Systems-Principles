# NAME: Samuel Shen
# EMAIL: sam.shen321@gmail.com
# ID: 405325252

CC = gcc

lab0: lab0.c
	gcc -Wall -Wextra -o lab0 lab0.c
check:
	./sanity.sh

clean:
	rm lab0
	rm lab0-405325252.tar.gz
dist:
	tar -czvf lab0-405325252.tar.gz lab0.c Makefile backtrace.png breakpoint.png README sanity.sh
