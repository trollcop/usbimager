ZStandard
=========

This is a stripped down version of [zstd](https://github.com/facebook/zstd), which only contains the decompressor but nothing else.

Compilation
-----------

```
make libzstd.a ZSTD_LIB_COMPRESSION=0 ZSTD_LIB_DICTBUILDER=0 ZSTD_LIB_DEPRECATED=0
```
