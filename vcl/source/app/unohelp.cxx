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
 *  Sun Microsystems Inc., October, 2000
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2000 by Sun Microsystems, Inc.
 *  901 San Antonio Road, Palo Alto, CA 94303, USA
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
 *  =================================================
 *  Modified June 2004 by Patrick Luby. SISSL Removed. NeoOffice is
 *  distributed under GPL only under modification term 3 of the LGPL.
 *
 *  Contributor(s): _______________________________________
 *
 ************************************************************************/

#pragma hdrstop

#include <unohelp.hxx>

#ifndef _COM_SUN_STAR_LANG_XSINGLESERVICEFACTORY_HPP_
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#endif

#ifndef _COM_SUN_STAR_LANG_XMULTISERVICEFACTORY_HPP_
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#endif

#ifndef _COMPHELPER_PROCESSFACTORY_HXX_
#include <comphelper/processfactory.hxx>
#endif

#ifndef _COM_SUN_STAR_TEXT_XBREAKITERATOR_HPP_
#include <com/sun/star/i18n/XBreakIterator.hpp>
#endif

#ifndef _COM_SUN_STAR_I18N_XCHARACTERCLASSIFICATION_HPP_
#include <com/sun/star/i18n/XCharacterClassification.hpp>
#endif

#ifndef _COM_SUN_STAR_UTIL_XCOLLATOR_HPP_
#include <com/sun/star/i18n/XCollator.hpp>
#endif

#ifndef _COM_SUN_STAR_AWT_XEXTENDEDTOOLKIT_HPP_
#include <com/sun/star/awt/XExtendedToolkit.hpp>
#endif

#ifndef _COM_SUN_STAR_ACCESSIBILITY_ACCESSIBLEEVENTOBJECT_HPP_
#include <com/sun/star/accessibility/AccessibleEventObject.hpp>
#endif

#ifndef _COM_SUN_STAR_ACCESSIBILITY_ACCESSIBLESTATETYPE_HPP_
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#endif


#include <com/sun/star/registry/XImplementationRegistration.hpp>

#include <cppuhelper/servicefactory.hxx>

#include <tools/tempfile.hxx>
#include <osl/file.hxx>

#include <svdata.hxx>
#include <svapp.hxx>

using namespace ::com::sun::star;
using namespace ::rtl;

#define DOSTRING( x )			   			#x
#define STRING( x )				   			DOSTRING( x )
#define UNOSUFFIX							.uno

struct VCLRegServiceInfo
{
	sal_Char*	pLibName;
	sal_Bool	bHasSUPD;
};

static VCLRegServiceInfo aVCLComponentsArray[] =
{
	{"i18n", sal_True},
    {"i18npool", sal_True},
#ifdef UNX
#ifdef USE_JAVA
	{"dtransjava", sal_True},
#else
#if defined MACOSX && defined QUARTZ
	{"dtransaqua", sal_True},
#else
	{"dtransX11", sal_True},
#endif
#endif
#endif
#ifdef WNT
	{"sysdtrans", sal_False},
#endif
	{"mcnttype", sal_False},
#ifdef USE_JAVA
	{"javavm" STRING(UNOSUFFIX), sal_False},
	{"cfgmgr2", sal_False},
	{"typeconverter" STRING(UNOSUFFIX), sal_False},
	{"sax" STRING(UNOSUFFIX), sal_False},
#endif
	{NULL, sal_False}
};

uno::Reference< lang::XMultiServiceFactory > vcl::unohelper::GetMultiServiceFactory()
{
	ImplSVData* pSVData = ImplGetSVData();
	if ( !pSVData->maAppData.mxMSF.is() )
	{
		pSVData->maAppData.mxMSF = ::comphelper::getProcessServiceFactory();
	}
	if ( !pSVData->maAppData.mxMSF.is() )
	{
		TempFile aTempFile;
		OUString aTempFileName;
		osl::FileBase::getSystemPathFromFileURL( aTempFile.GetName(), aTempFileName );
		pSVData->maAppData.mpMSFTempFileName = new String(aTempFileName);

		pSVData->maAppData.mxMSF = ::cppu::createRegistryServiceFactory( aTempFileName, rtl::OUString(), sal_False );


		uno::Reference < registry::XImplementationRegistration > xReg(
			pSVData->maAppData.mxMSF->createInstance( OUString::createFromAscii( "com.sun.star.registry.ImplementationRegistration" )), uno::UNO_QUERY );

		sal_Int32 nCompCount = 0;

		while ( aVCLComponentsArray[ nCompCount ].pLibName )
		{
			OUString aComponentPathString = CreateLibraryName( aVCLComponentsArray[ nCompCount ].pLibName,  aVCLComponentsArray[ nCompCount ].bHasSUPD );
			if (aComponentPathString.getLength() )
			{
				try
				{
					xReg->registerImplementation(
						OUString::createFromAscii( "com.sun.star.loader.SharedLibrary" ),aComponentPathString, NULL );
				}
				catch( ::com::sun::star::uno::Exception & )
				{
				}
			}
			nCompCount++;
		}
		::comphelper::setProcessServiceFactory( pSVData->maAppData.mxMSF );
	}
	return pSVData->maAppData.mxMSF;
}


