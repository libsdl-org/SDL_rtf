/*
 * This file was adapted from Microsoft Rich Text Format Specification 1.6
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnrtfspec/html/rtfspec.asp
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rtftype.h"
#include "rtfdecl.h"


/*
 * %%Function: ecAddFontEntry
 */

int
ecAddFontEntry(RTF_Context *ctx, int number, const char *name, int family)
{
    RTF_FontEntry *entry = (RTF_FontEntry *)malloc(sizeof(*entry));
    if (!entry)
        return ecStackOverflow;

    entry->number = number;
    entry->name = strdup(name);
    entry->family = (RTF_FontFamily)family;
    entry->fonts = NULL;
    entry->next = ctx->fontTable;
    ctx->fontTable = entry;
    return ecOK;
}

/*
 * %%Function: ecLookupFont
 */

void *
ecLookupFont(RTF_Context *ctx)
{
    RTF_FontEntry *entry;
    RTF_Font *font;
    int size;
    int style;

    /* Figure out what size we should use */
    size = ctx->chp.fFontSize;
    if (!size)
        size = 24;

    /* Figure out what style we should use */
    style = RTF_FontNormal;
    if (ctx->chp.fBold)
        style |= RTF_FontNormal;
    if (ctx->chp.fItalic)
        style |= RTF_FontItalic;
    if(ctx->chp.fUnderline)
        style |= RTF_FontUnderline;

    /* Search for the correct font */
    for (entry = ctx->fontTable; entry; entry = entry->next)
    {
        if (entry->number == ctx->chp.fFont)
            break;
    }
    if (!entry)
    {
        /* Search for the first default font */
        for (entry = ctx->fontTable; entry; entry = entry->next)
        {
            if (entry->family == RTF_FontDefault)
                break;
        }
    }
    if (!entry)
    {
        /* If we still didn't find a font, just use the first font */
        if (ctx->fontTable)
            entry = ctx->fontTable;
        else
            return NULL;
    }

    /* We found a font entry, now find the font */
    for (font = entry->fonts; font; font = font->next)
    {
        if (size == font->size && style == font->style)
            return font->font;
    }

    /* Create a new font entry */
    font = (RTF_Font *)malloc(sizeof(*font));
    if (!font)
        return NULL;

    font->font = ctx->fontEngine.CreateFont(entry->name, entry->family, size/2, style);
    if (!font->font)
    {
        free(font);
        return NULL;
    }
    font->size = size;
    font->style = style;
    font->next = entry->fonts;
    entry->fonts = font;
    return font->font;
}

/*
 * %%Function: ecClearFonts
 */

int
ecClearFonts(RTF_Context *ctx)
{
    while (ctx->fontTable)
    {
        RTF_FontEntry *entry = ctx->fontTable;
        ctx->fontTable = entry->next;
        free(entry->name);
        while (entry->fonts)
        {
            RTF_Font *font = entry->fonts;
            entry->fonts = font->next;
            ctx->fontEngine.FreeFont(font->font);
            free(font);
        }
        free(entry);
    }
    return ecOK;
}

/*
 * %%Function: ecAddColorEntry
 */

int
ecAddColorEntry(RTF_Context *ctx, int r, int g, int b)
{
    int index = ctx->colorTable.ncolors;
    ctx->colorTable.ncolors++;
    ctx->colorTable.colors = (SDL_Color *)realloc(ctx->colorTable.colors, ctx->colorTable.ncolors * sizeof(SDL_Color));
    if (!ctx->colorTable.colors)
    {
        ctx->colorTable.ncolors = 0;
        return ecStackOverflow;
    }
    ctx->colorTable.colors[index].r = (Uint8)r;
    ctx->colorTable.colors[index].g = (Uint8)g;
    ctx->colorTable.colors[index].b = (Uint8)b;
    return ecOK;
}

/*
 * %%Function: ecLookupColor
 */

