# pczero
experiments with bootable image of x86 and protected mode 32 bit code for the i386 platform

written in assembler and C++

contains:
* minimal kernel supporting multiple tasks
* sample toy application of a 2D game engine

```
sizes
25372   pczero.img
19111   src/osca.S
4543    src/main.cc
747     src/osca.h
6775    src/kernel.h
18340   src/lib.h
3895    src/lib2d.h
22634   src/libge.h
17719   src/Game.h

wc source
  482  2931 19111 src/osca.S
  191   533  4543 src/main.cc
   28   113   747 src/osca.h
  222   593  6775 src/kernel.h
  562  1614 18340 src/lib.h
  155   431  3895 src/lib2d.h
  707  2161 22634 src/libge.h
  691  1452 17719 src/Game.h
 3038  9828 93764 total

wc source | gzip
     93     583   24228
```