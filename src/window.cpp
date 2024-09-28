#include "window.hpp"

namespace Minecraft {

Window::Window(const char* title, const int width, const int height) : ShouldClose(false)
{
    SDL_Init(SDL_INIT_VIDEO);
    m_Window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
}

Window::~Window()
{
    SDL_DestroyWindow(m_Window);
    m_Window = nullptr;
}

}
