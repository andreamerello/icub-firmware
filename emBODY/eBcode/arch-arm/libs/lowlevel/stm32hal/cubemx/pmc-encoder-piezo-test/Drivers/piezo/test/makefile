all: piezo_test.o
	gcc -g piezo_test.c ../piezo_ad5664.c ../dmaspi.c test_tables.c -o piezo_test -lpthread
