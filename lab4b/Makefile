#NAME: Samuel Shen
#EMAIL: sam.shen321@gmail.com
#ID: 405325252

CC = gcc

default:
	gcc -Wall -Wextra -g -o lab4b -lmraa -lm lab4b.c
check:
	./commtest.sh
	
clean:
	rm -f lab4b-405325252.tar.gz
	rm -f lab4b
dist:
	tar -czvf lab4b-405325252.tar.gz lab4b.c Makefile README commtest.sh
