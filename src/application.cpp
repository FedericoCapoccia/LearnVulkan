#include "gfx_manager.hpp"
#include "window.hpp"
#include "logger.hpp"

bool m_Running = true;

int main()
{
    const Minecraft::Window window { 1280, 720 };

    Minecraft::GfxManager gfx_manager;
    if (!gfx_manager.init()) {
        return EXIT_FAILURE;
    }
    (void)gfx_manager;

    while (m_Running) {
        glfwPollEvents();
        m_Running = !window.should_close();
        m_Running = false; // TODO remove when starting to render so wayland opens the window
    }

    return EXIT_SUCCESS;
}
