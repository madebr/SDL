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
#include <notcurses/notcurses.h>

#ifdef SDL_VIDEO_DRIVER_NOTCURSES

#include "SDL_notcursesvideo.h"
#include "SDL_notcursesevents.h"
#include "SDL_notcursesdyn.h"


#define EVENT_KEY_MAPPING_INDEX(ID) ((ID)-PRETERUNICODEBASE)

static struct {
    SDL_Scancode scancode;
    SDL_Keycode key;
} event_key_mappings[512] = {
    [EVENT_KEY_MAPPING_INDEX(NCKEY_UP)] = { SDL_SCANCODE_UP, SDLK_UP },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_RIGHT)] = { SDL_SCANCODE_RIGHT, SDLK_RIGHT },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_DOWN)] = { SDL_SCANCODE_DOWN, SDLK_DOWN },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_LEFT)] = { SDL_SCANCODE_LEFT, SDLK_LEFT },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_INS)] = { SDL_SCANCODE_INSERT, SDLK_INSERT },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_DEL)] = { SDL_SCANCODE_DELETE, SDLK_DELETE },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_BACKSPACE)] = { SDL_SCANCODE_BACKSPACE, SDLK_BACKSPACE },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_PGDOWN)] = { SDL_SCANCODE_PAGEDOWN, SDLK_PAGEDOWN },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_PGUP)] = { SDL_SCANCODE_PAGEUP, SDLK_PAGEUP },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_HOME)] = { SDL_SCANCODE_HOME, SDLK_HOME },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_END)] = { SDL_SCANCODE_END, SDLK_END },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F00)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F01)] = { SDL_SCANCODE_F1, SDLK_F1 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F02)] = { SDL_SCANCODE_F2, SDLK_F2 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F03)] = { SDL_SCANCODE_F3, SDLK_F3 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F04)] = { SDL_SCANCODE_F4, SDLK_F4 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F05)] = { SDL_SCANCODE_F5, SDLK_F5 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F06)] = { SDL_SCANCODE_F6, SDLK_F6 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F07)] = { SDL_SCANCODE_F7, SDLK_F7 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F08)] = { SDL_SCANCODE_F8, SDLK_F8 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F09)] = { SDL_SCANCODE_F9, SDLK_F9 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F10)] = { SDL_SCANCODE_F10, SDLK_F10 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F11)] = { SDL_SCANCODE_F11, SDLK_F11 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F12)] = { SDL_SCANCODE_F12, SDLK_F12 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F13)] = { SDL_SCANCODE_F13, SDLK_F13 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F14)] = { SDL_SCANCODE_F14, SDLK_F14 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F15)] = { SDL_SCANCODE_F15, SDLK_F15 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F16)] = { SDL_SCANCODE_F16, SDLK_F16 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F17)] = { SDL_SCANCODE_F17, SDLK_F17 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F18)] = { SDL_SCANCODE_F18, SDLK_F18 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F19)] = { SDL_SCANCODE_F19, SDLK_F19 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F20)] = { SDL_SCANCODE_F20, SDLK_F20 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F21)] = { SDL_SCANCODE_F21, SDLK_F21 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F22)] = { SDL_SCANCODE_F22, SDLK_F22 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F23)] = { SDL_SCANCODE_F23, SDLK_F23 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F24)] = { SDL_SCANCODE_F24, SDLK_F24 },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F25)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F26)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F27)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F28)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F29)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F30)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F31)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F32)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F33)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F34)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F35)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F36)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F37)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F38)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F39)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F40)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F41)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F42)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F43)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F44)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F45)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F46)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F47)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F48)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F49)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F50)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F51)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F52)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F53)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F54)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F55)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F56)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F57)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F58)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F59)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_F60)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_ENTER)] = { SDL_SCANCODE_RETURN, SDLK_RETURN }, /* FIXME: no KP_ENTER */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_CLS)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_DLEFT)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_DRIGHT)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_ULEFT)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_URIGHT)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_CENTER)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_BEGIN)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_CANCEL)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_CLOSE)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_COMMAND)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_COPY)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_EXIT)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_PRINT)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_REFRESH)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_SEPARATOR)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_CAPS_LOCK)] = { SDL_SCANCODE_CAPSLOCK, SDLK_CAPSLOCK },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_SCROLL_LOCK)] = { SDL_SCANCODE_SCROLLLOCK, SDLK_SCROLLLOCK },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_NUM_LOCK)] = { SDL_SCANCODE_NUMLOCKCLEAR, SDLK_NUMLOCKCLEAR }, /* FIXME: Is this correct */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_PRINT_SCREEN)] = { SDL_SCANCODE_PRINTSCREEN, SDLK_PRINTSCREEN },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_PAUSE)] = { SDL_SCANCODE_PAUSE, SDLK_PAUSE },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MENU)] = { SDL_SCANCODE_MENU, SDLK_MENU },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_PLAY)] = { SDL_SCANCODE_MEDIA_PLAY, SDLK_MEDIA_PLAY },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_PAUSE)] = { SDL_SCANCODE_MEDIA_PAUSE, SDLK_MEDIA_PAUSE },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_PPAUSE)] = { SDL_SCANCODE_MEDIA_PLAY_PAUSE, SDLK_MEDIA_PLAY_PAUSE },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_REV)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_STOP)] = { SDL_SCANCODE_MEDIA_STOP, SDLK_MEDIA_STOP },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_FF)] = { SDL_SCANCODE_MEDIA_FAST_FORWARD, SDLK_MEDIA_FAST_FORWARD },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_REWIND)] = { SDL_SCANCODE_MEDIA_REWIND, SDLK_MEDIA_REWIND },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_NEXT)] = { SDL_SCANCODE_MEDIA_NEXT_TRACK, SDLK_MEDIA_NEXT_TRACK },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_PREV)] = { SDL_SCANCODE_MEDIA_PREVIOUS_TRACK, SDLK_MEDIA_PREVIOUS_TRACK },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_RECORD)] = { SDL_SCANCODE_MEDIA_RECORD, SDLK_MEDIA_RECORD },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_LVOL)] = { SDL_SCANCODE_VOLUMEDOWN, SDLK_VOLUMEDOWN }, /* FIXME: mapping correct? */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_RVOL)] = { SDL_SCANCODE_VOLUMEUP, SDLK_VOLUMEUP }, /* FIXME: mapping correct? */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_MEDIA_MUTE)] = { SDL_SCANCODE_MUTE, SDLK_MUTE },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_LSHIFT)] = { SDL_SCANCODE_LSHIFT, SDLK_LSHIFT },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_LCTRL)] = { SDL_SCANCODE_LCTRL, SDLK_LCTRL },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_LALT)] = { SDL_SCANCODE_LALT, SDLK_LALT },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_LSUPER)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_LHYPER)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_LMETA)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_RSHIFT)] = { SDL_SCANCODE_RSHIFT, SDLK_RSHIFT },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_RCTRL)] = { SDL_SCANCODE_RCTRL, SDLK_RCTRL },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_RALT)] = { SDL_SCANCODE_RALT, SDLK_RALT },
    [EVENT_KEY_MAPPING_INDEX(NCKEY_RSUPER)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_RHYPER)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_RMETA)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_L3SHIFT)] = { 0 }, /* FIXME: unmapped */
    [EVENT_KEY_MAPPING_INDEX(NCKEY_L5SHIFT)] = { 0 }, /* FIXME: unmapped */
};

