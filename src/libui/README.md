LibUI
=====

On MacOSX and Linux usbimager uses libui (but not on Windows, because I couldn't statically link with it, not even the provided examples).
This is a really great library, but its build system is a nightmare, a real pain in the ass. So I've decided to ship it as compiled static
library.

Compilation
-----------

If you want to recompile libui (you'll need cmake, meson, ninja and other non-standard toolchains), visit:

[https://github.com/andlabs/libui/](https://github.com/andlabs/libui/)
