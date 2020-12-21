#NAME: Samuel Shen
#EMAIL: sam.shen321@gmail.com
#ID: 405325252

CC = gcc

build:
	gcc -Wall -Wextra -pthread -o lab2_add lab2_add.c
	gcc -Wall -Wextra -pthread -o lab2_list lab2_list.c SortedList.c

tests: build
	python3 generate_add.py
	python3 generate_list.py

graphs: tests
	./lab2_add.gp
	./lab2_list.gp

dist: graphs
	tar -czvf lab2a-405325252.tar.gz lab2_add.c SortedList.h SortedList.c lab2_list.c Makefile lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png generate_add.py generate_list.py README lab2_add.gp lab2_list.gp
clean:
	rm -f lab2_add
	rm -f lab2_list
	rm -f lab2a-405325252.tar.gz
