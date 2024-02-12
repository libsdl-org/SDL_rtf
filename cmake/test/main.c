#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_rtf/SDL_rtf.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    const SDL_Version *v;

    if (SDL_Init(0) < 0) {
        SDL_Log("SDL_Init: could not initialize SDL: %s", SDL_GetError());
        return 1;
    }
    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init: %s", TTF_GetError());
    }
    v = RTF_Linked_Version();
    SDL_Log("SDL_rtf linked version: %u.%u.%u", v->major, v->minor, v->patch);

    TTF_Quit();
    SDL_Quit();
    return 0;
}
