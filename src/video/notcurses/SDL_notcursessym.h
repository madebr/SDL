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

/* *INDENT-OFF* */ // clang-format off

#ifndef SDL_NOTCURSES_MODULE
#define SDL_NOTCURSES_MODULE(modname)
#endif

#ifndef SDL_NOTCURSES_SYM
#define SDL_NOTCURSES_SYM(rc,fn,params)
#endif

SDL_NOTCURSES_MODULE(NOTCURSES_CORE)
SDL_NOTCURSES_SYM(const char*,notcurses_version,(void))
SDL_NOTCURSES_SYM(struct notcurses*,notcurses_core_init,(const notcurses_options*,FILE*))
SDL_NOTCURSES_SYM(int,notcurses_stop,(struct notcurses* nc))
SDL_NOTCURSES_SYM(struct ncplane*,notcurses_stdplane,(struct notcurses* nc))
SDL_NOTCURSES_SYM(void,ncplane_dim_yx,(const struct ncplane* n, unsigned* y, unsigned* x))
SDL_NOTCURSES_SYM(void,ncplane_set_resizecb,(struct ncplane* n, int(*resizecb)(struct ncplane*)))
SDL_NOTCURSES_SYM(uint32_t,notcurses_get,(struct notcurses* n, const struct timespec* ts,ncinput* ni))
SDL_NOTCURSES_SYM(int,notcurses_mice_enable,(struct notcurses* n, unsigned eventmask))
SDL_NOTCURSES_SYM(int,ncblit_rgba,(const void* data, int linesize, const struct ncvisual_options* vopts))
SDL_NOTCURSES_SYM(int,ncpile_render,(struct ncplane* n))
SDL_NOTCURSES_SYM(int,ncpile_rasterize,(struct ncplane* n))

#undef SDL_NOTCURSES_MODULE
#undef SDL_NOTCURSES_SYM

/* *INDENT-ON* */ // clang-format on
