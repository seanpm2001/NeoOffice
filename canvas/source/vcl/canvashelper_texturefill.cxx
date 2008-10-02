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
 * Modified December 2006 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_canvas.hxx"

#include <canvas/debug.hxx>
#include <tools/diagnose_ex.h>

#include <rtl/math.hxx>

#include <com/sun/star/rendering/TextDirection.hpp>
#include <com/sun/star/rendering/TexturingMode.hpp>
#include <com/sun/star/rendering/PathCapType.hpp>
#include <com/sun/star/rendering/PathJoinType.hpp>

#include <tools/poly.hxx>
#include <vcl/window.hxx>
#include <vcl/bitmapex.hxx>
#include <vcl/bmpacc.hxx>
#include <vcl/virdev.hxx>
#include <vcl/canvastools.hxx>

#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/range/b2drectangle.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/vector/b2dsize.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <basegfx/polygon/b2dlinegeometry.hxx>
#include <basegfx/tools/tools.hxx>
#include <basegfx/tools/canvastools.hxx>
#include <basegfx/numeric/ftools.hxx>

#include <comphelper/sequence.hxx>

#include <canvas/canvastools.hxx>
#include <canvas/parametricpolypolygon.hxx>

#include "spritecanvas.hxx"
#include "canvashelper.hxx"
#include "impltools.hxx"


using namespace ::com::sun::star;

namespace vclcanvas
{
    namespace
    {
        bool textureFill( OutputDevice&			rOutDev,
                          GraphicObject&		rGraphic,
                          const ::Point&		rPosPixel,
                          const ::Size&			rNextTileX,
                          const ::Size&			rNextTileY,
                          sal_Int32				nTilesX,
                          sal_Int32				nTilesY,
                          const ::Size&			rTileSize,
                          const GraphicAttr&	rAttr)
        {
            BOOL	bRet( false );
            Point 	aCurrPos;
            int 	nX, nY;

            for( nY=0; nY < nTilesY; ++nY )
            {
                aCurrPos.X() = rPosPixel.X() + nY*rNextTileY.Width();
                aCurrPos.Y() = rPosPixel.Y() + nY*rNextTileY.Height();

                for( nX=0; nX < nTilesX; ++nX )
                {
                    // update return value. This method should return true, if
                    // at least one of the looped Draws succeeded.
                    bRet |= rGraphic.Draw( &rOutDev, 
                                           aCurrPos,
                                           rTileSize,
                                           &rAttr );
                    
                    aCurrPos.X() += rNextTileX.Width();
                    aCurrPos.Y() += rNextTileX.Height();
                }
            }

            return bRet;
        }


        /** Fill linear or axial gradient

        	Since most of the code for linear and axial gradients are
        	the same, we've a unified method here
         */
        void fillGeneralLinearGradient( OutputDevice&					rOutDev,
                                        const ::basegfx::B2DHomMatrix&	rTextureTransform,
                                        const ::Rectangle&				rBounds,
                                        int								nStepCount,
                                        const ::Color& 					rColor1,
                                        const ::Color& 					rColor2,
                                        bool							bFillNonOverlapping,
                                        bool							bAxialGradient )
        {
            (void)bFillNonOverlapping;

            // determine general position of gradient in relation to
            // the bound rect
            // =====================================================

            ::basegfx::B2DPoint aLeftTop( 0.0, 0.0 );
            ::basegfx::B2DPoint aLeftBottom( 0.0, 1.0 );
            ::basegfx::B2DPoint aRightTop( 1.0, 0.0 );
            ::basegfx::B2DPoint aRightBottom( 1.0, 1.0 );

            aLeftTop	*= rTextureTransform;
            aLeftBottom *= rTextureTransform;
            aRightTop 	*= rTextureTransform;
            aRightBottom*= rTextureTransform;

            // calc length of bound rect diagonal
            const ::basegfx::B2DVector aBoundRectDiagonal( 
                ::vcl::unotools::b2DPointFromPoint( rBounds.TopLeft() ) -
                ::vcl::unotools::b2DPointFromPoint( rBounds.BottomRight() ) );
            const double nDiagonalLength( aBoundRectDiagonal.getLength() );

            // create direction of gradient:
            //     _______
            //     |  |  | 
            // ->  |  |  | ...
            //     |  |  | 
            //     -------            
            ::basegfx::B2DVector aDirection( aRightTop - aLeftTop );
            aDirection.normalize();

            // now, we potentially have to enlarge our gradient area
            // atop and below the transformed [0,1]x[0,1] unit rect,
            // for the gradient to fill the complete bound rect.
            ::basegfx::tools::infiniteLineFromParallelogram( aLeftTop,
                                                             aLeftBottom,
                                                             aRightTop,
                                                             aRightBottom,
                                                             ::vcl::unotools::b2DRectangleFromRectangle( rBounds ) );
            

            // render gradient
            // ===============

            // for linear gradients, it's easy to render
            // non-overlapping polygons: just split the gradient into
            // nStepCount small strips. Prepare the strip now.

            // For performance reasons, we create a temporary VCL
            // polygon here, keep it all the way and only change the
            // vertex values in the loop below (as ::Polygon is a
            // pimpl class, creating one every loop turn would really
            // stress the mem allocator)
            ::Polygon aTempPoly( static_cast<USHORT>(5) );

            OSL_ENSURE( nStepCount >= 3,
                        "fillLinearGradient(): stepcount smaller than 3" );

        
            // fill initial strip (extending two times the bound rect's
            // diagonal to the 'left'
            // ------------------------------------------------------

            // calculate left edge, by moving left edge of the
            // gradient rect two times the bound rect's diagonal to
            // the 'left'. Since we postpone actual rendering into the
            // loop below, we set the _right_ edge here, which will be
            // readily copied into the left edge in the loop below
            const ::basegfx::B2DPoint& rPoint1( aLeftTop - 2.0*nDiagonalLength*aDirection );
            aTempPoly[1] = ::Point( ::basegfx::fround( rPoint1.getX() ),
                                    ::basegfx::fround( rPoint1.getY() ) );

            const ::basegfx::B2DPoint& rPoint2( aLeftBottom - 2.0*nDiagonalLength*aDirection );
            aTempPoly[2] = ::Point( ::basegfx::fround( rPoint2.getX() ),
                                    ::basegfx::fround( rPoint2.getY() ) );


            // iteratively render all other strips
            // -----------------------------------
            
            // ensure that nStepCount is odd, to have a well-defined
            // middle index for axial gradients.
            if( bAxialGradient && !(nStepCount % 2) )
                ++nStepCount;

            const int nStepCountHalved( nStepCount / 2 );

            // only iterate nStepCount-1 steps, as the last strip is
            // explicitely painted below
            for( int i=0; i<nStepCount-1; ++i )
            {
                // lerp color
                if( bAxialGradient )
                {
                    // axial gradient has a triangle-like interpolation function
                    const int iPrime( i<=nStepCountHalved ? i : nStepCount-i-1);

                    rOutDev.SetFillColor( 
                        Color( (UINT8)(((nStepCountHalved - iPrime)*rColor1.GetRed() + iPrime*rColor2.GetRed())/nStepCountHalved),
                               (UINT8)(((nStepCountHalved - iPrime)*rColor1.GetGreen() + iPrime*rColor2.GetGreen())/nStepCountHalved),
                               (UINT8)(((nStepCountHalved - iPrime)*rColor1.GetBlue() + iPrime*rColor2.GetBlue())/nStepCountHalved) ) );
                }
                else
                {
                    // linear gradient has a plain lerp between start and end color
                    rOutDev.SetFillColor( 
                        Color( (UINT8)(((nStepCount - i)*rColor1.GetRed() + i*rColor2.GetRed())/nStepCount),
                               (UINT8)(((nStepCount - i)*rColor1.GetGreen() + i*rColor2.GetGreen())/nStepCount),
                               (UINT8)(((nStepCount - i)*rColor1.GetBlue() + i*rColor2.GetBlue())/nStepCount) ) );
                }

                // copy right egde of polygon to left edge (and also
                // copy the closing point)
                aTempPoly[0] = aTempPoly[4] = aTempPoly[1];
                aTempPoly[3] = aTempPoly[2];

                // calculate new right edge, from interpolating
                // between start and end line. Note that i is
                // increased by one, to account for the fact that we
                // calculate the right border here (whereas the fill
                // color is governed by the left edge)
                const ::basegfx::B2DPoint& rPoint3( 
                    (nStepCount - i-1)/double(nStepCount)*aLeftTop + 
                    (i+1)/double(nStepCount)*aRightTop );
                aTempPoly[1] = ::Point( ::basegfx::fround( rPoint3.getX() ),
                                        ::basegfx::fround( rPoint3.getY() ) );

                const ::basegfx::B2DPoint& rPoint4( 
                    (nStepCount - i-1)/double(nStepCount)*aLeftBottom + 
                    (i+1)/double(nStepCount)*aRightBottom );
                aTempPoly[2] = ::Point( ::basegfx::fround( rPoint4.getX() ),
                                        ::basegfx::fround( rPoint4.getY() ) );
                
                rOutDev.DrawPolygon( aTempPoly );
            }

            // fill final strip (extending two times the bound rect's
            // diagonal to the 'right'
            // ------------------------------------------------------

            // copy right egde of polygon to left edge (and also
            // copy the closing point)
            aTempPoly[0] = aTempPoly[4] = aTempPoly[1];
            aTempPoly[3] = aTempPoly[2];

            // calculate new right edge, by moving right edge of the
            // gradient rect two times the bound rect's diagonal to
            // the 'right'.
            const ::basegfx::B2DPoint& rPoint3( aRightTop + 2.0*nDiagonalLength*aDirection );
            aTempPoly[0] = aTempPoly[4] = ::Point( ::basegfx::fround( rPoint3.getX() ),
                                                   ::basegfx::fround( rPoint3.getY() ) );

            const ::basegfx::B2DPoint& rPoint4( aRightBottom + 2.0*nDiagonalLength*aDirection );
            aTempPoly[3] = ::Point( ::basegfx::fround( rPoint4.getX() ),
                                    ::basegfx::fround( rPoint4.getY() ) );

            if( bAxialGradient )
                rOutDev.SetFillColor( rColor1 );
            else
                rOutDev.SetFillColor( rColor2 );

            rOutDev.DrawPolygon( aTempPoly );
        }


