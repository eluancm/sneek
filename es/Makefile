PREFIX	=	$(DEVKITARM)/bin/arm-eabi-
CC	=	$(PREFIX)gcc
AS	=	$(PREFIX)as
LD	=	$(PREFIX)gcc
SSTRIP	=	$(DEVKITARM)/bin/arm-eabi-strip

CFLAGS	=	-mbig-endian -fomit-frame-pointer -O2 -Wall -I. -mcpu=arm926ej-s 
ASFLAGS	=	-mbig-endian -mcpu=arm926ej-s
LDFLAGS	=	-nostartfiles -nodefaultlibs -mbig-endian -Wl,-T,esmodule.ld,-Map,esmodule.map -n
LIBS	=	-lgcc

TARGET	=	esmodule.elf
OBJECTS	=	start.o common.o alloc.o gecko.o DI.o FS.o ES.o NAND.o main.o syscalls.o vsprintf.o string.o

.PHONY: FORCE

$(TARGET) : esmodule.ld $(OBJECTS)
	@echo  "LD	$@"
	@$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@$(SSTRIP) $@

%.o : %.s
	@echo  "AS	$@"
	@$(CC) $(CFLAGS) -D_LANGUAGE_ASSEMBLY -c -x assembler-with-cpp -o $@ $<

%.o : %.c
	@echo  "CC	$@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	@echo  "DEP	$@"
	@set -e; $(CC) -M $(CFLAGS) $< \
		| sed 's?\($*\)\.o[ :]*?\1.o $@ : ?g' > $@; \
		[ -s $@ ] || rm -f $@

%.d: %.S
	@echo	"DEP	$@"
	@touch $@

-include $(OBJECTS:.o=.d)

clean:
	-rm -f *.elf *.o *.bin *.d *.map