int
ecLookupColor(RTF_Context *ctx, SDL_Color *color)
{
    int index = ctx->chp.fFgColor;
    if(index < ctx->colorTable.ncolors)
    {
        *color = ctx->colorTable.colors[index];
        return ecOK;
    }
    memset(color, 0, sizeof(*color));
    return ecBadTable;
}

/*
 * %%Function: ecClearColors
 */

int
ecClearColors(RTF_Context *ctx)
{
    if (ctx->colorTable.colors)
    {
        free(ctx->colorTable.colors);
        ctx->colorTable.colors = NULL;
        ctx->colorTable.ncolors = 0;
    }
    return ecOK;
}

static void FreeTextBlock(RTF_TextBlock *text)
{
    if (text->text) 
        free(text->text);
    if (text->byteOffsets)
        free(text->byteOffsets);
    if (text->pixelOffsets)
        free(text->pixelOffsets);
    free(text);
}

static void FreeLineSurfaces(RTF_Line *line)
{
    while (line->startSurface)
    {
        RTF_Surface *surface = line->startSurface;
        line->startSurface = surface->next;
        SDL_FreeSurface(surface->surface);
        free(surface);
    }
}

static void FreeLine(RTF_Line *line)
{
    FreeLineSurfaces(line);
    while (line->start)
    {
        RTF_TextBlock *text = line->start;
        line->start = text->next;
        FreeTextBlock(text);
    }
    free(line);
}

/*
 * %%Function: ecAddLine
 */

int
ecAddLine(RTF_Context *ctx)
{
    RTF_Line *line;

    /* Lookup the current font */
    void *font = ecLookupFont(ctx);
    if (!font)
        return ecFontNotFound;

    line = (RTF_Line *)malloc(sizeof(*line));
    if (!line)
        return ecStackOverflow;

    line->pap = ctx->pap;
    line->lineWidth = 0;
    line->lineHeight = ctx->fontEngine.GetLineSpacing(font);
    line->tabs = 0;
    line->start = NULL;
    line->last = NULL;
    line->startSurface = NULL;
    line->lastSurface = NULL;
    line->next = NULL;

#ifdef DEBUG_RTF
    printf("Added line ---------------------\n");
#endif
    if (ctx->start)
        ctx->last->next = line;
    else
        ctx->start = line;
    ctx->last = line;
    return ecOK;
}

/*
 * %%Function: ecAddTab
 */

int
ecAddTab(RTF_Context *ctx)
{
    RTF_Line *line;

    /* Add the tabs to the last line added */
    if (!ctx->last)
    {
        int status = ecAddLine(ctx);
        if (status != ecOK)
            return status;
    }
    line = ctx->last;

    ++line->tabs;
    return ecOK;
}

/*
 * %%Function: ecAddText
 */

int
ecAddText(RTF_Context *ctx, const char *text)
{
    RTF_Line *line;
    RTF_TextBlock *textBlock;
    int numChars;

    /* Lookup the current font */
    void *font = ecLookupFont(ctx);
    if (!font)
        return ecFontNotFound;

    /* Add the text to the last line added */
    if (!ctx->last)
    {
        int status = ecAddLine(ctx);
        if (status != ecOK)
            return status;
    }
    line = ctx->last;

    textBlock = (RTF_TextBlock *)malloc(sizeof(*textBlock));
    if (!textBlock)
        return ecStackOverflow;

    textBlock->font = font;
    ecLookupColor(ctx, &textBlock->color);
    numChars = strlen(text)+1;
    textBlock->tabs = line->tabs;
    textBlock->text = strdup(text);
    textBlock->byteOffsets = (int *)malloc(numChars * sizeof(int));
    textBlock->pixelOffsets = (int *)malloc(numChars * sizeof(int));
    if (!textBlock->text || !textBlock->byteOffsets || !textBlock->pixelOffsets)
    {
        FreeTextBlock(textBlock);
        return ecStackOverflow;
    }
    textBlock->numChars = ctx->fontEngine.GetCharacterOffsets(font, text, textBlock->byteOffsets, textBlock->pixelOffsets, numChars);
    textBlock->lineHeight = ctx->fontEngine.GetLineSpacing(font);
    textBlock->next = NULL;

#ifdef DEBUG_RTF
    printf("Added text: '%s'\n", text);
#endif
    line->pap = ctx->pap;
    line->tabs = 0;
    if (line->start)
        line->last->next = textBlock;
    else
        line->start = textBlock;
    line->last = textBlock;
    return ecOK;
}

