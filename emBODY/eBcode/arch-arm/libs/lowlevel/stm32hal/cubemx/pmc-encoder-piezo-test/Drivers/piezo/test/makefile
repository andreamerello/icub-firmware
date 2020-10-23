all: piezo_test.o
	gcc -g piezo_test.c ../piezo_ad5664.c ../dmaspi.c ../gpl_utils.c test_tables.c -I ./ -o piezo_test -lpthread
