#NAME: Samuel Shen, Chandrasekhar Suresh
#EMAIL: sam.shen321@gmail.com, chandra.b.suresh@gmail.com
#ID: 405325252, 605304814

default: ./lab3a.c
	gcc -g -Wall -Wextra lab3a.c -o lab3a


clean:
	rm -f ./lab3a lab3a-605304814.tar.gz

dist: ./lab3a.c Makefile README
	tar -czvf lab3a-605304814.tar.gz ./lab3a.c Makefile README ext2_fs.h
