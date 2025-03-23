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


#ifdef SDL_INPUT_LINUXEV
#include "../../core/linux/SDL_evdev.h"
#endif

#include "SDL_notcursesdyn.h"
#include "SDL_notcursesevents.h"
#include "SDL_notcursesframebuffer_c.h"
//#include "SDL_notcursesmouse.h"  // FIXME!
#include "SDL_notcursesvideo.h"

#define DEBUG_NOTCURSES 0

static void NOTCURSES_DeleteDevice(SDL_VideoDevice *device)
{
    if (device->internal) {
        SDL_free(device->internal);
        device->internal = NULL;
    }

    SDL_free(device);

    SDL_NOTCURSES_UnloadSymbols();
}

//API void ncplane_set_resizecb(struct ncplane* n, int(*resizecb)(struct ncplane*));
//
//// Returns the ncplane's current resize callback.
//int stdplane_resize_callback(struct ncplane *n)
//{
//
//}

static SDL_VideoDevice *NOTCURSES_CreateDevice(void)
{
    SDL_VideoDevice *device = NULL;
    SDL_VideoData *viddata = NULL;
    struct notcurses_options nc_options;

    if (!SDL_NOTCURSES_LoadSymbols()) {
        return NULL;
    }

    device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        return NULL;
    }

    viddata = (SDL_VideoData *)SDL_calloc(1, sizeof(SDL_VideoData));
    if (!viddata) {
        goto cleanup;
    }
    device->internal = viddata;

    SDL_zero(nc_options);
#if DEBUG_NOTCURSES
    nc_options.loglevel = NCLOGLEVEL_DEBUG;
#endif
    nc_options.flags = NCOPTION_SUPPRESS_BANNERS;
    viddata->nc = NOTCURSES_notcurses_core_init(&nc_options, NULL);
    if (!viddata->nc) {
        goto cleanup;
    }

    viddata->stdplane = NOTCURSES_notcurses_stdplane(viddata->nc);
    if (!viddata->stdplane) {
        goto cleanup;
    }

    if (NOTCURSES_notcurses_mice_enable(viddata->nc, NCMICE_ALL_EVENTS) == -1) {
        // FIXME: log failure
    }

    // Setup all functions which we can handle
    device->VideoInit = NOTCURSES_VideoInit;
    device->VideoQuit = NOTCURSES_VideoQuit;
    device->GetDisplayModes = NOTCURSES_GetDisplayModes;
    device->SetDisplayMode = NOTCURSES_SetDisplayMode;
    device->CreateSDLWindow = NOTCURSES_CreateWindow;
    device->DestroyWindow = NOTCURSES_DestroyWindow;

    device->CreateWindowFramebuffer = NOTCURSES_CreateWindowFramebuffer;
    device->DestroyWindowFramebuffer = NOTCURSES_DestroyWindowFramebuffer;
    device->UpdateWindowFramebuffer = NOTCURSES_UpdateWindowFramebuffer;
//    device->SetWindowTitle = NOTCURSES_SetWindowTitle;
//    device->SetWindowPosition = NOTCURSES_SetWindowPosition;
//    device->SetWindowSize = NOTCURSES_SetWindowSize;
//    device->SetWindowFullscreen = NOTCURSES_SetWindowFullscreen;
//    device->ShowWindow = NOTCURSES_ShowWindow;
//    device->HideWindow = NOTCURSES_HideWindow;
//    device->RaiseWindow = NOTCURSES_RaiseWindow;
//    device->MaximizeWindow = NOTCURSES_MaximizeWindow;
//    device->MinimizeWindow = NOTCURSES_MinimizeWindow;
//    device->RestoreWindow = NOTCURSES_RestoreWindow;

#if 0
    // FIXME: is this even possible? (off-screen rendering?)
    device->GL_LoadLibrary = NOTCURSES_GLES_LoadLibrary;
    device->GL_GetProcAddress = NOTCURSES_GLES_GetProcAddress;
    device->GL_UnloadLibrary = NOTCURSES_GLES_UnloadLibrary;
    device->GL_CreateContext = NOTCURSES_GLES_CreateContext;
    device->GL_MakeCurrent = NOTCURSES_GLES_MakeCurrent;
    device->GL_SetSwapInterval = NOTCURSES_GLES_SetSwapInterval;
    device->GL_GetSwapInterval = NOTCURSES_GLES_GetSwapInterval;
    device->GL_SwapWindow = NOTCURSES_GLES_SwapWindow;
    device->GL_DestroyContext = NOTCURSES_GLES_DestroyContext;
    device->GL_DefaultProfileConfig = NOTCURSES_GLES_DefaultProfileConfig;
#endif

#if 0
    // FIXME: is this even possible? (off-screen rendering?)
#ifdef SDL_VIDEO_VULKAN
    device->Vulkan_LoadLibrary = NOTCURSES_Vulkan_LoadLibrary;
    device->Vulkan_UnloadLibrary = NOTCURSES_Vulkan_UnloadLibrary;
    device->Vulkan_GetInstanceExtensions = NOTCURSES_Vulkan_GetInstanceExtensions;
    device->Vulkan_CreateSurface = NOTCURSES_Vulkan_CreateSurface;
    device->Vulkan_DestroySurface = NOTCURSES_Vulkan_DestroySurface;
