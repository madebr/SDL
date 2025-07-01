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

// SDL internals
#include "../SDL_sysvideo.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_keyboard_c.h"

// DOS declarations
#include "SDL_dosvideo.h"
#include "SDL_dosevents_c.h"
#include "SDL_dosframebuffer_c.h"
#include "SDL_dosmouse.h"

// Some VESA usage information:
//   https://delorie.com/djgpp/doc/ug/graphics/vesa.html.en
//   https://delorie.com/djgpp/doc/ug/graphics/vbe20.html
//   https://wiki.osdev.org/User:Omarrx024/VESA_Tutorial
//   https://wiki.osdev.org/VESA_Video_Modes
//   https://www.phatcode.net/res/221/files/vbe20.pdf

#pragma pack(push, 1)
// this is the struct from the hardware; we save only a few parts from this, later, in SDL_VESAInfo.
typedef struct SDL_VESAHardwareInfo
{
    Uint8 VESASignature[4];
    Uint16 VESAVersion;
    Uint32 OEMStringPtr;  // segment:offset
    Uint8 Capabilities[4];
    Uint32 VideoModePtr;  // segment:offset
    Uint16 TotalMemory;
    Uint16 OEMSoftwareRev;
    Uint32 OEMVendorNamePtr;  // segment:offset
    Uint32 OEMProductNamePtr;  // segment:offset
    Uint32 OEMProductRevPtr;  // segment:offset
    Uint8 Reserved[222];
    Uint8 OemData[256];
} SDL_VESAHardwareInfo;

typedef struct SDL_VESAModeHardwareInfo
{
    Uint16 ModeAttributes;
    Uint8 WinAAttributes;
    Uint8 WinBAttributes;
    Uint16 WinGranularity;
    Uint16 WinSize;
    Uint16 WinASegment;
    Uint16 WinBSegment;
    Uint32 WinFuncPtr;
    Uint16 BytesPerScanLine;
    Uint16 XResolution;
    Uint16 YResolution;
    Uint8 XCharSize;
    Uint8 YCharSize;
    Uint8 NumberOfPlanes;
    Uint8 BitsPerPixel;
    Uint8 NumberOfBanks;
    Uint8 MemoryModel;
    Uint8 BankSize;
    Uint8 NumberOfImagePages;
    Uint8 Reserved_page;
    Uint8 RedMaskSize;
    Uint8 RedMaskPos;
    Uint8 GreenMaskSize;
    Uint8 GreenMaskPos;
    Uint8 BlueMaskSize;
    Uint8 BlueMaskPos;
    Uint8 ReservedMaskSize;
    Uint8 ReservedMaskPos;
    Uint8 DirectColorModeInfo;
    Uint32 PhysBasePtr;
    Uint32 OffScreenMemOffset;
    Uint16 OffScreenMemSize;
    Uint8 Reserved[206];
} SDL_VESAModeHardwareInfo;
#pragma pack(pop)

typedef struct SDL_VESAInfo
{
    Uint16 version;      // 0x200 == 2.0, etc.
    Uint32 total_memory;  // in bytes (SDL_VESAHardwareInfo::TotalMemory does it in 64k pages).
    Uint32 video_addr_segoffset;  // real mode segment:offset
    Uint16 oem_software_revision;
    char *oem_string;
    char *oem_vendor;
    char *oem_product;
    char *oem_revision;
} SDL_VESAInfo;

static SDL_VESAInfo *vesa_info = NULL;

static void FreeVESAInfo(void)
{
    if (vesa_info) {
        SDL_free(vesa_info->oem_string);
        SDL_free(vesa_info->oem_vendor);
        SDL_free(vesa_info->oem_product);
        SDL_free(vesa_info->oem_revision);
        SDL_free(vesa_info);
        vesa_info = NULL;
    }
}

