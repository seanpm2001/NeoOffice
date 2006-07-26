/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to
 *  the terms of GNU General Public License Version 2.1.
 *
 *
 *    GNU General Public License Version 2.1
 *    =============================================
 *    Copyright 2005 by Sun Microsystems, Inc.
 *    901 San Antonio Road, Palo Alto, CA 94303, USA
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public
 *    License version 2.1, as published by the Free Software Foundation.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 *
 *    Modified July 2006 by Patrick Luby. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/

#include "splash.hxx"
#include <stdio.h>
#ifndef _UTL_BOOTSTRAP_HXX
#include <unotools/bootstrap.hxx>
#endif
#ifndef _VOS_PROCESS_HXX_
#include <vos/process.hxx>
#endif
#ifndef _URLOBJ_HXX
#include <tools/urlobj.hxx>
#endif
#ifndef _STREAM_HXX
#include <tools/stream.hxx>
#endif
#ifndef _SFX_HRC
#include <sfx2/sfx.hrc>
#endif
#ifndef _SV_SVAPP_HXX
#include <vcl/svapp.hxx>
#endif
#ifndef _RTL_BOOTSTRAP_HXX_
#include <rtl/bootstrap.hxx>
#endif

#include <com/sun/star/registry/XRegistryKey.hpp>
#include <rtl/logfile.hxx>


#define NOT_LOADED  ((long)-1)

using namespace ::rtl;
using namespace ::com::sun::star::registry;

