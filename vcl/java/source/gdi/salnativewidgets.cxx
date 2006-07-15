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
 *  Edward Peterlin, April 2006
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2006 by Edward Peterlin (OPENSTEP@neooffice.org)
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

#define _SV_SALNATIVEWIDGETS_CXX

#ifndef _SV_SVSYS_H
#include <svsys.h>
#endif

#ifndef _SV_SALGDI_H
#include <salgdi.h>
#endif
#ifndef _SV_SALDATA_HXX
#include <saldata.hxx>
#endif
#ifndef _SV_SALBMP_H
#include <salbmp.h>
#endif

#ifndef _RTL_USTRING_H_
#include <rtl/ustring.h>
#endif
#include <osl/module.h>

#ifndef _SV_COM_SUN_STAR_VCL_VCLBITMAP_HXX
#include <com/sun/star/vcl/VCLBitmap.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif
#ifndef _SV_JAVA_TOOLS_HXX
#include <java/tools.hxx>
#endif

#ifdef __cplusplus
#include <premac.h>
#endif
#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>
#ifdef __cplusplus
#include <postmac.h>
#endif

using namespace vcl;
using namespace rtl;

#define COMBOBOX_BUTTON_WIDTH 22
#define COMBOBOX_BUTTON_TRIMWIDTH 3
#define CONTROL_TAB_PANE_TOP_OFFSET	( ( vcl::IsRunningPanther() ) ? 2 : 10 )
#define EDITBOX_TRIMWIDTH	3
#define LISTBOX_BUTTON_HORIZ_TRIMWIDTH	0
#define LISTBOX_BUTTON_VERT_TRIMWIDTH	2

#if ( BUILD_OS_MAJOR == 10 ) && ( BUILD_OS_MINOR == 3 )
// constants and structures for 10.3
#define kThemeScrollBarMedium 0
#define kThemeIndeterminateBarLarge kThemeLargeIndeterminateBar
#define kThemeProgressBarLarge kThemeLargeProgressBar

struct HIThemeTabDrawInfo104 {
  UInt32              version;
  ThemeTabStyle       style;
  ThemeTabDirection   direction;
  HIThemeTabSize      size;
  HIThemeTabAdornment  adornment;             /* various tab attributes */
  UInt32      kind;
  UInt32  position;
};

#define kHIThemeTabKindNormal 0
#define kHIThemeTabPositionFirst 0
#define kHIThemeTabPositionMiddle 1
#define kHIThemeTabPositionLast 2
#define kHIThemeTabPositionOnly 3

#define kHIThemeTabAdornmentTrailingSeparator (1 << 4)

typedef UInt32 HIThemeTabKind;
typedef UInt32 HIThemeTabPaneAdornment;

struct HIThemeTabPaneDrawInfo104 {
  UInt32              version;
  ThemeDrawState      state;
  ThemeTabDirection   direction;
  HIThemeTabSize      size;
  HIThemeTabKind      kind;
  HIThemeTabPaneAdornment  adornment;
};

#else if ( BUILD_OS_MAJOR >= 10 ) && ( BUILD_OS_MINOR > 3 )
typedef HIThemeTabDrawInfo HIThemeTabDrawInfo104;
typedef HIThemeTabPaneDrawInfo HIThemeTabPaneDrawInfo104;
#endif

struct VCLBitmapBuffer : BitmapBuffer
{
	com_sun_star_vcl_VCLBitmap* 	mpVCLBitmap;
	java_lang_Object*	 	mpData;
	CGContextRef			maContext;

							VCLBitmapBuffer();
	virtual					~VCLBitmapBuffer();

	BOOL					Create( long nWidth, long nHeight );
	void					Destroy();
	void					ReleaseContext();
};

// =======================================================================

VCLBitmapBuffer::VCLBitmapBuffer() : BitmapBuffer(), mpVCLBitmap( NULL ), mpData( NULL ), maContext( NULL )
{
	mnFormat = 0;
	mnWidth = 0;
	mnHeight = 0;
	mnScanlineSize = 0;
	mnBitCount = 0;
	mpBits = NULL;
}

// -----------------------------------------------------------------------

VCLBitmapBuffer::~VCLBitmapBuffer()
{
	Destroy();
}

// -----------------------------------------------------------------------

