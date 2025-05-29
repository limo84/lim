
lim: lim.c files.c
	gcc lim.c files.c -lcurses && ./a.out testfile.txt

debug: lim.c files.c
	gcc -g lim.c files.c -lcurses && gdb --args ./a.out testfile.txt

