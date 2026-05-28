CC = zig cc -target aarch64-linux-gnu
SYSROOT_INC = $(HOME)/clib/aarch64-linux-gnu/include
SYSROOT_LIB = $(HOME)/clib/aarch64-linux-gnu/lib

CFLAGS = -I. -I$(SYSROOT_INC)
LDFLAGS = -L. -L$(SYSROOT_LIB)
LDLIBS = -lwiringPi -lcrypt
LIBOBJS = ledControl.o cdsControl.o cdsAdcControl.o
SENDOBJS = main libcontrol.so pinnum.yaml

all: main send

%.o: %.c
	$(CC) $(CFLAGS) -c -fPIC $<

libcontrol.so: $(LIBOBJS)
	$(CC) $(LDFLAGS) -shared -o libcontrol.so $(LIBOBJS) $(LDLIBS)

main: main.c libcontrol.so
	$(CC) $(CFLAGS) $(LDFLAGS) -rdynamic main.c -o main -ldl -lpthread

send: $(SENDOBJS)
	scp $(SENDOBJS) Rappa:~/Documents/vscode/library_ftn_practice

clean:
	rm -f *.o *.so main