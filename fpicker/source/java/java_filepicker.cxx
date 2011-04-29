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
 *		 - GNU General Public License Version 2.1
 *
 *  Patrick Luby, July 2006
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2006 Planamesa Inc.
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

#include <stdio.h>

#ifndef _JAVA_FILEPICKER_HXX_
#include "java_filepicker.hxx"
#endif
#ifndef _JAVA_SERVICE_HXX_
#include "java_service.hxx"
#endif
#ifndef _COM_SUN_STAR_LANG_NULLPOINTEREXCEPTION_HPP_
#include <com/sun/star/lang/NullPointerException.hpp>
#endif
#ifndef _COM_SUN_STAR_UI_DIALOGS_COMMONFILEPICKERELEMENTIDS_HPP_
#include <com/sun/star/ui/dialogs/CommonFilePickerElementIds.hpp>
#endif
#ifndef _COM_SUN_STAR_UI_DIALOGS_CONTROLACTIONS_HPP_
#include <com/sun/star/ui/dialogs/ControlActions.hpp>
#endif
#ifndef  _COM_SUN_STAR_UI_DIALOGS_EXTENDEDFILEPICKERELEMENTIDS_HPP_
#include <com/sun/star/ui/dialogs/ExtendedFilePickerElementIds.hpp>
#endif
#ifndef _COM_SUN_STAR_UI_DIALOGS_TEMPLATEDESCRIPTION_HPP_
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#endif
#ifndef _SV_SVAPP_HXX
#include <vcl/svapp.hxx>
#endif
#ifndef _VOS_MUTEX_HXX_
#include <vos/mutex.hxx>
#endif

#include <premac.h>
#include <CoreFoundation/CoreFoundation.h>
#include <postmac.h>

#include "svtools/svtools.hrc"
#include "svtools/filedlg2.hrc"
#include "cocoa_dialog.h"

using namespace cppu;
using namespace com::sun::star::beans;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace com::sun::star::ui::dialogs;
using namespace rtl;
using namespace vos;
using namespace java;

static ::std::list< ::java::JavaFilePicker* > aFilePickers;

// ========================================================================

CocoaControlID GetCocoaControlId( sal_Int16 nControlId )
{
	CocoaControlID nRet = MAX_COCOA_CONTROL_ID;

	switch ( nControlId )
	{
		case ExtendedFilePickerElementIds::CHECKBOX_AUTOEXTENSION:
			nRet = COCOA_CONTROL_ID_AUTOEXTENSION;
			break;
		case ExtendedFilePickerElementIds::CHECKBOX_FILTEROPTIONS:
			nRet = COCOA_CONTROL_ID_FILTEROPTIONS;
			break;
		case ExtendedFilePickerElementIds::LISTBOX_IMAGE_TEMPLATE:
		case ExtendedFilePickerElementIds::LISTBOX_IMAGE_TEMPLATE_LABEL:
			nRet = COCOA_CONTROL_ID_IMAGE_TEMPLATE;
			break;
		case ExtendedFilePickerElementIds::CHECKBOX_LINK:
			nRet = COCOA_CONTROL_ID_LINK;
			break;
		case ExtendedFilePickerElementIds::CHECKBOX_PASSWORD:
			nRet = COCOA_CONTROL_ID_PASSWORD;
			break;
		case ExtendedFilePickerElementIds::PUSHBUTTON_PLAY:
			nRet = COCOA_CONTROL_ID_PLAY;
			break;
		case ExtendedFilePickerElementIds::CHECKBOX_PREVIEW:
			nRet = COCOA_CONTROL_ID_PREVIEW;
			break;
		case ExtendedFilePickerElementIds::CHECKBOX_READONLY:
			nRet = COCOA_CONTROL_ID_READONLY;
			break;
		case ExtendedFilePickerElementIds::CHECKBOX_SELECTION:
			nRet = COCOA_CONTROL_ID_SELECTION;
			break;
		case ExtendedFilePickerElementIds::LISTBOX_TEMPLATE:
		case ExtendedFilePickerElementIds::LISTBOX_TEMPLATE_LABEL:
			nRet = COCOA_CONTROL_ID_TEMPLATE;
			break;
		case ExtendedFilePickerElementIds::LISTBOX_VERSION:
		case ExtendedFilePickerElementIds::LISTBOX_VERSION_LABEL:
			nRet = COCOA_CONTROL_ID_VERSION;
			break;
		case CommonFilePickerElementIds::LISTBOX_FILTER:
		case CommonFilePickerElementIds::LISTBOX_FILTER_LABEL:
			nRet = COCOA_CONTROL_ID_FILETYPE;
			break;
		default:
			break;
	}

	return nRet;
}

