XZ (LZMA)
=========

I've only modified the Makefile a bit to create a static library, and added an UINT64_C() in xz_crc64. Otherwise this the verbatim code from the Linux kernel.

Compilation
-----------

```
make libxz.a
```
