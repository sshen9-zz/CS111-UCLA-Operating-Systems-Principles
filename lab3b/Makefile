#NAME: Samuel Shen, Chandrasekhar Suresh
#EMAIL: sam.shen321@gmail.com, chandra.b.suresh@gmail.com
#ID: 405325252, 605304814

CC = gcc

default: lab3b_c
#touch lab3b
	cp lab3b_c lab3b
	chmod +x lab3b
	echo "Success"

dist: lab3b.py README Makefile lab3b_c
	tar -czvf lab3b-405325252.tar.gz lab3b.py README Makefile lab3b_c

clean:
	rm -f lab3b-405325252.tar.gz
	rm -f lab3b
