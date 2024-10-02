#include "engine.hpp"


int main()
{
    Minecraft::Engine engine { 1280, 720 };
    if (!engine.init()) {
        return EXIT_FAILURE;
    }

    if (!engine.run()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
