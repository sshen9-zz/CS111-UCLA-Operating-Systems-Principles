#! /usr/local/cs/bin/gnuplot

set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "Throughput vs #Threads at 1000 Iterations"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep -E 'list-none-m,[0-9]{1,2},1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title 'Mutex lock' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[0-9]{1,2},1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title 'Spin lock' with linespoints lc rgb 'green'
    
#grep the same things as before    
set title "Mutex lock wait time & avg time/op vs threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep -E 'list-none-m,[0-9]{1,2},1000,1,' lab2b_list.csv" using ($2):($8) \
    title 'wait for lock time' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[0-9]{1,2},1000,1,' lab2b_list.csv" using ($2):($7) \
    title 'avg time/op' with linespoints lc rgb 'green'


set title "Different Syncs and Iterations that run without failure"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep -E 'list-id-none,[0-9]{1,2},[0-9]{1,2},4' lab2b_list.csv" using ($2):($3) \
    title 'no sync' with points lc rgb 'green', \
     "< grep -E 'list-id-m,[0-9]{1,2},[0-9]{1,2},4' lab2b_list.csv" using ($2):($3) \
    title 'mutex' with points lc rgb 'red', \
     "< grep -E 'list-id-s,[0-9]{1,2},[0-9]{1,2},4' lab2b_list.csv" using ($2):($3) \
    title 'spin' with points lc rgb 'violet'


#GRAPH 4 AND 5

set title "Throughput vs #Threads at 1000 Iterations for each List (Mutex)"                                                              
set xlabel "Threads"                                                                                                                     
set logscale x 2                                                                                                                         
set ylabel "Throughput (ops/sec)"                                                                                                        
set logscale y 10                                                                                                                        
set output 'lab2b_4.png'                                                                                                                 
                                                                                                                                         
plot \
     "< grep -E 'list-none-m,[0-9]{1,2},1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '1 List' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-m,[0-9]{1,2},1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '4 Lists' with linespoints lc rgb 'green', \
     "< grep -E 'list-none-m,[0-9]{1,2},1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '8 Lists' with linespoints lc rgb 'blue', \
     "< grep -E 'list-none-m,[0-9]{1,2},1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '16 Lists' with linespoints lc rgb 'yellow'



set title "Throughput vs #Threads at 1000 Iterations for each List (Spin)"                                                              
set xlabel "Threads"                                                                                                                     
set logscale x 2                                                                                                                         
set ylabel "Throughput (ops/sec)"                                                                                                        
set logscale y 10                                                                                                                        
set output 'lab2b_5.png'                                                                                                                 
                                                                                                                                         
plot \
     "< grep -E 'list-none-s,[0-9]{1,2},1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '1 List' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[0-9]{1,2},1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '4 Lists' with linespoints lc rgb 'green', \
     "< grep -E 'list-none-s,[0-9]{1,2},1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '8 Lists' with linespoints lc rgb 'blue', \
     "< grep -E 'list-none-s,[0-9]{1,2},1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '16 Lists' with linespoints lc rgb 'yellow'