/*
  showrtf:  An example of using the SDL_rtf library
  Copyright (C) 2003-2022 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* A simple program to test the RTF rendering of the SDL_rtf library */

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_rtf.h"

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

static const char *FontList[8];

/* Note, this is only one way of looking up fonts */
static int FontFamilyToIndex(RTF_FontFamily family)
{
    switch(family) {
        case RTF_FontDefault:
            return 0;
        case RTF_FontRoman:
            return 1;
        case RTF_FontSwiss:
            return 2;
        case RTF_FontModern:
            return 3;
        case RTF_FontScript:
            return 4;
        case RTF_FontDecor:
            return 5;
        case RTF_FontTech:
            return 6;
        case RTF_FontBidi:
            return 7;
        default:
            return 0;
    }
}

static Uint16 UTF8_to_UNICODE(const char *utf8, int *advance)
{
    int i = 0;
    Uint16 ch;

    ch = ((const unsigned char *)utf8)[i];
    if ( ch >= 0xF0 ) {
        ch  =  (Uint16)(utf8[i]&0x07) << 18;
        ch |=  (Uint16)(utf8[++i]&0x3F) << 12;
        ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
        ch |=  (Uint16)(utf8[++i]&0x3F);
    } else if ( ch >= 0xE0 ) {
        ch  =  (Uint16)(utf8[i]&0x3F) << 12;
        ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
        ch |=  (Uint16)(utf8[++i]&0x3F);
    } else if ( ch >= 0xC0 ) {
        ch  =  (Uint16)(utf8[i]&0x3F) << 6;
        ch |=  (Uint16)(utf8[++i]&0x3F);
    }
    *advance = (i+1);
    return ch;
}

static void * SDLCALL CreateFont(const char *name, RTF_FontFamily family, int charset, int size, int style)
{
    int index;
    TTF_Font *font;

    index = FontFamilyToIndex(family);
    if (!FontList[index])
        index = 0;

    font = TTF_OpenFont(FontList[index], size);
    if (font) {
        int TTF_style = TTF_STYLE_NORMAL;
        if ( style & RTF_FontBold )
            TTF_style |= TTF_STYLE_BOLD;
        if ( style & RTF_FontItalic )
            TTF_style |= TTF_STYLE_ITALIC;
        if ( style & RTF_FontUnderline )
            TTF_style |= TTF_STYLE_UNDERLINE;
        TTF_SetFontStyle(font, style);
    }

    /* FIXME: What do we do with the character set? */

    return font;
}

static int SDLCALL GetLineSpacing(void *_font)
{
    TTF_Font *font = (TTF_Font *)_font;
    return TTF_FontLineSkip(font);
}

static int SDLCALL GetCharacterOffsets(void *_font, const char *text, int *byteOffsets, int *pixelOffsets, int maxOffsets)
{
    TTF_Font *font = (TTF_Font *)_font;
    int i = 0;
    int bytes = 0;
    int pixels = 0;
    int advance;
    Uint16 ch;
    while ( *text && i < maxOffsets ) {
        byteOffsets[i] = bytes;
        pixelOffsets[i] = pixels;
        ++i;

        ch = UTF8_to_UNICODE(text, &advance);
        text += advance;
        bytes += advance;
        TTF_GlyphMetrics(font, ch, NULL, NULL, NULL, NULL, &advance);
        pixels += advance;
    }
    if ( i < maxOffsets ) {
        byteOffsets[i] = bytes;
        pixelOffsets[i] = pixels;
    }
    return i;
}

static SDL_Texture * SDLCALL RenderText(void *_font, SDL_Renderer *renderer, const char *text, SDL_Color fg)
{
    TTF_Font *font = (TTF_Font *)_font;
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, fg);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    return texture;
}

static void SDLCALL FreeFont(void *_font)
{
    TTF_Font *font = (TTF_Font *)_font;
    TTF_CloseFont(font);
}

static void LoadRTF(RTF_Context *ctx, const char *file)
{
    if ( RTF_Load(ctx, file) < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s: %s\n", file, RTF_GetError());
        return;
    }
}

static void PrintUsage(const char *argv0)
{
    SDL_Log("Usage: %s -fdefault font.ttf [-froman font.ttf] [-fswiss font.ttf] [-fmodern font.ttf] [-fscript font.ttf] [-fdecor font.ttf] [-ftech font.ttf] file.rtf\n", argv0);
}

