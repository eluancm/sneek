@echo off
cd es
make clean
make
cd ..\fs
make clean
make
cd ..
elfins\elfins fs\iosmodule.elf 0000000E 0000000E-tmp
elfins\elfins es\esmodule.elf 0000000E-tmp 0000000E-tm
boot2me\boot2me 0000000E-tm boot2.bin
del 0000000E-tmp
del 0000000E-tm
