GCC_DEVICE  = attiny85
DEVICE      = ATtiny85
CLOCK       = 8000000UL
PROGRAMMER  = buspirate
PORT        = /dev/ttyUSB0
FUSES       = -F -U lfuse:w:0xE2:m -U hfuse:w:0x5F:m -U efuse:w:0xFF:m
 
SOURCES=main.c softuart.c

CFLAGS=-I. -g -mmcu=$(GCC_DEVICE) -Os -DF_CPU=$(CLOCK) \
-D__AVR_$(DEVICE)__ \
-fpack-struct -fshort-enums \
-funsigned-bitfields -funsigned-char \
-Wall -Wstrict-prototypes \
-Wa,-ahlms=$(firstword $(filter %.lst, $(<:.c=.lst)))
 
ASMFLAGS =-I. -mmcu=$(GCC_DEVICE) -DF_CPU=$(CLOCK) \
-x assembler-with-cpp \
-Wa,-gstabs,-ahlms=$(firstword $(<:.S=.lst))
 
LDFLAGS=-Wl,-Map,main.out.map -mmcu=$(GCC_DEVICE)
 
CFILES=$(filter %.c, $(SOURCES))
ASMFILES=$(filter %.S, $(SOURCES))
 
OBJDEPS=$(CFILES:.c=.o) \
$(ASMFILES:.S=.o)
M4DEPS=$(ASMFILES:.S=_m4.S)
 
LST=$(filter %.lst, $(OBJDEPS:.o=.lst))
 
.SUFFIXES : .c .o .out .S .hex .ee.hex .h
 
.PHONY: writeflash clean stats gdbinit stats fuse
 
 
#############################
# Make targets:
# all, disasm, stats, hex, writeflash/install, clean, fuse
 
all: main.out
 
disasm: main.out
	avr-objdump -d main.out
 
stats: main.out
	avr-objdump -h main.out
	avr-size main.out 
 
hex: main.hex main.ee.hex
 
writeflash: hex
	avrdude -c $(PROGRAMMER) -p $(DEVICE) -P $(PORT) -U flash:w:main.hex:i
 
install: writeflash
 
clean:
	rm -f main.out main.out.map
	rm -f $(OBJDEPS)
	rm -f $(M4DEPS)
	rm -f $(LST)
	rm -f main.hex main.ee.hex main.elf
 
fuse:
	avrdude -c $(PROGRAMMER) -p $(DEVICE) -P $(PORT) $(FUSES)
    
 
main.out: $(OBJDEPS) $(M4DEPS)
	avr-gcc $(LDFLAGS) -o main.out $(OBJDEPS) $(M4DEPS)
 
 
#############################
# M4 Preprocessing
 
%_m4.S: %.S
	m4 < $< > $@
 
 
#############################
# Generating object files
 
.S.o : 
	avr-gcc $(ASMFLAGS) -c $< -o $@
 
.c.o : 
	avr-gcc $(CFLAGS) -c $< -o $@
 
#############################
# Generating hex files
 
# hex files from elf
.out.hex:
	avr-objcopy -j .text -j .data -O ihex $< $@
 
.out.ee.hex:
	avr-objcopy -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