/* FIXME: BUG!!!!! this mapping is keycode dependent! (hardcoded values for qwerty US layout */
struct {
    SDL_Scancode scancode;
    SDL_Keycode key;
} unicode_event_mappings[256] = {
    ['\x1b'] = { SDL_SCANCODE_ESCAPE, SDLK_ESCAPE },
    [','] = { SDL_SCANCODE_COMMA, SDLK_COMMA },
    ['<'] = { SDL_SCANCODE_COMMA, SDLK_LESS },
    ['.'] = { SDL_SCANCODE_PERIOD, SDLK_PERIOD },
    ['>'] = { SDL_SCANCODE_PERIOD, SDLK_GREATER },
    ['/'] = { SDL_SCANCODE_SLASH, SDLK_SLASH },
    ['?'] = { SDL_SCANCODE_SLASH, SDLK_QUESTION },
    [';'] = { SDL_SCANCODE_SEMICOLON, SDLK_SEMICOLON },
    [':'] = { SDL_SCANCODE_SEMICOLON, SDLK_COLON },
    ['\''] = { SDL_SCANCODE_APOSTROPHE, SDLK_APOSTROPHE },
    ['"'] = { SDL_SCANCODE_APOSTROPHE, SDLK_DBLAPOSTROPHE },
    ['`'] = { SDL_SCANCODE_GRAVE, SDLK_GRAVE },
    ['~'] = { SDL_SCANCODE_GRAVE, SDLK_TILDE },
    ['1'] = { SDL_SCANCODE_1, SDLK_1 },
    ['!'] = { SDL_SCANCODE_1, SDLK_EXCLAIM },
    ['2'] = { SDL_SCANCODE_2, SDLK_2 },
    ['@'] = { SDL_SCANCODE_2, SDLK_AT},
    ['3'] = { SDL_SCANCODE_3, SDLK_3 },
    ['#'] = { SDL_SCANCODE_3, SDLK_HASH },
    ['4'] = { SDL_SCANCODE_4, SDLK_4 },
    ['$'] = { SDL_SCANCODE_4, SDLK_DOLLAR },
    ['5'] = { SDL_SCANCODE_5, SDLK_5 },
    ['%'] = { SDL_SCANCODE_5, SDLK_PERCENT },
    ['6'] = { SDL_SCANCODE_6, SDLK_6 },
    ['^'] = { SDL_SCANCODE_6, SDLK_CARET },
    ['7'] = { SDL_SCANCODE_7, SDLK_7 },
    ['&'] = { SDL_SCANCODE_7, SDLK_AMPERSAND },
    ['8'] = { SDL_SCANCODE_8, SDLK_8 },
    ['*'] = { SDL_SCANCODE_8, SDLK_ASTERISK },
    ['9'] = { SDL_SCANCODE_9, SDLK_9 },
    ['('] = { SDL_SCANCODE_9, SDLK_LEFTPAREN },
    ['0'] = { SDL_SCANCODE_0, SDLK_0 },
    [')'] = { SDL_SCANCODE_0, SDLK_RIGHTPAREN },
    ['-'] = { SDL_SCANCODE_MINUS, SDLK_MINUS },
    ['_'] = { SDL_SCANCODE_MINUS, SDLK_UNDERSCORE },
    ['='] = { SDL_SCANCODE_EQUALS, SDLK_EQUALS },
    ['+'] = { SDL_SCANCODE_EQUALS, SDLK_PLUS },
    ['['] = { SDL_SCANCODE_LEFTBRACKET, SDLK_LEFTBRACKET },
    ['{'] = { SDL_SCANCODE_LEFTBRACKET, SDLK_LEFTBRACE },
    [']'] = { SDL_SCANCODE_RIGHTBRACKET, SDLK_RIGHTBRACKET },
    ['}'] = { SDL_SCANCODE_RIGHTBRACKET, SDLK_RIGHTBRACE },
    ['\\'] = { SDL_SCANCODE_BACKSLASH, SDLK_BACKSLASH },
    ['|'] = { SDL_SCANCODE_BACKSLASH, SDLK_PIPE },
    [' '] = { SDL_SCANCODE_SPACE, SDLK_SPACE },
    ['\t'] = { SDL_SCANCODE_TAB, SDLK_TAB },
};

