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
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 by Patrick Luby (patrick.luby@planamesa.com)
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

#define _SV_SALGDI3_CXX

#ifndef _SV_SALGDI_HXX
#include <salgdi.hxx>
#endif
#ifndef _SV_SALDATA_HXX
#include <saldata.hxx>
#endif
#ifndef _SV_SALINST_HXX
#include <salinst.hxx>
#endif
#ifndef _SV_SALLAYOUT_HXX
#include <sallayout.hxx>
#endif
#ifndef _SV_OUTDEV_H
#include <outdev.h>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLFONT_HXX
#include <com/sun/star/vcl/VCLFont.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif

#include <premac.h>
#include <Carbon/Carbon.h>
#include <postmac.h>

struct SVFontStyles
{
	void*			mpNativeBoldFont;
	void*			mpNativeBoldItalicFont;
	void*			mpNativeItalicFont;
	void*			mpNativePlainFont;
};

static ATSFontNotificationRef aNotification = NULL;
static ::std::map< void*, SVFontStyles* > aNativeFontStylesMapping;

using namespace rtl;
using namespace vcl;
using namespace vos;

// ============================================================================

static void ImplFontListChangedCallback( ATSFontNotificationInfoRef aInfo, void *pData )
{
	if ( !Application::IsShutDown() )
	{
		// We need to let any pending timers run so that we don't deadlock
		IMutex& rSolarMutex = Application::GetSolarMutex();
		bool bAcquired = false;
		while ( !Application::IsShutDown() )
		{
			if ( rSolarMutex.tryToAcquire() )
			{
				bAcquired = true;
				break;
			}

			ReceiveNextEvent( 0, NULL, 0, false, NULL );
			OThread::yield();
		}

		if ( bAcquired )
		{
			if ( !Application::IsShutDown() )
			{
				::std::list< ATSFontRef > aATSFontList;
				BOOL bContinue = TRUE;
				while ( bContinue )
				{
					ATSFontIterator aIterator;
					ATSFontIteratorCreate( kATSFontContextLocal, NULL, NULL, kATSOptionFlagsUnRestrictedScope, &aIterator );
					for ( ; ; )
					{
						ATSFontRef aFont;
						OSStatus nErr = ATSFontIteratorNext( aIterator, &aFont );
						if ( nErr == kATSIterationCompleted )
						{
							bContinue = FALSE;
							break;
						}
						else if ( nErr == kATSIterationScopeModified )
						{
							aATSFontList.clear();
							break;
						}
						else if ( aFont )
						{
							aATSFontList.push_back( aFont );
						}
					}

					ATSFontIteratorRelease( &aIterator );
				}

				// Update cached fonts
				SalData *pSalData = GetSalData();
				for ( std::list< ATSFontRef >::const_iterator ait = aATSFontList.begin(); ait != aATSFontList.end(); ++ait )
				{
					CFStringRef aString;
					if ( ATSFontGetName( *ait, kATSOptionFlagsDefault, &aString ) != noErr )
						continue;

					CFIndex nLen = CFStringGetLength( aString );
					CFRange aRange = CFRangeMake( 0, nLen );
					sal_Unicode pBuffer[ nLen + 1 ];
					CFStringGetCharacters( aString, aRange, pBuffer );
					pBuffer[ nLen ] = 0;
					OUString aFontName( pBuffer );
					void *pNativeFont = (void *)FMGetFontFromATSFontRef( *ait );

					std::map< void*, ImplFontData* >::const_iterator it = pSalData->maNativeFontMapping.find( pNativeFont );
					if ( it == pSalData->maNativeFontMapping.end() )
					{
						ImplFontData *pData = new ImplFontData();
						pData->mpNext = NULL;
						pData->mpSysData = (void *)( new com_sun_star_vcl_VCLFont( aFontName, pNativeFont, 12, 0, sal_True, sal_False, 1.0 ) );
						pData->maName = XubString( aFontName );
						// [ed] 11/1/04 Scalable fonts should always report
						// their width and height as zero. The single size of
						// zero causes higher-level font elements to treat
						// fonts as infinitely scalable and provide lists of
						// default font sizes. The size of zero matches the
						// unx implementation. Bug 196.
						pData->mnWidth = 0;
						pData->mnHeight = 0;
						pData->meFamily = FAMILY_DONTKNOW;
						pData->meCharSet = RTL_TEXTENCODING_UNICODE;
						pData->mePitch = PITCH_VARIABLE;
						pData->meWidthType = WIDTH_DONTKNOW;
						pData->meWeight = WEIGHT_NORMAL;
						pData->meItalic = ITALIC_NONE;
						pData->meType = TYPE_SCALABLE;
						pData->mnVerticalOrientation = 0;
						pData->mbOrientation = TRUE;
						pData->mbDevice = FALSE;
						pData->mnQuality = 0;
						pData->mbSubsettable = TRUE;
						pData->mbEmbeddable = FALSE;

						pSalData->maNativeFontMapping[ pNativeFont ] = pData;

						std::map< void*, SVFontStyles* >::const_iterator sit = aNativeFontStylesMapping.find( pNativeFont );
						if ( sit == aNativeFontStylesMapping.end() )
						{
							SVFontStyles *pFontStyles = new SVFontStyles();
							memset( pFontStyles, 0, sizeof( SVFontStyles ) );
							aNativeFontStylesMapping[ pNativeFont ] = pFontStyles;
						}
					}
				}
			}

			rSolarMutex.release();
		}
	}
}

