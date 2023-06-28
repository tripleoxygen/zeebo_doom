/*
 *  SPDX-FileCopyrightText: Copyright 2012-2023 Fausto "O3" Ribeiro | OpenZeebo <zeebo@tripleoxygen.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brew_blit.h"
#include "brew.h"
#include "AEEStdLib.h"
#include "AEEFile.h"
#include "prboom.h"

void BREW_Blit(void *fb, int width, int height, int depth)
{
  GLenum err = GL_NO_ERROR;

  GLint pScreenCoords[5] = {
      (int)((640 / 2) - (640 / 2)),
      (int)((480 / 2) - (400 / 2)),
      0,
      640,
      400};

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGB,
      512,
      256,
      0,
      GL_RGB,
      GL_UNSIGNED_SHORT_5_6_5,
      fb
  );

  pApp->glDrawTexivOES(pScreenCoords);

  eglSwapBuffers(pApp->eglDisplay, pApp->eglSurface);
}
