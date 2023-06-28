/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  System interface for sound.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIBSDL_MIXER
#define HAVE_MIXER
#endif
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_MIXER
#include "SDL_mixer.h"
#endif

#include "z_zone.h"

#include "m_swap.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "lprintf.h"
#include "s_sound.h"
#include "mmus2mid.h"

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "d_main.h"
#include "AEEIMedia.h"
#include "AEEMediaFormats.h"
#include "AEESource.h"
#include "IAudioSource.h"
#include "prboom.h"

extern PrBoomApp *pApp;

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Variables used by Boom from Allegro
// created here to avoid changes to core Boom files
int snd_card = 1;
int mus_card = 1;
int detect_voices = 0; // God knows

static boolean sound_inited = false;
static boolean first_sound_init = true;

// Needed for calling the actual sound output.
static int SAMPLECOUNT=   512;

#ifdef USE_BREW_MIXER
#define MAX_CHANNELS    4
void ChannelMediaNotify(void *po, AEEMediaCmdNotify *cmd);
#else
#define MAX_CHANNELS    32
#endif

// MWM 2000-01-08: Sample rate in samples/second
int snd_samplerate=11025;

// The actual output device.
int audio_fd;

typedef struct {
  // SFX id of the playing sound effect.
  // Used to catch duplicates (like chainsaw).
  int id;
// The channel step amount...
  unsigned int step;
// ... and a 0.16 bit remainder of last step.
  unsigned int stepremainder;
  unsigned int samplerate;
// The channel data pointers, start and end.
  const unsigned char* data;
  const unsigned char* enddata;
// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
  int starttime;
  // Hardware left and right channel volume lookup.
  int *leftvol_lookup;
  int *rightvol_lookup;
  int len;

#ifdef USE_BREW_MIXER
  IMedia *pIMedia;
#endif
} channel_info_t;

channel_info_t channelinfo[MAX_CHANNELS];

// Pitch to stepping lookup, unused.
int   steptable[256];

// Volume lookups.
int   vol_lookup[128*256];

/* cph
 * stopchan
 * Stops a sound, unlocks the data
 */

static void stopchan(int i)
{
  if (channelinfo[i].data) /* cph - prevent excess unlocks */
  {
    channelinfo[i].data=NULL;
    W_UnlockLumpNum(S_sfx[channelinfo[i].id].lumpnum);
  }
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
static int addsfx(int sfxid, int channel, const unsigned char* data, size_t len)
{
  stopchan(channel);

  channelinfo[channel].data = data;
  /* Set pointer to end of raw data. */
  channelinfo[channel].enddata = channelinfo[channel].data + len - 1;
  channelinfo[channel].samplerate = (channelinfo[channel].data[3]<<8)+channelinfo[channel].data[2];
  channelinfo[channel].data += 8; /* Skip header */
  channelinfo[channel].len = len;

  channelinfo[channel].stepremainder = 0;
  // Should be gametic, I presume.
  channelinfo[channel].starttime = gametic;

  // Preserve sound SFX id,
  //  e.g. for avoiding duplicates of chainsaw.
  channelinfo[channel].id = sfxid;

  return channel;
}

static void updateSoundParams(int handle, int volume, int seperation, int pitch)
{
  int slot = handle;
    int   rightvol;
    int   leftvol;
    int         step = steptable[pitch];

#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_UpdateSoundParams: handle out of range");
#endif
  // Set stepping
  // MWM 2000-12-24: Calculates proportion of channel samplerate
  // to global samplerate for mixing purposes.
  // Patched to shift left *then* divide, to minimize roundoff errors
  // as well as to use SAMPLERATE as defined above, not to assume 11025 Hz
    if (pitched_sounds)
    channelinfo[slot].step = step + (((channelinfo[slot].samplerate<<16)/snd_samplerate)-65536);
    else
    channelinfo[slot].step = ((channelinfo[slot].samplerate<<16)/snd_samplerate);

    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    leftvol = volume - ((volume*seperation*seperation) >> 16);
    seperation = seperation - 257;
    rightvol= volume - ((volume*seperation*seperation) >> 16);

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 127)
  I_Error("rightvol out of bounds");

    if (leftvol < 0 || leftvol > 127)
  I_Error("leftvol out of bounds");

    // Get the proper lookup table piece
    //  for this volume level???
  channelinfo[slot].leftvol_lookup = &vol_lookup[leftvol*256];
  channelinfo[slot].rightvol_lookup = &vol_lookup[rightvol*256];
}

