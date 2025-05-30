
lim: lim.c files.c
	gcc lim.c files.c -lncurses && ./a.out testfile.txt

debug: lim.c files.c
	gcc -g lim.c files.c -lncurses && gdb --args ./a.out testfile.txt

