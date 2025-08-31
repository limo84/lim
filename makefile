DEFINES := -DDEBUG_BAR=1

lim: lim.c files.c gap_buffer.c
	gcc -g lim.c files.c -lncurses $(DEFINES) && ./a.out testfile.txt

debug: lim.c files.c gap_buffer.c
	gcc -O0 -g3 lim.c files.c -lncurses $(DEFINES) && gdb -tui --args ./a.out testfile.txt

#needs sudo
install: lim.c files.c gap_buffer.c
	rm -rf ./bin
	mkdir -p ./bin
	gcc lim.c files.c -lncurses -DDEBUG_BAR=0 -o bin/lim
	cp -f ./bin/lim /usr/bin/lim
	rm -rf ./bin
