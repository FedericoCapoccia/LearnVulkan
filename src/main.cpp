#include <iostream>
#include <SDL3/SDL.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Minecraft - vulkan", 1280, 720, SDL_WINDOW_VULKAN);

    SDL_UpdateWindowSurface(window);
    SDL_Delay(1000);
    return 0;
}
