macro(SDL_Platform_OverrideOptionDefaults)
  set(SDL_SHARED_AVAILABLE OFF)
  set(SDL_LOADSO_DEFAULT OFF)
endmacro()

macro(SDL_Platform_ExtraOptions)
endmacro()

macro(SDL_Platform_Checks)
  sdl_compile_definitions(PRIVATE "PS2" "__PS2__")
  sdl_include_directories(PRIVATE SYSTEM "$ENV{PS2SDK}/ports/include" "$ENV{PS2DEV}/gsKit/include")

  sdl_glob_sources("${SDL3_SOURCE_DIR}/src/core/ps2/*.c")

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_PS2 1)
    sdl_glob_sources("${SDL3_SOURCE_DIR}/src/audio/ps2/*.c")
    set(HAVE_SDL_AUDIO TRUE)
  endif()
  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_PS2 1)
    sdl_glob_sources("${SDL3_SOURCE_DIR}/src/filesystem/ps2/*.c")
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()
  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_PS2 1)
    sdl_glob_sources("${SDL3_SOURCE_DIR}/src/joystick/ps2/*.c")
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()
  if(SDL_THREADS)
    set(SDL_THREAD_PS2 1)
    sdl_glob_sources(
      "${SDL3_SOURCE_DIR}/src/thread/generic/SDL_syscond.c"
      "${SDL3_SOURCE_DIR}/src/thread/generic/SDL_sysmutex.c"
      "${SDL3_SOURCE_DIR}/src/thread/generic/SDL_sysrwlock.c"
      "${SDL3_SOURCE_DIR}/src/thread/generic/SDL_systls.c"
      "${SDL3_SOURCE_DIR}/src/thread/ps2/*.c"
    )
    set(HAVE_SDL_THREADS TRUE)
  endif()
  if(SDL_TIMERS)
    set(SDL_TIMER_PS2 1)
    sdl_glob_sources("${SDL3_SOURCE_DIR}/src/timer/ps2/*.c")
    set(HAVE_SDL_TIMERS TRUE)
  endif()
  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_PS2 1)
    set(SDL_VIDEO_RENDER_PS2 1)
    sdl_glob_sources(
      "${SDL3_SOURCE_DIR}/src/video/ps2/*.c"
      "${SDL3_SOURCE_DIR}/src/render/ps2/*.c"
    )
    set(SDL_VIDEO_OPENGL 0)
    set(HAVE_SDL_VIDEO TRUE)
  endif()

  sdl_link_dependency(base
    LIBS
    patches
    gskit
    dmakit
    ps2_drivers
  )

  sdl_compile_options(PRIVATE "-Wno-error=declaration-after-statement")
endmacro()
