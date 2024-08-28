#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_rtf/SDL_rtf.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    int version;

    if (!SDL_Init(0)) {
        SDL_Log("SDL_Init: could not initialize SDL: %s", SDL_GetError());
        return 1;
    }
    if (!TTF_Init()) {
        SDL_Log("TTF_Init: %s", SDL_GetError());
    }
    version = RTF_Version();
    SDL_Log("SDL_rtf linked version: %u.%u.%u",
                SDL_VERSIONNUM_MAJOR(version),
                SDL_VERSIONNUM_MINOR(version),
                SDL_VERSIONNUM_MICRO(version));

    TTF_Quit();
    SDL_Quit();
    return 0;
}
