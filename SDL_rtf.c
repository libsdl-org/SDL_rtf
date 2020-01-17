/*
  SDL_rtf:  A companion library to SDL for displaying RTF format text
  Copyright (C) 2003-2020 Sam Lantinga <slouken@libsdl.org>

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

#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_rtf.h"

#include "rtftype.h"
#include "rtfdecl.h"
#include "SDL_rtfreadr.h"


/* rcg06192001 get linked library's version. */
const SDL_version *RTF_Linked_Version(void)
{
    static SDL_version linked_version;
    SDL_RTF_VERSION(&linked_version);
    return(&linked_version);
}


/* Create an RTF display context, with the given font engine.
 * Once a context is created, it can be used to load and display
 * text in Microsoft RTF format.
 */
RTF_Context *RTF_CreateContext(SDL_Renderer *renderer, RTF_FontEngine *fontEngine)
{
    RTF_Context *ctx;

    if ( fontEngine->version != RTF_FONT_ENGINE_VERSION ) {
            RTF_SetError("Unknown font engine version");
            return(NULL);
    }

    ctx = (RTF_Context *)malloc(sizeof(*ctx));
    if ( ctx == NULL ) {
            RTF_SetError("Out of memory");
            return(NULL);
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->renderer = renderer;
    ctx->fontEngine = malloc(sizeof *fontEngine);
    if ( ctx->fontEngine == NULL ) {
        RTF_SetError("Out of memory");
        free(ctx);
        return(NULL);
    }
    memcpy(ctx->fontEngine, fontEngine, sizeof(*fontEngine));
    return(ctx);
}

/* Set the text of an RTF context.
 * This function returns 0 if it succeeds or -1 if it fails.
 * Use RTF_GetError() to get a text message corresponding to the error.
 */
int RTF_Load_RW(RTF_Context *ctx, SDL_RWops *src, int freesrc)
{
    int retval;

    ecClearContext(ctx);

    /* Set up the input stream for loading */
    ctx->rds = 0;
    ctx->ris = 0;
    ctx->cbBin = 0;
    ctx->fSkipDestIfUnk = 0;
    ctx->stream = src;
    ctx->nextch = -1;

    /* Parse the RTF text and clean up */
    switch(ecRtfParse(ctx)) {
        case ecOK:
            retval = 0;
            break;
        case ecStackUnderflow:
            RTF_SetError("Unmatched '}'");
            retval = -1;
            break;
        case ecStackOverflow:
            RTF_SetError("Too many '{' -- memory exhausted");
            retval = -1;
            break;
        case ecUnmatchedBrace:
            RTF_SetError("RTF ended during an open group");
            retval = -1;
            break;
        case ecInvalidHex:
            RTF_SetError("Invalid hex character found in data");
            retval = -1;
            break;
        case ecBadTable:
            RTF_SetError("RTF table (sym or prop) invalid");
            retval = -1;
            break;
        case ecAssertion:
            RTF_SetError("Assertion failure");
            retval = -1;
            break;
        case ecEndOfFile:
            RTF_SetError("End of file reached while reading RTF");
            retval = -1;
            break;
        case ecFontNotFound:
            RTF_SetError("Couldn't find font for text");
            retval = -1;
            break;
    }
    while ( ctx->psave ) {
        ecPopRtfState(ctx);
    }
    ctx->stream = NULL;

    if ( freesrc ) {
        SDL_RWclose(src);
    }
    return(retval);
}
int RTF_Load(RTF_Context *ctx, const char *file)
{
    SDL_RWops *rw = SDL_RWFromFile(file, "rb");
    if ( rw == NULL ) {
        /*RTF_SetError(SDL_GetError());*/
        return -1;
    }
    return RTF_Load_RW(ctx, rw, 1);
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
    if ( !rect ) {
        SDL_RenderGetViewport(renderer, &fullRect);
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
    free(ctx->fontEngine);
    free(ctx);
}

/* vi: set ts=4 sw=4 expandtab: */
