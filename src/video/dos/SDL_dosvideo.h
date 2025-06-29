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

#ifndef SDL_dosvideo_h
#define SDL_dosvideo_h

#include "SDL_internal.h"
#include "../SDL_sysvideo.h"
#include "../../core/dos/SDL_dos.h"

struct SDL_DisplayModeData
{
    // we can add more fields to this, if we want them, later.
    Uint16 mode_id;
    Uint16 attributes;
    Uint16 pitch;
    Uint16 w;
    Uint16 h;
    Uint8 num_planes;
    Uint8 bpp;
    Uint8 memory_model;
    Uint8 num_image_pages;
    Uint8 red_mask_size;
    Uint8 red_mask_pos;
    Uint8 green_mask_size;
    Uint8 green_mask_pos;
    Uint8 blue_mask_size;
    Uint8 blue_mask_pos;
    Uint32 physical_base_addr;
};

struct SDL_VideoData
{
    __dpmi_meminfo mapping;  // video memory mapping.
    SDL_DisplayMode current_mode;
};

struct SDL_DisplayData
{
    int unused;  // for now
};

struct SDL_WindowData
{
    int framebuffer_vsync;
};

#endif // SDL_dosvideo_h
