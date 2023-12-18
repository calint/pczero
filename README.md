# pczero
experiments with bootable image of x86 and protected mode 32 bit code for the i386 platform

written in assembler and C++

intention:
* travel back to the 80's when the 32 bit intel 386 with math co-processor appeared
* write a micro kernel (1 KB) in assembler that does preemptive multitasking
* handle multiple tasks and keyboard with a simple kernel written in c++
* write a small 2d framework for a toy application

contains:
* minimal assembler kernel supporting preemptive multitasking (osca.S)
* minimal c++ kernel (kernel.h)
  - task switch (ctrl+tab)
  - enabling/disabling tasks (ctrl+F1, F2 etc)
  - keyboard focus on tasks that handle input
* sample toy application of a 2d game engine (lib.h, libge.h, Game.h)
  - collision detection between convex shapes done with linear algebra
  - simple targeting system of objects in motion (can easily be extended to 3d and non-linear trajectories)
* sample tasks (main.cc)