BOOL VCLBitmapBuffer::Create( long nWidth, long nHeight )
{
	Destroy();

	mpVCLBitmap = new com_sun_star_vcl_VCLBitmap( nWidth, nHeight, 32 );
	if ( !mpVCLBitmap || !mpVCLBitmap->getJavaObject() )
	{
		Destroy();
		return FALSE;
	}

	VCLThreadAttach t;
	if ( !t.pEnv )
	{
		Destroy();
		return FALSE;
	}

	mpData = mpVCLBitmap->getData();
	if ( !mpData )
	{
		Destroy();
		return FALSE;
	}

	mnFormat = JavaSalBitmap::Get32BitNativeFormat() | BMP_FORMAT_TOP_DOWN;
	mnWidth = nWidth;
	mnHeight = nHeight;
	mnScanlineSize = nWidth * sizeof( jint );
	mnBitCount = 32;

	jboolean bCopy( sal_False );
	mpBits = (BYTE *)t.pEnv->GetPrimitiveArrayCritical( (jintArray)mpData->getJavaObject(), &bCopy );
	if ( !mpBits )
	{
		Destroy();
		return FALSE;
	}

	CGColorSpaceRef aColorSpace = CGColorSpaceCreateDeviceRGB();
	if ( !aColorSpace )
	{
		Destroy();
		return FALSE;
	}

#ifdef POWERPC
	maContext = CGBitmapContextCreate( mpBits, nWidth, nHeight, 8, mnScanlineSize, aColorSpace, kCGImageAlphaPremultipliedFirst );
#else	// POWERPC
	maContext = CGBitmapContextCreate( mpBits, nWidth, nHeight, 8, mnScanlineSize, aColorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little );
#endif	// POWERPC
	CGColorSpaceRelease( aColorSpace );
	if ( !maContext )
	{
		Destroy();
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------

void VCLBitmapBuffer::Destroy()
{
	mnFormat = 0;
	mnWidth = 0;
	mnHeight = 0;
	mnScanlineSize = 0;
	mnBitCount = 0;

	if ( maContext )
	{
		CGContextRelease( maContext );
		maContext = NULL;
	}

	if ( mpBits )
	{
		if ( mpData )
		{
			VCLThreadAttach t;
			if ( t.pEnv )
				t.pEnv->ReleasePrimitiveArrayCritical( (jintArray)mpData->getJavaObject(), mpBits, JNI_ABORT );
		}
		mpBits = NULL;
	}

	if ( mpData )
	{
		delete mpData;
		mpData = NULL;
	}

	if ( mpVCLBitmap )
	{
		delete mpVCLBitmap;
		mpVCLBitmap = NULL;
	}
}

// -----------------------------------------------------------------------

void VCLBitmapBuffer::ReleaseContext()
{
	if ( maContext )
	{
		CGContextRelease( maContext );
		maContext = NULL;
	}

	if ( mpBits )
	{
		if ( mpData )
		{
			VCLThreadAttach t;
			if ( t.pEnv )
				t.pEnv->ReleasePrimitiveArrayCritical( (jintArray)mpData->getJavaObject(), mpBits, 0 );
		}
		mpBits = NULL;
	}

	if ( mpData )
	{
		delete mpData;
		mpData = NULL;
	}
}

// =======================================================================

static BOOL InitButtonDrawInfo( HIThemeButtonDrawInfo *pButtonDrawInfo, ControlState nState )
{
	memset( pButtonDrawInfo, 0, sizeof( HIThemeButtonDrawInfo ) );
	pButtonDrawInfo->version = 0;
	pButtonDrawInfo->kind = kThemeComboBox;
	if ( nState & ( CTRL_STATE_PRESSED | CTRL_STATE_SELECTED ) )
		pButtonDrawInfo->state = kThemeStatePressed;
	else if ( nState & CTRL_STATE_ENABLED )
		pButtonDrawInfo->state = kThemeStateActive;
	else
		pButtonDrawInfo->state = kThemeStateInactive;

	if ( nState & CTRL_STATE_FOCUSED )
		pButtonDrawInfo->adornment = kThemeAdornmentFocus;
	else
		pButtonDrawInfo->adornment = kThemeAdornmentDefault;
	return TRUE;
}

// =======================================================================

/**
 * (static) Initialize an HITheme button structure to draw spinner arrows for a
 * spinbox, an editable control with arrow controls for incrementing and
 * decrementing a value.
 *
 * @param pButtonDrawInfo		HITheme button structure for drawing spinner
 * @param pSpinbuttonValue		VCL structure holding enable state of the
 * @return TRUE if successful, FALSE on failure
 */
static BOOL InitSpinbuttonDrawInfo( HIThemeButtonDrawInfo *pButtonDrawInfo, SpinbuttonValue *pSpinbuttonValue )
{
	memset( pButtonDrawInfo, 0, sizeof( HIThemeButtonDrawInfo ) );
	pButtonDrawInfo->version = 0;
	pButtonDrawInfo->kind = kThemeIncDecButton;
	if( pSpinbuttonValue )
	{
		if( pSpinbuttonValue->mnUpperState & CTRL_STATE_PRESSED )
			pButtonDrawInfo->state = kThemeStatePressedUp;
		else if( pSpinbuttonValue->mnLowerState & CTRL_STATE_PRESSED )
			pButtonDrawInfo->state = kThemeStatePressedDown;
		else if( ( ! ( pSpinbuttonValue->mnUpperState & CTRL_STATE_ENABLED ) ) && ( ! ( pSpinbuttonValue->mnLowerState & CTRL_STATE_ENABLED ) ) )
			pButtonDrawInfo->state = kThemeStateInactive;
		else
			pButtonDrawInfo->state = kThemeStateActive;
	}
	else
		pButtonDrawInfo->state = kThemeStateActive;
	return TRUE;
}

// =======================================================================

/**
 * (static) Convert a VCL scrollbar value structure into HITheme structures.
 *
 * @param pScrollBarTrackInfo		HITheme scrollbar structure
 * @param pHITrackInfo				alternative HITheme scrollbar structure used
 *									by some calls
 * @param nState					overall control state of the scrollbar.
 *									states of subparts are contained in the
 *									scrollbar value structure.
 * @param bounds					drawing bounds of the scrollbar
 * @param pScrollbarValue			VCL implcontrolvalue containing scrollbar
 *									value
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitScrollBarTrackInfo( HIThemeTrackDrawInfo *pTrackDrawInfo, HIScrollBarTrackInfo *pHITrackInfo, ControlState nState, Rectangle bounds, ScrollbarValue *pScrollbarValue )
{
	memset( pTrackDrawInfo, 0, sizeof( HIThemeTrackDrawInfo ) );
	if( pHITrackInfo )
		memset( pHITrackInfo, 0, sizeof( HIScrollBarTrackInfo ) );
	pTrackDrawInfo->version = 0;
	pTrackDrawInfo->kind = kThemeScrollBarMedium;
	pTrackDrawInfo->bounds.origin.x = 0;
	pTrackDrawInfo->bounds.origin.y = 0;
	pTrackDrawInfo->bounds.size.width = bounds.GetWidth();
	pTrackDrawInfo->bounds.size.height = bounds.GetHeight();
	if( bounds.GetWidth() > bounds.GetHeight() )
		pTrackDrawInfo->attributes |= kThemeTrackHorizontal;
	pTrackDrawInfo->attributes |= kThemeTrackShowThumb;
	if( nState & CTRL_STATE_ENABLED )
		pTrackDrawInfo->enableState = kThemeTrackActive;
	else
		pTrackDrawInfo->enableState = kThemeTrackDisabled;
	if( pHITrackInfo )
		pHITrackInfo->enableState = pTrackDrawInfo->enableState;
	if( pScrollbarValue )
	{
		pTrackDrawInfo->min = pScrollbarValue->mnMin;
		pTrackDrawInfo->max = pScrollbarValue->mnMax-pScrollbarValue->mnVisibleSize;
		pTrackDrawInfo->value = pScrollbarValue->mnCur;
		pTrackDrawInfo->trackInfo.scrollbar.viewsize = pScrollbarValue->mnVisibleSize;
		if( pScrollbarValue->mnButton1State & CTRL_STATE_PRESSED )
			pTrackDrawInfo->trackInfo.scrollbar.pressState |= ( kThemeLeftInsideArrowPressed );
		if( pScrollbarValue->mnButton2State & CTRL_STATE_PRESSED )
			pTrackDrawInfo->trackInfo.scrollbar.pressState |= ( kThemeRightOutsideArrowPressed );
		if( pScrollbarValue->mnPage1State & CTRL_STATE_PRESSED )
			pTrackDrawInfo->trackInfo.scrollbar.pressState |= kThemeLeftTrackPressed;
		if( pScrollbarValue->mnPage2State & CTRL_STATE_PRESSED )
			pTrackDrawInfo->trackInfo.scrollbar.pressState |= kThemeRightTrackPressed;
		if( pScrollbarValue->mnThumbState & CTRL_STATE_PRESSED )
			pTrackDrawInfo->trackInfo.scrollbar.pressState |= kThemeThumbPressed;
	}

	if( pTrackDrawInfo->min == pTrackDrawInfo->max )
	{
		// we need to seed the min, max, and value with "reasonable" values
		// in order for scrollbar metrics to be computed properly by HITheme.
		// If the values are all equal, HITheme will return a NULL rectangle
		// for all potential scrollbar parts.

		pTrackDrawInfo->min = 0;
		pTrackDrawInfo->max = 1;
		pTrackDrawInfo->value = 0;
		pTrackDrawInfo->trackInfo.scrollbar.viewsize = 0;
	}
	if( pHITrackInfo )
	{
		pHITrackInfo->pressState = pTrackDrawInfo->trackInfo.scrollbar.pressState;
		pHITrackInfo->viewsize = pTrackDrawInfo->trackInfo.scrollbar.viewsize;
	}
	return TRUE;
}

// =======================================================================

/**
 * (static) Convert VCL progress bar codes into an HITheme progress bar
 * structure
 *
 * @param pTrackDrawInfo	HITheme progress bar structure
 * @param nState			control state of the progress bar (enabled vs. disabled)
 * @param bounds			drawing bounds of the progress bar
 * @param pProgressbarValue	optional value providing progress bar specific
 *							info
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitProgressbarTrackInfo( HIThemeTrackDrawInfo *pTrackDrawInfo, ControlState nState, Rectangle bounds, ProgressbarValue *pProgressbarValue )
{
	static UInt8 phase = 0x1;

	memset( pTrackDrawInfo, 0, sizeof( HIThemeTrackDrawInfo ) );
	pTrackDrawInfo->version = 0;
	pTrackDrawInfo->kind = ( ( pProgressbarValue && pProgressbarValue->mbIndeterminate ) ? kThemeIndeterminateBarLarge : kThemeProgressBarLarge );
	pTrackDrawInfo->bounds.origin.x = 0;
	pTrackDrawInfo->bounds.origin.y = 0;
	pTrackDrawInfo->bounds.size.width = bounds.GetWidth();
	pTrackDrawInfo->bounds.size.height = bounds.GetHeight();
	if( bounds.GetWidth() > bounds.GetHeight() )
		pTrackDrawInfo->attributes |= kThemeTrackHorizontal;
	pTrackDrawInfo->enableState = kThemeTrackActive;
	pTrackDrawInfo->min = 0;
	pTrackDrawInfo->max = 100;
	if( pProgressbarValue )
	{
		pTrackDrawInfo->value = (int)pProgressbarValue->mdPercentComplete;
		if( pProgressbarValue->mbIndeterminate )
		{
			pTrackDrawInfo->trackInfo.progress.phase = phase++;
		}
	}
	return TRUE;
}

// =======================================================================

/**
 * (static) Convert VCL tab information structure into an HITheme tab
 * description structure.  This structure can be used to draw an
 * individual tab/segmented control into a window.
 *
 * @param pTabDrawInfo		pointer to HITheme tab drawing structure
 * @param nState			overall control state of the tab item.
 * @param pTabValue			VCL structure containing information about
 * 							the tab's position
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitTabDrawInfo( HIThemeTabDrawInfo104 *pTabDrawInfo, ControlState nState, TabitemValue *pTabValue )
{
	memset( pTabDrawInfo, 0, sizeof( HIThemeTabDrawInfo104 ) );
	pTabDrawInfo->version = 1;
	if( nState & CTRL_STATE_SELECTED )
		pTabDrawInfo->style = kThemeTabFront;
	else if( nState & CTRL_STATE_PRESSED )
		pTabDrawInfo->style = kThemeTabNonFrontPressed;
	else if( nState & CTRL_STATE_ENABLED )
		pTabDrawInfo->style = kThemeTabNonFront;
	else
		pTabDrawInfo->style = kThemeTabNonFrontInactive;
	pTabDrawInfo->direction = kThemeTabNorth;	// always assume tabs are at top of tab controls in standard position
	pTabDrawInfo->size = kHIThemeTabSizeNormal;
	if( nState & CTRL_STATE_FOCUSED )
		pTabDrawInfo->adornment = kHIThemeTabAdornmentFocus;
	else
		pTabDrawInfo->adornment = kHIThemeTabAdornmentNone;
	pTabDrawInfo->kind = kHIThemeTabKindNormal;
	pTabDrawInfo->position = kHIThemeTabPositionMiddle;
	if( pTabValue )
	{
		if ( pTabValue->isFirst() && pTabValue->isLast() )
			pTabDrawInfo->position = kHIThemeTabPositionOnly;
		else if( pTabValue->isFirst() )
			pTabDrawInfo->position = kHIThemeTabPositionFirst;
		else if( pTabValue->isLast() )
			pTabDrawInfo->position = kHIThemeTabPositionLast;
	}
	switch( pTabDrawInfo->position )
	{
		case kHIThemeTabPositionMiddle:
		case kHIThemeTabPositionFirst:
			pTabDrawInfo->adornment |= kHIThemeTabAdornmentTrailingSeparator;
			break;
	}
	return TRUE;
}

// =======================================================================

/**
 * (static) Initialize the HITheme strucutre used to draw the bounding box
 * for a tab control background bounding box
 *
 * @param pTabPaneDrawInfo	pointer to HITheme tab drawing structure
 * @param nState			overall control state of the tab item.
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitTabPaneDrawInfo( HIThemeTabPaneDrawInfo104 *pTabPaneDrawInfo, ControlState nState )
{
	memset( pTabPaneDrawInfo, 0, sizeof( HIThemeTabPaneDrawInfo104 ) );
	pTabPaneDrawInfo->version = 1;
	pTabPaneDrawInfo->direction = kThemeTabNorth;
	pTabPaneDrawInfo->size = kHIThemeTabSizeNormal;
	pTabPaneDrawInfo->adornment = kHIThemeTabAdornmentNone;
	if( ! ( nState & CTRL_STATE_ENABLED ) )
		pTabPaneDrawInfo->state = kThemeStateInactive;
	else
		pTabPaneDrawInfo->state = kThemeStateActive;
	return TRUE;
}

// =======================================================================

/**
 * (static) Initialize an HITheme groupbox draw structure to draw a primary
 * group box.
 *
 * @param pDrawInfo		pointer to the HITheme group box draw structure to
 *						initialize
 * @param nState		overall control state of the group box
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitPrimaryGroupBoxDrawInfo( HIThemeGroupBoxDrawInfo *pDrawInfo, ControlState nState )
{
	memset( pDrawInfo, 0, sizeof( HIThemeGroupBoxDrawInfo ) );
	pDrawInfo->version = 0;
	if( ! ( nState & CTRL_STATE_ENABLED ) )
		pDrawInfo->state = kThemeStateInactive;
	else
		pDrawInfo->state = kThemeStateActive;
	pDrawInfo->kind = kHIThemeGroupBoxKindPrimary;
	return TRUE;
}

// =======================================================================

/**
 * (static) Initialize HITheme structures used to draw the frame of an
 * edit field.
 *
 * @param pFrameInfo		pointer to the HITheme frame info structure
 *							to be initialized
 * @param nState			control state of the edit field
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitEditFieldDrawInfo( HIThemeFrameDrawInfo *pFrameInfo, ControlState nState )
{
	memset( pFrameInfo, 0, sizeof( HIThemeFrameDrawInfo ) );
	pFrameInfo->version = 0;
	pFrameInfo->kind = kHIThemeFrameTextFieldSquare;
	if( ! ( nState & CTRL_STATE_ENABLED ) )
		pFrameInfo->state = kThemeStateInactive;
	else
		pFrameInfo->state = kThemeStateActive;
	if( nState & CTRL_STATE_FOCUSED )
	{
		pFrameInfo->isFocused = true;
		pFrameInfo->state |= kThemeStateActive;	// logically we can't have a focused edit field that's inactive
	}
	return TRUE;
}

// =======================================================================

/**
 * (static) Draw a ComboBox into the graphics port at the specified location.
 * ComboBoxes are editable pulldowns, the left portion of which is an edit
 * field and the right portion a downward arrow button used to display the
 * full list contents.
 *
 * Due to VM implementation, JComboBox Swing elements cannot be drawn into
 * a Graphics unless the JComboBox is properly embedded into a visible JFrame.
 * The VM implementation draws these objects asynchronously.  Since we can't
 * easily handle it in Java, we'll use HITheme APIs to draw it into a
 * SalBitmap that we then blit into the graphics.
 *
 * @param pGraphics		pointer to the destination graphics where we'll
 *				be drawing
 * @param rDestBounds		eventual destination rectangle that encompasses
 *				the entire control, editing area as well as
 *				popup arrow
 * @param nState		state of the button to b drawn (enabled/pressed/etc.)
 * @param aCaption		text used for the control.  Presently ignored
 *				as we draw only the frame and let VCL draw
 *				the text
 * @return TRUE if successful, FALSE on error
 */
static BOOL DrawNativeComboBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, OUString aCaption )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeButtonDrawInfo aButtonDrawInfo;
		InitButtonDrawInfo( &aButtonDrawInfo, nState );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth() - COMBOBOX_BUTTON_TRIMWIDTH;
		destRect.size.height = rDestBounds.GetHeight();
		bRet = ( HIThemeDrawButton( &destRect, &aButtonDrawInfo, aBuffer.maContext, kHIThemeOrientationInverted, NULL ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a popupmenu into the graphics port at the specified location.
 * Popup menus, a.k.a listboxes or non-editable combo boxes, consist of a
 * regular popup button and text.
 *
 * Due to VM implementation, JComboBox Swing elements cannot be drawn into
 * a Graphics unless the JComboBox is properly embedded into a visible JFrame.
 * The VM implementation draws these objects asynchronously.  Since we can't
 * easily handle it in Java, we'll use HITheme APIs to draw it into a
 * SalBitmap that we then blit into the graphics.
 *
 * @param pGraphics		pointer to the destination graphics where we'll
 *				be drawing
 * @param rDestBounds		eventual destination rectangle that encompasses
 *				the entire control, editing area as well as
 *				popup arrow
 * @param nState		state of the button to b drawn (enabled/pressed/etc.)
 * @param aCaption		text used for the control.  Presently ignored
 *				as we draw only the frame and let VCL draw
 *				the text
 * @return TRUE if successful, FALSE on error
 */
static BOOL DrawNativeListBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, OUString aCaption )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeButtonDrawInfo aButtonDrawInfo;
		InitButtonDrawInfo( &aButtonDrawInfo, nState );
		aButtonDrawInfo.kind = kThemePopupButton;

		HIRect destRect;
		destRect.origin.x = LISTBOX_BUTTON_HORIZ_TRIMWIDTH;
		destRect.origin.y = LISTBOX_BUTTON_VERT_TRIMWIDTH;
		destRect.size.width = rDestBounds.GetWidth() - LISTBOX_BUTTON_HORIZ_TRIMWIDTH;
		destRect.size.height = rDestBounds.GetHeight() - LISTBOX_BUTTON_VERT_TRIMWIDTH;
		bRet = ( HIThemeDrawButton( &destRect, &aButtonDrawInfo, aBuffer.maContext, kHIThemeOrientationInverted, NULL ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return TRUE;
}

// =======================================================================

/**
 * (static) Draw a scrollbar into the graphics port at the specified location.
 * Swing scrollbars can lead to some odd tracking issues and don't provide
 * easy access to all of the individual subparts of the control proper,
 * so we'll draw them using HIThemes and SalBitmaps.
 *
 * @param pGraphics			pointer to the destination graphics where we'll be drawing
 * @param rDestBounds		eventual destination rectangle for the scrollbar
 * @param nState			overall scrollbar state (active vs. disabled)
 * @param pScrollbarValue	VCL scrollbar info value
 * @return TRUE if successful, FALSE on error
 */
static BOOL DrawNativeScrollBar( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, ScrollbarValue *pScrollbarValue )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeTrackDrawInfo pTrackDrawInfo;
		InitScrollBarTrackInfo( &pTrackDrawInfo, NULL, nState, rDestBounds, pScrollbarValue );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();
		
		// note that Aqua scrollbars always are 15 pixels in width and the rest
		// of the background will not be drawn.  use an opaque white fill to 
		// prevent graphics glitches from anything still left in the buffer
		// should it not be sized to exact Aqua metrics.
		float whiteColor[] = { 1.0, 1.0, 1.0, 1.0 };
		CGContextSetFillColor( aBuffer.maContext, whiteColor );
		CGContextFillRect( aBuffer.maContext, destRect );
		
		bRet = ( HIThemeDrawTrack( &pTrackDrawInfo, NULL, aBuffer.maContext, kHIThemeOrientationInverted ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a spinbox using native controls.  This consists of a set of
 * stepper arrows on the right portion of the control and an edit field on
 * the left.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 * @param pValue			optional value giving enabled & pressed state for
 *							subcontrols.
 * @return TRUE if drawing successful, FALSE if not
 */
static BOOL DrawNativeSpinbox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, SpinbuttonValue *pValue )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeButtonDrawInfo aButtonDrawInfo;
		InitSpinbuttonDrawInfo( &aButtonDrawInfo, pValue );

		HIRect arrowRect;
		arrowRect.origin.x = rDestBounds.GetWidth() - 15;
		if( arrowRect.origin.x < 0 )
			arrowRect.origin.x = 0;
		arrowRect.origin.y = 0;
		arrowRect.size.width = 15;
		arrowRect.size.height = rDestBounds.GetHeight() - 1;
		
		// ensure we won't overdraw our buffer beyond its boundaries
		CGRect clipBounds;
		clipBounds.origin.x = 0;
		clipBounds.origin.y = 0;
		clipBounds.size.width = rDestBounds.GetWidth();
		clipBounds.size.height = rDestBounds.GetHeight();
		CGContextClipToRect( aBuffer.maContext, clipBounds );
		
		bRet = ( HIThemeDrawButton( &arrowRect, &aButtonDrawInfo, aBuffer.maContext, kHIThemeOrientationInverted, NULL ) == noErr );

		if( bRet )
		{
			HIRect editRect;
			editRect.origin.x = 0;
			editRect.origin.y = 0;
			editRect.size.width = rDestBounds.GetWidth() - arrowRect.size.width - 4;
			editRect.size.height = rDestBounds.GetHeight();

			HIThemeFrameDrawInfo aFrameInfo;
			memset( &aFrameInfo, 0, sizeof( HIThemeFrameDrawInfo ) );

			aFrameInfo.kind = kHIThemeFrameTextFieldSquare;
			if( ! nState )
				aFrameInfo.state = kThemeStateInactive;
			else
				aFrameInfo.state = kThemeStateActive;
			if( nState & CTRL_STATE_FOCUSED )
				aFrameInfo.isFocused = TRUE;
			else
				aFrameInfo.isFocused = FALSE;

			bRet = ( HIThemeDrawFrame( &editRect, &aFrameInfo, aBuffer.maContext, kHIThemeOrientationInverted ) == noErr );
		}
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a progress bar using the native widget appearance.  The
 * progressbar indicates percentage of task completion.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 * @param pValue			value providing the percentage completion and other
 *							progressbar state
 * @return TRUE if drawing successful, FALSE if not
 */
static BOOL DrawNativeProgressbar( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, ProgressbarValue *pValue )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeTrackDrawInfo aTrackDrawInfo;
		InitProgressbarTrackInfo( &aTrackDrawInfo, nState, rDestBounds, pValue );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		bRet = ( HIThemeDrawTrack( &aTrackDrawInfo, NULL, aBuffer.maContext, kHIThemeOrientationInverted ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw an individual tab control using the native widget appearance.
 * The tab consists of only a single element in a list of tabs that appear
 * at the top of a full tab group.  Each tab gets drawn independently.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 * @param pValue			value providing the tab style, its position in
 *							the tab order relative to other tabs, and other
 *							tab-specific values
 * @return TRUE if drawing successful, FALSE if not
 */
static BOOL DrawNativeTab( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, TabitemValue *pValue )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeTabDrawInfo104 pTabDrawInfo;
		InitTabDrawInfo( &pTabDrawInfo, nState, pValue );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		HIRect labelRect; // ignored

		bRet = ( HIThemeDrawTab( &destRect, (HIThemeTabDrawInfo *)&pTabDrawInfo, aBuffer.maContext, kHIThemeOrientationInverted, &labelRect ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native tab box.  This includes the rounded boundary and
 * flat grey tab background.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 */
static BOOL DrawNativeTabBoundingBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeTabPaneDrawInfo104 pTabPaneDrawInfo;
		InitTabPaneDrawInfo( &pTabPaneDrawInfo, nState );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		bRet = ( HIThemeDrawTabPane( &destRect, (HIThemeTabPaneDrawInfo *)&pTabPaneDrawInfo, aBuffer.maContext, kHIThemeOrientationInverted ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a primary group box.  This includes the rounded boundary used
 * to group controls together.
 *
 * @param pGraphics			pointer into graphics object where box should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 */
static BOOL DrawNativePrimaryGroupBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeGroupBoxDrawInfo pGroupBoxDrawInfo;
		InitPrimaryGroupBoxDrawInfo( &pGroupBoxDrawInfo, nState );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		bRet = ( HIThemeDrawGroupBox( &destRect, &pGroupBoxDrawInfo, aBuffer.maContext, kHIThemeOrientationInverted ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw the background for a native menu.
 *
 * @param pGraphics			pointer into graphics object where box should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 */
static BOOL DrawNativeMenuBackground( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeMenuDrawInfo pMenuDrawInfo;
		memset( &pMenuDrawInfo, 0, sizeof( pMenuDrawInfo ) );
		pMenuDrawInfo.version = 0;
		pMenuDrawInfo.menuType = kThemeMenuTypePopUp;

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		bRet = ( HIThemeDrawMenuBackground( &destRect, &pMenuDrawInfo, aBuffer.maContext, kHIThemeOrientationInverted ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw the background for a native edit field.
 *
 * @param pGraphics			pointer into graphics object where box should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 */
static BOOL DrawNativeEditBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	VCLBitmapBuffer aBuffer;
	BOOL bRet = aBuffer.Create( rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	if ( bRet )
	{
		HIThemeFrameDrawInfo pFrameInfo;
		InitEditFieldDrawInfo( &pFrameInfo, nState );

		HIRect destRect;
		destRect.origin.x = EDITBOX_TRIMWIDTH;
		destRect.origin.y = EDITBOX_TRIMWIDTH;
		destRect.size.width = rDestBounds.GetWidth() - 2*EDITBOX_TRIMWIDTH;
		destRect.size.height = rDestBounds.GetHeight() - 2*EDITBOX_TRIMWIDTH;

		// clear the active editing portion of the control
		float whiteColor[] = { 1.0, 1.0, 1.0, 1.0 };
		CGContextSetFillColor( aBuffer.maContext, whiteColor );
		CGContextFillRect( aBuffer.maContext, destRect );
		// draw frame around the background
		bRet = ( HIThemeDrawFrame( &destRect, &pFrameInfo, aBuffer.maContext, kHIThemeOrientationInverted ) == noErr );
	}

	if ( bRet )
	{
		aBuffer.ReleaseContext();
		pGraphics->mpVCLGraphics->drawBitmap( aBuffer.mpVCLBitmap, 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight(), rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() );
	}

	return bRet;
}

// =======================================================================

/**
 * Determine if support exists for drawing a particular native widget in the
 * interface.
 *
 * @param nType         control flavor that is requsted to be drawn
 * @param nPart         subpart of the control that is requested to be drawn
 * @return TRUE if supported, FALSE if not
 */
BOOL JavaSalGraphics::IsNativeControlSupported( ControlType nType, ControlPart nPart )
{
	BOOL isSupported = NULL;

	switch( nType )
	{
		case CTRL_PUSHBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_RADIOBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_CHECKBOX:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_COMBOBOX:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == HAS_BACKGROUND_TEXTURE ) )
				isSupported = TRUE;
			break;

		case CTRL_LISTBOX:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == HAS_BACKGROUND_TEXTURE ) )
				isSupported = TRUE;
			break;

		case CTRL_SCROLLBAR:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == PART_DRAW_BACKGROUND_HORZ ) || ( nPart == PART_DRAW_BACKGROUND_VERT ) )
				isSupported = TRUE;
			break;

		case CTRL_SPINBOX:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_PROGRESSBAR:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_TAB_ITEM:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_TAB_PANE:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_FIXEDBORDER:
		case CTRL_GROUPBOX:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_MENU_POPUP:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_EDITBOX:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == HAS_BACKGROUND_TEXTURE ) )
				isSupported = TRUE;
			break;

		default:
			isSupported = FALSE;
			break;
	}

	return isSupported;
}

// =======================================================================

/**
 * Called to perform hit testing on native widgets.  If the widget itself
 * conforms to standard OpenOffice.org hit testing and subcontrol positions,
 * overloading the OOo hit testing should not be necessary.
 *
 * @param nType                 primary type of control to be hit tested
 * @param nPart                 subportion of the control to be hit tested
 * @param rControlRegion
 * @param aPos                  coordinate to hit test
 * @param rControlHandle
 * @param rIsInside             return parameter indicating whether aPos was
 *                              within the control or not
 * @return TRUE if the function performed hit testing, FALSE if default OOo
 *      hit testing should be used instead
 */
BOOL JavaSalGraphics::hitTestNativeControl( ControlType nType, ControlPart nPart, const Region& rControlRegion, const Point& aPos, SalControlHandle& rControlHandle, BOOL& rIsInside )
{
	// [ed] Scrollbars are a special case:  in order to get proper regions,
	// a full description of the scrollbar is required including its values
	// and visible width.  We'll rely on the VCL scrollbar, which queried
	// these regions, to perform our hit testing.

	if( nType== CTRL_SCROLLBAR )
		return FALSE;

	Region aNativeBoundingRegion;
	Region aNativeContentRegion;
	if ( getNativeControlRegion( nType, nPart, rControlRegion, 0, ImplControlValue(), rControlHandle, OUString(), aNativeBoundingRegion, aNativeContentRegion ) )
		return aNativeBoundingRegion.IsInside( aPos );
	else
		return FALSE;
}

// =======================================================================

/**
 * Draw a widget using the native platform appearance.
 *
 * @param nType                 type of control to draw
 * @param nPart			subpart of the control to draw
 * @param nState                current control state (e.g. pressed, disabled)
 * @param rControlRegion        bounding region of the entire control in VCL
 *				frame coordinates
 * @param aValue		An optional value used for certain control types
 * @param rControlHandle	Platform dependent control data
 * @param aCaption		Caption title or string for the control.
 *				Contains keyboard shortcuts prefixed with ~
 * @return TRUE if drawing was successful, FALSE if drawing was not successful
 */
BOOL JavaSalGraphics::drawNativeControl( ControlType nType, ControlPart nPart, const Region& rControlRegion, ControlState nState, const ImplControlValue& aValue, SalControlHandle& rControlHandle, OUString aCaption )
{
	BOOL bOK = FALSE;

	switch( nType )
	{
		case CTRL_PUSHBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				mpVCLGraphics->drawPushButton( buttonRect.Left(), buttonRect.Top(), buttonRect.GetWidth(), buttonRect.GetHeight(), aCaption, ( nState & CTRL_STATE_ENABLED ), ( nState & CTRL_STATE_FOCUSED ), ( nState & CTRL_STATE_PRESSED ), ( nState & CTRL_STATE_DEFAULT ) );
				bOK = TRUE;
			}
			break;

		case CTRL_RADIOBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				mpVCLGraphics->drawRadioButton( buttonRect.Left(), buttonRect.Top(), buttonRect.GetWidth(), buttonRect.GetHeight(), aCaption, ( nState & CTRL_STATE_ENABLED ), ( nState & CTRL_STATE_FOCUSED ), ( nState & CTRL_STATE_PRESSED ), aValue.getTristateVal() );
				bOK = TRUE;
			}
			break;

		case CTRL_CHECKBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				mpVCLGraphics->drawCheckBox( buttonRect.Left(), buttonRect.Top(), buttonRect.GetWidth(), buttonRect.GetHeight(), aCaption, ( nState & CTRL_STATE_ENABLED ), ( nState & CTRL_STATE_FOCUSED ), ( nState & CTRL_STATE_PRESSED ), aValue.getTristateVal() );
				bOK = TRUE;
			}
			break;

		case CTRL_COMBOBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				bOK = DrawNativeComboBox( this, buttonRect, nState, aCaption );
			}
			break;

		case CTRL_LISTBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				bOK = DrawNativeListBox( this, buttonRect, nState, aCaption );
			}
			break;

		case CTRL_SCROLLBAR:
			if( ( nPart == PART_ENTIRE_CONTROL) || ( nPart == PART_DRAW_BACKGROUND_HORZ ) || ( nPart == PART_DRAW_BACKGROUND_VERT ) )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				ScrollbarValue *pValue = static_cast<ScrollbarValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeScrollBar( this, buttonRect, nState, pValue );
			}
			break;

		case CTRL_SPINBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				SpinbuttonValue *pValue = static_cast<SpinbuttonValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeSpinbox( this, buttonRect, nState, pValue );
			}
			break;

		case CTRL_PROGRESSBAR:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rControlRegion.GetBoundRect();
				ProgressbarValue *pValue = static_cast<ProgressbarValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeProgressbar( this, ctrlRect, nState, pValue );
			}
			break;

		case CTRL_TAB_ITEM:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rControlRegion.GetBoundRect();
				TabitemValue *pValue = static_cast<TabitemValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeTab( this, ctrlRect, nState, pValue );
			}
			break;

		case CTRL_TAB_PANE:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rControlRegion.GetBoundRect();
				// hack - on 10.3+ tab panes visually need to intersect the
				// middle of the associated segmented control.  Subtract
				// 15 off the height to shoehorn the drawing in.
				ctrlRect.setY( ctrlRect.getY() - CONTROL_TAB_PANE_TOP_OFFSET );
				ctrlRect.setHeight( ctrlRect.getHeight() + CONTROL_TAB_PANE_TOP_OFFSET );
				bOK = DrawNativeTabBoundingBox( this, ctrlRect, nState );
			}
			break;

		case CTRL_FIXEDBORDER:
		case CTRL_GROUPBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rControlRegion.GetBoundRect();
				bOK = DrawNativePrimaryGroupBox( this, ctrlRect, nState );
			}
			break;

		case CTRL_MENU_POPUP:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rControlRegion.GetBoundRect();
				bOK = DrawNativeMenuBackground( this, ctrlRect );
			}
			break;

		case CTRL_EDITBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rControlRegion.GetBoundRect();
				bOK = DrawNativeEditBox( this, ctrlRect, nState );
			}
			break;
	}

	return bOK;
}

