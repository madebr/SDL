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

#include "../SDL_sysvideo.h"
#include "../../SDL_properties_c.h"
#include "SDL_notcursesdyn.h"
#include "SDL_notcursesvideo.h"
#include "SDL_notcursesframebuffer_c.h"

//#include "SDL_notcurseswindow.h"

#define NOTCURSES_SURFACE "SDL.internal.window.surface"

bool NOTCURSES_CreateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window, Uint32 *format, void **pixels, int *pitch)
{
    SDL_Surface *surface;
    const SDL_PixelFormat surface_format = SDL_PIXELFORMAT_ABGR8888;
    int w, h;

    // Create a new framebuffer
    SDL_GetWindowSizeInPixels(window, &w, &h);
    surface = SDL_CreateSurface(w, h, surface_format);
    if (!surface) {
        return false;
    }

    // Save the info and return!
    SDL_SetSurfaceProperty(SDL_GetWindowProperties(window), NOTCURSES_SURFACE, surface);
    *format = surface_format;
    *pixels = surface->pixels;
    *pitch = surface->pitch;
    return true;
}

bool NOTCURSES_UpdateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    SDL_VideoData *viddata = device->internal;
    struct ncvisual_options visual_options;
    SDL_Surface *surface;

    surface = (SDL_Surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), NOTCURSES_SURFACE, NULL);
    if (!surface) {
        return SDL_SetError("Couldn't find dummy surface for window");
    }

    SDL_zero(visual_options);
    visual_options.n = viddata->stdplane;
    visual_options.scaling = NCSCALE_SCALE_HIRES;
    visual_options.lenx = surface->w;
    visual_options.leny = surface->h;
    if (NOTCURSES_ncblit_rgba(surface->pixels, surface->w * 4, &visual_options) == -1) {
        return SDL_SetError("ncblit_rgba");
    }
    if (NOTCURSES_ncpile_render(viddata->stdplane)) {
        return SDL_SetError("ncpile_render");
    }
    if (NOTCURSES_ncpile_rasterize(viddata->stdplane)){
        return SDL_SetError("ncpile_rasterize");
    }

    return true;
}

void NOTCURSES_DestroyWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window)
{
    SDL_ClearProperty(SDL_GetWindowProperties(window), NOTCURSES_SURFACE);
}

#endif /* SDL_VIDEO_DRIVER_NOTCURSES */
