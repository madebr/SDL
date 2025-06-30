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

#include "../../events/SDL_events_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "../SDL_sysvideo.h"
#include "SDL_dosvideo.h"
#include "SDL_dosevents_c.h"

void DOSVESA_PumpEvents(SDL_VideoDevice *device)
{
    SDL_Mouse *mouse = SDL_GetMouse();
    if (mouse->internal) {  // if non-NULL, there's a mouse detected on the system.
        __dpmi_regs regs;

        regs.x.ax = 0x3;   // read mouse buttons and position.
        __dpmi_int(0x33, &regs);
        const Uint16 buttons = (int) (Sint16) regs.x.bx;

        SDL_SendMouseButton(0, mouse->focus, SDL_DEFAULT_MOUSE_ID, SDL_BUTTON_LEFT, (buttons & (1 << 0)) != 0);
        SDL_SendMouseButton(0, mouse->focus, SDL_DEFAULT_MOUSE_ID, SDL_BUTTON_RIGHT, (buttons & (1 << 1)) != 0);
        SDL_SendMouseButton(0, mouse->focus, SDL_DEFAULT_MOUSE_ID, SDL_BUTTON_MIDDLE, (buttons & (1 << 2)) != 0);

        if (!mouse->relative_mode) {
            const int x = (int) (Sint16) regs.x.cx;  // part of function 0x3's return value.
            const int y = (int) (Sint16) regs.x.dx;
            SDL_SendMouseMotion(0, mouse->focus, SDL_DEFAULT_MOUSE_ID, false, x, y);
        } else {
            regs.x.ax = 0xB;   // read motion counters
            __dpmi_int(0x33, &regs);
            // values returned here are -32768 to 32767
            const float MICKEYS_PER_HPIXEL = 4.0f;  // !!! FIXME: what should this be?
            const float MICKEYS_PER_VPIXEL = 4.0f;
            const int mickeys_x = (int) (Sint16) regs.x.cx;
            const int mickeys_y = (int) (Sint16) regs.x.dx;
            SDL_SendMouseMotion(0, mouse->focus, SDL_DEFAULT_MOUSE_ID, true, mickeys_x / MICKEYS_PER_HPIXEL, mickeys_y / MICKEYS_PER_VPIXEL);
        }
    }
}

#endif // SDL_VIDEO_DRIVER_DOSVESA
