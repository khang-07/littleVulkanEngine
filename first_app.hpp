#pragma once

#include "lve_window.hpp"
#include "lve_pipeline.hpp"
#include "lve_swap_chain.hpp"

#include <memory>
#include <vector>

namespace lve { // hi
class FirstApp {
    public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();
    ~FirstApp();

    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

    private:
    void createPipelineLayout();
    void createPipeline();
    void createCommandBuffers();
    void drawFrame();
    

    LveWindow lveWindow{WIDTH, HEIGHT, "wassup"};
    LveDevice lveDevice{lveWindow};
    LveSwapChain lveSwapChain{lveDevice, lveWindow.getExtent()};
    std::unique_ptr<LvePipeline> lvePipeline; // unique : auto memory manage
    VkPipelineLayout pipelineLayout;
    std::vector<VkCommandBuffer> commandBuffers;
};
}