// ------------------------------------------------------------------------

void JavaFilePicker_controlStateChanged( int nID, void *pPicker )
{
	if ( pPicker && nID == COCOA_CONTROL_ID_FILETYPE )
	{
		IMutex& rSolarMutex = Application::GetSolarMutex();
		if ( !Application::IsShutDown() )
		{
			JavaFilePicker *pJavaFilePicker = NULL;
			for ( ::std::list< JavaFilePicker* >::const_iterator it = aFilePickers.begin(); it != aFilePickers.end(); ++it )
			{
				if ( *it == pPicker )
				{
					pJavaFilePicker = (JavaFilePicker *)pPicker;
					break;
				}
			}

			if ( pJavaFilePicker )
			{
				FilePickerEvent aEvent;
				aEvent.ElementId = CommonFilePickerElementIds::LISTBOX_FILTER;
				pJavaFilePicker->controlStateChanged( aEvent );
			}

			rSolarMutex.release();
		}
	}
}

// ========================================================================

namespace java {

Sequence< OUString > SAL_CALL JavaFilePicker_getSupportedServiceNames()
{
	Sequence< OUString > aRet( 2 );
	aRet[0] = OUString::createFromAscii( "com.sun.star.ui.dialogs.FilePicker" );
	aRet[1] = OUString::createFromAscii( FILE_PICKER_SERVICE_NAME );
	return aRet;
}

// ------------------------------------------------------------------------

Reference< XInterface > SAL_CALL JavaFilePicker_createInstance( const Reference< XMultiServiceFactory >& xMultiServiceFactory )
{
	return Reference< XInterface >( static_cast< XFilePicker* >( new JavaFilePicker( xMultiServiceFactory ) ) );
}

}

// ========================================================================

JavaFilePicker::JavaFilePicker( const Reference< XMultiServiceFactory >& xServiceMgr ) : WeakComponentImplHelper9< XFilterManager, XFilterGroupManager, XFilePickerControlAccess, XFilePickerNotifier, XFilePreview, XInitialization, XCancellable, XEventListener, XServiceInfo >( maMutex ), mpDialog( NULL ), mbInExecute( false ), mpResMgr( NULL ), mnType( TemplateDescription::FILEOPEN_SIMPLE )
{
}

// ------------------------------------------------------------------------

