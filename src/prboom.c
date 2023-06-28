/*
 *  SPDX-FileCopyrightText: Copyright 2012-2023 Fausto "O3" Ribeiro | OpenZeebo <zeebo@tripleoxygen.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "AEEHID.bid"
#include "AEEStdLib.h"
#include "AEEHIDDevice_Joystick.h"
#include "AEEHIDButtons.h"
#include <EGL_1x.h>
#include <GLES_1x.h>
#include <gles/egl.h>
#include <gles/gl.h>
#include <gles/glESext.h>
#include "prboom.h"
#include "brew.h"
#include "i_joy.h"
#include "mathfixed.h"

#define AEECLSID_SignalCBFactory 0x01041207

PrBoomApp *pApp = NULL;

unsigned char version[] = "v0.4 - Zeebo Version - OpenZeebo - Fausto \"O3\" Ribeiro";

void D_DoomMain(void);

int AEEClsCreateInstance(AEECLSID ClsId, IShell *pIShell, IModule *pMod,
                         void **ppObj);
static boolean PrBoomApp_InitAppData(PrBoomApp *pApp);
static void PrBoomApp_FreeAppData(PrBoomApp *pApp);
static boolean PrBoomApp_HandleEvent(PrBoomApp *pApp, AEEEvent eCode,
                                     uint16 wParam, uint32 dwParam);
boolean InitGL();
boolean SetupGL();
void AddThumbstickToQueue(void *);
void AddButtonToQueue(void *);
void FreeAudio(void);

void AddThumbstickToQueue(void *param)
{
  AEEResult ret = EFAILED;
  AEEHIDPositionInfo pi;

  // ISignalCtl_Enable(pApp->m_SignalThumbstick);

  if (IHIDDevice_GetPositionState(pApp->m_IHIDDevice, &pi) != SUCCESS)
  {
    printf("IHIDDevice_GetPositionState failed");
    return;
  }

  printf("pi rel %d nX %d nY %d nZ %d",
         pi.bRelativeAxes,
         pi.nX,
         pi.nY,
         pi.nZ);
}

void AddButtonToQueue(void *param)
{
  AEEResult ret = EFAILED;
  AEEHIDButtonInfo bi;
  uint32 pdwTimestamp;
  boolean pbDroppedEvents;

  // ISignalCtl_Enable(pApp->m_SignalButton);

  if (AEEHIDButton_GetNextButtonEvent(pApp->m_IHIDDevice, &bi, &pdwTimestamp, &pbDroppedEvents) != SUCCESS)
  {
    printf("AEEHIDButton_GetNextButtonEvent failed");
    return;
  }

  printf("bi ts %u dre %d",
         pdwTimestamp,
         pbDroppedEvents);
  printf("bi id %d st %d uid %d",
         bi.nButtonID,
         bi.nState,
         bi.nButtonUID);
}

int AEEClsCreateInstance(AEECLSID ClsId, IShell *pIShell, IModule *pMod,
                         void **ppObj)
{
  *ppObj = NULL;

  if (ClsId == AEECLSID_PRBOOM)
  {
    // Create the applet and make room for the applet structure
    if (AEEApplet_New(sizeof(PrBoomApp),
                      ClsId,
                      pIShell,
                      pMod,
                      (IApplet **)ppObj,
                      (AEEHANDLER)PrBoomApp_HandleEvent,
                      (PFNFREEAPPDATA)PrBoomApp_FreeAppData)) // the FreeAppData function is called after sending EVT_APP_STOP to the HandleEvent function
    {
      pApp = (PrBoomApp *)*ppObj;

      // Initialize applet data, this is called before sending EVT_APP_START
      //  to the HandleEvent function
      if (PrBoomApp_InitAppData(pApp))
      {
        // Data initialized successfully
        return (AEE_SUCCESS);
      }
      else
      {
        // Release the applet. This will free the memory allocated for the applet when
        //  AEEApplet_New was called.
        IAPPLET_Release((IApplet *)*ppObj);
        return EFAILED;
      }
    } // end AEEApplet_New
  }

  return (EFAILED);
}

static boolean PrBoomApp_InitAppData(PrBoomApp *pApp)
{
  AEEResult ret;
  int nConnectedDevices;
  int *pDevHandles;
  int nErr;
  AEEHIDDeviceInfo pDevInfo;

  (void)version;

  pApp->m_DeviceInfo.wStructSize = sizeof(pApp->m_DeviceInfo);
  ISHELL_GetDeviceInfo(pApp->a.m_pIShell, &pApp->m_DeviceInfo);

  pApp->m_pMusicBuffer = NULL;
  pApp->m_pMusicIMedia = NULL;
#ifndef USE_BREW_MIXER
  pApp->m_pSoundIMedia = NULL;
#endif

  pApp->eglDisplay = EGL_NO_DISPLAY;
  pApp->eglSurface = EGL_NO_SURFACE;
  pApp->eglContext = EGL_NO_CONTEXT;

  if (ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_FILEMGR,
                            (void **)(&pApp->m_pIFileMgr)) != SUCCESS)
  {
    DBGPRINTF("[-] Failed to instantiate IFileMgr.");
    return FALSE;
  }

  if (!InitGL())
  {
    printf("Failed to InitGL");
    return (FALSE);
  }

  if (!SetupGL())
  {
    printf("Failed to SetupGL");
    return (FALSE);
  }

#ifndef AEE_SIMULATOR
  if (ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_HID,
                            (void **)(&pApp->m_IHID)) != SUCCESS)
  {
    DBGPRINTF("[-] Failed to instantiate AEECLSID_HID.");
    return FALSE;
  }

  if (ISHELL_CreateInstance(pApp->a.m_pIShell, AEECLSID_SignalCBFactory,
                            (void **)(&pApp->m_SignalFactory)) != SUCCESS)
  {
    DBGPRINTF("[-] Failed to instantiate AEECLSID_SignalCBFactory.");
    return FALSE;
  }

  nErr = IHID_GetConnectedDevices(pApp->m_IHID, AEEUID_HID_Joystick_Device,
                                  NULL, 0, &nConnectedDevices);
  if ((SUCCESS == nErr) && (nConnectedDevices != 0))
  {
    pDevHandles = (int *)MALLOC(nConnectedDevices * sizeof(int));
    if (NULL != pDevHandles)
    {
      nErr = IHID_GetConnectedDevices(pApp->m_IHID, AEEUID_HID_Joystick_Device,
                                      pDevHandles, nConnectedDevices,
                                      &nConnectedDevices);
      if (SUCCESS == nErr)
      {
        int nIndex;
        for (nIndex = 0; nIndex < nConnectedDevices; nIndex++)
        {
          if (IHID_GetDeviceInfo(pApp->m_IHID, pDevHandles[nIndex],
                                 &pDevInfo) == AEE_SUCCESS)
          {
            printf("HID %u [%x:%x]", pDevInfo.nDeviceType, pDevInfo.wVendorID,
                   pDevInfo.wProductID);
          }
        }

        if (IHID_CreateDevice(pApp->m_IHID, pDevHandles[0],
                              &pApp->m_IHIDDevice) != AEE_SUCCESS)
        {
          printf("Failed to IHID_CreateDevice");
        }
      }
      FREE(pDevHandles);
    }
  }

  if (pApp->m_IHIDDevice)
  {
    /*
    ret = AEEHIDThumbstick_InitializeMapping(pApp->m_IHIDDevice, &pApp->m_IHIDThumbstick);

    if (ret != AEE_SUCCESS)
    {
        printf("Failed AEEHIDThumbstick_InitializeMapping");
        printf("err = %d", ret);
    }
    
    ret = ISignalCBFactory_CreateSignal(
        pApp->m_SignalFactory,
        AddThumbstickToQueue,
        NULL,
        (ISignal **)0,
        &pApp->m_SignalThumbstick);

    if (ret != AEE_SUCCESS)
    {
      printf("Failed ISignalCBFactory_CreateSignal Thumbstick");
    }

    ret = ISignalCBFactory_CreateSignal(
        pApp->m_SignalFactory,
        AddButtonToQueue,
        NULL,
        (ISignal **)0,
        &pApp->m_SignalButton);

    if (ret != AEE_SUCCESS)
    {
      printf("Failed ISignalCBFactory_CreateSignal Button");
    }


    if(ret == AEE_SUCCESS) {
      ret = IHIDDevice_RegisterForPositionChange(pApp->m_IHIDDevice,
                                                 (ISignal *)pApp->m_SignalThumbstick);
      if (ret != AEE_SUCCESS)
      {
        printf("Failed IHIDDevice_RegisterForPositionChange");
      }

      ret = IHIDDevice_RegisterForButtonEvent(pApp->m_IHIDDevice,
                                                 (ISignal *)pApp->m_SignalButton);
      if (ret != AEE_SUCCESS)
      {
        printf("Failed IHIDDevice_RegisterForButtonEvent");
      }
    }
  */
  }
