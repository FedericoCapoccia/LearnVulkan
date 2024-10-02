#include "engine.hpp"
#include "logger.hpp"


int main()
{
    Minecraft::Engine engine { 1280, 720 };
    if (!engine.init()) {
        return EXIT_FAILURE;
    }

    (void)engine;

    //while (engine.is_running()) {
    //    glfwPollEvents();
    //    m_Running = !window.should_close();
    //    m_Running = false; // TODO remove when starting to render so wayland opens the window
    //}

    return EXIT_SUCCESS;
}
