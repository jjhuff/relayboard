# vim: noet

PRG	= relayboard
OBJ	=  simple-httpd.o clock-arch.o main.o enc28j60.o
OBJ += uip/uiplib.o uip/psock.o uip/uip_arp.o uip/timer.o uip/uip.o

MCU_TARGET	= atmega328p
AVRDUDE_ARGS = -c buspirate -P /dev/bus_pirate
OPTIMIZE	= -Os

DEFS	=
LIBS	=

CC		= avr-gcc
CFLAGS	= -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -fpack-struct -funsigned-char -funsigned-bitfields -mcall-prologues -Wstrict-prototypes -std=gnu99 -fno-strict-aliasing -I. $(DEFS)
LDFLAGS	= -Wl,-Map,$(PRG).map

OBJCOPY	= avr-objcopy
OBJDUMP	= avr-objdump

all: $(PRG).elf lst text eeprom

burn: $(PRG).hex
	avrdude $(AVRDUDE_ARGS) -p $(MCU_TARGET) -v -e -U flash:w:$^

burn-fuse:
	avrdude $(AVRDUDE_ARGS) -p $(MCU_TARGET) -v -U lfuse:w:0xc0:m -U hfuse:w:0xd6:m

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf $(OBJ) $(PRG).elf $(PRG).hex $(PRG).bin $(PRG)_eeprom.hex $(PRG)_eeprom.bin
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin

hex:  $(PRG).hex
bin:  $(PRG).bin

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@ \
	|| { echo empty $@ not generated; exit 0; }

