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
 * Modified March 2010 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svx.hxx"

#include "viewcontactoftableobj.hxx"
#include <svx/svdotable.hxx>
#include <com/sun/star/table/XTable.hpp>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <drawinglayer/primitive2d/polygonprimitive2d.hxx>
#include <drawinglayer/attribute/sdrattribute.hxx>
#include <svx/sdr/primitive2d/sdrattributecreator.hxx>
#include <drawinglayer/primitive2d/groupprimitive2d.hxx>
#include <svx/sdr/primitive2d/sdrdecompositiontools.hxx>
#include <drawinglayer/attribute/sdrattribute.hxx>
#include <svx/sdr/primitive2d/sdrattributecreator.hxx>
#include <drawinglayer/attribute/fillattribute.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <svx/sdr/attribute/sdrtextattribute.hxx>
#include <svx/sdr/attribute/sdrallattribute.hxx>
#include <svx/sdr/primitive2d/svx_primitivetypes2d.hxx>
#include <svx/borderline.hxx>
#include <drawinglayer/primitive2d/borderlineprimitive2d.hxx>

#include "cell.hxx"
#include "tablelayouter.hxx"

#ifdef USE_JAVA

#include <vcl/svapp.hxx>
#include "tablecontroller.hxx"

#endif	// USE_JAVA

//////////////////////////////////////////////////////////////////////////////

using namespace com::sun::star;

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		class SdrCellPrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DHomMatrix						maTransform;
			attribute::SdrFillTextAttribute				maSdrFTAttribute;
#ifdef USE_JAVA
			bool										mbUseNativeHighlighting;
#endif	// USE_JAVA

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& aViewInformation) const;

		public:
			SdrCellPrimitive2D(
				const basegfx::B2DHomMatrix& rTransform, 
#ifdef USE_JAVA
				const attribute::SdrFillTextAttribute& rSdrFTAttribute, bool bUseNativeHighlighting)
#else	// USE_JAVA
				const attribute::SdrFillTextAttribute& rSdrFTAttribute)
#endif	// USE_JAVA
			:	BasePrimitive2D(),
				maTransform(rTransform),
				maSdrFTAttribute(rSdrFTAttribute)
#ifdef USE_JAVA
				, mbUseNativeHighlighting(bUseNativeHighlighting)
#endif	// USE_JAVA
			{
			}

			// data access
			const basegfx::B2DHomMatrix& getTransform() const { return maTransform; }
			const attribute::SdrFillTextAttribute& getSdrFTAttribute() const { return maSdrFTAttribute; }

#ifdef USE_JAVA
			bool UseNativeHighlighting() const { return mbUseNativeHighlighting; }
#endif	// USE_JAVA

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};

		Primitive2DSequence SdrCellPrimitive2D::createLocalDecomposition(const geometry::ViewInformation2D& /*aViewInformation*/) const
		{
			Primitive2DSequence aRetval;

			if(getSdrFTAttribute().getFill() || getSdrFTAttribute().getText())
			{
				// prepare unit polygon
				const basegfx::B2DRange aUnitRange(0.0, 0.0, 1.0, 1.0);
				const basegfx::B2DPolyPolygon aUnitPolyPolygon(basegfx::tools::createPolygonFromRect(aUnitRange));

				// add fill
				if(getSdrFTAttribute().getFill())
				{
					appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, createPolyPolygonFillPrimitive(
						aUnitPolyPolygon, 
						getTransform(), 
						*getSdrFTAttribute().getFill(), 
						getSdrFTAttribute().getFillFloatTransGradient()));
				}

#ifdef USE_JAVA
				// Add native highlighting
				if(UseNativeHighlighting())
				{
					attribute::SdrFillAttribute aNativeHighlightFillAttr(0.25, Application::GetSettings().GetStyleSettings().GetHighlightColor().getBColor());
					appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, createPolyPolygonFillPrimitive(
						aUnitPolyPolygon, 
						getTransform(), 
						aNativeHighlightFillAttr,
						NULL));
				}
