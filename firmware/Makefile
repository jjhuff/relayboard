# vim: noet

# MCU name
MCU = atmega328p

# Processor frequency.
#     This will define a symbol, F_CPU, in all source code files equal to the 
#     processor frequency. You can then use this symbol in your source code to 
#     calculate timings. Do NOT tack on a 'UL' at the end, this will be done
#     automatically to create a 32-bit value in your source code.
F_CPU = 12500000

# Input clock frequency.
#     This will define a symbol, F_CLOCK, in all source code files equal to the 
#     input clock frequency (before any prescaling is performed). This value may
#     differ from F_CPU if prescaling is used on the latter, and is required as the
#     raw input clock is fed directly to the PLL sections of the AVR for high speed
#     clock generation for the USB and other AVR subsections. Do NOT tack on a 'UL'
#     at the end, this will be done automatically to create a 32-bit value in your
#     source code.
#
#     If no clock division is performed on the input clock inside the AVR (via the
#     CPU clock adjust registers or the clock division fuses), this will be equal to F_CPU.
F_CLOCK = 12500000

## General Flags
PROJECT = relay-board
TARGET = $(PROJECT).hex
ELF = $(PROJECT).elf
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRDUDE = avrdude
SIZE = avr-size
AVRDUDE_ARGS = -c buspirate -P /dev/bus_pirate -p $(MCU)

## optimization flags 
OPTIMIZATION = -Os

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -DF_CLOCK=$(F_CLOCK)UL

## Compiler flags to generate dependency files
GENDEPFLAGS = -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Compile options common for all C compilation units.
CFLAGS = $(COMMON) $(GENDEPFLAGS)
CFLAGS += -Inet -Inet_app -I.
CFLAGS += -Wall -gdwarf-2 -std=gnu99 $(OPTIMIZATION) -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -mcall-prologues
CFLAGS += $(GENDEPFLAGS)


#---------------- Library Options ----------------
# Minimalistic printf version
PRINTF_LIB_MIN = -Wl,-u,vfprintf -lprintf_min

# Floating point printf version (requires MATH_LIB = -lm below)
PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt

# If this is left blank, then it will use the Standard printf version.
#PRINTF_LIB = 
PRINTF_LIB = $(PRINTF_LIB_MIN)
#PRINTF_LIB = $(PRINTF_LIB_FLOAT)


# Minimalistic scanf version
SCANF_LIB_MIN = -Wl,-u,vfscanf -lscanf_min

# Floating point + %[ scanf version (requires MATH_LIB = -lm below)
SCANF_LIB_FLOAT = -Wl,-u,vfscanf -lscanf_flt

# If this is left blank, then it will use the Standard scanf version.
#SCANF_LIB = 
SCANF_LIB = $(SCANF_LIB_MIN)
#SCANF_LIB = $(SCANF_LIB_FLOAT)

#MATH_LIB = -lm
MATH_LIB =

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=$(PROJECT).map
LDFLAGS += -Wl,--relax 
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(PRINTF_LIB) $(SCANF_LIB) $(MATH_LIB)

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES =  

## Sources network
SRC =   net/uip_arp.c \
        net/enc28j60.c \
        net/nic.c \
        net/uip.c \
        net/psock.c \
        net/timer.c \
        net/uip-split.c \
        net/uip-empty-packet.c 

## DHCP client
SRC +=  net_app/dhcpc.c 

## sofisticated webserver
SRC +=  webserver/httpd.c \
        webserver/http-strings.c \
        webserver/httpd-fs.c \
        webserver/httpd-cgi.c

## main function
SRC +=  main.c 

## Objects that must be built in order to link
COBJ      = $(SRC:.c=.o)  
AOBJ      = $(ASRC:.S=.o)

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lst size

## Compile
$(COBJ) : %.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $< -o $@ 

## Compile
$(AOBJ) : %.o : %.S
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $< -o $@ 


##Link
$(ELF): $(COBJ) $(AOBJ)
	$(CC) $(LDFLAGS) $(COBJ) $(AOBJ) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(ELF)

## Burning
burn: $(PROJECT).hex
	$(AVRDUDE) $(AVRDUDE_ARGS) -U flash:w:$^

burn-eeprom: $(PROJECT).eep
	$(AVRDUDE) $(AVRDUDE_ARGS) -U eeprom:w:$^

burn-fuse:
	$(AVRDUDE) $(AVRDUDE_ARGS) -U lfuse:w:0xc0:m -U hfuse:w:0xd6:m



%.hex: $(ELF)
	 $(OBJCOPY)  -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(ELF)
	 $(OBJCOPY)  $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lst: $(ELF)
	$(OBJDUMP) -h -S $< > $@


# Display size of file.
HEXSIZE = $(SIZE) --target=$(FORMAT) $(TARGET)
ELFSIZE = $(SIZE) $(MCU_FLAG) $(FORMAT_FLAG) $(ELF)
MCU_FLAG = $(shell $(SIZE) --help | grep -- --mcu > /dev/null && echo --mcu=$(MCU) )
FORMAT_FLAG = $(shell $(SIZE) --help | grep -- --format=.*avr > /dev/null && echo --format=avr )

size: ${ELF}
	@if test -f $(TARGET); then echo; $(ELFSIZE); \
	2>/dev/null; echo; fi

## Clean target
.PHONY: clean
clean:
	-rm -rf $(COBJ) dep/* $(PROJECT).elf $(PROJECT).hex $(PROJECT).eep $(PROJECT).lst $(PROJECT).map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

# web server dependency
webserver/httpd-fs.c: webserver/httpd-fsdata.c
	touch webserver/httpd-fs.c 
