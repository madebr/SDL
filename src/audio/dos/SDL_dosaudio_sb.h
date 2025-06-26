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

#ifndef SDL_dosaudio_sb_h_
#define SDL_dosaudio_sb_h_

#include "../SDL_sysaudio.h"
#include "../../core/dos/SDL_dos.h"

struct SDL_PrivateAudioData
{
    Uint8 *dma_buffer;
    size_t dma_buflen;
    int dma_channel;
    _go32_dpmi_seginfo dma_seginfo;
    int interrupt_vector;
    _go32_dpmi_seginfo irq_handler_seginfo;
    _go32_dpmi_seginfo original_irq_handler_seginfo;
};

#endif // SDL_ALSA_audio_h_