#endif	// USE_JAVA

				// add text
				if(getSdrFTAttribute().getText())
				{
					appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, createTextPrimitive(
						aUnitPolyPolygon, 
						getTransform(), 
						*getSdrFTAttribute().getText(),
						0,
						true, false));
				}
			}

			return aRetval;
		}

		bool SdrCellPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
		{
			if(BasePrimitive2D::operator==(rPrimitive))
			{
				const SdrCellPrimitive2D& rCompare = (SdrCellPrimitive2D&)rPrimitive;
				
				return (getTransform() == rCompare.getTransform()
#ifdef USE_JAVA
					&& UseNativeHighlighting() == rCompare.UseNativeHighlighting()
#endif	// USE_JAVA
					&& getSdrFTAttribute() == rCompare.getSdrFTAttribute());
			}

			return false;
		}

		// provide unique ID
		ImplPrimitrive2DIDBlock(SdrCellPrimitive2D, PRIMITIVE2D_ID_SDRCELLPRIMITIVE2D)

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		class SdrBorderlinePrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DHomMatrix						maTransform;
			SvxBorderLine								maLeftLine;
			SvxBorderLine								maBottomLine;
			SvxBorderLine								maRightLine;
			SvxBorderLine								maTopLine;

			// bitfield
			unsigned									mbLeftIsOutside : 1;
			unsigned									mbBottomIsOutside : 1;
			unsigned									mbRightIsOutside : 1;
			unsigned									mbTopIsOutside : 1;
			unsigned									mbInTwips : 1;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& aViewInformation) const;

		public:
			SdrBorderlinePrimitive2D(
				const basegfx::B2DHomMatrix& rTransform, 
				const SvxBorderLine& rLeftLine,
				const SvxBorderLine& rBottomLine,
				const SvxBorderLine& rRightLine,
				const SvxBorderLine& rTopLine,
				bool bLeftIsOutside,
				bool bBottomIsOutside,
				bool bRightIsOutside,
				bool bTopIsOutside,
				bool bInTwips)
			:	BasePrimitive2D(),
				maTransform(rTransform),
				maLeftLine(rLeftLine),
				maBottomLine(rBottomLine),
				maRightLine(rRightLine),
				maTopLine(rTopLine),
				mbLeftIsOutside(bLeftIsOutside),
				mbBottomIsOutside(bBottomIsOutside),
				mbRightIsOutside(bRightIsOutside),
				mbTopIsOutside(bTopIsOutside),
				mbInTwips(bInTwips)
			{
			}


			// data access
			const basegfx::B2DHomMatrix& getTransform() const { return maTransform; }
			const SvxBorderLine& getLeftLine() const { return maLeftLine; }
			const SvxBorderLine& getBottomLine() const { return maBottomLine; }
			const SvxBorderLine& getRightLine() const { return maRightLine; }
			const SvxBorderLine& getTopLine() const { return maTopLine; }
			bool getLeftIsOutside() const { return mbLeftIsOutside; }
			bool getBottomIsOutside() const { return mbBottomIsOutside; }
			bool getRightIsOutside() const { return mbRightIsOutside; }
			bool getTopIsOutside() const { return mbTopIsOutside; }
			bool getInTwips() const { return mbInTwips; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};

		sal_uInt16 getBorderLineOutWidth(const SvxBorderLine& rLineA)
		{
			return (1 == rLineA.GetOutWidth() ? 0 : rLineA.GetOutWidth());
		}

		sal_uInt16 getBorderLineDistance(const SvxBorderLine& rLineA)
		{
			return (1 == rLineA.GetDistance() ? 0 : rLineA.GetDistance());
		}

		sal_uInt16 getBorderLineInWidth(const SvxBorderLine& rLineA)
		{
			return (1 == rLineA.GetInWidth() ? 0 : rLineA.GetInWidth());
		}

		sal_uInt16 getBorderLineWidth(const SvxBorderLine& rLineA)
		{
			return getBorderLineOutWidth(rLineA) + getBorderLineDistance(rLineA) + getBorderLineInWidth(rLineA);
		}

		double getInnerExtend(const SvxBorderLine& rLineA, bool bSideToUse)
		{
			if(!rLineA.isEmpty())
			{
				if(rLineA.isDouble())
				{
					// reduce to inner edge of associated matching line
					return -((getBorderLineWidth(rLineA) / 2.0) - (bSideToUse ? getBorderLineOutWidth(rLineA) : getBorderLineInWidth(rLineA)));
				}
				else
				{
					// extend to overlap with single line
					return getBorderLineWidth(rLineA) / 2.0;
				}
			}

			return 0.0;
		}

		double getOuterExtend(const SvxBorderLine& rLineA)
		{
			if(!rLineA.isEmpty())
			{
				// extend to overlap with single line
				return getBorderLineWidth(rLineA) / 2.0;
			}

			return 0.0;
		}

		double getChangedValue(sal_uInt16 nValue, bool bChangeToMM)
		{
			if(1 == nValue)
				return 1.0;

			if(bChangeToMM)
				return nValue * (127.0 / 72.0);

			return (double)nValue;
		}

		Primitive2DSequence SdrBorderlinePrimitive2D::createLocalDecomposition(const geometry::ViewInformation2D& /*aViewInformation*/) const
		{
			Primitive2DSequence xRetval(4);
			sal_uInt32 nInsert(0);
			const double fTwipsToMM(getInTwips() ? (127.0 / 72.0) : 1.0);

			if(!getLeftLine().isEmpty())
			{
				// create left line from top to bottom
				const basegfx::B2DPoint aStart(getTransform() * basegfx::B2DPoint(0.0, 0.0));
				const basegfx::B2DPoint aEnd(getTransform() * basegfx::B2DPoint(0.0, 1.0));

				if(!aStart.equal(aEnd))
				{
					const double fExtendIS(getInnerExtend(getTopLine(), false));
					const double fExtendIE(getInnerExtend(getBottomLine(), true));
					double fExtendOS(0.0);
					double fExtendOE(0.0);

					if(getLeftIsOutside())
					{
						if(getTopIsOutside())
						{
							fExtendOS = getOuterExtend(getTopLine());
						}

						if(getBottomIsOutside())
						{
							fExtendOE = getOuterExtend(getBottomLine());
						}
					}

					xRetval[nInsert++] = Primitive2DReference(new BorderLinePrimitive2D(
						aStart,
						aEnd,
						getChangedValue(getLeftLine().GetOutWidth(), getInTwips()),
						getChangedValue(getLeftLine().GetDistance(), getInTwips()),
						getChangedValue(getLeftLine().GetInWidth(), getInTwips()),
						fExtendIS * fTwipsToMM,
						fExtendIE * fTwipsToMM,
						fExtendOS * fTwipsToMM,
						fExtendOE * fTwipsToMM,
						true,
						getLeftIsOutside(),
						getLeftLine().GetColor().getBColor()));
				}
			}

			if(!getBottomLine().isEmpty())
			{
				// create bottom line from left to right
				const basegfx::B2DPoint aStart(getTransform() * basegfx::B2DPoint(0.0, 1.0));
				const basegfx::B2DPoint aEnd(getTransform() * basegfx::B2DPoint(1.0, 1.0));

				if(!aStart.equal(aEnd))
				{
					const double fExtendIS(getInnerExtend(getLeftLine(), true));
					const double fExtendIE(getInnerExtend(getRightLine(), false));
					double fExtendOS(0.0);
					double fExtendOE(0.0);

					if(getBottomIsOutside())
					{
						if(getLeftIsOutside())
						{
							fExtendOS = getOuterExtend(getLeftLine());
						}

						if(getRightIsOutside())
						{
							fExtendOE = getOuterExtend(getRightLine());
						}
					}

					xRetval[nInsert++] = Primitive2DReference(new BorderLinePrimitive2D(
						aStart,
						aEnd,
						getChangedValue(getBottomLine().GetOutWidth(), getInTwips()),
						getChangedValue(getBottomLine().GetDistance(), getInTwips()),
						getChangedValue(getBottomLine().GetInWidth(), getInTwips()),
						fExtendIS * fTwipsToMM,
						fExtendIE * fTwipsToMM,
						fExtendOS * fTwipsToMM,
						fExtendOE * fTwipsToMM,
						true,
						getBottomIsOutside(),
						getBottomLine().GetColor().getBColor()));
				}
			}

			if(!getRightLine().isEmpty())
			{
				// create right line from top to bottom
				const basegfx::B2DPoint aStart(getTransform() * basegfx::B2DPoint(1.0, 0.0));
				const basegfx::B2DPoint aEnd(getTransform() * basegfx::B2DPoint(1.0, 1.0));

				if(!aStart.equal(aEnd))
				{
					const double fExtendIS(getInnerExtend(getTopLine(), false));
					const double fExtendIE(getInnerExtend(getBottomLine(), true));
					double fExtendOS(0.0);
					double fExtendOE(0.0);

					if(getRightIsOutside())
					{
						if(getTopIsOutside())
						{
							fExtendOS = getOuterExtend(getTopLine());
						}

						if(getBottomIsOutside())
						{
							fExtendOE = getOuterExtend(getBottomLine());
						}
					}

					xRetval[nInsert++] = Primitive2DReference(new BorderLinePrimitive2D(
						aStart,
						aEnd,
						getChangedValue(getRightLine().GetOutWidth(), getInTwips()),
						getChangedValue(getRightLine().GetDistance(), getInTwips()),
						getChangedValue(getRightLine().GetInWidth(), getInTwips()),
						fExtendOS * fTwipsToMM,
						fExtendOE * fTwipsToMM,
						fExtendIS * fTwipsToMM,
						fExtendIE * fTwipsToMM,
						getRightIsOutside(),
						true,
						getRightLine().GetColor().getBColor()));
				}
			}

			if(!getTopLine().isEmpty())
			{
				// create top line from left to right
				const basegfx::B2DPoint aStart(getTransform() * basegfx::B2DPoint(0.0, 0.0));
				const basegfx::B2DPoint aEnd(getTransform() * basegfx::B2DPoint(1.0, 0.0));

				if(!aStart.equal(aEnd))
				{
					const double fExtendIS(getInnerExtend(getLeftLine(), true));
					const double fExtendIE(getInnerExtend(getRightLine(), false));
					double fExtendOS(0.0);
					double fExtendOE(0.0);

					if(getTopIsOutside())
					{
						if(getLeftIsOutside())
						{
							fExtendOS = getOuterExtend(getLeftLine());
						}

						if(getRightIsOutside())
						{
							fExtendOE = getOuterExtend(getRightLine());
						}
					}

					xRetval[nInsert++] = Primitive2DReference(new BorderLinePrimitive2D(
						aStart,
						aEnd,
						getChangedValue(getTopLine().GetOutWidth(), getInTwips()),
						getChangedValue(getTopLine().GetDistance(), getInTwips()),
						getChangedValue(getTopLine().GetInWidth(), getInTwips()),
						fExtendOS * fTwipsToMM,
						fExtendOE * fTwipsToMM,
						fExtendIS * fTwipsToMM,
						fExtendIE * fTwipsToMM,
						getTopIsOutside(),
						true,
						getTopLine().GetColor().getBColor()));
				}
			}

			xRetval.realloc(nInsert);
			return xRetval;
		}

		bool SdrBorderlinePrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
		{
			if(BasePrimitive2D::operator==(rPrimitive))
			{
				const SdrBorderlinePrimitive2D& rCompare = (SdrBorderlinePrimitive2D&)rPrimitive;
				
				return (getTransform() == rCompare.getTransform()
					&& getLeftLine() == rCompare.getLeftLine()
					&& getBottomLine() == rCompare.getBottomLine()
					&& getRightLine() == rCompare.getRightLine()
					&& getTopLine() == rCompare.getTopLine()
					&& getLeftIsOutside() == rCompare.getLeftIsOutside()
					&& getBottomIsOutside() == rCompare.getBottomIsOutside()
					&& getRightIsOutside() == rCompare.getRightIsOutside()
					&& getTopIsOutside() == rCompare.getTopIsOutside()
					&& getInTwips() == rCompare.getInTwips());
			}

			return false;
		}

		// provide unique ID
		ImplPrimitrive2DIDBlock(SdrBorderlinePrimitive2D, PRIMITIVE2D_ID_SDRBORDERLINEPRIMITIVE2D)

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace sdr
{
	namespace contact
	{
		void impGetLine(SvxBorderLine& aLine, const sdr::table::TableLayouter& rLayouter, sal_Int32 nX, sal_Int32 nY, bool bHorizontal, sal_Int32 nColCount, sal_Int32 nRowCount, bool bIsRTL)
		{
			if(nX >= 0 && nX <= nColCount && nY >= 0 && nY <= nRowCount)
			{
				const SvxBorderLine* pLine = rLayouter.getBorderLine(nX, nY, bHorizontal);

				if(pLine)
				{
					// copy line content
					aLine = *pLine;

					// check for mirroring. This shall always be done when it is
					// not a top- or rightmost line
					bool bMirror(aLine.isDouble());

					if(bMirror)
					{
						if(bHorizontal)
						{
							// mirror all bottom lines
							bMirror = (0 != nY);
						}
						else
						{
							// mirror all left lines
							bMirror = (bIsRTL ? 0 != nX : nX != nColCount);
						}
					}
						
					if(bMirror)
					{
						aLine.SetOutWidth(pLine->GetInWidth());
						aLine.SetInWidth(pLine->GetOutWidth());
					}

					return;
				}
			}

			// no success, copy empty line
			const SvxBorderLine aEmptyLine;
			aLine = aEmptyLine;
		}

		drawinglayer::primitive2d::Primitive2DSequence ViewContactOfTableObj::createViewIndependentPrimitive2DSequence() const
        {
            drawinglayer::primitive2d::Primitive2DSequence xRetval;
			const sdr::table::SdrTableObj& rTableObj = GetTableObj();
            const uno::Reference< com::sun::star::table::XTable > xTable = rTableObj.getTable();
           	const SfxItemSet& rObjectItemSet = rTableObj.GetMergedItemSet();

            if(xTable.is())
            {
                // create primitive representation for table
                const sal_Int32 nRowCount(xTable->getRowCount());
                const sal_Int32 nColCount(xTable->getColumnCount());
                const sal_Int32 nAllCount(nRowCount * nColCount);

                if(nAllCount)
                {
					const sdr::table::TableLayouter& rTableLayouter = rTableObj.getTableLayouter();
					const bool bIsRTL(com::sun::star::text::WritingMode_RL_TB == rTableObj.GetWritingMode());
                	sdr::table::CellPos aCellPos;
					sdr::table::CellRef xCurrentCell;
					basegfx::B2IRectangle aCellArea;

		            // create range using the model data directly. This is in SdrTextObj::aRect which i will access using
                    // GetGeoRect() to not trigger any calculations. It's the unrotated geometry.
		            const Rectangle& rObjectRectangle(rTableObj.GetGeoRect());
		            const basegfx::B2DRange aObjectRange(rObjectRectangle.Left(), rObjectRectangle.Top(), rObjectRectangle.Right(), rObjectRectangle.Bottom());

					// for each cell we need potentially a cell primitive and a border primitive
					// (e.g. single cell). Prepare sequences and input counters
					drawinglayer::primitive2d::Primitive2DSequence xCellSequence(nAllCount);
					drawinglayer::primitive2d::Primitive2DSequence xBorderSequence(nAllCount);
					sal_uInt32 nCellInsert(0);
					sal_uInt32 nBorderInsert(0);
	                
					// variables for border lines
					SvxBorderLine aLeftLine;
					SvxBorderLine aBottomLine;
					SvxBorderLine aRightLine;
					SvxBorderLine aTopLine;
					
					// create single primitives per cell
					for(aCellPos.mnRow = 0; aCellPos.mnRow < nRowCount; aCellPos.mnRow++)
                    {
		                for(aCellPos.mnCol = 0; aCellPos.mnCol < nColCount; aCellPos.mnCol++)
                        {
							xCurrentCell.set(dynamic_cast< sdr::table::Cell* >(xTable->getCellByPosition(aCellPos.mnCol, aCellPos.mnRow).get()));

							if(xCurrentCell.is() && !xCurrentCell->isMerged())
							{
								if(rTableLayouter.getCellArea(aCellPos, aCellArea))
								{
									// create cell transformation matrix
									basegfx::B2DHomMatrix aCellMatrix;
									aCellMatrix.set(0, 0, (double)aCellArea.getWidth());
									aCellMatrix.set(1, 1, (double)aCellArea.getHeight());
									aCellMatrix.set(0, 2, (double)aCellArea.getMinX() + aObjectRange.getMinX());
									aCellMatrix.set(1, 2, (double)aCellArea.getMinY() + aObjectRange.getMinY());

									// handle cell fillings and text
									const SfxItemSet& rCellItemSet = xCurrentCell->GetItemSet();
									const sal_uInt32 nTextIndex(nColCount * aCellPos.mnRow + aCellPos.mnCol);
									const SdrText* pSdrText = rTableObj.getText(nTextIndex);
									drawinglayer::attribute::SdrFillTextAttribute* pAttribute = drawinglayer::primitive2d::createNewSdrFillTextAttribute(rCellItemSet, pSdrText);

									if(pAttribute)
									{
										if(pAttribute->isVisible())
										{
											const drawinglayer::primitive2d::Primitive2DReference xCellReference(new drawinglayer::primitive2d::SdrCellPrimitive2D(
#ifdef USE_JAVA
												aCellMatrix, *pAttribute, rTableObj.mpTableController ? rTableObj.mpTableController->IsNativeHighlightColorCellPos(aCellPos) : false));
#else	// USE_JAVA
												aCellMatrix, *pAttribute));
#endif	// USE_JAVA
											xCellSequence[nCellInsert++] = xCellReference;
										}

										delete pAttribute;
									}

									// handle cell borders
									const sal_Int32 nX(bIsRTL ? nColCount - aCellPos.mnCol : aCellPos.mnCol);
									const sal_Int32 nY(aCellPos.mnRow);

									// get access values for X,Y at the cell's end
									const sal_Int32 nXSpan(xCurrentCell->getColumnSpan());
									const sal_Int32 nYSpan(xCurrentCell->getRowSpan());
									const sal_Int32 nXRight(bIsRTL ? nX - nXSpan : nX + nXSpan);
									const sal_Int32 nYBottom(nY + nYSpan);
									
									// get basic lines
									impGetLine(aLeftLine, rTableLayouter, nX, nY, false, nColCount, nRowCount, bIsRTL);
									impGetLine(aBottomLine, rTableLayouter, nX, nYBottom, true, nColCount, nRowCount, bIsRTL);
									impGetLine(aRightLine, rTableLayouter, nXRight, nY, false, nColCount, nRowCount, bIsRTL);
									impGetLine(aTopLine, rTableLayouter, nX, nY, true, nColCount, nRowCount, bIsRTL);
									
									// create the primtive containing all data for one cell with borders
									xBorderSequence[nBorderInsert++] = drawinglayer::primitive2d::Primitive2DReference(
										new drawinglayer::primitive2d::SdrBorderlinePrimitive2D(
											aCellMatrix,
											aLeftLine, 
											aBottomLine, 
											aRightLine, 
											aTopLine,
											bIsRTL ? nX == nColCount : 0 == nX,
											nRowCount == nYBottom,
											bIsRTL ? 0 == nXRight : nXRight == nColCount,
											0 == nY,
											true));
								}
							}
                        }
					}

					// no empty references; reallocate sequences by used count
					xCellSequence.realloc(nCellInsert);
					xBorderSequence.realloc(nBorderInsert);

					// append to target. We want fillings and text first
					xRetval = xCellSequence;
					drawinglayer::primitive2d::appendPrimitive2DSequenceToPrimitive2DSequence(xRetval, xBorderSequence);
				}
			}

            if(xRetval.hasElements())
            {
                // check and create evtl. shadow for created content
                drawinglayer::attribute::SdrShadowAttribute* pNewShadowAttribute = drawinglayer::primitive2d::createNewSdrShadowAttribute(rObjectItemSet);

                if(pNewShadowAttribute)
                {
			        // attention: shadow is added BEFORE object stuff to render it BEHIND object (!)
			        const drawinglayer::primitive2d::Primitive2DReference xShadow(drawinglayer::primitive2d::createShadowPrimitive(xRetval, *pNewShadowAttribute));

			        if(xShadow.is())
			        {
				        drawinglayer::primitive2d::Primitive2DSequence xContentWithShadow(2);
				        xContentWithShadow[0] = xShadow;
				        xContentWithShadow[1] = drawinglayer::primitive2d::Primitive2DReference(new drawinglayer::primitive2d::GroupPrimitive2D(xRetval));
				        xRetval = xContentWithShadow;
			        }
                    
                    delete pNewShadowAttribute;
                }
            }

			return xRetval;
        }

		ViewContactOfTableObj::ViewContactOfTableObj(::sdr::table::SdrTableObj& rTableObj)
		:	ViewContactOfSdrObj(rTableObj)
		{
		}

		ViewContactOfTableObj::~ViewContactOfTableObj()
		{
		}
	} // end of namespace contact
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////
// eof
