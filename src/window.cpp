#include "window.hpp"
#include "logger.hpp"

namespace Minecraft {

Window::Window(const int32_t width, const int32_t height)
    : m_Width(width)
    , m_Height(height)
{
    if (!glfwInit()) {
        LOG_ERROR("Failed to init glfw");
        assert(false);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_Window = glfwCreateWindow(m_Width, m_Height, "Minecraft", nullptr, nullptr);

    if (!m_Window) {
        LOG_ERROR("Failed to create glfw window");
        assert(false);
    }
}

bool Window::should_close() const { return glfwWindowShouldClose(m_Window); }

Window::~Window()
{
    LOG("Window destructor");
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

}