static void cleanup(void)
{
    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    int i, start, stop;
    int w, h;
    int done;
    int height;
    int offset;
    SDL_Window *window;
    SDL_Renderer *renderer;
    RTF_Context *ctx;
    RTF_FontEngine fontEngine;
    const Uint8 *keystate;

    /* Parse command line arguments */
    for ( i = 1; i < argc; ++i ) {
        if ( SDL_strcmp(argv[i], "-fdefault") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontDefault)] = argv[++i];
        } else if ( SDL_strcmp(argv[i], "-froman") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontRoman)] = argv[++i];
        } else if ( SDL_strcmp(argv[i], "-fswiss") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontSwiss)] = argv[++i];
        } else if ( SDL_strcmp(argv[i], "-fmodern") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontModern)] = argv[++i];
        } else if ( SDL_strcmp(argv[i], "-fscript") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontScript)] = argv[++i];
        } else if ( SDL_strcmp(argv[i], "-fdecor") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontDecor)] = argv[++i];
        } else if ( SDL_strcmp(argv[i], "-ftech") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontTech)] = argv[++i];
        } else {
            break;
        }
    }
    start = i;
    stop = (argc-1);
    if ( !FontList[0] || (start > stop) ) {
        PrintUsage(argv[0]);
        return(1);
    }

    /* Initialize the TTF library */
    if ( TTF_Init() < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize TTF: %s\n",SDL_GetError());
        SDL_Quit();
        return(3);
    }

    if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindowAndRenderer() failed: %s\n", SDL_GetError());
        cleanup();
        return(4);
    }

    /* Create and load the RTF document */
    fontEngine.version = RTF_FONT_ENGINE_VERSION;
    fontEngine.CreateFont = CreateFont;
    fontEngine.GetLineSpacing = GetLineSpacing;
    fontEngine.GetCharacterOffsets = GetCharacterOffsets;
    fontEngine.RenderText = RenderText;
    fontEngine.FreeFont = FreeFont;
    ctx = RTF_CreateContext(renderer, &fontEngine);
    if ( ctx == NULL ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create RTF context: %s\n", RTF_GetError());
        cleanup();
        return(5);
    }
    LoadRTF(ctx, argv[i]);
    SDL_SetWindowTitle(window, RTF_GetTitle(ctx));

    /* Render the document to the screen */
    done = 0;
    offset = 0;
    SDL_GetWindowSize(window, &w, &h);
    height = RTF_GetHeight(ctx, w);
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                float ratio = (float)offset / height;
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Resetting window\n");
                SDL_GetWindowSize(window, &w, &h);
                SDL_RenderSetViewport(renderer, NULL);
                height = RTF_GetHeight(ctx, w);
                offset = (int)(ratio * height);
            }
            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        done = 1;
                        break;
                    case SDLK_LEFT:
                        if ( i > start ) {
                            --i;
                            LoadRTF(ctx, argv[i]);
                            offset = 0;
                            height = RTF_GetHeight(ctx, w);
                        }
                        break;
                    case SDLK_RIGHT:
                        if ( i < stop ) {
                            ++i;
                            LoadRTF(ctx, argv[i]);
                            offset = 0;
                            height = RTF_GetHeight(ctx, w);
                        }
                        break;
                    case SDLK_HOME:
                        offset = 0;
                        break;
                    case SDLK_END:
                        offset = (height - h);
                        break;
                    case SDLK_PAGEUP:
                        offset -= h;
                        if ( offset < 0 )
                            offset = 0;
                        break;
                    case SDLK_PAGEDOWN:
                    case SDLK_SPACE:
                        offset += h;
                        if ( offset > (height - h) )
                            offset = (height - h);
                        break;
                    default:
                        break;
                }
            }
            if (event.type == SDL_QUIT) {
                done = 1;
            }
        }
        keystate = SDL_GetKeyboardState(NULL);
        if ( keystate[SDL_SCANCODE_UP] ) {
            offset -= 1;
            if ( offset < 0 )
                offset = 0;
        }
        if ( keystate[SDL_SCANCODE_DOWN] ) {
            offset += 1;
            if ( offset > (height - h) )
                offset = (height - h);
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);
        RTF_Render(ctx, NULL, offset);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    /* Clean up and exit */
    RTF_FreeContext(ctx);
    cleanup();

    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
