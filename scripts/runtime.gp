reset
set ylabel 'time(sec)'
set style fill solid
set title 'perfomance comparison'
set term png enhanced font 'Verdana,10'
set output 'runtime.png'

plot [:][0:0.010]'plot.txt' using 2:xtic(1) with histogram title 'threadpool' 