namespace desktop
{

SplashScreen::SplashScreen(const Reference< XMultiServiceFactory >& rSMgr)
    : IntroWindow()
    , _vdev(*((IntroWindow*)this))
    , _cProgressFrameColor(NOT_LOADED)
    , _cProgressBarColor(NOT_LOADED)
	, _iProgress(0)
	, _iMax(100)
	, _bPaintBitmap(sal_True)
	, _bPaintProgress(sal_False)
    , _tlx(NOT_LOADED)
    , _tly(NOT_LOADED)
    , _barwidth(NOT_LOADED)
    , _xoffset(12)
	, _yoffset(18)
    , _barheight(NOT_LOADED)
    , _barspace(2)
{
	_rFactory = rSMgr;

    loadConfig();
	initBitmap();
	Size aSize = _aIntroBmp.GetSizePixel();
	SetOutputSizePixel( aSize );
    _vdev.SetOutputSizePixel( aSize );
	_height = aSize.Height();
	_width = aSize.Width();

#ifdef USE_JAVA
    _pProgressBar = new ProgressBar( this );
    _pProgressBar->SetPaintTransparent( TRUE );
    _barheight = _pProgressBar->GetOutputSizePixel().Height();
#endif	// USE_JAVA

    if (_width > 500)
    {
        Point xtopleft(212,216);
        if ( NOT_LOADED == _tlx || NOT_LOADED == _tly )
        {
            _tlx = xtopleft.X();    // top-left x
            _tly = xtopleft.Y();    // top-left y
        }
        if ( NOT_LOADED == _barwidth )
            _barwidth = 263;
        if ( NOT_LOADED == _barheight )
            _barheight = 8;

#ifdef USE_JAVA
        if ( _barheight > 8 )
            _tly -= _barheight - 8;
#endif	// USE_JAVA
    }
    else
    {
        if ( NOT_LOADED == _barwidth )
            _barwidth  = _width - (2 * _xoffset);
        if ( NOT_LOADED == _barheight )
            _barheight = 6;
        if ( NOT_LOADED == _tlx || NOT_LOADED == _tly )
        {
            _tlx = _xoffset;           // top-left x
            _tly = _height - _yoffset; // top-left y
        }

#ifdef USE_JAVA
        if ( _barheight > 6 )
            _tly -= _barheight - 6;
#endif	// USE_JAVA
    }

    if ( NOT_LOADED == _cProgressFrameColor.GetColor() )
        _cProgressFrameColor = Color( COL_LIGHTGRAY );

    if ( NOT_LOADED == _cProgressBarColor.GetColor() )
    {
        // progress bar: new color only for big bitmap format
        if ( _width > 500 )
            _cProgressBarColor = Color( 157, 202, 18 );
        else
            _cProgressBarColor = Color( COL_BLUE );
    }

#ifdef USE_JAVA
    _pProgressBar->SetPosSizePixel( Point( _tlx, _tly ), Size( _barwidth, _barheight ) );
#endif	// USE_JAVA

    Application::AddEventListener(
		LINK( this, SplashScreen, AppEventListenerHdl ) );

    SetBackgroundBitmap( _aIntroBmp );
}

SplashScreen::~SplashScreen()
{
	Application::RemoveEventListener(
		LINK( this, SplashScreen, AppEventListenerHdl ) );
	Hide();

#ifdef USE_JAVA
	delete _pProgressBar;
#endif	// USE_JAVA
}

void SAL_CALL SplashScreen::start(const OUString& aText, sal_Int32 nRange)
	throw (RuntimeException)
{
	_iMax = nRange;
	if (_bVisible) {
		::vos::OGuard aSolarGuard( Application::GetSolarMutex() );
		Show();
#ifdef USE_JAVA
        _pProgressBar->Show();
#endif	// USE_JAVA
		Paint(Rectangle());
	}
}
void SAL_CALL SplashScreen::end()
	throw (RuntimeException)
{
	_iProgress = _iMax;
	updateStatus();
	if (_bVisible) Hide();
#ifdef USE_JAVA
	if (_bVisible) _pProgressBar->Hide();
#endif	// USE_JAVA
}
void SAL_CALL SplashScreen::reset()
	throw (RuntimeException)
{
	_iProgress = 0;
    Show();
#ifdef USE_JAVA
    _pProgressBar->Show();
#endif	// USE_JAVA
	updateStatus();
}

void SAL_CALL SplashScreen::setText(const OUString& aText)
	throw (RuntimeException)
{
    if (_bVisible) {
        Show();
#ifdef USE_JAVA
        _pProgressBar->Show();
#endif	// USE_JAVA
    }
}

void SAL_CALL SplashScreen::setValue(sal_Int32 nValue)
	throw (RuntimeException)
{
    RTL_LOGFILE_CONTEXT( aLog, "::SplashScreen::setValue (lo119109)" );
    RTL_LOGFILE_CONTEXT_TRACE1( aLog, "value=%d", nValue );

    ::vos::OGuard aSolarGuard( Application::GetSolarMutex() );
    if (_bVisible) {
        Show();
#ifdef USE_JAVA
        _pProgressBar->Show();
#endif	// USE_JAVA
	    if (nValue >= _iMax) _iProgress = _iMax;
	    else _iProgress = nValue;
	    updateStatus();
    }
}

// XInitialize
void SAL_CALL
SplashScreen::initialize( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any>& aArguments )
	throw (RuntimeException)
{
	::osl::ClearableMutexGuard	aGuard(	_aMutex );
	if (aArguments.getLength() > 0)
		aArguments[0] >>= _bVisible;
}

void SplashScreen::updateStatus()
{
	if (!_bVisible) return;
	if (!_bPaintProgress) _bPaintProgress = sal_True;
	//_bPaintBitmap=sal_False;
	Paint(Rectangle());
	//_bPaintBitmap=sal_True;
}

// internal private methods
IMPL_LINK( SplashScreen, AppEventListenerHdl, VclWindowEvent *, inEvent )
{
	if ( inEvent != 0 )
	{
        // Paint( Rectangle() );
		switch ( inEvent->GetId() )
		{
			case VCLEVENT_WINDOW_SHOW:
				Paint( Rectangle() );
				break;
			default:
				break;
		}
	}
	return 0;
}

OUString implReadBootstrapKey( const ::rtl::Bootstrap& _rIniFile, const OUString& _rKey )
{
    OUString sValue;
    _rIniFile.getFrom( _rKey, sValue );
    return sValue;
}

void SplashScreen::loadConfig()
{
    // detect execute path
    ::vos::OStartupInfo().getExecutableFile( _sExecutePath );
    sal_uInt32 nLastIndex = _sExecutePath.lastIndexOf( '/' );
    if ( nLastIndex > 0 )
        _sExecutePath = _sExecutePath.copy( 0, nLastIndex + 1 );

    // read keys from soffice.ini (sofficerc)
    OUString sIniFileName = _sExecutePath;
    sIniFileName += OUString( RTL_CONSTASCII_USTRINGPARAM( SAL_CONFIGFILE( "soffice" ) ) );
    rtl::Bootstrap aIniFile( sIniFileName );

    OUString sProgressFrameColor = implReadBootstrapKey(
        aIniFile, OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressFrameColor" ) ) );
    OUString sProgressBarColor = implReadBootstrapKey(
        aIniFile, OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressBarColor" ) ) );
    OUString sSize = implReadBootstrapKey(
        aIniFile, OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressSize" ) ) );
    OUString sPosition = implReadBootstrapKey(
        aIniFile, OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressPosition" ) ) );

    if ( sProgressFrameColor.getLength() )
    {
        UINT8 nRed = 0;
        UINT8 nGreen = 0;
        UINT8 nBlue = 0;
        sal_Int32 idx = 0;
        sal_Int32 temp = sProgressFrameColor.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            nRed = static_cast< UINT8 >( temp );
            temp = sProgressFrameColor.getToken( 0, ',', idx ).toInt32();
        }
        if ( idx != -1 )
        {
            nGreen = static_cast< UINT8 >( temp );
            nBlue = static_cast< UINT8 >( sProgressFrameColor.getToken( 0, ',', idx ).toInt32() );
            _cProgressFrameColor = Color( nRed, nGreen, nBlue );
        }
    }

    if ( sProgressBarColor.getLength() )
    {
        UINT8 nRed = 0;
        UINT8 nGreen = 0;
        UINT8 nBlue = 0;
        sal_Int32 idx = 0;
        sal_Int32 temp = sProgressBarColor.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            nRed = static_cast< UINT8 >( temp );
            temp = sProgressBarColor.getToken( 0, ',', idx ).toInt32();
        }
        if ( idx != -1 )
        {
            nGreen = static_cast< UINT8 >( temp );
            nBlue = static_cast< UINT8 >( sProgressBarColor.getToken( 0, ',', idx ).toInt32() );
            _cProgressBarColor = Color( nRed, nGreen, nBlue );
        }
    }

    if ( sSize.getLength() )
    {
        sal_Int32 idx = 0;
        sal_Int32 temp = sSize.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            _barwidth = temp;
            _barheight = sSize.getToken( 0, ',', idx ).toInt32();
        }
    }

    if ( _barheight >= 10 )
        _barspace = 3;  // more space between frame and bar

    if ( sPosition.getLength() )
    {
        sal_Int32 idx = 0;
        sal_Int32 temp = sPosition.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            _tlx = temp;
            _tly = sPosition.getToken( 0, ',', idx ).toInt32();
        }
    }
}

void SplashScreen::initBitmap()
{
    rtl::OUString aLogo( RTL_CONSTASCII_USTRINGPARAM( "1" ) );
    aLogo = ::utl::Bootstrap::getLogoData( aLogo );
    sal_Bool bShowLogo = (sal_Bool)aLogo.toInt32();

	if ( bShowLogo )
	{
		xub_StrLen nIndex = 0;
        String aBmpFileName( UniString(RTL_CONSTASCII_USTRINGPARAM("intro.bmp")) );

        bool haveBitmap = false;

        // First, try to use custom bitmap data.
        rtl::OUString value;
        rtl::Bootstrap::get(
            rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CustomDataUrl" ) ), value );
        if ( value.getLength() > 0 )
        {
            if ( value[ value.getLength() - 1 ] != sal_Unicode( '/' ) )
                value += rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/program" ) );
            else
                value += rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "program" ) );

            INetURLObject aObj( value, INET_PROT_FILE );
            aObj.insertName( aBmpFileName );

            SvFileStream aStrm( aObj.PathToFileName(), STREAM_STD_READ );
            if ( !aStrm.GetError() )
            {
                // Default case, we load the intro bitmap from a seperate file
                // (e.g. staroffice_intro.bmp or starsuite_intro.bmp)
                aStrm >> _aIntroBmp;
                haveBitmap = true;
            }
        }

        // Then, try to use bitmap located in the same directory as the executable.
        if ( !haveBitmap )
        {
            INetURLObject aObj( _sExecutePath, INET_PROT_FILE );
            aObj.insertName( aBmpFileName );

            SvFileStream aStrm( aObj.PathToFileName(), STREAM_STD_READ );
            if ( !aStrm.GetError() )
            {
                // Default case, we load the intro bitmap from a seperate file
                // (e.g. staroffice_intro.bmp or starsuite_intro.bmp)
                aStrm >> _aIntroBmp;
                haveBitmap = true;
            }
        }

        if ( !haveBitmap )
		{
			// Save case:
			// Create resource manager for intro bitmap. Due to our problem that we don't have
			// any language specific information, we have to search for the correct resource
			// file. The bitmap resource is language independent.
			const USHORT nResId = RID_DEFAULTINTRO;
            ByteString aMgrName( "iso" );
            aMgrName += ByteString::CreateFromInt32(SUPD); // current build version
            ResMgr* pLabelResMgr = ResMgr::CreateResMgr( aMgrName.GetBuffer() );
			if ( !pLabelResMgr )
			{
				// no "iso" resource -> search for "ooo" resource
                aMgrName = "ooo";
                aMgrName += ByteString::CreateFromInt32(SUPD); // current build version
                pLabelResMgr = ResMgr::CreateResMgr( aMgrName.GetBuffer() );
			}
			if ( pLabelResMgr )
			{
				ResId aIntroBmpRes( nResId, pLabelResMgr );
				_aIntroBmp = Bitmap( aIntroBmpRes );
			}
			delete pLabelResMgr;
		}
	}
}