void I_UpdateSoundParams(int handle, int volume, int seperation, int pitch)
{
  //SDL_LockAudio();
  updateSoundParams(handle, volume, seperation, pitch);
  //SDL_UnlockAudio();
}

//
// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//
void I_SetChannels(void)
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process.
  int   i;
  int   j;

  int*  steptablemid = steptable + 128;

  // Okay, reset internal mixing channels to zero.
  for (i=0; i<MAX_CHANNELS; i++)
  {
    memset(&channelinfo[i],0,sizeof(channel_info_t));
  }

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = (int)(pow(1.2, ((double)i/(64.0*snd_samplerate/11025)))*65536.0);


  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
    {
      // proff - made this a little bit softer, because with
      // full volume the sound clipped badly
      vol_lookup[i*256+j] = (i*(j-128)*256)/191;
      //vol_lookup[i*256+j] = (i*(j-128)*256)/127;
    }
}

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}

#ifdef USE_BREW_MIXER

void I_BrewPlaySingle(int channel)
{
	channel_info_t *channel_info = &channelinfo[channel];
	AEEMediaDataEx mde;
	AEEMediaData md;
	AEEMediaWaveSpec mws;
	int ret;
	int state;
	boolean pbStateChanging;

	if(ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_MEDIAPCM, (void **)(&channel_info->pIMedia)) != SUCCESS)
		I_Error("Failed to create channel IMedia instance");

	printf("IMedia %x data %x len %d", channel_info->pIMedia, channel_info->data, channel_info->len);

   memset(&mws, (int)0, sizeof(AEEMediaWaveSpec));
   mws.wSize = sizeof(AEEMediaWaveSpec);
   mws.clsMedia = AEECLSID_MEDIAPCM;
   mws.wChannels = 2;
   mws.dwSamplesPerSec = snd_samplerate;
   mws.wBitsPerSample = 16;
   mws.bUnsigned = FALSE;

   mde.clsData = MMD_BUFFER;
   mde.pData = (void *)channel_info->data;
   mde.dwSize = channel_info->len;
   mde.bRaw = TRUE;
   mde.dwCaps = 0;
   //mde.pSpec = &mws;
   //mde.dwSpecSize = sizeof(AEEMediaWaveSpec);

  if(IMedia_EnableChannelShare(channel_info->pIMedia, true) != SUCCESS) {
    I_Error("Failed SOUNDEnableChannelShare");
  }

  ret = IMedia_SetMediaDataEx(channel_info->pIMedia, &mde, 1);
  if(ret != SUCCESS) {
    I_Error("I_BrewPlaySingle Failed IMedia_SetMediaDataEx ret = %d", ret);
  }

  state = IMedia_GetState(channel_info->pIMedia, &pbStateChanging);
  printf("I_BrewPlaySingle state1 = %d", state);

	if(IMedia_RegisterNotify(channel_info->pIMedia, (PFNMEDIANOTIFY)ChannelMediaNotify, (void *)channel_info) != SUCCESS) {
	  printf("Failed to register SIMedia notify");
	}

   state = IMedia_GetState(channel_info->pIMedia, &pbStateChanging);
  printf("I_BrewPlaySingle state2 = %d", state);

   if(IMedia_Play(channel_info->pIMedia) != SUCCESS)
     I_Error("I_BrewPlaySingle Failed to start Play");

  state = IMedia_GetState(channel_info->pIMedia, &pbStateChanging);
  printf("I_BrewPlaySingle state3 = %d", state);
}

