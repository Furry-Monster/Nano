#include "render/window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdlib>

int main(int argc, char** argv)
{
    while (Nano::g_window.shouldClose())
    {
    }

    return EXIT_SUCCESS;
}