/*
 * %%Function: ecClearLines
 */

int
ecClearLines(RTF_Context *ctx)
{
    while (ctx->start)
    {
        RTF_Line *line = ctx->start;
        ctx->start = line->next;
        FreeLine(line);
    }
    ctx->last = NULL;
    return ecOK;
}

/*
 * %%Function: ecClearContext
 */

int
ecClearContext(RTF_Context *ctx)
{
    if ( ctx->data )
    {
        free(ctx->data);
        ctx->data = NULL;
        ctx->datapos = 0;
        ctx->datamax = 0;
    }
    memset(ctx->values, 0, sizeof(ctx->values));

    ecClearFonts(ctx);
    ecClearColors(ctx);

    if ( ctx->title )
    {
        free(ctx->title);
        ctx->title = NULL;
    }
    if ( ctx->subject )
    {
        free(ctx->subject);
        ctx->subject = NULL;
    }
    if ( ctx->author )
    {
        free(ctx->author);
        ctx->author = NULL;
    }

    memset(&ctx->chp, 0, sizeof(ctx->chp));
    memset(&ctx->pap, 0, sizeof(ctx->pap));
    memset(&ctx->sep, 0, sizeof(ctx->sep));
    memset(&ctx->dop, 0, sizeof(ctx->dop));

    ecClearLines(ctx);

    ctx->displayWidth = 0;
    ctx->displayHeight = 0;

    return ecOK;
}

/*
 * %%Function: ecRtfGetChar
 */

int
ecRtfGetChar(RTF_Context *ctx, int *ch)
{
    if ( ctx->nextch >= 0 )
    {
        *ch = ctx->nextch;
        ctx->nextch = -1;
    }
    else
    {
        unsigned char c;
        if ( ctx->stream->read(ctx->stream, &c, 1, 1) != 1 )
            return ecEndOfFile;
        *ch = c;
    }
    return ecOK;
}

/*
 * %%Function: ecRtfUngetChar
 */

int
ecRtfUngetChar(RTF_Context *ctx, int ch)
{
    ctx->nextch = ch;
    return ecOK;
}

/*
 * %%Function: ecRtfParse
 *
 * Step 1:
 * Isolate RTF keywords and send them to ecParseRtfKeyword;
 * Push and pop state at the start and end of RTF groups;
 * Send text to ecParseChar for further processing.
 */

int
ecRtfParse(RTF_Context *ctx)
{
    int ch;
    int ec;
    int cNibble = 2;
    int b = 0;

    while (ecRtfGetChar(ctx, &ch) == ecOK)
    {
        if (ctx->cGroup < 0)
            return ecStackUnderflow;
        if (ctx->ris == risBin)                      /* if we're parsing binary data, handle it directly */
        {
            if ((ec = ecParseChar(ctx, ch)) != ecOK)
                return ec;
        }
        else
        {
            switch (ch)
            {
            case '{':
                if ((ec = ecPushRtfState(ctx)) != ecOK)
                    return ec;
                break;
            case '}':
                if ((ec = ecPopRtfState(ctx)) != ecOK)
                    return ec;
                break;
            case '\\':
                if ((ec = ecParseRtfKeyword(ctx)) != ecOK)
                    return ec;
                break;
            case 0x0d:
            case 0x0a:          /* cr and lf are noise characters... */
                break;
            default:
                if (ctx->ris == risNorm)
                {
                    if ((ec = ecParseChar(ctx, ch)) != ecOK)
                        return ec;
                }
                else
                {               /* parsing hex data */
                    if (ctx->ris != risHex)
                        return ecAssertion;
                    b = b << 4;
                    if (isdigit(ch))
                        b += (char) ch - '0';
                    else
                    {
                        if (islower(ch))
                        {
                            if (ch < 'a' || ch > 'f')
                                return ecInvalidHex;
                            b += 10 + (ch - 'a');
                        }
                        else
                        {
                            if (ch < 'A' || ch > 'F')
                                return ecInvalidHex;
                            b += 10 + (ch - 'A');
                        }
                    }
                    cNibble--;
                    if (!cNibble)
                    {
                        if ((ec = ecParseChar(ctx, b)) != ecOK)
                            return ec;
                        cNibble = 2;
                        b = 0;
                        ctx->ris = risNorm;
                    }
                }                   /* end else (ris != risNorm) */
                break;
            }       /* switch */
        }           /* else (ris != risBin) */
    }               /* while */
    if (ctx->cGroup < 0)
        return ecStackUnderflow;
    if (ctx->cGroup > 0)
        return ecUnmatchedBrace;
    return ecOK;
}

