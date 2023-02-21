# pczero
experiments with bootable image of x86 and protected mode 32 bit code for the i386 platform

written in assembler and C++

contains:
* minimal kernel supporting multiple tasks
* sample toy application of a 2D game engine

```
sizes
24788	pczero.img
15436	src/osca.S
4881	src/main.cc
1718	src/osca.h
7924	src/kernel.h
18471	src/lib.h
3895	src/lib2d.h
22734	src/libge.h
17855	src/Game.h

wc source
  418  2623 15436 src/osca.S
  197   575  4881 src/main.cc
   69   227  1718 src/osca.h
  247   715  7924 src/kernel.h
  562  1632 18471 src/lib.h
  155   431  3895 src/lib2d.h
  708  2175 22734 src/libge.h
  694  1460 17855 src/Game.h
 3050  9838 92914 total

wc source | gzip
     97     533   24514
```