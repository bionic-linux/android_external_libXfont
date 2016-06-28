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

static FontRenderersRec	renderers;

/*
 * XXX Maybe should allow unregistering renders. For now, just clear the
 * list at each new generation.
 */
static unsigned long rendererGeneration = 0;

Bool
FontFileRegisterRenderer (FontRendererPtr renderer)
{
    int		    i;
    struct _FontRenderersElement *new;

    if (rendererGeneration != __GetServerGeneration()) {
	rendererGeneration = __GetServerGeneration();
	renderers.number = 0;
	if (renderers.renderers)
	   free(renderers.renderers);
	renderers.renderers = NULL;
    }

    i = renderers.number;
    new = realloc (renderers.renderers, sizeof(*new) * (i + 1));
    if (!new)
	return FALSE;
    renderers.renderers = new;
    renderers.number = i + 1;

    renderers.renderers[i].renderer = renderer;
    return TRUE;
}

FontRendererPtr
FontFileMatchRenderer (char *fileName)
{
    int			i;
    int			fileLen, suffixLen;
    FontRendererPtr	r;

    fileLen = strlen (fileName);
    for (i = 0; i < renderers.number; i++)
    {
	r = renderers.renderers[i].renderer;
	suffixLen = strlen(r->fileSuffix);
	if (fileLen >= suffixLen &&
	    !strcasecmp (fileName + fileLen - suffixLen, r->fileSuffix))
	{
	    return r;
	}
    }
    return 0;
}
