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
cd ..\di
make
make clean
cd ..
elfins\elfins fs\iosmodule.elf 0000000E 0000000E-tmp -s
elfins\elfins es\esmodule.elf 0000000E-tmp boot2_sd.bin
elfins\elfins fs-usb\iosmodule.elf 0000000E 0000000E-tmp -u
elfins\elfins es\esmodule.elf 0000000E-tmp boot2_usb.bin
elfins\elfins fs-usb\iosmodule.elf 00000001 di.bin -d
del 0000000E-tmp
