/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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
#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_NOTCURSES

#define DEBUG_DYNAMIC_NOTCURSES 0

#include "SDL_notcursesdyn.h"

#if DEBUG_DYNAMIC_NOTCURSES
#include <stdio.h>
#endif

#ifdef SDL_VIDEO_DRIVER_NOTCURSES_CORE_DYNAMIC

typedef struct
{
    SDL_SharedObject *lib;
    const char *libname;
} notcursesdynlib;

#ifndef SDL_VIDEO_DRIVER_NOTCURSES_CORE_DYNAMIC
#define SDL_VIDEO_DRIVER_NOTCURSES_CORE_DYNAMIC NULL
#endif

static notcursesdynlib notcurseslibs[] = {
    { NULL, SDL_VIDEO_DRIVER_NOTCURSES_CORE_DYNAMIC },
};

static void *NOTCURSES_GetSym(const char *fnname, int *pHasModule)
{
    int i;
    void *fn = NULL;
    for (i = 0; i < SDL_arraysize(notcurseslibs); i++) {
        if (notcurseslibs[i].lib) {
            fn = SDL_LoadFunction(notcurseslibs[i].lib, fnname);
            if (fn) {
                break;
            }
        }
    }

#if DEBUG_DYNAMIC_NOTCURSES
    if (fn) {
        printf("NOTCURSES: Found '%s' in %s (%p)\n", fnname, notcurseslibs[i].libname, fn);
    } else {
        printf("NOTCURSES: Symbol '%s' NOT FOUND!\n", fnname);
    }
#endif

    if (!fn) {
        *pHasModule = 0; // kill this module.
    }

    return fn;
}

#endif // SDL_VIDEO_DRIVER_NOTCURSES_DYNAMIC

// Define all the function pointers and wrappers...
#define SDL_NOTCURSES_SYM(rc, fn, params) SDL_DYNNOTCURSESFN_##fn NOTCURSES_##fn = NULL;
#include "SDL_notcursessym.h"

/* These SDL_NOTCURSES_HAVE_* flags are here whether you have dynamic notcurses or not. */
#define SDL_NOTCURSES_MODULE(modname) int SDL_NOTCURSES_HAVE_##modname = 0;
#include "SDL_notcursessym.h"

static int notcurses_load_refcount = 0;

void SDL_NOTCURSES_UnloadSymbols(void)
{
    // Don't actually unload if more than one module is using the libs...
    if (notcurses_load_refcount > 0) {
        if (--notcurses_load_refcount == 0) {
#ifdef SDL_VIDEO_DRIVER_NOTCURSES_CORE_DYNAMIC
            int i;
#endif

            // set all the function pointers to NULL.
#define SDL_NOTCURSES_MODULE(modname)                SDL_NOTCURSES_HAVE_##modname = 0;
#define SDL_NOTCURSES_SYM(rc, fn, params) NOTCURSES_##fn = NULL;
#include "SDL_notcursessym.h"

#ifdef SDL_VIDEO_DRIVER_NOTCURSES_CORE_DYNAMIC
            for (i = 0; i < SDL_arraysize(notcurseslibs); i++) {
                if (notcurseslibs[i].lib) {
                    SDL_UnloadObject(notcurseslibs[i].lib);
                    notcurseslibs[i].lib = NULL;
                }
            }
#endif
        }
    }
}

// returns non-zero if all needed symbols were loaded.
bool SDL_NOTCURSES_LoadSymbols(void)
{
    bool result = true; // always succeed if not using Dynamic notcurses stuff.

    // deal with multiple modules (dga, notcurses, etc) needing these symbols...
    if (notcurses_load_refcount++ == 0) {
#ifdef SDL_VIDEO_DRIVER_NOTCURSES_CORE_DYNAMIC
        int i;
        int *thismod = NULL;
        for (i = 0; i < SDL_arraysize(notcurseslibs); i++) {
            if (notcurseslibs[i].libname) {
                notcurseslibs[i].lib = SDL_LoadObject(notcurseslibs[i].libname);
            }
        }

#define SDL_NOTCURSES_MODULE(modname) SDL_NOTCURSES_HAVE_##modname = 1; // default yes
#include "SDL_notcursessym.h"

#define SDL_NOTCURSES_MODULE(modname)     thismod = &SDL_NOTCURSES_HAVE_##modname;
#define SDL_NOTCURSES_SYM(rc, fn, params) NOTCURSES_##fn = (SDL_DYNNOTCURSESFN_##fn)NOTCURSES_GetSym(#fn, thismod);
#include "SDL_notcursessym.h"

        if (SDL_NOTCURSES_HAVE_NOTCURSES_CORE) {
            // all required symbols loaded.
            SDL_ClearError();
        } else {
            // in case something got loaded...
            SDL_NOTCURSES_UnloadSymbols();
            result = false;
        }

#else // no dynamic notcurses

#define SDL_NOTCURSES_MODULE(modname)     SDL_NOTCURSES_HAVE_##modname = 1; // default yes
#define SDL_NOTCURSES_SYM(rc, fn, params) NOTCURSES_##fn = (SDL_DYNNOTCURSESFN_##fn)fn;
#include "SDL_notcursessym.h"

#endif
    }

    return result;
}

#endif // SDL_VIDEO_DRIVER_NOTCURSES
