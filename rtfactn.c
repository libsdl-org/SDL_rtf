/*
 * This file was adapted from Microsoft Rich Text Format Specification 1.6
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnrtfspec/html/rtfspec.asp
 */

#include "SDL.h"

#include "rtftype.h"
#include "rtfdecl.h"

/* RTF parser tables */

/* Property descriptions */
PROP rgprop[ipropMax] =
{
    {actnSpec, propChp, 0},     /* ipropFontFamily */
    {actnWord, propChp, offsetof(CHP, fFontCharset)},   /* ipropFontCharset */
    {actnSpec, propChp, 0},     /* ipropColorRed */
    {actnSpec, propChp, 0},     /* ipropColorGreen */
    {actnSpec, propChp, 0},     /* ipropColorBlue */
    {actnWord, propChp, offsetof(CHP, fFont)},      /* ipropFont */
    {actnWord, propChp, offsetof(CHP, fFontSize)},  /* ipropFontSize */
    {actnByte, propChp, offsetof(CHP, fBgColor)},   /* ipropBgColor */
    {actnByte, propChp, offsetof(CHP, fFgColor)},   /* ipropFgColor */
    {actnByte, propChp, offsetof(CHP, fBold)},      /* ipropBold */
    {actnByte, propChp, offsetof(CHP, fItalic)},    /* ipropItalic */
    {actnByte, propChp, offsetof(CHP, fUnderline)}, /* ipropUnderline */
    {actnWord, propPap, offsetof(PAP, xaLeft)},     /* ipropLeftInd */
    {actnWord, propPap, offsetof(PAP, xaRight)},    /* ipropRightInd */
    {actnWord, propPap, offsetof(PAP, xaFirst)},    /* ipropFirstInd */
    {actnWord, propSep, offsetof(SEP, cCols)},      /* ipropCols */
    {actnWord, propSep, offsetof(SEP, xaPgn)},      /* ipropPgnX */
    {actnWord, propSep, offsetof(SEP, yaPgn)},      /* ipropPgnY */
    {actnWord, propDop, offsetof(DOP, xaPage)},     /* ipropXaPage */
    {actnWord, propDop, offsetof(DOP, yaPage)},     /* ipropYaPage */
    {actnWord, propDop, offsetof(DOP, xaLeft)},     /* ipropXaLeft */
    {actnWord, propDop, offsetof(DOP, xaRight)},    /* ipropXaRight */
    {actnWord, propDop, offsetof(DOP, yaTop)},      /* ipropYaTop */
    {actnWord, propDop, offsetof(DOP, yaBottom)},   /* ipropYaBottom */
    {actnWord, propDop, offsetof(DOP, pgnStart)},   /* ipropPgnStart */
    {actnByte, propSep, offsetof(SEP, sbk)},        /* ipropSbk */
    {actnByte, propSep, offsetof(SEP, pgnFormat)},  /* ipropPgnFormat */
    {actnByte, propDop, offsetof(DOP, fFacingp)},   /* ipropFacingp */
    {actnByte, propDop, offsetof(DOP, fLandscape)}, /* ipropLandscape */
    {actnByte, propPap, offsetof(PAP, just)},       /* ipropJust */
    {actnSpec, propPap, 0},                         /* ipropPard */
    {actnSpec, propChp, 0},                         /* ipropPlain */
    {actnSpec, propSep, 0},                         /* ipropSectd */
};

