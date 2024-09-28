#include <cassert>
#include "window.hpp"

void handle_keyboard_input(Minecraft::Window& window, const SDL_Event* event)
{
    assert(event->type == SDL_EVENT_KEY_DOWN); // Method called with wrong event type
    if (event->key.scancode == SDL_SCANCODE_ESCAPE) { window.ShouldClose = true; }
}

int main() {
    Minecraft::Window window { "Minecraft", 1280, 720 };

    SDL_Event event;
    while(!window.ShouldClose) {

        window.UpdateSurface();

        // Event Loop
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {

            case SDL_EVENT_QUIT:
                window.ShouldClose = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                handle_keyboard_input(window, &event);
                break;

            default:
                break;
            }

        }
    }

    return 0;
}
