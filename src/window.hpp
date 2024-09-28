#ifndef WINDOW_HPP
#define WINDOW_HPP

namespace Minecraft {

class Window {
public:
    Window(int32_t width, int32_t height);
    ~Window();
    [[nodiscard]] bool should_close() const;

private:
    GLFWwindow* m_Window { nullptr };
    int32_t m_Width, m_Height;
};

}

#endif
