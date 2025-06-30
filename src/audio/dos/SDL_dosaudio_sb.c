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

#ifdef SDL_AUDIO_DRIVER_DOS_SOUNDBLASTER

#include "SDL_dosaudio_sb.h"

static int soundblaster_base_port = -1;
static int soundblaster_irq = -1;
static int soundblaster_dma_channel = -1;
static int soundblaster_highdma_channel = -1;
static int soundblaster_version = -1;
static int soundblaster_version_minor = -1;

static void ResetSoundBlasterDSP(void)
{
    // reset the DSP.
    const int reset_port = soundblaster_base_port + 0x6;
    outportb(reset_port, 1);
    SDL_DelayPrecise(3000);  // wait at least 3 microseconds for hardware to see it.
    outportb(reset_port, 0);
}

static bool ReadSoundBlasterReady(void)
{
    const int ready_port = soundblaster_base_port + 0xE;
    return ((inportb(ready_port) & (1<<7)) != 0);
}

static void WriteSoundBlasterDSP(const Uint8 val)
{
    const int port = soundblaster_base_port + 0xC;
    while (inportb(port) & (1<<7)) { /* spin until the DSP says it can accept a command. */ }
    outportb(port, val);
}

static Uint8 ReadSoundBlasterDSP(void)
{
    const int query_port = soundblaster_base_port + 0xA;
    while (!ReadSoundBlasterReady()) { /* spin until the DSP says it has a reply available. */ }
    return (Uint8) inportb(query_port);
}

volatile int audio_streams_locked = 0;
static SDL_AudioDevice *opened_soundblaster_device = NULL;
static volatile bool soundblaster_irq_fired = false;
static void SoundBlasterIRQHandler(void)  // this is wrapped in a thing that handles IRET, etc.
{
    if (opened_soundblaster_device) {
        if (audio_streams_locked) {
            soundblaster_irq_fired = true;  // will need to handle this during unlock.
        } else {
            SDL_PlaybackAudioThreadIterate(opened_soundblaster_device);
        }
    }

    inportb(soundblaster_base_port + 0xF);  // acknowledge the interrupt by reading this port. Makes the SB stop pulling the line.  This is a different port (+ 0xE) when not using DSP 4.xx features!
    DOS_EndOfInterrupt();
}

// this is sort of hacky, but we need to make sure the audio interrupt doesn't
//  run while an audio stream is locked, since we don't have real mutexes, and
//  this is as close to multithreaded as we'll be getting for now.
// !!! FIXME: we should probably do this only for streams bound to an audio
// !!! FIXME: device, but good enough for now.
void SDL_DOS_LockAudioStream(SDL_AudioStream *stream)
{
    DOS_DisableInterrupts();
    audio_streams_locked++;
    DOS_EnableInterrupts();
}

void SDL_DOS_UnlockAudioStream(SDL_AudioStream *stream)
{
    DOS_DisableInterrupts();

    if (audio_streams_locked > 0) {
        if (--audio_streams_locked == 0) {
            if (opened_soundblaster_device && soundblaster_irq_fired) {
                // uhoh, IRQ fired while we were locked, run an iteration right now to catch up!
                // if you locked for a _really_ long time, you're going to get skips, but we can't help you there.
                soundblaster_irq_fired = false;
                SDL_PlaybackAudioThreadIterate(opened_soundblaster_device);
            }
        }
    }

    DOS_EnableInterrupts();
}

