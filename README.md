USBImager
=========

<img src="https://gitlab.com/bztsrc/usbimager/raw/master/src/misc/icon32.png">
[USBImager](https://gitlab.com/bztsrc/usbimager) is a really really simple GUI application that writes disk images to USB drives.
Available platforms: Windows, MacOSX and Linux. Its interface is as simple as it gets, totally bloat-free.

Installation
------------

1. download one of the `usbimager-*.zip` archives above for your operating system
2. extract to: `C:\Program Files` (Windows), `/Applications` (MacOSX) or `/usr` (Linux)
3. Enjoy!

You can use the executable in the archive as-is, the other files only provide integration with your desktop (icons and such).

Features
--------

- Open Source and MIT licensed
- Portable executable, no installation needed, just extract the archives
- Small. Really small, few kilobytes only, yet has no dependencies
- No privacy concerns nor advertisements like with etch*r
- Minimalist, native interface on all platforms
- Tries to be bullet-proof and avoids overwriting of the system disk
- Can verify writing by comparing the disk to the image
- Can read raw disk images: .img, .bin, .raw, .iso, .dd, etc.
- Can read compressed images on-the-fly: .gz, .bz2, .xz
- Can read archives on-the-fly: .zip (PKZIP and ZIP64) (*)

(* - for archives with multiple files, the first file in the archive is used as input)

<img src="https://gitlab.com/bztsrc/usbimager/raw/master/usbimager.png">

Compilation
-----------

### Windows

Dependencies: just standard Win32 DLLs, and MinGW for compilation.

1. install [MinGW](https://osdn.net/projects/mingw/releases), this will give you "gcc" and "make" under Windows
2. in the src directory, run `make`
3. to create the archive, run `make package`

### MacOSX

Dependencies: just standard core frameworks, libui for Cocoa (included) and command line XCode tools.

1. in a Terminal, run `xcode-select --install` and in the pop-up window click "Install". This will give you "gcc" and "make" under MacOSX.
2. in the src directory, run `make`
3. to create the archive, run `make package`

### Linux

Dependencies: libc, libui for GTK+-3.0 (included) and GNU toolchain. Libui relies on hell lot of a gtk libraries though.

1. in the src directory, run `make`
2. to create the archive, run `make package`
3. to install, run `sudo make install`

Known Issues
------------

Needs lots of testing. MacOSX port seems to have issues with phtread? It would be nice to have a full native main_darwin.m in Obj-C, would solve
the ugly interface and threading problems.

Authors
-------

- libui: Pietro Gagliardi
- bzip2: Julian R. Seward
- xz: Igor Pavlov and Lasse Collin
- zlib: Mark Adler
- zip format: bzt (no PKWARE-related lib nor source was used in this project)
- usbimager: bzt

That all,

bzt
