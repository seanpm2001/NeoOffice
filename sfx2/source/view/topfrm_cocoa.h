/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Patrick Luby, May 2011
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2011 Planamesa Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/
 
#ifndef __TOPFRM_COCOA_H__
#define __TOPFRM_COCOA_H__

#if defined __cplusplus && !defined __OBJC__
#include <premac.h>
#include <CoreFoundation/CoreFoundation.h>
#include <postmac.h>
class NSView;
#elif defined __OBJC__
class SfxTopViewFrame;
#endif

#ifdef __cplusplus
extern "C" {
#endif
::rtl::OUString NSDocument_revertToSavedLocalizedString();
::rtl::OUString NSDocument_saveAVersionLocalizedString();
BOOL NSDocument_versionsEnabled();
BOOL NSDocument_versionsSupported();
void SFXDocument_createDocument( SfxTopViewFrame *pFrame, NSView *pView, CFURLRef aURL, BOOL bReadOnly );
BOOL SFXDocument_hasDocument( SfxTopViewFrame *pFrame );
void SFXDocument_releaseDocument( SfxTopViewFrame *pFrame );
void SFXDocument_reload( SfxTopViewFrame *pFrame );
void SFXDocument_revertDocumentToSaved( SfxTopViewFrame *pFrame );
void SFXDocument_saveVersionOfDocument( SfxTopViewFrame *pFrame );
#ifdef __cplusplus
}
#endif

#endif
