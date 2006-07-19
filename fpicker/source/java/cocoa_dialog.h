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
 *  Patrick Luby, July 2006
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2006 by Patrick Luby (patrick.luby@planamesa.com)
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

#ifndef _COCOA_FILEDIALOG_H_
#define _COCOA_FILEDIALOG_H_

#ifdef __cplusplus
#include <tools/solar.h>

#include <premac.h>
#include <CoreFoundation/CoreFoundation.h>
#include <postmac.h>

typedef void* id;
#endif

#ifdef __cplusplus
BEGIN_C
#endif
id NSFileDialog_create( BOOL bUseFileOpenDialog, BOOL bChooseFiles, BOOL bShowAutoExtension, BOOL bShowFilterOptions, BOOL bShowImageTemplate, BOOL bShowLink, BOOL bShowPassword, BOOL bShowPreview, BOOL bShowReadOnly, BOOL bShowSelection, BOOL bShowTemplate, BOOL bShowVersion );
CFStringRef NSFileDialog_directory( void *pDialog );
CFStringRef *NSFileDialog_fileNames( void *pDialog );
BOOL NSFileDialog_isExtensionHidden( void *pDialog );
void NSFileDialog_release( void *pDialog );
void NSFontManager_releaseFileNames( CFStringRef *pURLs );
int NSFileDialog_result( id pDialog );
int NSFileDialog_showFileDialog( id pDialog );
void NSFileDialog_setDirectory( void *pDialog, CFStringRef aDirectory );
void NSFileDialog_setExtensionHidden( void *pDialog, BOOL bExtensionHidden );
void NSFileDialog_setMultiSelectionMode( void *pDialog, BOOL bMultiSelectionMode );
void NSFileDialog_setTitle( void *pDialog, CFStringRef aTitle );
#ifdef __cplusplus
END_C
#endif

#endif	// _COCOA_FILEDIALOG_H_
