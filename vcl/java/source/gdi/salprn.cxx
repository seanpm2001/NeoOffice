/*************************************************************************
 *
 * $RCSfile$
 *
 * $Revision$
 *
 * last change: $Author$ $Date$
 *
 * The Contents of this file are made available subject to the terms of
 * either of the following licenses
 *
 *        - GNU General Public License Version 2.1
 *
 * Patrick Luby, June 2003
 *
 * GNU General Public License Version 2.1
 * =============================================
 * Copyright 2003 by Patrick Luby (patrick.luby@planamesa.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2.1, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 ************************************************************************/

#define _SV_SALPRN_CXX

#include <stdio.h>

#ifndef _SV_SALPRN_HXX
#include <salprn.hxx>
#endif
#ifndef _SV_SALPTYPE_HXX
#include <salptype.hxx>
#endif
#ifndef _SV_JOBSET_H
#include <jobset.h>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLPAGEFORMAT_HXX
#include <com/sun/star/vcl/VCLPageFormat.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLPRINTJOB_HXX
#include <com/sun/star/vcl/VCLPrintJob.hxx>
#endif

#ifdef MACOSX

#ifndef _SV_JAVA_LANG_CLASS_HXX
#include <java/lang/Class.hxx>
#endif
#ifndef _VOS_MODULE_HXX_
#include <vos/module.hxx>
#endif

#include <premac.h>
#include <Carbon/Carbon.h>
typedef OSStatus PMCreatePageFormat_Type( PMPageFormat* );
typedef OSStatus PMGetResolution_Type( PMPageFormat, PMResolution* );
typedef OSStatus PMSessionDefaultPageFormat_Type( PMPrintSession, PMPageFormat );
#include <postmac.h>

using namespace rtl;
using namespace vos;

#endif	// MACOSX

using namespace vcl;

// =======================================================================

SalInfoPrinter::SalInfoPrinter()
{
}

// -----------------------------------------------------------------------

SalInfoPrinter::~SalInfoPrinter()
{
}

// -----------------------------------------------------------------------

SalGraphics* SalInfoPrinter::GetGraphics()
{
	if ( maPrinterData.mbGraphics )
		return NULL;

	maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics = maPrinterData.mpVCLPageFormat->getGraphics();
	maPrinterData.mbGraphics = TRUE;

	return maPrinterData.mpGraphics;
}

// -----------------------------------------------------------------------

void SalInfoPrinter::ReleaseGraphics( SalGraphics* pGraphics )
{
	if ( pGraphics != maPrinterData.mpGraphics )
		return;

	if ( maPrinterData.mpGraphics && maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics )
		delete maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics;
	maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics = NULL;
	maPrinterData.mbGraphics = FALSE;
}

// -----------------------------------------------------------------------