void SplashScreen::Paint( const Rectangle& r)
{
	if(!_bVisible) return;

	// draw bitmap
#ifdef USE_JAVA
	if (_bPaintBitmap && !_iProgress)
#else	// USE_JAVA
	if (_bPaintBitmap)
#endif	// USE_JAVA
		_vdev.DrawBitmap( Point(), _aIntroBmp );

#ifndef USE_JAVA
	if (_bPaintProgress) {
		// draw progress...
		long length = (_iProgress * _barwidth / _iMax) - (2 * _barspace);
		if (length < 0) length = 0;

		// border
		_vdev.SetFillColor();
        _vdev.SetLineColor( _cProgressFrameColor );
        _vdev.DrawRect(Rectangle(_tlx, _tly, _tlx+_barwidth, _tly+_barheight));
        _vdev.SetFillColor( _cProgressBarColor );
		_vdev.SetLineColor();
        Rectangle aRect(_tlx+_barspace, _tly+_barspace, _tlx+_barspace+length, _tly+_barheight-_barspace);
		_vdev.DrawRect(Rectangle(_tlx+_barspace, _tly+_barspace,
			_tlx+_barspace+length, _tly+_barheight-_barspace));
	}
#endif	// !USE_JAVA
    Size aSize =  GetOutputSizePixel();
    Size bSize =  _vdev.GetOutputSizePixel();
    //_vdev.Flush();
    //_vdev.DrawOutDev(Point(), GetOutputSize(), Point(), GetOutputSize(), *((IntroWindow*)this) );

#ifdef USE_JAVA
    if ( _bPaintBitmap && !_iProgress )
        DrawOutDev(Point(), GetOutputSizePixel(), Point(), _vdev.GetOutputSizePixel(), _vdev );
	if ( _bPaintProgress )
    {
        // HACK: clip out the top pixel as it will be merely background color
        Rectangle aClipRect( Point( 0, 1 ), Size( GetSizePixel().Width(), GetSizePixel().Height() - 1 ) );
        Region aOldClipRgn = _pProgressBar->GetClipRegion();
        _pProgressBar->SetClipRegion( Region( aClipRect ) );
        _pProgressBar->SetValue( _iProgress );
        _pProgressBar->SetClipRegion( aOldClipRgn );
    }
#else	// USE_JAVA
    DrawOutDev(Point(), GetOutputSizePixel(), Point(), _vdev.GetOutputSizePixel(), _vdev );
#endif	// USE_JAVA
	//Flush();
}


