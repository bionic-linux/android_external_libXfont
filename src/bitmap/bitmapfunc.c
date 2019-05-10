/*

Copyright 1991, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"

#include <X11/fonts/fntfilst.h>
#include <X11/fonts/bitmap.h>
#include <X11/fonts/fontutil.h>
#if XFONT_PCFFORMAT
#include <X11/fonts/pcf.h>
#endif

#if XFONT_PCFFORMAT
typedef struct _BitmapFileFunctions {
    int         (*ReadFont) (FontPtr /* pFont */, FontFilePtr /* file */,
			     int /* bit */, int /* byte */,
			     int /* glyph */, int /* scan */);
    int         (*ReadInfo) (  FontInfoPtr /* pFontInfo */,
			       FontFilePtr /* file */ );
}           BitmapFileFunctionsRec, *BitmapFileFunctionsPtr;

static int BitmapGetRenderIndex(FontRendererPtr renderer);

/*
 * the readers[] and renderers[] arrays must be in the same order,
 * and also in the same order as scale[] and find_scale[] in bitscale.c
 *
 */
static BitmapFileFunctionsRec readers[] = {
#if XFONT_PCFFORMAT
    { pcfReadFont, pcfReadFontInfo} ,
#endif
};


#define CAPABILITIES (CAP_MATRIX | CAP_CHARSUBSETTING)

static int
BitmapOpenBitmap (FontPathElementPtr fpe, FontPtr *ppFont, int flags,
		  FontEntryPtr entry, char *fileName,
		  fsBitmapFormat format, fsBitmapFormatMask fmask,
		  FontPtr non_cachable_font) /* We don't do licensing */
{
    FontFilePtr	file;
    FontPtr     pFont;
    int         i;
    int         ret;
    int         bit,
                byte,
                glyph,
                scan,
		image;

    i = BitmapGetRenderIndex(entry->u.bitmap.renderer);
    file = FontFileOpen (fileName);
    if (!file)
	return BadFontName;
    if (!(pFont = CreateFontRec())) {
	fprintf(stderr, "Error: Couldn't allocate pFont (%ld)\n",
		(unsigned long)sizeof(FontRec));
	FontFileClose (file);
	return AllocError;
    }
    /* set up default values */
    FontDefaultFormat(&bit, &byte, &glyph, &scan);
    /* get any changes made from above */
    ret = CheckFSFormat(format, fmask, &bit, &byte, &scan, &glyph, &image);

    /* Fill in font record. Data format filled in by reader. */
    pFont->refcnt = 0;

    ret = (*readers[i].ReadFont) (pFont, file, bit, byte, glyph, scan);

    FontFileClose (file);
    if (ret != Successful) {
	free(pFont);
    } else {
	*ppFont = pFont;
    }
    return ret;
}

static int
BitmapGetInfoBitmap (FontPathElementPtr fpe, FontInfoPtr pFontInfo,
		     FontEntryPtr entry, char *fileName)
{
    FontFilePtr file;
    int		i;
    int		ret;
    FontRendererPtr renderer;

    renderer = FontFileMatchRenderer (fileName);
    if (!renderer)
	return BadFontName;
    i = BitmapGetRenderIndex(renderer);
    file = FontFileOpen (fileName);
    if (!file)
	return BadFontName;
    ret = (*readers[i].ReadInfo) (pFontInfo, file);
    FontFileClose (file);
    return ret;
}

static FontRendererRec	renderers[] = {
#if XFONT_PCFFORMAT
    { ".pcf", 4, BitmapOpenBitmap, BitmapOpenScalable,
	BitmapGetInfoBitmap, BitmapGetInfoScalable, 0,
	CAPABILITIES },
#endif
};

#define numRenderers	(sizeof renderers / sizeof renderers[0])

void
BitmapRegisterFontFileFunctions (void)
{
    int	    i;

    for (i = 0; i < numRenderers; i++)
	FontFileRegisterRenderer (&renderers[i]);
}

/*
 * compute offset into renderers array - used to find the font reader,
 * the font info reader, and the bitmap scaling routine.  All users
 * of this routine must be kept in step with the renderer array.
 */
static int
BitmapGetRenderIndex(FontRendererPtr renderer)
{
    return renderer - renderers;
}

#else
void
BitmapRegisterFontFileFunctions (void)
{
    /* nothing to do */
}
#endif /* XFONT_PCFFORMAT */