/*
 * %%Function: ecPushRtfState
 *
 * Save relevant info on a linked list of SAVE structures.
 */

int
ecPushRtfState(RTF_Context *ctx)
{
    SAVE *psaveNew = malloc(sizeof(SAVE));
    if (!psaveNew)
        return ecStackOverflow;

    psaveNew -> pNext = ctx->psave;
    psaveNew -> chp = ctx->chp;
    psaveNew -> pap = ctx->pap;
    psaveNew -> sep = ctx->sep;
    psaveNew -> dop = ctx->dop;
    psaveNew -> rds = ctx->rds;
    psaveNew -> ris = ctx->ris;
    ctx->ris = risNorm;
    ctx->psave = psaveNew;
    ctx->cGroup++;
    return ecOK;
}

/*
 * %%Function: ecPopRtfState
 *
 * If we're ending a destination (that is, the destination is changing),
 * call ecEndGroupAction.
 * Always restore relevant info from the top of the SAVE list.
 */

int
ecPopRtfState(RTF_Context *ctx)
{
    SAVE *psaveOld;
    int ec;

    if (!ctx->psave)
        return ecStackUnderflow;

    if (ctx->rds != ctx->psave->rds)
    {
        if ((ec = ecEndGroupAction(ctx, ctx->rds)) != ecOK)
            return ec;
    }
    ecProcessData(ctx);

    ctx->chp = ctx->psave->chp;
    ctx->pap = ctx->psave->pap;
    ctx->sep = ctx->psave->sep;
    ctx->dop = ctx->psave->dop;
    ctx->rds = ctx->psave->rds;
    ctx->ris = ctx->psave->ris;

    psaveOld = ctx->psave;
    ctx->psave = ctx->psave->pNext;
    ctx->cGroup--;
    free(psaveOld);
    return ecOK;
}

/*
 * %%Function: ecParseRtfKeyword
 *
 * Step 2:
 * get a control word (and its associated value) and
 * call ecTranslateKeyword to dispatch the control.
 */

int
ecParseRtfKeyword(RTF_Context *ctx)
{
    int ch;
    char fParam = fFalse;
    char fNeg = fFalse;
    int param = 0;
    char *pch;
    char szKeyword[30];
    char szParameter[20];

    szKeyword[0] = '\0';
    szParameter[0] = '\0';
    if (ecRtfGetChar(ctx, &ch) != ecOK)
        return ecEndOfFile;
    if (!isalpha(ch))           /* a control symbol; no delimiter. */
    {
        szKeyword[0] = (char) ch;
        szKeyword[1] = '\0';
        return ecTranslateKeyword(ctx, szKeyword, 0, fParam);
    }
    for (pch = szKeyword; isalpha(ch); )
    {
        *pch++ = (char) ch;
        if (ecRtfGetChar(ctx, &ch) != ecOK)
            return ecEndOfFile;
    }
    *pch = '\0';
    if (ch == '-')
    {
        fNeg  = fTrue;
        if (ecRtfGetChar(ctx, &ch) != ecOK)
            return ecEndOfFile;
    }
    if (isdigit(ch))
    {
        fParam = fTrue;         /* a digit after the control means we have a parameter */
        for (pch = szParameter; isdigit(ch); )
        {
            *pch++ = (char) ch;
            if (ecRtfGetChar(ctx, &ch) != ecOK)
                return ecEndOfFile;
        }
        *pch = '\0';
        param = atoi(szParameter);
        if (fNeg)
            param = -param;
        ctx->lParam = atol(szParameter);
        if (fNeg)
            param = -param;
    }
    if (ch != ' ')
        ecRtfUngetChar(ctx, ch);
    return ecTranslateKeyword(ctx, szKeyword, param, fParam);
}

