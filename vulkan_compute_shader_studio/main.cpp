#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include "VkRenderer.h"
#include "Simulation.h"
#include <fstream>
#include <iostream>
#include <string>

using std::string;



GLFWwindow* window = nullptr;
VkRenderer renderer;

void initWindow(string wName = "Beautiful Window", const int width = 800, const int height = 600)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}
void clean()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {

    initWindow();
    if (renderer.init(window) == EXIT_FAILURE) return EXIT_FAILURE;

    const char* computeShaderFile = "shaders/comp.spv";
    Simulation simulation = Simulation{ &renderer,computeShaderFile };
    simulation.init();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        simulation.run();
    }

    clean();
    simulation.close();

    return 0;
}
