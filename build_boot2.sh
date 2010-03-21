#!/bin/sh
make -C es clean all
make -C fs clean all
make -C fs-usb clean all
make -C di clean all
make -C mini-tree-mod clean all

echo Patching..
cp es/esmodule.elf . -v
cp fs/iosmodule.elf sdfsmodule.elf -v
cp fs-usb/iosmodule.elf usbfsmodule.elf -v
cp di/iosmodule.elf dimodule.elf -v

echo Patching for SNEEK
echo IOSKPatch: SD \(with di\) 
./IOSKpatch/IOSKPatch 0000000E 0000000E-TMP -s -d > /dev/null
echo elfins: Creating boot2_di.bin \(SDCard as NAND, with DI module support\)
./ELFIns/elfins 0000000E-TMP boot2_di.bin esmodule.elf sdfsmodule.elf > /dev/null

echo IOSKPatch: SD \(no di\) 
./IOSKpatch/IOSKPatch 0000000E 0000000E-TMP -s > /dev/null
echo elfins: Creating boot2_sd.bin \(SDCard as NAND\)
./ELFIns/elfins 0000000E-TMP boot2_sd.bin esmodule.elf sdfsmodule.elf > /dev/null

echo Patching for UNEEK
echo IOSKPatch: USB \(no di\)
./IOSKpatch/IOSKPatch 0000000E 0000000E-TMPU -u > /dev/null
echo elfins: Creating boot2_usb.bin \(USB as NAND\)
./ELFIns/elfins 0000000E-TMPU boot2_usb.bin esmodule.elf usbfsmodule.elf > /dev/null

echo elfins: Creating di.bin
./ELFIns/elfins 00000001 di.bin dimodule.elf > /dev/null

rm *.elf 0000000E-TMP 0000000E-TMPU -v
