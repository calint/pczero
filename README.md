# pczero
experiments with bootable image of x86 and protected mode 32 bit code for the i386 platform

written in assembler and C++

contains:
* minimal kernel supporting multiple tasks
* sample toy application of a 2D game engine

```
sizes
25400	pczero.img
19111	src/osca.S
4543	src/main.cc
747	src/osca.h
6845	src/kernel.h
18472	src/lib.h
3895	src/lib2d.h
22734	src/libge.h
17737	src/Game.h

wc source
  482  2931 19111 src/osca.S
  191   533  4543 src/main.cc
   28   113   747 src/osca.h
  223   601  6845 src/kernel.h
  562  1632 18472 src/lib.h
  155   431  3895 src/lib2d.h
  708  2175 22734 src/libge.h
  691  1456 17737 src/Game.h
 3040  9872 94084 total

wc source | gzip
    117     567   24327
```