BOOL SalInfoPrinter::Setup( SalFrame* pFrame, ImplJobSetup* pSetupData )
{
	// Display a native page setup dialog
	Orientation nOrientation = maPrinterData.mpVCLPageFormat->getOrientation();
	maPrinterData.mpVCLPageFormat->setOrientation( pSetupData->meOrientation );
	if ( !maPrinterData.mpVCLPageFormat->setup() )
	{
		maPrinterData.mpVCLPageFormat->setOrientation( nOrientation );
		return FALSE;
	}

	// Update values
	SetData( 0, pSetupData );

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL SalInfoPrinter::SetPrinterData( ImplJobSetup* pSetupData )
{
	// Set but don't update values
	ImplJobSetup aSetupData( *pSetupData );
	SetData( SAL_JOBSET_ALL, &aSetupData );

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL SalInfoPrinter::SetData( ULONG nFlags, ImplJobSetup* pSetupData )
{
	// Set and update values
	if ( nFlags & SAL_JOBSET_ORIENTATION )
		maPrinterData.mpVCLPageFormat->setOrientation( pSetupData->meOrientation );

	// Populate the job setup
	pSetupData->meOrientation = maPrinterData.mpVCLPageFormat->getOrientation();
	pSetupData->mnPaperBin = 0;
	pSetupData->mePaperFormat = PAPER_USER;
	Size aSize( maPrinterData.mpVCLPageFormat->getPageSize() );
	pSetupData->mnPaperWidth = aSize.Width();
	pSetupData->mnPaperHeight = aSize.Height();
	if ( pSetupData->mpDriverData )
		((SalDriverData *)pSetupData->mpDriverData)->meOrientation = pSetupData->meOrientation;

	return TRUE;
}

// -----------------------------------------------------------------------

ULONG SalInfoPrinter::GetPaperBinCount( const ImplJobSetup* pSetupData )
{
	// Return a dummy value
	return 1;
}

// -----------------------------------------------------------------------

XubString SalInfoPrinter::GetPaperBinName( const ImplJobSetup* pSetupData, ULONG nPaperBin )
{
	// Return a dummy value
	return XubString();
}

// -----------------------------------------------------------------------

ULONG SalInfoPrinter::GetCapabilities( const ImplJobSetup* pSetupData, USHORT nType )
{
	if ( nType == PRINTER_CAPABILITIES_SETORIENTATION )
		return 1;
	else
		return 0;
}

// -----------------------------------------------------------------------

void SalInfoPrinter::GetPageInfo( const ImplJobSetup* pSetupData,
								  long& rOutWidth, long& rOutHeight,
								  long& rPageOffX, long& rPageOffY,
								  long& rPageWidth, long& rPageHeight )
{
	Size aSize( maPrinterData.mpVCLPageFormat->getPageSize() );
	Rectangle aRect( maPrinterData.mpVCLPageFormat->getImageableBounds() );
	rPageWidth = aSize.Width();
	rPageHeight = aSize.Height();
	rPageOffX = aRect.nLeft;
	rPageOffY = aRect.nTop;
	rOutWidth = aRect.nRight - aRect.nLeft + 1;
	rOutHeight = aRect.nBottom - aRect.nTop + 1;
}

// =======================================================================

SalPrinter::SalPrinter()
{
	maPrinterData.mpGraphics->maGraphicsData.mpPrinter = this;
}

// -----------------------------------------------------------------------

SalPrinter::~SalPrinter()
{
}

// -----------------------------------------------------------------------

BOOL SalPrinter::StartJob( const XubString* pFileName,
						   const XubString& rJobName,
						   const XubString&,
						   ULONG nCopies, BOOL bCollate,
						   ImplJobSetup* pSetupData )
{
	// Set but don't update values
	maPrinterData.mpPrinter->SetPrinterData( pSetupData );

	maPrinterData.mbStarted = maPrinterData.mpVCLPrintJob->startJob( maPrinterData.mpPrinter->maPrinterData.mpVCLPageFormat );

	if ( maPrinterData.mbStarted )
	{
#ifdef MACOSX
	// Test the JVM version and if it is below 1.4, use Carbon printing APIs
	java_lang_Class* pClass = java_lang_Class::forName( OUString::createFromAscii( "java/lang/CharSequence" ) );
	if ( !pClass )
	{
		PMPrintSession pSession = (PMPrintSession)maPrinterData.mpVCLPrintJob->getNativePrintJob();
		if ( pSession )
		{
			OModule aModule;
			if ( aModule.load( OUString::createFromAscii( "/System/Library/Frameworks/Carbon.framework/Carbon" ) ) )
			{
				PMCreatePageFormat_Type *pCreatePageFormat = (PMCreatePageFormat_Type *)aModule.getSymbol( OUString::createFromAscii( "PMCreatePageFormat" ) );
				PMGetResolution_Type *pGetResolution = (PMGetResolution_Type *)aModule.getSymbol( OUString::createFromAscii( "PMGetResolution" ) );
				PMSessionDefaultPageFormat_Type *pSessionDefaultPageFormat = (PMSessionDefaultPageFormat_Type *)aModule.getSymbol( OUString::createFromAscii( "PMSessionDefaultPageFormat" ) );
				if ( pCreatePageFormat && pGetResolution && pSessionDefaultPageFormat )
				{
					PMPageFormat aPageFormat;
					if ( pCreatePageFormat( &aPageFormat ) == kPMNoError )
					{
						PMResolution aResolution;
						if ( pSessionDefaultPageFormat( pSession, aPageFormat ) == kPMNoError && pGetResolution( aPageFormat, &aResolution ) == kPMNoError )
							maPrinterData.mpPrinter->maPrinterData.mpVCLPageFormat->setPageResolution( (long)aResolution.hRes, (long)aResolution.vRes );
					}
				}
				aModule.unload();
			}
		}
	}
#endif	// MACOSX
	}

	return maPrinterData.mbStarted;
}

// -----------------------------------------------------------------------

BOOL SalPrinter::EndJob()
{
	maPrinterData.mpVCLPrintJob->endJob();
	return TRUE;
}

// -----------------------------------------------------------------------

BOOL SalPrinter::AbortJob()
{
	maPrinterData.mpVCLPrintJob->abortJob();
	return TRUE;
}

// -----------------------------------------------------------------------

SalGraphics* SalPrinter::StartPage( ImplJobSetup* pSetupData, BOOL bNewJobData )
{
	if ( maPrinterData.mbGraphics )
		return NULL;

	maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics = maPrinterData.mpVCLPrintJob->startPage();
	if ( !maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics )
		return NULL;
	maPrinterData.mbGraphics = TRUE;

	return maPrinterData.mpGraphics;
}

// -----------------------------------------------------------------------

BOOL SalPrinter::EndPage()
{
	maPrinterData.mpVCLPrintJob->endPage();
	if ( maPrinterData.mpGraphics && maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics )
		delete maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics;
	maPrinterData.mpGraphics->maGraphicsData.mpVCLGraphics = NULL;
	maPrinterData.mbGraphics = FALSE;
	return TRUE;
}

// -----------------------------------------------------------------------

ULONG SalPrinter::GetErrorCode()
{
	if ( !maPrinterData.mbStarted || maPrinterData.mpVCLPrintJob->isFinished() )
		return SAL_PRINTER_ERROR_ABORT;
	else
		return 0;
}

// =======================================================================

SalPrinterData::SalPrinterData()
{
	mpPrinter = NULL;
	mbStarted = FALSE;
	mpGraphics = new SalGraphics();
	mbGraphics = FALSE;
	mpVCLPrintJob = new com_sun_star_vcl_VCLPrintJob();
}

// -----------------------------------------------------------------------

SalPrinterData::~SalPrinterData()
{
	if ( mpGraphics )
		delete mpGraphics;

	if ( mpVCLPrintJob )
	{
		mpVCLPrintJob->dispose();
		delete mpVCLPrintJob;
	}
}

// =======================================================================

SalInfoPrinterData::SalInfoPrinterData()
{
	mpGraphics = new SalGraphics();
	mbGraphics = FALSE;
	mpVCLPageFormat = NULL;
}

// -----------------------------------------------------------------------

SalInfoPrinterData::~SalInfoPrinterData()
{
	if ( mpGraphics )
		delete mpGraphics;
	if ( mpVCLPageFormat )
		delete mpVCLPageFormat;
}
