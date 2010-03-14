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
 *	 - GNU General Public License Version 2.1
 *
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 Planamesa Inc.
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

#ifndef _SV_SALFRAME_H
#define _SV_SALFRAME_H

#include <list>

#ifndef _SV_SV_H
#include <vcl/sv.h>
#endif
#ifndef _SV_SYSDATA_HXX 
#include <vcl/sysdata.hxx>
#endif
#ifndef _SV_SALFRAME_HXX
#include <vcl/salframe.hxx>
#endif
#ifndef _SV_SALGEOM_HXX
#include <vcl/salgeom.hxx>
#endif

namespace vcl
{
class com_sun_star_vcl_VCLEvent;
class com_sun_star_vcl_VCLEventQueue;
class com_sun_star_vcl_VCLFrame;
class com_sun_star_vcl_VCLMenuBar;
}

class JavaSalGraphics;
class JavaSalMenu;
class JavaSalObject;
class SalBitmap;

// ----------------
// - JavaSalFrame -
// ----------------

class JavaSalFrame : public SalFrame
{
public:
	::vcl::com_sun_star_vcl_VCLFrame*	mpVCLFrame;
	JavaSalGraphics*		mpGraphics;
	ULONG					mnStyle;
	JavaSalFrame*			mpParent;
	BOOL					mbGraphics;
	BOOL					mbVisible;
	::std::list< JavaSalFrame* >	maChildren;
	SystemEnvData			maSysData;
	BOOL					mbCenter;
	SalFrameGeometry		maOriginalGeometry;
	BOOL					mbFullScreen;
	BOOL					mbPresentation;
	JavaSalMenu*			mpMenuBar;
	BOOL					mbInSetPosSize;
	BOOL					mbInShow;
	::std::list< JavaSalObject* >	maObjects;
	::std::list< JavaSalObject* >	maVisibleObjects;
	::rtl::OUString			maTitle;
	BOOL					mbShowOnlyMenus;
	BOOL					mbInShowOnlyMenus;
	::std::list< JavaSalMenu* >	maUpdateMenuList;

							JavaSalFrame();
	virtual					~JavaSalFrame();

	void					AddObject( JavaSalObject *pObject, bool bVisible );
	bool					IsFloatingFrame();
	bool					IsUtilityWindow();
	void					RemoveObject( JavaSalObject *pObject, bool bDeleted );
	void					FlushAllObjects();

	virtual SalGraphics*	GetGraphics();
	virtual void			ReleaseGraphics( SalGraphics* pGraphics );
	virtual BOOL			PostEvent( void* pData );
	virtual void			SetTitle( const XubString& rTitle );
	virtual void			SetIcon( USHORT nIcon );
	virtual void			SetMenu( SalMenu *pSalMenu );
	virtual void			DrawMenuBar();
	virtual void			SetExtendedFrameStyle( SalExtStyle nExtStyle );
	virtual void			Show( BOOL bVisible, BOOL bNoActivate = FALSE );
	virtual void			Enable( BOOL bEnable );
	virtual void			SetMinClientSize( long nWidth, long nHeight );
	virtual void			SetMaxClientSize( long nWidth, long nHeight );
	virtual void			SetPosSize( long nX, long nY, long nWidth, long nHeight, USHORT nFlags );
	virtual void			GetClientSize( long& rWidth, long& rHeight );
	virtual void			GetWorkArea( Rectangle& rRect );
	virtual SalFrame*		GetParent() const;
	virtual void			SetWindowState( const SalFrameState* pState );
	virtual BOOL			GetWindowState( SalFrameState* pState );
	virtual void			ShowFullScreen( BOOL bFullScreen, sal_Int32 nDisplay );
	virtual void			StartPresentation( BOOL bStart );
	virtual void			SetAlwaysOnTop( BOOL bOnTop );
	virtual void			ToTop( USHORT nFlags );
	virtual void			SetPointer( PointerStyle ePointerStyle );
	virtual void			CaptureMouse( BOOL bMouse );
	virtual void			SetPointerPos( long nX, long nY );
	virtual void			Flush();
	virtual void			Sync();
	virtual void			SetInputContext( SalInputContext* pContext );
	virtual void			EndExtTextInput( USHORT nFlags );
	virtual String			GetKeyName( USHORT nKeyCode );
	virtual String			GetSymbolKeyName( const XubString& rFontName, USHORT nKeyCode );
	virtual BOOL			MapUnicodeToKeyCode( sal_Unicode aUnicode, LanguageType aLangType, KeyCode& rKeyCode );
	virtual LanguageType	GetInputLanguage();
	virtual SalBitmap*		SnapShot();
	virtual void			UpdateSettings( AllSettings& rSettings );
	virtual void			Beep( SoundType eSoundType );
	virtual const SystemEnvData*	GetSystemData() const;
	virtual void			SetBackgroundBitmap( SalBitmap* );
	virtual SalPointerState	GetPointerState();
	virtual void			SetParent( SalFrame* pNewParent );
	virtual bool			SetPluginParent( SystemParentData* pNewParent );
	virtual void			ResetClipRegion();
	virtual void			BeginSetClipRegion( ULONG nRects );
	virtual void			UnionClipRegion( long nX, long nY, long nWidth, long nHeight );
	virtual void			EndSetClipRegion();
	virtual void			SetScreenNumber( unsigned int nScreen );
};

// Note: this must not be static as the symbol will be loaded by the framework
// module
extern "C" sal_Bool SAL_DLLPUBLIC_EXPORT IsShowOnlyMenusWindow( Window *pWindow );
extern "C" void SAL_DLLPUBLIC_EXPORT ShowOnlyMenusForWindow( Window *pWindow, sal_Bool bShowOnlyMenus );

#endif // _SV_SALFRAME_H
