@echo off
echo Building..
make -C es clean all
make -C fs clean all
make -C fs-usb clean all
make -C di clean all
make -C mini-tree-mod clean all

echo Patching..

echo Patching for SNEEK
echo IOSKPatch: SD (with di) 
IOSKpatch\IOSKPatch 0000000e.app 0000000E-TMP.app -s -d > NUL
echo elfins: Creating kernel_di.bin (SDCard as NAND, with DI module support)
ELFIns\elfins 0000000E-TMP.app kernel_di.bin es\esmodule.elf fs\iosmodule.elf > NUL

echo IOSKPatch: SD (no di) 
IOSKpatch\IOSKPatch 0000000e.app 0000000E-TMP.app -s > NUL
echo elfins: Creating kernel_sd.bin (SDCard as NAND)
ELFIns\elfins 0000000E-TMP.app kernel_sd.bin es\esmodule.elf fs\iosmodule.elf > NUL

echo Patching for UNEEK
echo IOSKPatch: USB (no di) 
IOSKpatch\IOSKPatch 0000000e.app 0000000E-TMP.app -u > NUL
echo elfins: Creating kernel_usb.bin (USB as NAND)
ELFIns\elfins 0000000E-TMP.app kernel_usb.bin es\esmodule.elf fs-usb\iosmodule.elf > NUL

echo eFIX: Creating di.bin
eFIX\efix.exe di\dimodule.elf > NUL

del 0000000E-TMP.app
