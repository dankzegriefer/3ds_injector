3DS Loader Replacement
======================

This is an open source implementation of 3DS `loader` system module--with 
additional features. The current aim of the project is to provide a nice 
entry point for patching 3DS modules.


## Roadmap
Right now, this can serve as an open-source replacement for the built in loader. 
There is additional support for patching any executable after it's loaded but 
before it starts. For example, you can patch `menu` to skip region checks and 
have region free game launching directly from the home menu. There is also 
support for SDMC reading (not found in original loader implementation) which 
means that patches can be loaded from the SD card. Ultimately, there would be 
a patch system that supports easy loading of patches from the SD card.


## Build
You need a working 3DS build environment with a fairly recent copy of devkitARM, 
ctrulib, and makerom. If you see any errors in the build process, it's likely 
that you're using an older version.

You also need Python 3.x + PyYAML.

## How to use

###### In order to use the locale patching (aka language emulation), you should place a file called "titleid_of_game.txt" in the "/injector/locales" folder, and its structure must be:

Characters 0-3:
"JPN", "USA", "EUR", "AUS", "CHN", "KOR" or "TWN", depending on the region you want to simulate

Character 4:
A regular space (hex 0x20)

Characters 5-6:
"JP", "EN", "FR", "DE", "IT", "ES", "ZH", "KO", "NL", "PT", "RU", "TW", depending on the language you want to simulate

For example, if you want to launch a EUR copy of Super Smash Bros in Spanish you must create "/injector/locales/00040000000EE000.txt" with contents "EUR ES"

###### In order to use code section replacement, you must obtain a decrypted exefs.bin and use ctrtool to extract the decompressed code.bin

Place the code.bin inside "/injector/code" as "titleid_of_game.bin", and it should patch automatically on boot

###### To activate N3DS overclocking, create a file called "/injector/clock.txt" and store either "1" or "2" as plain ASCII text.

No file / "0" = no overclocking
"1" = 804MHz
"2" = 804MHz + L2 cache enabled


## This Repo
This repo takes some of the Luma code for injector and the Cakes loader Makefile, recipe.yaml, and patissier.py in order to bring
Luma mods to Cakes.

Adds decompressed code replacement (HANS-style), allowing you to load entire code sections on the fly, like Mystery Machine and SaltySD.


## Credits

yifanlu - Original open source loader module replacement

mid-kid - patissier.py and Makefile (besides CakesFW, which is an awesome project)

AuroraWright - Some modifications and cleanup

TuxSH - Overclocking/L2 patch, language emulation patch

