![http://det1re.de/a/sneek/sneek_faq.png](http://det1re.de/a/sneek/sneek_faq.png)


![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# My drive just blinks #
When your drive blinks in a pattern something went wrong, here is a list of what each pattern means:
  * 1:The TMD of the requested title is missing
  * 2:The TMD of the IOS the current title uses is missing
    * Can be cause by incorrect cluster size ( only 32KB and less are supported)
  * 3:The kernel.bin is not found on sd and or usb
  * 4:Failed to load the requested IOS (missing IOS, missing TMD/Ticket/content)
  * 5:Could not open or read sd:/sneek/kernel.bin

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# I just get a black screen #
  * Assure that you put the NAND FS dump into the root of the SD card(SNEEK) or USB device(UNEEK).
  * Assure that the kernel.bin is in the correct folders:
    * SNEEK: sd:/sneek/kernel.bin
    * UNEEK: sd/sneek/kernel.bin AND usb/sneek/kernel.bin
  * Some SD cards/usb devices are not supported try another one.
  * If you want to use SNEEK/UNEEK without DI assure that there is no di.bin in the sneek folder, as SNEEK/UNEEK will load it when found, and if you didn't apply the patch to use DI this will cause a freeze.
  * The FAT lib used doesn't seem to work with 64K cluster size, assure that you use a smaller size
  * SNEEK/UNEEK uses USB Gecko for debug output, if you have anything in slot B it might keep SNEEK/UNEEK from booting, also when an USB Gecko is inserted SNEEK/UNEEK will hold until you read the output.
  * The bootmii folder should have this file:
    1. armboot.bin (compiled from source or direct download)
  * Only full FS dumps work with SNEEK/UNEEK, some dumpers skip IOSs or data folders. The system menu won't load without a setting.txt present.
  * SNEEK only works with modular IOSs (>28) if the system tries to load any IOS <= 28 it will fall back to IOS 35, so assure IOS 35 is installed.
  * Preloader/Priiloader CAN cause problems with SNEEK/UNEEK, you can easily remove it on the virutal NAND.
    1. goto \title\00000001\00000002\content\
    1. there should be a file called 100000xx.app and a file called 000000xx.app, delete the 000000xx.app file
    1. rename the 100000xx.app to 000000xx.app, that's it

If all the above fail or a previously working NAND stops working you can try the following:
  * Delete "\title\00000001\00000002\data\cdb.vff" — _**Note:** the recreating of the file can take up to 5 mins or even longer._
  * delete everything in the "\title\00000001\00000002\data\" folder EXCEPT the setting.txt! — _**Note:** again recreating of the files can take up to 5 mins or even longer._
  * delete the shared2 folder — _**Note:** if you delete the shared2 folder you have to setup your whole Wii again!_

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# cIOS doesn't work #

Use cIOSX rev18 or higher.

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Stuff loads slow #

The speed issue is quite tricky, one rule of thumb is the less is on the card the faster it is.

There is on way to gain some speed, reformat your SD card with the highest sector per cluster count possible but no more than 64 sectors per cluster (32kB Clusters).

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# SD cards do not work for Wii applications (Only when using SNEEK) #

SD support for Wii applications is currently disabled, due it not working very well.

If you still want to give it a try search for this string in the main.c of the ES module
```
*SDStatus = 0x00000002;
```
and change it to
```
*SDStatus = 0x00000001;
```
Note: this feature is buggy and might break the FS on your SD card, make a backup before using thise feature! Reading from SD has been told to work, but writing should be an issue.