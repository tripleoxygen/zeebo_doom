/*
 *  SPDX-FileCopyrightText: Copyright 2012-2023 Fausto "O3" Ribeiro | OpenZeebo <zeebo@tripleoxygen.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _IAUDIOSOURCE_H_
#define _IAUDIOSOURCE_H_

#include "AEE.h"
#include "AEEISource.h"
#include "AEEInterface.h"

typedef struct _IAudioSource IAudioSource;

#define INHERIT_IAudioSource(iname) \
    INHERIT_ISource(iname)

QINTERFACE(IAudioSource)
{
   INHERIT_IAudioSource(IAudioSource);
};

#define IAudioSource_AddRef(p) GET_PVTBL(p, IAudioSource)->AddRef(p)
#define IAudioSource_Release(p) GET_PVTBL(p, IAudioSource)->Release(p)
#define IAudioSource_QueryInterface(p, i, o) GET_PVTBL(p, IAudioSource)->QueryInterface(p, i, o)
#define IAudioSource_Readable(p, pcb) GET_PVTBL(p, IAudioSource)->Readable(p, pcb)
#define IAudioSource_Read(p, b, c) GET_PVTBL(p, IAudioSource)->Read(p, b, c)

int IAudioSource_CreateInstance(IAudioSource **ppobj);

#endif //  _AUDIO_SOURCE_H_
