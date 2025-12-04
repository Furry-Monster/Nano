#include <cstdlib>
#include <exception>
#include <iostream>

#include "application.h"

int main(int /*argc*/, const char** /*argv*/)
{
    Nano::Application app;

    try
    {
        app.boot();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}