#endif // USE_BREW_MIXER

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int I_StartSound(int id, int channel, int vol, int sep, int pitch, int priority)
{
  const unsigned char* data;
  int lump;
  size_t len;

  if ((channel < 0) || (channel >= MAX_CHANNELS))
#ifdef RANGECHECK
    I_Error("I_StartSound: handle out of range");
#else
    return -1;
#endif

  lump = S_sfx[id].lumpnum;

  // We will handle the new SFX.
  // Set pointer to raw data.
  len = W_LumpLength(lump);

  // e6y: Crash with zero-length sounds.
  // Example wad: dakills (http://www.doomworld.com/idgames/index.php?id=2803)
  // The entries DSBSPWLK, DSBSPACT, DSSWTCHN and DSSWTCHX are all zero-length sounds
  if (len<=8) return -1;

  /* Find padded length */
  len -= 8;
  // do the lump caching outside the SDL_LockAudio/SDL_UnlockAudio pair
  // use locking which makes sure the sound data is in a malloced area and
  // not in a memory mapped one
  data = W_LockLumpNum(lump);

  //SDL_LockAudio();

  // Returns a handle (not used).
  addsfx(id, channel, data, len);
  updateSoundParams(channel, vol, sep, pitch);

  //SDL_UnlockAudio();

#ifdef USE_BREW_MIXER
  I_BrewPlaySingle(channel);
  // sound is played instantaneously, return -1 so we don't alloc channel
  //channel = -1;
#endif

  return channel;
}



void I_StopSound (int handle)
{
#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_StopSound: handle out of range");
#endif
  //SDL_LockAudio();
  stopchan(handle);
  //SDL_UnlockAudio();
}


boolean I_SoundIsPlaying(int handle)
{
#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_SoundIsPlaying: handle out of range");
#endif
  return channelinfo[handle].data != NULL;
}


boolean I_AnySoundStillPlaying(void)
{
  boolean result = false;
  int i;

  for (i=0; i<MAX_CHANNELS; i++)
    result |= channelinfo[i].data != NULL;

  return result;
}


//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the given
//  mixing buffer, and clamping it to the allowed
//  range.
//
// This function currently supports only 16bit.
//

void I_UpdateSound(void *unused, int *stream, int len)
{
  // Mix current sound data.
  // Data, from raw sound, for right and left.
  unsigned char sample;
  int    dl;
  int    dr;

  // Pointers in audio stream, left, right, end.
  signed short*   leftout;
  signed short*   rightout;
  signed short*   leftend;
  // Step in stream, left and right, thus two.
  int       step;

  // Mixing channel index.
  int       chan;

    // Left and right channel
    //  are in audio stream, alternating.
    leftout = (signed short *)stream;
    rightout = ((signed short *)stream)+1;
    step = 2;

    // Determine end, for left channel only
    //  (right channel is implicit).
    leftend = leftout + (len/4)*step;

    // Mix sounds into the mixing buffer.
    // Loop over step*SAMPLECOUNT,
    //  that is 512 values for two channels.
    while (leftout != leftend)
    {
  // Reset left/right value.
  dl = 0;
  dr = 0;
  // dl = *leftout;
  // dr = *rightout;

  // Love thy L2 chache - made this a loop.
  // Now more channels could be set at compile time
  //  as well. Thus loop those  channels.
    for ( chan = 0; chan < numChannels; chan++ )
  {
      // Check channel, if active.
      if (channelinfo[chan].data)
      {
    // Get the raw data from the channel.
        // no filtering
        // sample = *channelinfo[chan].data;
        // linear filtering
        sample = (((unsigned int)channelinfo[chan].data[0] * (0x10000 - channelinfo[chan].stepremainder))
                + ((unsigned int)channelinfo[chan].data[1] * (channelinfo[chan].stepremainder))) >> 16;

    // Add left and right part
    //  for this channel (sound)
    //  to the current data.
    // Adjust volume accordingly.
        dl += channelinfo[chan].leftvol_lookup[sample];
        dr += channelinfo[chan].rightvol_lookup[sample];
    // Increment index ???
        channelinfo[chan].stepremainder += channelinfo[chan].step;
    // MSB is next sample???
        channelinfo[chan].data += channelinfo[chan].stepremainder >> 16;
    // Limit to LSB???
        channelinfo[chan].stepremainder &= 0xffff;

    // Check whether we are done.
        if (channelinfo[chan].data >= channelinfo[chan].enddata)
      stopchan(chan);
      }
  }

  // Clamp to range. Left hardware channel.
  // Has been char instead of short.
  // if (dl > 127) *leftout = 127;
  // else if (dl < -128) *leftout = -128;
  // else *leftout = dl;

  if (dl > SHRT_MAX)
      *leftout = SHRT_MAX;
  else if (dl < SHRT_MIN)
      *leftout = SHRT_MIN;
  else
      *leftout = (signed short)dl;

  // Same for right hardware channel.
  if (dr > SHRT_MAX)
      *rightout = SHRT_MAX;
  else if (dr < SHRT_MIN)
      *rightout = SHRT_MIN;
  else
      *rightout = (signed short)dr;

  // Increment current pointers in stream
  leftout += step;
  rightout += step;
    }
}

