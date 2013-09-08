all: udpserv

udpserv: popen_arr.c popen_arr.h Makefile udpserv.c
	${CC} -ggdb -Wall popen_arr.c udpserv.c -o udpserv
	
popen_arr_test: popen_arr.c popen_arr.h popen_arr_test.c Makefile
	${CC} -ggdb -Wall popen_arr_test.c popen_arr.c -o popen_arr_test
