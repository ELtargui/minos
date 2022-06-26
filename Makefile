COMPILER :=i686-pc-targui
CC := $(COMPILER)-gcc
AS := $(COMPILER)-as
LD := $(COMPILER)-ld
NM := $(COMPILER)-nm
AR := $(COMPILER)-ar
BASE := base

KCFLAGS  = -Wall -Wextra -Wno-format -Wno-unused-function -Wno-unused-parameter
KCFLAGS += -ffreestanding -g -O2 -std=gnu99 
KCFLAGS += -fsanitize=undefined


CFLAGS = -std=gnu99 -g -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter\
		 -Wno-format
# CFLAGS += -fsanitize=undefined

all:cdrom.iso
ubsan:base/usr/lib/libubsan.a

minKERNEL:cdrom/minKERNEL
LIBC := base/usr/lib/libc.a base/usr/lib/crt0.o base/usr/lib/crti.o base/usr/lib/crtn.o
LIB := base/usr/lib/libds.a\
		base/usr/lib/libgfx.a\
		base/usr/lib/libcompress.a\
		base/usr/lib/libgui.a\
		base/usr/lib/libubsan.a


BIN :=	base/bin/init \
		base/bin/hello \
		base/bin/about \
		base/bin/calculator \
		base/bin/taskbar \
		base/bin/menu \
		base/bin/files

APPS:= $(BASE)/bin/wm 

KOBJS =$(patsubst %.c,%.o,$(wildcard minK/*.c))
KOBJS+=$(patsubst %.c,%.o,$(wildcard minK/*/*.c))
KOBJS+=$(patsubst %.s,%.o,$(wildcard minK/*/*.s))

LIBCOBJS = $(patsubst %.c,%.o,$(wildcard minLIBS/libc/*.c))
LIBCOBJS += $(patsubst %.c,%.o,$(wildcard minLIBS/libc/*/*.c))

LIBDSO += $(patsubst %.c,%.o,$(wildcard minLIBS/libds/*.c))
LIBGFXO += $(patsubst %.c,%.o,$(wildcard minLIBS/libgfx/*.c))
LIBGUIO += $(patsubst %.c,%.o,$(wildcard minLIBS/libgui/*.c))
LIBCOMPRESSO += $(patsubst %.c,%.o,$(wildcard minLIBS/libcompress/*.c))

WMO = $(patsubst %.c,%.o,$(wildcard apps/wm/*.c))


RAMDISK = $(LIBC) $(LIB) $(BIN) $(KOBJS) $(APPS)

minK/%.o:minK/%.c
	$(CC) $(KCFLAGS) -c $< -o $@
minK/%.o:minK/%.s
	$(AS) --32 $< -o $@

cdrom/minKERNEL:$(KOBJS)
	$(LD) -T minK/linker.ld $(KOBJS) -o $@
	$(NM) -n --defined-only -g $@ > base/ksym.nm

minLIBS/%.o:minLIBS/%.c
	$(CC) $(CFLAGS) -c $< -o $@

minLIBS/libc/%.o:minLIBS/libc/%.c
	$(CC) $(CFLAGS) -c $< -o $@

base/usr/lib/libc.a:$(LIBCOBJS)
	$(AR) cr base/usr/lib/libc.a $(LIBCOBJS)

base/usr/lib/crt%.o:minLIBS/libc/crt%.s
	$(AS) --32 $< -o $@

base/usr/lib/libds.a:$(LIBDSO)
	$(AR) cr base/usr/lib/libds.a $(LIBDSO)
	
base/usr/lib/libgfx.a:$(LIBGFXO)
	$(AR) cr base/usr/lib/libgfx.a $(LIBGFXO)

base/usr/lib/libcompress.a:$(LIBCOMPRESSO)
	$(AR) cr base/usr/lib/libcompress.a $(LIBCOMPRESSO)

base/usr/lib/libgui.a:$(LIBGUIO)
	$(AR) cr base/usr/lib/libgui.a $(LIBGUIO)
base/usr/lib/libubsan.a:minLIBS/libubsan/ubsan.o
	$(AR) cr base/usr/lib/libubsan.a minLIBS/libubsan/ubsan.o


base/bin/init:bin/init.c
	$(CC) $(CFLAGS) $< -o $@ 
base/bin/hello:bin/hello.c
	$(CC) $(CFLAGS) $< -o $@
base/bin/about:apps/about.c
	$(CC) $(CFLAGS) $< -o $@ -lgui -lgfx -lds -lcompress
base/bin/calculator:apps/calculator.c
	$(CC) $(CFLAGS) $< -o $@ -lgui -lgfx -lds -lcompress
base/bin/files:apps/files.c
	$(CC) $(CFLAGS) $< -o $@ -lgui -lgfx -lds -lcompress
base/bin/taskbar:apps/taskbar.c
	$(CC) $(CFLAGS) $< -o $@ -lgui -lgfx -lds -lcompress
base/bin/menu:apps/menu.c
	$(CC) $(CFLAGS) $< -o $@ -lgui -lgfx -lds -lcompress


$(BASE)/bin/wm:$(WMO)
	$(CC) $(CFLAGS) $(WMO) -o $@ -lgui -lgfx -lds -lcompress

cdrom/ramdisk.img:$(RAMDISK) 
	genext2fs -d base -B 4096 -b 1280 cdrom/ramdisk.img

cdrom.iso:cdrom/minKERNEL cdrom/ramdisk.img
	grub-mkrescue -o $@ cdrom

run_kvm:cdrom.iso
	qemu-system-i386 -enable-kvm -serial stdio -m 128M -vga std -cdrom cdrom.iso -d guest_errors,cpu_reset

run:cdrom.iso
	qemu-system-i386 -serial stdio -m 128M -vga std -cdrom cdrom.iso -d guest_errors,cpu_reset

clean_kernel:
	rm -f cdrom/minKERNEL
	rm -f $(KOBJS)

clean_bin:
	rm -f $(WMO)
	rm -f $(APPS)
	rm -f $(BIN)

clean_lib:clean_bin
	rm -f $(LIBDSO)
	rm -f $(LIBGFXO)
	rm -f $(LIBCOMPRESSO)
	rm -f $(LIBGUIO)
	rm -f $(LIB)

clean_libc:clean_lib clean_bin
	rm -f $(LIBCOBJS)
	rm -f $(LIBC)


clean:clean_kernel clean_libc
	rm -f cdrom/ramdisk.img
	rm -f cdrom.iso