static const SDL_VESAInfo *GetVESAInfo(void)
{
    if (vesa_info) {
        return vesa_info;
    }

    _go32_dpmi_seginfo hwinfo_seginfo;
    SDL_VESAHardwareInfo *hwinfo = (SDL_VESAHardwareInfo *) DOS_AllocateConventionalMemory(sizeof (*hwinfo), &hwinfo_seginfo);
    if (!hwinfo) {
        return NULL;
    }

    SDL_zerop(hwinfo);
    SDL_memcpy(hwinfo->VESASignature, "VBE2", 4);

    __dpmi_regs regs;
    regs.x.ax = 0x4F00;
    regs.x.es = DOS_LinearToPhysical(hwinfo) / 16;
    regs.x.di = DOS_LinearToPhysical(hwinfo) & 0xF;
    __dpmi_int(0x10, &regs);

    // al is 0x4F if VESA is supported, ah is 0x00 if this specific call succeeded.
    // If the interrupt call didn't replace VESASignature with "VESA" then something went wrong, too.
    if ((regs.x.ax != 0x004F) || (SDL_memcmp(hwinfo->VESASignature, "VESA", 4) != 0)) {
        SDL_SetError("VESA video not supported on this system");
    } else {
        vesa_info = (SDL_VESAInfo *) SDL_calloc(1, sizeof (*vesa_info));
        if (vesa_info) {
            vesa_info->version = hwinfo->VESAVersion;
            vesa_info->total_memory = ((Uint32) hwinfo->TotalMemory) * (64 * 1024);  // TotalMemory is 64k chunks, convert to bytes.
            vesa_info->video_addr_segoffset = hwinfo->VideoModePtr;
            vesa_info->oem_software_revision = hwinfo->OEMSoftwareRev;
            // these strings are often empty (or maybe NULL), but it's fine. We don't _actually_ need them.
            vesa_info->oem_string = DOS_GetFarPtrCString(hwinfo->OEMStringPtr);
            vesa_info->oem_vendor = DOS_GetFarPtrCString(hwinfo->OEMVendorNamePtr);
            vesa_info->oem_product = DOS_GetFarPtrCString(hwinfo->OEMProductNamePtr);
            vesa_info->oem_revision = DOS_GetFarPtrCString(hwinfo->OEMProductRevPtr);
        }
    }

    DOS_FreeConventionalMemory(&hwinfo_seginfo);

    return vesa_info;
}

static bool SupportsVESA2(void)
{
    const SDL_VESAInfo *info = GetVESAInfo();
    if (!info) {
        return false;  // it will have set an SDL error string.
    } else if (info->version < 0x200) {   // not at least VESA 2.0?
        FreeVESAInfo();  // won't be needing this, then.
        return SDL_SetError("Hardware is not VESA 2.0 compatible");
    }
    // don't free `info`, it's cached for later.
    return true;
}

static bool GetVESAModeInfo(Uint16 mode_id, SDL_DisplayModeData *info)
{
    _go32_dpmi_seginfo hwinfo_seginfo;
    SDL_VESAModeHardwareInfo *hwinfo = (SDL_VESAModeHardwareInfo *) DOS_AllocateConventionalMemory(sizeof (*hwinfo), &hwinfo_seginfo);
    if (!hwinfo) {
        return NULL;
    }

    SDL_zerop(hwinfo);

    __dpmi_regs regs;
    regs.x.ax = 0x4F01;
    regs.x.es = DOS_LinearToPhysical(hwinfo) / 16;
    regs.x.di = DOS_LinearToPhysical(hwinfo) & 0xF;
    regs.x.cx = mode_id;
    __dpmi_int(0x10, &regs);

    const bool retval = (regs.x.ax == 0x004F);
    if (retval) {
        SDL_zerop(info);
        info->mode_id = mode_id;
        info->attributes = hwinfo->ModeAttributes;
        info->pitch = hwinfo->BytesPerScanLine;
        info->w = hwinfo->XResolution;
        info->h = hwinfo->YResolution;
        info->num_planes = hwinfo->NumberOfPlanes;
        info->bpp = hwinfo->BitsPerPixel;
        info->memory_model = hwinfo->MemoryModel;
        info->num_image_pages = hwinfo->NumberOfImagePages;
        info->red_mask_size = hwinfo->RedMaskSize;
        info->red_mask_pos = hwinfo->RedMaskPos;
        info->green_mask_size = hwinfo->GreenMaskSize;
        info->green_mask_pos = hwinfo->GreenMaskPos;
        info->blue_mask_size = hwinfo->BlueMaskSize;
        info->blue_mask_pos = hwinfo->BlueMaskPos;
        info->physical_base_addr = hwinfo->PhysBasePtr;
    }

    DOS_FreeConventionalMemory(&hwinfo_seginfo);

    return retval;
}

