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

Currently, there is no support for FIRM building, so you need to do some steps 
manually. First, you have to add padding to make sure the NCCH is of the right 
size to drop in as a replacement. A hacky way is 
[this patch](http://pastebin.com/nyKXLnNh) which adds junk data. Play around 
with the size value to get the NCCH to be the exact same size as the one 
found in your decrypted FIRM dump.

## This Repo
This repo takes the AuReiNand code for injector and the Cakes loader Makefile, Recipe.yaml, and patissier.py to bring
AuReiNand's mods to Cakes. It also takes some code commited by Wolfvak.

It also removes some features like SecureInfo loading in order to keep it simple, if you want to use a different SecureInfo just use a {red, emu}NAND.
Adds decompressed code section replacement (HANS-style) to this loader, allowing you to load code patches on the fly, like Mystery Machine (6th gen Pokemon games) and SaltySD (SSB RomFS redirection patch).
NOT WORKING YET FOR REBOOT TITLES ON O3DS!

AuroraWright and mid-kid did all of the hard code work, all I did was copy+paste, I didn't make this a pull request to mid's repo because mid-kid wants to make a stable CakesFW release.


DUE TO A GLITCH IN OVERCLOCKING RELEASE 2.0 HAS BEEN REMOVED.

