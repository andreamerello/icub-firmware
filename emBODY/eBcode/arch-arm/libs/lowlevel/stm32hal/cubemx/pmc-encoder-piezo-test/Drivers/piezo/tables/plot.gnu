set datafile separator ","
plot "raw_table.csv" using 1 with linespoints lc rgb "blue", "interpolated_table.csv" using 1 with linespoints lc rgb "red", "interpolated_and_scaled_table.csv" using 1 with linespoints lc rgb "green"
pause mouse close
