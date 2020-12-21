#NAME: Samuel Shen
#EMAIL: sam.shen321@gmail.com
#ID: 405325252

CC = gcc

build:
	gcc -Wall -Wextra -pthread -g -lprofiler -o lab2_list lab2_list.c SortedList.c

profile:
	LD_PRELOAD=/usr/lib64/libprofiler.so
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=runThread ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

tests: build profile
	python3 generate_list.py

graphs: tests
	./lab2b_list.gp

dist: graphs
	tar -czvf lab2b-405325252.tar.gz SortedList.h SortedList.c lab2_list.c Makefile lab2b_list.csv profile.out lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png generate_list.py README lab2b_list.gp

clean:
	rm -f lab2_list
	rm -f lab2b-405325252.tar.gz
