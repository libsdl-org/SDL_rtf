/*
 * This file was adapted from Microsoft Rich Text Format Specification 1.6
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnrtfspec/html/rtfspec.asp
*/

/* RTF parser declarations */

int ecAddFontEntry(RTF_Context *ctx, int number, const char *name, int family);
void *ecLookupFont(RTF_Context *ctx);
int ecClearFonts(RTF_Context *ctx);
int ecAddColorEntry(RTF_Context *ctx, int r, int g, int b);
int ecLookupColor(RTF_Context *ctx, SDL_Color *color);
int ecClearColors(RTF_Context *ctx);
int ecAddLine(RTF_Context *ctx);
int ecAddTab(RTF_Context *ctx);
int ecAddText(RTF_Context *ctx, const char *text);
int ecClearLines(RTF_Context *ctx);
int ecClearContext(RTF_Context *ctx);

int ecRtfGetChar(RTF_Context *ctx, int *ch);
int ecRtfUngetChar(RTF_Context *ctx, int ch);
int ecRtfParse(RTF_Context *ctx);
int ecPushRtfState(RTF_Context *ctx);
int ecPopRtfState(RTF_Context *ctx);
int ecParseRtfKeyword(RTF_Context *ctx);
int ecParseChar(RTF_Context *ctx, int c);
int ecTranslateKeyword(RTF_Context *ctx, char *szKeyword, int param, bool fParam);
int ecPrintChar(RTF_Context *ctx, int ch);
int ecProcessData(RTF_Context *ctx);
int ecTabstop(RTF_Context *ctx);
int ecLinebreak(RTF_Context *ctx);
int ecParagraph(RTF_Context *ctx);

int ecReflowText(RTF_Context *ctx, int width);
int ecRenderText(RTF_Context *ctx, SDL_Surface *target, const SDL_Rect *rect, int yOffset);

int ecEndGroupAction(RTF_Context *ctx, RDS rds);
int ecApplyPropChange(RTF_Context *ctx, IPROP iprop, int val);
int ecChangeDest(RTF_Context *ctx, IDEST idest);
int ecParseSpecialKeyword(RTF_Context *ctx, IPFN ipfn);
int ecParseSpecialProperty(RTF_Context *ctx, IPROP iprop, int val);
int ecParseHexByte(RTF_Context *ctx);

/* RTF parser error codes */

#define ecOK 0                      /* Everything's fine! */
#define ecStackUnderflow    1       /* Unmatched '}' */
#define ecStackOverflow     2       /* Too many '{' -- memory exhausted */
#define ecUnmatchedBrace    3       /* RTF ended during an open group. */
#define ecInvalidHex        4       /* invalid hex character found in data */
#define ecBadTable          5       /* RTF table (sym or prop) invalid */
#define ecAssertion         6       /* Assertion failure */
#define ecEndOfFile         7       /* End of file reached while reading RTF */
#define ecFontNotFound      8       /* Couldn't find font for text */
