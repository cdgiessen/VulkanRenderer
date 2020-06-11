# VulkanRenderer

C++17 Game Engine built on top of the Vulkan Graphics API

Used as a testing ground for learning various software techniques and solutions for engineering problems. 

Undergone heavy revision since July 2017 and is still under construction.

## Major Features
 * Terrain Renderer, Heightmap based with automatic level of detail adjustment using a Quadtree  and memory pool for quick allocation. 
 * Procedural Noise Texture Generator, Visual node graph using the FastNoiseSIMD library. An easy to use tool for creating custom procedural terrain. Implements saving and loading with json files. Uses Dear-ImGUI
 * Windows and Linux Support, Cross platform.
 * Multitasking System, (WIP) Thread pool based tasking system that enables task based multithreading of various jobs. 
 * Frame Graph, (WIP) Takes all rendering operations and resources and abstracts into a single graph, enabling simpler code and reasoning about how a frame is rendered.

## Build Process
Requirements: C++17 Compiler, CMake 3.11, Vulkan 1.1 SDK

```bash
git clone https://github.com/cdgiessen/VulkanRenderer
mkdir build
cd build
cmake ../VulkanRenderer
```

## Screenshots
General view from a landscape
![](https://github.com/cdgiessen/VulkanRenderer/blob/master/assets/screenshots/Siggraph_lanscape2.jpg)

Visual Node Graph Example
![](https://github.com/cdgiessen/VulkanRenderer/blob/master/assets/screenshots/Siggraph_node_graph.jpg)

Terrain Renderer in wireframe mode to show LOD
![](https://github.com/cdgiessen/VulkanRenderer/blob/master/assets/screenshots/siggraph_terrain_lod.jpg)

## Plans
I plan to include the aforementioned Multitasking system and Frame Graph as well as the following:
 * Depth Prepass
 * Forward+ lighting
 * Shader creator - allowing easy shader creation
 * glTF model loading
 * Terrain scatter/futher terrain details
 * Procedural Sky Shader
 * Integrate Bullet Physics
 
