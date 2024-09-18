/*
  SDL_rtf:  A companion library to SDL for displaying RTF format text
  Copyright (C) 2003-2024 Sam Lantinga <slouken@libsdl.org>

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

/* $Id$ */

#ifndef SDL_RTF_H_
#define SDL_RTF_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_begin_code.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Printable format: "%d.%d.%d", MAJOR, MINOR, MICRO
 */
#define SDL_RTF_MAJOR_VERSION   3
#define SDL_RTF_MINOR_VERSION   0
#define SDL_RTF_MICRO_VERSION   0

/**
 * This is the version number macro for the current SDL_rtf version.
 */
#define SDL_RTF_VERSION \
    SDL_VERSIONNUM(SDL_RTF_MAJOR_VERSION, SDL_RTF_MINOR_VERSION, SDL_RTF_MICRO_VERSION)

/**
 * This macro will evaluate to true if compiled with SDL_rtf at least X.Y.Z.
 */
#define SDL_RTF_VERSION_ATLEAST(X, Y, Z) \
    ((SDL_RTF_MAJOR_VERSION >= X) && \
     (SDL_RTF_MAJOR_VERSION > X || SDL_RTF_MINOR_VERSION >= Y) && \
     (SDL_RTF_MAJOR_VERSION > X || SDL_RTF_MINOR_VERSION > Y || SDL_RTF_MICRO_VERSION >= Z))

/**
 * This function gets the version of the dynamically linked SDL_rtf library.
 *
 * \returns SDL_rtf version.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC int SDLCALL RTF_Version(void);


typedef struct _RTF_Context RTF_Context;

typedef enum RTF_FontFamily
{
    RTF_FontDefault,    /* Unknown or default font */
    RTF_FontRoman,      /* Proportionally spaced serif fonts,
                           e.g. Times New Roman, Palatino */
    RTF_FontSwiss,      /* Proportionally spaced sans serif fonts,
                           e.g. Arial */
    RTF_FontModern,     /* Fixed pitch serif and sans serif fonts,
                           e.g. Courier New, Pica */
    RTF_FontScript,     /* Script fonts, e.g. Cursive */
    RTF_FontDecor,      /* Decorative fonts, e.g. Zapf Chancery */
    RTF_FontTech,       /* Technical, symbol, and math fonts,
                           e.g. Symbol */
    RTF_FontBidi        /* Bidirectional fonts, like Arabic or Hebrew */
}
RTF_FontFamily;

typedef enum RTF_FontStyle
{
    RTF_FontNormal    = 0x00,
    RTF_FontBold      = 0x01,
    RTF_FontItalic    = 0x02,
    RTF_FontUnderline = 0x04
}
RTF_FontStyle;

/* Various functions that need to be provided to give SDL_rtf font support */

#define RTF_FONT_ENGINE_VERSION 1

typedef struct _RTF_FontEngine
{
    int version;        /* This should be set to RTF_FONT_ENGINE_VERSION */

    /* A function to create a font matching the requested parameters.
       The family is one of those listed in the RTF_FontFamily enum.
       The charset is a Windows character set
       The size is in points.
       The style is a bitmask of the constants in RTF_FontStyle.
     */
    void *(SDLCALL *CreateFont)(const char *name, RTF_FontFamily family, int charset, int size, int style);

    /* Return the spacing in pixels between rows of text using this font */
    int (SDLCALL *GetLineSpacing)(void *font);

    /* Fill in and return the byte and pixel offsets to each character
     * within the given UTF-8 text.
     */
    int (SDLCALL *GetCharacterOffsets)(void *font, const char *text, int *byteOffsets, int *pixelOffsets, int maxOffsets);

    /* Create a texture containing a row of the given UTF-8 text */
    SDL_Texture *(SDLCALL *RenderText)(void *font, SDL_Renderer *renderer, const char *text, SDL_Color fg);

    /* Free a font */
    void (SDLCALL *FreeFont)(void *font);
} RTF_FontEngine;


