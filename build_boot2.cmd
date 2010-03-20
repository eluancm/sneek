@echo off
echo Building..
cd es
make clean
make
cd ..\fs
make clean
make
cd ..\fs-usb
make clean
make
cd ..\di
make clean
make
cd ..
echo Patching..
copy es\esmodule.elf .
copy fs\iosmodule.elf sdfsmodule.elf 
copy fs-usb\iosmodule.elf usbfsmodule.elf
copy di\iosmodule.elf dimodule.elf

echo Patching for SNEEK
echo IOSKPatch: SD (with di) 
IOSKpatch\IOSKPatch 0000000E 0000000E-TMP -s -d > NUL
echo elfins: Creating boot2_di.bin (SDCard as NAND, with DI module support)
ELFIns\elfins 0000000E-TMP boot2_di.bin esmodule.elf sdfsmodule.elf > NUL

echo IOSKPatch: SD (no di) 
IOSKpatch\IOSKPatch 0000000E 0000000E-TMP -s > NUL
echo elfins: Creating boot2_sd.bin (SDCard as NAND)
ELFIns\elfins 0000000E-TMP boot2_sd.bin esmodule.elf sdfsmodule.elf > NUL

echo Patching for UNEEK
echo IOSKPatch: USB (no di) 
IOSKpatch\IOSKPatch 0000000E 0000000E-TMPU -u > NUL
echo elfins: Creating boot2_usb.bin (USB as NAND)
ELFIns\elfins 0000000E-TMPU boot2_usb.bin esmodule.elf usbfsmodule.elf > NUL

echo elfins: Creating di.bin
ELFIns\elfins 00000001 di.bin dimodule.elf > NUL

del *.elf 0000000E-TMP 0000000E-TMPU