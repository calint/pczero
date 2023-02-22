# pczero
experiments with bootable image of x86 and protected mode 32 bit code for the i386 platform

written in assembler and C++

contains:
* minimal kernel supporting multiple tasks
* sample toy application of a 2D game engine

```
sizes
25256	pczero.img
16594	src/osca.S
5693	src/main.cc
2917	src/osca.h
8430	src/kernel.h
18471	src/lib.h
3895	src/lib2d.h
22734	src/libge.h
17851	src/Game.h

wc source
  444  2786 16594 src/osca.S
  221   628  5693 src/main.cc
   96   341  2917 src/osca.h
  256   763  8430 src/kernel.h
  562  1632 18471 src/lib.h
  155   431  3895 src/lib2d.h
  708  2175 22734 src/libge.h
  696  1460 17851 src/Game.h
 3138 10216 96585 total

wc source | gzip
    112     562   25285
```