/* Keyword descriptions */
SYM rgsymRtf[] =
{
    /* keyword, dflt, fPassDflt, kwd, idx */
    {"fonttbl", 0, false, kwdDest, idestFontTable},
    {"fnil", fnil, true, kwdProp, ipropFontFamily},
    {"froman", froman, true, kwdProp, ipropFontFamily},
    {"fswiss", fswiss, true, kwdProp, ipropFontFamily},
    {"fmodern", fmodern, true, kwdProp, ipropFontFamily},
    {"fscript", fscript, true, kwdProp, ipropFontFamily},
    {"fdecor", fdecor, true, kwdProp, ipropFontFamily},
    {"ftech", ftech, true, kwdProp, ipropFontFamily},
    {"fbidi", fbidi, true, kwdProp, ipropFontFamily},
    {"fcharset", 0, false, kwdProp, ipropFontCharset},
    {"colortbl", 0, false, kwdDest, idestColorTable},
    {"red", 0, false, kwdProp, ipropColorRed},
    {"green", 0, false, kwdProp, ipropColorGreen},
    {"blue", 0, false, kwdProp, ipropColorBlue},
    {"info", 0, false, kwdDest, idestInfo},
    {"title", 0, false, kwdDest, idestTitle},
    {"subject", 0, false, kwdDest, idestSubject},
    {"author", 0, false, kwdDest, idestAuthor},
    {"f", 0, false, kwdProp, ipropFont},
    {"fs", 24, false, kwdProp, ipropFontSize},
    {"cb", 1, false, kwdProp, ipropBgColor},
    {"cf", 1, false, kwdProp, ipropFgColor},
    {"b", 1, false, kwdProp, ipropBold},
    {"ul", 1, false, kwdProp, ipropUnderline},
    {"ulnone", 0, true, kwdProp, ipropUnderline},
    {"i", 1, false, kwdProp, ipropItalic},
    {"li", 0, false, kwdProp, ipropLeftInd},
    {"ri", 0, false, kwdProp, ipropRightInd},
    {"fi", 0, false, kwdProp, ipropFirstInd},
    {"cols", 1, false, kwdProp, ipropCols},
    {"sbknone", sbkNon, true, kwdProp, ipropSbk},
    {"sbkcol", sbkCol, true, kwdProp, ipropSbk},
    {"sbkeven", sbkEvn, true, kwdProp, ipropSbk},
    {"sbkodd", sbkOdd, true, kwdProp, ipropSbk},
    {"sbkpage", sbkPg, true, kwdProp, ipropSbk},
    {"pgnx", 0, false, kwdProp, ipropPgnX},
    {"pgny", 0, false, kwdProp, ipropPgnY},
    {"pgndec", pgDec, true, kwdProp, ipropPgnFormat},
    {"pgnucrm", pgURom, true, kwdProp, ipropPgnFormat},
    {"pgnlcrm", pgLRom, true, kwdProp, ipropPgnFormat},
    {"pgnucltr", pgULtr, true, kwdProp, ipropPgnFormat},
    {"pgnlcltr", pgLLtr, true, kwdProp, ipropPgnFormat},
    {"qc", justC, true, kwdProp, ipropJust},
    {"ql", justL, true, kwdProp, ipropJust},
    {"qr", justR, true, kwdProp, ipropJust},
    {"qj", justF, true, kwdProp, ipropJust},
    {"paperw", 12240, false, kwdProp, ipropXaPage},
    {"paperh", 15480, false, kwdProp, ipropYaPage},
    {"margl", 1800, false, kwdProp, ipropXaLeft},
    {"margr", 1800, false, kwdProp, ipropXaRight},
    {"margt", 1440, false, kwdProp, ipropYaTop},
    {"margb", 1440, false, kwdProp, ipropYaBottom},
    {"pgnstart", 1, true, kwdProp, ipropPgnStart},
    {"facingp", 1, true, kwdProp, ipropFacingp},
    {"landscape", 1, true, kwdProp, ipropLandscape},
    {"line", 0, false, kwdChar, '\n'},
    {"par", 0, false, kwdChar, '\n'},
    {"\0x0a", 0, false, kwdChar, '\n'},
    {"\r", 0, false, kwdChar, '\n'},
    {"\0x0d", 0, false, kwdChar, '\n'},
    {"\n", 0, false, kwdChar, '\n'},
    {"tab", 0, false, kwdChar, '\t'},
    {"ldblquote", 0, false, kwdChar, '"'},
    {"rdblquote", 0, false, kwdChar, '"'},
    {"bin", 0, false, kwdSpec, ipfnBin},
    {"*", 0, false, kwdSpec, ipfnSkipDest},
    {"'", 0, false, kwdSpec, ipfnHex},
    {"bkmkend", 0, false, kwdDest, idestSkip},
    {"bkmkstart", 0, false, kwdDest, idestSkip},
    {"buptim", 0, false, kwdDest, idestSkip},
    {"colortbl", 0, false, kwdDest, idestSkip},
    {"comment", 0, false, kwdDest, idestSkip},
    {"creatim", 0, false, kwdDest, idestSkip},
    {"doccomm", 0, false, kwdDest, idestSkip},
    {"fonttbl", 0, false, kwdDest, idestSkip},
    {"footer", 0, false, kwdDest, idestSkip},
    {"footerf", 0, false, kwdDest, idestSkip},
    {"footerl", 0, false, kwdDest, idestSkip},
    {"footerr", 0, false, kwdDest, idestSkip},
    {"footnote", 0, false, kwdDest, idestSkip},
    {"ftncn", 0, false, kwdDest, idestSkip},
    {"ftnsep", 0, false, kwdDest, idestSkip},
    {"ftnsepc", 0, false, kwdDest, idestSkip},
    {"header", 0, false, kwdDest, idestSkip},
    {"headerf", 0, false, kwdDest, idestSkip},
    {"headerl", 0, false, kwdDest, idestSkip},
    {"headerr", 0, false, kwdDest, idestSkip},
    {"info", 0, false, kwdDest, idestSkip},
    {"keywords", 0, false, kwdDest, idestSkip},
    {"operator", 0, false, kwdDest, idestSkip},
    {"pict", 0, false, kwdDest, idestSkip},
    {"printim", 0, false, kwdDest, idestSkip},
    {"private1", 0, false, kwdDest, idestSkip},
    {"revtim", 0, false, kwdDest, idestSkip},
    {"rxe", 0, false, kwdDest, idestSkip},
    {"stylesheet", 0, false, kwdDest, idestSkip},
    {"tc", 0, false, kwdDest, idestSkip},
    {"txe", 0, false, kwdDest, idestSkip},
    {"xe", 0, false, kwdDest, idestSkip},
    {"{", 0, false, kwdChar, '{'},
    {"}", 0, false, kwdChar, '}'},
    {"\\", 0, false, kwdChar, '\\'},
    {"pard", 0, false, kwdProp, ipropPard},
    {"plain", 0, false, kwdProp, ipropPlain},
    {"sectd", 0, false, kwdProp, ipropSectd}
};
int isymMax = sizeof(rgsymRtf) / sizeof(SYM);

