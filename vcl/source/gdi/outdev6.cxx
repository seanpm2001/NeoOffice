/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified February 2006 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_vcl.hxx"

#ifndef _SV_SVSYS_HXX
#include <svsys.h>
#endif
#include <vcl/salgdi.hxx>
#include <tools/debug.hxx>
#include <vcl/outdev.h>
#include <vcl/outdev.hxx>
#include <vcl/virdev.hxx>
#include <vcl/bmpacc.hxx>
#include <vcl/metaact.hxx>
#include <vcl/gdimtf.hxx>
#include <vcl/svapp.hxx>
#include <vcl/wrkwin.hxx>
#include <vcl/graph.hxx>
#include <vcl/wall2.hxx>
#include <com/sun/star/uno/Sequence.hxx>

#include <basegfx/vector/b2dvector.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <math.h>
#include <vcl/window.h>
#include <vcl/svdata.hxx>

#ifdef USE_JAVA

#ifndef _SV_SALGDI_H
#include <salgdi.h>
#endif

#define MAX_TRANSPARENT_GRADIENT_BMP_PIXELS ( 2 * 1024 * 1024 )
#define MIN_TRANSPARENT_GRADIENT_RESOLUTION 72

#endif	// USE_JAVA

// ========================================================================

DBG_NAMEEX( OutputDevice )

// ------------------------------------------------------------------------

void OutputDevice::DrawGrid( const Rectangle& rRect, const Size& rDist, ULONG nFlags )
{
	DBG_TRACE( "OutputDevice::DrawGrid()" );
	DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

	Rectangle aDstRect( PixelToLogic( Point() ), GetOutputSize() );
	aDstRect.Intersection( rRect );

	if( aDstRect.IsEmpty() || ImplIsRecordLayout() )
		return;

	if( !mpGraphics && !ImplGetGraphics() )
		return;

	if( mbInitClipRegion )
		ImplInitClipRegion();

	if( mbOutputClipped )
		return;

	const long	nDistX = Max( rDist.Width(), 1L );
	const long	nDistY = Max( rDist.Height(), 1L );
	long		nX = ( rRect.Left() >= aDstRect.Left() ) ? rRect.Left() : ( rRect.Left() + ( ( aDstRect.Left() - rRect.Left() ) / nDistX ) * nDistX );
	long		nY = ( rRect.Top() >= aDstRect.Top() ) ? rRect.Top() : ( rRect.Top() + ( ( aDstRect.Top() - rRect.Top() ) / nDistY ) * nDistY );
	const long	nRight = aDstRect.Right();
	const long	nBottom = aDstRect.Bottom();
	const long	nStartX = ImplLogicXToDevicePixel( nX );
	const long	nEndX = ImplLogicXToDevicePixel( nRight );
	const long	nStartY = ImplLogicYToDevicePixel( nY );
	const long	nEndY = ImplLogicYToDevicePixel( nBottom );
	long		nHorzCount = 0L;
	long		nVertCount = 0L;

	::com::sun::star::uno::Sequence< sal_Int32 > aVertBuf;
	::com::sun::star::uno::Sequence< sal_Int32 > aHorzBuf;

	if( ( nFlags & GRID_DOTS ) || ( nFlags & GRID_HORZLINES ) )
	{
		aVertBuf.realloc( aDstRect.GetHeight() / nDistY + 2L );
		aVertBuf[ nVertCount++ ] = nStartY;
		while( ( nY += nDistY ) <= nBottom )
			aVertBuf[ nVertCount++ ] = ImplLogicYToDevicePixel( nY );
	}

	if( ( nFlags & GRID_DOTS ) || ( nFlags & GRID_VERTLINES ) )
	{
		aHorzBuf.realloc( aDstRect.GetWidth() / nDistX + 2L );
		aHorzBuf[ nHorzCount++ ] = nStartX;
		while( ( nX += nDistX ) <= nRight )
			aHorzBuf[ nHorzCount++ ] = ImplLogicXToDevicePixel( nX );
	}

	if( mbInitLineColor )
		ImplInitLineColor();

	if( mbInitFillColor )
		ImplInitFillColor();

	const BOOL bOldMap = mbMap;
	EnableMapMode( FALSE );

	if( nFlags & GRID_DOTS )
	{
		for( long i = 0L; i < nVertCount; i++ )
			for( long j = 0L, Y = aVertBuf[ i ]; j < nHorzCount; j++ )
				mpGraphics->DrawPixel( aHorzBuf[ j ], Y, this );
	}
	else
	{
		if( nFlags & GRID_HORZLINES )
		{
			for( long i = 0L; i < nVertCount; i++ )
			{
				nY = aVertBuf[ i ];
				mpGraphics->DrawLine( nStartX, nY, nEndX, nY, this );
			}
		}

		if( nFlags & GRID_VERTLINES )
		{
			for( long i = 0L; i < nHorzCount; i++ )
			{
				nX = aHorzBuf[ i ];
				mpGraphics->DrawLine( nX, nStartY, nX, nEndY, this );
			}
		}
	}

	EnableMapMode( bOldMap );

    if( mpAlphaVDev )
        mpAlphaVDev->DrawGrid( rRect, rDist, nFlags );
}

// ------------------------------------------------------------------------
// Caution: This method is nearly the same as
// void OutputDevice::DrawPolyPolygon( const basegfx::B2DPolyPolygon& rB2DPolyPoly )
// so when changes are made here do not forget to make change sthere, too

