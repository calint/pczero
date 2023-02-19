# pczero
experiments with bootable image of x86 and protected mode 32 bit code for the i386 platform

written in assembler and C++

contains:
* minimal kernel supporting multiple tasks
* sample toy application of a 2D game engine

```
sizes
25404   pczero.img
19165   src/osca.S
4543    src/main.cc
747     src/osca.h
6979    src/kernel.h
18301   src/lib.h
3895    src/lib2d.h
22852   src/libge.h
17686   src/Game.h

wc source
  477  2941 19165 src/osca.S
  191   533  4543 src/main.cc
   28   113   747 src/osca.h
  231   601  6979 src/kernel.h
  560  1606 18301 src/lib.h
  155   431  3895 src/lib2d.h
  716  2185 22852 src/libge.h
  688  1448 17686 src/Game.h
 3046  9858 94168 total

wc source | gzip
    102     590   24281
```