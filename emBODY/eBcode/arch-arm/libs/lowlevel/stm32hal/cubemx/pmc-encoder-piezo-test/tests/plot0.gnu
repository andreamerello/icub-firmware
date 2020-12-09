set datafile separator ","
plot "test_move_out.csv" using 1 with linespoints lc rgb "red", "test_move_out.csv" using 2 with linespoints lc rgb "blue"
pause mouse close