static bool DOSVESA_GetDisplayModes(SDL_VideoDevice *device, SDL_VideoDisplay *sdl_display)
{
    const SDL_VESAInfo *vinfo = GetVESAInfo();
    SDL_assert(vinfo != NULL);  // we should have already cached this.

    /* read the list of available modes */
    for (Uint32 segoffset = vinfo->video_addr_segoffset; ; segoffset += sizeof (Uint16)) {
        const Uint16 modeid = DOS_PeekUint16(segoffset);
        if (modeid == 0xFFFF) {
            break;  // end of mode list.
        }

        SDL_DisplayModeData info;
        if (!GetVESAModeInfo(modeid, &info)) {
            continue;
        }

        #if 0
        SDL_Log("VESA: mode=%d %dx%dx%d attr=%X pitch=%d planes=%d pages=%d r=%d@%d g=%d@%d b=%d@%d addr=%X\n",
                (int) info.mode_id, (int) info.w, (int) info.h, (int) info.bpp,
                (unsigned int) info.attributes, (int) info.pitch,
                (int) info.num_planes, (int) info.num_image_pages,
                (int) info.red_mask_size, (int) info.red_mask_pos,
                (int) info.green_mask_size, (int) info.green_mask_pos,
                (int) info.blue_mask_size, (int) info.blue_mask_pos,
                (unsigned int) info.physical_base_addr);
        #endif

        if ((info.attributes & 0x99) != 0x99) {  // bit 0: supported in hardware, bit 3:color, bit 4:graphics (not text), bit 7:linear framebuffer == 0b10011001 == 0x99.
            continue;  // Doesn't have all the attributes we want.
        } else if (info.num_planes != 1) {
            continue;  // skip planar pixel layouts.
        } else if (info.bpp < 15) {
            continue;  // we're not messing with palettes, etc.  (does SDL3 even support this at the display level?!)
        } else if (!info.w || !info.h) {
            continue;  // zero-area display mode?!
        } else if ((info.memory_model != 4) && (info.memory_model != 6)) {
            continue;  // must be either packed pixel (4) or Direct Color (6).
        }

        SDL_PixelFormat format = SDL_PIXELFORMAT_UNKNOWN;
        if (info.memory_model == 4) {  // packed pixel
            switch (info.bpp) {
                case 15: format = SDL_PIXELFORMAT_XRGB1555; break;
                case 16: format = SDL_PIXELFORMAT_RGB565; break;
                case 24: format = SDL_PIXELFORMAT_RGB24; break;
                case 32: format = SDL_PIXELFORMAT_XRGB8888; break;
                default: break;
            }
        } else {
            SDL_assert(info.memory_model == 6);  // Direct Color.
            const Uint32 rmask = ((((Uint32)1) << info.red_mask_size) - 1) << info.red_mask_pos;
            const Uint32 gmask = ((((Uint32)1) << info.green_mask_size) - 1) << info.green_mask_pos;
            const Uint32 bmask = ((((Uint32)1) << info.blue_mask_size) - 1) << info.blue_mask_pos;
            format = SDL_GetPixelFormatForMasks(info.bpp, rmask, gmask, bmask, 0x00000000);
        }

        if (format == SDL_PIXELFORMAT_UNKNOWN) {
            continue;  // don't know what to do with this one.
        }

        // okay, add this mode.

        #if 0
        SDL_Log("ADD VESA MODE: mode=%d %dx%dx%d attr=%X pitch=%d planes=%d pages=%d r=%d@%d g=%d@%d b=%d@%d addr=%X\n",
                (int) info.mode_id, (int) info.w, (int) info.h, (int) info.bpp,
                (unsigned int) info.attributes, (int) info.pitch,
                (int) info.num_planes, (int) info.num_image_pages,
                (int) info.red_mask_size, (int) info.red_mask_pos,
                (int) info.green_mask_size, (int) info.green_mask_pos,
                (int) info.blue_mask_size, (int) info.blue_mask_pos,
                (unsigned int) info.physical_base_addr);
        #endif

        SDL_DisplayModeData *internal = (SDL_DisplayModeData *) SDL_malloc(sizeof (*internal));
        if (!internal) {
            continue;  // oof.
        }

        SDL_copyp(internal, &info);

        SDL_DisplayMode mode;
        SDL_zero(mode);
        mode.format = format;
        mode.w = (int) info.w;
        mode.h = (int) info.h;
        mode.pixel_density = 1.0f;  // no HighDPI scaling here.

        // !!! FIXME: we need to parse EDID data (VESA function 0x4F15, subfunction 0x01) to get refresh rates. Leaving as 0 for now.
        //float refresh_rate;             /**< refresh rate (or 0.0f for unspecified) */
        //int refresh_rate_numerator;     /**< precise refresh rate numerator (or 0 for unspecified) */
        //int refresh_rate_denominator;   /**< precise refresh rate denominator */

        mode.internal = internal;

        if (!SDL_AddFullscreenDisplayMode(sdl_display, &mode)) {
            SDL_free(internal);  // oh well, carry on without it.
        }
    }

    return true;
}

