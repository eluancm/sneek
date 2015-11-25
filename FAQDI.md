![http://det1re.de/a/sneek/sneek_faqdi.png](http://det1re.de/a/sneek/sneek_faqdi.png)
# **Only use this FAQ if your normal SNEEK setup works, but when you try to use DI with it doesn't.** #
![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)




![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# I added/removed games but the list doesn't update #
To speed up the boot process an option got added which allows to skip the check for removed/added games.
  * You can either force a re-check in the settings menu via 'recreate game cache' or just set 'AutoUpdate Games' to On which will then check for a change on every boot!
![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# I just get a black screen #
  * Only clusters sizes 32K and lower are supported
  * DI will always use the first partition if multiple are found
  * Assure that you patched your 0000000E.app correctly with IOSKPatch i.e.:
> > "IOSKPatch.exe 0000000E.app 0000000E\_patched.app -s -d -v"
  * At least one game is required to boot
  * Games must be put into the usb:/games folder in this format:
```
  usb:/games/GameName/files/<extracted game files>

  usb:/games/GameName/sys/main.dol
  usb:/games/GameName/sys/apploader.img
  usb:/games/GameName/sys/fst.bin
  usb:/games/GameName/sys/boot.bin
  usb:/games/GameName/sys/bi2.bin

  usb:/games/GameName/ticket.bin
  usb:/games/GameName/cert.bin
  usb:/games/GameName/tmd.bin
```
![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# I can't open the SNEEK menu #
    * You must HOLD the ONE button on the first wiimote in the system menu to open the menu.
    * A file with a bitmaped font is required for the menu to work and is expect at sd:/sneek/font.bin (SNEEK) or usb:/sneek/font.bin (UNEEK).
      * You can find the font.bin at the Downloads
![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# I get "This disc can not be read" #
    * Assure that the selected region matches your system menu's region, you can change the region within the SNEEK menu (hold 1 to open the menu, press B to switch to the settings menu)
    * System menu 4.2 has improved region checking without a patch, to either the system menu or the game, foreign games won't boot (SNEEK includes patches for 4.2E and U)
![http://det1re.de/a/sneek/sneek_div.png](http://det1re.de/a/sneek/sneek_div.png)
# Game shows up in the game channel but when booted stays at a black screen / Channels boot to a black screen #
    * Assure that the IOS version the Game/Channel runs on is installed
    * Especially foreign channels can cause black screen because the SDK has a check in place which forbids a video mode change between certain modes and causes a fatal error(black screen).
      * Try to use the Game/Channel version of your own region
      * (Only works with Games!)Try to enable the video mode patch within the settings menu of SNEEK/UNEEK
      * Patch the video mode check yourself