// get service instance...
SplashScreen *SplashScreen::_pINSTANCE = NULL;
osl::Mutex SplashScreen::_aMutex;

Reference< XInterface > SplashScreen::getInstance(const Reference< XMultiServiceFactory >& rSMgr)
{
	if ( _pINSTANCE == 0 )
	{
		osl::MutexGuard guard(_aMutex);
		if (_pINSTANCE == 0)
			return (XComponent*)new SplashScreen(rSMgr);
	}

	return (XComponent*)0;
}

// static service info...
const char* SplashScreen::interfaces[] =
{
    "com.sun.star.task.XStartusIndicator",
    "com.sun.star.lang.XInitialization",
    NULL,
};
const sal_Char *SplashScreen::serviceName = "com.sun.star.office.SplashScreen";
const sal_Char *SplashScreen::implementationName = "com.sun.star.office.comp.SplashScreen";
const sal_Char *SplashScreen::supportedServiceNames[] = {"com.sun.star.office.SplashScreen", NULL};
OUString SplashScreen::impl_getImplementationName()
{
	return OUString::createFromAscii(implementationName);
}
Sequence<OUString> SplashScreen::impl_getSupportedServiceNames()
{
	Sequence<OUString> aSequence;
	for (int i=0; supportedServiceNames[i]!=NULL; i++) {
		aSequence.realloc(i+1);
		aSequence[i]=(OUString::createFromAscii(supportedServiceNames[i]));
	}
	return aSequence;
}

}

