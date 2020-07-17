run: test.c LibFS.c LibDisk.c
	gcc -c LibDisk.c LibFS.c test.c
	gcc test.o LibFS.o LibDisk.o -o test
	./test