/*
 * %%Function: ecApplyPropChange
 *
 * Set the property identified by _iprop_ to the value _val_.
 *
 */
int ecApplyPropChange(RTF_Context *ctx, IPROP iprop, int val)
{
    char *pb;

    if (ctx->rds == rdsSkip)    /* If we're skipping text, */
        return ecOK;            /* don't do anything. */

    switch (rgprop[iprop].prop)
    {
        case propDop:
            pb = (char *) &ctx->dop;
            break;
        case propSep:
            pb = (char *) &ctx->sep;
            break;
        case propPap:
            pb = (char *) &ctx->pap;
            break;
        case propChp:
            pb = (char *) &ctx->chp;
            break;
        default:
            pb = NULL;
            if (rgprop[iprop].actn != actnSpec)
                return ecBadTable;
            break;
    }
    switch (rgprop[iprop].actn)
    {
        case actnByte:
            pb[rgprop[iprop].offset] = (unsigned char) val;
            break;
        case actnWord:
            (*(int *) (pb + rgprop[iprop].offset)) = val;
            break;
        case actnSpec:
            return ecParseSpecialProperty(ctx, iprop, val);
            break;
        default:
            return ecBadTable;
    }
    return ecOK;
}

/*
 * %%Function: ecParseSpecialProperty
 *
 * Set a property that requires code to evaluate.
 */
int ecParseSpecialProperty(RTF_Context *ctx, IPROP iprop, int val)
{
    switch (iprop)
    {
        case ipropFontFamily:
            ctx->values[0] = val;
            return ecOK;
        case ipropColorRed:
            ctx->values[0] = val;
            return ecOK;
        case ipropColorGreen:
            ctx->values[1] = val;
            return ecOK;
        case ipropColorBlue:
            ctx->values[2] = val;
            return ecOK;
        case ipropPard:
            SDL_memset(&ctx->pap, 0, sizeof(ctx->pap));
            return ecOK;
        case ipropPlain:
            SDL_memset(&ctx->chp, 0, sizeof(ctx->chp));
            return ecOK;
        case ipropSectd:
            SDL_memset(&ctx->sep, 0, sizeof(ctx->sep));
            return ecOK;
        default:
            return ecBadTable;
    }
    return ecBadTable;
}

/*
 * %%Function: ecTranslateKeyword.
 *
 * Step 3.
 * Search rgsymRtf for szKeyword and evaluate it appropriately.
 *
 * Inputs:
 * szKeyword:   The RTF control to evaluate.
 * param:       The parameter of the RTF control.
 * fParam:      true if the control had a parameter; (that is, if param
 *                    is valid)
 *              false if it did not.
 */