static bool DOSSOUNDBLASTER_OpenDevice(SDL_AudioDevice *device)
{
    // !!! FIXME: some things are hardcoded for this below, but we can use different formats in reality.
    // Although note that while an audio device can offer less than this--and will with pre-SB16 devices--this
    // is actually the minimum SDL will ever request, since it wants to make sure a low-quality logical device
    // doesn't force the hardware to low quality for future logical devices.
    device->spec.format = SDL_AUDIO_S16LE;
    device->spec.channels = 2;
    device->spec.freq = 44100;
    device->sample_frames = SDL_GetDefaultSampleFramesFromFreq(device->spec.freq);

    // Calculate the final parameters for this audio specification
    SDL_UpdatedAudioDeviceFormat(device);

    if (device->buffer_size > (32 * 1024)) {
        return SDL_SetError("Buffer size is too large (choose smaller audio format and/or less sample frames");  // DMA buffer has to fit in 64K segment, so buffer_size has to be half that, as we double it.
    }

    // Initialize all variables that we clean on shutdown
    struct SDL_PrivateAudioData *hidden = (struct SDL_PrivateAudioData *) SDL_calloc(1, sizeof(*device->hidden));
    if (!hidden) {
        return false;
    }

    device->hidden = hidden;

    ResetSoundBlasterDSP();

    // allocate conventional memory for the DMA buffer.
    hidden->dma_channel = soundblaster_highdma_channel;  // !!! FIXME
    hidden->dma_buflen = device->buffer_size * 2;
    hidden->dma_buffer = (Uint8 *)  DOS_AllocateDMAMemory(hidden->dma_buflen, &hidden->dma_seginfo);
	if (!hidden->dma_buffer) {
        return SDL_SetError("Couldn't allocate Sound Blaster DMA buffer!");
    }

    SDL_Log("SOUNDBLASTER: Allocated %d bytes of conventional memory at segment %d (ptr=%p)", (int) hidden->dma_buflen, (int) hidden->dma_seginfo.rm_segment, hidden->dma_buffer);

    // silence the DMA buffer to start
    memset(hidden->dma_buffer, '\0', hidden->dma_buflen);

    // !!! FIXME: this is different for 8-bit DMA.
    // set up DMA controller.
    const int dma_words = (hidden->dma_buflen / 2) - 1;
    const Uint32 physical = DOS_LinearToPhysical(hidden->dma_buffer);
    const Uint8 physical_page = (physical >> 16) & 0xFF;
    outportb(0xD4, 0x04 | hidden->dma_channel);  // mask the DMA channel
    outportb(0xD6, 0x59);  // auto-init playback.
    outportb(0x8B, (Uint8) physical_page);  // page to transfer
    outportb(0xD8, 0x00);  // clear the flip-flop, whatever that is.
    outportb(0xC4, (Uint8) ((physical >> 1) & 0xFF));  // offset of words to transfer (low)    // the extra bitshift is to halve the size, so we're in words, not bytes. 8-bit audio won't do this!
    outportb(0xC4, (Uint8) ((physical >> 9) & 0xFF));  // offset of words to transfer (high)
    outportb(0xD8, 0x00);  // clear the flip-flop, whatever that is.
    outportb(0xC6, (Uint8) (dma_words & 0xFF));  // number of words to transfer (low)   // (8-bit audio will do this in bytes, not words!)
    outportb(0xC6, (Uint8) ((dma_words >> 8) & 0xFF));  // number of words to transfer (high)
    outportb(0xDC, 0);  // clear the write mask.
    outportb(0xD4, ~4 & hidden->dma_channel);  // unmask the DMA channel FIXME: is this different for low DMA?

    soundblaster_irq_fired = false;
    DOS_HookInterrupt(soundblaster_irq, SoundBlasterIRQHandler, &hidden->interrupt_hook);

    WriteSoundBlasterDSP(0xD1);  // turn on the speaker
    // !!! FIXME: can we query (soundblaster_base_port + 0xC) to see if this is done faster?
    SDL_Delay(112);  // takes a maximum of 112 milliseconds to complete this command! Gosh!

    // set hardware to audio format.
    // !!! FIXME: DSP version 4+ (sb16 and later) can use this command. Older versions need to use a different command to set a "Time Constant".
    WriteSoundBlasterDSP(0x41);  // set output sampling rate
    WriteSoundBlasterDSP((Uint8) (device->spec.freq >> 8));
    WriteSoundBlasterDSP((Uint8) (device->spec.freq & 0xFF));

    // start auto-initialize DMA mode
    // half the total buffer per transfer, then convert to samples (divide by 2 because they are 16-bits each).
    const int block_size = ((hidden->dma_buflen / 2) / sizeof (Sint16)) - 1;  // one less than samples to be transferred.  // !!! FIXME: work with 8-bit samples, too.
    WriteSoundBlasterDSP(0xB6);  // 16-bit output  // !!! FIXME: don't hardcode this.
    WriteSoundBlasterDSP(0x30);  // 16-bit stereo signed PCM  // !!! FIXME: don't hardcode this.
    WriteSoundBlasterDSP((Uint8) (block_size & 0xFF));
    WriteSoundBlasterDSP((Uint8) (block_size >> 8));

    opened_soundblaster_device = device;

    SDL_Log("SoundBlaster opened!");
    return true;
}

static Uint8 *DOSSOUNDBLASTER_GetDeviceBuf(SDL_AudioDevice *device, int *buffer_size)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    SDL_assert(*buffer_size == (hidden->dma_buflen / 2));
    const int halfdma = *buffer_size;

    // !!! FIXME: this count is 16-bit samples, need to adjust for 8-bit.
    int count = (int) inportb(0xc0+(hidden->dma_channel-4)*4+2);    // !!! FIXME: is this different for low DMA?
    count += (int) inportb(0xc0+(hidden->dma_channel-4)*4+2) << 8;
    return hidden->dma_buffer + (count < (halfdma/2) ? 0 : halfdma);  // !!! FIXME: dealing in 16-bit samples (the `/ 2`)

}

static void DOSSOUNDBLASTER_CloseDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    if (hidden) {
        // Disable PCM.
        WriteSoundBlasterDSP(0xDA);  // exit auto-init
        WriteSoundBlasterDSP(0xD3);  // turn off the speaker

        DOS_UnhookInterrupt(&hidden->interrupt_hook, true);

        // disable DMA.
        if (hidden->dma_buffer) {
            // !!! FIXME: is high DMA or low?
            outportb(0xD4, 0x04 | hidden->dma_channel);  // mask the DMA channel.
            DOS_FreeConventionalMemory(&hidden->dma_seginfo);
        }

        soundblaster_irq_fired = false;
        opened_soundblaster_device = NULL;

        SDL_free(hidden);
    }
}