// =======================================================================

void SalGraphics::SetTextColor( SalColor nSalColor )
{
	maGraphicsData.mnTextColor = nSalColor | 0xff000000;
}

// -----------------------------------------------------------------------

USHORT SalGraphics::SetFont( ImplFontSelectData* pFont, int nFallbackLevel )
{
	SalData *pSalData = GetSalData();

	// Don't change the font for fallback levels as we need the first font
	// to properly determine the fallback font
	if ( !nFallbackLevel )
	{
		sal_Bool bBold = ( pFont->meWeight > pFont->mpFontData->meWeight );
		sal_Bool bItalic = ( ( pFont->meItalic == ITALIC_OBLIQUE || pFont->meItalic == ITALIC_NORMAL ) && pFont->meItalic != pFont->mpFontData->meItalic );

		// Handle remapping to and from bold and italic fonts
		if ( bBold || bItalic || pFont->mpFontData->maName != pFont->maFoundName )
		{
			void *pNativeFont = ((com_sun_star_vcl_VCLFont *)pFont->mpFontData->mpSysData)->getNativeFont();
			::std::map< void*, SVFontStyles* >::const_iterator sit = aNativeFontStylesMapping.find( pNativeFont );
			if ( sit != pSalData->maNativeFontMapping.end() )
			{
				void *pReplacementFont;
				if ( bBold && bItalic )
					pReplacementFont = sit->second->mpNativeBoldItalicFont;
				else if ( bBold )
					pReplacementFont = sit->second->mpNativeBoldFont;
				else if ( bItalic )
					pReplacementFont = sit->second->mpNativeItalicFont;
				else
					pReplacementFont = sit->second->mpNativePlainFont;

				if ( pReplacementFont )
				{
					::std::map< void*, ImplFontData* >::const_iterator it = pSalData->maNativeFontMapping.find( pReplacementFont );
					if ( it != pSalData->maNativeFontMapping.end() )
						pFont->mpFontData = it->second;
				}
			}
		}
	}
	else
	{
		// Retrieve the fallback font if one has been set by a text layout
		::std::map< int, com_sun_star_vcl_VCLFont* >::const_iterator ffit = maGraphicsData.maFallbackFonts.find( nFallbackLevel );
		if ( ffit != maGraphicsData.maFallbackFonts.end() )
		{
			void *pNativeFont = ffit->second->getNativeFont();
			::std::map< void*, ImplFontData* >::const_iterator it = pSalData->maNativeFontMapping.find( pNativeFont );
			if ( it != pSalData->maNativeFontMapping.end() )
				pFont->mpFontData = it->second;
		}
	}

	pFont->maFoundName = pFont->mpFontData->maName;

	if ( !nFallbackLevel )
	{
		// Set font for graphics device
		if ( maGraphicsData.mpVCLFont )
			delete maGraphicsData.mpVCLFont;
		maGraphicsData.mpVCLFont = ((com_sun_star_vcl_VCLFont *)pFont->mpFontData->mpSysData)->deriveFont( pFont->mnHeight, pFont->mnOrientation, !pFont->mbNonAntialiased, pFont->mbVertical, pFont->mnWidth ? (double)pFont->mnWidth / (double)pFont->mnHeight : 1.0 );
	}

	return 0;
}

