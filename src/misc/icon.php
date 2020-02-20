#!/usr/bin/php
<?php
/*
 * usbimager/misc/icon.php
 *
 * Copyright (C) 2020 bzt (bztsrc@gitlab)
 *
 * @brief small tool to generate wm_icon.h
 */

$f=fopen("wm_icon.h","w");
fprintf($f,"long wm_icon[] = {");
foreach([16,32] as $size){
    system("convert icon".$size.".png icon.rgba");
    $data=file_get_contents("icon.rgba");
    unlink("icon.rgba");
    fprintf($f,"\n\t%d, %d,\n",$size,$size);
    for($i=0;$i<strlen($data);$i+=4) {
        $p = unpack("V",substr($data,$i,4))[1];
        $p = (0xFF00FF00 & $p) | (($p >> 16) & 0xFF) | (($p & 0xFF) << 16);
        fprintf($f,"0x%x,",$p);
    }
}
fprintf($f,"\n\t0, 0\n};\n\n");
fclose($f);
