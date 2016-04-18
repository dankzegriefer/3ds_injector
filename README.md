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

#### In order to use the locale patching (AKA langemu), you should place a file called "titleIDofGame.txt" in the "/injector/locales" folder, and its structure must be:

First 3 characters:
"JPN", "USA", "EUR", "AUS", "CHN", "KOR" or "TWN", depending on the region you want to emulate

4th character: A space (hex 0x20)

5th and 6th characters:

"JP", "EN", "FR", "DE", "IT", "ES", "ZH", "KO", "NL", "PT", "RU", "TW", depending on what language you want the game to boot up

For example, if you want to launch a EUR copy of Super Smash Bros. in Spanish you must create "/injector/locales/00040000000EE000.txt" with contents "EUR ES"

###### Please note, this isn't an automatic translator, the game must already include support for the selected language!

#### In order to use code section replacement, you must obtain a decrypted exefs.bin and use ctrtool to extract the decompressed code.bin

Place the code.bin inside "/injector/code" as "titleIDofGame.bin", and it should patch automatically on boot

###### Please note, this exefs.bin must be obtained from the latest update (if applicable)

#### In order to apply the N3DS clock speed patch you must create a file called "/injector/clock/titleIDofGame.cfg" and write:
###### Note: these are completely regular ASCII characters, not HEX values

"0" = Don't apply any patches
"1" = Apply overclocking patch
"2" = Apply overclocking patch + enable L2 cache
For most games, you'll want to set it to "2"

Values higher than "2" will make the patcher act like "2" was read.

For example, if you want to run USA Pokémon X with this patch, you must create a file called "/injector/clock/0004000000055D00.cfg" and only write "2" to it.
ALWAYS make sure the file length is 1 byte.

## This Repo
This repo takes the AuReiNand code for injector and the Cakes loader Makefile, recipe.yaml, and patissier.py in order to bring
AuReiNand's mods to Cakes.

It also removes some features like SecureInfo loading in order to keep it simple, if you want to use a different SecureInfo just use a {red, emu}NAND.

Adds decompressed code replacement (HANS-style), allowing you to load entire code sections on the fly, like Mystery Machine and SaltySD.


## Credits

yifanlu - Original open source loader module
AuroraWright - Some modifications and cleanup
TuxSH - N3DS clock and L2 patch, language emulation patch
Wolfvak - Code replacement patch