void NOTCURSES_PumpEvents(SDL_VideoDevice *_this)
{
    SDL_VideoData *viddata = _this->internal;
    struct timespec ts;
    ncinput ni;

    SDL_zero(ts);
    while (NOTCURSES_notcurses_get(viddata->nc, &ts, &ni) > 0) {
        SDL_Event event;
        SDL_zero(event);
        if (ni.id >= PRETERUNICODEBASE) {
            switch (ni.id) {
            case NCKEY_INVALID:
            case NCKEY_SIGNAL:
            case NCKEY_EOF:
                SDL_Log("Unhandled notcurses event id=%d evtype=0x%x", ni.id - PRETERUNICODEBASE, ni.evtype);
                break;
            case NCKEY_UP:
            case NCKEY_RIGHT:
            case NCKEY_DOWN:
            case NCKEY_LEFT:
            case NCKEY_INS:
            case NCKEY_DEL:
            case NCKEY_BACKSPACE:
            case NCKEY_PGDOWN:
            case NCKEY_PGUP:
            case NCKEY_HOME:
            case NCKEY_END:
            case NCKEY_F00:
            case NCKEY_F01:
            case NCKEY_F02:
            case NCKEY_F03:
            case NCKEY_F04:
            case NCKEY_F05:
            case NCKEY_F06:
            case NCKEY_F07:
            case NCKEY_F08:
            case NCKEY_F09:
            case NCKEY_F10:
            case NCKEY_F11:
            case NCKEY_F12:
            case NCKEY_F13:
            case NCKEY_F14:
            case NCKEY_F15:
            case NCKEY_F16:
            case NCKEY_F17:
            case NCKEY_F18:
            case NCKEY_F19:
            case NCKEY_F20:
            case NCKEY_F21:
            case NCKEY_F22:
            case NCKEY_F23:
            case NCKEY_F24:
            case NCKEY_F25:
            case NCKEY_F26:
            case NCKEY_F27:
            case NCKEY_F28:
            case NCKEY_F29:
            case NCKEY_F30:
            case NCKEY_F31:
            case NCKEY_F32:
            case NCKEY_F33:
            case NCKEY_F34:
            case NCKEY_F35:
            case NCKEY_F36:
            case NCKEY_F37:
            case NCKEY_F38:
            case NCKEY_F39:
            case NCKEY_F40:
            case NCKEY_F41:
            case NCKEY_F42:
            case NCKEY_F43:
            case NCKEY_F44:
            case NCKEY_F45:
            case NCKEY_F46:
            case NCKEY_F47:
            case NCKEY_F48:
            case NCKEY_F49:
            case NCKEY_F50:
            case NCKEY_F51:
            case NCKEY_F52:
            case NCKEY_F53:
            case NCKEY_F54:
            case NCKEY_F55:
            case NCKEY_F56:
            case NCKEY_F57:
            case NCKEY_F58:
            case NCKEY_F59:
            case NCKEY_F60:
            case NCKEY_ENTER:
            case NCKEY_CLS:
            case NCKEY_DLEFT:
            case NCKEY_DRIGHT:
            case NCKEY_ULEFT:
            case NCKEY_URIGHT:
            case NCKEY_CENTER:
            case NCKEY_BEGIN:
            case NCKEY_CANCEL:
            case NCKEY_CLOSE:
            case NCKEY_COMMAND:
            case NCKEY_COPY:
            case NCKEY_EXIT:
            case NCKEY_PRINT:
            case NCKEY_REFRESH:
            case NCKEY_SEPARATOR:
            case NCKEY_CAPS_LOCK:
            case NCKEY_SCROLL_LOCK:
            case NCKEY_NUM_LOCK:
            case NCKEY_PRINT_SCREEN:
            case NCKEY_PAUSE:
            case NCKEY_MENU:
            case NCKEY_MEDIA_PLAY:
            case NCKEY_MEDIA_PAUSE:
            case NCKEY_MEDIA_PPAUSE:
            case NCKEY_MEDIA_REV:
            case NCKEY_MEDIA_STOP:
            case NCKEY_MEDIA_FF:
            case NCKEY_MEDIA_REWIND:
            case NCKEY_MEDIA_NEXT:
            case NCKEY_MEDIA_PREV:
            case NCKEY_MEDIA_RECORD:
            case NCKEY_MEDIA_LVOL:
            case NCKEY_MEDIA_RVOL:
            case NCKEY_MEDIA_MUTE:
            case NCKEY_LSHIFT:
            case NCKEY_LCTRL:
            case NCKEY_LALT:
            case NCKEY_LSUPER:
            case NCKEY_LHYPER:
            case NCKEY_LMETA:
            case NCKEY_RSHIFT:
            case NCKEY_RCTRL:
            case NCKEY_RALT:
            case NCKEY_RSUPER:
            case NCKEY_RHYPER:
            case NCKEY_RMETA:
            case NCKEY_L3SHIFT:
            case NCKEY_L5SHIFT:
                if (EVENT_KEY_MAPPING_INDEX(ni.id) < SDL_arraysize(event_key_mappings)) {
                    event.type = SDL_EVENT_KEY_DOWN; /* FIXME: how to get key up events? */
                    event.key.scancode = event_key_mappings[EVENT_KEY_MAPPING_INDEX(ni.id)].scancode;
                    event.key.key = event_key_mappings[EVENT_KEY_MAPPING_INDEX(ni.id)].key;
                    SDL_PushEvent(&event);
                }
                if (!event.key.scancode) {
                    SDL_Log("Unhandled notcurses event id=%d evtype=0x%x", EVENT_KEY_MAPPING_INDEX(ni.id), ni.evtype);
                }
                break;
            case NCKEY_RESIZE:
                event.type = SDL_EVENT_WINDOW_RESIZED;
                event.window.data1 = ni.x; /* FIXME: use ni.xpx */
                event.window.data2 = ni.y; /* FIXME: use ni.ypx */
                SDL_PushEvent(&event);
                break;
            case NCKEY_MOTION:
                event.type = SDL_EVENT_MOUSE_MOTION;
                event.motion.x = (float) ni.x; /* FIXME: use ni.xpx */
                event.motion.y = (float) ni.y; /* FIXME: use ni.ypx */
                SDL_PushEvent(&event);
                break;
            case NCKEY_BUTTON1:
            case NCKEY_BUTTON2:
            case NCKEY_BUTTON3:
            case NCKEY_BUTTON4:
            case NCKEY_BUTTON5:
            case NCKEY_BUTTON6:
            case NCKEY_BUTTON7:
            case NCKEY_BUTTON8:
            case NCKEY_BUTTON9:
            case NCKEY_BUTTON10:
            case NCKEY_BUTTON11:
                if (ni.evtype == NCTYPE_UNKNOWN) {
                    continue;
                }
                if (ni.evtype == NCTYPE_PRESS || ni.evtype == NCTYPE_REPEAT) {
                    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                } else {
                    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
                }
                event.button.x = (float) ni.x; /* FIXME: use ni.xpx*/
                event.button.y = (float) ni.y; /* FIXME: use ni.ypx*/
                event.button.clicks = 1; /* FIXME: use .evtype */
                event.button.button = ni.id - NCKEY_BUTTON1;
                event.button.down = false;
                SDL_PushEvent(&event);
                break;
            }
        } else {
            event.type = SDL_EVENT_KEY_DOWN;
            /* FIXME: NCKEY_MOD_HYPER NCKEY_MOD_META NCKEY_MOD_CAPSLOCK NCKEY_MOD_NUMLOCK */
            if ((ni.id >= 'a' && ni.id <= 'z') || (ni.id >= 'A' && ni.id <= 'Z')) {
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.scancode = SDL_SCANCODE_A + ni.id - 'a';
                event.key.key = SDLK_A + ni.id - 'a';
            } else if (ni.id >= '0' && ni.id <= '9') {
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.scancode = SDL_SCANCODE_0 + ni.id - '0';
                event.key.key = SDLK_0 + ni.id - '0';
            } else {
                if (ni.id < SDL_arraysize(unicode_event_mappings)) {
                    event.key.scancode = unicode_event_mappings[ni.id].scancode;
                    event.key.key = unicode_event_mappings[ni.id].key;
                }
            }
            if (event.key.scancode) {
                if (ni.modifiers & NCKEY_MOD_SHIFT) {
                    event.key.mod |= SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT; /* FIXME: what shift? */
                }
                if (ni.modifiers & NCKEY_MOD_ALT) {
                    event.key.mod |= SDL_KMOD_LALT | SDL_KMOD_RALT; /* FIXME: what shift? */
                }
                if (ni.modifiers & NCKEY_MOD_SUPER) {
                    event.key.mod |= SDL_KMOD_LGUI | SDL_KMOD_RGUI; /* FIXME: what shift? */
                }
                SDL_PushEvent(&event);
            } else {
                SDL_Log("Unhandled notcurses unicode event unicode=0x%x evtype=0x%x modifiers=0x%x", ni.id, ni.evtype, ni.modifiers);
            }
        }
    }
}

#endif // SDL_VIDEO_DRIVER_KMSDRM
