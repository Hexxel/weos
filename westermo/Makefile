BOOT-VERSION := 1.0.2

# Called from barebox.bin build target?
ifeq ($(CONFIG_MACH_DAGGER),y)
lwl-y += boot-core.o
lwl-y += boot-start.o
else
.PHONY: all clean test

# Address at which the pbl is linked. It is relocatable anyway, but
# simplifies debugging because you can do a simple "load" from gdb to

# load it to the SRAM in the PONCat.
LOAD-ADDR := 0x40000000

# Test setup
# Uses conserver(8) consoles, one connected to a BDI3k and one to the
# target to boot the target in UART mode using the specified image.
BDI-CONSOLE ?= dagger_bdi
CON-CONSOLE ?= ser1
TESTIMAGE  ?= dagger-pbl-uart0.bin

bdi = ($(1)) | socat -U exec:"console -f $(BDI-CONSOLE)",crnl stdin >/dev/null
con = ($(1)) | socat -U exec:"console -f $(CON-CONSOLE)",crnl stdin >/dev/null

# Setup toolchain
CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
CFLAGS  := -g -O3 -fPIC -nostdinc -nostdlib -Wall -Wextra -Werror
ASFLAGS  := $(CFLAGS)

# Rebuild if header script or any of the files it references changes
hdr-deps := MVHeaderfile MVHeaderfile.uart0 $(shell ./mkhdr.py -D)
pbl-imgs := dagger-pbl-spi.bin

all: boot-version.h $(pbl-imgs) boot.bin

clean:
	rm -f *.bin *.o

test: $(TESTIMAGE)
	@echo resetting system
	@$(call bdi,echo "reset")
	@sleep 0.5
	@echo sending uart boot magic
	@$(call con,printf '\273\021\042\063\104\125\146\167%.0s' $(shell seq 100)) &
	@$(call bdi,echo "go")
	@sleep 0.5
	@echo sending $<
	@$(call con,printf '\005c|'; sleep 0.5; echo "sx $<"; sleep 1; while pidof sx; do sleep 1; done)

dagger-pbl-%.bin: $(hdr-deps)
	./mkhdr.py -B $* -o $@
	$(Q)cp -f $@ ..

dagger-pbl-uart0.bin: $(hdr-deps)
	./mkhdr.py -f MVHeaderfile.uart0 -B uart0 -o $@
	$(Q)cp -f $@ ..

boot-version.h: Makefile
	(echo \#define BOOT_VERSION \"$(BOOT-VERSION)\" > boot-version.h);

# boot.bin - copies 2nd stage from spi flash to ram
boot.bin: boot.o
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -I ../include -march=armv7-a -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -I .. -march=armv7-a -c -o $@ $<

boot.o: CPPFLAGS  := -march=armv7-a
boot.o: boot-start.o boot-core.o
	$(CC) -nostdlib -Wl,-Tboot.lds $^ -o $@

boot-start.o: drg-spi.bin

# drg-spi.bin - dragonite spi firmware
drg-spi.bin: drg-spi.o
	$(OBJCOPY) -O binary $< $@

%.o: %.armv5.c
	$(CC) $(CFLAGS) -march=armv5 $^ -c -o $@
    
%.o: %.armv5.S
	$(CC) $(ASFLAGS) -march=armv5 $^ -c -o $@
    
drg-spi.o: CPPFLAGS  := -march=armv5
drg-spi.o: drg-spi-start.o drg-spi-core.o
	$(CC) -nostdlib -Wl,-Tboot.lds $^ -o $@
endif
