APP = giftest
CC = arm-elf-gcc
AS = arm-elf-as
GCCBASE = c:/devkitpro/devkitARM
LIBGBA = c:/devkitpro/libgba/include
GCCDIR = $(GCCBASE)/lib/gcc/arm-elf/4.0.1
GBAINC = -I $(GCCDIR)/include -I $(LIBGBA)
ARCH = -mthumb -mthumb-interwork
AFLAGS = $(GBAINC) $(ARCH)
CFLAGS = -c -g -Wall -D_GBA_OPTMEM -D_GBA_NO_FILEIO -O3 $(ARCH) -mcpu=arm7tdmi -ffast-math -I. $(GBAINC)
LFLAGS = $(ARCH) -specs=gba_mb.specs -Wl,-Map,$(APP).map

build : $(APP).gba $(APP).mb

all: clean build

$(APP).gba : $(APP).elf
	arm-elf-objcopy -v -O binary $(APP).elf $@

$(APP).mb : $(APP).gba
	gba-header -i $(APP).gba -o $(APP).mb -fix

$(APP).elf : giftest.o dgif_lib.o gif_err.o gifalloc.o
	$(CC) -o $@ $(LFLAGS) giftest.o dgif_lib.o gif_err.o gifalloc.o

giftest.o : giftest.c res/cover.c res/porsche-240x160.c res/x-trans.c
	$(CC) $(CFLAGS) giftest.c

dgif_lib.o : ../lib/dgif_lib.c
	$(CC) $(CFLAGS) ../lib/dgif_lib.c

gif_err.o : ../lib/gif_err.c
	$(CC) $(CFLAGS) ../lib/gif_err.c

gifalloc.o : ../lib/gifalloc.c
	$(CC) $(CFLAGS) ../lib/gifalloc.c

res/cover.c : res/cover.gif
	bin2c $*.gif $@ cover

res/porsche-240x160.c : res/porsche-240x160.gif
	bin2c $*.gif $@ porsche_240x160

res/x-trans.c : res/x-trans.gif
	bin2c $*.gif $@ x_trans

clean :
	rm -f *.o $(APP).elf $(APP).map $(APP).gba $(APP).mb res/*.c

cln : clean
	rm -f $(APP).dsw $(APP).ncb $(APP).opt $(APP).plg
	rmdir Debug
