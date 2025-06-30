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

#ifdef SDL_VIDEO_DRIVER_DOSVESA

#include "../SDL_sysvideo.h"
#include "SDL_dosvideo.h"
#include "SDL_dosframebuffer_c.h"
#include "SDL_dosmouse.h"
#include "../../events/SDL_mouse_c.h"
#include "../../SDL_properties_c.h"

// note that DOS_SURFACE's value is the same string that the dummy driver uses.
#define DOS_SURFACE "SDL.internal.window.surface"
#define DOS_LFB_SURFACE "SDL.internal.window.lfb_surface"

bool DOSVESA_CreateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window, SDL_PixelFormat *format, void **pixels, int *pitch)
{
    SDL_VideoData *data = device->internal;
    const SDL_DisplayMode *mode = &data->current_mode;
    const SDL_PixelFormat surface_format = mode->format;
    int w, h;

    // writing to video RAM shows up as the screen refreshes, done or not, and it might have a weird pitch, so give the app a buffer of system RAM.
    SDL_GetWindowSizeInPixels(window, &w, &h);
    SDL_Surface *surface = SDL_CreateSurface(w, h, surface_format);
    if (!surface) {
        return false;
    }

    // we make a surface that uses video memory directly, so SDL can optimize the blit for us.
    void *lfb_pixels = DOS_PhysicalToLinear(data->mapping.address);
    SDL_Surface *lfb_surface = SDL_CreateSurfaceFrom(mode->w, mode->h, surface_format, lfb_pixels, mode->internal->pitch);
    if (!lfb_surface) {
        SDL_DestroySurface(surface);
        return false;
    }

    // clear the framebuffer completely, in case another window at a larger size was using this before us.
    SDL_ClearSurface(lfb_surface, 0.0f, 0.0f, 0.0f, 0.0f);

    // Save the info and return!
    SDL_SetSurfaceProperty(SDL_GetWindowProperties(window), DOS_SURFACE, surface);
    SDL_SetSurfaceProperty(SDL_GetWindowProperties(window), DOS_LFB_SURFACE, lfb_surface);

    *format = surface_format;
    *pixels = surface->pixels;
    *pitch = surface->pitch;
    return true;
}

bool DOSVESA_SetWindowFramebufferVSync(SDL_VideoDevice *device, SDL_Window *window, int vsync)
{
    if (vsync < 0) {
        return SDL_SetError("Unsupported vsync type");
    }
    SDL_WindowData *data = window->internal;
    data->framebuffer_vsync = vsync;
    return true;
}

bool DOSVESA_GetWindowFramebufferVSync(SDL_VideoDevice *device, SDL_Window *window, int *vsync)
{
    if (vsync) {
        SDL_WindowData *data = window->internal;
        *vsync = data->framebuffer_vsync;
    }
    return true;
}

bool DOSVESA_UpdateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    SDL_WindowData *windata = window->internal;
    SDL_Surface *src = (SDL_Surface *) SDL_GetPointerProperty(SDL_GetWindowProperties(window), DOS_SURFACE, NULL);
    if (!src) {
        return SDL_SetError("Couldn't find DOS surface for window");
    }

    SDL_Surface *dst = (SDL_Surface *) SDL_GetPointerProperty(SDL_GetWindowProperties(window), DOS_LFB_SURFACE, NULL);
    if (!dst) {
        return SDL_SetError("Couldn't find VESA linear framebuffer surface for window");
    }

    const SDL_Rect dstrect = { (dst->w - src->w) / 2, (dst->h - src->h) / 2, src->w, src->h };
    SDL_Mouse *mouse = SDL_GetMouse();
    SDL_Surface *cursor = NULL;
    SDL_Rect cursorrect;

    if (mouse && mouse->internal && !mouse->relative_mode && mouse->cursor_visible && mouse->cur_cursor && mouse->cur_cursor->internal) {
        cursor = mouse->cur_cursor->internal->surface;
        if (cursor) {
            cursorrect.x = dstrect.x + SDL_clamp((int) mouse->x, 0, window->w);
            cursorrect.y = dstrect.y + SDL_clamp((int) mouse->y, 0, window->h);
        }
    }

    // wait for vsync if necessary...
    const int vsync_interval = windata->framebuffer_vsync;
    for (int i = 0; i < vsync_interval; i++) {
        while (!(inportb(0x3DA) & 0x08)) {
            SDL_CPUPauseInstruction();
        }
    }

    if (!SDL_BlitSurface(src, NULL, dst, &dstrect)) {
        return false;
    }

    if (cursor) {
        if (!SDL_BlitSurface(cursor, NULL, dst, &cursorrect)) {
            return false;
        }
    }

    return true;
}

void DOSVESA_DestroyWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window)
{
    SDL_Surface *lfb_surface = (SDL_Surface *) SDL_GetPointerProperty(SDL_GetWindowProperties(window), DOS_LFB_SURFACE, NULL);
    if (lfb_surface) {
        SDL_ClearSurface(lfb_surface, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    SDL_ClearProperty(SDL_GetWindowProperties(window), DOS_SURFACE);
    SDL_ClearProperty(SDL_GetWindowProperties(window), DOS_LFB_SURFACE);
}

#endif // SDL_VIDEO_DRIVER_DOSVESA