#endif

  pApp->m_JoystickButtonStates = (boolean *)MALLOC(AEEHIDBUTTON_NUM_OF_BUTTONS * sizeof(boolean));
  if (pApp->m_JoystickButtonStates == NULL)
  {
    printf("Failed to allocate button states");
    return FALSE;
  }
  BREW_memset(pApp->m_JoystickButtonStates, '\0', AEEHIDBUTTON_NUM_OF_BUTTONS * sizeof(boolean));

  return TRUE;
}

void FreeGLSurface()
{
  if (pApp->eglDisplay != EGL_NO_DISPLAY)
  {
    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    eglDestroyContext(pApp->eglDisplay, pApp->eglContext);
    eglDestroySurface(pApp->eglDisplay, pApp->eglSurface);
    eglTerminate(pApp->eglDisplay);
    pApp->eglDisplay = EGL_NO_DISPLAY;

    GLES_Release();
    EGL_Release();
  }
}

boolean InitGL()
{
  EGLint numConfigs = 1;
  EGLConfig myConfig = {0};
  EGLint attrib[] = {
      EGL_RED_SIZE, 5, EGL_GREEN_SIZE, 6, EGL_BLUE_SIZE, 5,
      EGL_DEPTH_SIZE, 16, EGL_STENCIL_SIZE, 0, EGL_NONE};
  IBitmap *pIBitmapDDB;
  IDIB *pDIB;
  PFNGLBINDFRAMEBUFFEROESPROC glBindFramebufferOES;

  if (EGL_Init(pApp->a.m_pIShell) != SUCCESS)
  {
    return FALSE;
  }

  if (GLES_Init(pApp->a.m_pIShell) != SUCCESS)
  {
    return FALSE;
  }

  if ((pApp->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) ==
      EGL_NO_DISPLAY)
  {
    return FALSE;
  }

  if (eglInitialize(pApp->eglDisplay, NULL, NULL) != EGL_TRUE)
  {
    printf("eglInitialize failed");
    return FALSE;
  }

  if (eglChooseConfig(pApp->eglDisplay, attrib, &myConfig, 1, &numConfigs) !=
      EGL_TRUE)
  {
    printf("eglChooseConfig failed");
    return FALSE;
  }

  if (IDISPLAY_GetDeviceBitmap(pApp->a.m_pIDisplay, &pIBitmapDDB) != SUCCESS)
  {
    return FALSE;
  }

  if (IBITMAP_QueryInterface(pIBitmapDDB, AEECLSID_DIB, (void **)&pDIB) !=
      SUCCESS)
  {
    return FALSE;
  }

  if ((pApp->eglSurface = eglCreateWindowSurface(pApp->eglDisplay, myConfig,
                                                 (NativeWindowType)pDIB, 0)) ==
      EGL_NO_SURFACE)
  {
    printf("eglCreateWindowSurface failed");
    return FALSE;
  }

  IDIB_Release(pDIB);
  IBITMAP_Release(pIBitmapDDB);

  if ((pApp->eglContext = eglCreateContext(pApp->eglDisplay, myConfig, 0, 0)) ==
      EGL_NO_CONTEXT)
  {
    return FALSE;
  }

  if (eglMakeCurrent(pApp->eglDisplay, pApp->eglSurface, pApp->eglSurface,
                     pApp->eglContext) != EGL_TRUE)
  {
    return FALSE;
  }

  return TRUE;
}