// =======================================================================

/**
 * Draws the requested text for a native control.  If the caption of the
 * control is drawn by drawNativeControl, this function does not need to be
 * implemented.
 *
 * @param nType			type of control to draw
 * @param nPart			subpart of the control to draw
 * @param rControlRegion	bounding region of the complete control in VCL frame coordiantes.
 * @param nState		current control state (e.g. pressed, disabled)
 * @param aValue		An optional value used for certain control types
 * @param rControlHandle	Platform dependent control data
 * @param aCaption		Caption title or string for the control.
 *				Contains keyboard shortcuts prefixed with ~
 * @return TRUE if the text was drawn, FALSE if the control had its text drawn
 *	with drawNativeControl()
 */
BOOL JavaSalGraphics::drawNativeControlText( ControlType nType, ControlPart nPart, const Region& rControlRegion, ControlState nState, const ImplControlValue& aValue, SalControlHandle& rControlHandle, OUString aCaption )
{
	return FALSE;
}

// =======================================================================

/**
 * Obtains information about the actual bounding area and available drawing
 * area of a native control.
 *
 * @param nType			type of control to query
 * @param nPart			subpart of the control to query
 * @param rControlRegion	bounding region of the control in VCL frame coordinates
 * @param nState		current control state (e.g. pressed, disabled)
 * @param aValue		An optional value used for certain control types
 * @param rControlHandle	Platform dependent control data
 * @param aCaption		Caption title or string for the control.
 *				Contains keyboard shortcuts prefixed with ~
 * @param rNativeBoundingRegion	return parameter that contains the true bounds
 *				of the control along with any platform specific
 *				control adornment (shadows, etc.)
 * @param rNativeContentRegion	return parameter that indicates the region which
 *				can safely be overdrawn with custom content
 *				without overdrawing the system adornments of
 *				the control
 * @return TRUE if appropriate information was returned about any native widget
 *	drawing areas, FALSE if the entire control region can be considered
 *	an accurate enough representation of the native widget area
 */