static bool CheckForSoundBlaster(void)
{
    ResetSoundBlasterDSP();

    // wait for the DSP to say it's ready.
    bool ready = false;
    for (int i = 0; i < 300; i++) { // may take up to 100msecs to initialize. We'll give it 300.
        SDL_DelayPrecise(1000);
        if (ReadSoundBlasterReady()) {
            ready = true;
            break;
        }
    }

    if (!ready) {
        return SDL_SetError("No SoundBlaster detected on port 0x%X", soundblaster_base_port);  // either no SoundBlaster or it's on a different base port.
    } else if (ReadSoundBlasterDSP() != 0xAA) {
        return SDL_SetError("Not a SoundBlaster at port 0x%X\n", soundblaster_base_port);  // either it's not a SoundBlaster or there's a problem.
    }
    return true;
}

static bool IsSoundBlasterPresent(void)
{
    const char *env = SDL_getenv("BLASTER");
    if (!env) {
        return SDL_SetError("No BLASTER environment variable to find Sound Blaster");   // definitely doesn't have a Sound Blaster (or they screwed up).
    }

    char *copy = SDL_strdup(env);
    if (!copy) {
        return false;  // oh well.
    }

    char *str = copy;
    char *saveptr = NULL;

    char *token;
    while ((token = SDL_strtok_r(str, " ", &saveptr)) != NULL) {
        str = NULL;  // must be NULL for future calls to tokenize the same string.
        char *endp = NULL;
        const int num = (int) SDL_strtol(token+1, &endp, 16);
        if ((token[1] == 0) || (*endp != 0)) {  // bogus num
            continue;
        } else if (num < 0) {
            continue;
        }

        switch (SDL_toupper(*token)) {
            case 'A':  // Base i/o port (in hex)
                soundblaster_base_port = num;
                break;

            case 'I':  // IRQ
                soundblaster_irq = num;
                break;

            case 'D':  // DMA channel
                soundblaster_dma_channel = num;
                break;

            case 'H':  // High DMA channel
                soundblaster_highdma_channel = num;
                break;

            // don't care about these.
            //case 'M':  // mixer chip base port
            //case 'P':  // MPU-401 base port
            //case 'T':  // type of device
            //case 'E':  // EMU8000 base port: an AWE32 thing
            default: break;
        }
    }
    SDL_free(copy);

    if (!soundblaster_base_port || !soundblaster_irq || (!soundblaster_dma_channel && !soundblaster_highdma_channel)) {
        return SDL_SetError("BLASTER environment variable is incomplete or incorrect");
    } else if (!CheckForSoundBlaster()) {
        return false;
    }

    WriteSoundBlasterDSP(0xE1);  // query DSP version
    soundblaster_version = (int) ReadSoundBlasterDSP();
    soundblaster_version_minor = (int) ReadSoundBlasterDSP();

    //SDL_Log("SoundBlaster DSP version %d.%d\n", (int) sbversion, (int) sbversion_minor);

    if (soundblaster_version < 4) {  // a sound blaster, but earlier than an sb16
        return SDL_SetError("Sound Blaster detected, but it's not at least an SB16.");  // !!! FIXME
    }

    SDL_Log("SB: BLASTER env='%s'", env);
    SDL_Log("SB: port=0x%X", soundblaster_base_port);
    SDL_Log("SB: irq=%d", soundblaster_irq);
    SDL_Log("SB: dma8=%d", soundblaster_dma_channel);
    SDL_Log("SB: dma16=%d", soundblaster_highdma_channel);
    SDL_Log("SB: version=%d.%d", soundblaster_version, soundblaster_version_minor);

    return true;
}

static bool DOSSOUNDBLASTER_Init(SDL_AudioDriverImpl *impl)
{
    if (!IsSoundBlasterPresent()) {
        return false;
    }

    impl->OpenDevice = DOSSOUNDBLASTER_OpenDevice;
    impl->GetDeviceBuf = DOSSOUNDBLASTER_GetDeviceBuf;
    impl->CloseDevice = DOSSOUNDBLASTER_CloseDevice;

    // !!! FIXME: maybe later
    //impl->WaitRecordingDevice = DOSSOUNDBLASTER_WaitDevice;
    //impl->RecordDevice = DOSSOUNDBLASTER_RecordDevice;
    //impl->FlushRecording = DOSSOUNDBLASTER_FlushRecording;
    //impl->HasRecordingSupport = true;
    //impl->OnlyHasDefaultRecordingDevice = true;

    impl->ProvidesOwnCallbackThread = true;  // hardware interrupts!
    impl->OnlyHasDefaultPlaybackDevice = true;

    return true;
}

AudioBootStrap DOSSOUNDBLASTER_bootstrap = {
    "soundblaster", "Sound Blaster", DOSSOUNDBLASTER_Init, false, false
};

#endif // SDL_AUDIO_DRIVER_OSS