boolean SetupGL(void)
{
  char *pszExtensions;
  GLenum err = GL_NO_ERROR;
  int crop_rect[] = {
      0,
      0,
      320,
      200,
  };

  pszExtensions = (char *)glGetString(GL_EXTENSIONS);

  if (strstr(pszExtensions, "GL_OES_draw_texture") == NULL)
  {
    printf("GL_OES_draw_texture not found");
    return FALSE;
  }

  pApp->glDrawTexivOES =
      (PFNGLDRAWTEXIVOESPROC)eglGetProcAddress("glDrawTexivOES");

  pApp->glTexParameteriv =
      (PFNGLTEXPARAMETERIVPROC)eglGetProcAddress("glTexParameteriv");

  pApp->eglSurfaceRotateEnable =
      (PFNEGLSURFACEROTATEENABLEQUALCOMMPROC)eglGetProcAddress(
          "eglSurfaceRotateEnableQUALCOMM");

  pApp->eglSetSurfaceRotate =
      (PFNEGLSETSURFACEROTATEQUALCOMMPROC)eglGetProcAddress(
          "eglSetSurfaceRotateQUALCOMM");

  if (pApp->glDrawTexivOES == NULL || pApp->glTexParameteriv == NULL || pApp->eglSurfaceRotateEnable == NULL || pApp->eglSetSurfaceRotate == NULL)
  {
    printf("gl funcs not found");
    return FALSE;
  }

  pApp->eglSurfaceRotateEnable(pApp->eglDisplay, pApp->eglSurface, true);
  pApp->eglSetSurfaceRotate(pApp->eglDisplay, pApp->eglSurface, false, false,
                            true);

  glViewport(0, 0, (GLsizei)pApp->m_DeviceInfo.cxScreen,
             (GLsizei)pApp->m_DeviceInfo.cyScreen);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColorx(F_ZERO, F_ZERO, F_ZERO, F_ONE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &pApp->nTexureId);
  glBindTexture(GL_TEXTURE_2D, pApp->nTexureId);

  glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  pApp->glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop_rect);

  err = glGetError();
  if (err != GL_NO_ERROR)
  {
    DBGPRINTF("glTexParameteriv glGetError = %x", err);
  }

  return TRUE;
}

