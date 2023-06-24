#pragma once

#include "lve_window.hpp"
#include "lve_pipeline.hpp"

namespace lve { // hi
class FirstApp {
    public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    void run();

    private:
    LveWindow lveWindow{WIDTH, HEIGHT, "wassup"};
    LveDevice lveDevice{lveWindow};
    LvePipeline lvePipeline{
        lveDevice, 
        "shaders/simple_shader.vert.spv", 
        "shaders/simple_shader.frag.spv",
        LvePipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
};
}