void I_ShutdownSound(void)
{
  if (sound_inited) {
    lprintf(LO_INFO, "I_ShutdownSound: ");
#ifdef HAVE_MIXER
    Mix_CloseAudio();
#else
    //SDL_CloseAudio();
#endif
    lprintf(LO_INFO, "\n");
    sound_inited = false;
  }
}

void MusicMediaNotify(void *po, AEEMediaCmdNotify *cmd) {
  int state;
  boolean pbStateChanging;


	printf("MN cmd %d sub %d status %d", cmd->nCmd, cmd->nSubCmd, cmd->nStatus);

	if(MM_STATUS_MEDIA_SPEC == cmd->nStatus)
	{
		printf("MM_STATUS_MEDIA_SPEC %x", cmd->pCmdData);
	}

   //if(MM_STATUS_START == cmd->nStatus)
   //{
   //  if(IMedia_Play(pApp->m_pSoundIMedia) != SUCCESS)
   //      printf("SSS m_pSoundIMedia Failed Play");

   //    state = IMedia_GetState(pApp->m_pSoundIMedia, &pbStateChanging);
   //    printf("m_pSoundIMedi@a state = %d", state);
   //}
}

#ifdef USE_BREW_MIXER

void ChannelMediaNotify(void *po, AEEMediaCmdNotify *cmd) {
	channel_info_t *channel_info = (channel_info_t *)po;
  int state;
  boolean pbStateChanging;

	printf("SN cmd %d sub %d status %d", cmd->nCmd, cmd->nSubCmd, cmd->nStatus);

  // if(MM_STATUS_START == cmd->nStatus)
  // {
  //   if(IMedia_Play(pApp->m_pMusicIMedia) != SUCCESS)
  //       printf("SSS m_pMusicIMedia Failed Play");

  //     state = IMedia_GetState(pApp->m_pMusicIMedia, &pbStateChanging);
  //     printf("m_pMusicIMedia state = %d", state);
  // }
   if(MM_STATUS_DONE == cmd->nStatus) {
	   state = IMedia_GetState(channel_info->pIMedia, &pbStateChanging);
       printf("ChannelMediaNotify state = %d", state);
   }
}

#else

void SoundMediaNotify(void *po, AEEMediaCmdNotify *cmd) {
  int state;
  boolean pbStateChanging;

	printf("SN cmd %d sub %d status %d", cmd->nCmd, cmd->nSubCmd, cmd->nStatus);

  /* if(MM_STATUS_START == cmd->nStatus)
   {
     if(IMedia_Play(pApp->m_pMusicIMedia) != SUCCESS)
         printf("SSS m_pMusicIMedia Failed Play");

       state = IMedia_GetState(pApp->m_pMusicIMedia, &pbStateChanging);
       printf("m_pMusicIMedia state = %d", state);
   }*/
  //  if(MM_STATUS_DONE == cmd->nStatus) {
  //      if(IMedia_Play(pApp->m_pSoundIMedia) != SUCCESS)
  //        printf("SSS m_pSoundIMedia Failed Play");

  //      state = IMedia_GetState(pApp->m_pMusicIMedia, &pbStateChanging);
  //      printf("m_pSoundIMedia state = %d", state);
  //  }
}

void StartSoundSession(void) {
  int state;
  boolean pbStateChanging;

  printf("StartSoundSession");

  if(IMedia_Play(pApp->m_pSoundIMedia) != SUCCESS)
    printf("StartSoundSession Failed Play");

  state = IMedia_GetState(pApp->m_pSoundIMedia, &pbStateChanging);
  printf("StartSoundSession state = %d", state);
}