/*
 * %%Function: ecParseChar
 *
 * Route the character to the appropriate destination stream.
 */

int
ecParseChar(RTF_Context *ctx, int ch)
{
    if (ctx->ris == risBin && --ctx->cbBin <= 0)
        ctx->ris = risNorm;
    switch (ctx->rds)
    {
    case rdsNorm:
        /* Output a character. Properties are valid at this point. */
        if (ch == '\t') {
            return ecTabstop(ctx);
        }
        if (ch == '\r') {
            return ecLinebreak(ctx);
        }
        if (ch == '\n') {
            return ecParagraph(ctx);
        }
        return ecPrintChar(ctx, ch);
    case rdsSkip:
        /* Toss this character. */
        return ecOK;
    case rdsFontTable:
        if(ch == ';') {
            ctx->data[ctx->datapos] = '\0';
            ecAddFontEntry(ctx, ctx->chp.fFont, ctx->data, (FFAM)ctx->values[0]);
            ctx->datapos = 0;
            ctx->values[0] = 0;
        } else {
            return ecPrintChar(ctx, ch);
        }
        return ecOK;
    case rdsColorTable:
        if(ch == ';') {
            ecAddColorEntry(ctx, ctx->values[0], ctx->values[1], ctx->values[2]);
            ctx->values[0] = 0;
            ctx->values[1] = 0;
            ctx->values[2] = 0;
        }
        return ecOK;
    case rdsTitle:
    case rdsSubject:
    case rdsAuthor:
        return ecPrintChar(ctx, ch);
    default:
        /* handle other destinations.... */
        return ecOK;
    }
}

/*
 * %%Function: ecPrintChar
 *
 * Add a character to the output text
 */

int
ecPrintChar(RTF_Context *ctx, int ch)
{
    int len;
    if(ctx->datapos >= (ctx->datamax-4)) {
        ctx->datamax += 256;    /* 256 byte chunk size */
        ctx->data = (char *)realloc(ctx->data, ctx->datamax);
        if(!ctx->data) {
            return ecStackOverflow;
        }
    }
    /* Some common characters aren't in TrueType font maps */
    if (ch == 147 || ch == 148)
        ch = '"';

    /* Convert character into UTF-8 */
    if (ch <= 0x7fUL) {
        ctx->data[ctx->datapos++] = ch;
    } else if (ch <= 0x7ffUL) {
        ctx->data[ctx->datapos++] = 0xc0 | (ch >> 6);
        ctx->data[ctx->datapos++] = 0x80 | (ch & 0x3f);
    } else if (ch <= 0xffffUL) {
        ctx->data[ctx->datapos++] = 0xe0 | (ch >> 12);
        ctx->data[ctx->datapos++] = 0x80 | ((ch >> 6) & 0x3f);
        ctx->data[ctx->datapos++] = 0x80 | (ch & 0x3f);
    } else {
        ctx->data[ctx->datapos++] = 0xf0 | (ch >> 18);
        ctx->data[ctx->datapos++] = 0x80 | ((ch >> 12) & 0x3f);
        ctx->data[ctx->datapos++] = 0x80 | ((ch >> 6) & 0x3f);
        ctx->data[ctx->datapos++] = 0x80 | (ch & 0x3f);
    }
    return ecOK;
}

/*
 * %%Function: ecProcessData
 *
 * Flush the output text
 */

