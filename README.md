# pczero
experiments with bootable image of x86 and protected mode 32 bit code for the i386 platform

written in assembler and C++

intention:
* travel back to the 80's when the 32 bit intel 386 with math co-processor appeared, equipped with c++ from the future
* write a micro kernel (1 KB) in assembler that does preemptive multitasking
* handle multiple tasks and keyboard with a simple kernel written in c++
* write a small 2d framework for a toy application

contains:
* minimal assembler kernel supporting preemptive multitasking (osca.S)
* minimal c++ kernel (lib.h, kernel.h)
  - task switch (ctrl+tab)
  - enable/disable task (ctrl+F1, F2 etc)
  - keyboard focus on tasks that handle input
* sample toy application of a 2d game engine (libge.h, game.h)
  - collision detection between convex shapes
  - simple targeting system of objects in motion (can easily be extended to 3d and non-linear trajectories)
* sample tasks (main.cc)


![screenshot](https://github.com/calint/pczero/assets/1920811/00afdb23-8a76-4390-a7cc-df34f018f979)
