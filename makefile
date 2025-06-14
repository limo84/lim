DEFINES := -DSHOW_BAR=1

lim: lim.c files.c gap_buffer.c
	gcc lim.c files.c -lncurses $(DEFINES) && ./a.out testfile.txt

debug: lim.c files.c gap_buffer.c
	gcc -g lim.c files.c -lncurses && gdb --args ./a.out testfile.txt

#needs sudo
install: lim.c files.c gap_buffer.c
	rm -rf ./bin
	mkdir -p ./bin
	gcc lim.c files.c -lncurses -DSHOW_BAR=0 -o bin/lim
	cp -f ./bin/lim /usr/bin/lim
