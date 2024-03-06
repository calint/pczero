#!/bin/sh
qemu-system-i386 -display gtk,zoom-to-fit=on -m 2M -drive file=pczero.img,format=raw