static bool DOSVESA_SetDisplayMode(SDL_VideoDevice *device, SDL_VideoDisplay *sdl_display, SDL_DisplayMode *mode)
{
    SDL_VideoData *data = device->internal;
    const SDL_DisplayModeData *modedata = mode->internal;

    if (data->current_mode.internal && (data->current_mode.internal->mode_id == modedata->mode_id)) {
        return true;
    }

    if (data->mapping.size) {
        __dpmi_free_physical_address_mapping(&data->mapping);  // dump existing video mapping.
    }

    __dpmi_regs regs;
    regs.x.ax = 0x4F02;
    regs.x.bx = modedata->mode_id | 0x4000;  // the 0x4000 is the "I want a linear framebuffer" flag.
    __dpmi_int(0x10, &regs);

    if (regs.x.ax != 0x004F) {
        return SDL_SetError("Failed to set VESA video mode");
    }

    data->mapping.address = modedata->physical_base_addr;
    data->mapping.size = GetVESAInfo()->total_memory;
    if (__dpmi_physical_address_mapping(&data->mapping) != 0) {
        SDL_zero(data->mapping);
        regs.x.ax = 0x03;  // try to dump us back into text mode. Not sure if this is a good idea, though.
        __dpmi_int(0x10, &regs);
        SDL_zero(data->current_mode);
        return SDL_SetError("Failed to map VESA video memory");
    }

    // make sure framebuffer is blanked out.
    SDL_memset(DOS_PhysicalToLinear(data->mapping.address), '\0', modedata->h * modedata->pitch);

    SDL_copyp(&data->current_mode, mode);

    if (SDL_GetMouse()->internal != NULL) {  // internal != NULL) == int 33h services available.
        regs.x.ax = 0x7;  // set mouse min/max horizontal position.
        regs.x.cx = 0;
        regs.x.dx = (Uint16) mode->w;
        __dpmi_int(0x33, &regs);

        regs.x.ax = 0x8;  // set mouse min/max vertical position.
        regs.x.cx = 0;
        regs.x.dx = (Uint16) mode->h;
        __dpmi_int(0x33, &regs);
    }

    return true;
}

static bool DOSVESA_CreateWindow(SDL_VideoDevice *device, SDL_Window *window, SDL_PropertiesID create_props)
{
    // Allocate window internal data
    SDL_WindowData *wdata = (SDL_WindowData *)SDL_calloc(1, sizeof(SDL_WindowData));
    if (!wdata) {
        return false;
    }

    SDL_VideoDisplay *display = SDL_GetVideoDisplayForWindow(window);

    SDL_DisplayMode closest;
    if (!SDL_GetClosestFullscreenDisplayMode(display->id, window->w, window->h, 0.0f, false, &closest)) {
        SDL_assert(display->num_fullscreen_modes > 0);
        SDL_copyp(&closest, &display->fullscreen_modes[0]);
        window->w = closest.w;  // clamp window to the largest size we have available.
        window->h = closest.h;  // clamp window to the largest size we have available.
    }

    // if we're going fullscreen, don't set a video mode now, since we're just going to set one in a moment anyhow.
    if ((window->pending_flags & SDL_WINDOW_FULLSCREEN) == 0) {
        if (!DOSVESA_SetDisplayMode(device, display, &closest)) {
            return false;
        }
    }

    // Setup driver data for this window
    window->internal = wdata;

    // One window, it always has focus
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);

    // Window has been successfully created
    return true;
}