// -----------------------------------------------------------------------

void SalGraphics::GetFontMetric( ImplFontMetricData* pMetric )
{
	ImplFontData *pData = NULL;
	if ( maGraphicsData.mpVCLFont )
	{
		SalData *pSalData = GetSalData();

		void *pNativeFont = maGraphicsData.mpVCLFont->getNativeFont();
		::std::map< void*, ImplFontData* >::const_iterator it = pSalData->maNativeFontMapping.find( pNativeFont );
		if ( it != pSalData->maNativeFontMapping.end() )
			pData = it->second;
	}

	if ( maGraphicsData.mpVCLFont )
	{
		pMetric->mnAscent = maGraphicsData.mpVCLFont->getAscent();
		pMetric->mnDescent = maGraphicsData.mpVCLFont->getDescent();
		pMetric->mnLeading = maGraphicsData.mpVCLFont->getLeading();
		pMetric->mnOrientation = maGraphicsData.mpVCLFont->getOrientation();
		pMetric->mnWidth = maGraphicsData.mpVCLFont->getSize();
	}
	else
	{
		pMetric->mnAscent = 0;
		pMetric->mnDescent = 0;
		pMetric->mnLeading = 0;
		pMetric->mnOrientation = 0;
		pMetric->mnWidth = 0;
	}

	if ( pData )
	{
		pMetric->meCharSet = pData->meCharSet;
		pMetric->meFamily = pData->meFamily;
		pMetric->meItalic = pData->meItalic;
		pMetric->maName = pData->maName;
		pMetric->mePitch = pData->mePitch;
		pMetric->meWeight = pData->meWeight;
	}
	else
	{
		pMetric->meCharSet = RTL_TEXTENCODING_UNICODE;
		pMetric->meFamily = FAMILY_DONTKNOW;
		pMetric->meItalic = ITALIC_NONE;
		pMetric->maName = OUString();
		pMetric->mePitch = PITCH_VARIABLE;
		pMetric->meWeight = WEIGHT_NORMAL;
	}

	pMetric->mbDevice = FALSE;
	pMetric->mnFirstChar = 0;
	pMetric->mnLastChar = 255;
	pMetric->mnSlant = 0;
	pMetric->meType = TYPE_SCALABLE;
}

// -----------------------------------------------------------------------

ULONG SalGraphics::GetKernPairs( ULONG nPairs, ImplKernPairData* pKernPairs )
{
	if ( !maGraphicsData.mpVCLFont )
		return 0;
	
	ImplKernPairData *pPair = pKernPairs;
	for ( ULONG i = 0; i < nPairs; i++ )
	{
		pPair->mnKern = maGraphicsData.mpVCLFont->getKerning( pPair->mnChar1, pPair->mnChar2 );
		pPair++;
	}

	return nPairs;
}

// -----------------------------------------------------------------------

ULONG SalGraphics::GetFontCodeRanges( sal_uInt32* pCodePairs ) const
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::GetFontCodeRanges not implemented\n" );
#endif
	return 0;
}

// -----------------------------------------------------------------------

