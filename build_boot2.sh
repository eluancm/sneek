#!/bin/sh
make -C es clean all
make -C fs clean all
make -C fs-usb clean all
make -C di clean all
make -C mini-tree-mod clean all

echo Patching..

echo Patching for SNEEK
echo IOSKPatch: SD \(with di\) 
./IOSKpatch/IOSKPatch 0000000e.app 0000000E-TMP -s -d > /dev/null
echo elfins: Creating boot2_di.bin \(SDCard as NAND, with DI module support\)
./ELFIns/elfins 0000000E-TMP boot2_di.bin es/esmodule.elf fs/iosmodule.elf > /dev/null

echo IOSKPatch: SD \(no di\) 
./IOSKpatch/IOSKPatch 0000000e.app 0000000E-TMP -s > /dev/null
echo elfins: Creating boot2_sd.bin \(SDCard as NAND\)
./ELFIns/elfins 0000000E-TMP boot2_sd.bin es/esmodule.elf fs/iosmodule.elf > /dev/null

echo Patching for UNEEK
echo IOSKPatch: USB \(no di\)
./IOSKpatch/IOSKPatch 0000000e.app 0000000E-TMP -u > /dev/null
echo elfins: Creating boot2_usb.bin \(USB as NAND\)
./ELFIns/elfins 0000000E-TMP boot2_usb.bin es/esmodule.elf fs-usb/iosmodule.elf > /dev/null

echo elfins: Creating di.bin
./ELFIns/elfins 00000001 di.bin di/iosmodule.elf > /dev/null

rm 0000000E-TMP -v
