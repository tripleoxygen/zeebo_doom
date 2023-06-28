/*
 *  SPDX-FileCopyrightText: Copyright 2012-2023 Fausto "O3" Ribeiro | OpenZeebo <zeebo@tripleoxygen.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __ZEEBO__
#define __ZEEBO__

#include "AEEAppGen.h"
#include "AEEFile.h"
#include "AEEHIDThumbsticks.h"
#include "AEEIHID.h"
#include "AEEISignal.h"
#include "AEEISignalCBFactory.h"
#include "AEEIMedia.h"
#include "AEESource.h"
#include "IAudioSource.h"
#include "AEEMediaFormats.h"
#include <gles/egl.h>
#include <gles/glESext.h>
#include <gles/EGLext.h>

#define AEECLSID_PRBOOM 0x0103D666

#define SAFE_FREE(p) if ((p) != NULL) BREW_free(p); (p)=NULL;

typedef struct _PrBoomApp
{
  AEEApplet a;
  AEEDeviceInfo m_DeviceInfo;
  IFileMgr *m_pIFileMgr;
  IHID *m_IHID;
  IHIDDevice *m_IHIDDevice;
  AEEHIDThumbstickMapping *m_IHIDThumbstick;
  IDIB *m_Framebuffer;
  ISignalCBFactory *m_SignalFactory;
  ISignalCtl *m_SignalThumbstick;
  ISignalCtl *m_SignalButton;
  boolean *m_JoystickButtonStates;
  int axis_nX, axis_nZ, axis_nY;
  int keyboard_nX, keyboard_nY;

#ifndef USE_BREW_MIXER
  IMedia *m_pSoundIMedia;
  AEEMediaWaveSpec *m_pSoundWavSpec;
  AEEMediaDataEx *m_pSoundMDE;
  IAudioSource *m_pAudioSource;
#endif

  IMedia *m_pMusicIMedia;
  byte *m_pMusicBuffer;
  int m_iMusicBufferSize;

  // IFile *f;
  // ISource *source;

  EGLDisplay eglDisplay;
  EGLSurface eglSurface;
  EGLContext eglContext;

  PFNGLDRAWTEXIVOESPROC glDrawTexivOES;
  PFNGLTEXPARAMETERIVPROC glTexParameteriv;

  PFNEGLSURFACEROTATEENABLEQUALCOMMPROC eglSurfaceRotateEnable;
  PFNEGLSETSURFACEROTATEQUALCOMMPROC eglSetSurfaceRotate;

  GLuint nTexureId;
} PrBoomApp;

extern PrBoomApp *pApp;

void PrBoomApp_Exit(void);

#endif
