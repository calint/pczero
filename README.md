# pczero

Experiments with bootable image of x86 and protected mode 32 bit code for the
i386 platform.

Written in assembler and C++.

## Intention

* travel back to the 80's when the 32 bit Intel 386 with math co-processor
appeared, equipped with C++ from the future
* write a micro kernel (1 KB) in assembler that does preemptive multitasking
* handle multiple tasks and keyboard with a simple kernel written in C++
* write a small 2D framework for a toy application
* no dependencies, bare metal

## Contains

* minimal assembler kernel supporting preemptive multitasking (`osca.S`)
* minimal C++ kernel (`lib.hpp`, `kernel.hpp`)
  * task switch (ctrl+tab)
  * enable/disable task (ctrl+F1, F2 etc)
  * keyboard focus on tasks that handle input
* sample toy application of a 2D game engine (`libge.hpp`, `game.hpp`)
  * collision detection between convex shapes
  * simple targeting system of objects in motion (can easily be extended to 3D
  and non-linear trajectories)
* sample tasks (`main.cpp`)

![screenshot 2024-03-13--12-07-40](https://github.com/calint/pczero/assets/1920811/fdbb313f-c202-411b-9806-6f472d39d167)
