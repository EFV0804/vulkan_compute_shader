# vulkan_compute_shader

---
**Description**

A Vulkan simulation engine with a compute and graphics pipeline.

The program is in C++ using the vulkan.hpp wrapper included in the Vulkan SDK, and the shaders are written in GLSL.


**State**

Work in progress.

Right now, it doesn't do much else than show the classic triangle on a blue background. It should be able to modify position data in the compute shader and pass that into the vertex buffer. The Simulation::updateBuffers() function doesn't work and the triangle stops displaying.

I've include a simple compute exemple that serves as a base for the simulation engine. It should run a shader that returns the squared value of the number inputed.

Based on that, I refactored it into a VkCompute class and a VkComputeShader class that get data from a Simulation class. Parallel to the compute side, I implemented a graphics pipeline to render some images.

I am currently trying to pass data from the simulation through the compute pipeline and to the graphics pipeline. The goal is to input positions, compute a transform, and pass that transformed position to the vertex shader.

---
**Instructions**

Run using Visual Studio. There's a Visual Studio property sheet included with the project to set include and lib path as needed. If Vulkan SDK is not in the default install location, it will need to be reset.

The dependencies are included in the 'external' folder.

**Dependencies:**
- GLFW  3.3.6 WIN64: for the window.
- glm 0.9.9.8: C++ Math library based on GLSL
- Vulkan SDK 1.2.198.1

---
**Credit/Resources:**
- [A Simple Vulkan Compute Example in C++][1] by Baked Bits
- [A simple Vulkan compute example][5] by Neil Henning
- [Vulkan C++ examples and demos][2] by SaschaWillems
- [Vulkan Guide][3]
- [Vulkan Tutorial][6]
- [Memory Management in Vulkan][4] by Jordan Logan

[1]:https://bakedbits.dev/posts/vulkan-compute-example/
[2]:https://github.com/SaschaWillems/Vulkan
[3]:https://vkguide.dev/docs/gpudriven/compute_shaders/
[4]:https://www.khronos.org/assets/uploads/developers/library/2018-vulkan-devday/03-Memory.pdf
[5]:https://www.duskborn.com/posts/a-simple-vulkan-compute-example/
[6]:https://vulkan-tutorial.com/