void OutputDevice::DrawTransparent( const basegfx::B2DPolyPolygon& rB2DPolyPoly, double fTransparency)
{
	DBG_TRACE( "OutputDevice::DrawTransparent(B2D&,transparency)" );
	DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    // AW: Do NOT paint empty PolyPolygons
    if(!rB2DPolyPoly.count())
        return;

    // we need a graphics
    if( !mpGraphics )
	    if( !ImplGetGraphics() )
		    return;

    if( mbInitClipRegion )
	    ImplInitClipRegion();
    if( mbOutputClipped )
	    return;

    if( mbInitLineColor )
	    ImplInitLineColor();
    if( mbInitFillColor )
	    ImplInitFillColor();

	if((mnAntialiasing & ANTIALIASING_ENABLE_B2DDRAW) && mpGraphics->supportsOperation(OutDevSupport_B2DDraw))
    {
        // b2dpolygon support not implemented yet on non-UNX platforms
        const ::basegfx::B2DHomMatrix aTransform = ImplGetDeviceTransformation();
        ::basegfx::B2DPolyPolygon aB2DPP = rB2DPolyPoly;
        aB2DPP.transform( aTransform );
        
        if( mpGraphics->DrawPolyPolygon( aB2DPP, fTransparency, this ) )
        {
#if 0 
            // MetaB2DPolyPolygonAction is not implemented yet:
            // according to AW adding it is very dangerous since there is a lot
            // of code that uses the metafile actions directly and unless every
            // place that does this knows about the new action we need to fallback
            if( mpMetaFile )
                mpMetaFile->AddAction( new MetaB2DPolyPolygonAction( rB2DPolyPoly ) );
#else
            if( mpMetaFile )
    	        mpMetaFile->AddAction( new MetaTransparentAction( PolyPolygon( rB2DPolyPoly ), static_cast< sal_uInt16 >(fTransparency * 100.0)));
#endif
            return;
        }
    }

    // fallback to old polygon drawing if needed
    const PolyPolygon aToolsPolyPolygon( rB2DPolyPoly );
    DrawTransparent(PolyPolygon(rB2DPolyPoly), static_cast< sal_uInt16 >(fTransparency * 100.0));
}

// ------------------------------------------------------------------------