        inline void fillLinearGradient( OutputDevice&							rOutDev,
                                        const ::Color&							rColor1,
                                        const ::Color&							rColor2, 
                                        const ::basegfx::B2DHomMatrix&			rTextureTransform,
                                        const ::Rectangle&						rBounds,
                                        int										nStepCount,
                                        bool									bFillNonOverlapping )
        {
            fillGeneralLinearGradient( rOutDev,
                                       rTextureTransform,
                                       rBounds,
                                       nStepCount,
                                       rColor1,
                                       rColor2,
                                       bFillNonOverlapping,
                                       false );
        }

        inline void fillAxialGradient( OutputDevice&							rOutDev,
                                       const ::Color&							rColor1,
                                       const ::Color&							rColor2, 
                                       const ::basegfx::B2DHomMatrix&			rTextureTransform,
                                       const ::Rectangle&						rBounds,
                                       int										nStepCount,
                                       bool										bFillNonOverlapping )
        {
            fillGeneralLinearGradient( rOutDev,
                                       rTextureTransform,
                                       rBounds,
                                       nStepCount,
                                       rColor1,
                                       rColor2,
                                       bFillNonOverlapping,
                                       true );
        }

        void fillPolygonalGradient( OutputDevice&                                  rOutDev,
                                    const ::canvas::ParametricPolyPolygon::Values& rValues,
                                    const ::Color&                                 rColor1,
                                    const ::Color&                                 rColor2, 
                                    const ::basegfx::B2DHomMatrix&                 rTextureTransform,
                                    const ::Rectangle&                             rBounds,
                                    int                                            nStepCount,
                                    bool                                           bFillNonOverlapping )
        {
            const ::basegfx::B2DPolygon& rGradientPoly( rValues.maGradientPoly );

            ENSURE_OR_THROW( rGradientPoly.count() > 2,
                              "fillPolygonalGradient(): polygon without area given" );

            // For performance reasons, we create a temporary VCL polygon
            // here, keep it all the way and only change the vertex values
            // in the loop below (as ::Polygon is a pimpl class, creating
            // one every loop turn would really stress the mem allocator)
            ::basegfx::B2DPolygon 	aOuterPoly( rGradientPoly );
            ::basegfx::B2DPolygon 	aInnerPoly;

            // subdivide polygon _before_ rendering, would otherwise have
            // to be performed on every loop turn.
            if( aOuterPoly.areControlPointsUsed() )
                aOuterPoly = ::basegfx::tools::adaptiveSubdivideByAngle(aOuterPoly);

            aInnerPoly = aOuterPoly;
        
            // only transform outer polygon _after_ copying it into
            // aInnerPoly, because inner polygon has to be scaled before
            // the actual texture transformation takes place
            aOuterPoly.transform( rTextureTransform );

            // determine overall transformation for inner polygon (might
            // have to be prefixed by anisotrophic scaling)
            ::basegfx::B2DHomMatrix aInnerPolygonTransformMatrix;

        
            // apply scaling (possibly anisotrophic) to inner polygon
            // ------------------------------------------------------

            // move center of scaling to origin
            aInnerPolygonTransformMatrix.translate( -0.5, -0.5 );
        
            // scale inner polygon according to aspect ratio: for
            // wider-than-tall bounds (nAspectRatio > 1.0), the inner
            // polygon, representing the gradient focus, must have
            // non-zero width. Specifically, a bound rect twice as wide as
            // tall has a focus polygon of half it's width.
            const double nAspectRatio( rValues.mnAspectRatio );
            if( nAspectRatio > 1.0 )
            {
                // width > height case
                aInnerPolygonTransformMatrix.scale( 1.0 - 1.0/nAspectRatio,
                                                    0.0 );
            }
            else if( nAspectRatio < 1.0 )
            {
                // width < height case
                aInnerPolygonTransformMatrix.scale( 0.0, 
                                                    1.0 - nAspectRatio );
            }
            else
            {
                // isotrophic case
                aInnerPolygonTransformMatrix.scale( 0.0, 0.0 );
            }

            // move origin back to former center of polygon
            aInnerPolygonTransformMatrix.translate( 0.5, 0.5 );

            // and finally, add texture transform to it.
            aInnerPolygonTransformMatrix *= rTextureTransform;

            // apply final matrix to polygon
            aInnerPoly.transform( aInnerPolygonTransformMatrix );
        

            const sal_Int32			nNumPoints( aOuterPoly.count() );
            ::Polygon				aTempPoly( static_cast<USHORT>(nNumPoints+1) );
        
            // increase number of steps by one: polygonal gradients have
            // the outermost polygon rendered in rColor2, and the
            // innermost in rColor1. The innermost polygon will never
            // have zero area, thus, we must divide the interval into
            // nStepCount+1 steps. For example, to create 3 steps:
            //
            // |                       |
            // |-------|-------|-------|
            // |                       |
            // 3       2       1       0 
            //
            // This yields 4 tick marks, where 0 is never attained (since
            // zero-area polygons typically don't display perceivable
            // color).
            ++nStepCount;

            if( !bFillNonOverlapping )
            {
                // fill background
                rOutDev.SetFillColor( rColor1 );
                rOutDev.DrawRect( rBounds );

                // render polygon
                // ==============

                for( int i=1,p; i<nStepCount; ++i )
                {
                    // lerp color
                    rOutDev.SetFillColor( 
                        Color( (UINT8)(((nStepCount - i)*rColor1.GetRed() + i*rColor2.GetRed())/nStepCount),
                               (UINT8)(((nStepCount - i)*rColor1.GetGreen() + i*rColor2.GetGreen())/nStepCount),
                               (UINT8)(((nStepCount - i)*rColor1.GetBlue() + i*rColor2.GetBlue())/nStepCount) ) );

                    // scale and render polygon, by interpolating between
                    // outer and inner polygon. 

                    // calc interpolation parameter in [0,1] range
                    const double nT( (nStepCount-i)/double(nStepCount) );
            
                    for( p=0; p<nNumPoints; ++p )
                    {
                        const ::basegfx::B2DPoint& rOuterPoint( aOuterPoly.getB2DPoint(p) );
                        const ::basegfx::B2DPoint& rInnerPoint( aInnerPoly.getB2DPoint(p) );

                        aTempPoly[(USHORT)p] = ::Point( 
                            basegfx::fround( (1.0-nT)*rInnerPoint.getX() + nT*rOuterPoint.getX() ),
                            basegfx::fround( (1.0-nT)*rInnerPoint.getY() + nT*rOuterPoint.getY() ) );
                    }

                    // close polygon explicitely
                    aTempPoly[(USHORT)p] = aTempPoly[0];

                    // TODO(P1): compare with vcl/source/gdi/outdev4.cxx,
                    // OutputDevice::ImplDrawComplexGradient(), there's a note
                    // that on some VDev's, rendering disjunct poly-polygons
                    // is faster!
                    rOutDev.DrawPolygon( aTempPoly );
                }
            }
            else
            {
                // render polygon
                // ==============

                // For performance reasons, we create a temporary VCL polygon
                // here, keep it all the way and only change the vertex values
                // in the loop below (as ::Polygon is a pimpl class, creating
                // one every loop turn would really stress the mem allocator)
                ::PolyPolygon			aTempPolyPoly;
                ::Polygon				aTempPoly2( static_cast<USHORT>(nNumPoints+1) );

                aTempPoly2[0] = rBounds.TopLeft();
                aTempPoly2[1] = rBounds.TopRight();
                aTempPoly2[2] = rBounds.BottomRight();
                aTempPoly2[3] = rBounds.BottomLeft();
                aTempPoly2[4] = rBounds.TopLeft();

                aTempPolyPoly.Insert( aTempPoly );
                aTempPolyPoly.Insert( aTempPoly2 );

                for( int i=0,p; i<nStepCount; ++i )
                {
                    // lerp color
                    rOutDev.SetFillColor( 
                        Color( (UINT8)(((nStepCount - i)*rColor1.GetRed() + i*rColor2.GetRed())/nStepCount),
                               (UINT8)(((nStepCount - i)*rColor1.GetGreen() + i*rColor2.GetGreen())/nStepCount),
                               (UINT8)(((nStepCount - i)*rColor1.GetBlue() + i*rColor2.GetBlue())/nStepCount) ) );

#if defined(VERBOSE) && OSL_DEBUG_LEVEL > 0        
                    if( i && !(i % 10) )
                        rOutDev.SetFillColor( COL_RED );
#endif

                    // scale and render polygon. Note that here, we
                    // calculate the inner polygon, which is actually the
                    // start of the _next_ color strip. Thus, i+1

                    // calc interpolation parameter in [0,1] range
                    const double nT( (nStepCount-i-1)/double(nStepCount) );
            
                    for( p=0; p<nNumPoints; ++p )
                    {
                        const ::basegfx::B2DPoint& rOuterPoint( aOuterPoly.getB2DPoint(p) );
                        const ::basegfx::B2DPoint& rInnerPoint( aInnerPoly.getB2DPoint(p) );

                        aTempPoly[(USHORT)p] = ::Point( 
                            basegfx::fround( (1.0-nT)*rInnerPoint.getX() + nT*rOuterPoint.getX() ),
                            basegfx::fround( (1.0-nT)*rInnerPoint.getY() + nT*rOuterPoint.getY() ) );
                    }

                    // close polygon explicitely
                    aTempPoly[(USHORT)p] = aTempPoly[0];

                    // swap inner and outer polygon
                    aTempPolyPoly.Replace( aTempPolyPoly.GetObject( 1 ), 0 );

                    if( i+1<nStepCount )
                    {
                        // assign new inner polygon. Note that with this
                        // formulation, the internal pimpl objects for both
                        // temp polygons and the polypolygon remain identical,
                        // minimizing heap accesses (only a Polygon wrapper
                        // object is freed and deleted twice during this swap).
                        aTempPolyPoly.Replace( aTempPoly, 1 );
                    }
                    else
                    {
                        // last, i.e. inner strip. Now, the inner polygon
                        // has zero area anyway, and to not leave holes in
                        // the gradient, finally render a simple polygon:
                        aTempPolyPoly.Remove( 1 );
                    }

                    rOutDev.DrawPolyPolygon( aTempPolyPoly );
                }
            }
        }

