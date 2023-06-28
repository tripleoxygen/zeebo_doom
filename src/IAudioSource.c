/*
 *  SPDX-FileCopyrightText: Copyright 2012-2023 Fausto "O3" Ribeiro | OpenZeebo <zeebo@tripleoxygen.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "IAudioSource.h"
#include "brew.h"
#include "i_sound.h"

uint32 IAUDIOSOURCE_AddRef(IAudioSource *p)
{
    return 1;
}

uint32 IAUDIOSOURCE_Release(IAudioSource *p)
{
    IAudioSource *pme = (IAudioSource *)p;

    FREE_VTBL(pme, IAudioSource);
    BREW_free(pme);

    return 0;
}

int IAUDIOSOURCE_QueryInterface(IAudioSource *p, AEEIID i, void **o)
{
    return SUCCESS;
}

int32 IAUDIOSOURCE_Read(IAudioSource *p, char *pcBuf, int32 cbBuf)
{
    I_UpdateSound(NULL, (int *)pcBuf, cbBuf);

    return cbBuf;
}

void IAUDIOSOURCE_Readable(IAudioSource *p, AEECallback *pcb)
{
    return;
}

int IAudioSource_CreateInstance(IAudioSource **ppobj)
{
    IAudioSource *ias = NULL;
    VTBL(IAudioSource) * appFuncs;

    if (!ppobj)
        return ENOMEMORY;

    *ppobj = NULL;

    if (NULL == (ias = (IAudioSource *)BREW_malloc(sizeof(IAudioSource) + sizeof(VTBL(IAudioSource)))))
        return ENOMEMORY;

    appFuncs = (VTBL(IAudioSource) *)((byte *)ias + sizeof(IAudioSource));

    appFuncs->AddRef = IAUDIOSOURCE_AddRef;
    appFuncs->Release = IAUDIOSOURCE_Release;
    appFuncs->Read = IAUDIOSOURCE_Read;
    appFuncs->Readable = IAUDIOSOURCE_Readable;
    appFuncs->QueryInterface = IAUDIOSOURCE_QueryInterface;

    INIT_VTBL(ias, IAudioSource, *appFuncs);

    *ppobj = (IAudioSource *)ias;

    return SUCCESS;
}
