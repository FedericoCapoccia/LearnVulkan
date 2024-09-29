#include "gfx_manager.hpp"
#include "window.hpp"

bool m_Running = true;

int main()
{
    const Minecraft::Window window { 1280, 720 };

    Minecraft::GfxManager gfx_manager;
    if (!gfx_manager.init()) {
        std::cerr << "Failed to initialize graphic manager\n";
        return EXIT_FAILURE;
    }
    (void)gfx_manager;

    while (m_Running) {
        m_Running = !window.should_close();
        glfwPollEvents();
        m_Running = false;
    }

    return EXIT_SUCCESS;
}
