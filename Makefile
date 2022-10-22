INCLUDES := -I /usr/include/gtk-3.0/ -I /usr/include/glib-2.0/ -I usr/lib/glib-2.0/include/ \
						-I /usr/lib/x86_64-linux-gnu/glib-2.0/include/ -I /usr/include/pango-1.0/ \
						-I /usr/include/harfbuzz/ -I /usr/include/cairo/ -I /usr/include/gdk-pixbuf-2.0/ \
						-I /usr/include/atk-1.0/

LINK := -lgtk-3 -lgdk-3 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lcairo \
			  -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0

# SOURCES := $(shell find . -name "*.c")
SOURCES := main.c lim_functions/lim_functions.c

all:
	make app && ./app main.c

app: $(SOURCES)
	gcc $(SOURCES) -g -o app $(INCLUDES) $(LINK)
