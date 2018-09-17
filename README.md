# VulkanRenderer

C++17 Game Engine built ontop of the Vulkan Graphics API

Used as a testing ground for learning various design patterns and software engineering techniques. 

Undergone heavy revision since July 2017.

## Major Features
 * Terrain Renderer, Heightmap based with automatic level of detail adjustment using a Quadtree  and memory pool for quick allocation. 
 * Procedural Noise Texture Generator, Visual node graph replacement for libNoise that takes advantage of SIMD hardware using the FastNoiseSIMD library. Meant to be an easy to use tool for creating custom procedural terrain. Implements saving and loading with a json schema. Uses Dear ImGUI for its GUI
 * Windows and Linux Support, Cross platform. Built and tested on both major OS’s.
 * Multitasking System, (WIP) Thread pool based tasking system that enables task based multithreading of various jobs. 
 * Frame Graph, (WIP) Takes all rendering operations and resources and abstracts into a single graph, enabling simpler code and reasoning about how a frame is rendered.

## Build Process

Cmake based build system.
C++17 compiler required. On windows open the folder using Visual Studio's "open folder" functionality. linux
Requires glfw and glm installed. 
