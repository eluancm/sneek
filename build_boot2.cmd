@echo off
cd es
make clean
make
cd ..\fs
make clean
make
cd ..
if exist 0000000E (goto wo_app) else (goto w_app)

:wo_app
elfins\elfins fs\iosmodule.elf 0000000E 0000000E-tmp
goto esmodule

:w_app
elfins\elfins fs\iosmodule.elf 0000000e.app 0000000E-tmp
goto esmodule

:esmodule
elfins\elfins es\esmodule.elf 0000000E-tmp 0000000E-tm
boot2me\boot2me 0000000E-tm boot2.bin
del 0000000E-tmp
del 0000000E-tm
