#NAME: Samuel Shen
#EMAIL: sam.shen321@gmail.com
#ID: 405325252

CC = gcc

default:
	gcc -Wall -Wextra -g -o lab4c_tcp -lmraa -lm lab4c_tcp.c
	gcc -Wall -Wextra -g -o lab4c_tls -lssl -lcrypto -lmraa -lm lab4c_tls.c
clean:
	rm -f lab4c-405325252.tar.gz
	rm -f lab4c_tls
	rm -f lab4c_tcp
dist:
	tar -czvf lab4c-405325252.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
