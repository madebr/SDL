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

#ifndef SDL_notcursesvideo_h
#define SDL_notcursesvideo_h

#include "../SDL_sysvideo.h"

struct SDL_VideoData
{
    struct notcurses* nc;
    struct ncplane* stdplane;
};

//struct SDL_DisplayModeData
//{
//};
//
//struct SDL_DisplayData
//{
//};
//
//struct SDL_WindowData
//{
//};

/****************************************************************************/
// SDL_VideoDevice functions declaration
/****************************************************************************/

// Display and window functions
extern bool NOTCURSES_VideoInit(SDL_VideoDevice *_this);
extern void NOTCURSES_VideoQuit(SDL_VideoDevice *_this);
extern bool NOTCURSES_GetDisplayModes(SDL_VideoDevice *_this, SDL_VideoDisplay *display);
extern bool NOTCURSES_SetDisplayMode(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_DisplayMode *mode);
extern bool NOTCURSES_CreateWindow(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID create_props);
extern void NOTCURSES_DestroyWindow(SDL_VideoDevice *_this, SDL_Window *window);
//extern void NOTCURSES_SetWindowTitle(SDL_VideoDevice *_this, SDL_Window *window);
//extern bool NOTCURSES_SetWindowPosition(SDL_VideoDevice *_this, SDL_Window *window);
//extern void NOTCURSES_SetWindowSize(SDL_VideoDevice *_this, SDL_Window *window);
//extern SDL_FullscreenResult NOTCURSES_SetWindowFullscreen(SDL_VideoDevice *_this, SDL_Window *window, SDL_VideoDisplay *_display, SDL_FullscreenOp fullscreen);
//extern void NOTCURSES_ShowWindow(SDL_VideoDevice *_this, SDL_Window *window);
//extern void NOTCURSES_HideWindow(SDL_VideoDevice *_this, SDL_Window *window);
//extern void NOTCURSES_RaiseWindow(SDL_VideoDevice *_this, SDL_Window *window);
//extern void NOTCURSES_MaximizeWindow(SDL_VideoDevice *_this, SDL_Window *window);
//extern void NOTCURSES_MinimizeWindow(SDL_VideoDevice *_this, SDL_Window *window);
//extern void NOTCURSES_RestoreWindow(SDL_VideoDevice *_this, SDL_Window *window);

#endif // SDL_notcursesvideo_h
