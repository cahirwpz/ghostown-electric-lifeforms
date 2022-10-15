#!/bin/sh

echo $1.exe > startup-sequence
xdftool $1_bootable.adf format dos + write $1.exe + makedir s + write startup-sequence s
dd if=$2 of=$1_bootable.adf conv=notrunc
rm startup-sequence