int
ecProcessData(RTF_Context *ctx)
{
    int status = ecOK;
    if(ctx->datapos > 0) {
        ctx->data[ctx->datapos] = '\0';
        status = ecAddText(ctx, ctx->data);
        ctx->datapos = 0;
    }
    return status;
}

/*
 * %%Function: ecTabstop
 *
 * Flush the output text and add a tabstop
 */

int
ecTabstop(RTF_Context *ctx)
{
    int status;
    status = ecProcessData(ctx);
    if (status == ecOK)
        status = ecAddTab(ctx);
    return status;
}

/*
 * %%Function: ecLinebreak
 *
 * Flush the output text and move to the next line
 */

int
ecLinebreak(RTF_Context *ctx)
{
    int status;
    status = ecProcessData(ctx);
    if (status == ecOK)
        status = ecAddLine(ctx);
    return status;
}

/*
 * %%Function: ecParagraph
 *
 * Flush the output text and start a new paragraph
 */

int
ecParagraph(RTF_Context *ctx)
{
    return ecLinebreak(ctx);
}

/*
 * %%Function: ecReflowText
 *
 * Reflow the text to a new width
 */

/* FIXME: How does this need to be calculated? */
static int TwipsToPixels(int twips)
{
    return twips/20;    /* twips are 1/20 of a pointsize */
}

static RTF_Surface *CreateSurface(RTF_Context *ctx, RTF_TextBlock *textBlock, int offset, int numChars)
{
    RTF_Surface *surface = (RTF_Surface *)malloc(sizeof(*surface));
    if (surface) {
        char *text = &textBlock->text[textBlock->byteOffsets[offset]];
        char *end = &textBlock->text[textBlock->byteOffsets[offset+numChars]];
        char ch;
        ch = *end;
        *end = '\0';
        surface->surface = ctx->fontEngine.RenderText(textBlock->font, text, textBlock->color);
        *end = ch;
        if (!surface->surface)
        {
            free(surface);
            return(NULL);
        }
        surface->x = 0;
        surface->y = 0;
        surface->next = NULL;
    }
    return surface;
}

static int TextWithinWidth(RTF_TextBlock *textBlock, int offset, int width, int *wrapped)
{
    int num = 0;
    int wrapIndex;

    /* Fit as many characters as possible into the available width */
    *wrapped = 0;
    if (offset+num < textBlock->numChars)
    {
        while ((textBlock->pixelOffsets[offset+num+1] -
                textBlock->pixelOffsets[offset]) <= width )
        {
            ++num;
            if (offset+num == textBlock->numChars)
                return num;
        }

        /* Do word wrapping */
        for (wrapIndex = offset+num-1; wrapIndex > offset; --wrapIndex)
        {
            if (isspace(textBlock->text[textBlock->byteOffsets[wrapIndex]]))
                break;
        }
        if (wrapIndex > offset)
        {
            num = wrapIndex - offset + 1;
            *wrapped = 1;
        }
    }
    return num;
}