static void DOSVESA_DestroyWindow(SDL_VideoDevice *device, SDL_Window *window)
{
    SDL_free(window->internal);
    window->internal = NULL;
}

static bool DOSVESA_VideoInit(SDL_VideoDevice *device)
{
    SDL_VideoData *data = device->internal;

    // We are probably in text mode at startup, so we don't have a real "desktop mode" atm.
    // Pick something _super_ conservative for now.
    //  We'll change to a real video mode when they create an SDL window.
    SDL_DisplayMode mode;
    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 320;
    mode.h = 200;
    SDL_DisplayID display_id = SDL_AddBasicVideoDisplay(&mode);
    if (!display_id) {
        return false;
    }

    SDL_zero(data->current_mode);

    SDL_VideoDisplay *display = SDL_GetVideoDisplay(display_id);
    if (!display || !DOSVESA_GetDisplayModes(device, display)) {
        return false;
    }

    DOSVESA_InitMouse(device);
    DOSVESA_InitKeyboard(device);

    return true;
}

static void DOSVESA_VideoQuit(SDL_VideoDevice *device)
{
    SDL_VideoData *data = device->internal;

    if (data->mapping.size) {
        __dpmi_free_physical_address_mapping(&data->mapping);  // dump existing video mapping.
        SDL_zero(data->mapping);
    }

    // Force us back to text mode, in case we're not there.
    __dpmi_regs regs;
    regs.x.ax = 0x03;
    __dpmi_int(0x10, &regs);

    SDL_zero(data->current_mode);

    DOSVESA_QuitMouse(device);
    DOSVESA_QuitKeyboard(device);
}

static void DOSVESA_Destroy(SDL_VideoDevice *device)
{
    SDL_free(device->internal);
    SDL_free(device);
    FreeVESAInfo();
}

static SDL_VideoDevice *DOSVESA_CreateDevice(void)
{
    if (!SupportsVESA2()) {
        return NULL;
    }

    SDL_VideoDevice *device;
    SDL_VideoData *phdata;

    // Initialize SDL_VideoDevice structure
    device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        return NULL;
    }

    // Initialize internal data
    phdata = (SDL_VideoData *)SDL_calloc(1, sizeof(SDL_VideoData));
    if (!phdata) {
        SDL_free(device);
        return NULL;
    }

    device->internal = phdata;
    device->free = DOSVESA_Destroy;
    device->VideoInit = DOSVESA_VideoInit;
    device->VideoQuit = DOSVESA_VideoQuit;
    device->CreateSDLWindow = DOSVESA_CreateWindow;
    device->DestroyWindow = DOSVESA_DestroyWindow;
    device->CreateWindowFramebuffer = DOSVESA_CreateWindowFramebuffer;
    device->SetWindowFramebufferVSync = DOSVESA_SetWindowFramebufferVSync;
    device->GetWindowFramebufferVSync = DOSVESA_GetWindowFramebufferVSync;
    device->UpdateWindowFramebuffer = DOSVESA_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = DOSVESA_DestroyWindowFramebuffer;
    device->PumpEvents = DOSVESA_PumpEvents;

    // strictly-speaking, we're fullscreen-only, but we don't know the "default" video mode
    // because we're probably in text mode! So in CreateWindow we try to set a resolution
    // close to the window size and center it if they aren't asking for a specific mode
    // but we can't do this if VIDEO_DEVICE_CAPS_FULLSCREEN_ONLY is set, since SDL will
    // change the window size to the current "desktop" mode.
    //device->device_caps = VIDEO_DEVICE_CAPS_FULLSCREEN_ONLY;

    return device;
}

VideoBootStrap DOSVESA_bootstrap = {
    "vesa",
    "DOS VESA Video Driver",
    DOSVESA_CreateDevice,
    NULL, // no ShowMessageBox implementation
    false
};

#endif // SDL_VIDEO_DRIVER_DOSVESA
