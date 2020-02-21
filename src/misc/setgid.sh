#!/bin/sh

if [ "$1" == "" -o "$2" == "" ]; then echo "Don't call directly."; exit; fi
if [ "`groups | grep disk`" != "" -o "`whoami`" = "root" ]; then
    chgrp $1 $2
    chmod g+s $2
else
    echo
    echo "WARNING - Your user is not member of the 'disk' group, can't grant"
    echo "access to USBImager. Run the following two commands manually:"
    echo
    echo "  sudo chgrp $1 $2"
    echo "  sudo chmod g+s $2"
    echo
fi