static int ReflowLine(RTF_Context *ctx, RTF_Line *line, int width)
{
    FreeLineSurfaces(line);
    if (line->start)
    {
        int leftMargin = TwipsToPixels(line->pap.xaLeft);
        int rightMargin = TwipsToPixels(line->pap.xaRight);
        int tabStop = TwipsToPixels(720);
        RTF_TextBlock *textBlock;
        RTF_Surface *surface;
        RTF_Surface *rowStart = NULL;
        int lineHeight = 0;
        int lineWidth = TwipsToPixels(line->pap.xaFirst);
        width -= leftMargin;
        width -= rightMargin;
        line->lineWidth = 0;
        line->lineHeight = 0;
        for (textBlock = line->start; textBlock; textBlock = textBlock->next)
        {
            int num, wrapped, numChars = 0;
            int tab;
            /* This is sort of a hack, but good enough for now */
            for (tab = 0; tab < textBlock->tabs; ++tab)
            {
                lineWidth /= tabStop;
                lineWidth += 1;
                lineWidth *= tabStop;
            }
            do {
                num = TextWithinWidth(textBlock, numChars, (width-lineWidth), &wrapped);
                if (num > 0)
                {
                    surface = CreateSurface(ctx, textBlock, numChars, num);
                    if (surface)
                    {
                        if (!rowStart)
                        {
                            rowStart = surface;
                        }
                        surface->x = leftMargin + lineWidth;
                        surface->y = line->lineHeight;
                        if (line->startSurface)
                            line->lastSurface->next = surface;
                        else
                            line->startSurface = surface;
                        line->lastSurface = surface;
                    }
                    if (lineHeight < textBlock->lineHeight)
                        lineHeight = textBlock->lineHeight;
                    lineWidth += (textBlock->pixelOffsets[numChars+num] -
                                  textBlock->pixelOffsets[numChars]);
                    numChars += num;
                }
                if (wrapped)
                {
                    if (lineWidth > line->lineWidth)
                        line->lineWidth = lineWidth;
                    line->lineHeight += lineHeight;

                    if (line->pap.just == justC)
                    {
                        int offset = (width - lineWidth) / 2;
                        while (rowStart)
                        {
                            rowStart->x += offset;
                            rowStart = rowStart->next;
                        }
                    }
                    else if (line->pap.just == justR)
                    {
                        int offset = (leftMargin + width - lineWidth);
                        while (rowStart)
                        {
                            rowStart->x += offset;
                            rowStart = rowStart->next;
                        }
                    }
                    rowStart = NULL;

                    lineWidth = 0;
                    lineHeight = 0;
                }
            } while (num > 0);
        }
        if (lineWidth > line->lineWidth) {
            line->lineWidth = lineWidth;
        }
        line->lineHeight += lineHeight;

        if (line->pap.just == justC)
        {
            int offset = (width - lineWidth) / 2;
            while (rowStart)
            {
                rowStart->x += offset;
                rowStart = rowStart->next;
            }
        }
        else if (line->pap.just == justR)
        {
            int offset = (leftMargin + width - lineWidth);
            while (rowStart)
            {
                rowStart->x += offset;
                rowStart = rowStart->next;
            }
        }
    }
    return line->lineHeight;
}

int
ecReflowText(RTF_Context *ctx, int width)
{
    RTF_Line *line;

    if (ctx->displayWidth == width)
        return ecOK;

    /* Reflow the text to the new width */
    ctx->displayWidth = width;
    ctx->displayHeight = 0;
    for (line = ctx->start; line; line = line->next) {
        ctx->displayHeight += ReflowLine(ctx, line, width);
    }
    return ecOK;
}

/*
 * %%Function: ecReflowText
 *
 * Render the text to a surface
 */

static void RenderLine(RTF_Line *line, SDL_Surface *target, const SDL_Rect *rect, int yOffset)
{
    SDL_Rect srcRect;
    SDL_Rect dstRect;
    RTF_Surface *surface;
    for (surface = line->startSurface; surface; surface = surface->next)
    {
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = surface->surface->w;
        srcRect.h = surface->surface->h;

        dstRect.x = rect->x + surface->x;
        dstRect.y = rect->y + yOffset + surface->y;
        dstRect.w = surface->surface->w;
        dstRect.h = surface->surface->h;

        SDL_BlitSurface(surface->surface, &srcRect, target, &dstRect);
    }
}

int
ecRenderText(RTF_Context *ctx, SDL_Surface *target, const SDL_Rect *rect, int yOffset)
{
    RTF_Line *line;
    SDL_Rect savedRect;

    ecReflowText(ctx, rect->w);

    SDL_GetClipRect(target, &savedRect);
    SDL_SetClipRect(target, rect);
    for (line = ctx->start; line && yOffset < rect->h; line = line->next) {
        if ( yOffset+line->lineHeight > 0 )
            RenderLine(line, target, rect, yOffset);
        yOffset += line->lineHeight;
    }
    SDL_SetClipRect(target, &savedRect);

    return ecOK;
}