BOOL JavaSalGraphics::getNativeControlRegion( ControlType nType, ControlPart nPart, const Region& rControlRegion, ControlState nState, const ImplControlValue& aValue, SalControlHandle& rControlHandle, OUString aCaption, Region &rNativeBoundingRegion, Region &rNativeContentRegion )
{
	BOOL bReturn = FALSE;

	switch( nType )
	{
		case CTRL_PUSHBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( mpVCLGraphics->getPreferredPushButtonBounds( buttonRect.Left(), buttonRect.Top(), buttonRect.GetWidth(), buttonRect.GetHeight(), aCaption ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );
				bReturn = TRUE;
			}
			break;

		case CTRL_RADIOBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( mpVCLGraphics->getPreferredRadioButtonBounds( buttonRect.Left(), buttonRect.Top(), buttonRect.GetWidth(), buttonRect.GetHeight(), aCaption ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );
				bReturn = TRUE;
			}
			break;

		case CTRL_CHECKBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( mpVCLGraphics->getPreferredCheckBoxBounds( buttonRect.Left(), buttonRect.Top(), buttonRect.GetWidth(), buttonRect.GetHeight(), aCaption ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );
				bReturn = TRUE;
			}
			break;

		case CTRL_COMBOBOX:
		case CTRL_LISTBOX:
			{
				Rectangle comboBoxRect = rControlRegion.GetBoundRect();

				HIThemeButtonDrawInfo aButtonDrawInfo;
				InitButtonDrawInfo( &aButtonDrawInfo, nState );

				HIShapeRef preferredShape;
				HIRect destRect;
				destRect.origin.x = comboBoxRect.Left();
				destRect.origin.y = comboBoxRect.Top();
				destRect.size.width = comboBoxRect.GetWidth();
				destRect.size.height = comboBoxRect.GetHeight();
				if ( HIThemeGetButtonShape( &destRect, &aButtonDrawInfo, &preferredShape ) == noErr )
				{
					HIRect preferredRect;
					HIShapeGetBounds( preferredShape, &preferredRect );
					CFRelease( preferredShape );

					switch( nPart )
					{
						case PART_BUTTON_DOWN:
							{
								Point topLeft( (long)preferredRect.origin.x + (long)preferredRect.size.width - COMBOBOX_BUTTON_WIDTH, (long)preferredRect.origin.y );
								Size boundsSize( COMBOBOX_BUTTON_WIDTH + COMBOBOX_BUTTON_TRIMWIDTH, (long)preferredRect.size.height );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;

						case PART_SUB_EDIT:
							{
								Point topLeft( (long)preferredRect.origin.x, (long)preferredRect.origin.y );
								Size boundsSize( (long)preferredRect.size.width - COMBOBOX_BUTTON_WIDTH, (long)preferredRect.size.height );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;
					}
				}
			}
			break;

		case CTRL_SCROLLBAR:
			{
				Rectangle comboBoxRect = rControlRegion.GetBoundRect();

				ScrollbarValue *pValue = static_cast<ScrollbarValue *> ( aValue.getOptionalVal() );

				HIThemeTrackDrawInfo pTrackDrawInfo;
				HIScrollBarTrackInfo pScrollBarTrackInfo;
				InitScrollBarTrackInfo( &pTrackDrawInfo, &pScrollBarTrackInfo, nState, comboBoxRect, pValue );

				HIRect bounds;

				switch ( nPart )
				{
					case PART_ENTIRE_CONTROL:
						HIThemeGetTrackBounds( &pTrackDrawInfo, &bounds );
						break;

					case PART_BUTTON_LEFT:
						HIThemeGetTrackPartBounds( &pTrackDrawInfo, kAppearancePartDownButton, &bounds );
						bounds.origin.x++;
						bounds.origin.x -= bounds.size.width;
						break;

					case PART_BUTTON_UP:
						HIThemeGetTrackPartBounds( &pTrackDrawInfo, kAppearancePartDownButton, &bounds );
						if ( !vcl::IsRunningPanther() )
							bounds.origin.y++;
						bounds.origin.y -= bounds.size.height;
						break;

					case PART_BUTTON_RIGHT:
						HIThemeGetTrackPartBounds( &pTrackDrawInfo, kAppearancePartDownButton, &bounds );
						bounds.origin.x++;

					case PART_BUTTON_DOWN:
						HIThemeGetTrackPartBounds( &pTrackDrawInfo, kAppearancePartDownButton, &bounds );
						if ( !vcl::IsRunningPanther() )
						{
							bounds.origin.y++;
							bounds.size.height--;
						}
						break;

					case PART_TRACK_HORZ_LEFT:
					case PART_TRACK_VERT_UPPER:
						HIThemeGetTrackPartBounds( &pTrackDrawInfo, kAppearancePartPageUpArea, &bounds );
						if( ! bounds.size.width && ! bounds.size.height )
						{
							// disabled control or other invalid settings.  Set to the entire
							// track.

							HIThemeGetScrollBarTrackRect( &pTrackDrawInfo.bounds, &pScrollBarTrackInfo, ( ( comboBoxRect.GetWidth() > comboBoxRect.GetHeight() ) ? true : false ), &bounds );
						}
						break;

					case PART_TRACK_HORZ_RIGHT:
					case PART_TRACK_VERT_LOWER:
						HIThemeGetTrackPartBounds( &pTrackDrawInfo, kAppearancePartPageDownArea, &bounds );
						if( ! bounds.size.width && ! bounds.size.height )
						{
							// disabled control or other invalid settings.  Set to the entire
							// track.

							HIThemeGetScrollBarTrackRect( &pTrackDrawInfo.bounds, &pScrollBarTrackInfo, ( ( comboBoxRect.GetWidth() > comboBoxRect.GetHeight() ) ? true : false ), &bounds );
						}
						break;

					case PART_THUMB_HORZ:
					case PART_THUMB_VERT:
						HIThemeGetTrackPartBounds( &pTrackDrawInfo, kAppearancePartIndicator, &bounds );
						if( ! bounds.size.width && ! bounds.size.height )
						{
							// disabled control or other invalid settings.  Set to the entire
							// track.

							HIThemeGetScrollBarTrackRect( &pTrackDrawInfo.bounds, &pScrollBarTrackInfo, ( ( comboBoxRect.GetWidth() > comboBoxRect.GetHeight() ) ? true : false ), &bounds );
						}
						break;
				}

				Point topLeft( (long)(comboBoxRect.Left()+bounds.origin.x), (long)(comboBoxRect.Top()+bounds.origin.y) );
				Size boundsSize( (long)bounds.size.width, (long)bounds.size.height );
				rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_SPINBOX:
			{
				Rectangle spinboxRect = rControlRegion.GetBoundRect();

				HIThemeButtonDrawInfo aThemeButtonDrawInfo;
				SpinbuttonValue *pValue = static_cast<SpinbuttonValue *> ( aValue.getOptionalVal() );
				InitSpinbuttonDrawInfo( &aThemeButtonDrawInfo, pValue );

				HIShapeRef preferredShape;
				HIRect destRect;
				destRect.origin.x = spinboxRect.Left();
				destRect.origin.y = spinboxRect.Top();
				destRect.size.width = spinboxRect.GetWidth();
				destRect.size.height = spinboxRect.GetHeight();
				if ( HIThemeGetButtonShape( &destRect, &aThemeButtonDrawInfo, &preferredShape ) == noErr )
				{
					HIRect preferredRect;
					HIShapeGetBounds( preferredShape, &preferredRect );
					CFRelease( preferredShape );

					// note that HIThemeGetButtonShape won't clip the width to the actual recommended width of spinner arrows
					// leave room for left edge adornments
					if( preferredRect.size.width > 20 )
						preferredRect.size.width = 20;

					switch( nPart )
					{
						case PART_BUTTON_UP:
							{
								Point topLeft( (long)(spinboxRect.Right()-preferredRect.size.width), (long)(spinboxRect.Top()) );
								Size boundsSize( (long)(preferredRect.size.width), (long)(preferredRect.size.height / 2) );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;

						case PART_BUTTON_DOWN:
							{
								Point topLeft( (long)(spinboxRect.Right()-preferredRect.size.width), (long)(spinboxRect.Top()+(preferredRect.size.height / 2)) );
								Size boundsSize( (long)preferredRect.size.width, (long)(preferredRect.size.height / 2) );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;
					}
				}
			}
			break;

		case CTRL_PROGRESSBAR:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle controlRect = rControlRegion.GetBoundRect();

				ProgressbarValue *pValue = static_cast<ProgressbarValue *> ( aValue.getOptionalVal() );

				HIThemeTrackDrawInfo pTrackDrawInfo;
				InitProgressbarTrackInfo( &pTrackDrawInfo, nState, controlRect, pValue );

				HIRect bounds;
				HIThemeGetTrackBounds( &pTrackDrawInfo, &bounds );

				Point topLeft( (long)(controlRect.Left()+bounds.origin.x), (long)(controlRect.Top()+bounds.origin.y) );
				Size boundsSize( (long)bounds.size.width, (long)bounds.size.height );
				rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_TAB_ITEM:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle controlRect = rControlRegion.GetBoundRect();

				TabitemValue *pValue = static_cast<TabitemValue *> ( aValue.getOptionalVal() );

				HIThemeTabDrawInfo104 pTabDrawInfo;
				InitTabDrawInfo( &pTabDrawInfo, nState, pValue );

				HIRect proposedBounds;
				proposedBounds.origin.x = 0;
				proposedBounds.origin.y = 0;
				proposedBounds.size.width = controlRect.Right() - controlRect.Left();
				proposedBounds.size.height = controlRect.Bottom() - controlRect.Top();

				HIShapeRef tabShape;
				HIThemeGetTabShape( &proposedBounds, (HIThemeTabDrawInfo *)&pTabDrawInfo, &tabShape );

				HIRect preferredRect;
				HIShapeGetBounds( tabShape, &preferredRect );
				CFRelease( tabShape );

				Point topLeft( controlRect.Left(), controlRect.Top() );
				Size boundsSize( (long)preferredRect.size.width, (long)preferredRect.size.height );
				rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_TAB_PANE:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// for now, assume tab panes will occupy the full rectangle and
				// not require bound adjustment.
				Rectangle controlRect = rControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_FIXEDBORDER:
		case CTRL_GROUPBOX:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// for now, assume primary group boxes will occupy the full rectangle and
				// not require bound adjustment.
				Rectangle controlRect = rControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_MENU_POPUP:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// we can draw menu backgrounds for any size rectangular area
				Rectangle controlRect = rControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_EDITBOX:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// fill entire control area with edit box
				Rectangle controlRect = rControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				Point contentTopLeft( controlRect.Left() + EDITBOX_TRIMWIDTH, controlRect.Top() + EDITBOX_TRIMWIDTH );
				Size contentSize( controlRect.GetWidth() - 2*EDITBOX_TRIMWIDTH, controlRect.GetHeight() - 2*EDITBOX_TRIMWIDTH );
				rNativeContentRegion = Region( Rectangle( contentTopLeft, contentSize ) );

				bReturn = TRUE;
			}
			break;
	}

	return bReturn;
}

