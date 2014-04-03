PREFIX	=	$(DEVKITARM)/bin/arm-eabi-
CC	=	$(PREFIX)gcc
AS	=	$(PREFIX)as
LD	=	$(PREFIX)gcc
SSTRIP	=	$(DEVKITARM)/bin/arm-eabi-strip

CFLAGS	=	-mbig-endian -fomit-frame-pointer -O2 -Wall -I. -mcpu=arm926ej-s -fno-builtin-memcpy -fno-builtin-toupper
ASFLAGS	=	-mbig-endian -mcpu=arm926ej-s
LDFLAGS	=	-nostartfiles -nodefaultlibs -mbig-endian -Wl,-T,iosmodule.ld,-Map,iosmodule.map -n
LIBS	=	-lgcc

TARGET	=	iosmodule.elf
OBJECTS	=	start.o gecko.o utils_asm.o memory_asm.o common.o sdhc.o ff.o sdmmc.o diskio.o FS.o main.o syscalls.o vsprintf.o string.o

.PHONY: FORCE

all: $(TARGET)

$(TARGET) : iosmodule.ld $(OBJECTS)
	@$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@$(SSTRIP) $@

%.o : %.s
	@$(CC) $(CFLAGS) -D_LANGUAGE_ASSEMBLY -c -x assembler-with-cpp -o $@ $<

%.o : %.S
	@$(CC) $(CFLAGS) -D_LANGUAGE_ASSEMBLY -c -x assembler-with-cpp -o $@ $<

%.o : %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	@set -e; $(CC) -M $(CFLAGS) $< \
		| sed 's?\($*\)\.o[ :]*?\1.o $@ : ?g' > $@; \
		[ -s $@ ] || rm -f $@

%.d: %.S
	@touch $@

-include $(OBJECTS:.o=.d)

clean:
	-rm -f *.elf *.o *.bin *.d *.map