/**
 * Create an RTF display context, with the given font engine.
 *
 * Once a context is created, it can be used to load and display text in
 * Microsoft RTF format.
 *
 * \param renderer an SDL renderer to use for drawing.
 * \param fontEngine the font engine to use for rendering text.
 * \returns a new RTF display context, or NULL on error.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC RTF_Context * SDLCALL RTF_CreateContext(SDL_Renderer *renderer, RTF_FontEngine *fontEngine);

/**
 * Set the text of an RTF context, with data loaded from a filename.
 *
 * This can be called multiple times to change the text displayed.
 *
 * On failure, call RTF_GetError() to get a human-readable text message
 * corresponding to the error.
 *
 * \param ctx the RTF context to update.
 * \param file the file path to load RTF data from.
 * \returns true on success or false on failure; call SDL_GetError() for more
 *          information.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC bool SDLCALL RTF_Load(RTF_Context *ctx, const char *file);

/**
 * Set the text of an RTF context, with data loaded from an SDL_IOStream.
 *
 * This can be called multiple times to change the text displayed.
 *
 * If `closeio` is true, this function will close `src`, whether this function
 * succeeded or not.
 *
 * On failure, call RTF_GetError() to get a human-readable text message
 * corresponding to the error.
 *
 * \param ctx the RTF context to update.
 * \param src the SDL_IOStream to load RTF data from.
 * \param closeio true to close `src` when the font is closed, false to leave
 *                it open.
 * \returns true on success or false on failure; call SDL_GetError() for more
 *          information.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC bool SDLCALL RTF_Load_IO(RTF_Context *ctx, SDL_IOStream *src, bool closeio);

/**
 * Get the title of an RTF document.
 *
 * The returned string is part of the RTF_Context, and should not be modified
 * or freed by the application. The pointer remains valid until the
 * RTF_Context is freed.
 *
 * \param ctx the RTF context to query.
 * \returns the document title in UTF-8 encoding.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC const char * SDLCALL RTF_GetTitle(RTF_Context *ctx);

/**
 * Get the subject of an RTF document.
 *
 * The returned string is part of the RTF_Context, and should not be modified
 * or freed by the application. The pointer remains valid until the
 * RTF_Context is freed.
 *
 * \param ctx the RTF context to query.
 * \returns the document subject in UTF-8 encoding.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC const char * SDLCALL RTF_GetSubject(RTF_Context *ctx);

/**
 * Get the author of an RTF document.
 *
 * The returned string is part of the RTF_Context, and should not be modified
 * or freed by the application. The pointer remains valid until the
 * RTF_Context is freed.
 *
 * \param ctx the RTF context to query.
 * \returns the document author in UTF-8 encoding.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC const char * SDLCALL RTF_GetAuthor(RTF_Context *ctx);

/**
 * Get the height of an RTF render area given a certain width.
 *
 * The text is automatically reflowed to this new width, and should match the
 * width of the clipping rectangle used for rendering later.
 *
 * \param ctx the RTF context to query.
 * \param width the width, in pixels, to use for text flow.
 * \returns the height, in pixels, of an RTF render area.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC int SDLCALL RTF_GetHeight(RTF_Context *ctx, int width);

/**
 * Render the RTF document to a rectangle in an SDL_Renderer.
 *
 * Rendering is done through the SDL_Renderer specified in RTF_CreateContext.
 *
 * The text is reflowed to match the width of the rectangle.
 *
 * The rendering is offset up (and clipped) by yOffset pixels.
 *
 * \param ctx the RTF context render to.
 * \param rect the area to render text into.
 * \param yOffset offset up (and clip) by this many pixels.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC void SDLCALL RTF_Render(RTF_Context *ctx, SDL_Rect *rect, int yOffset);

/**
 * Free an RTF display context.
 *
 * Call this when done rendering RTF content, to free resources used by this
 * context.
 *
 * The context is not valid after this call. This does not destroy the
 * associated SDL_Renderer, which can continue to draw and present.
 *
 * \param ctx the RTF context to free.
 *
 * \since This function is available since SDL_rtf 3.0.0.
 */
extern SDL_DECLSPEC void SDLCALL RTF_FreeContext(RTF_Context *ctx);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include <SDL3/SDL_close_code.h>

#endif /* SDL_RTF_H_ */

