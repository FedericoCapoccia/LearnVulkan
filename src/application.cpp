#include "engine.hpp"


int main()
{
    Minecraft::Engine engine { true };
    if (!engine.init(1280, 720)) {
        return EXIT_FAILURE;
    }

    if (!engine.run()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
