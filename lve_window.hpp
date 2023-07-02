#pragma once

#define GLFW_INCLUDE_VULKAN
#include "include/GLFW/glfw3.h"

#include <string>
namespace lve {

class LveWindow {
    public:
    LveWindow(int w, int h, std::string name);
    ~LveWindow(); // destructor

    LveWindow(const LveWindow &) = delete; // copy -> delete og -> dangling pt -> big bad
    LveWindow &operator=(const LveWindow &) = delete;

    bool shouldClose() { return glfwWindowShouldClose(window); }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    int getWidth() { return width; }
    int getHeight() { return height; }
        
    private:
    void initWindow();
    const int width;
    const int height;

    std::string windowName;
    GLFWwindow *window;
};
}