static boolean PrBoomApp_HandleEvent(PrBoomApp *pApp, AEEEvent eCode,
                                     uint16 wParam, uint32 dwParam)
{
  int ret;

  switch (eCode)
  {
  case EVT_APP_START:
    D_DoomMain();
    return (TRUE);

  case EVT_APP_STOP:
    return (TRUE);

  case EVT_APP_SUSPEND:
  case EVT_APP_RESUME:
    PrBoomApp_Exit();

    return TRUE;

  case EVT_KEY_PRESS:
    I_EventFromKey(pApp, wParam, true);

    return TRUE;

  case EVT_KEY_RELEASE:
    I_EventFromKey(pApp, wParam, false);

    return TRUE;

  case EVT_CHAR:
    return TRUE;

  default:
    break;
  }

  return (FALSE);
}

static void PrBoomApp_FreeAppData(PrBoomApp *pApp)
{
  ISHELL_CancelTimer(pApp->a.m_pIShell, NULL, NULL);

  if (pApp->m_IHIDDevice)
  {
    IHIDDevice_Release(pApp->m_IHIDDevice);
  }

  if (pApp->m_IHID)
  {
    IHID_Release(pApp->m_IHID);
  }

  SAFE_FREE(pApp->m_JoystickButtonStates);

  FreeAudio();
  FreeGLSurface();
}

void FreeAudio(void)
{
  if (pApp->m_pMusicIMedia)
  {
    IMedia_Stop(pApp->m_pMusicIMedia);
    IMedia_Release(pApp->m_pMusicIMedia);
  }

#ifndef USE_BREW_MIXER
  if (pApp->m_pSoundIMedia)
  {
    IMedia_Stop(pApp->m_pSoundIMedia);
    IMedia_Release(pApp->m_pSoundIMedia);
  }

  if (pApp->m_pAudioSource)
  {
    IAudioSource_Release(pApp->m_pAudioSource);
  }

  SAFE_FREE(pApp->m_pSoundMDE);
  SAFE_FREE(pApp->m_pSoundWavSpec);
#endif

  SAFE_FREE(pApp->m_pMusicBuffer);
}

void PrBoomApp_Exit(void)
{
  ISHELL_CancelTimer(pApp->a.m_pIShell, NULL, NULL);
  ISHELL_CloseApplet(pApp->a.m_pIShell, FALSE);
}
