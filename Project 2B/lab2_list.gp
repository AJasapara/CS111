#! /usr/local/cs/bin/gnuplot
#
# NAME: Arpit Jasapara
# EMAIL: ajasapara@ucla.edu
# ID: 504742401
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#	lab2b_3.png ... successful iterations vs. threads for each synchronization method.
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists.
#	lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "Aggregate Throughput vs Number of Threads"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Aggregate Throughput"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep -e \'list-none-m,[0-9]*,1000,1,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'synchronized list w/mutex' with linespoints lc rgb 'blue', \
     "< grep -e \'list-none-s,[0-9]*,1000,1,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'synchronized list w/spin-lock' with linespoints lc rgb 'green'


set title "Average Time per Operation and Average Time per Mutex Wait"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep -e \'list-none-m,[0-9]*,1000,1,\' lab2b_list.csv" using ($2):($7) \
	title 'time per op' with linespoints lc rgb 'blue', \
     "< grep -e \'list-none-m,[0-9]*,1000,1,\' lab2b_list.csv" using ($2):($8) \
	title 'avg mutex wait time' with linespoints lc rgb 'green'
     
set title "Successful Iterations vs. Number of Threads"
set logscale x 2
set xrange [0.75:]
set xlabel "Number of Threads"
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep -e \'list-id-none,[0-9]*,[0-9]*,4,\' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "red" title "unprotected", \
    "< grep -e \'list-id-m,[0-9]*,[0-9]*,4,\' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "blue" title "w/mutex", \
    "< grep -e \'list-id-s,[0-9]*,[0-9]*,4,\' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "green" title "w/spin-lock"

set title "Throughput vs. Number of Threads for Mutex"
set xlabel "Number of Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'
plot \
     "< grep -e \'list-none-m,[0-9]*,1000,1,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'red', \
     "< grep -e \'list-none-m,[0-9]*,1000,4,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 lists' with linespoints lc rgb 'orange', \
     "< grep -e \'list-none-m,[0-9]*,1000,8,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 lists' with linespoints lc rgb 'green', \
     "< grep -e \'list-none-m,[0-9]*,1000,16,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 lists' with linespoints lc rgb 'blue'

set title "Throughput vs. Number of Threads for Spin-Lock"
set xlabel "Number of Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_5.png'
plot \
     "< grep -e \'list-none-s,[0-9]*,1000,1,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'red', \
     "< grep -e \'list-none-s,[0-9]*,1000,4,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 lists' with linespoints lc rgb 'orange', \
     "< grep -e \'list-none-s,[0-9]*,1000,8,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 lists' with linespoints lc rgb 'green', \
     "< grep -e \'list-none-s,[0-9]*,1000,16,\' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 lists' with linespoints lc rgb 'blue'