        void doGradientFill( OutputDevice&                                  rOutDev,
                             const ::canvas::ParametricPolyPolygon::Values&	rValues,
                             const ::Color&                                 rColor1,
                             const ::Color&                                 rColor2, 
                             const ::basegfx::B2DHomMatrix&                 rTextureTransform,
                             const ::Rectangle&                             rBounds,
                             int                                            nStepCount,
                             bool                                           bFillNonOverlapping )
        {
            switch( rValues.meType )
            {
                case ::canvas::ParametricPolyPolygon::GRADIENT_LINEAR:
                    fillLinearGradient( rOutDev,
                                        rColor1,
                                        rColor2,
                                        rTextureTransform,
                                        rBounds,
                                        nStepCount,
                                        bFillNonOverlapping );
                    break;
               
                case ::canvas::ParametricPolyPolygon::GRADIENT_AXIAL:
                    fillAxialGradient( rOutDev,
                                       rColor1,
                                       rColor2,
                                       rTextureTransform,
                                       rBounds,
                                       nStepCount,
                                       bFillNonOverlapping );
                    break;

                case ::canvas::ParametricPolyPolygon::GRADIENT_ELLIPTICAL:
                    // FALLTHROUGH intended
                case ::canvas::ParametricPolyPolygon::GRADIENT_RECTANGULAR:
                    fillPolygonalGradient( rOutDev,
                                           rValues,
                                           rColor1,
                                           rColor2,
                                           rTextureTransform,
                                           rBounds,
                                           nStepCount,
                                           bFillNonOverlapping );
                    break;

                default:
                    ENSURE_OR_THROW( false,
                                      "CanvasHelper::doGradientFill(): Unexpected case" );
            }
        }

