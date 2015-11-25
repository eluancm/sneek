![http://det1re.de/a/sneek/sneek_startupguide.png](http://det1re.de/a/sneek/sneek_startupguide.png)


![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Requirements #

  * [devkitPro](http://sourceforge.net/project/showfiles.php?group_id=114505&package_id=160396)
  * devkitARM (included in devkitPro, but most people didn't install it)
  * a SVN-Client ([sliksvn](http://www.sliksvn.com), [tortoisseSVN](http://tortoisesvn.tigris.org), or other)
  * [Python interpreter](http://www.python.org/download/) (2.6.x)

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Introduction #

Setting up the requirements above is not handled here. You'll find guides for these online.
As of 13.03. or any SVN revision >= [r27](https://code.google.com/p/sneek/source/detail?r=27), there is a new DI module. For more information see the [dimodule](dimodule.md) wiki page..

| **WARNING**: It has been reported multiple times, that never revisions of SNEEK won't work with devkitARM release 26 and above. For this reason install [devkitARM release 24](http://dl.qj.net/download/devkitarm-r24-windows.html). |
|:--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Creating a NAND filesystem dump #
There are two main ways to get your NAND filesystem dumped. The first one is using crediar's FSDumper _(other Filesystem Dumpers should work, too)_ and the second features a `BootMii` nand.bin, which get's unpacked by a tool. This way should be prefered if there is already a `BootMii` backup, that is at a considerable state.

## Using FSDumper ##

**1.** Download our very own [FSDumper](http://sneek.googlecode.com/files/FSDumper.zip).

**2.** Start it using your prefered method, e.g. via Homebrew Channel or Bannerbomb.

**3.** Press **(1)** on your WiiMote to start the dumping process. FSDumper will automatically place all files in the correct spot on your SD card. As a result you would get this folders on your SD's root:
  * import
  * meta
  * shared1
  * shared2
  * sys
  * ticket
  * title
  * tmp

**4.** Place these folders on the SD card you want to use with SNEEK.

## Using a BootMii nand.bin ##

**1.** Get your nand.bin file ready and download & unpack [NAND Extractor](http://www.gbatemp.net/index.php?showtopic=179630). Note, that it requires .Net Framework 3.5

**2.** In NAND Extractor choose "File" » "Open", choose your nand.bin and click "OK".

**3.** You should now see the file-/folderstructure. Click "File" » "Extract All" and let it unpack the dump to the root of your SD card. After unpacking is done check the integrity of the dump with the above list.

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Downloading the source #

To download the source you need an Subversion (short: svn) client. With it you can download the complete source of SNEEK (and other code.google projects) in one run.

In the following I will shortly explain how to download the source using sliksvn:

**1.** Open a command prompt and navigate to the directory where you want the source to be placed. I'm choosing my Wii projects folder here:
```
C:\>k:

K:\>cd coding\c\wii

K:\Coding\C\wii>
```

**2.** Now simply enter the the command presented at the "Source Checkout" page:

> ![http://det1re.de/a/sneek/sneek_source.png](http://det1re.de/a/sneek/sneek_source.png)

**3.** You can also modify the command via altering the subfolder for SNEEK from the default `"sneek-read-only"` to `"sneek"` or anything. However sliksvn will then download the latest source files. You can now go on compiling the files.

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Compiling #

**1.** After you downloaded the source code, get [this archive](http://det1re.de/a/sneek/~sneek-batch.zip).

**2.** Extract the included file to a directory of your choice (note, that it is not necessary that it is in the SNEEK directory)

**3.** start SNEEK.cmd _from a cmd prompt **(!)**_. If you start it by double clicking it could happen that your installed devkitPro is not found. If you have a USB Gecko and want debug output you can run the batch with the switch "-v" appended, to turn verbose mode on:
```
C:\>SNEEK.cmd        // » w/o debug output in SNEEK
C:\>SNEEK.cmd -v     // » w/  debug output in SNEEK
```

**4.** it will ask you to insert the complete path to the dir, where you downloaded the SNEEK source. In my case, I have to input _(always without trailing slash)_
```
Please insert the complete path to the SNEEK source code:
   - k:\coding\c\wii\sneek
```

**5.** The batch will ask you for your Python installation directory is installed. Insert the same way as with SNEEK
```
Where is Python installed (i.e. "C:\python26"):
   - c:\python26
```

**6.** The batch will now run the compile on all modules (don't worry too much about warnings). After this an Explorer will pop up and the batch will ask about IOS70-v6687 **0000000e.app** and IOS60-v6174 **00000001.app**. Get it using [NUSD](http://wiibrew.org/wiki/NUS_Downloader) and place it in the directory the Explorer shows. A how to on NUSD is found near the end of this [article](StartupGuide#Using_NUSD_to_acquire_IOS_modules.md). Place both files in the Explorer window.

**7.** Return to the batch and hit ENTER to let it finish.

**8.** At the end you should see an output comparable to this. If all files could be copied, everything went good:
```
Creating finalized files
   [...]
   - creating SNEEK preset
        1 file(s) copied.
        1 file(s) copied.
        1 file(s) copied.
   - creating UNEEK preset
        1 file(s) copied.
        1 file(s) copied.
        1 file(s) copied.
```

**9.**Another Explorer will open with the directory that has both versions in sepeare directories (`SNEEK` and `UNEEK`). In there you find directories for SD and USB root. The perfect run (compiling messages where removed from this screenshot) should look like this:

> ![http://det1re.de/a/sneek/sneek_perfect_run.png](http://det1re.de/a/sneek/sneek_perfect_run.png)

**10.** Also for SNEEK you'll have to add your NAND Filesystem dump to the SD; for UNEEK to USB. Congratulations, you successfully compiled SNEEK. I should mention, that the di.bin is not used, unless you set it up correctly using the DI module guide.

![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Using NUSD to acquire IOS modules #

**1.** Download [NUS Downloader](http://wiibrew.org/wiki/NUS_Downloader).

**2.** Get the key.bin file (also known as common-key.bin) and place it in the same directory as NUSD. You can find that file on the Internet or use a tool named `MakeKeyBin.exe`, also found on the Internet.

**3.** Click "Database" and choose "IOS" » "000000010000003C - IOS60" » "v6174".

> ![http://det1re.de/a/sneek/sneek_nusd.png](http://det1re.de/a/sneek/sneek_nusd.png)

**4.** Check the "Decrypt" option and "Start NUS Download"! NUSD will now download the modules of IOS60-v6174.

**5.** In the same folder as nusd.exe there is an dir called "000000010000003Cv6174". Open it and you will find your "**00000001.app**". That's the file you want. Also, go sure you are copying the .app file. There is an "00000001", too — but this one is not decrypted and thus can't be used for SNEEK. This module will be used for the DI.

**6.** Repeat the steps 3-5 with "IOS" » "0000000100000046 - IOS70" » "v6687". Here you'll have to copy the "**0000000e.app**".