void SalGraphics::GetDevFontList( ImplDevFontList* pList )
{
	SalData *pSalData = GetSalData();

	if ( !aNotification )
	{
		if ( ATSFontNotificationSubscribe( ImplFontListChangedCallback, kATSFontNotifyOptionReceiveWhileSuspended, NULL, &aNotification ) == noErr )
			ImplFontListChangedCallback( NULL, NULL );
	}

	// Iterate through fonts and add each to the font list
	for ( ::std::map< void*, ImplFontData* >::const_iterator it = pSalData->maNativeFontMapping.begin(); it != pSalData->maNativeFontMapping.end(); ++it )
	{
		// Set default values
		com_sun_star_vcl_VCLFont *pVCLFont = (com_sun_star_vcl_VCLFont *)it->second->mpSysData;
		if ( !pVCLFont )
			continue;

		ImplFontData *pFontData = new ImplFontData();
		pFontData->mpNext = it->second->mpNext;
		pFontData->mpSysData = (void *)( new com_sun_star_vcl_VCLFont( pVCLFont->getJavaObject() ) );
		pFontData->maName = it->second->maName;
		pFontData->mnWidth = it->second->mnWidth;
		pFontData->mnHeight = it->second->mnHeight;
		pFontData->meFamily = it->second->meFamily;
		pFontData->meCharSet = it->second->meCharSet;
		pFontData->mePitch = it->second->mePitch;
		pFontData->meWidthType = it->second->meWidthType;
		pFontData->meWeight = it->second->meWeight;
		pFontData->meItalic = it->second->meItalic;
		pFontData->meType = it->second->meType;
		pFontData->mnVerticalOrientation = it->second->mnVerticalOrientation;
		pFontData->mbOrientation = it->second->mbOrientation;
		pFontData->mbDevice = it->second->mbDevice;
		pFontData->mnQuality = it->second->mnQuality;
		pFontData->mbSubsettable = it->second->mbSubsettable;
		pFontData->mbEmbeddable = it->second->mbEmbeddable;

		// Add to list
		pList->Add( pFontData );
	}
}

// -----------------------------------------------------------------------

BOOL SalGraphics::GetGlyphBoundRect( long nIndex, Rectangle& rRect,
                                     const OutputDevice *pOutDev )
{
	if ( maGraphicsData.mpVCLFont )
	{
		rRect = maGraphicsData.mpVCLGraphics->getGlyphBounds( nIndex & GF_IDXMASK, maGraphicsData.mpVCLFont, nIndex & GF_ROTMASK );
		return TRUE;
	}
	else
	{
		rRect.SetEmpty();
		return FALSE;
	}
}

// -----------------------------------------------------------------------

BOOL SalGraphics::GetGlyphOutline( long nIndex, PolyPolygon& rPolyPoly,
                                   const OutputDevice *pOutDev )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::GetGlyphOutline not implemented\n" );
#endif
	rPolyPoly.Clear();
	return FALSE;
}

// -----------------------------------------------------------------------

void SalGraphics::GetDevFontSubstList( OutputDevice* pOutDev )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::GetDevFontSubstList not implemented\n" );
#endif
}

// -----------------------------------------------------------------------

ImplFontData* SalGraphics::AddTempDevFont( const String& rFontFileURL,
                                           const String& rFontName )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::AddTempDevFont not implemented\n" );
#endif
	return NULL;
}

// -----------------------------------------------------------------------

void SalGraphics::RemovingFont( ImplFontData* )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::RemovingFont not implemented\n" );
#endif
}

// -----------------------------------------------------------------------

BOOL SalGraphics::CreateFontSubset( const rtl::OUString& rToFile,
                                    ImplFontData* pFont, long* pGlyphIDs,
                                    sal_uInt8* pEncoding, sal_Int32* pWidths,
                                    int nGlyphs, FontSubsetInfo& rInfo )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::CreateFontSubset not implemented\n" );
#endif
	return FALSE;
}

// -----------------------------------------------------------------------

const void* SalGraphics::GetEmbedFontData( ImplFontData* pFont,
                                           const sal_Unicode* pUnicodes,
                                           sal_Int32* pWidths,
                                           FontSubsetInfo& rInfo,
                                           long* pDataLen )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::GetEmbedFontData not implemented\n" );
#endif
	return NULL;
}

// -----------------------------------------------------------------------

void SalGraphics::FreeEmbedFontData( const void* pData, long nLen )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::FreeEmbedFontData not implemented\n" );
#endif
}

// -----------------------------------------------------------------------

const std::map< sal_Unicode, sal_Int32 >* SalGraphics::GetFontEncodingVector(
                ImplFontData* pFont,
                const std::map< sal_Unicode, rtl::OString >** pNonEncoded )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::GetFontEncodingVector not implemented\n" );
#endif
	if ( pNonEncoded )
		*pNonEncoded = NULL;
	return NULL;
}
