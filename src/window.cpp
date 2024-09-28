#include "window.hpp"

namespace Minecraft {

Window::Window(const int32_t width, const int32_t height)
    : m_Width(width)
    , m_Height(height)
{
    assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_Window = glfwCreateWindow(m_Width, m_Height, "Minecraft", nullptr, nullptr);

    assert(m_Window);
}

bool Window::should_close() const { return glfwWindowShouldClose(m_Window); }

Window::~Window()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

}