#include <cstdlib>
#include <exception>
#include <iostream>

#include "application.h"

int main(int /*argc*/, const char** /*argv*/)
{
    try
    {
        Nano::ApplicationConfig config;
        config.graphics_api         = Nano::GraphicsAPI::Vulkan;
        config.window_config.width  = 1280;
        config.window_config.height = 720;
        config.window_config.title  = "Nano Application";

        Nano::Application app(config);
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}