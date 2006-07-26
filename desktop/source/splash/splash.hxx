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

#ifndef _COM_SUN_STAR_LANG_XSERVICEINFO_HPP_
#include <com/sun/star/lang/XServiceInfo.hpp>
#endif
#ifndef _COM_SUN_STAR_UNO_EXCEPTION_HPP_
#include <com/sun/star/uno/Exception.hpp>
#endif
#ifndef _COM_SUN_STAR_UNO_REFERENCE_H_
#include <com/sun/star/uno/Reference.h>
#endif
#ifndef _COM_SUN_STAR_LANG_XCOMPONENT_HPP_
#include <com/sun/star/lang/XComponent.hpp>
#endif
#ifndef _COM_SUN_STAR_TASK_XSTATUSINDICATOR_HPP_
#include <com/sun/star/task/XStatusIndicator.hpp>
#endif
#ifndef _COM_SUN_STAR_LANG_XINITIALIZATION_HPP_
#include <com/sun/star/lang/XInitialization.hpp>
#endif
#ifndef _CPPUHELPER_IMPLBASE2_HXX_
#include <cppuhelper/implbase2.hxx>
#endif
#ifndef _CPPUHELPER_INTERFACECONTAINER_H_
#include <cppuhelper/interfacecontainer.h>
#endif
#ifndef _SV_INTROWIN_HXX
#include <vcl/introwin.hxx>
#endif
#ifndef _SV_BITMAP_HXX
#include <vcl/bitmap.hxx>
#endif
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <osl/mutex.hxx>
#include <vcl/virdev.hxx>

#ifdef USE_JAVA

#ifndef _PRGSBAR_HXX
#include <svtools/prgsbar.hxx>
#endif

#endif	// USE_JAVA


using namespace ::rtl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::task;

namespace desktop {

class  SplashScreen
	: public ::cppu::WeakImplHelper2< XStatusIndicator, XInitialization >
	, public IntroWindow
{
private:
	// don't allow anybody but ourselves to create instances of this class
	SplashScreen(const SplashScreen&);
	SplashScreen(void);
	SplashScreen operator =(const SplashScreen&);

	SplashScreen(const Reference< XMultiServiceFactory >& xFactory);

	DECL_LINK( AppEventListenerHdl, VclWindowEvent * );
    virtual ~SplashScreen();
    void loadConfig();
	void initBitmap();
	void updateStatus();

	static  SplashScreen *_pINSTANCE;

	static osl::Mutex _aMutex;
	Reference< XMultiServiceFactory > _rFactory;

    VirtualDevice   _vdev;
    Bitmap          _aIntroBmp;
    Color           _cProgressFrameColor;
    Color           _cProgressBarColor;
    OUString        _sExecutePath;

	sal_Int32 _iMax;
	sal_Int32 _iProgress;
	sal_Bool _bPaintBitmap;
	sal_Bool _bPaintProgress;
	sal_Bool _bVisible;
	long _height, _width, _tlx, _tly, _barwidth;
    long _barheight, _barspace;
    const long _xoffset, _yoffset;
#ifdef USE_JAVA
    ProgressBar* _pProgressBar;
#endif	// USE_JAVA

public:
    static const char* interfaces[];
	static const sal_Char *serviceName;
	static const sal_Char *implementationName;
	static const sal_Char *supportedServiceNames[];

    static Reference< XInterface > getInstance(const Reference < XMultiServiceFactory >& xFactory);

	// static service info
    static OUString  impl_getImplementationName();
	static Sequence<OUString> impl_getSupportedServiceNames();

	// XStatusIndicator
	virtual void SAL_CALL end() throw ( RuntimeException );
	virtual void SAL_CALL reset() throw ( RuntimeException );
	virtual void SAL_CALL setText(const OUString& aText) throw ( RuntimeException );
	virtual void SAL_CALL setValue(sal_Int32 nValue) throw ( RuntimeException );
	virtual void SAL_CALL start(const OUString& aText, sal_Int32 nRange) throw ( RuntimeException );

	// XInitialize
	virtual void SAL_CALL initialize( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any>& aArguments )
		throw ( RuntimeException );

	// workwindow
	virtual void Paint( const Rectangle& );

};

}