void OutputDevice::DrawTransparent( const PolyPolygon& rPolyPoly,
									USHORT nTransparencePercent )
{
	DBG_TRACE( "OutputDevice::DrawTransparent()" );
	DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

	// short circuit for drawing an opaque polygon
	if( (nTransparencePercent < 1) || ((mnDrawMode & DRAWMODE_NOTRANSPARENCY) != 0) )
	{
		DrawPolyPolygon( rPolyPoly );
		return;
	}

	// short circut for drawing an invisible polygon
	if( !mbFillColor || (nTransparencePercent >= 100) )
	{
		// short circuit if the polygon border is invisible too
		if( !mbLineColor )
			return;

		// DrawTransparent() assumes that the border is NOT to be drawn transparently???
		Push( PUSH_FILLCOLOR );
		SetFillColor();
		DrawPolyPolygon( rPolyPoly );
		Pop();
		return;
	}

	// handle metafile recording
	if( mpMetaFile )
		mpMetaFile->AddAction( new MetaTransparentAction( rPolyPoly, nTransparencePercent ) );

    bool bDrawn = !IsDeviceOutputNecessary() || ImplIsRecordLayout();
    if( bDrawn )
		return;

	// get the device graphics as drawing target
	if( !mpGraphics )
		if( !ImplGetGraphics() )
			return;

    // debug helper:
    static const char* pDisableNative = getenv( "SAL_DISABLE_NATIVE_ALPHA");

    // try hard to draw it directly, because the emulation layers are slower
	if( !pDisableNative
	    && mpGraphics->supportsOperation( OutDevSupport_B2DDraw ) 
#ifdef WIN32
        // workaround bad dithering on remote displaying when using GDI+ with toolbar buttoin hilighting
        && !rPolyPoly.IsRect()
#endif
        )
	{
		// prepare the graphics device
		if( mbInitClipRegion )
			ImplInitClipRegion();
        if( mbOutputClipped )
			return;
		if( mbInitLineColor )
			ImplInitLineColor();
		if( mbInitFillColor )
			ImplInitFillColor();

		// get the polygon in device coordinates
		basegfx::B2DPolyPolygon aB2DPolyPolygon( rPolyPoly.getB2DPolyPolygon() );
        aB2DPolyPolygon.setClosed( true );
		const ::basegfx::B2DHomMatrix aTransform = ImplGetDeviceTransformation();
		aB2DPolyPolygon.transform( aTransform );

		// draw the transparent polygon
		bDrawn = mpGraphics->DrawPolyPolygon( aB2DPolyPolygon, nTransparencePercent*0.01, this );

		// DrawTransparent() assumes that the border is NOT to be drawn transparently???
		if( mbLineColor )
		{
			// disable the fill color for now
			mpGraphics->SetFillColor();
			// draw the border line
			const basegfx::B2DVector aLineWidths( 1, 1 );
			const int nPolyCount = aB2DPolyPolygon.count();
			for( int nPolyIdx = 0; nPolyIdx < nPolyCount; ++nPolyIdx )
			{
				const ::basegfx::B2DPolygon& rPolygon = aB2DPolyPolygon.getB2DPolygon( nPolyIdx );
				mpGraphics->DrawPolyLine( rPolygon, aLineWidths, ::basegfx::B2DLINEJOIN_NONE, this );
			}
			// prepare to restore the fill color
			mbInitFillColor = mbFillColor;
		}
	}

	if( bDrawn )
		return;

	if( 1 )
	{
        VirtualDevice* pOldAlphaVDev = mpAlphaVDev;

        // #110958# Disable alpha VDev, we perform the necessary
        // operation explicitely further below.
        if( mpAlphaVDev )
            mpAlphaVDev = NULL;

		GDIMetaFile* pOldMetaFile = mpMetaFile;
		mpMetaFile = NULL;

#ifdef USE_JAVA
		if ( !mpGraphics )
			ImplGetGraphics();
		if ( mpGraphics )
		{
			((JavaSalGraphics *)mpGraphics)->setLineTransparency( (sal_uInt8)nTransparencePercent );
			((JavaSalGraphics *)mpGraphics)->setFillTransparency( (sal_uInt8)nTransparencePercent );

			DrawPolyPolygon( rPolyPoly );

			((JavaSalGraphics *)mpGraphics)->setLineTransparency( 0 );
			((JavaSalGraphics *)mpGraphics)->setFillTransparency( 0 );
		}
#else	// USE_JAVA
		if( OUTDEV_PRINTER == meOutDevType )
		{
			Rectangle		aPolyRect( LogicToPixel( rPolyPoly ).GetBoundRect() );
			const Size		aDPISize( LogicToPixel( Size( 1, 1 ), MAP_INCH ) );
			const long		nBaseExtent = Max( FRound( aDPISize.Width() / 300. ), 1L );
			long			nMove;
			const USHORT	nTrans = ( nTransparencePercent < 13 ) ? 0 :
									 ( nTransparencePercent < 38 ) ? 25 :
									 ( nTransparencePercent < 63 ) ? 50 :
									 ( nTransparencePercent < 88 ) ? 75 : 100;

			switch( nTrans )
			{
				case( 25 ): nMove = nBaseExtent * 3; break;
				case( 50 ): nMove = nBaseExtent * 4; break;
				case( 75 ): nMove = nBaseExtent * 6; break;
							// TODO What is the correct VALUE???
				default:    nMove = 0; break;
			}

			Push( PUSH_CLIPREGION | PUSH_LINECOLOR );
			IntersectClipRegion( rPolyPoly );
			SetLineColor( GetFillColor() );

			Rectangle aRect( aPolyRect.TopLeft(), Size( aPolyRect.GetWidth(), nBaseExtent ) );

			const BOOL bOldMap = mbMap;
			EnableMapMode( FALSE );

			while( aRect.Top() <= aPolyRect.Bottom() )
			{
				DrawRect( aRect );
				aRect.Move( 0, nMove );
			}

			aRect = Rectangle( aPolyRect.TopLeft(), Size( nBaseExtent, aPolyRect.GetHeight() ) );
			while( aRect.Left() <= aPolyRect.Right() )
			{
				DrawRect( aRect );
				aRect.Move( nMove, 0 );
			}

			EnableMapMode( bOldMap );
			Pop();
		}
		else
		{
 			PolyPolygon 	aPolyPoly( LogicToPixel( rPolyPoly ) );
			Rectangle		aPolyRect( aPolyPoly.GetBoundRect() );
			Point			aPoint;
			Rectangle		aDstRect( aPoint, GetOutputSizePixel() );

			aDstRect.Intersection( aPolyRect );

			if( OUTDEV_WINDOW == meOutDevType )
			{
				const Region aPaintRgn( ( (Window*) this )->GetPaintRegion() );

				if( !aPaintRgn.IsNull() )
					aDstRect.Intersection( LogicToPixel( aPaintRgn ).GetBoundRect() );
			}

			if( !aDstRect.IsEmpty() )
			{
                // #i66849# Added fast path for exactly rectangular
                // polygons
                // #i83087# Naturally, system alpha blending cannot
                // work with separate alpha VDev
                if( !mpAlphaVDev && !pDisableNative && aPolyPoly.IsRect() )
                {
                    // setup Graphics only here (other cases delegate
                    // to basic OutDev methods)
                    if( 1 )
                    {
                        if ( mbInitClipRegion )
                            ImplInitClipRegion();        
                        if ( mbInitLineColor )
                            ImplInitLineColor();
                        if ( mbInitFillColor )
                            ImplInitFillColor();
    
                        Rectangle aLogicPolyRect( rPolyPoly.GetBoundRect() );
                        Rectangle aPixelRect( ImplLogicToDevicePixel( aLogicPolyRect ) );

                        if( !mbOutputClipped )
                        {
                            bDrawn = mpGraphics->DrawAlphaRect( 
                               aPixelRect.Left(), aPixelRect.Top(), 
                               // #i98405# use methods with small g, else one pixel too much will be painted.
                               // This is because the source is a polygon which when painted would not paint
                               // the rightmost and lowest pixel line(s), so use one pixel less for the 
                               // rectangle, too.
                               aPixelRect.getWidth(), aPixelRect.getHeight(),
                               sal::static_int_cast<sal_uInt8>(nTransparencePercent), 
                               this );
                        }
                        else
                            bDrawn = true;
                    }
                }

                if( !bDrawn )
                {
                    VirtualDevice	aVDev( *this, 1 );
                    const Size		aDstSz( aDstRect.GetSize() );
                    const BYTE		cTrans = (BYTE) MinMax( FRound( nTransparencePercent * 2.55 ), 0, 255 );

                    if( aDstRect.Left() || aDstRect.Top() )
                        aPolyPoly.Move( -aDstRect.Left(), -aDstRect.Top() );

                    if( aVDev.SetOutputSizePixel( aDstSz ) )
                    {
                        const BOOL bOldMap = mbMap;

                        EnableMapMode( FALSE );

                        aVDev.SetLineColor( COL_BLACK );
                        aVDev.SetFillColor( COL_BLACK );
                        aVDev.DrawPolyPolygon( aPolyPoly );

                        Bitmap				aPaint( GetBitmap( aDstRect.TopLeft(), aDstSz ) );
                        Bitmap				aPolyMask( aVDev.GetBitmap( Point(), aDstSz ) );

                        // #107766# check for non-empty bitmaps before accessing them
                        if( !!aPaint && !!aPolyMask )
                        {
                            BitmapWriteAccess*	pW = aPaint.AcquireWriteAccess();
                            BitmapReadAccess*	pR = aPolyMask.AcquireReadAccess();

                            if( pW && pR )
                            {
                                BitmapColor 		aPixCol;
                                const BitmapColor	aFillCol( GetFillColor() );
                                const BitmapColor	aWhite( pR->GetBestMatchingColor( Color( COL_WHITE ) ) );
                                const BitmapColor	aBlack( pR->GetBestMatchingColor( Color( COL_BLACK ) ) );
                                const long			nWidth = pW->Width(), nHeight = pW->Height();
                                const long			nR = aFillCol.GetRed(), nG = aFillCol.GetGreen(), nB = aFillCol.GetBlue();
                                long				nX, nY;

                                if( aPaint.GetBitCount() <= 8 )
                                {
                                    const BitmapPalette&	rPal = pW->GetPalette();
                                    const USHORT			nCount = rPal.GetEntryCount();
                                    BitmapColor*			pMap = (BitmapColor*) new BYTE[ nCount * sizeof( BitmapColor ) ];

                                    for( USHORT i = 0; i < nCount; i++ )
                                    {
                                        BitmapColor aCol( rPal[ i ] );
                                        pMap[ i ] = BitmapColor( (BYTE) rPal.GetBestIndex( aCol.Merge( aFillCol, cTrans ) ) );
                                    }

                                    if( pR->GetScanlineFormat() == BMP_FORMAT_1BIT_MSB_PAL &&
                                        pW->GetScanlineFormat() == BMP_FORMAT_8BIT_PAL )
                                    {
                                        const BYTE cBlack = aBlack.GetIndex();

                                        for( nY = 0; nY < nHeight; nY++ )
                                        {
                                            Scanline	pWScan = pW->GetScanline( nY );
                                            Scanline	pRScan = pR->GetScanline( nY );
                                            BYTE		cBit = 128;

                                            for( nX = 0; nX < nWidth; nX++, cBit >>= 1, pWScan++ )
                                            {
                                                if( !cBit )
                                                    cBit = 128, pRScan++;

                                                if( ( *pRScan & cBit ) == cBlack )
                                                    *pWScan = (BYTE) pMap[ *pWScan ].GetIndex();
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for( nY = 0; nY < nHeight; nY++ )
                                            for( nX = 0; nX < nWidth; nX++ )
                                                if( pR->GetPixel( nY, nX ) == aBlack )
                                                    pW->SetPixel( nY, nX, pMap[ pW->GetPixel( nY, nX ).GetIndex() ] );
                                    }

                                    delete[] (BYTE*) pMap;
                                }
                                else
                                {
                                    if( pR->GetScanlineFormat() == BMP_FORMAT_1BIT_MSB_PAL &&
                                        pW->GetScanlineFormat() == BMP_FORMAT_24BIT_TC_BGR )
                                    {
                                        const BYTE cBlack = aBlack.GetIndex();

                                        for( nY = 0; nY < nHeight; nY++ )
                                        {
                                            Scanline	pWScan = pW->GetScanline( nY );
                                            Scanline	pRScan = pR->GetScanline( nY );
                                            BYTE		cBit = 128;

                                            for( nX = 0; nX < nWidth; nX++, cBit >>= 1, pWScan += 3 )
                                            {
                                                if( !cBit )
                                                    cBit = 128, pRScan++;

                                                if( ( *pRScan & cBit ) == cBlack )
                                                {
                                                    pWScan[ 0 ] = COLOR_CHANNEL_MERGE( pWScan[ 0 ], nB, cTrans );
                                                    pWScan[ 1 ] = COLOR_CHANNEL_MERGE( pWScan[ 1 ], nG, cTrans );
                                                    pWScan[ 2 ] = COLOR_CHANNEL_MERGE( pWScan[ 2 ], nR, cTrans );
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for( nY = 0; nY < nHeight; nY++ )
                                        {
                                            for( nX = 0; nX < nWidth; nX++ )
                                            {
                                                if( pR->GetPixel( nY, nX ) == aBlack )
                                                {
                                                    aPixCol = pW->GetColor( nY, nX );
                                                    pW->SetPixel( nY, nX, aPixCol.Merge( aFillCol, cTrans ) );
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            aPolyMask.ReleaseAccess( pR );
                            aPaint.ReleaseAccess( pW );

                            DrawBitmap( aDstRect.TopLeft(), aPaint );

                            EnableMapMode( bOldMap );

                            if( mbLineColor )
                            {
                                Push( PUSH_FILLCOLOR );
                                SetFillColor();
                                DrawPolyPolygon( rPolyPoly );
                                Pop();
                            }
                        }
                    }
                    else
                        DrawPolyPolygon( rPolyPoly );
                }
            }
        }
#endif	// USE_JAVA

		mpMetaFile = pOldMetaFile;

        // #110958# Restore disabled alpha VDev
        mpAlphaVDev = pOldAlphaVDev;

        // #110958# Apply alpha value also to VDev alpha channel
        if( mpAlphaVDev )
        {
            const Color aFillCol( mpAlphaVDev->GetFillColor() );
            mpAlphaVDev->SetFillColor( Color(sal::static_int_cast<UINT8>(255*nTransparencePercent/100),
                                             sal::static_int_cast<UINT8>(255*nTransparencePercent/100),
                                             sal::static_int_cast<UINT8>(255*nTransparencePercent/100)) );

            mpAlphaVDev->DrawTransparent( rPolyPoly, nTransparencePercent );

            mpAlphaVDev->SetFillColor( aFillCol );
        }
	}
}

// -----------------------------------------------------------------------

void OutputDevice::DrawTransparent( const GDIMetaFile& rMtf, const Point& rPos,
									const Size& rSize, const Gradient& rTransparenceGradient )
{
	DBG_TRACE( "OutputDevice::DrawTransparent()" );
	DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

	const Color aBlack( COL_BLACK );

	if( mpMetaFile )
		mpMetaFile->AddAction( new MetaFloatTransparentAction( rMtf, rPos, rSize, rTransparenceGradient ) );

	if( ( rTransparenceGradient.GetStartColor() == aBlack && rTransparenceGradient.GetEndColor() == aBlack ) ||
		( mnDrawMode & ( DRAWMODE_NOTRANSPARENCY ) ) )
	{
		( (GDIMetaFile&) rMtf ).WindStart();
		( (GDIMetaFile&) rMtf ).Play( this, rPos, rSize );
		( (GDIMetaFile&) rMtf ).WindStart();
	}
	else
	{
		GDIMetaFile*	pOldMetaFile = mpMetaFile;
		Rectangle		aOutRect( LogicToPixel( rPos ), LogicToPixel( rSize ) );
		Point			aPoint;
		Rectangle		aDstRect( aPoint, GetOutputSizePixel() );

		mpMetaFile = NULL;
		aDstRect.Intersection( aOutRect );

		if( OUTDEV_WINDOW == meOutDevType )
		{
			const Region aPaintRgn( ( (Window*) this )->GetPaintRegion() );

			if( !aPaintRgn.IsNull() )
				aDstRect.Intersection( LogicToPixel( aPaintRgn.GetBoundRect() ) );
		}

		if( !aDstRect.IsEmpty() )
		{
			VirtualDevice* pVDev = new VirtualDevice;

#ifdef USE_JAVA
			// Prevent runaway memory usage when drawing to the printer by
			// lower the resolution of the temporary buffer if necessary
			Rectangle aVirDevRect( Point( 0, 0 ), aDstRect.GetSize() );
			float fExcessPixelRatio = (float)MAX_TRANSPARENT_GRADIENT_BMP_PIXELS / ( ( mnDPIX * aDstRect.GetWidth() ) + ( mnDPIY * aDstRect.GetHeight() ) );
			if ( fExcessPixelRatio < 1.0 )
			{
				((OutputDevice*)pVDev)->mnDPIX = (sal_uInt32)( mnDPIX * fExcessPixelRatio );
				((OutputDevice*)pVDev)->mnDPIY = (sal_uInt32)( mnDPIY * fExcessPixelRatio );

				if ( ((OutputDevice*)pVDev)->mnDPIX < MIN_TRANSPARENT_GRADIENT_RESOLUTION)
					((OutputDevice*)pVDev)->mnDPIX = MIN_TRANSPARENT_GRADIENT_RESOLUTION;
				if ( ((OutputDevice*)pVDev)->mnDPIY < MIN_TRANSPARENT_GRADIENT_RESOLUTION)
					((OutputDevice*)pVDev)->mnDPIY = MIN_TRANSPARENT_GRADIENT_RESOLUTION;
				Fraction aScaleX( ((OutputDevice*)pVDev)->mnDPIX, mnDPIX );
				Fraction aScaleY( ((OutputDevice*)pVDev)->mnDPIY, mnDPIY );

				aVirDevRect = Rectangle( Point( 0, 0 ), Size( (long)( ( (double)aScaleX * aDstRect.GetWidth() ) + 0.5 ), (long)( ( (double)aScaleY * aDstRect.GetHeight() ) + 0.5 ) ) );
			}
			else
			{
				((OutputDevice*)pVDev)->mnDPIX = mnDPIX;
				((OutputDevice*)pVDev)->mnDPIY = mnDPIY;
			}

			if( pVDev->SetOutputSizePixel( aVirDevRect.GetSize() ) )
#else	// USE_JAVA
			((OutputDevice*)pVDev)->mnDPIX = mnDPIX;
			((OutputDevice*)pVDev)->mnDPIY = mnDPIY;

			if( pVDev->SetOutputSizePixel( aDstRect.GetSize() ) )
#endif	// USE_JAVA
			{
				Bitmap		aPaint, aMask;
				AlphaMask	aAlpha;
				MapMode 	aMap( GetMapMode() );
				Point		aOutPos( PixelToLogic( aDstRect.TopLeft() ) );
				const BOOL	bOldMap = mbMap;

				aMap.SetOrigin( Point( -aOutPos.X(), -aOutPos.Y() ) );
				pVDev->SetMapMode( aMap );
				const BOOL	bVDevOldMap = pVDev->IsMapModeEnabled();

				// create paint bitmap
				( (GDIMetaFile&) rMtf ).WindStart();
				( (GDIMetaFile&) rMtf ).Play( pVDev, rPos, rSize );
				( (GDIMetaFile&) rMtf ).WindStart();
				pVDev->EnableMapMode( FALSE );
				aPaint = pVDev->GetBitmap( Point(), pVDev->GetOutputSizePixel() );
				pVDev->EnableMapMode( bVDevOldMap ); // #i35331#: MUST NOT use EnableMapMode( TRUE ) here!

				// create mask bitmap
				pVDev->SetLineColor( COL_BLACK );
				pVDev->SetFillColor( COL_BLACK );
				pVDev->DrawRect( Rectangle( pVDev->PixelToLogic( Point() ), pVDev->GetOutputSize() ) );
				pVDev->SetDrawMode( DRAWMODE_WHITELINE | DRAWMODE_WHITEFILL | DRAWMODE_WHITETEXT |
									DRAWMODE_WHITEBITMAP | DRAWMODE_WHITEGRADIENT );
				( (GDIMetaFile&) rMtf ).WindStart();
				( (GDIMetaFile&) rMtf ).Play( pVDev, rPos, rSize );
				( (GDIMetaFile&) rMtf ).WindStart();
				pVDev->EnableMapMode( FALSE );
				aMask = pVDev->GetBitmap( Point(), pVDev->GetOutputSizePixel() );
				pVDev->EnableMapMode( bVDevOldMap ); // #i35331#: MUST NOT use EnableMapMode( TRUE ) here!

				// create alpha mask from gradient
				pVDev->SetDrawMode( DRAWMODE_GRAYGRADIENT );
				pVDev->DrawGradient( Rectangle( rPos, rSize ), rTransparenceGradient );
				pVDev->SetDrawMode( DRAWMODE_DEFAULT );
				pVDev->EnableMapMode( FALSE );
				pVDev->DrawMask( Point(), pVDev->GetOutputSizePixel(), aMask, Color( COL_WHITE ) );

				aAlpha = pVDev->GetBitmap( Point(), pVDev->GetOutputSizePixel() );

				delete pVDev;

				EnableMapMode( FALSE );
#ifdef USE_JAVA
				DrawBitmapEx( aDstRect.TopLeft(), aDstRect.GetSize(), aVirDevRect.TopLeft(), aVirDevRect.GetSize(), BitmapEx( aPaint, aAlpha ) );
#else	// USE_JAVA
				DrawBitmapEx( aDstRect.TopLeft(), BitmapEx( aPaint, aAlpha ) );
#endif	// USE_JAVA
				EnableMapMode( bOldMap );
			}
			else
				delete pVDev;
		}

		mpMetaFile = pOldMetaFile;
	}
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawColorWallpaper( long nX, long nY,
										   long nWidth, long nHeight,
										   const Wallpaper& rWallpaper )
{
	// Wallpaper ohne Umrandung zeichnen
	Color aOldLineColor = GetLineColor();
	Color aOldFillColor = GetFillColor();
	SetLineColor();
	SetFillColor( rWallpaper.GetColor() );
    BOOL bMap = mbMap;
    EnableMapMode( FALSE );
    DrawRect( Rectangle( Point( nX, nY ), Size( nWidth, nHeight ) ) );
	SetLineColor( aOldLineColor );
	SetFillColor( aOldFillColor );
    EnableMapMode( bMap );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawBitmapWallpaper( long nX, long nY,
											long nWidth, long nHeight,
											const Wallpaper& rWallpaper )
{
	BitmapEx				aBmpEx;
	const BitmapEx* 		pCached = rWallpaper.ImplGetImpWallpaper()->ImplGetCachedBitmap();
	Point					aPos;
	Size					aSize;
	GDIMetaFile*			pOldMetaFile = mpMetaFile;
	const WallpaperStyle	eStyle = rWallpaper.GetStyle();
	const BOOL				bOldMap = mbMap;
	BOOL					bDrawn = FALSE;
	BOOL					bDrawGradientBackground = FALSE;
	BOOL					bDrawColorBackground = FALSE;

	if( pCached )
		aBmpEx = *pCached;
	else
		aBmpEx = rWallpaper.GetBitmap();

	const long nBmpWidth = aBmpEx.GetSizePixel().Width();
	const long nBmpHeight = aBmpEx.GetSizePixel().Height();
	const BOOL bTransparent = aBmpEx.IsTransparent();

	// draw background
	if( bTransparent )
	{
		if( rWallpaper.IsGradient() )
			bDrawGradientBackground = TRUE;
		else
		{
			if( !pCached && !rWallpaper.GetColor().GetTransparency() )
			{
				VirtualDevice aVDev( *this );
				aVDev.SetBackground( rWallpaper.GetColor() );
				aVDev.SetOutputSizePixel( Size( nBmpWidth, nBmpHeight ) );
				aVDev.DrawBitmapEx( Point(), aBmpEx );
				aBmpEx = aVDev.GetBitmap( Point(), aVDev.GetOutputSizePixel() );
			}

			bDrawColorBackground = TRUE;
		}
	}
	else if( eStyle != WALLPAPER_TILE && eStyle != WALLPAPER_SCALE )
	{
		if( rWallpaper.IsGradient() )
			bDrawGradientBackground = TRUE;
		else
			bDrawColorBackground = TRUE;
	}

	// background of bitmap?
	if( bDrawGradientBackground )
		ImplDrawGradientWallpaper( nX, nY, nWidth, nHeight, rWallpaper );
	else if( bDrawColorBackground && bTransparent )
	{
		ImplDrawColorWallpaper( nX, nY, nWidth, nHeight, rWallpaper );
		bDrawColorBackground = FALSE;
	}

	// calc pos and size
	if( rWallpaper.IsRect() )
	{
		const Rectangle aBound( LogicToPixel( rWallpaper.GetRect() ) );
		aPos = aBound.TopLeft();
		aSize = aBound.GetSize();
	}
	else
	{
		aPos = Point( nX, nY );
		aSize = Size( nWidth, nHeight );
	}

	mpMetaFile = NULL;
	EnableMapMode( FALSE );
	Push( PUSH_CLIPREGION );
	IntersectClipRegion( Rectangle( Point( nX, nY ), Size( nWidth, nHeight ) ) );

	switch( eStyle )
	{
		case( WALLPAPER_SCALE ):
		{
			if( !pCached || ( pCached->GetSizePixel() != aSize ) )
			{
				if( pCached )
					rWallpaper.ImplGetImpWallpaper()->ImplReleaseCachedBitmap();

				aBmpEx = rWallpaper.GetBitmap();
				aBmpEx.Scale( aSize );
				aBmpEx = BitmapEx( aBmpEx.GetBitmap().CreateDisplayBitmap( this ), aBmpEx.GetMask() );
			}
		}
		break;

		case( WALLPAPER_TOPLEFT ):
		break;

		case( WALLPAPER_TOP ):
			aPos.X() += ( aSize.Width() - nBmpWidth ) >> 1;
		break;

		case( WALLPAPER_TOPRIGHT ):
			aPos.X() += ( aSize.Width() - nBmpWidth );
		break;

		case( WALLPAPER_LEFT ):
			aPos.Y() += ( aSize.Height() - nBmpHeight ) >> 1;
		break;

		case( WALLPAPER_CENTER ):
		{
			aPos.X() += ( aSize.Width() - nBmpWidth ) >> 1;
			aPos.Y() += ( aSize.Height() - nBmpHeight ) >> 1;
		}
		break;

		case( WALLPAPER_RIGHT ):
		{
			aPos.X() += ( aSize.Width() - nBmpWidth );
			aPos.Y() += ( aSize.Height() - nBmpHeight ) >> 1;
		}
		break;

		case( WALLPAPER_BOTTOMLEFT ):
			aPos.Y() += ( aSize.Height() - nBmpHeight );
		break;

		case( WALLPAPER_BOTTOM ):
		{
			aPos.X() += ( aSize.Width() - nBmpWidth ) >> 1;
			aPos.Y() += ( aSize.Height() - nBmpHeight );
		}
		break;

		case( WALLPAPER_BOTTOMRIGHT ):
		{
			aPos.X() += ( aSize.Width() - nBmpWidth );
			aPos.Y() += ( aSize.Height() - nBmpHeight );
		}
		break;

		default:
		{
			const long	nRight = nX + nWidth - 1L;
			const long	nBottom = nY + nHeight - 1L;
			long		nFirstX;
			long		nFirstY;

			if( eStyle == WALLPAPER_TILE )
			{
				nFirstX = aPos.X();
				nFirstY = aPos.Y();
			}
			else
			{
				nFirstX = aPos.X() + ( ( aSize.Width() - nBmpWidth ) >> 1 );
				nFirstY = aPos.Y() + ( ( aSize.Height() - nBmpHeight ) >> 1 );
			}

			const long	nOffX = ( nFirstX - nX ) % nBmpWidth;
			const long	nOffY = ( nFirstY - nY ) % nBmpHeight;
			long		nStartX = nX + nOffX;
			long		nStartY = nY + nOffY;

			if( nOffX > 0L )
				nStartX -= nBmpWidth;

			if( nOffY > 0L )
				nStartY -= nBmpHeight;

			for( long nBmpY = nStartY; nBmpY <= nBottom; nBmpY += nBmpHeight )
				for( long nBmpX = nStartX; nBmpX <= nRight; nBmpX += nBmpWidth )
					DrawBitmapEx( Point( nBmpX, nBmpY ), aBmpEx );

			bDrawn = TRUE;
		}
		break;
	}

	if( !bDrawn )
	{
		// optimized for non-transparent bitmaps
		if( bDrawColorBackground )
		{
			const Size		aBmpSize( aBmpEx.GetSizePixel() );
			const Point 	aTmpPoint;
			const Rectangle aOutRect( aTmpPoint, GetOutputSizePixel() );
			const Rectangle aColRect( Point( nX, nY ), Size( nWidth, nHeight ) );
			Rectangle		aWorkRect;

			aWorkRect = Rectangle( 0, 0, aOutRect.Right(), aPos.Y() - 1L );
			aWorkRect.Justify();
			aWorkRect.Intersection( aColRect );
			if( !aWorkRect.IsEmpty() )
			{
				ImplDrawColorWallpaper( aWorkRect.Left(), aWorkRect.Top(),
										aWorkRect.GetWidth(), aWorkRect.GetHeight(),
										rWallpaper );
			}

			aWorkRect = Rectangle( 0, aPos.Y(), aPos.X() - 1L, aPos.Y() + aBmpSize.Height() - 1L );
			aWorkRect.Justify();
			aWorkRect.Intersection( aColRect );
			if( !aWorkRect.IsEmpty() )
			{
				ImplDrawColorWallpaper( aWorkRect.Left(), aWorkRect.Top(),
										aWorkRect.GetWidth(), aWorkRect.GetHeight(),
										rWallpaper );
			}

			aWorkRect = Rectangle( aPos.X() + aBmpSize.Width(), aPos.Y(), aOutRect.Right(), aPos.Y() + aBmpSize.Height() - 1L );
			aWorkRect.Justify();
			aWorkRect.Intersection( aColRect );
			if( !aWorkRect.IsEmpty() )
			{
				ImplDrawColorWallpaper( aWorkRect.Left(), aWorkRect.Top(),
										aWorkRect.GetWidth(), aWorkRect.GetHeight(),
										rWallpaper );
			}

			aWorkRect = Rectangle( 0, aPos.Y() + aBmpSize.Height(), aOutRect.Right(), aOutRect.Bottom() );
			aWorkRect.Justify();
			aWorkRect.Intersection( aColRect );
			if( !aWorkRect.IsEmpty() )
			{
				ImplDrawColorWallpaper( aWorkRect.Left(), aWorkRect.Top(),
										aWorkRect.GetWidth(), aWorkRect.GetHeight(),
										rWallpaper );
			}
		}

		DrawBitmapEx( aPos, aBmpEx );
	}

	rWallpaper.ImplGetImpWallpaper()->ImplSetCachedBitmap( aBmpEx );

	Pop();
	EnableMapMode( bOldMap );
	mpMetaFile = pOldMetaFile;
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawGradientWallpaper( long nX, long nY,
											  long nWidth, long nHeight,
											  const Wallpaper& rWallpaper )
{
	Rectangle		aBound;
	GDIMetaFile*	pOldMetaFile = mpMetaFile;
	const BOOL		bOldMap = mbMap;
    BOOL            bNeedGradient = TRUE;

/*
	if ( rWallpaper.IsRect() )
		aBound = LogicToPixel( rWallpaper.GetRect() );
	else
*/
		aBound = Rectangle( Point( nX, nY ), Size( nWidth, nHeight ) );

	mpMetaFile = NULL;
	EnableMapMode( FALSE );
	Push( PUSH_CLIPREGION );
	IntersectClipRegion( Rectangle( Point( nX, nY ), Size( nWidth, nHeight ) ) );

    if( OUTDEV_WINDOW == meOutDevType && rWallpaper.GetStyle() == WALLPAPER_APPLICATIONGRADIENT )
    {
        Window *pWin = dynamic_cast< Window* >( this );
        if( pWin )
        {
            // limit gradient to useful size, so that it still can be noticed
            // in maximized windows
            long gradientWidth = pWin->GetDesktopRectPixel().GetSize().Width();
            if( gradientWidth > 1024 )
                gradientWidth = 1024;
            if( mnOutOffX+nWidth > gradientWidth )
		        ImplDrawColorWallpaper(  nX, nY, nWidth, nHeight, rWallpaper.GetGradient().GetEndColor() );
            if( mnOutOffX > gradientWidth )
                bNeedGradient = FALSE;
            else
                aBound = Rectangle( Point( -mnOutOffX, nY ), Size( gradientWidth, nHeight ) );
        }
    }

    if( bNeedGradient )
	    DrawGradient( aBound, rWallpaper.GetGradient() );

	Pop();
	EnableMapMode( bOldMap );
	mpMetaFile = pOldMetaFile;
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawWallpaper( long nX, long nY,
									  long nWidth, long nHeight,
									  const Wallpaper& rWallpaper )
{
	if( rWallpaper.IsBitmap() )
		ImplDrawBitmapWallpaper( nX, nY, nWidth, nHeight, rWallpaper );
	else if( rWallpaper.IsGradient() )
		ImplDrawGradientWallpaper( nX, nY, nWidth, nHeight, rWallpaper );
	else
		ImplDrawColorWallpaper(  nX, nY, nWidth, nHeight, rWallpaper );
}

// -----------------------------------------------------------------------

void OutputDevice::DrawWallpaper( const Rectangle& rRect,
								  const Wallpaper& rWallpaper )
{
	if ( mpMetaFile )
		mpMetaFile->AddAction( new MetaWallpaperAction( rRect, rWallpaper ) );

	if ( !IsDeviceOutputNecessary() || ImplIsRecordLayout() )
		return;

	if ( rWallpaper.GetStyle() != WALLPAPER_NULL )
	{
		Rectangle aRect = LogicToPixel( rRect );
		aRect.Justify();

		if ( !aRect.IsEmpty() )
		{
			ImplDrawWallpaper( aRect.Left(), aRect.Top(), aRect.GetWidth(), aRect.GetHeight(),
							   rWallpaper );
		}
	}
    
    if( mpAlphaVDev )
        mpAlphaVDev->DrawWallpaper( rRect, rWallpaper );
}

// -----------------------------------------------------------------------

void OutputDevice::Erase()
{
	if ( !IsDeviceOutputNecessary() || ImplIsRecordLayout() )
		return;
    
    BOOL bNativeOK = FALSE; 
    if( meOutDevType == OUTDEV_WINDOW )
    {
        Window* pWindow = static_cast<Window*>(this);
        ControlPart aCtrlPart = pWindow->ImplGetWindowImpl()->mnNativeBackground; 
        if( aCtrlPart != 0 && ! pWindow->IsControlBackground() )
        {
            ImplControlValue    aControlValue;
            Point               aGcc3WorkaroundTemporary;
            Region              aCtrlRegion( Rectangle( aGcc3WorkaroundTemporary, GetOutputSizePixel() ) );
            ControlState        nState = 0;
            
            if( pWindow->IsEnabled() ) 				nState |= CTRL_STATE_ENABLED;
            bNativeOK = pWindow->DrawNativeControl( CTRL_WINDOW_BACKGROUND, aCtrlPart, aCtrlRegion,
                                                    nState, aControlValue, rtl::OUString() );
        }
    }

	if ( mbBackground && ! bNativeOK )
	{
		RasterOp eRasterOp = GetRasterOp();
		if ( eRasterOp != ROP_OVERPAINT )
			SetRasterOp( ROP_OVERPAINT );
		ImplDrawWallpaper( 0, 0, mnOutWidth, mnOutHeight, maBackground );
		if ( eRasterOp != ROP_OVERPAINT )
			SetRasterOp( eRasterOp );
	}

    if( mpAlphaVDev )
        mpAlphaVDev->Erase();
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDraw2ColorFrame( const Rectangle& rRect,
										const Color& rLeftTopColor,
										const Color& rRightBottomColor )
{
	SetFillColor( rLeftTopColor );
	DrawRect( Rectangle( rRect.TopLeft(), Point( rRect.Left(), rRect.Bottom()-1 ) ) );
	DrawRect( Rectangle( rRect.TopLeft(), Point( rRect.Right()-1, rRect.Top() ) ) );
	SetFillColor( rRightBottomColor );
	DrawRect( Rectangle( rRect.BottomLeft(), rRect.BottomRight() ) );
	DrawRect( Rectangle( rRect.TopRight(), rRect.BottomRight() ) );
}

// -----------------------------------------------------------------------

void OutputDevice::DrawEPS( const Point& rPoint, const Size& rSize,
							const GfxLink& rGfxLink, GDIMetaFile* pSubst )
{
	if ( mpMetaFile )
	{
		GDIMetaFile aSubst;

		if( pSubst )
			aSubst = *pSubst;

		mpMetaFile->AddAction( new MetaEPSAction( rPoint, rSize, rGfxLink, aSubst ) );
	}

	if ( !IsDeviceOutputNecessary() || ImplIsRecordLayout() )
		return;

	if( mbOutputClipped )
		return;

	Rectangle	aRect( ImplLogicToDevicePixel( Rectangle( rPoint, rSize ) ) );
	if( !aRect.IsEmpty() )
	{
        // draw the real EPS graphics
		bool bDrawn = FALSE;
        if( rGfxLink.GetData() && rGfxLink.GetDataSize() )
        {
            if( !mpGraphics && !ImplGetGraphics() )
                return;

            if( mbInitClipRegion )
                ImplInitClipRegion();

            aRect.Justify();
            bDrawn = mpGraphics->DrawEPS( aRect.Left(), aRect.Top(), aRect.GetWidth(), aRect.GetHeight(),
                         (BYTE*) rGfxLink.GetData(), rGfxLink.GetDataSize(), this );
        }

        // else draw the substitution graphics
		if( !bDrawn && pSubst )
		{
			GDIMetaFile* pOldMetaFile = mpMetaFile;

			mpMetaFile = NULL;
			Graphic( *pSubst ).Draw( this, rPoint, rSize );
			mpMetaFile = pOldMetaFile;
		}
	}

    if( mpAlphaVDev )
        mpAlphaVDev->DrawEPS( rPoint, rSize, rGfxLink, pSubst );
}