        bool gradientFill( OutputDevice&                                   rOutDev,
                           OutputDevice*                                   p2ndOutDev,
                           const ::canvas::ParametricPolyPolygon::Values&  rValues,
                           const ::Color&                                  rColor1,
                           const ::Color&                                  rColor2, 
                           const PolyPolygon&                              rPoly,
                           const rendering::ViewState&                     viewState, 
                           const rendering::RenderState&                   renderState,
                           const rendering::Texture&                       texture,
                           int                                             nTransparency )
        {
            (void)nTransparency;

            // TODO(T2): It is maybe necessary to lock here, should
            // maGradientPoly someday cease to be const. But then, beware of
            // deadlocks, canvashelper calls this method with locked own
            // mutex.

            // calculate overall texture transformation (directly from
            // texture to device space).
            ::basegfx::B2DHomMatrix aMatrix;
            ::basegfx::B2DHomMatrix aTextureTransform;

            ::basegfx::unotools::homMatrixFromAffineMatrix( aTextureTransform, 
                                                            texture.AffineTransform );
            ::canvas::tools::mergeViewAndRenderTransform(aMatrix,
                                                         viewState,
                                                         renderState);
            aTextureTransform *= aMatrix; // prepend total view/render transformation

            // determine maximal bound rect of gradient-filled polygon
            const ::Rectangle aPolygonDeviceRectOrig( 
                rPoly.GetBoundRect() );

            // determine size of gradient in device coordinate system
            // (to e.g. determine sensible number of gradient steps)
            ::basegfx::B2DPoint aLeftTop( 0.0, 0.0 );
            ::basegfx::B2DPoint aLeftBottom( 0.0, 1.0 );
            ::basegfx::B2DPoint aRightTop( 1.0, 0.0 );
            ::basegfx::B2DPoint aRightBottom( 1.0, 1.0 );

            aLeftTop	*= aTextureTransform;
            aLeftBottom *= aTextureTransform;
            aRightTop 	*= aTextureTransform;
            aRightBottom*= aTextureTransform;


            // calc step size
            // --------------
            const int nColorSteps( 
                ::std::max( 
                    labs( rColor1.GetRed() - rColor2.GetRed() ),
                    ::std::max(                    
                        labs( rColor1.GetGreen() - rColor2.GetGreen() ),
                        labs( rColor1.GetBlue()  - rColor2.GetBlue() ) ) ) );

            // longest line in gradient bound rect
            const int nGradientSize( 
                static_cast<int>( 
                    ::std::max( 
                        ::basegfx::B2DVector(aRightBottom-aLeftTop).getLength(),
                        ::basegfx::B2DVector(aRightTop-aLeftBottom).getLength() ) + 1.0 ) );

            // typical number for pixel of the same color (strip size)
            const int nStripSize( nGradientSize < 50 ? 2 : 4 );

            // use at least three steps, and at utmost the number of color
            // steps
            const int nStepCount( 
                ::std::max( 
                    3,
                    ::std::min( 
                        nGradientSize / nStripSize,
                        nColorSteps ) ) );        

            rOutDev.SetLineColor(); 

            if( tools::isRectangle( rPoly ) )
            {
                // use optimized output path
                // -------------------------

                // this distinction really looks like a
                // micro-optimisation, but in fact greatly speeds up
                // especially complex gradients. That's because when using
                // clipping, we can output polygons instead of
                // poly-polygons, and don't have to output the gradient
                // twice for XOR

                rOutDev.Push( PUSH_CLIPREGION );
                rOutDev.IntersectClipRegion( aPolygonDeviceRectOrig );
                doGradientFill( rOutDev,
                                rValues,
                                rColor1,
                                rColor2,
                                aTextureTransform,
                                aPolygonDeviceRectOrig,
                                nStepCount,
                                false );
                rOutDev.Pop();

                if( p2ndOutDev )
                {
                    p2ndOutDev->Push( PUSH_CLIPREGION );
                    p2ndOutDev->IntersectClipRegion( aPolygonDeviceRectOrig );
                    doGradientFill( *p2ndOutDev,
                                    rValues,
                                    rColor1,
                                    rColor2,
                                    aTextureTransform,
                                    aPolygonDeviceRectOrig,
                                    nStepCount,
                                    false );
                    p2ndOutDev->Pop();
                }
            }
            else
            {
                // output gradient the hard way: XORing out the polygon
#ifdef USE_JAVA
                MapMode aVDevMap;
                VirtualDevice aVDev( rOutDev );
                if ( aVDev.SetOutputSizePixel( aPolygonDeviceRectOrig.GetSize() ) )
                {
                    aVDev.DrawOutDev( Point(), aPolygonDeviceRectOrig.GetSize(), aPolygonDeviceRectOrig.TopLeft(), aPolygonDeviceRectOrig.GetSize(), rOutDev );
                    rOutDev.Push( PUSH_CLIPREGION );
                    rOutDev.IntersectClipRegion( aPolygonDeviceRectOrig );
                    doGradientFill( rOutDev,
                                    rValues,
                                    rColor1,
                                    rColor2,
                                    aTextureTransform,
                                    aPolygonDeviceRectOrig,
                                    nStepCount,
                                    true );
                    rOutDev.Pop();
                    aVDev.Push( PUSH_RASTEROP );
                    aVDev.SetRasterOp( ROP_XOR );
                    aVDev.DrawOutDev( Point(), aPolygonDeviceRectOrig.GetSize(), aPolygonDeviceRectOrig.TopLeft(), aPolygonDeviceRectOrig.GetSize(), rOutDev );
                    aVDev.SetFillColor( COL_BLACK );
                    aVDev.SetRasterOp( ROP_0 );
                    aVDevMap.SetOrigin( Point( -aPolygonDeviceRectOrig.Left(), -aPolygonDeviceRectOrig.Top() ) );
                    aVDev.SetMapMode( aVDevMap );
                    aVDev.DrawPolyPolygon( rPoly );
                    aVDevMap.SetOrigin( Point() );
                    aVDev.SetMapMode( aVDevMap );
                    aVDev.Pop();
                    rOutDev.Push( PUSH_RASTEROP );
                    rOutDev.SetRasterOp( ROP_XOR );
                    rOutDev.DrawOutDev( aPolygonDeviceRectOrig.TopLeft(), aPolygonDeviceRectOrig.GetSize(), Point(), aPolygonDeviceRectOrig.GetSize(), aVDev );
                    rOutDev.Pop();
                    if( p2ndOutDev )
                    {
                        aVDev.SetLineColor();
                        aVDev.SetFillColor( COL_BLACK );
                        aVDev.DrawRect( Rectangle( Point(), aPolygonDeviceRectOrig.GetSize() ) );
                        aVDev.DrawOutDev( Point(), aPolygonDeviceRectOrig.GetSize(), aPolygonDeviceRectOrig.TopLeft(), aPolygonDeviceRectOrig.GetSize(), *p2ndOutDev );
                        p2ndOutDev->Push( PUSH_CLIPREGION );
                        p2ndOutDev->IntersectClipRegion( aPolygonDeviceRectOrig );
                        doGradientFill( *p2ndOutDev,
                                        rValues,
                                        rColor1,
                                        rColor2,
                                        aTextureTransform,
                                        aPolygonDeviceRectOrig,
                                        nStepCount,
                                        true );
                        p2ndOutDev->Pop();
                        aVDev.Push( PUSH_RASTEROP );
                        aVDev.SetRasterOp( ROP_XOR );
                        aVDev.DrawOutDev( Point(), aPolygonDeviceRectOrig.GetSize(), aPolygonDeviceRectOrig.TopLeft(), aPolygonDeviceRectOrig.GetSize(), *p2ndOutDev );
                        aVDev.SetFillColor( COL_BLACK );
                        aVDev.SetRasterOp( ROP_0 );
                        aVDevMap.SetOrigin( Point( -aPolygonDeviceRectOrig.Left(), -aPolygonDeviceRectOrig.Top() ) );
                        aVDev.SetMapMode( aVDevMap );
                        aVDev.DrawPolyPolygon( rPoly );
                        aVDevMap.SetOrigin( Point() );
                        aVDev.SetMapMode( aVDevMap );
                        aVDev.Pop();
                        p2ndOutDev->Push( PUSH_RASTEROP );
                        p2ndOutDev->SetRasterOp( ROP_XOR );
                        p2ndOutDev->DrawOutDev( aPolygonDeviceRectOrig.TopLeft(), aPolygonDeviceRectOrig.GetSize(), Point(), aPolygonDeviceRectOrig.GetSize(), aVDev );
                        p2ndOutDev->Pop();
                    }
                }
#else	// USE_JAVA
                rOutDev.Push( PUSH_RASTEROP );
                rOutDev.SetRasterOp( ROP_XOR );
                doGradientFill( rOutDev,
                                rValues,
                                rColor1,
                                rColor2,
                                aTextureTransform,
                                aPolygonDeviceRectOrig,
                                nStepCount,
                                true );
                rOutDev.SetFillColor( COL_BLACK );
                rOutDev.SetRasterOp( ROP_0 );
                rOutDev.DrawPolyPolygon( rPoly );
                rOutDev.SetRasterOp( ROP_XOR );
                doGradientFill( rOutDev,
                                rValues,
                                rColor1,
                                rColor2,
                                aTextureTransform,
                                aPolygonDeviceRectOrig,
                                nStepCount,
                                true );
                rOutDev.Pop();

                if( p2ndOutDev )
                {
                    p2ndOutDev->Push( PUSH_RASTEROP );
                    p2ndOutDev->SetRasterOp( ROP_XOR );
                    doGradientFill( *p2ndOutDev,
                                    rValues,
                                    rColor1,
                                    rColor2,
                                    aTextureTransform,
                                    aPolygonDeviceRectOrig,
                                    nStepCount,
                                    true );
                    p2ndOutDev->SetFillColor( COL_BLACK );
                    p2ndOutDev->SetRasterOp( ROP_0 );
                    p2ndOutDev->DrawPolyPolygon( rPoly );
                    p2ndOutDev->SetRasterOp( ROP_XOR );
                    doGradientFill( *p2ndOutDev,
                                    rValues,
                                    rColor1,
                                    rColor2,
                                    aTextureTransform,
                                    aPolygonDeviceRectOrig,
                                    nStepCount,
                                    true );
                    p2ndOutDev->Pop();
                }
#endif	// USE_JAVA
            }

#if defined(VERBOSE) && OSL_DEBUG_LEVEL > 0        
            {
                ::basegfx::B2DRectangle aRect(0.0, 0.0, 1.0, 1.0);
                ::basegfx::B2DRectangle aTextureDeviceRect;
                ::canvas::tools::calcTransformedRectBounds( aTextureDeviceRect, 
                                                            aRect, 
                                                            aTextureTransform );
                rOutDev.SetLineColor( COL_RED ); 
                rOutDev.SetFillColor();
                rOutDev.DrawRect( ::vcl::unotools::rectangleFromB2DRectangle( aTextureDeviceRect ) );

                rOutDev.SetLineColor( COL_BLUE ); 
                ::Polygon aPoly1( 
                    ::vcl::unotools::rectangleFromB2DRectangle( aRect ));
                ::basegfx::B2DPolygon aPoly2( aPoly1.getB2DPolygon() );
                aPoly2.transform( aTextureTransform );
                ::Polygon aPoly3( aPoly2 );
                rOutDev.DrawPolygon( aPoly3 );
            }
#endif

            return true;
        }
    }