#endif
#endif

    device->PumpEvents = NOTCURSES_PumpEvents;
    device->free = NOTCURSES_DeleteDevice;

#ifdef SDL_INPUT_LINUXEV
//    SDL_EVDEV_Init();
#endif

    return device;

cleanup:
    if (device) {
        SDL_free(device);
    }
    if (viddata) {
        if (viddata->nc) {
            NOTCURSES_notcurses_stop(viddata->nc);
        }
        SDL_free(viddata);
    }
    return NULL;
}

VideoBootStrap NOTCURSES_bootstrap = {
    "notcurses",
    "Notcurses Video Driver",
    NOTCURSES_CreateDevice,
    NULL, // FIXME: no ShowMessageBox implementation
    false
};

bool NOTCURSES_VideoInit(SDL_VideoDevice *_this)
{
    SDL_VideoData *viddata = _this->internal;
    SDL_DisplayMode display_mode;
    bool result = true;
    unsigned int w, h;

    SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "NOTCURSES_VideoInit()");

    // FIXME: width/height depends on size of terminal
    NOTCURSES_ncplane_dim_yx(viddata->stdplane, &h, &w);
    SDL_zero(display_mode);
    display_mode.w = (int)w;
    display_mode.h = (int)h;
    display_mode.format = SDL_PIXELFORMAT_RGBA8888;
    if (SDL_AddBasicVideoDisplay(&display_mode) == 0) {
        return false;
    }

    return result;
}

/* The internal pointers, like dispdata, viddata, windata, etc...
   are freed by SDL internals, so not our job. */
void NOTCURSES_VideoQuit(SDL_VideoDevice *device)
{
    struct SDL_VideoData *viddata = device->internal;

    if (viddata->nc) {
        NOTCURSES_notcurses_stop(device->internal->nc);
        device->internal->nc = NULL;
    }
#ifdef SDL_INPUT_LINUXEV
//    SDL_EVDEV_Quit();
#endif
//    SDL_VideoData *viddata = _this->internal;

    // FIXME: implement
}

// Read modes from the connector modes, and store them in display->display_modes.
bool NOTCURSES_GetDisplayModes(SDL_VideoDevice *_this, SDL_VideoDisplay *display)
{
    SDL_VideoData *viddata = _this->internal;
    SDL_DisplayMode mode = { 0 };
    unsigned int w, h;

    NOTCURSES_ncplane_dim_yx(viddata->stdplane, &w, &h);
    mode.w = w;
    mode.h = h;
    mode.format = SDL_PIXELFORMAT_RGB24;

    if (!SDL_AddFullscreenDisplayMode(display, &mode)) {
        return false;
    }

    return true;
}

bool NOTCURSES_SetDisplayMode(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    return true;
}

bool NOTCURSES_CreateWindow(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID create_props)
{
//    SDL_WindowData *windata = NULL;
//    SDL_VideoData *viddata = _this->internal;
//    SDL_VideoDisplay *display = SDL_GetVideoDisplayForWindow(window);
//    SDL_DisplayData *dispdata = display->internal;
    bool result = true;

    // FIXME: implement

    return result;
}

void NOTCURSES_DestroyWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *windata = window->internal;
    //    SDL_DisplayData *dispdata = SDL_GetDisplayDriverDataForWindow(window);
    //    SDL_VideoData *viddata;

    if (!windata) {
        return;
    }

    // FIXME: implement

    /*********************************************************************/
    // Free the window internal. Bye bye, surface and buffer pointers!
    /*********************************************************************/
    SDL_free(window->internal);
    window->internal = NULL;
}

//void NOTCURSES_SetWindowTitle(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//}
//
//bool NOTCURSES_SetWindowPosition(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    return SDL_Unsupported();
//}
//
//void NOTCURSES_SetWindowSize(SDL_VideoDevice *_this, SDL_Window *window)
//{
////    SDL_VideoData *viddata = _this->internal;
//    // FIXME: implement
//}
//
//SDL_FullscreenResult NOTCURSES_SetWindowFullscreen(SDL_VideoDevice *_this, SDL_Window *window, SDL_VideoDisplay *display, SDL_FullscreenOp fullscreen)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//    return SDL_FULLSCREEN_FAILED;
//}
//
//void NOTCURSES_ShowWindow(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//}
//
//void NOTCURSES_HideWindow(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//}
//
//void NOTCURSES_RaiseWindow(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//}
//
//void NOTCURSES_MaximizeWindow(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//}
//
//void NOTCURSES_MinimizeWindow(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//}
//
//void NOTCURSES_RestoreWindow(SDL_VideoDevice *_this, SDL_Window *window)
//{
//    // FIXME: implement
//    SDL_Unsupported();
//}

#endif // SDL_VIDEO_DRIVER_NOTCURSES