#endif // USE_BREW_MIXER

void I_InitSound(void)
{
  boolean pbStateChanging;
  AEEMediaData md;
  int ret = EFAILED;
  int state;
  //ISourceUtil *psu;

  //CALLBACK_Init(&pApp->m_SoundCallback, (PFNNOTIFY)MediaCallback, pApp);

  lprintf(LO_INFO,"I_InitSound: ");

  // Init music first
  if (!nomusicparm)
    I_InitMusic();

  if(sound_inited)
    I_ShutdownSound();

#ifdef USE_BREW_MIXER

  sound_inited = true;
  printf("Sound Inited (BREW mixer)");

#else

  if(ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_MEDIAADPCM, (void **)(&pApp->m_pSoundIMedia)) != SUCCESS)
    I_Error("Failed to create IMedia instance");

  if(IAudioSource_CreateInstance(&pApp->m_pAudioSource) != SUCCESS)
    I_Error("Failed to create m_pAudioSource");


  // pApp->f = IFILEMGR_OpenFile(pApp->m_pIFileMgr, "sounds_alert.wav", _OFM_READ);
  // if(pApp->f == NULL)
  //   I_Error("wav not found");

  // ret = ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_SOURCEUTIL, (void**)&psu);
  // if(ret != SUCCESS)
  //   I_Error("sourceutil not found");

	// ret = ISOURCEUTIL_SourceFromAStream(psu, (IAStream *)pApp->f, &pApp->source);
  // if(ret != SUCCESS)
  //   I_Error("ISOURCEUTIL_SourceFromAStream not found");

  // ISOURCEUTIL_Release(psu);

  // if(ISOURCEUTIL_PeekFromMemory(pApp->m_pSoundSourceUtil,
  //   pApp->m_SoundBuffer,
  //   pApp->m_SoundBufferSize,
  //   NULL,
  //   NULL,
  //   &pApp->m_pSoundSource) != SUCCESS)
  //   I_Error("Failed to create source from memory");

  // ISource_Readable(pApp->m_pSoundSource, &pApp->m_SoundCallback);

  //ISOURCEUTIL_Release(pApp->m_pSoundSourceUtil);

  // md.clsData = MMD_FILE_NAME;
  // md.pData = (void *)"sounds_alert.wav";
  // md.dwSize = 0;

  SAMPLECOUNT = BREW_AUDIO_BUF_SAMPLES;

  pApp->m_pSoundWavSpec = (AEEMediaWaveSpec *)BREW_malloc(sizeof(AEEMediaWaveSpec));
  pApp->m_pSoundMDE = (AEEMediaDataEx *)BREW_malloc(sizeof(AEEMediaDataEx));

  if(NULL == pApp->m_pSoundMDE || NULL == pApp->m_pSoundWavSpec)
    I_Error("Failed to allocate mde and ws buffer");

  memset(pApp->m_pSoundWavSpec, (int)0, sizeof(AEEMediaWaveSpec));
  pApp->m_pSoundWavSpec->wSize = sizeof(AEEMediaWaveSpec);
  pApp->m_pSoundWavSpec->clsMedia = AEECLSID_MEDIAADPCM;
  pApp->m_pSoundWavSpec->wChannels = 2;
  pApp->m_pSoundWavSpec->dwSamplesPerSec = snd_samplerate;
  pApp->m_pSoundWavSpec->wBitsPerSample = 16;
  pApp->m_pSoundWavSpec->bUnsigned = FALSE;

  pApp->m_pSoundMDE->clsData = MMD_ISOURCE;
  pApp->m_pSoundMDE->pData = (void *)pApp->m_pAudioSource;
  pApp->m_pSoundMDE->dwStructSize = sizeof(AEEMediaDataEx);
  pApp->m_pSoundMDE->dwCaps = 0;
  pApp->m_pSoundMDE->dwSize = 0;
  pApp->m_pSoundMDE->bRaw = TRUE;
  pApp->m_pSoundMDE->dwBufferSize = BREW_AUDIO_BUF_SAMPLES;
  //pApp->m_pSoundMDE->dwBufferSize = 0;
  pApp->m_pSoundMDE->pSpec = pApp->m_pSoundWavSpec;
  pApp->m_pSoundMDE->dwSpecSize = sizeof(AEEMediaWaveSpec);
  //pApp->m_pSoundMDE->pSpec = NULL;
  //pApp->m_pSoundMDE->dwSpecSize = 0;

  if(IMedia_EnableChannelShare(pApp->m_pSoundIMedia, false) != SUCCESS) {
    I_Error("Failed SOUNDEnableChannelShare");
  }

  ret = IMedia_SetMediaDataEx(pApp->m_pSoundIMedia, pApp->m_pSoundMDE, 1);
  if(ret != SUCCESS) {
    I_Error("Failed IMedia_SetMediaDataEx ret = %d", ret);
  }

  state = IMedia_GetState(pApp->m_pSoundIMedia, &pbStateChanging);
  printf("I_InitSound state1 = %d", state);

	if(IMedia_RegisterNotify(pApp->m_pSoundIMedia, (PFNMEDIANOTIFY)SoundMediaNotify, NULL) != SUCCESS) {
	  printf("Failed to register SIMedia notify");
	}

   state = IMedia_GetState(pApp->m_pSoundIMedia, &pbStateChanging);
  printf("I_InitSound state2 = %d", state);

  // if(IMedia_Play(pApp->m_pSoundIMedia) != SUCCESS)
  //   I_Error("Failed to start Play");

  state = IMedia_GetState(pApp->m_pSoundIMedia, &pbStateChanging);
  printf("I_InitSound state3 = %d", state);

  sound_inited = true;
  printf("Sound Inited (DOOM mixer)");

  StartSoundSession();
