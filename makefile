
lim: lim.c
	gcc -g lim.c -lcurses && ./a.out testfile.txt

debug: lim.c
	gcc -g lim.c -lcurses && gdb --args ./a.out testfile.txt

game: game.c
	gcc game.c -lcurses && ./a.out
