/* Common utility code for interacting with libXfont from test utilities
 *
 * Note that this code is designed for test programs, and thus uses assert()
 * and fatal err() calls in places that real code would do error handling,
 * since the goal is to catch bugs faster, not help users get past problems.
 */

/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on code from xorg-server/dix/dixfont.c covered by this notice:

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

 */

#include "font-test-utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <err.h>

static FPEFunctions *fpe_functions = NULL;
static int num_fpe_types = 0;

/* Callback from libXfont when each fpe handler is registered */
_X_EXPORT int
RegisterFPEFunctions(NameCheckFunc name_func,
		     InitFpeFunc init_func,
		     FreeFpeFunc free_func,
		     ResetFpeFunc reset_func,
		     OpenFontFunc open_func,
		     CloseFontFunc close_func,
		     ListFontsFunc list_func,
		     StartLfwiFunc start_lfwi_func,
		     NextLfwiFunc next_lfwi_func,
		     WakeupFpeFunc wakeup_func,
		     ClientDiedFunc client_died,
		     LoadGlyphsFunc load_glyphs,
		     StartLaFunc start_list_alias_func,
		     NextLaFunc next_list_alias_func,
		     SetPathFunc set_path_func)
{
    FPEFunctions *new;

    /* grow the list */
    new = realloc(fpe_functions, (num_fpe_types + 1) * sizeof(FPEFunctions));
    assert (new != NULL);
    fpe_functions = new;

    fpe_functions[num_fpe_types].name_check = name_func;
    fpe_functions[num_fpe_types].open_font = open_func;
    fpe_functions[num_fpe_types].close_font = close_func;
    fpe_functions[num_fpe_types].wakeup_fpe = wakeup_func;
    fpe_functions[num_fpe_types].list_fonts = list_func;
    fpe_functions[num_fpe_types].start_list_fonts_with_info = start_lfwi_func;
    fpe_functions[num_fpe_types].list_next_font_with_info = next_lfwi_func;
    fpe_functions[num_fpe_types].init_fpe = init_func;
    fpe_functions[num_fpe_types].free_fpe = free_func;
    fpe_functions[num_fpe_types].reset_fpe = reset_func;
    fpe_functions[num_fpe_types].client_died = client_died;
    fpe_functions[num_fpe_types].load_glyphs = load_glyphs;
    fpe_functions[num_fpe_types].start_list_fonts_and_aliases =
	start_list_alias_func;
    fpe_functions[num_fpe_types].list_next_font_or_alias = next_list_alias_func;
    fpe_functions[num_fpe_types].set_path_hook = set_path_func;

    return num_fpe_types++;
}

FPEFunctions *
init_font_handlers(int *fpe_function_count)
{
    register_fpe_functions();
    /* make sure our callbacks were called & worked */
    assert (fpe_functions != NULL);
    assert (num_fpe_types > 0);
    *fpe_function_count = num_fpe_types;
    return fpe_functions;
}


/* does the necessary magic to figure out the fpe type */
static int
DetermineFPEType(const char *pathname)
{
    int i;

    /* make sure init_font_handlers was called first */
    assert (num_fpe_types > 0);

    for (i = 0; i < num_fpe_types; i++) {
        if ((*fpe_functions[i].name_check) (pathname))
            return i;
    }
    return -1;
}


static const char * const default_fpes[] = {
    "catalogue:/etc/X11/fontpath.d",
    "built-ins"
};
#define num_default_fpes  (sizeof(default_fpes) / sizeof(*default_fpes))

FontPathElementPtr *
init_font_paths(const char * const *font_paths, int *num_fpes)
{
    FontPathElementPtr *fpe_list;
    int i;

    /* make sure init_font_handlers was called first */
    assert (num_fpe_types > 0);

    /* Use default if caller didn't supply any */
    if (*num_fpes == 0) {
	font_paths = default_fpes;
	*num_fpes = num_default_fpes;
    }

    fpe_list = calloc(*num_fpes, sizeof(FontPathElementPtr));
    assert(fpe_list != NULL);

    for (i = 0; i < *num_fpes; i++) {
	int result;
	FontPathElementPtr fpe = calloc(1, sizeof(FontPathElementRec));
	assert(fpe != NULL);

	fpe->name = strdup(font_paths[i]);
	assert(fpe->name != NULL);
	fpe->name_length = strlen(fpe->name);
	assert(fpe->name_length > 0);
	/* If path is to fonts.dir file, trim it off and use the full
	   directory path instead.  Simplifies testing with afl. */
	if (fpe->name_length > sizeof("/fonts.dir")) {
	    char *tail = fpe->name + fpe->name_length -
		(sizeof("/fonts.dir") - 1);

	    if (strcmp(tail, "/fonts.dir") == 0) {
		char *fullpath;

		*tail = '\0';
		fullpath = realpath(fpe->name, NULL);
		assert(fullpath != NULL);
		free(fpe->name);
		fpe->name = fullpath;
		fpe->name_length = strlen(fpe->name);
		assert(fpe->name_length > 0);
	    }
	}
	fpe->type = DetermineFPEType(fpe->name);
	if (fpe->type == -1)
	    err(BadFontPath, "Unable to find handler for font path %s",
		fpe->name);
	result = (*fpe_functions[fpe->type].init_fpe) (fpe);
	if (result != Successful)
	    err(result, "init_fpe failed for font path %s: error %d",
		fpe->name, result);

	printf("Initialized font path element #%d: %s\n", i, fpe->name);
	fpe_list[i] = fpe;
    }
    printf("\n");

    return fpe_list;
}