#endif // USE_BREW_MIXER

  if (first_sound_init) {
    //atexit(I_ShutdownSound);
    first_sound_init = false;
  }

  // Finished initialization.
  lprintf(LO_INFO,"I_InitSound: sound module ready\n");
}

//
// MUSIC API.
//

#ifndef HAVE_OWN_MUSIC

#ifdef HAVE_MIXER
#include "SDL_mixer.h"
#include "mmus2mid.h"

static Mix_Music *music[2] = { NULL, NULL };

char* music_tmp = NULL; /* cph - name of music temporary file */

#endif

void I_ShutdownMusic(void)
{
#ifdef HAVE_MIXER
  if (music_tmp) {
    unlink(music_tmp);
    lprintf(LO_DEBUG, "I_ShutdownMusic: removing %s\n", music_tmp);
    free(music_tmp);
	music_tmp = NULL;
  }
#endif
}

void I_InitMusic(void)
{
  AEEMediaData md;
  AEEMediaMIDISpec ms;
  IFile *f;
  int ret;
  int state;
  boolean pb;

  printf("I_InitMusic");

  if(ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_MEDIAMIDI, (void **)(&pApp->m_pMusicIMedia)) != SUCCESS) {
    I_Error("Failed to create music IMedia instance");
    return;
  }

  //not valid
  /* ret = IMedia_EnableChannelShare(pApp->m_pMusicIMedia, true);
   if(ret != SUCCESS) {
     printf("! MUSIC_EnableChannelShare %d", ret);
   }*/

  // f = IFILEMGR_OpenFile(pApp->m_pIFileMgr, "midi.mid", _OFM_READ);
  // if(f == NULL)
  //   I_Error("Failed to open midi file");

  // pApp->m_pMusicBuffer = BREW_malloc(11591);
  // if(pApp->m_pMusicBuffer == NULL)
  //   I_Error("Failed to allocate buffer midi file");

  // ret = IFILE_Read(f, pApp->m_pMusicBuffer, 11591);
  // if(ret != 11591)
  //   I_Error("Couldnt read 11591");

  // IFILE_Release(f);

  // memset(&ms, (int)0, sizeof(ms));
  // ms.nFormat = 1;
  // ms.nTracks = 1;
  // ms.nDivision = 89;

  // mde.clsData = MMD_BUFFER;
  // mde.pData = (void *)pApp->m_pMusicBuffer;
  // mde.dwStructSize = sizeof(mde);
  // mde.dwCaps = 0;
  // mde.dwSize = 1832;
  // mde.bRaw = FALSE;

  md.clsData = MMD_FILE_NAME;
  md.pData = (void *)"midi.mid";
  // mde.dwStructSize = sizeof(mde);
  // mde.dwCaps = 0;
  // mde.dwSize = 11591;
  // mde.bRaw = FALSE;
  // mde.pSpec = &ms;
  // mde.dwSpecSize = sizeof(ms);

  //mde.dwBufferSize = BREW_AUDIO_BUF_SAMPLES;
  //mde.pSpec = &ws;
  //mde.dwSpecSize = sizeof(ws);


  // md.clsData = MMD_FILE_NAME;
  // md.pData = (void *)pApp->m_pMusicBuffer;
  // md.pData = (void *)"midi.mid";
  // md.dwSize = 1832;
  // md.dwSize = 0;

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_InitMusic state1 = %d", state);

  ret = IMedia_SetMediaData(pApp->m_pMusicIMedia, &md);
  if(ret != SUCCESS)
    I_Error("Error on SMDE for music %d", ret);

  if(IMedia_RegisterNotify(pApp->m_pMusicIMedia, (PFNMEDIANOTIFY)MusicMediaNotify, NULL) != SUCCESS) {
	 printf("Failed to register MIMedia notify");
	}

  // ret = IMedia_SetMediaParm(pApp->m_pMusicIMedia, MM_PARM_PLAY_REPEAT, 0, 0);
  // if(ret != SUCCESS)
  //   printf("Error on IMedia_SetMediaParm %d", ret);

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_InitMusic state2 = %d", state);

  /*ret = IMedia_Play(pApp->m_pMusicIMedia);

   if(ret != SUCCESS) {
     printf("!!! ERRPLAYH %d", ret);
   }

   state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
   printf("I_InitMusic state2 = %d", state);*/
}

