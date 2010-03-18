@echo off
cd es
make clean
make
cd ..\fs
make clean
make
cd ..\fs-usb
make clean
make
cd ..
elfins\elfins fs\iosmodule.elf 0000000E 0000000E-tmp -s
elfins\elfins es\esmodule.elf 0000000E-tmp boot2_sd.bin
elfins\elfins fs-usb\iosmodule.elf 0000000E 0000000E-tmp -u
elfins\elfins es\esmodule.elf 0000000E-tmp boot2_usb.bin
del 0000000E-tmp
