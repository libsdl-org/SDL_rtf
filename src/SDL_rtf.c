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

#include <SDL3/SDL.h>
#include <SDL3_rtf/SDL_rtf.h>

#include "rtftype.h"
#include "rtfdecl.h"
#include "SDL_rtfreadr.h"

/* rcg06192001 get linked library's version. */
int RTF_Version(void)
{
    return SDL_RTF_VERSION;
}

/* Create an RTF display context, with the given font engine.
 * Once a context is created, it can be used to load and display
 * text in Microsoft RTF format.
 */
RTF_Context *RTF_CreateContext(SDL_Renderer *renderer, RTF_FontEngine *fontEngine)
{
    RTF_Context *ctx;

    if (fontEngine->version != RTF_FONT_ENGINE_VERSION) {
            SDL_SetError("Unknown font engine version");
            return NULL;
    }

    ctx = (RTF_Context *)SDL_malloc(sizeof(*ctx));
    if (!ctx) {
            SDL_SetError("Out of memory");
            return NULL;
    }
    SDL_memset(ctx, 0, sizeof(*ctx));
    ctx->renderer = renderer;
    ctx->fontEngine = (RTF_FontEngine *)SDL_malloc(sizeof *fontEngine);
    if (!ctx->fontEngine) {
        SDL_SetError("Out of memory");
        SDL_free(ctx);
        return NULL;
    }
    SDL_memcpy(ctx->fontEngine, fontEngine, sizeof(*fontEngine));
    return ctx;
}

/* Set the text of an RTF context.
 * This function returns true if it succeeds or false if it fails.
 * Use SDL_GetError() to get a text message corresponding to the error.
 */
bool RTF_Load_IO(RTF_Context *ctx, SDL_IOStream *src, bool closeio)
{
    bool retval;

    ecClearContext(ctx);

    /* Set up the input stream for loading */
    ctx->rds = 0;
    ctx->ris = 0;
    ctx->cbBin = 0;
    ctx->fSkipDestIfUnk = 0;
    ctx->stream = src;
    ctx->nextch = -1;

    /* Parse the RTF text and clean up */
    switch (ecRtfParse(ctx)) {
        case ecOK:
            retval = true;
            break;
        case ecStackUnderflow:
            retval = SDL_SetError("Unmatched '}'");
            break;
        case ecStackOverflow:
            retval = SDL_SetError("Too many '{' -- memory exhausted");
            break;
        case ecUnmatchedBrace:
            retval = SDL_SetError("RTF ended during an open group");
            break;
        case ecInvalidHex:
            retval = SDL_SetError("Invalid hex character found in data");
            break;
        case ecBadTable:
            retval = SDL_SetError("RTF table (sym or prop) invalid");
            break;
        case ecAssertion:
            retval = SDL_SetError("Assertion failure");
            break;
        case ecEndOfFile:
            retval = SDL_SetError("End of file reached while reading RTF");
            break;
        case ecFontNotFound:
            retval = SDL_SetError("Couldn't find font for text");
            break;
        default:
            retval = SDL_SetError("Unknown error");
            break;
    }
    while (ctx->psave) {
        ecPopRtfState(ctx);
    }
    ctx->stream = NULL;

    if (closeio) {
        SDL_CloseIO(src);
    }
    return retval;
}

bool RTF_Load(RTF_Context *ctx, const char *file)
{
    SDL_IOStream *src = SDL_IOFromFile(file, "rb");
    if (!src) {
        return false;
    }
    return RTF_Load_IO(ctx, src, 1);
}

/* Get the title of an RTF document */
const char *RTF_GetTitle(RTF_Context *ctx)
{
    return ctx->title ? ctx->title : "";
}

/* Get the subject of an RTF document */
const char *RTF_GetSubject(RTF_Context *ctx)
{
    return ctx->subject ? ctx->subject : "";
}

/* Get the author of an RTF document */
const char *RTF_GetAuthor(RTF_Context *ctx)
{
    return ctx->author ? ctx->author : "";
}

/* Get the height of an RTF render area given a certain width.
 * The text is automatically reflowed to this new width, and should match
 * the width of the clipping rectangle used for rendering later.
 */
int RTF_GetHeight(RTF_Context *ctx, int width)
{
    ecReflowText(ctx, width);
    return ctx->displayHeight;
}

/* Render the RTF document to a rectangle of a surface.
   The text is reflowed to match the width of the rectangle.
   The rendering is offset up (and clipped) by yOffset pixels.
*/
void RTF_Render(RTF_Context *ctx, SDL_Rect *rect, int yOffset)
{
    SDL_Renderer *renderer = (SDL_Renderer *)ctx->renderer;
    SDL_Rect fullRect;
    if (!rect) {
        SDL_GetRenderViewport(renderer, &fullRect);
        fullRect.x = 0;
        fullRect.y = 0;
        rect = &fullRect;
    }
    ecRenderText(ctx, rect, -yOffset);
}

/* Free an RTF display context */
void RTF_FreeContext(RTF_Context *ctx)
{
    /* Free it all! */
    ecClearContext(ctx);
    SDL_free(ctx->fontEngine);
    SDL_free(ctx);
}