uno::Reference < i18n::XBreakIterator > vcl::unohelper::CreateBreakIterator()
{
	uno::Reference < i18n::XBreakIterator > xB;
	uno::Reference< lang::XMultiServiceFactory > xMSF = GetMultiServiceFactory();
	if ( xMSF.is() )
	{
		uno::Reference < uno::XInterface > xI = xMSF->createInstance( ::rtl::OUString::createFromAscii( "com.sun.star.i18n.BreakIterator" ) );
		if ( xI.is() )
		{
			uno::Any x = xI->queryInterface( ::getCppuType((const uno::Reference< i18n::XBreakIterator >*)0) );
			x >>= xB;
		}
	}
	return xB;
}

uno::Reference < i18n::XCharacterClassification > vcl::unohelper::CreateCharacterClassification()
{
	uno::Reference < i18n::XCharacterClassification > xB;
	uno::Reference< lang::XMultiServiceFactory > xMSF = GetMultiServiceFactory();
	if ( xMSF.is() )
	{
		uno::Reference < uno::XInterface > xI = xMSF->createInstance( ::rtl::OUString::createFromAscii( "com.sun.star.i18n.CharacterClassification" ) );
		if ( xI.is() )
		{
			uno::Any x = xI->queryInterface( ::getCppuType((const uno::Reference< i18n::XCharacterClassification >*)0) );
			x >>= xB;
		}
	}
	return xB;
}

uno::Reference < i18n::XCollator > vcl::unohelper::CreateCollator()
{
	uno::Reference < i18n::XCollator > xB;
	uno::Reference< lang::XMultiServiceFactory > xMSF = GetMultiServiceFactory();
	if ( xMSF.is() )
	{
		uno::Reference < uno::XInterface > xI = xMSF->createInstance( ::rtl::OUString::createFromAscii( "com.sun.star.i18n.Collator" ) );
		if ( xI.is() )
		{
			uno::Any x = xI->queryInterface( ::getCppuType((const uno::Reference< i18n::XCollator >*)0) );
			x >>= xB;
		}
	}
	return xB;
}

::rtl::OUString vcl::unohelper::CreateLibraryName( const sal_Char* pModName, BOOL bSUPD )
{
	// create variable library name suffixes
	OUString aSUPDString( OUString::valueOf( (sal_Int32)SUPD, 10 ));
	OUString aDLLSuffix = OUString::createFromAscii( STRING(DLLSUFFIX) );
	OUString aUNOString = OUString::createFromAscii( STRING(UNOSUFFIX) );

	OUString aLibName = OUString::createFromAscii( pModName );

	sal_Int32 nIndex = aLibName.getLength() - aUNOString.getLength();
	BOOL bUNO = ( nIndex > 0 && aLibName.lastIndexOf( aUNOString ) == nIndex );

#ifdef WNT
	if ( !bUNO && bSUPD )
	{
		aLibName += aSUPDString;
		aLibName += aDLLSuffix;
	}
	aLibName += rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".dll" ));
#else
	if ( !bUNO )
	{
		aLibName = OUString( RTL_CONSTASCII_USTRINGPARAM( "lib" )) + aLibName;
		if ( bSUPD )
		{
			aLibName += aSUPDString;
			aLibName += aDLLSuffix;
		}
	}
#ifdef MACOSX
	aLibName += OUString( RTL_CONSTASCII_USTRINGPARAM( ".dylib" ));
#else
	aLibName += OUString( RTL_CONSTASCII_USTRINGPARAM( ".so" ));
#endif
#endif

	return aLibName;
}

void vcl::unohelper::NotifyAccessibleStateEventGlobally( const ::com::sun::star::accessibility::AccessibleEventObject& rEventObject )
{
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XExtendedToolkit > xExtToolkit( Application::GetVCLToolkit(), uno::UNO_QUERY );
    if ( xExtToolkit.is() )
    {
        // Only for focus events
        sal_Int16 nType;
        rEventObject.NewValue >>= nType;
        if ( nType == ::com::sun::star::accessibility::AccessibleStateType::FOCUSED )
            xExtToolkit->fireFocusGained( rEventObject.Source );
        else
        {
            rEventObject.OldValue >>= nType;
            if ( nType == ::com::sun::star::accessibility::AccessibleStateType::FOCUSED )
                xExtToolkit->fireFocusLost( rEventObject.Source );
        }
        
    }
}