    uno::Reference< rendering::XCachedPrimitive > CanvasHelper::fillTexturedPolyPolygon( const rendering::XCanvas* 							pCanvas, 
                                                                                         const uno::Reference< rendering::XPolyPolygon2D >& xPolyPolygon,
                                                                                         const rendering::ViewState& 						viewState,
                                                                                         const rendering::RenderState& 						renderState,
                                                                                         const uno::Sequence< rendering::Texture >& 		textures )
    {
        ENSURE_ARG_OR_THROW( xPolyPolygon.is(),
                         "CanvasHelper::fillPolyPolygon(): polygon is NULL");
        ENSURE_ARG_OR_THROW( textures.getLength(), 
                         "CanvasHelper::fillTexturedPolyPolygon: empty texture sequence");

        if( mpOutDev )
        {
            tools::OutDevStateKeeper aStateKeeper( mpProtectedOutDev );

            const int nTransparency( setupOutDevState( viewState, renderState, IGNORE_COLOR ) );
            PolyPolygon aPolyPoly( tools::mapPolyPolygon( 
                                       ::basegfx::unotools::b2DPolyPolygonFromXPolyPolygon2D(xPolyPolygon),
                                       viewState, renderState ) );

            // TODO(F1): Multi-texturing
            if( textures[0].Gradient.is() )
            {
                // try to cast XParametricPolyPolygon2D reference to
                // our implementation class.
                ::canvas::ParametricPolyPolygon* pGradient = 
                      dynamic_cast< ::canvas::ParametricPolyPolygon* >( textures[0].Gradient.get() );

                if( pGradient )
                {
                    // copy state from Gradient polypoly locally
                    // (given object might change!)
                    const ::canvas::ParametricPolyPolygon::Values& rValues(
                        pGradient->getValues() );

                    // TODO: use all the colors and place them on given positions/stops
                    const ::Color aColor1( 
                        ::vcl::unotools::stdColorSpaceSequenceToColor(
                            rValues.maColors [0] ) );
                    const ::Color aColor2( 
                        ::vcl::unotools::stdColorSpaceSequenceToColor(
                            rValues.maColors [rValues.maColors.getLength () - 1] ) );

                    // TODO(E1): Return value
                    // TODO(F1): FillRule
                    gradientFill( mpOutDev->getOutDev(),
                                  mp2ndOutDev.get() ? &mp2ndOutDev->getOutDev() : (OutputDevice*)NULL,
                                  rValues,
                                  aColor1,
                                  aColor2,
                                  aPolyPoly,
                                  viewState,
                                  renderState,
                                  textures[0],
                                  nTransparency );
                }
                else
                {
                    // TODO(F1): The generic case is missing here
                    ENSURE_OR_THROW( false,
                                      "CanvasHelper::fillTexturedPolyPolygon(): unknown parametric polygon encountered" );
                }
            }
            else if( textures[0].Bitmap.is() )
            {
                OSL_ENSURE( textures[0].RepeatModeX == rendering::TexturingMode::REPEAT &&
                            textures[0].RepeatModeY == rendering::TexturingMode::REPEAT,
                            "CanvasHelper::fillTexturedPolyPolygon(): VCL canvas cannot currently clamp textures." );
                    
                const geometry::IntegerSize2D aBmpSize( textures[0].Bitmap->getSize() );
                
                ENSURE_ARG_OR_THROW( aBmpSize.Width != 0 &&
                                 aBmpSize.Height != 0,
                                 "CanvasHelper::fillTexturedPolyPolygon(): zero-sized texture bitmap" );

                // determine maximal bound rect of texture-filled
                // polygon
                const ::Rectangle aPolygonDeviceRect( 
                    aPolyPoly.GetBoundRect() );


                // first of all, determine whether we have a
                // drawBitmap() in disguise
                // =========================================

                const bool bRectangularPolygon( tools::isRectangle( aPolyPoly ) );

                ::basegfx::B2DHomMatrix aTotalTransform;
                ::canvas::tools::mergeViewAndRenderTransform(aTotalTransform,
                                                             viewState,
                                                             renderState);
                ::basegfx::B2DHomMatrix aTextureTransform;
                ::basegfx::unotools::homMatrixFromAffineMatrix( aTextureTransform, 
                                                                textures[0].AffineTransform );
                
                aTotalTransform *= aTextureTransform;

                const ::basegfx::B2DRectangle aRect(0.0, 0.0, 1.0, 1.0);
                ::basegfx::B2DRectangle aTextureDeviceRect;
                ::canvas::tools::calcTransformedRectBounds( aTextureDeviceRect, 
                                                            aRect, 
                                                            aTotalTransform );

                const ::Rectangle aIntegerTextureDeviceRect( 
                    ::vcl::unotools::rectangleFromB2DRectangle( aTextureDeviceRect ) );

                if( bRectangularPolygon &&
                    aIntegerTextureDeviceRect == aPolygonDeviceRect )
                {
                    rendering::RenderState aLocalState( renderState );
                    ::canvas::tools::appendToRenderState(aLocalState,
                                                         aTextureTransform);
                    ::basegfx::B2DHomMatrix aScaleCorrection;
                    aScaleCorrection.scale( 1.0/aBmpSize.Width,
                                            1.0/aBmpSize.Height );
                    ::canvas::tools::appendToRenderState(aLocalState,
                                                         aScaleCorrection);

                    // need alpha modulation?
                    if( !::rtl::math::approxEqual( textures[0].Alpha,
                                                   1.0 ) )
                    {
                        // setup alpha modulation values
                        aLocalState.DeviceColor.realloc(4);
                        double* pColor = aLocalState.DeviceColor.getArray();
                        pColor[0] =
                        pColor[1] =
                        pColor[2] = 0.0;
                        pColor[3] = textures[0].Alpha;

                        return drawBitmapModulated( pCanvas, 
                                                    textures[0].Bitmap,
                                                    viewState,
                                                    aLocalState );
                    }
                    else
                    {
                        return drawBitmap( pCanvas, 
                                           textures[0].Bitmap,
                                           viewState,
                                           aLocalState );
                    }
                }
                else
                {
                    // No easy mapping to drawBitmap() - calculate
                    // texturing parameters
                    // ===========================================

                    BitmapEx aBmpEx( tools::bitmapExFromXBitmap( textures[0].Bitmap ) );

                    // scale down bitmap to [0,1]x[0,1] rect, as required
                    // from the XCanvas interface.
                    ::basegfx::B2DHomMatrix aScaling;
                    ::basegfx::B2DHomMatrix aPureTotalTransform; // pure view*render*texture transform
                    aScaling.scale( 1.0/aBmpSize.Width,
                                    1.0/aBmpSize.Height );

                    aTotalTransform = aTextureTransform * aScaling;
                    aPureTotalTransform = aTextureTransform;

                    // combine with view and render transform
                    ::basegfx::B2DHomMatrix aMatrix;
                    ::canvas::tools::mergeViewAndRenderTransform(aMatrix, viewState, renderState);
                    
                    // combine all three transformations into one
                    // global texture-to-device-space transformation
                    aTotalTransform *= aMatrix;
                    aPureTotalTransform *= aMatrix;

                    // analyze transformation, and setup an
                    // appropriate GraphicObject
                    ::basegfx::B2DVector aScale;
                    ::basegfx::B2DPoint  aOutputPos;
                    double				 nRotate;
                    double				 nShearX;
                    aTotalTransform.decompose( aScale, aOutputPos, nRotate, nShearX );
                
                    GraphicAttr 			aGrfAttr;
                    GraphicObjectSharedPtr 	pGrfObj;

                    if( ::basegfx::fTools::equalZero( nShearX ) )
                    {
                        // no shear, GraphicObject is enough (the
                        // GraphicObject only supports scaling, rotation
                        // and translation)

                        // setup GraphicAttr
                        aGrfAttr.SetMirrorFlags( 
                            ( aScale.getX() < 0.0 ? BMP_MIRROR_HORZ : 0 ) | 
                            ( aScale.getY() < 0.0 ? BMP_MIRROR_VERT : 0 ) );
                        aGrfAttr.SetRotation( static_cast< USHORT >(::basegfx::fround( nRotate*10.0 )) );

                        pGrfObj.reset( new GraphicObject( aBmpEx ) );
                    }
                    else
                    {
                        // complex transformation, use generic affine bitmap
                        // transformation
                        aBmpEx = tools::transformBitmap( aBmpEx,
                                                         aTotalTransform,
                                                         uno::Sequence< double >(),
                                                         tools::MODULATE_NONE);

                        pGrfObj.reset( new GraphicObject( aBmpEx ) );

                        // clear scale values, generated bitmap already
                        // contains scaling
                        aScale.setX( 0.0 ); aScale.setY( 0.0 );
                    }


                    // render texture tiled into polygon
                    // =================================

                    // calc device space direction vectors. We employ
                    // the followin approach for tiled output: the
                    // texture bitmap is output in texture space
                    // x-major order, i.e. tile neighbors in texture
                    // space x direction are rendered back-to-back in
                    // device coordinate space (after the full device
                    // transformation). Thus, the aNextTile* vectors
                    // denote the output position updates in device
                    // space, to get from one tile to the next.
                    ::basegfx::B2DVector aNextTileX( 1.0, 0.0 );
                    ::basegfx::B2DVector aNextTileY( 0.0, 1.0 );
                    aNextTileX *= aPureTotalTransform;
                    aNextTileY *= aPureTotalTransform;

                    ::basegfx::B2DHomMatrix aInverseTextureTransform( aPureTotalTransform );

                    ENSURE_ARG_OR_THROW( aInverseTextureTransform.isInvertible(),
                                     "CanvasHelper::fillTexturedPolyPolygon(): singular texture matrix" );

                    aInverseTextureTransform.invert();

                    // calc bound rect of extended texture area in
                    // device coordinates. Therefore, we first calc
                    // the area of the polygon bound rect in texture
                    // space. To maintain texture phase, this bound
                    // rect is then extended to integer coordinates
                    // (extended, because shrinking might leave some
                    // inner polygon areas unfilled).
                    // Finally, the bound rect is transformed back to
                    // device coordinate space, were we determine the
                    // start point from it.
                    ::basegfx::B2DRectangle aTextureSpacePolygonRect;
                    ::canvas::tools::calcTransformedRectBounds( aTextureSpacePolygonRect, 
                                                                ::vcl::unotools::b2DRectangleFromRectangle(
                                                                    aPolygonDeviceRect ), 
                                                                aInverseTextureTransform );

                    // calc left, top of extended polygon rect in
                    // texture space, create one-texture instance rect
                    // from it (i.e. rect from start point extending
                    // 1.0 units to the right and 1.0 units to the
                    // bottom). Note that the rounding employed here
                    // is a bit subtle, since we need to round up/down
                    // as _soon_ as any fractional amount is
                    // encountered. This is to ensure that the full
                    // polygon area is filled with texture tiles.
                    const sal_Int32 nX1( ::canvas::tools::roundDown( aTextureSpacePolygonRect.getMinX() ) );
                    const sal_Int32 nY1( ::canvas::tools::roundDown( aTextureSpacePolygonRect.getMinY() ) );
                    const sal_Int32 nX2( ::canvas::tools::roundUp( aTextureSpacePolygonRect.getMaxX() ) );
                    const sal_Int32 nY2( ::canvas::tools::roundUp( aTextureSpacePolygonRect.getMaxY() ) );
                    const ::basegfx::B2DRectangle aSingleTextureRect(
                        nX1, nY1,
                        nX1 + 1.0,
                        nY1 + 1.0 );

                    // and convert back to device space
                    ::basegfx::B2DRectangle aSingleDeviceTextureRect;
                    ::canvas::tools::calcTransformedRectBounds( aSingleDeviceTextureRect, 
                                                                aSingleTextureRect, 
                                                                aPureTotalTransform );
                    
                    const ::Point aPt( ::vcl::unotools::pointFromB2DPoint( 
                                           aSingleDeviceTextureRect.getMinimum() ) );
                    const ::Size  aSz( ::basegfx::fround( aScale.getX() * aBmpSize.Width ),
                                       ::basegfx::fround( aScale.getY() * aBmpSize.Height ) );
                    const ::Size  aIntegerNextTileX( ::vcl::unotools::sizeFromB2DSize(aNextTileX) );
                    const ::Size  aIntegerNextTileY( ::vcl::unotools::sizeFromB2DSize(aNextTileY) );

                    const sal_Int32 nTilesX( nX2 - nX1 );
                    const sal_Int32 nTilesY( nY2 - nY1 );

                    OutputDevice& rOutDev( mpOutDev->getOutDev() );

                    if( bRectangularPolygon )
                    {
                        // use optimized output path
                        // -------------------------

                        // this distinction really looks like a
                        // micro-optimisation, but in fact greatly speeds up
                        // especially complex fills. That's because when using
                        // clipping, we can output polygons instead of
                        // poly-polygons, and don't have to output the gradient
                        // twice for XOR

                        // setup alpha modulation
                        if( !::rtl::math::approxEqual( textures[0].Alpha,
                                                       1.0 ) )
                        {
                            // TODO(F1): Note that the GraphicManager has
                            // a subtle difference in how it calculates
                            // the resulting alpha value: it's using the
                            // inverse alpha values (i.e. 'transparency'),
                            // and calculates transOrig + transModulate,
                            // instead of transOrig + transModulate -
                            // transOrig*transModulate (which would be
                            // equivalent to the origAlpha*modulateAlpha
                            // the DX canvas performs)
                            aGrfAttr.SetTransparency( 
                                static_cast< BYTE >( 
                                    ::basegfx::fround( 255.0*( 1.0 - textures[0].Alpha ) ) ) );
                        }

                        rOutDev.IntersectClipRegion( aPolygonDeviceRect );
                        textureFill( rOutDev,
                                     *pGrfObj,
                                     aPt,
                                     aIntegerNextTileX,
                                     aIntegerNextTileY,
                                     nTilesX,
                                     nTilesY,
                                     aSz,
                                     aGrfAttr );

                        if( mp2ndOutDev )
                        {
                            OutputDevice& r2ndOutDev( mp2ndOutDev->getOutDev() );
                            r2ndOutDev.IntersectClipRegion( aPolygonDeviceRect );
                            textureFill( r2ndOutDev,
                                         *pGrfObj,
                                         aPt,
                                         aIntegerNextTileX,
                                         aIntegerNextTileY,
                                         nTilesX,
                                         nTilesY,
                                         aSz,
                                         aGrfAttr );
                        }
                    }
                    else
                    {
                        // output texture the hard way: XORing out the
                        // polygon
                        // ===========================================

                        if( !::rtl::math::approxEqual( textures[0].Alpha,
                                                       1.0 ) )
                        {
                            // uh-oh. alpha blending is required,
                            // cannot do direct XOR, but have to
                            // prepare the filled polygon within a
                            // VDev
                            VirtualDevice aVDev( rOutDev );
                            aVDev.SetOutputSizePixel( aPolygonDeviceRect.GetSize() );

                            // shift output to origin of VDev
                            const ::Point aOutPos( aPt - aPolygonDeviceRect.TopLeft() );
                            aPolyPoly.Translate( ::Point( -aPolygonDeviceRect.Left(),
                                                          -aPolygonDeviceRect.Top() ) );

                            aVDev.SetRasterOp( ROP_XOR );
                            textureFill( aVDev,
                                         *pGrfObj,
                                         aOutPos,
                                         aIntegerNextTileX,
                                         aIntegerNextTileY,
                                         nTilesX,
                                         nTilesY,
                                         aSz,
                                         aGrfAttr );
                            aVDev.SetFillColor( COL_BLACK );
                            aVDev.SetRasterOp( ROP_0 );
                            aVDev.DrawPolyPolygon( aPolyPoly );
                            aVDev.SetRasterOp( ROP_XOR );
                            textureFill( aVDev,
                                         *pGrfObj,
                                         aOutPos,
                                         aIntegerNextTileX,
                                         aIntegerNextTileY,
                                         nTilesX,
                                         nTilesY,
                                         aSz,
                                         aGrfAttr );

                            // output VDev content alpha-blended to
                            // target position.
                            const ::Point aEmptyPoint;
                            Bitmap aContentBmp( 
                                aVDev.GetBitmap( aEmptyPoint, 
                                                 aVDev.GetOutputSizePixel() ) );

                            BYTE nCol( static_cast< BYTE >( 
                                           ::basegfx::fround( 255.0*( 1.0 - textures[0].Alpha ) ) ) );
                            AlphaMask aAlpha( aVDev.GetOutputSizePixel(),
                                              &nCol );

                            BitmapEx aOutputBmpEx( aContentBmp, aAlpha );
                            rOutDev.DrawBitmapEx( aPolygonDeviceRect.TopLeft(),
                                                  aOutputBmpEx );

                            if( mp2ndOutDev )
                                mp2ndOutDev->getOutDev().DrawBitmapEx( aPolygonDeviceRect.TopLeft(),
                                                                       aOutputBmpEx );
                        }
                        else
                        {
                            // output via repeated XORing
                            rOutDev.Push( PUSH_RASTEROP );
                            rOutDev.SetRasterOp( ROP_XOR );
                            textureFill( rOutDev,
                                         *pGrfObj,
                                         aPt,
                                         aIntegerNextTileX,
                                         aIntegerNextTileY,
                                         nTilesX,
                                         nTilesY,
                                         aSz,
                                         aGrfAttr );
                            rOutDev.SetFillColor( COL_BLACK );
                            rOutDev.SetRasterOp( ROP_0 );
                            rOutDev.DrawPolyPolygon( aPolyPoly );
                            rOutDev.SetRasterOp( ROP_XOR );
                            textureFill( rOutDev,
                                         *pGrfObj,
                                         aPt,
                                         aIntegerNextTileX,
                                         aIntegerNextTileY,
                                         nTilesX,
                                         nTilesY,
                                         aSz,
                                         aGrfAttr );
                            rOutDev.Pop();

                            if( mp2ndOutDev )
                            {
                                OutputDevice& r2ndOutDev( mp2ndOutDev->getOutDev() );
                                r2ndOutDev.Push( PUSH_RASTEROP );
                                r2ndOutDev.SetRasterOp( ROP_XOR );
                                textureFill( r2ndOutDev,
                                             *pGrfObj,
                                             aPt,
                                             aIntegerNextTileX,
                                             aIntegerNextTileY,
                                             nTilesX,
                                             nTilesY,
                                             aSz,
                                             aGrfAttr );
                                r2ndOutDev.SetFillColor( COL_BLACK );
                                r2ndOutDev.SetRasterOp( ROP_0 );
                                r2ndOutDev.DrawPolyPolygon( aPolyPoly );
                                r2ndOutDev.SetRasterOp( ROP_XOR );
                                textureFill( r2ndOutDev,
                                             *pGrfObj,
                                             aPt,
                                             aIntegerNextTileX,
                                             aIntegerNextTileY,
                                             nTilesX,
                                             nTilesY,
                                             aSz,
                                             aGrfAttr );
                                r2ndOutDev.Pop();
                            }
                        }
                    }
                }
            }
        }

        // TODO(P1): Provide caching here.
        return uno::Reference< rendering::XCachedPrimitive >(NULL);
    }

}
