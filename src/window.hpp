#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SDL3/SDL.h>

namespace Minecraft {

class Window {
public:
  Window(const char* title, int width, int height);
  ~Window();
  bool ShouldClose;
  void UpdateSurface() const { SDL_UpdateWindowSurface(m_Window); }
private:
  SDL_Window* m_Window { nullptr };
};

}

#endif