int ecTranslateKeyword(RTF_Context *ctx, char *szKeyword, int param,
        bool fParam)
{
    int isym;

    /* search for szKeyword in rgsymRtf */
    for (isym = 0; isym < isymMax; isym++)
        if (SDL_strcmp(szKeyword, rgsymRtf[isym].szKeyword) == 0)
            break;
    if (isym == isymMax)        /* control word not found */
    {
#ifdef DEBUG_RTF
        fprintf(stderr, "Ignoring unknown keyword: %s\n", szKeyword);
#endif
        if (ctx->fSkipDestIfUnk)     /* if this is a new destination */
            ctx->rds = rdsSkip; /* skip the destination */
        /* else just discard it */
        ctx->fSkipDestIfUnk = false;
        return ecOK;
    }

    /* found it!  use kwd and idx to determine what to do with it. */
    ctx->fSkipDestIfUnk = false;
    switch (rgsymRtf[isym].kwd)
    {
        case kwdProp:
            if (rgsymRtf[isym].fPassDflt || !fParam)
                param = rgsymRtf[isym].dflt;
            return ecApplyPropChange(ctx, rgsymRtf[isym].idx, param);
        case kwdChar:
            return ecParseChar(ctx, rgsymRtf[isym].idx);
        case kwdDest:
            return ecChangeDest(ctx, rgsymRtf[isym].idx);
        case kwdSpec:
            return ecParseSpecialKeyword(ctx, rgsymRtf[isym].idx);
        default:
            return ecBadTable;
    }
    return ecBadTable;
}

/*
 * %%Function: ecChangeDest
 *
 * Change to the destination specified by idest.
 * There's usually more to do here than this...
 */
int ecChangeDest(RTF_Context *ctx, IDEST idest)
{
    if (ctx->rds == rdsSkip)    /* if we're skipping text, */
        return ecOK;            /* don't do anything */

    switch (idest)
    {
        case idestFontTable:
            ctx->rds = rdsFontTable;
            ctx->datapos = 0;
            break;
        case idestColorTable:
            ctx->rds = rdsColorTable;
            ctx->values[0] = 0;
            ctx->values[1] = 0;
            ctx->values[2] = 0;
            break;
        case idestInfo:
            ctx->rds = rdsInfo;
            ctx->datapos = 0;
            break;
        case idestTitle:
            ctx->rds = rdsTitle;
            ctx->datapos = 0;
            break;
        case idestSubject:
            ctx->rds = rdsSubject;
            ctx->datapos = 0;
            break;
        case idestAuthor:
            ctx->rds = rdsAuthor;
            ctx->datapos = 0;
            break;
        default:
            ctx->rds = rdsSkip; /* when in doubt, skip it... */
            break;
    }
    return ecOK;
}

/*
 * %%Function: ecEndGroupAction
 *
 * The destination specified by rds is coming to a close.
 * If there's any cleanup that needs to be done, do it now.
 */
int ecEndGroupAction(RTF_Context *ctx, RDS rds)
{
    switch (ctx->rds)
    {
        case rdsTitle:
            SDL_free(ctx->title);
            ctx->data[ctx->datapos] = '\0';
            ctx->title = *ctx->data ? SDL_strdup(ctx->data) : NULL;
            ctx->datapos = 0;
            break;
        case rdsSubject:
            SDL_free(ctx->subject);
            ctx->data[ctx->datapos] = '\0';
            ctx->subject = *ctx->data ? SDL_strdup(ctx->data) : NULL;
            ctx->datapos = 0;
            break;
        case rdsAuthor:
            SDL_free(ctx->author);
            ctx->data[ctx->datapos] = '\0';
            ctx->author = *ctx->data ? SDL_strdup(ctx->data) : NULL;
            ctx->datapos = 0;
            break;
        default:
            break;
    }
    return ecOK;
}

/*
 * %%Function: ecParseSpecialKeyword
 *
 * Evaluate an RTF control that needs special processing.
 */
int ecParseSpecialKeyword(RTF_Context *ctx, IPFN ipfn)
{
    if (ctx->rds == rdsSkip && ipfn != ipfnBin) /* if we're skipping, and it's not */
        return ecOK;            /* the \bin keyword, ignore it. */
    switch (ipfn)
    {
        case ipfnBin:
            ctx->ris = risBin;
            ctx->cbBin = ctx->lParam;
            break;
        case ipfnSkipDest:
            ctx->fSkipDestIfUnk = true;
            break;
        case ipfnHex:
            ctx->ris = risHex;
            break;
        default:
            return ecBadTable;
    }
    return ecOK;
}

/* vi: set ts=4 sw=4 expandtab: */