void I_PlaySong(int handle, int looping)
{
  boolean pb;
  int state;
  int ret;

  printf("I_PlaySong handle = %d loop = %d", handle, looping);

  return;

  if(pApp->m_pMusicIMedia == NULL)
    return;

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_PlaySong state = %d", state);

  if(state == MM_STATE_READY) {
    printf("Will play!");
    ret = IMedia_Play(pApp->m_pMusicIMedia);

    if(ret != SUCCESS) {
      printf("!!! ERRPLAYH %d", ret);
    }
  }

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_PlaySong AFTER state = %d", state);
}

extern int mus_pause_opt; // From m_misc.c

void I_PauseSong (int handle)
{
  printf("I_PauseSong");

#ifdef HAVE_MIXER
  switch(mus_pause_opt) {
  case 0:
      I_StopSong(handle);
    break;
  case 1:
      Mix_PauseMusic();
    break;
  }
#endif
  // Default - let music continue
}

void I_ResumeSong (int handle)
{
  printf("I_ResumeSong");
#ifdef HAVE_MIXER
  switch(mus_pause_opt) {
  case 0:
      I_PlaySong(handle,1);
    break;
  case 1:
      Mix_ResumeMusic();
    break;
  }
#endif
  /* Otherwise, music wasn't stopped */
}

void I_StopSong(int handle)
{
  boolean pb;
  int state;

  return;

  printf("I_StopSong handle = %d", handle);

  if(pApp->m_pMusicIMedia == NULL)
    return;

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_StopSong will stop state = %d", state);

  if(state == MM_STATE_PLAY || state == MM_STATE_PLAY_PAUSE)
    IMedia_Stop(pApp->m_pMusicIMedia);
}

void I_UnRegisterSong(int handle)
{
  boolean pb;
  int state;

  return;

  printf("I_UnRegisterSong handle %d", handle);

  if(pApp->m_pMusicIMedia) {
    state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
    printf("I_UnRegisterSong will stop state = %d", state);

    if(state == MM_STATE_PLAY || state == MM_STATE_PLAY_PAUSE)
      IMedia_Stop(pApp->m_pMusicIMedia);

    IMedia_Release(pApp->m_pMusicIMedia);
    pApp->m_pMusicIMedia = NULL;
  }

  printf("Free!");
  free(pApp->m_pMusicBuffer);
  printf("After Free!");
}

