PREFIX	=	$(DEVKITARM)/bin/arm-eabi-
CC	=	$(PREFIX)gcc
AS	=	$(PREFIX)as
LD	=	$(PREFIX)gcc
SSTRIP	=	$(DEVKITARM)/bin/arm-eabi-strip

CFLAGS	=	-mbig-endian -fomit-frame-pointer -Os -Wall -I. -mcpu=arm926ej-s -mthumb -fno-builtin-memcpy
ASFLAGS	=	-mbig-endian -mcpu=arm926ej-s
LDFLAGS	=	-nostartfiles -nodefaultlibs -mbig-endian -Wl,-T,iosmodule.ld,-Map,iosmodule.map -n
LIBS	=	-lgcc

TARGET	=	iosmodule.elf
OBJECTS	=	start.o alloc.o diskio.o ff.o syscalls.o utils_asm.o gecko.o FS.o main.o vsprintf.o string.o tiny_ehci_glue.o usb_os.o

.PHONY: FORCE

all: $(TARGET)

$(TARGET) : iosmodule.ld $(OBJECTS)
	@echo  "LD	$@"
	@$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@echo @$(SSTRIP) $@

%.o : %.s
	@echo  "AS	$@"
	@$(CC) $(CFLAGS) -D_LANGUAGE_ASSEMBLY -c -x assembler-with-cpp -o $@ $<

%.o : %.S
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
