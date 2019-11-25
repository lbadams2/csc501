defrag: main.o
	gcc -o defrag main.o

main.o: disk.h main.c
	gcc -g -c -O0 -Wall main.c

clean:
	rm *.o defrag