// does MIDI if I_RegisterMusic can't register a HQ song
int I_RegisterSong(const void *data, size_t len)
{
  AEEMediaData md;
  MIDI *mididata;
  boolean pb;
  int ret = EFAILED;
  int state;

  return 0;

  if(pApp->m_pMusicIMedia)
    I_Error("Music IMedia already allocated");

  if(ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_MEDIAMIDI, (void **)(&pApp->m_pMusicIMedia)) != SUCCESS)
    I_Error("Failed to create music IMedia instance");

  printf("I_RegisterSong data %x len %d", data, len);

  // ret = IMedia_EnableChannelShare(pApp->m_pMusicIMedia, TRUE);
  // if(ret != SUCCESS)
  //   I_Error("Failed music IMedia_EnableChannelShare %d", ret);

  if ( len < 32 )
    return 0; // the data should at least as big as the MUS header

  /* Convert MUS chunk to MIDI? */
  if ( memcmp(data, "MUS", 3) == 0 )
  {
    int midlen;

    mididata = malloc(sizeof(MIDI));

    if(mididata == NULL)
      I_Error("Failed to allocate mididata");

    mmus2mid(data, mididata, 89, 0);
    MIDIToMidi(mididata, &pApp->m_pMusicBuffer, &pApp->m_iMusicBufferSize);

    free_mididata(mididata);
    free(mididata);
  } else {
    printf("not MUS!");
    pApp->m_pMusicBuffer = NULL;
    // pApp->m_iMusicBufferSize = len;
  }

  if ( pApp->m_pMusicBuffer == NULL )
    I_Error("Couldn't load MIDI");

  md.clsData = MMD_BUFFER;
  md.pData = (void *)pApp->m_pMusicBuffer;
  md.dwSize = pApp->m_iMusicBufferSize;

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_RegisterSong state1 = %d", state);

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_RegisterSong state2 = %d", state);

  ret = IMedia_SetMediaData(pApp->m_pMusicIMedia, &md);
  if(ret != SUCCESS)
    I_Error("Error on IMedia_SetMediaData for music %d", ret);

  if(IMedia_RegisterNotify(pApp->m_pMusicIMedia, (PFNMEDIANOTIFY)MusicMediaNotify, NULL) != SUCCESS) {
	  printf("Failed to register music IMedia notify");
	}

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_RegisterSong state3 = %d", state);

  return 0;
}

// cournia - try to load a music file into SDL_Mixer
//           returns true if could not load the file
int I_RegisterMusic( const char* filename, musicinfo_t *song )
{
  AEEMediaData md;
  IFile *f;
  int ret;
  int state;
  boolean pb;

  printf("I_RegisterMusic");

  if(ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_MEDIAMP3, (void **)(&pApp->m_pMusicIMedia)) != SUCCESS) {
    I_Error("Failed to create music IMedia instance");
    return;
  }

  ret = IMedia_EnableChannelShare(pApp->m_pMusicIMedia, true);
  if(ret != SUCCESS) {
    printf("! MUSIC_EnableChannelShare %d", ret);
  }

  md.clsData = MMD_FILE_NAME;
  md.pData = (void *)filename;

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_InitMusic state1 = %d", state);

  ret = IMedia_SetMediaData(pApp->m_pMusicIMedia, &md);
  if(ret != SUCCESS)
    I_Error("Error on SMDE for music %d", ret);

  if(IMedia_RegisterNotify(pApp->m_pMusicIMedia, (PFNMEDIANOTIFY)MusicMediaNotify, NULL) != SUCCESS) {
	 printf("Failed to register MIMedia notify");
	}

  // ret = IMedia_SetMediaParm(pApp->m_pMusicIMedia, MM_PARM_PLAY_REPEAT, 0, 0);
  // if(ret != SUCCESS)
  //   printf("Error on IMedia_SetMediaParm %d", ret);

  state = IMedia_GetState(pApp->m_pMusicIMedia, &pb);
  printf("I_InitMusic state2 = %d", state);

  return 0;
}

void I_SetMusicVolume(int volume)
{
#ifdef HAVE_MIXER
  Mix_VolumeMusic(volume*8);
#endif
}

#endif /* HAVE_OWN_MUSIC */

