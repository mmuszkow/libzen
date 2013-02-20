CC=gcc
CFLAGS=-Wall -c -O2 -DNP_DRIVER_DEATTACH
CONS_OUT=zen_console
GTK_OUT=zen_tray
GTK_FLAGS=`pkg-config --libs --cflags gtk+-3.0`
OBJS=libzen.o libusb-attach-dev.o

all: tray console

console: console.o $(OBJS)
	$(CC) console.o $(OBJS) -lusb -o $(CONS_OUT)

tray: tray.o $(OBJS)
	$(CC) tray.o $(OBJS) $(GTK_FLAGS) -lusb -o $(GTK_OUT)

libzen.o: src/libzen.c
	$(CC) $(CFLAGS) src/libzen.c

libusb-attach-dev.o: src/libusb-attach-dev.c
	$(CC) $(CFLAGS) src/libusb-attach-dev.c

console.o: src/console.c
	$(CC) $(CFLAGS) src/console.c

tray.o: src/tray.c
	$(CC) $(CFLAGS) $(GTK_FLAGS) src/tray.c

clean:
	rm *.o
	rm $(CONS_OUT)
	rm $(GTK_OUT)