JavaFilePicker::~JavaFilePicker()
{
	aFilePickers.remove( this );

	if ( mpDialog )
	{
		if ( mbInExecute )
			NSFileDialog_cancel( mpDialog );
		NSFileDialog_release( mpDialog );
	}

	if ( mpResMgr )
		delete mpResMgr;
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::addFilePickerListener( const Reference< XFilePickerListener >& xListener ) throw( RuntimeException )
{
	maListeners.push_back( xListener );
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::removeFilePickerListener( const Reference< XFilePickerListener >& xListener ) throw( RuntimeException )
{
	maListeners.remove( xListener );
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setTitle( const OUString& aTitle ) throw( RuntimeException )
{
	implInit();

	CFStringRef aString = CFStringCreateWithCharacters( NULL, aTitle.getStr(), aTitle.getLength() );
	if ( aString )
	{
		NSFileDialog_setTitle( mpDialog, aString );
		CFRelease( aString );
	}
}

// ------------------------------------------------------------------------

sal_Int16 SAL_CALL JavaFilePicker::execute() throw( RuntimeException )
{
	sal_Int16 nRet = 0;

	implInit();

	// Don't allow recursion
	if ( mbInExecute )
		return nRet;

	mbInExecute = true;

	// Don't lock mutex as we expect callbacks to this object from a
	// a different thread while the dialog is showing
	ULONG nCount = Application::ReleaseSolarMutex();
	nRet = NSFileDialog_showFileDialog( mpDialog );
	Application::AcquireSolarMutex( nCount );

	mbInExecute = false;

	return nRet;
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setMultiSelectionMode( sal_Bool bMode ) throw( RuntimeException )
{
	implInit();

	NSFileDialog_setMultiSelectionMode( mpDialog, bMode ? TRUE : FALSE );
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setDefaultName( const OUString& aName ) throw( RuntimeException )
{
	implInit();

	CFStringRef aString = CFStringCreateWithCharacters( NULL, aName.getStr(), aName.getLength() );
	if ( aString )
	{
		NSFileDialog_setDefaultName( mpDialog, aString );
		CFRelease( aString );
	}
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setDisplayDirectory( const OUString& aDirectory ) throw( com::sun::star::lang::IllegalArgumentException, RuntimeException )
{
	implInit();

	if ( aDirectory.getLength() )
	{
		CFStringRef aString = CFStringCreateWithCharacters( NULL, aDirectory.getStr(), aDirectory.getLength() );
		if ( aString )
		{
			NSFileDialog_setDirectory( mpDialog, aString );
			CFRelease( aString );
		}
	}
}

// ------------------------------------------------------------------------

OUString SAL_CALL JavaFilePicker::getDisplayDirectory() throw( RuntimeException )
{
	OUString aRet;

	implInit();

	CFStringRef aString = NSFileDialog_directory( mpDialog );
	if ( aString )
	{
		CFIndex nLen = CFStringGetLength( aString );
		CFRange aRange = CFRangeMake( 0, nLen );
		sal_Unicode pBuffer[ nLen + 1 ];
		CFStringGetCharacters( aString, aRange, pBuffer );
		pBuffer[ nLen ] = 0;
		CFRelease( aString );
		aRet = OUString( pBuffer );
	}

	return aRet;
}

// ------------------------------------------------------------------------

Sequence< OUString > SAL_CALL JavaFilePicker::getFiles() throw( RuntimeException )
{
	Sequence< OUString > aRet;

	implInit();

	CFStringRef *pURLs = NSFileDialog_URLs( mpDialog );
	if ( pURLs )
	{
		int nCount = 0;
		for ( ; pURLs[ nCount ]; nCount++ )
			;

		if ( nCount == 1 )
		{
			aRet = Sequence< OUString >( nCount );
			CFStringRef aString = pURLs[ 0 ];
			CFIndex nLen = CFStringGetLength( aString );
			CFRange aRange = CFRangeMake( 0, nLen );
			sal_Unicode pBuffer[ nLen + 1 ];
			CFStringGetCharacters( aString, aRange, pBuffer ); 
			pBuffer[ nLen ] = 0;
			aRet[ 0 ] = OUString( pBuffer );
		}
		else if ( nCount > 1 )
		{
			// Apparently, when there is more than one file returned, OOo
			// expects the first element to be the directory and each
			// following element to be the file name
			aRet = Sequence< OUString >( ++nCount );
			for ( int i = 0; i < nCount; i++ )
			{
				CFStringRef aString = pURLs[ i ? i - 1 : 0 ];
				CFRange aSearchRange;
				aSearchRange.location = 0;
				aSearchRange.length = CFStringGetLength( aString );
				CFRange aFoundRange;
				if ( CFStringFindWithOptions( aString, CFSTR( "/" ), aSearchRange, kCFCompareBackwards, &aFoundRange ) )
				{
					CFStringRef aSplitString = NULL;
					if ( i )
					{
						aFoundRange.location++;
						aFoundRange.length = aSearchRange.length - aFoundRange.location;
						aSplitString = CFStringCreateWithSubstring( NULL, aString, aFoundRange );
					}
					else
					{
						aFoundRange.length = aFoundRange.location + 1;
						aFoundRange.location = 0;
						aSplitString = CFStringCreateWithSubstring( NULL, aString, aFoundRange );
					}

					if ( aSplitString )
					{
						CFIndex nLen = CFStringGetLength( aSplitString );
						CFRange aRange = CFRangeMake( 0, nLen );
						sal_Unicode pBuffer[ nLen + 1 ];
						CFStringGetCharacters( aSplitString, aRange, pBuffer ); 
						pBuffer[ nLen ] = 0;
						CFRelease( aSplitString );
						aRet[ i ] = OUString( pBuffer );
					}
				}
			}
		}

		NSFileManager_releaseURLs( pURLs );
	}

	return aRet;
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::appendFilter( const OUString& aTitle, const OUString& aFilter ) throw( IllegalArgumentException, RuntimeException )
{
	implInit();

	CFStringRef aString = CFStringCreateWithCharacters( NULL, aTitle.getStr(), aTitle.getLength() );
	if ( aString )
	{
		CFStringRef aFilterString = CFStringCreateWithCharacters( NULL, aFilter.getStr(), aFilter.getLength() );
		if ( aFilterString )
		{
			NSFileDialog_addFilter( mpDialog, aString, aFilterString );
			CFRelease( aFilterString );
		}

		CFRelease( aString );
	}
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setCurrentFilter( const OUString& aTitle ) throw( IllegalArgumentException, RuntimeException )
{
	implInit();

	CFStringRef aString = CFStringCreateWithCharacters( NULL, aTitle.getStr(), aTitle.getLength() );
	if ( aString )
	{
		NSFileDialog_setSelectedFilter( mpDialog, aString );
		CFRelease( aString );
	}
}

// ------------------------------------------------------------------------

OUString SAL_CALL JavaFilePicker::getCurrentFilter() throw( RuntimeException )
{
	OUString aRet;

	implInit();

	CFStringRef aString = NSFileDialog_selectedFilter( mpDialog );
	if ( aString )
	{
		CFIndex nLen = CFStringGetLength( aString );
		CFRange aRange = CFRangeMake( 0, nLen );
		sal_Unicode pBuffer[ nLen + 1 ];
		CFStringGetCharacters( aString, aRange, pBuffer );
		pBuffer[ nLen ] = 0;
		CFRelease( aString );
		aRet = OUString( pBuffer );
	}

	return aRet;
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::appendFilterGroup( const OUString& sGroupTitle, const Sequence< StringPair >& aFilters ) throw( IllegalArgumentException, RuntimeException )
{
	int nCount = aFilters.getLength();
	for ( int i = 0; i < nCount; i++ )
		appendFilter( aFilters[ i ].First, aFilters[ i ].Second );
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setValue( sal_Int16 nControlId, sal_Int16 nControlAction, const Any& aValue ) throw( RuntimeException )
{
	implInit();

	CocoaControlID nCocoaControlId = GetCocoaControlId( nControlId );
	int nCocoaControlType = NSFileDialog_controlType( nCocoaControlId );
	switch ( nCocoaControlType )
	{
		case COCOA_CONTROL_TYPE_CHECKBOX:
			sal_Bool bChecked;
			aValue >>= bChecked;
			NSFileDialog_setChecked( mpDialog, nCocoaControlId, bChecked ? TRUE : FALSE );
			break;
		case COCOA_CONTROL_TYPE_POPUP:
			switch ( nControlAction )
			{
				case ControlActions::ADD_ITEM:
					{
						OUString aItem;
						aValue >>= aItem;
						XubString aRealItem( aItem );
						aRealItem.EraseAllChars('~');
						CFStringRef aString = CFStringCreateWithCharacters( NULL, aRealItem.GetBuffer(), aRealItem.Len() );
						if ( aString )
						{
							NSFileDialog_addItem( mpDialog, nCocoaControlId, aString );
							CFRelease( aString );
						}
					}
					break;
				case ControlActions::ADD_ITEMS:
					{
						Sequence< OUString > aItems;
						aValue >>= aItems;
						sal_Int32 nCount = aItems.getLength();
						for ( sal_Int32 i = 0; i < nCount; i++ )
						{
							XubString aRealItem( aItems[ i ] );
							aRealItem.EraseAllChars('~');
							CFStringRef aString = CFStringCreateWithCharacters( NULL, aRealItem.GetBuffer(), aRealItem.Len() );
							if ( aString )
							{
								NSFileDialog_addItem( mpDialog, nCocoaControlId, aString );
								CFRelease( aString );
							}
						}
					}
					break;
				case ControlActions::DELETE_ITEM:
					{
						OUString aItem;
						aValue >>= aItem;
						XubString aRealItem( aItem );
						aRealItem.EraseAllChars('~');
						CFStringRef aString = CFStringCreateWithCharacters( NULL, aRealItem.GetBuffer(), aRealItem.Len() );
						if ( aString )
						{
							NSFileDialog_deleteItem( mpDialog, nCocoaControlId, aString );
							CFRelease( aString );
						}
					}
					break;
				case ControlActions::DELETE_ITEMS:
					{
						Sequence< OUString > aItems;
						aValue >>= aItems;
						sal_Int32 nCount = aItems.getLength();
						for ( sal_Int32 i = 0; i < nCount; i++ )
						{
							XubString aRealItem( aItems[ i ] );
							aRealItem.EraseAllChars('~');
							CFStringRef aString = CFStringCreateWithCharacters( NULL, aRealItem.GetBuffer(), aRealItem.Len() );
							if ( aString )
							{
								NSFileDialog_addItem( mpDialog, nCocoaControlId, aString );
								CFRelease( aString );
							}
						}
					}
					break;
				case ControlActions::SET_SELECT_ITEM:
					{
						sal_Int32 nItem;
						aValue >>= nItem;
						NSFileDialog_setSelectedItem( mpDialog, nCocoaControlId, nItem );
					}
					break;
			}
			break;
		default:
			break;
	}
}

// ------------------------------------------------------------------------

Any SAL_CALL JavaFilePicker::getValue( sal_Int16 nControlId, sal_Int16 nControlAction ) throw( RuntimeException )
{
	Any aRet;

	implInit();

	CocoaControlID nCocoaControlId = GetCocoaControlId( nControlId );
	int nCocoaControlType = NSFileDialog_controlType( nCocoaControlId );
	switch ( nCocoaControlType )
	{
		case COCOA_CONTROL_TYPE_CHECKBOX:
			aRet <<= (sal_Bool)NSFileDialog_isChecked( mpDialog, nCocoaControlId );
			break;
		case COCOA_CONTROL_TYPE_POPUP:
			switch ( nControlAction )
			{
				case ControlActions::GET_ITEMS:
					{
						CFStringRef *pItems = NSFileDialog_items( mpDialog, nCocoaControlId );
						if ( pItems )
						{
							int nCount = 0;
							for ( ; pItems[ nCount ]; nCount++ )
								;

							if ( nCount )
							{
								Sequence< OUString > aItems( nCount );
								for ( int i = 0; i < nCount; i++ )
								{
									CFStringRef aString = pItems[ i ];
									CFIndex nLen = CFStringGetLength( aString );
									CFRange aRange = CFRangeMake( 0, nLen );
									sal_Unicode pBuffer[ nLen + 1 ];
									CFStringGetCharacters( aString, aRange, pBuffer ); 
									pBuffer[ nLen ] = 0;
									aItems[ i ] = OUString( pBuffer );
								}

								aRet <<= aItems;
							}

							NSFileManager_releaseItems( pItems );
						}
					}
					break;
				case ControlActions::GET_SELECTED_ITEM:
					{
						CFStringRef aString = NSFileDialog_selectedItem( mpDialog, nCocoaControlId );
						if ( aString )
						{
							CFIndex nLen = CFStringGetLength( aString );
							CFRange aRange = CFRangeMake( 0, nLen );
							sal_Unicode pBuffer[ nLen + 1 ];
							CFStringGetCharacters( aString, aRange, pBuffer ); 
							pBuffer[ nLen ] = 0;
							CFRelease( aString );
							aRet <<= OUString( pBuffer );
						}
					}
					break;
				case ControlActions::GET_SELECTED_ITEM_INDEX:
					aRet <<= (sal_Int32)NSFileDialog_selectedItemIndex( mpDialog, nCocoaControlId );
					break;
			}
			break;
		default:
			break;
	}

	return aRet;
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::enableControl( sal_Int16 nControlId, sal_Bool bEnable ) throw( RuntimeException )
{
	implInit();

	NSFileDialog_setEnabled( mpDialog, GetCocoaControlId( nControlId ), bEnable );
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setLabel( sal_Int16 nControlId, const OUString& aLabel ) throw( RuntimeException )
{
	implInit();

	XubString aRealLabel( aLabel );
	aRealLabel.EraseAllChars('~');

	CFStringRef aString = CFStringCreateWithCharacters( NULL, aRealLabel.GetBuffer(), aRealLabel.Len() );
	if ( !aString )
		return;

	NSFileDialog_setLabel( mpDialog, GetCocoaControlId( nControlId ), aString );

	CFRelease( aString );
}

// ------------------------------------------------------------------------

OUString SAL_CALL JavaFilePicker::getLabel( sal_Int16 nControlId ) throw( RuntimeException )
{
	OUString aRet;

	implInit();

	CFStringRef aString = NSFileDialog_label( mpDialog, GetCocoaControlId( nControlId ) );
	if ( aString )
	{
		CFIndex nLen = CFStringGetLength( aString );
		CFRange aRange = CFRangeMake( 0, nLen );
		sal_Unicode pBuffer[ nLen + 1 ];
		CFStringGetCharacters( aString, aRange, pBuffer );
		pBuffer[ nLen ] = 0;
		CFRelease( aString );
		aRet = OUString( pBuffer );
	}

	return aRet;
}

// ------------------------------------------------------------------------

Sequence< sal_Int16 > SAL_CALL JavaFilePicker::getSupportedImageFormats() throw( RuntimeException )
{
	// The native Cocoa file dialog does all previewing for us
	return Sequence< sal_Int16 >();
}

// ------------------------------------------------------------------------

sal_Int32 SAL_CALL JavaFilePicker::getTargetColorDepth() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "JavaFilePicker::getTargetColorDepth not implemented\n" );
#endif
	return 0;
}

// ------------------------------------------------------------------------

sal_Int32 SAL_CALL JavaFilePicker::getAvailableWidth() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "JavaFilePicker::getAvailableWidth not implemented\n" );
#endif
	return 0;
}

// ------------------------------------------------------------------------

sal_Int32 SAL_CALL JavaFilePicker::getAvailableHeight() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "JavaFilePicker::getAvailableHeight not implemented\n" );
#endif
	return 0;
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::setImage( sal_Int16 aImageFormat, const Any& aImage ) throw( IllegalArgumentException, RuntimeException )
{
	// The native Cocoa file dialog does all previewing for us
}

// ------------------------------------------------------------------------

sal_Bool SAL_CALL JavaFilePicker::setShowState( sal_Bool bShowState ) throw( RuntimeException )
{
	// Previewing is done by the native Cocoa file dialog
	return sal_False;
}

// ------------------------------------------------------------------------

sal_Bool SAL_CALL JavaFilePicker::getShowState() throw( RuntimeException )
{
	// Previewing is done by the native Cocoa file dialog
	return sal_False;
}

// ------------------------------------------------------------------------

void JavaFilePicker::implInit() throw( Exception )
{
	// Check if this instance has already been initialized
	if ( mpDialog )
		return;

	mpResMgr = SimpleResMgr::Create( CREATEVERSIONRESMGR_NAME( fps_office ) );
	if ( !mpResMgr )
		throw NullPointerException();

	BOOL bUseFileOpenDialog = TRUE;
	BOOL bShowAutoExtension = FALSE;
	BOOL bShowFilterOptions = FALSE;
	BOOL bShowImageTemplate = FALSE;
	BOOL bShowLink = FALSE;
	BOOL bShowPassword = FALSE;
	BOOL bShowReadOnly = FALSE;
	BOOL bShowSelection = FALSE;
	BOOL bShowTemplate = FALSE;
	BOOL bShowVersion = FALSE;

	// Determine which native Cocoa controls to show. Note that we do not
	// do anything if the preview or play settings are set as the Mac OS X
	// native Cocoa file dialog already handles these functions.
	switch ( mnType )
	{
		case TemplateDescription::FILEOPEN_SIMPLE:
			break;
		case TemplateDescription::FILESAVE_SIMPLE:
			bUseFileOpenDialog = FALSE;
			break;
		case TemplateDescription::FILESAVE_AUTOEXTENSION_PASSWORD:
			bUseFileOpenDialog = FALSE;
			bShowAutoExtension = TRUE;
			bShowPassword = TRUE;
			break;
		case TemplateDescription::FILESAVE_AUTOEXTENSION_PASSWORD_FILTEROPTIONS:
			bUseFileOpenDialog = FALSE;
			bShowAutoExtension = TRUE;
			bShowFilterOptions = TRUE;
			bShowPassword = TRUE;
			break;
		case TemplateDescription::FILESAVE_AUTOEXTENSION_SELECTION:
			bUseFileOpenDialog = FALSE;
			bShowAutoExtension = TRUE;
			bShowSelection = TRUE;
			break;
		case TemplateDescription::FILESAVE_AUTOEXTENSION_TEMPLATE:
			bUseFileOpenDialog = FALSE;
			bShowAutoExtension = TRUE;
			bShowTemplate = TRUE;
			break;
		case TemplateDescription::FILEOPEN_LINK_PREVIEW_IMAGE_TEMPLATE:
			bShowLink = TRUE;
			bShowImageTemplate = TRUE;
			break;
		case TemplateDescription::FILEOPEN_PLAY:		
			break;
		case TemplateDescription::FILEOPEN_READONLY_VERSION:
			bShowReadOnly = TRUE;
			bShowVersion = TRUE;
			break;
		case TemplateDescription::FILEOPEN_LINK_PREVIEW:
			bShowLink = TRUE;
			break;
		case TemplateDescription::FILESAVE_AUTOEXTENSION:
			bUseFileOpenDialog = FALSE;
			bShowAutoExtension = TRUE;
			break;
		default:
			throw IllegalArgumentException( OUString::createFromAscii( "Unknown template" ), static_cast< XFilePicker* >( this ), 1 );
	}

	mpDialog = NSFileDialog_create( this, bUseFileOpenDialog, TRUE, bShowAutoExtension, bShowFilterOptions, bShowImageTemplate, bShowLink, bShowPassword, bShowReadOnly, bShowSelection, bShowTemplate, bShowVersion );
	if ( !mpDialog )
		throw NullPointerException();

	SimpleResMgr *pSvtResMgr = SimpleResMgr::Create( CREATEVERSIONRESMGR_NAME( svt ) );
	if ( pSvtResMgr )
	{
		OUString aLabel( pSvtResMgr->ReadString( STR_FILEDLG_TYPE ) );
		setLabel( CommonFilePickerElementIds::LISTBOX_FILTER_LABEL, aLabel );
		delete pSvtResMgr;
	}
	else
	{
		throw NullPointerException();
	}

	// Set initial values
	if ( bShowFilterOptions )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_FILTER_OPTIONS ) );
		setLabel( ExtendedFilePickerElementIds::CHECKBOX_FILTEROPTIONS, aLabel );
	}
	if ( bShowImageTemplate )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_IMAGE_TEMPLATE ) );
		setLabel( ExtendedFilePickerElementIds::LISTBOX_IMAGE_TEMPLATE_LABEL, aLabel );
	}
	if ( bShowLink )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_INSERT_AS_LINK ) );
		setLabel( ExtendedFilePickerElementIds::CHECKBOX_LINK, aLabel );
	}
	if ( bShowLink )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_INSERT_AS_LINK ) );
		setLabel( ExtendedFilePickerElementIds::CHECKBOX_LINK, aLabel );
	}
	if ( bShowPassword )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_PASSWORD ) );
		setLabel( ExtendedFilePickerElementIds::CHECKBOX_PASSWORD, aLabel );
	}
	if ( bShowReadOnly )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_READONLY ) );
		setLabel( ExtendedFilePickerElementIds::CHECKBOX_READONLY, aLabel );
	}
	if ( bShowSelection )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_SELECTION ) );
		setLabel( ExtendedFilePickerElementIds::CHECKBOX_SELECTION, aLabel );
	}
	if ( bShowTemplate )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_TEMPLATES ) );
		setLabel( ExtendedFilePickerElementIds::LISTBOX_IMAGE_TEMPLATE_LABEL, aLabel );
	}
	if ( bShowVersion )
	{
		OUString aLabel( mpResMgr->ReadString( STR_SVT_FILEPICKER_VERSION ) );
		setLabel( ExtendedFilePickerElementIds::LISTBOX_VERSION_LABEL, aLabel );
	}

	aFilePickers.push_back( this );
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::initialize( const Sequence< Any >& aArguments ) throw( Exception, RuntimeException )
{
	if ( !aArguments.getLength() )
		throw IllegalArgumentException( OUString::createFromAscii( "no arguments" ), static_cast< XFilePicker* >( this ), 1 );

	Any aAny = aArguments[0];
	if ( ( aAny.getValueType() != getCppuType( (sal_Int16*)0) ) && ( aAny.getValueType() != getCppuType( (sal_Int8*)0 ) ) )
		throw IllegalArgumentException( OUString::createFromAscii( "invalid argument type" ), static_cast< XFilePicker* >( this ), 1 );

	aAny >>= mnType;

	implInit();
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::cancel() throw( RuntimeException )
{
	implInit();

	NSFileDialog_cancel( mpDialog );
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::disposing( const EventObject& aEvent ) throw( RuntimeException )
{
	Reference< XFilePickerListener > xListener( aEvent.Source, UNO_QUERY );
	if ( xListener.is() )
		removeFilePickerListener( xListener );
}

// ------------------------------------------------------------------------

OUString SAL_CALL JavaFilePicker::getImplementationName() throw( RuntimeException )
{
	return OUString::createFromAscii( FILE_PICKER_IMPL_NAME );
}

// ------------------------------------------------------------------------

sal_Bool SAL_CALL JavaFilePicker::supportsService( const OUString& ServiceName ) throw( RuntimeException )
{
	Sequence < OUString > aSupportedServicesNames = JavaFilePicker_getSupportedServiceNames();
 
	for ( sal_Int32 n = aSupportedServicesNames.getLength(); n--; )
		if ( aSupportedServicesNames[n].compareTo(ServiceName) == 0 )
			return sal_True;
 
	return sal_False;
}

// ------------------------------------------------------------------------

Sequence< OUString > SAL_CALL JavaFilePicker::getSupportedServiceNames() throw( RuntimeException )
{
	return JavaFilePicker_getSupportedServiceNames();
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::fileSelectionChanged( FilePickerEvent aEvent )
{
#ifdef DEBUG
	fprintf( stderr, "JavaFilePicker::fileSelectionChanged not implemented\n" );
#endif
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::directoryChanged( FilePickerEvent aEvent )
{
#ifdef DEBUG
	fprintf( stderr, "JavaFilePicker::directoryChanged not implemented\n" );
#endif
}

// ------------------------------------------------------------------------

OUString SAL_CALL JavaFilePicker::helpRequested( FilePickerEvent aEvent ) const
{
	// The native Cocoa file dialog does not have any help buttons
	return OUString();
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::controlStateChanged( FilePickerEvent aEvent )
{
	::std::list< Reference< XFilePickerListener > > listeners( maListeners );

	for ( ::std::list< Reference< XFilePickerListener > >::const_iterator it = listeners.begin(); it != listeners.end(); ++it )
	{
		if ( (*it).is() )
			(*it)->controlStateChanged( aEvent );
	}
}

// ------------------------------------------------------------------------

void SAL_CALL JavaFilePicker::dialogSizeChanged()
{
#ifdef DEBUG
	fprintf( stderr, "JavaFilePicker::dialogSizeChanged not implemented\n" );
#endif
}
