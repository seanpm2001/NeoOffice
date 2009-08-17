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
 * Modified August 2009 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svx.hxx"

#include <tools/bigint.hxx>
#include <svx/svdopath.hxx>
#include <math.h>
#include <svx/xpool.hxx>
#include <svx/xpoly.hxx>
#include <svx/xoutx.hxx>
#include "svdxout.hxx"
#include <svx/svdattr.hxx>
#include "svdtouch.hxx"
#include <svx/svdtrans.hxx>
#include <svx/svdetc.hxx>
#include <svx/svddrag.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdhdl.hxx>
#include <svx/svdview.hxx>  // fuer MovCreate bei Freihandlinien
#include "svdglob.hxx"  // Stringcache
#include "svdstr.hrc"   // Objektname

#ifdef _MSC_VER
#pragma optimize ("",off)
#pragma warning(disable: 4748) // "... because optimizations are disabled ..."
#endif

#include <svx/xlnwtit.hxx>
#include <svx/xlnclit.hxx>
#include <svx/xflclit.hxx>
#include <svx/svdogrp.hxx>

#include <svx/polypolygoneditor.hxx>
#include <svx/xlntrit.hxx>
#include <vcl/salbtype.hxx>		// FRound
#include "svdoimp.hxx"
#include <basegfx/matrix/b2dhommatrix.hxx>

// #104018# replace macros above with type-safe methods
inline sal_Int32 ImplTwipsToMM(sal_Int32 nVal) { return ((nVal * 127 + 36) / 72); }
inline sal_Int32 ImplMMToTwips(sal_Int32 nVal) { return ((nVal * 72 + 63) / 127); }
inline sal_Int64 ImplTwipsToMM(sal_Int64 nVal) { return ((nVal * 127 + 36) / 72); }
inline sal_Int64 ImplMMToTwips(sal_Int64 nVal) { return ((nVal * 72 + 63) / 127); }
inline double ImplTwipsToMM(double fVal) { return (fVal * (127.0 / 72.0)); }
inline double ImplMMToTwips(double fVal) { return (fVal * (72.0 / 127.0)); }
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/range/b2drange.hxx>
#include <basegfx/curve/b2dcubicbezier.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>

#ifdef USE_JAVA
#include "xattr.hxx"
#endif	// USE_JAVA

using namespace sdr;

inline USHORT GetPrevPnt(USHORT nPnt, USHORT nPntMax, FASTBOOL bClosed)
{
	if (nPnt>0) {
		nPnt--;
	} else {
		nPnt=nPntMax;
		if (bClosed) nPnt--;
	}
	return nPnt;
}

inline USHORT GetNextPnt(USHORT nPnt, USHORT nPntMax, FASTBOOL bClosed)
{
	nPnt++;
	if (nPnt>nPntMax || (bClosed && nPnt>=nPntMax)) nPnt=0;
	return nPnt;
}

struct ImpSdrPathDragData  : public SdrDragStatUserData
{
	XPolygon					aXP;            // Ausschnitt aud dem Originalpolygon
	FASTBOOL					bValid;         // FALSE = zu wenig Punkte
	FASTBOOL					bClosed;        // geschlossenes Objekt?
	USHORT						nPoly;          // Nummer des Polygons im PolyPolygon
	USHORT						nPnt;           // Punktnummer innerhalb des obigen Polygons
	USHORT						nPntAnz;        // Punktanzahl des Polygons
	USHORT						nPntMax;        // Maximaler Index
	FASTBOOL					bBegPnt;        // Gedraggter Punkt ist der Anfangspunkt einer Polyline
	FASTBOOL					bEndPnt;        // Gedraggter Punkt ist der Endpunkt einer Polyline
	USHORT						nPrevPnt;       // Index des vorherigen Punkts
	USHORT						nNextPnt;       // Index des naechsten Punkts
	FASTBOOL					bPrevIsBegPnt;  // Vorheriger Punkt ist Anfangspunkt einer Polyline
	FASTBOOL					bNextIsEndPnt;  // Folgepunkt ist Endpunkt einer Polyline
	USHORT						nPrevPrevPnt;   // Index des vorvorherigen Punkts
	USHORT						nNextNextPnt;   // Index des uebernaechsten Punkts
	FASTBOOL					bControl;       // Punkt ist ein Kontrollpunkt
	FASTBOOL					bIsPrevControl; // Punkt ist Kontrollpunkt vor einem Stuetzpunkt
	FASTBOOL					bIsNextControl; // Punkt ist Kontrollpunkt hinter einem Stuetzpunkt
	FASTBOOL					bPrevIsControl; // Falls nPnt ein StPnt: Davor ist ein Kontrollpunkt
	FASTBOOL					bNextIsControl; // Falls nPnt ein StPnt: Dahinter ist ein Kontrollpunkt
	USHORT						nPrevPrevPnt0;
	USHORT						nPrevPnt0;
	USHORT						nPnt0;
	USHORT						nNextPnt0;
	USHORT						nNextNextPnt0;
	FASTBOOL					bEliminate;     // Punkt loeschen? (wird von MovDrag gesetzt)

	// ##
	BOOL						mbMultiPointDrag;
	const XPolyPolygon			maOrig;
	XPolyPolygon				maMove;
	Container					maHandles;

public:
	ImpSdrPathDragData(const SdrPathObj& rPO, const SdrHdl& rHdl, BOOL bMuPoDr, const SdrDragStat& rDrag);
	void ResetPoly(const SdrPathObj& rPO);
	BOOL IsMultiPointDrag() const { return mbMultiPointDrag; }
};

ImpSdrPathDragData::ImpSdrPathDragData(const SdrPathObj& rPO, const SdrHdl& rHdl, BOOL bMuPoDr, const SdrDragStat& rDrag)
:	aXP(5),
	mbMultiPointDrag(bMuPoDr),
	maOrig(rPO.GetPathPoly()),
	maHandles(0)
{
	if(mbMultiPointDrag)
	{
		const SdrMarkView& rMarkView = *rDrag.GetView();
		const SdrHdlList& rHdlList = rMarkView.GetHdlList();
		const sal_uInt32 nHdlCount = rHdlList.GetHdlCount();

		for(sal_uInt32 a(0); a < nHdlCount; a++)
		{
			SdrHdl* pTestHdl = rHdlList.GetHdl(a);

			if(pTestHdl
				&& pTestHdl->IsSelected()
				&& pTestHdl->GetObj() == (SdrObject*)&rPO)
			{
				maHandles.Insert(pTestHdl, CONTAINER_APPEND);
			}
		}

		maMove = maOrig;
		bValid = TRUE;
	}
	else
	{
		bValid=FALSE;
		bClosed=rPO.IsClosed();          // geschlossenes Objekt?
		nPoly=(sal_uInt16)rHdl.GetPolyNum();            // Nummer des Polygons im PolyPolygon
		nPnt=(sal_uInt16)rHdl.GetPointNum();            // Punktnummer innerhalb des obigen Polygons
		const XPolygon aTmpXP(rPO.GetPathPoly().getB2DPolygon(nPoly));
		nPntAnz=aTmpXP.GetPointCount();        // Punktanzahl des Polygons
		if (nPntAnz==0 || (bClosed && nPntAnz==1)) return; // min. 1Pt bei Line, min. 2 bei Polygon
		nPntMax=nPntAnz-1;                  // Maximaler Index
		bBegPnt=!bClosed && nPnt==0;        // Gedraggter Punkt ist der Anfangspunkt einer Polyline
		bEndPnt=!bClosed && nPnt==nPntMax;  // Gedraggter Punkt ist der Endpunkt einer Polyline
		if (bClosed && nPntAnz<=3) {        // Falls Polygon auch nur eine Linie ist
			bBegPnt=(nPntAnz<3) || nPnt==0;
			bEndPnt=(nPntAnz<3) || nPnt==nPntMax-1;
		}
		nPrevPnt=nPnt;                      // Index des vorherigen Punkts
		nNextPnt=nPnt;                      // Index des naechsten Punkts
		if (!bBegPnt) nPrevPnt=GetPrevPnt(nPnt,nPntMax,bClosed);
		if (!bEndPnt) nNextPnt=GetNextPnt(nPnt,nPntMax,bClosed);
		bPrevIsBegPnt=bBegPnt || (!bClosed && nPrevPnt==0);
		bNextIsEndPnt=bEndPnt || (!bClosed && nNextPnt==nPntMax);
		nPrevPrevPnt=nPnt;                  // Index des vorvorherigen Punkts
		nNextNextPnt=nPnt;                  // Index des uebernaechsten Punkts
		if (!bPrevIsBegPnt) nPrevPrevPnt=GetPrevPnt(nPrevPnt,nPntMax,bClosed);
		if (!bNextIsEndPnt) nNextNextPnt=GetNextPnt(nNextPnt,nPntMax,bClosed);
		bControl=rHdl.IsPlusHdl();          // Punkt ist ein Kontrollpunkt
		bIsPrevControl=FALSE;               // Punkt ist Kontrollpunkt vor einem Stuetzpunkt
		bIsNextControl=FALSE;               // Punkt ist Kontrollpunkt hinter einem Stuetzpunkt
		bPrevIsControl=FALSE;               // Falls nPnt ein StPnt: Davor ist ein Kontrollpunkt
		bNextIsControl=FALSE;               // Falls nPnt ein StPnt: Dahinter ist ein Kontrollpunkt
		if (bControl) {
			bIsPrevControl=aTmpXP.IsControl(nPrevPnt);
			bIsNextControl=!bIsPrevControl;
		} else {
			bPrevIsControl=!bBegPnt && !bPrevIsBegPnt && aTmpXP.GetFlags(nPrevPnt)==XPOLY_CONTROL;
			bNextIsControl=!bEndPnt && !bNextIsEndPnt && aTmpXP.GetFlags(nNextPnt)==XPOLY_CONTROL;
		}
		nPrevPrevPnt0=nPrevPrevPnt;
		nPrevPnt0    =nPrevPnt;
		nPnt0        =nPnt;
		nNextPnt0    =nNextPnt;
		nNextNextPnt0=nNextNextPnt;
		nPrevPrevPnt=0;
		nPrevPnt=1;
		nPnt=2;
		nNextPnt=3;
		nNextNextPnt=4;
		bEliminate=FALSE;
		ResetPoly(rPO);
		bValid=TRUE;
	}
}

void ImpSdrPathDragData::ResetPoly(const SdrPathObj& rPO)
{
	const XPolygon aTmpXP(rPO.GetPathPoly().getB2DPolygon(nPoly));
	aXP[0]=aTmpXP[nPrevPrevPnt0];  aXP.SetFlags(0,aTmpXP.GetFlags(nPrevPrevPnt0));
	aXP[1]=aTmpXP[nPrevPnt0];      aXP.SetFlags(1,aTmpXP.GetFlags(nPrevPnt0));
	aXP[2]=aTmpXP[nPnt0];          aXP.SetFlags(2,aTmpXP.GetFlags(nPnt0));
	aXP[3]=aTmpXP[nNextPnt0];      aXP.SetFlags(3,aTmpXP.GetFlags(nNextPnt0));
	aXP[4]=aTmpXP[nNextNextPnt0];  aXP.SetFlags(4,aTmpXP.GetFlags(nNextNextPnt0));
}

/*************************************************************************/

struct ImpPathCreateUser  : public SdrDragStatUserData
{
	Point					aBezControl0;
	Point					aBezStart;
	Point					aBezCtrl1;
	Point					aBezCtrl2;
	Point					aBezEnd;
	Point					aCircStart;
	Point					aCircEnd;
	Point					aCircCenter;
	Point					aLineStart;
	Point					aLineEnd;
	Point					aRectP1;
	Point					aRectP2;
	Point					aRectP3;
	long					nCircRadius;
	long					nCircStWink;
	long					nCircRelWink;
	FASTBOOL				bBezier;
	FASTBOOL				bBezHasCtrl0;
	FASTBOOL				bCurve;
	FASTBOOL				bCircle;
	FASTBOOL				bAngleSnap;
	FASTBOOL				bLine;
	FASTBOOL				bLine90;
	FASTBOOL				bRect;
	FASTBOOL				bMixedCreate;
	USHORT					nBezierStartPoint;
	SdrObjKind				eStartKind;
	SdrObjKind				eAktKind;

public:
	ImpPathCreateUser(): nCircRadius(0),nCircStWink(0),nCircRelWink(0),
		bBezier(FALSE),bBezHasCtrl0(FALSE),bCurve(FALSE),bCircle(FALSE),bAngleSnap(FALSE),bLine(FALSE),bLine90(FALSE),bRect(FALSE),
		bMixedCreate(FALSE),nBezierStartPoint(0),eStartKind(OBJ_NONE),eAktKind(OBJ_NONE) { }

	void ResetFormFlags() { bBezier=FALSE; bCurve=FALSE; bCircle=FALSE; bLine=FALSE; bRect=FALSE; }
	FASTBOOL IsFormFlag() const { return bBezier || bCurve || bCircle || bLine || bRect; }
	XPolygon GetFormPoly() const;
	FASTBOOL CalcBezier(const Point& rP1, const Point& rP2, const Point& rDir, FASTBOOL bMouseDown);
	XPolygon GetBezierPoly() const;
	//FASTBOOL CalcCurve(const Point& rP1, const Point& rP2, const Point& rDir, SdrView* pView) { return FALSE; }
	XPolygon GetCurvePoly() const { return XPolygon(); }
	FASTBOOL CalcCircle(const Point& rP1, const Point& rP2, const Point& rDir, SdrView* pView);
	XPolygon GetCirclePoly() const;
	FASTBOOL CalcLine(const Point& rP1, const Point& rP2, const Point& rDir, SdrView* pView);
	Point    CalcLine(const Point& rCsr, long nDirX, long nDirY, SdrView* pView) const;
	XPolygon GetLinePoly() const;
	FASTBOOL CalcRect(const Point& rP1, const Point& rP2, const Point& rDir, SdrView* pView);
	XPolygon GetRectPoly() const;
};

XPolygon ImpPathCreateUser::GetFormPoly() const
{
	if (bBezier) return GetBezierPoly();
	if (bCurve)  return GetCurvePoly();
	if (bCircle) return GetCirclePoly();
	if (bLine)   return GetLinePoly();
	if (bRect)   return GetRectPoly();
	return XPolygon();
}

FASTBOOL ImpPathCreateUser::CalcBezier(const Point& rP1, const Point& rP2, const Point& rDir, FASTBOOL bMouseDown)
{
	FASTBOOL bRet=TRUE;
	aBezStart=rP1;
	aBezCtrl1=rP1+rDir;
	aBezCtrl2=rP2;

	// #i21479#
	// Also copy the end point when no end point is set yet
	if (!bMouseDown || (0L == aBezEnd.X() && 0L == aBezEnd.Y())) aBezEnd=rP2;

	bBezier=bRet;
	return bRet;
}

XPolygon ImpPathCreateUser::GetBezierPoly() const
{
	XPolygon aXP(4);
	aXP[0]=aBezStart; aXP.SetFlags(0,XPOLY_SMOOTH);
	aXP[1]=aBezCtrl1; aXP.SetFlags(1,XPOLY_CONTROL);
	aXP[2]=aBezCtrl2; aXP.SetFlags(2,XPOLY_CONTROL);
	aXP[3]=aBezEnd;
	return aXP;
}

FASTBOOL ImpPathCreateUser::CalcCircle(const Point& rP1, const Point& rP2, const Point& rDir, SdrView* pView)
{
	long nTangAngle=GetAngle(rDir);
	aCircStart=rP1;
	aCircEnd=rP2;
	aCircCenter=rP1;
	long dx=rP2.X()-rP1.X();
	long dy=rP2.Y()-rP1.Y();
	long dAngle=GetAngle(Point(dx,dy))-nTangAngle;
	dAngle=NormAngle360(dAngle);
	long nTmpAngle=NormAngle360(9000-dAngle);
	FASTBOOL bRet=nTmpAngle!=9000 && nTmpAngle!=27000;
	long nRad=0;
	if (bRet) {
		double cs=cos(nTmpAngle*nPi180);
		double nR=(double)GetLen(Point(dx,dy))/cs/2;
		nRad=Abs(Round(nR));
	}
	if (dAngle<18000) {
		nCircStWink=NormAngle360(nTangAngle-9000);
		nCircRelWink=NormAngle360(2*dAngle);
		aCircCenter.X()+=Round(nRad*cos((nTangAngle+9000)*nPi180));
		aCircCenter.Y()-=Round(nRad*sin((nTangAngle+9000)*nPi180));
	} else {
		nCircStWink=NormAngle360(nTangAngle+9000);
		nCircRelWink=-NormAngle360(36000-2*dAngle);
		aCircCenter.X()+=Round(nRad*cos((nTangAngle-9000)*nPi180));
		aCircCenter.Y()-=Round(nRad*sin((nTangAngle-9000)*nPi180));
	}
	bAngleSnap=pView!=NULL && pView->IsAngleSnapEnabled();
	if (bAngleSnap) {
		long nSA=pView->GetSnapAngle();
		if (nSA!=0) { // Winkelfang
			FASTBOOL bNeg=nCircRelWink<0;
			if (bNeg) nCircRelWink=-nCircRelWink;
			nCircRelWink+=nSA/2;
			nCircRelWink/=nSA;
			nCircRelWink*=nSA;
			nCircRelWink=NormAngle360(nCircRelWink);
			if (bNeg) nCircRelWink=-nCircRelWink;
		}
	}
	nCircRadius=nRad;
	if (nRad==0 || Abs(nCircRelWink)<5) bRet=FALSE;
	bCircle=bRet;
	return bRet;
}

XPolygon ImpPathCreateUser::GetCirclePoly() const
{
	if (nCircRelWink>=0) {
		XPolygon aXP(aCircCenter,nCircRadius,nCircRadius,
					 USHORT((nCircStWink+5)/10),USHORT((nCircStWink+nCircRelWink+5)/10),FALSE);
		aXP[0]=aCircStart; aXP.SetFlags(0,XPOLY_SMOOTH);
		if (!bAngleSnap) aXP[aXP.GetPointCount()-1]=aCircEnd;
		return aXP;
	} else {
		XPolygon aXP(aCircCenter,nCircRadius,nCircRadius,
					 USHORT(NormAngle360(nCircStWink+nCircRelWink+5)/10),USHORT((nCircStWink+5)/10),FALSE);
		USHORT nAnz=aXP.GetPointCount();
		for (USHORT nNum=nAnz/2; nNum>0;) {
			nNum--; // XPoly Punktreihenfolge umkehren
			USHORT n2=nAnz-nNum-1;
			Point aPt(aXP[nNum]);
			aXP[nNum]=aXP[n2];
			aXP[n2]=aPt;
		}
		aXP[0]=aCircStart; aXP.SetFlags(0,XPOLY_SMOOTH);
		if (!bAngleSnap) aXP[aXP.GetPointCount()-1]=aCircEnd;
		return aXP;
	}
}

Point ImpPathCreateUser::CalcLine(const Point& aCsr, long nDirX, long nDirY, SdrView* pView) const
{
	long x=aCsr.X(),x1=x,x2=x;
	long y=aCsr.Y(),y1=y,y2=y;
	FASTBOOL bHLin=nDirY==0;
	FASTBOOL bVLin=nDirX==0;
	if (bHLin) y=0;
	else if (bVLin) x=0;
	else {
		x1=BigMulDiv(y,nDirX,nDirY);
		y2=BigMulDiv(x,nDirY,nDirX);
		long l1=Abs(x1)+Abs(y1);
		long l2=Abs(x2)+Abs(y2);
		if (l1<=l2 !=(pView!=NULL && pView->IsBigOrtho())) {
			x=x1; y=y1;
		} else {
			x=x2; y=y2;
		}
	}
	return Point(x,y);
}

FASTBOOL ImpPathCreateUser::CalcLine(const Point& rP1, const Point& rP2, const Point& rDir, SdrView* pView)
{
	aLineStart=rP1;
	aLineEnd=rP2;
	bLine90=FALSE;
	if (rP1==rP2 || (rDir.X()==0 && rDir.Y()==0)) { bLine=FALSE; return FALSE; }
	Point aTmpPt(rP2-rP1);
	long nDirX=rDir.X();
	long nDirY=rDir.Y();
	Point aP1(CalcLine(aTmpPt, nDirX, nDirY,pView)); aP1-=aTmpPt; long nQ1=Abs(aP1.X())+Abs(aP1.Y());
	Point aP2(CalcLine(aTmpPt, nDirY,-nDirX,pView)); aP2-=aTmpPt; long nQ2=Abs(aP2.X())+Abs(aP2.Y());
	if (pView!=NULL && pView->IsOrtho()) nQ1=0; // Ortho schaltet rechtwinklig aus
	bLine90=nQ1>2*nQ2;
	if (!bLine90) { // glatter Uebergang
		aLineEnd+=aP1;
	} else {          // rechtwinkliger Uebergang
		aLineEnd+=aP2;
	}
	bLine=TRUE;
	return TRUE;
}

XPolygon ImpPathCreateUser::GetLinePoly() const
{
	XPolygon aXP(2);
	aXP[0]=aLineStart; if (!bLine90) aXP.SetFlags(0,XPOLY_SMOOTH);
	aXP[1]=aLineEnd;
	return aXP;
}

FASTBOOL ImpPathCreateUser::CalcRect(const Point& rP1, const Point& rP2, const Point& rDir, SdrView* pView)
{
	aRectP1=rP1;
	aRectP2=rP1;
	aRectP3=rP2;
	if (rP1==rP2 || (rDir.X()==0 && rDir.Y()==0)) { bRect=FALSE; return FALSE; }
	Point aTmpPt(rP2-rP1);
	long nDirX=rDir.X();
	long nDirY=rDir.Y();
	long x=aTmpPt.X();
	long y=aTmpPt.Y();
	FASTBOOL bHLin=nDirY==0;
	FASTBOOL bVLin=nDirX==0;
	if (bHLin) y=0;
	else if (bVLin) x=0;
	else {
		y=BigMulDiv(x,nDirY,nDirX);
		long nHypLen=aTmpPt.Y()-y;
		long nTangAngle=-GetAngle(rDir);
		// sin=g/h, g=h*sin
		double a=nTangAngle*nPi180;
		double sn=sin(a);
		double cs=cos(a);
		double nGKathLen=nHypLen*sn;
		y+=Round(nGKathLen*sn);
		x+=Round(nGKathLen*cs);
	}
	aRectP2.X()+=x;
	aRectP2.Y()+=y;
	if (pView!=NULL && pView->IsOrtho()) {
		long dx1=aRectP2.X()-aRectP1.X(); long dx1a=Abs(dx1);
		long dy1=aRectP2.Y()-aRectP1.Y(); long dy1a=Abs(dy1);
		long dx2=aRectP3.X()-aRectP2.X(); long dx2a=Abs(dx2);
		long dy2=aRectP3.Y()-aRectP2.Y(); long dy2a=Abs(dy2);
		FASTBOOL b1MoreThan2=dx1a+dy1a>dx2a+dy2a;
		if (b1MoreThan2 != pView->IsBigOrtho()) {
			long xtemp=dy2a-dx1a; if (dx1<0) xtemp=-xtemp;
			long ytemp=dx2a-dy1a; if (dy1<0) ytemp=-ytemp;
			aRectP2.X()+=xtemp;
			aRectP2.Y()+=ytemp;
			aRectP3.X()+=xtemp;
			aRectP3.Y()+=ytemp;
		} else {
			long xtemp=dy1a-dx2a; if (dx2<0) xtemp=-xtemp;
			long ytemp=dx1a-dy2a; if (dy2<0) ytemp=-ytemp;
			aRectP3.X()+=xtemp;
			aRectP3.Y()+=ytemp;
		}
	}
	bRect=TRUE;
	return TRUE;
}

XPolygon ImpPathCreateUser::GetRectPoly() const
{
	XPolygon aXP(3);
	aXP[0]=aRectP1; aXP.SetFlags(0,XPOLY_SMOOTH);
	aXP[1]=aRectP2;
	if (aRectP3!=aRectP2) aXP[2]=aRectP3;
	return aXP;
}

/*************************************************************************/

class ImpPathForDragAndCreate
{
	SdrPathObj&					mrSdrPathObject;
	XPolyPolygon				aPathPolygon;
	SdrObjKind					meObjectKind;
	bool						mbCreating;

public:
	ImpPathForDragAndCreate(SdrPathObj& rSdrPathObject);
	~ImpPathForDragAndCreate();

	// drag stuff
	FASTBOOL BegDrag(SdrDragStat& rDrag) const;
	FASTBOOL MovDrag(SdrDragStat& rDrag) const;
	FASTBOOL EndDrag(SdrDragStat& rDrag);
	void BrkDrag(SdrDragStat& rDrag) const;
	String GetDragComment(const SdrDragStat& rDrag, FASTBOOL bUndoDragComment, FASTBOOL bCreateComment) const;
	basegfx::B2DPolyPolygon TakeDragPoly(const SdrDragStat& rDrag) const;

	// create stuff
	FASTBOOL BegCreate(SdrDragStat& rStat);
	FASTBOOL MovCreate(SdrDragStat& rStat);
	FASTBOOL EndCreate(SdrDragStat& rStat, SdrCreateCmd eCmd);
	FASTBOOL BckCreate(SdrDragStat& rStat);
	void BrkCreate(SdrDragStat& rStat);
	Pointer GetCreatePointer() const;

	// helping stuff
	bool IsClosed(SdrObjKind eKind) const { return eKind==OBJ_POLY || eKind==OBJ_PATHPOLY || eKind==OBJ_PATHFILL || eKind==OBJ_FREEFILL || eKind==OBJ_SPLNFILL; }
	bool IsFreeHand(SdrObjKind eKind) const { return eKind==OBJ_FREELINE || eKind==OBJ_FREEFILL; }
	bool IsBezier(SdrObjKind eKind) const { return eKind==OBJ_PATHLINE || eKind==OBJ_PATHFILL; }
	bool IsCreating() const { return mbCreating; }

	// get the polygon
	basegfx::B2DPolyPolygon TakeObjectPolyPolygon(const SdrDragStat& rDrag) const;
	basegfx::B2DPolyPolygon TakeDragPolyPolygon(const SdrDragStat& rDrag) const;
	basegfx::B2DPolyPolygon getModifiedPolyPolygon() const { return  aPathPolygon.getB2DPolyPolygon(); }
};

ImpPathForDragAndCreate::ImpPathForDragAndCreate(SdrPathObj& rSdrPathObject)
:	mrSdrPathObject(rSdrPathObject),
	aPathPolygon(rSdrPathObject.GetPathPoly()),
	meObjectKind(mrSdrPathObject.meKind),
	mbCreating(false)
{
}

ImpPathForDragAndCreate::~ImpPathForDragAndCreate()
{
}

FASTBOOL ImpPathForDragAndCreate::BegDrag(SdrDragStat& rDrag) const
{
	const SdrHdl* pHdl=rDrag.GetHdl();
	if(!pHdl) 
		return FALSE;

	BOOL bMultiPointDrag(TRUE);

	if(aPathPolygon[(sal_uInt16)pHdl->GetPolyNum()].IsControl((sal_uInt16)pHdl->GetPointNum()))
		bMultiPointDrag = FALSE;

	if(bMultiPointDrag)
	{
		const SdrMarkView& rMarkView = *rDrag.GetView();
		const SdrHdlList& rHdlList = rMarkView.GetHdlList();
		const sal_uInt32 nHdlCount = rHdlList.GetHdlCount();
		sal_uInt32 nSelectedPoints(0);

		for(sal_uInt32 a(0); a < nHdlCount; a++)
		{
			SdrHdl* pTestHdl = rHdlList.GetHdl(a);

			if(pTestHdl
				&& pTestHdl->IsSelected()
                    // #i77148# OOps - comparison need to be done with the SdrObject, not with his.
                    // This is an error from drag isolation and a good example that direct c-casts
                    // are bad.
				&& pTestHdl->GetObj() == &mrSdrPathObject) 
			{
				nSelectedPoints++;
			}
		}

		if(nSelectedPoints <= 1)
			bMultiPointDrag = FALSE;
	}

	ImpSdrPathDragData* pID=new ImpSdrPathDragData(mrSdrPathObject,*pHdl,bMultiPointDrag,rDrag);
	if (!pID->bValid) {
		DBG_ERROR("ImpPathForDragAndCreate::BegDrag(): ImpSdrPathDragData ist ungueltig");
		delete pID;
		return FALSE;
	}

	rDrag.SetUser(pID);
	return TRUE;
}

FASTBOOL ImpPathForDragAndCreate::MovDrag(SdrDragStat& rDrag) const
{
	ImpSdrPathDragData* pID=(ImpSdrPathDragData*)rDrag.GetUser();
	if (pID==NULL || !pID->bValid) {
		DBG_ERROR("ImpPathForDragAndCreate::MovDrag(): ImpSdrPathDragData ist ungueltig");
		return FALSE;
	}

	if(pID->IsMultiPointDrag())
	{
		Point aDelta(rDrag.GetNow() - rDrag.GetStart());

		if(aDelta.X() || aDelta.Y())
		{
			for(sal_uInt32 a(0); a < pID->maHandles.Count(); a++)
			{
				SdrHdl* pHandle = (SdrHdl*)pID->maHandles.GetObject(a);
				const sal_uInt16 nPolyIndex((sal_uInt16)pHandle->GetPolyNum());
				const sal_uInt16 nPointIndex((sal_uInt16)pHandle->GetPointNum());
				const XPolygon& rOrig = pID->maOrig[nPolyIndex];
				XPolygon& rMove = pID->maMove[nPolyIndex];
				const sal_uInt16 nPointCount(rOrig.GetPointCount());
				BOOL bClosed(rOrig[0] == rOrig[nPointCount-1]);

				// move point itself
				rMove[nPointIndex] = rOrig[nPointIndex] + aDelta;

				// when point is first and poly closed, move close point, too.
				if(nPointCount > 0 && !nPointIndex && bClosed)
				{
					rMove[nPointCount - 1] = rOrig[nPointCount - 1] + aDelta;

					// when moving the last point it may be necessary to move the 
					// control point in front of this one, too.
					if(nPointCount > 1 && rOrig.IsControl(nPointCount - 2))
						rMove[nPointCount - 2] = rOrig[nPointCount - 2] + aDelta;
				}

				// is a control point before this?
				if(nPointIndex > 0 && rOrig.IsControl(nPointIndex - 1))
				{
					// Yes, move it, too
					rMove[nPointIndex - 1] = rOrig[nPointIndex - 1] + aDelta;
				}

				// is a control point after this?
				if(nPointIndex + 1 < nPointCount && rOrig.IsControl(nPointIndex + 1))
				{
					// Yes, move it, too
					rMove[nPointIndex + 1] = rOrig[nPointIndex + 1] + aDelta;
				}
			}
		}
	}
	else
	{
		pID->ResetPoly(mrSdrPathObject);

		// Div. Daten lokal Kopieren fuer weniger Code und schnelleren Zugriff
		FASTBOOL bClosed       =pID->bClosed       ; // geschlossenes Objekt?
		USHORT   nPnt          =pID->nPnt          ; // Punktnummer innerhalb des obigen Polygons
		FASTBOOL bBegPnt       =pID->bBegPnt       ; // Gedraggter Punkt ist der Anfangspunkt einer Polyline
		FASTBOOL bEndPnt       =pID->bEndPnt       ; // Gedraggter Punkt ist der Endpunkt einer Polyline
		USHORT   nPrevPnt      =pID->nPrevPnt      ; // Index des vorherigen Punkts
		USHORT   nNextPnt      =pID->nNextPnt      ; // Index des naechsten Punkts
		FASTBOOL bPrevIsBegPnt =pID->bPrevIsBegPnt ; // Vorheriger Punkt ist Anfangspunkt einer Polyline
		FASTBOOL bNextIsEndPnt =pID->bNextIsEndPnt ; // Folgepunkt ist Endpunkt einer Polyline
		USHORT   nPrevPrevPnt  =pID->nPrevPrevPnt  ; // Index des vorvorherigen Punkts
		USHORT   nNextNextPnt  =pID->nNextNextPnt  ; // Index des uebernaechsten Punkts
		FASTBOOL bControl      =pID->bControl      ; // Punkt ist ein Kontrollpunkt
		//FASTBOOL bIsPrevControl=pID->bIsPrevControl; // Punkt ist Kontrollpunkt vor einem Stuetzpunkt
		FASTBOOL bIsNextControl=pID->bIsNextControl; // Punkt ist Kontrollpunkt hinter einem Stuetzpunkt
		FASTBOOL bPrevIsControl=pID->bPrevIsControl; // Falls nPnt ein StPnt: Davor ist ein Kontrollpunkt
		FASTBOOL bNextIsControl=pID->bNextIsControl; // Falls nPnt ein StPnt: Dahinter ist ein Kontrollpunkt

		// Ortho bei Linien/Polygonen = Winkel beibehalten
		if (!bControl && rDrag.GetView()!=NULL && rDrag.GetView()->IsOrtho()) {
			FASTBOOL bBigOrtho=rDrag.GetView()->IsBigOrtho();
			Point  aPos(rDrag.GetNow());      // die aktuelle Position
			Point  aPnt(pID->aXP[nPnt]);      // der gedraggte Punkt
			USHORT nPnt1=0xFFFF,nPnt2=0xFFFF; // seine Nachbarpunkte
			Point  aNeuPos1,aNeuPos2;         // die neuen Alternativen fuer aPos
			FASTBOOL bPnt1=FALSE,bPnt2=FALSE; // die neuen Alternativen gueltig?
			if (!bClosed && pID->nPntAnz>=2) { // Mind. 2 Pt bei Linien
				if (!bBegPnt) nPnt1=nPrevPnt;
				if (!bEndPnt) nPnt2=nNextPnt;
			}
			if (bClosed && pID->nPntAnz>=3) { // Mind. 3 Pt bei Polygon
				nPnt1=nPrevPnt;
				nPnt2=nNextPnt;
			}
			if (nPnt1!=0xFFFF && !bPrevIsControl) {
				Point aPnt1=pID->aXP[nPnt1];
				long ndx0=aPnt.X()-aPnt1.X();
				long ndy0=aPnt.Y()-aPnt1.Y();
				FASTBOOL bHLin=ndy0==0;
				FASTBOOL bVLin=ndx0==0;
				if (!bHLin || !bVLin) {
					long ndx=aPos.X()-aPnt1.X();
					long ndy=aPos.Y()-aPnt1.Y();
					bPnt1=TRUE;
					double nXFact=0; if (!bVLin) nXFact=(double)ndx/(double)ndx0;
					double nYFact=0; if (!bHLin) nYFact=(double)ndy/(double)ndy0;
					FASTBOOL bHor=bHLin || (!bVLin && (nXFact>nYFact) ==bBigOrtho);
					FASTBOOL bVer=bVLin || (!bHLin && (nXFact<=nYFact)==bBigOrtho);
					if (bHor) ndy=long(ndy0*nXFact);
					if (bVer) ndx=long(ndx0*nYFact);
					aNeuPos1=aPnt1;
					aNeuPos1.X()+=ndx;
					aNeuPos1.Y()+=ndy;
				}
			}
			if (nPnt2!=0xFFFF && !bNextIsControl) {
				Point aPnt2=pID->aXP[nPnt2];
				long ndx0=aPnt.X()-aPnt2.X();
				long ndy0=aPnt.Y()-aPnt2.Y();
				FASTBOOL bHLin=ndy0==0;
				FASTBOOL bVLin=ndx0==0;
				if (!bHLin || !bVLin) {
					long ndx=aPos.X()-aPnt2.X();
					long ndy=aPos.Y()-aPnt2.Y();
					bPnt2=TRUE;
					double nXFact=0; if (!bVLin) nXFact=(double)ndx/(double)ndx0;
					double nYFact=0; if (!bHLin) nYFact=(double)ndy/(double)ndy0;
					FASTBOOL bHor=bHLin || (!bVLin && (nXFact>nYFact) ==bBigOrtho);
					FASTBOOL bVer=bVLin || (!bHLin && (nXFact<=nYFact)==bBigOrtho);
					if (bHor) ndy=long(ndy0*nXFact);
					if (bVer) ndx=long(ndx0*nYFact);
					aNeuPos2=aPnt2;
					aNeuPos2.X()+=ndx;
					aNeuPos2.Y()+=ndy;
				}
			}
			if (bPnt1 && bPnt2) { // beide Alternativen vorhanden (Konkurenz)
				BigInt nX1(aNeuPos1.X()-aPos.X()); nX1*=nX1;
				BigInt nY1(aNeuPos1.Y()-aPos.Y()); nY1*=nY1;
				BigInt nX2(aNeuPos2.X()-aPos.X()); nX2*=nX2;
				BigInt nY2(aNeuPos2.Y()-aPos.Y()); nY2*=nY2;
				nX1+=nY1; // Korrekturabstand zum Quadrat
				nX2+=nY2; // Korrekturabstand zum Quadrat
				// Die Alternative mit dem geringeren Korrekturbedarf gewinnt
				if (nX1<nX2) bPnt2=FALSE; else bPnt1=FALSE;
			}
			if (bPnt1) rDrag.Now()=aNeuPos1;
			if (bPnt2) rDrag.Now()=aNeuPos2;
		}
		rDrag.SetActionRect(Rectangle(rDrag.GetNow(),rDrag.GetNow()));

		// IBM Special: Punkte eliminieren, wenn die beiden angrenzenden
		//              Linien eh' fast 180 deg sind.
		if (!bControl && rDrag.GetView()!=NULL && rDrag.GetView()->IsEliminatePolyPoints() &&
			!bBegPnt && !bEndPnt && !bPrevIsControl && !bNextIsControl)
		{
			Point aPt(pID->aXP[nNextPnt]);
			aPt-=rDrag.GetNow();
			long nWink1=GetAngle(aPt);
			aPt=rDrag.GetNow();
			aPt-=pID->aXP[nPrevPnt];
			long nWink2=GetAngle(aPt);
			long nDiff=nWink1-nWink2;
			nDiff=Abs(nDiff);
			pID->bEliminate=nDiff<=rDrag.GetView()->GetEliminatePolyPointLimitAngle();
			if (pID->bEliminate) { // Position anpassen, damit Smooth an den Enden stimmt
				aPt=pID->aXP[nNextPnt];
				aPt+=pID->aXP[nPrevPnt];
				aPt/=2;
				rDrag.Now()=aPt;
			}
		}

		// Um diese Entfernung wurde insgesamt gedraggd
		Point aDiff(rDrag.GetNow()); aDiff-=pID->aXP[nPnt];

		// Insgesamt sind 8 Faelle moeglich:
		//    X      1. Weder rechts noch links Ctrl.
		// o--X--o   2. Rechts und links Ctrl, gedraggd wird St.
		// o--X      3. Nur links Ctrl, gedraggd wird St.
		//    X--o   4. Nur rechts Ctrl, gedraggd wird St.
		// x--O--o   5. Rechts und links Ctrl, gedraggd wird links.
		// x--O      6. Nur links Ctrl, gedraggd wird links.
		// o--O--x   7. Rechts und links Ctrl, gedraggd wird rechts.
		//    O--x   8. Nur rechts Ctrl, gedraggd wird rechts.
		// Zusaetzlich ist zu beachten, dass das Veraendern einer Linie (keine Kurve)
		// eine evtl. Kurve am anderen Ende der Linie bewirkt, falls dort Smooth
		// gesetzt ist (Kontrollpunktausrichtung an Gerade).

		pID->aXP[nPnt]+=aDiff;

		// Nun symmetrische PlusHandles etc. checken
		if (bControl) { // Faelle 5,6,7,8
			USHORT   nSt=nPnt;   // der zugehoerige Stuetzpunkt
			USHORT   nFix=nPnt;  // der gegenueberliegende Kontrollpunkt
			if (bIsNextControl) { // Wenn der naechste ein Kontrollpunkt ist, muss der vorh. der Stuetzpunkt sein
				nSt=nPrevPnt;
				nFix=nPrevPrevPnt;
			} else {
				nSt=nNextPnt;
				nFix=nNextNextPnt;
			}
			if (pID->aXP.IsSmooth(nSt)) {
				pID->aXP.CalcSmoothJoin(nSt,nPnt,nFix);
			}
		}

		if (!bControl) { // Faelle 1,2,3,4 wobei bei 1 nix passiert und bei 3+4 unten noch mehr folgt
			// die beiden Kontrollpunkte mit verschieben
			if (bPrevIsControl) pID->aXP[nPrevPnt]+=aDiff;
			if (bNextIsControl) pID->aXP[nNextPnt]+=aDiff;
			// Kontrollpunkt ggf. an Gerade ausrichten
			if (pID->aXP.IsSmooth(nPnt)) {
				if (bPrevIsControl && !bNextIsControl && !bEndPnt) { // Fall 3
					pID->aXP.CalcSmoothJoin(nPnt,nNextPnt,nPrevPnt);
				}
				if (bNextIsControl && !bPrevIsControl && !bBegPnt) { // Fall 4
					pID->aXP.CalcSmoothJoin(nPnt,nPrevPnt,nNextPnt);
				}
			}
			// Und nun noch die anderen Enden der Strecken ueberpruefen (nPnt+-1).
			// Ist dort eine Kurve (IsControl(nPnt+-2)) mit SmoothJoin (nPnt+-1),
			// so muss der entsprechende Kontrollpunkt (nPnt+-2) angepasst werden.
			if (!bBegPnt && !bPrevIsControl && !bPrevIsBegPnt && pID->aXP.IsSmooth(nPrevPnt)) {
				if (pID->aXP.IsControl(nPrevPrevPnt)) {
					pID->aXP.CalcSmoothJoin(nPrevPnt,nPnt,nPrevPrevPnt);
				}
			}
			if (!bEndPnt && !bNextIsControl && !bNextIsEndPnt && pID->aXP.IsSmooth(nNextPnt)) {
				if (pID->aXP.IsControl(nNextNextPnt)) {
					pID->aXP.CalcSmoothJoin(nNextPnt,nPnt,nNextNextPnt);
				}
			}
		}
	}

	return TRUE;
}

FASTBOOL ImpPathForDragAndCreate::EndDrag(SdrDragStat& rDrag)
{
	Point aLinePt1;
	Point aLinePt2;
	bool bLineGlueMirror(OBJ_LINE == meObjectKind);
	if (bLineGlueMirror) { // #40549#
		XPolygon& rXP=aPathPolygon[0];
		aLinePt1=rXP[0];
		aLinePt2=rXP[1];
	}
	ImpSdrPathDragData* pID=(ImpSdrPathDragData*)rDrag.GetUser();
	
	if(pID->IsMultiPointDrag())
	{
		aPathPolygon = pID->maMove;
	}
	else
	{
		const SdrHdl* pHdl=rDrag.GetHdl();
		if (pID==NULL || !pID->bValid) {
			DBG_ERROR("ImpPathForDragAndCreate::EndDrag(): ImpSdrPathDragData ist ungueltig");
			return FALSE;
		}

		// Referenz auf das Polygon
		XPolygon& rXP=aPathPolygon[(sal_uInt16)pHdl->GetPolyNum()];

		// Die 5 Punkte die sich evtl. geaendert haben
		if (!pID->bPrevIsBegPnt) rXP[pID->nPrevPrevPnt0]=pID->aXP[pID->nPrevPrevPnt];
		if (!pID->bNextIsEndPnt) rXP[pID->nNextNextPnt0]=pID->aXP[pID->nNextNextPnt];
		if (!pID->bBegPnt)       rXP[pID->nPrevPnt0]    =pID->aXP[pID->nPrevPnt];
		if (!pID->bEndPnt)       rXP[pID->nNextPnt0]    =pID->aXP[pID->nNextPnt];
								 rXP[pID->nPnt0]        =pID->aXP[pID->nPnt];

		// Letzter Punkt muss beim Geschlossenen immer gleich dem Ersten sein
		if (pID->bClosed) rXP[rXP.GetPointCount()-1]=rXP[0];

		if (pID->bEliminate) 
		{
			basegfx::B2DPolyPolygon aTempPolyPolygon(aPathPolygon.getB2DPolyPolygon());
			sal_uInt32 nPoly,nPnt;

			if(PolyPolygonEditor::GetRelativePolyPoint(aTempPolyPolygon, rDrag.GetHdl()->GetSourceHdlNum(), nPoly, nPnt)) 
			{
				basegfx::B2DPolygon aCandidate(aTempPolyPolygon.getB2DPolygon(nPoly));
				aCandidate.remove(nPnt);
				
				if((IsClosed(meObjectKind) && aCandidate.count() < 3L) || aCandidate.count() < 2L) 
				{
					aTempPolyPolygon.remove(nPoly);
				}
				else
				{
					aTempPolyPolygon.setB2DPolygon(nPoly, aCandidate);
				}
			}

			aPathPolygon = XPolyPolygon(aTempPolyPolygon);
		}

		// Winkel anpassen fuer Text an einfacher Linie
		if (bLineGlueMirror) 
		{ // #40549#
			Point aLinePt1_(aPathPolygon[0][0]);
			Point aLinePt2_(aPathPolygon[0][1]);
			FASTBOOL bXMirr=(aLinePt1_.X()>aLinePt2_.X())!=(aLinePt1.X()>aLinePt2.X());
			FASTBOOL bYMirr=(aLinePt1_.Y()>aLinePt2_.Y())!=(aLinePt1.Y()>aLinePt2.Y());
			if (bXMirr || bYMirr) {
				Point aRef1(mrSdrPathObject.GetSnapRect().Center());
				if (bXMirr) {
					Point aRef2(aRef1);
					aRef2.Y()++;
					mrSdrPathObject.NbcMirrorGluePoints(aRef1,aRef2);
				}
				if (bYMirr) {
					Point aRef2(aRef1);
					aRef2.X()++;
					mrSdrPathObject.NbcMirrorGluePoints(aRef1,aRef2);
				}
			}
		}
	}

	delete pID;
	rDrag.SetUser(NULL);

	return TRUE;
}

void ImpPathForDragAndCreate::BrkDrag(SdrDragStat& rDrag) const
{
	ImpSdrPathDragData* pID=(ImpSdrPathDragData*)rDrag.GetUser();
	if (pID!=NULL) {
		delete pID;
		rDrag.SetUser(NULL);
	}
}

XubString ImpPathForDragAndCreate::GetDragComment(const SdrDragStat& rDrag, FASTBOOL bUndoDragComment, FASTBOOL bCreateComment) const
{
	ImpSdrPathDragData* pID = (ImpSdrPathDragData*)rDrag.GetUser();

	if(!pID || !pID->bValid) 
		return String();

	// Hier auch mal pID verwenden !!!
	XubString aStr;
	
	if(!bCreateComment) 
	{
		const SdrHdl* pHdl = rDrag.GetHdl();

		if(bUndoDragComment || !mrSdrPathObject.GetModel() || !pHdl) 
		{
			mrSdrPathObject.ImpTakeDescriptionStr(STR_DragPathObj, aStr);
		} 
		else 
		{
			if(!pID->IsMultiPointDrag() && pID->bEliminate) 
			{
				// Punkt von ...
				mrSdrPathObject.ImpTakeDescriptionStr(STR_ViewMarkedPoint, aStr); 

				// %O loeschen
				XubString aStr2(ImpGetResStr(STR_EditDelete)); 

				// UNICODE: Punkt von ... loeschen
				aStr2.SearchAndReplaceAscii("%O", aStr); 

				return aStr2;
			}

			// dx=0.00 dy=0.00                // Beide Seiten Bezier
			// dx=0.00 dy=0.00  l=0.00 0.00�  // Anfang oder Ende oder eine Seite Bezier bzw. Hebel
			// dx=0.00 dy=0.00  l=0.00 0.00� / l=0.00 0.00�   // Mittendrin
			XubString aMetr;
			Point aBeg(rDrag.GetStart());
			Point aNow(rDrag.GetNow());

			aStr = String();
			aStr.AppendAscii("dx=");   
			mrSdrPathObject.GetModel()->TakeMetricStr(aNow.X() - aBeg.X(), aMetr, TRUE); 
			aStr += aMetr;

			aStr.AppendAscii(" dy="); 
			mrSdrPathObject.GetModel()->TakeMetricStr(aNow.Y() - aBeg.Y(), aMetr, TRUE); 
			aStr += aMetr;

			if(!pID->IsMultiPointDrag())
			{
				UINT16 nPntNum((sal_uInt16)pHdl->GetPointNum());
				const XPolygon& rXPoly = aPathPolygon[(sal_uInt16)rDrag.GetHdl()->GetPolyNum()];
				UINT16 nPntAnz((sal_uInt16)rXPoly.GetPointCount());
				BOOL bClose(IsClosed(meObjectKind));
				
				if(bClose) 
					nPntAnz--;

				if(pHdl->IsPlusHdl()) 
				{ 
					// Hebel
					UINT16 nRef(nPntNum);
					
					if(rXPoly.IsControl(nPntNum + 1)) 
						nRef--; 
					else 
						nRef++;

					aNow -= rXPoly[nRef];
					
					INT32 nLen(GetLen(aNow));
					aStr.AppendAscii("  l="); 
					mrSdrPathObject.GetModel()->TakeMetricStr(nLen, aMetr, TRUE); 
					aStr += aMetr;
					
					INT32 nWink(GetAngle(aNow));
					aStr += sal_Unicode(' ');
					mrSdrPathObject.GetModel()->TakeWinkStr(nWink, aMetr); 
					aStr += aMetr;
				} 
				else if(nPntAnz > 1) 
				{
					UINT16 nPntMax(nPntAnz - 1);
					Point aPt1,aPt2;
					BOOL bIsClosed(IsClosed(meObjectKind));
					BOOL bPt1(nPntNum > 0);
					BOOL bPt2(nPntNum < nPntMax);
					
					if(bIsClosed && nPntAnz > 2) 
					{
						bPt1 = TRUE;
						bPt2 = TRUE;
					}

					UINT16 nPt1,nPt2;

					if(nPntNum > 0) 
						nPt1 = nPntNum - 1; 
					else 
						nPt1 = nPntMax;

					if(nPntNum < nPntMax) 
						nPt2 = nPntNum + 1; 
					else 
						nPt2 = 0;

					if(bPt1 && rXPoly.IsControl(nPt1)) 
						bPt1 = FALSE; // Keine Anzeige

					if(bPt2 && rXPoly.IsControl(nPt2)) 
						bPt2 = FALSE; // von Bezierdaten

					if(bPt1) 
					{
						Point aPt(aNow);
						aPt -= rXPoly[nPt1];
						
						INT32 nLen(GetLen(aPt));
						aStr.AppendAscii("  l="); 
						mrSdrPathObject.GetModel()->TakeMetricStr(nLen, aMetr, TRUE); 
						aStr += aMetr;
						
						INT32 nWink(GetAngle(aPt));
						aStr += sal_Unicode(' '); 
						mrSdrPathObject.GetModel()->TakeWinkStr(nWink, aMetr); 
						aStr += aMetr;
					}
					
					if(bPt2) 
					{
						if(bPt1) 
							aStr.AppendAscii(" / "); 
						else 
							aStr.AppendAscii("  ");

						Point aPt(aNow);
						aPt -= rXPoly[nPt2];
						
						INT32 nLen(GetLen(aPt));
						aStr.AppendAscii("l="); 
						mrSdrPathObject.GetModel()->TakeMetricStr(nLen, aMetr, TRUE); 
						aStr += aMetr;
						
						INT32 nWink(GetAngle(aPt));
						aStr += sal_Unicode(' '); 
						mrSdrPathObject.GetModel()->TakeWinkStr(nWink, aMetr); 
						aStr += aMetr;
					}
				}
			}
		}
	} 
	else if(mrSdrPathObject.GetModel() && !pID->IsMultiPointDrag()) 
	{ 
		// Ansonsten CreateComment
		ImpPathCreateUser* pU = (ImpPathCreateUser*)rDrag.GetUser();
		SdrObjKind eKindMerk = meObjectKind;
		
		// fuer Description bei Mixed das Aktuelle...
		mrSdrPathObject.meKind = pU->eAktKind; 
		mrSdrPathObject.ImpTakeDescriptionStr(STR_ViewCreateObj, aStr);
		mrSdrPathObject.meKind = eKindMerk;

		Point aPrev(rDrag.GetPrev());
		Point aNow(rDrag.GetNow());
		
		if(pU->bLine) 
			aNow = pU->aLineEnd;

		aNow -= aPrev;
		aStr.AppendAscii(" (");
		
		XubString aMetr;
		
		if(pU->bCircle) 
		{
			mrSdrPathObject.GetModel()->TakeWinkStr(Abs(pU->nCircRelWink), aMetr); 
			aStr += aMetr;
			aStr.AppendAscii(" r="); 
			mrSdrPathObject.GetModel()->TakeMetricStr(pU->nCircRadius, aMetr, TRUE); 
			aStr += aMetr;
		}

		aStr.AppendAscii("dx=");  
		mrSdrPathObject.GetModel()->TakeMetricStr(aNow.X(), aMetr, TRUE); 
		aStr += aMetr;

		aStr.AppendAscii(" dy="); 
		mrSdrPathObject.GetModel()->TakeMetricStr(aNow.Y(), aMetr, TRUE); 
		aStr += aMetr;
		
		if(!IsFreeHand(meObjectKind)) 
		{
			INT32 nLen(GetLen(aNow));
			aStr.AppendAscii("  l="); 
			mrSdrPathObject.GetModel()->TakeMetricStr(nLen, aMetr, TRUE); 
			aStr += aMetr;

			INT32 nWink(GetAngle(aNow));
			aStr += sal_Unicode(' '); 
			mrSdrPathObject.GetModel()->TakeWinkStr(nWink, aMetr); 
			aStr += aMetr;
		}

		aStr += sal_Unicode(')');
	}

	return aStr;
}

basegfx::B2DPolyPolygon ImpPathForDragAndCreate::TakeDragPoly(const SdrDragStat& rDrag) const
{
	XPolyPolygon aXPP;
	ImpSdrPathDragData* pID=(ImpSdrPathDragData*)rDrag.GetUser();

	if(pID->IsMultiPointDrag())
	{
		aXPP.Insert(pID->maMove);
	}
	else
	{
		const XPolygon& rXP=aPathPolygon[(sal_uInt16)rDrag.GetHdl()->GetPolyNum()];
		if (rXP.GetPointCount()<=2 /*|| rXPoly.GetFlags(1)==XPOLY_CONTROL && rXPoly.GetPointCount()<=4*/) {
			XPolygon aXPoly(rXP);
			aXPoly[(sal_uInt16)rDrag.GetHdl()->GetPointNum()]=rDrag.GetNow();
			aXPP.Insert(aXPoly);
			return aXPP.getB2DPolyPolygon();
		}
		// Div. Daten lokal Kopieren fuer weniger Code und schnelleren Zugriff
		FASTBOOL bClosed       =pID->bClosed       ; // geschlossenes Objekt?
		USHORT   nPntAnz       =pID->nPntAnz       ; // Punktanzahl
		USHORT   nPnt          =pID->nPnt          ; // Punktnummer innerhalb des Polygons
		FASTBOOL bBegPnt       =pID->bBegPnt       ; // Gedraggter Punkt ist der Anfangspunkt einer Polyline
		FASTBOOL bEndPnt       =pID->bEndPnt       ; // Gedraggter Punkt ist der Endpunkt einer Polyline
		USHORT   nPrevPnt      =pID->nPrevPnt      ; // Index des vorherigen Punkts
		USHORT   nNextPnt      =pID->nNextPnt      ; // Index des naechsten Punkts
		FASTBOOL bPrevIsBegPnt =pID->bPrevIsBegPnt ; // Vorheriger Punkt ist Anfangspunkt einer Polyline
		FASTBOOL bNextIsEndPnt =pID->bNextIsEndPnt ; // Folgepunkt ist Endpunkt einer Polyline
		USHORT   nPrevPrevPnt  =pID->nPrevPrevPnt  ; // Index des vorvorherigen Punkts
		USHORT   nNextNextPnt  =pID->nNextNextPnt  ; // Index des uebernaechsten Punkts
		FASTBOOL bControl      =pID->bControl      ; // Punkt ist ein Kontrollpunkt
		//FASTBOOL bIsPrevControl=pID->bIsPrevControl; // Punkt ist Kontrollpunkt vor einem Stuetzpunkt
		FASTBOOL bIsNextControl=pID->bIsNextControl; // Punkt ist Kontrollpunkt hinter einem Stuetzpunkt
		FASTBOOL bPrevIsControl=pID->bPrevIsControl; // Falls nPnt ein StPnt: Davor ist ein Kontrollpunkt
		FASTBOOL bNextIsControl=pID->bNextIsControl; // Falls nPnt ein StPnt: Dahinter ist ein Kontrollpunkt
		XPolygon aXPoly(pID->aXP);
		XPolygon aLine1(2);
		XPolygon aLine2(2);
		XPolygon aLine3(2);
		XPolygon aLine4(2);
		if (bControl) {
			aLine1[1]=pID->aXP[nPnt];
			if (bIsNextControl) { // bin ich Kontrollpunkt hinter der Stuetzstelle?
				aLine1[0]=pID->aXP[nPrevPnt];
				aLine2[0]=pID->aXP[nNextNextPnt];
				aLine2[1]=pID->aXP[nNextPnt];
				if (pID->aXP.IsSmooth(nPrevPnt) && !bPrevIsBegPnt && pID->aXP.IsControl(nPrevPrevPnt)) {
					aXPoly.Insert(0,rXP[pID->nPrevPrevPnt0-1],XPOLY_CONTROL);
					aXPoly.Insert(0,rXP[pID->nPrevPrevPnt0-2],XPOLY_NORMAL);
					// Hebellienien fuer das gegenueberliegende Kurvensegment
					aLine3[0]=pID->aXP[nPrevPnt];
					aLine3[1]=pID->aXP[nPrevPrevPnt];
					aLine4[0]=rXP[pID->nPrevPrevPnt0-2];
					aLine4[1]=rXP[pID->nPrevPrevPnt0-1];
				} else {
					aXPoly.Remove(0,1);
				}
			} else { // ansonsten bin ich Kontrollpunkt vor der Stuetzstelle
				aLine1[0]=pID->aXP[nNextPnt];
				aLine2[0]=pID->aXP[nPrevPrevPnt];
				aLine2[1]=pID->aXP[nPrevPnt];
				if (pID->aXP.IsSmooth(nNextPnt) && !bNextIsEndPnt && pID->aXP.IsControl(nNextNextPnt)) {
					aXPoly.Insert(XPOLY_APPEND,rXP[pID->nNextNextPnt0+1],XPOLY_CONTROL);
					aXPoly.Insert(XPOLY_APPEND,rXP[pID->nNextNextPnt0+2],XPOLY_NORMAL);
					// Hebellinien fuer das gegenueberliegende Kurvensegment
					aLine3[0]=pID->aXP[nNextPnt];
					aLine3[1]=pID->aXP[nNextNextPnt];
					aLine4[0]=rXP[pID->nNextNextPnt0+2];
					aLine4[1]=rXP[pID->nNextNextPnt0+1];
				} else {
					aXPoly.Remove(aXPoly.GetPointCount()-1,1);
				}
			}
		} else { // ansonsten kein Kontrollpunkt
			if (pID->bEliminate) {
				aXPoly.Remove(2,1);
			}
			if (bPrevIsControl) aXPoly.Insert(0,rXP[pID->nPrevPrevPnt0-1],XPOLY_NORMAL);
			else if (!bBegPnt && !bPrevIsBegPnt && pID->aXP.IsControl(nPrevPrevPnt)) {
				aXPoly.Insert(0,rXP[pID->nPrevPrevPnt0-1],XPOLY_CONTROL);
				aXPoly.Insert(0,rXP[pID->nPrevPrevPnt0-2],XPOLY_NORMAL);
			} else {
				aXPoly.Remove(0,1);
				if (bBegPnt) aXPoly.Remove(0,1);
			}
			if (bNextIsControl) aXPoly.Insert(XPOLY_APPEND,rXP[pID->nNextNextPnt0+1],XPOLY_NORMAL);
			else if (!bEndPnt && !bNextIsEndPnt && pID->aXP.IsControl(nNextNextPnt)) {
				aXPoly.Insert(XPOLY_APPEND,rXP[pID->nNextNextPnt0+1],XPOLY_CONTROL);
				aXPoly.Insert(XPOLY_APPEND,rXP[pID->nNextNextPnt0+2],XPOLY_NORMAL);
			} else {
				aXPoly.Remove(aXPoly.GetPointCount()-1,1);
				if (bEndPnt) aXPoly.Remove(aXPoly.GetPointCount()-1,1);
			}
			if (bClosed) { // "Birnenproblem": 2 Linien, 1 Kurve, alles Smooth, Punkt zw. beiden Linien wird gedraggt
				if (aXPoly.GetPointCount()>nPntAnz && aXPoly.IsControl(1)) {
					USHORT a=aXPoly.GetPointCount();
					aXPoly[a-2]=aXPoly[2]; aXPoly.SetFlags(a-2,aXPoly.GetFlags(2));
					aXPoly[a-1]=aXPoly[3]; aXPoly.SetFlags(a-1,aXPoly.GetFlags(3));
					aXPoly.Remove(0,3);
				}
			}
		}
		aXPP.Insert(aXPoly);
		if (aLine1.GetPointCount()>1) aXPP.Insert(aLine1);
		if (aLine2.GetPointCount()>1) aXPP.Insert(aLine2);
		if (aLine3.GetPointCount()>1) aXPP.Insert(aLine3);
		if (aLine4.GetPointCount()>1) aXPP.Insert(aLine4);
	}

	return aXPP.getB2DPolyPolygon();
}

FASTBOOL ImpPathForDragAndCreate::BegCreate(SdrDragStat& rStat)
{
	bool bFreeHand(IsFreeHand(meObjectKind));
	rStat.SetNoSnap(bFreeHand);
	rStat.SetOrtho8Possible();
	aPathPolygon.Clear();
	mbCreating=TRUE;
	FASTBOOL bMakeStartPoint=TRUE;
	SdrView* pView=rStat.GetView();
	if (pView!=NULL && pView->IsUseIncompatiblePathCreateInterface() &&
		(meObjectKind==OBJ_POLY || meObjectKind==OBJ_PLIN || meObjectKind==OBJ_PATHLINE || meObjectKind==OBJ_PATHFILL)) {
		bMakeStartPoint=FALSE;
	}
	aPathPolygon.Insert(XPolygon());
	aPathPolygon[0][0]=rStat.GetStart();
	if (bMakeStartPoint) {
		aPathPolygon[0][1]=rStat.GetNow();
	}
	ImpPathCreateUser* pU=new ImpPathCreateUser;
	pU->eStartKind=meObjectKind;
	pU->eAktKind=meObjectKind;
	rStat.SetUser(pU);
	return TRUE;
}

FASTBOOL ImpPathForDragAndCreate::MovCreate(SdrDragStat& rStat)
{
	ImpPathCreateUser* pU=(ImpPathCreateUser*)rStat.GetUser();
	SdrView* pView=rStat.GetView();
	XPolygon& rXPoly=aPathPolygon[aPathPolygon.Count()-1];
	if (pView!=NULL && pView->IsCreateMode()) {
		// ggf. auf anderes CreateTool umschalten
		UINT16 nIdent;
		UINT32 nInvent;
		pView->TakeCurrentObj(nIdent,nInvent);
		if (nInvent==SdrInventor && pU->eAktKind!=(SdrObjKind)nIdent) {
			SdrObjKind eNewKind=(SdrObjKind)nIdent;
			switch (eNewKind) {
				case OBJ_CARC: case OBJ_CIRC: case OBJ_CCUT: case OBJ_SECT: eNewKind=OBJ_CARC;
				case OBJ_RECT:
				case OBJ_LINE: case OBJ_PLIN: case OBJ_POLY:
				case OBJ_PATHLINE: case OBJ_PATHFILL:
				case OBJ_FREELINE: case OBJ_FREEFILL:
				case OBJ_SPLNLINE: case OBJ_SPLNFILL: {
					pU->eAktKind=eNewKind;
					pU->bMixedCreate=TRUE;
					pU->nBezierStartPoint=rXPoly.GetPointCount();
					if (pU->nBezierStartPoint>0) pU->nBezierStartPoint--;
				} break;
				default: break;
			} // switch
		}
	}
	USHORT nActPoint=rXPoly.GetPointCount();
	if (aPathPolygon.Count()>1 && rStat.IsMouseDown() && nActPoint<2) {
		rXPoly[0]=rStat.GetPos0();
		rXPoly[1]=rStat.GetNow();
		nActPoint=2;
	}
	if (nActPoint==0) {
		rXPoly[0]=rStat.GetPos0();
	} else nActPoint--;
	FASTBOOL bFreeHand=IsFreeHand(pU->eAktKind);
	rStat.SetNoSnap(bFreeHand /*|| (pU->bMixed && pU->eAktKind==OBJ_LINE)*/);
	rStat.SetOrtho8Possible(pU->eAktKind!=OBJ_CARC && pU->eAktKind!=OBJ_RECT && (!pU->bMixedCreate || pU->eAktKind!=OBJ_LINE));
	Point aActMerk(rXPoly[nActPoint]);
	rXPoly[nActPoint]=rStat.Now();
	if (!pU->bMixedCreate && pU->eStartKind==OBJ_LINE && rXPoly.GetPointCount()>=1) {
		Point aPt(rStat.Start());
		if (pView!=NULL && pView->IsCreate1stPointAsCenter()) {
			aPt+=aPt;
			aPt-=rStat.Now();
		}
		rXPoly[0]=aPt;
	}
	OutputDevice* pOut=pView==NULL ? NULL : pView->GetFirstOutputDevice(); // GetWin(0);
	if (bFreeHand) {
		if (pU->nBezierStartPoint>nActPoint) pU->nBezierStartPoint=nActPoint;
		if (rStat.IsMouseDown() && nActPoint>0) {
			// keine aufeinanderfolgenden Punkte an zu Nahe gelegenen Positionen zulassen
			long nMinDist=1;
			if (pView!=NULL) nMinDist=pView->GetFreeHandMinDistPix();
			if (pOut!=NULL) nMinDist=pOut->PixelToLogic(Size(nMinDist,0)).Width();
			if (nMinDist<1) nMinDist=1;

			Point aPt0(rXPoly[nActPoint-1]);
			Point aPt1(rStat.Now());
			long dx=aPt0.X()-aPt1.X(); if (dx<0) dx=-dx;
			long dy=aPt0.Y()-aPt1.Y(); if (dy<0) dy=-dy;
			if (dx<nMinDist && dy<nMinDist) return FALSE;

			// folgendes ist aus EndCreate kopiert (nur kleine Modifikationen)
			// und sollte dann mal in eine Methode zusammengefasst werden:

			if (nActPoint-pU->nBezierStartPoint>=3 && ((nActPoint-pU->nBezierStartPoint)%3)==0) {
				rXPoly.PointsToBezier(nActPoint-3);
				rXPoly.SetFlags(nActPoint-1,XPOLY_CONTROL);
				rXPoly.SetFlags(nActPoint-2,XPOLY_CONTROL);

				if (nActPoint>=6 && rXPoly.IsControl(nActPoint-4)) {
					rXPoly.CalcTangent(nActPoint-3,nActPoint-4,nActPoint-2);
					rXPoly.SetFlags(nActPoint-3,XPOLY_SMOOTH);
				}
			}
			rXPoly[nActPoint+1]=rStat.Now();
			rStat.NextPoint();
		} else {
			pU->nBezierStartPoint=nActPoint;
		}
	}

	pU->ResetFormFlags();
	if (IsBezier(pU->eAktKind)) {
		if (nActPoint>=2) {
			pU->CalcBezier(rXPoly[nActPoint-1],rXPoly[nActPoint],rXPoly[nActPoint-1]-rXPoly[nActPoint-2],rStat.IsMouseDown());
		} else if (pU->bBezHasCtrl0) {
			pU->CalcBezier(rXPoly[nActPoint-1],rXPoly[nActPoint],pU->aBezControl0-rXPoly[nActPoint-1],rStat.IsMouseDown());
		}
	}
	if (pU->eAktKind==OBJ_CARC && nActPoint>=2) {
		pU->CalcCircle(rXPoly[nActPoint-1],rXPoly[nActPoint],rXPoly[nActPoint-1]-rXPoly[nActPoint-2],pView);
	}
	if (pU->eAktKind==OBJ_LINE && nActPoint>=2) {
		pU->CalcLine(rXPoly[nActPoint-1],rXPoly[nActPoint],rXPoly[nActPoint-1]-rXPoly[nActPoint-2],pView);
	}
	if (pU->eAktKind==OBJ_RECT && nActPoint>=2) {
		pU->CalcRect(rXPoly[nActPoint-1],rXPoly[nActPoint],rXPoly[nActPoint-1]-rXPoly[nActPoint-2],pView);
	}

	return TRUE;
}

FASTBOOL ImpPathForDragAndCreate::EndCreate(SdrDragStat& rStat, SdrCreateCmd eCmd)
{
	ImpPathCreateUser* pU=(ImpPathCreateUser*)rStat.GetUser();
	FASTBOOL bRet=FALSE;
	SdrView* pView=rStat.GetView();
	FASTBOOL bIncomp=pView!=NULL && pView->IsUseIncompatiblePathCreateInterface();
	XPolygon& rXPoly=aPathPolygon[aPathPolygon.Count()-1];
	USHORT nActPoint=rXPoly.GetPointCount()-1;
	Point aAktMerk(rXPoly[nActPoint]);
	rXPoly[nActPoint]=rStat.Now();
	if (!pU->bMixedCreate && pU->eStartKind==OBJ_LINE) {
		if (rStat.GetPointAnz()>=2) eCmd=SDRCREATE_FORCEEND;
		bRet=eCmd==SDRCREATE_FORCEEND;
		if (bRet) {
			mbCreating=FALSE;
			delete pU;
			rStat.SetUser(NULL);
		}
		return bRet;
	}

	if (!pU->bMixedCreate && IsFreeHand(pU->eStartKind)) {
		if (rStat.GetPointAnz()>=2) eCmd=SDRCREATE_FORCEEND;
		bRet=eCmd==SDRCREATE_FORCEEND;
		if (bRet) {
			mbCreating=FALSE;
			delete pU;
			rStat.SetUser(NULL);
		}
		return bRet;
	}
	if (eCmd==SDRCREATE_NEXTPOINT || eCmd==SDRCREATE_NEXTOBJECT) {
		// keine aufeinanderfolgenden Punkte an identischer Position zulassen
		if (nActPoint==0 || rStat.Now()!=rXPoly[nActPoint-1]) {
			if (bIncomp) {
				if (pU->nBezierStartPoint>nActPoint) pU->nBezierStartPoint=nActPoint;
				if (IsBezier(pU->eAktKind) && nActPoint-pU->nBezierStartPoint>=3 && ((nActPoint-pU->nBezierStartPoint)%3)==0) {
					rXPoly.PointsToBezier(nActPoint-3);
					rXPoly.SetFlags(nActPoint-1,XPOLY_CONTROL);
					rXPoly.SetFlags(nActPoint-2,XPOLY_CONTROL);

					if (nActPoint>=6 && rXPoly.IsControl(nActPoint-4)) {
						rXPoly.CalcTangent(nActPoint-3,nActPoint-4,nActPoint-2);
						rXPoly.SetFlags(nActPoint-3,XPOLY_SMOOTH);
					}
				}
			} else {
				if (nActPoint==1 && IsBezier(pU->eAktKind) && !pU->bBezHasCtrl0) {
					pU->aBezControl0=rStat.GetNow();;
					pU->bBezHasCtrl0=TRUE;
					nActPoint--;
				}
				if (pU->IsFormFlag()) {
					USHORT nPtAnz0=rXPoly.GetPointCount();
					rXPoly.Remove(nActPoint-1,2); // die letzten beiden Punkte entfernen und durch die Form ersetzen
					rXPoly.Insert(XPOLY_APPEND,pU->GetFormPoly());
					USHORT nPtAnz1=rXPoly.GetPointCount();
					for (USHORT i=nPtAnz0+1; i<nPtAnz1-1; i++) { // Damit BckAction richtig funktioniert
						if (!rXPoly.IsControl(i)) rStat.NextPoint();
					}
					nActPoint=rXPoly.GetPointCount()-1;
				}
			}
			nActPoint++;
			rXPoly[nActPoint]=rStat.GetNow();
		}
		if (eCmd==SDRCREATE_NEXTOBJECT) {
			if (rXPoly.GetPointCount()>=2) {
				pU->bBezHasCtrl0=FALSE;
				// nur einzelnes Polygon kann offen sein, deshalb schliessen
				rXPoly[nActPoint]=rXPoly[0];
				XPolygon aXP;
				aXP[0]=rStat.GetNow();
				aPathPolygon.Insert(aXP);
			}
		}
	}

	USHORT nPolyAnz=aPathPolygon.Count();
	if (nPolyAnz!=0) {
		// den letzten Punkt ggf. wieder loeschen
		if (eCmd==SDRCREATE_FORCEEND) {
			XPolygon& rXP=aPathPolygon[nPolyAnz-1];
			USHORT nPtAnz=rXP.GetPointCount();
			if (nPtAnz>=2) {
				if (!rXP.IsControl(nPtAnz-2)) {
					if (rXP[nPtAnz-1]==rXP[nPtAnz-2]) {
						rXP.Remove(nPtAnz-1,1);
					}
				} else {
					if (rXP[nPtAnz-3]==rXP[nPtAnz-2]) {
						rXP.Remove(nPtAnz-3,3);
					}
				}
			}
		}
		for (USHORT nPolyNum=nPolyAnz; nPolyNum>0;) {
			nPolyNum--;
			XPolygon& rXP=aPathPolygon[nPolyNum];
			USHORT nPtAnz=rXP.GetPointCount();
			// Polygone mit zu wenig Punkten werden geloescht
			if (nPolyNum<nPolyAnz-1 || eCmd==SDRCREATE_FORCEEND) {
				if (nPtAnz<2) aPathPolygon.Remove(nPolyNum);
			}
		}
	}
	pU->ResetFormFlags();
	bRet=eCmd==SDRCREATE_FORCEEND;
	if (bRet) {
		mbCreating=FALSE;
		delete pU;
		rStat.SetUser(NULL);
	}
	return bRet;
}

FASTBOOL ImpPathForDragAndCreate::BckCreate(SdrDragStat& rStat)
{
	ImpPathCreateUser* pU=(ImpPathCreateUser*)rStat.GetUser();
	if (aPathPolygon.Count()>0) {
		XPolygon& rXPoly=aPathPolygon[aPathPolygon.Count()-1];
		USHORT nActPoint=rXPoly.GetPointCount();
		if (nActPoint>0) {
			nActPoint--;
			// Das letzte Stueck einer Bezierkurve wird erstmal zu 'ner Linie
			rXPoly.Remove(nActPoint,1);
			if (nActPoint>=3 && rXPoly.IsControl(nActPoint-1)) {
				// Beziersegment am Ende sollte zwar nicht vorkommen, aber falls doch ...
				rXPoly.Remove(nActPoint-1,1);
				if (rXPoly.IsControl(nActPoint-2)) rXPoly.Remove(nActPoint-2,1);
			}
		}
		nActPoint=rXPoly.GetPointCount();
		if (nActPoint>=4) { // Kein Beziersegment am Ende
			nActPoint--;
			if (rXPoly.IsControl(nActPoint-1)) {
				rXPoly.Remove(nActPoint-1,1);
				if (rXPoly.IsControl(nActPoint-2)) rXPoly.Remove(nActPoint-2,1);
			}
		}
		if (rXPoly.GetPointCount()<2) {
			aPathPolygon.Remove(aPathPolygon.Count()-1);
		}
		if (aPathPolygon.Count()>0) {
			XPolygon& rLocalXPoly=aPathPolygon[aPathPolygon.Count()-1];
			USHORT nLocalActPoint=rLocalXPoly.GetPointCount();
			if (nLocalActPoint>0) {
				nLocalActPoint--;
				rLocalXPoly[nLocalActPoint]=rStat.Now();
			}
		}
	}
	pU->ResetFormFlags();
	return aPathPolygon.Count()!=0;
}

void ImpPathForDragAndCreate::BrkCreate(SdrDragStat& rStat)
{
	ImpPathCreateUser* pU=(ImpPathCreateUser*)rStat.GetUser();
	aPathPolygon.Clear();
	mbCreating=FALSE;
	delete pU;
	rStat.SetUser(NULL);
}

basegfx::B2DPolyPolygon ImpPathForDragAndCreate::TakeObjectPolyPolygon(const SdrDragStat& rDrag) const
{
	basegfx::B2DPolyPolygon aRetval(aPathPolygon.getB2DPolyPolygon());
	SdrView* pView = rDrag.GetView();

	if(pView && pView->IsUseIncompatiblePathCreateInterface()) 
		return aRetval;

	ImpPathCreateUser* pU = (ImpPathCreateUser*)rDrag.GetUser();
	basegfx::B2DPolygon aNewPolygon(aRetval.count() ? aRetval.getB2DPolygon(aRetval.count() - 1L) : basegfx::B2DPolygon());

	if(pU->IsFormFlag() && aNewPolygon.count() > 1L)
	{ 
		// remove last segment and replace with current
		// do not forget to rescue the previous control point which will be lost when 
		// the point it's associated with is removed
		const sal_uInt32 nChangeIndex(aNewPolygon.count() - 2);
		const basegfx::B2DPoint aSavedPrevCtrlPoint(aNewPolygon.getPrevControlPoint(nChangeIndex));
		
		aNewPolygon.remove(nChangeIndex, 2L);
		aNewPolygon.append(pU->GetFormPoly().getB2DPolygon());

		if(nChangeIndex < aNewPolygon.count())
		{
			// if really something was added, set the saved prev control point at the
			// point where it belongs
			aNewPolygon.setPrevControlPoint(nChangeIndex, aSavedPrevCtrlPoint);
		}
	}

	if(aRetval.count())
	{
		aRetval.setB2DPolygon(aRetval.count() - 1L, aNewPolygon);
	}
	else
	{
		aRetval.append(aNewPolygon);
	}

	return aRetval;
}

basegfx::B2DPolyPolygon ImpPathForDragAndCreate::TakeDragPolyPolygon(const SdrDragStat& rDrag) const
{
	basegfx::B2DPolyPolygon aRetval;
	SdrView* pView = rDrag.GetView();

	if(pView && pView->IsUseIncompatiblePathCreateInterface()) 
		return aRetval;

	ImpPathCreateUser* pU = (ImpPathCreateUser*)rDrag.GetUser();

	if(pU && pU->bBezier && rDrag.IsMouseDown()) 
	{
		// no more XOR, no need for complicated helplines
		basegfx::B2DPolygon aHelpline;
		aHelpline.append(basegfx::B2DPoint(pU->aBezCtrl2.X(), pU->aBezCtrl2.Y()));
		aHelpline.append(basegfx::B2DPoint(pU->aBezEnd.X(), pU->aBezEnd.Y()));
		aRetval.append(aHelpline);
	}

	return aRetval;
}

Pointer ImpPathForDragAndCreate::GetCreatePointer() const
{
	switch (meObjectKind) {
		case OBJ_LINE    : return Pointer(POINTER_DRAW_LINE);
		case OBJ_POLY    : return Pointer(POINTER_DRAW_POLYGON);
		case OBJ_PLIN    : return Pointer(POINTER_DRAW_POLYGON);
		case OBJ_PATHLINE: return Pointer(POINTER_DRAW_BEZIER);
		case OBJ_PATHFILL: return Pointer(POINTER_DRAW_BEZIER);
		case OBJ_FREELINE: return Pointer(POINTER_DRAW_FREEHAND);
		case OBJ_FREEFILL: return Pointer(POINTER_DRAW_FREEHAND);
		case OBJ_SPLNLINE: return Pointer(POINTER_DRAW_FREEHAND);
		case OBJ_SPLNFILL: return Pointer(POINTER_DRAW_FREEHAND);
		case OBJ_PATHPOLY: return Pointer(POINTER_DRAW_POLYGON);
		case OBJ_PATHPLIN: return Pointer(POINTER_DRAW_POLYGON);
		default: break;
	} // switch
	return Pointer(POINTER_CROSS);
}

/*************************************************************************/

SdrPathObjGeoData::SdrPathObjGeoData()
{
}

SdrPathObjGeoData::~SdrPathObjGeoData()
{
}

/*************************************************************************/

TYPEINIT1(SdrPathObj,SdrTextObj);

SdrPathObj::SdrPathObj(SdrObjKind eNewKind)
:	meKind(eNewKind),
	mpDAC(0L)
{
	bClosedObj = IsClosed();
}

SdrPathObj::SdrPathObj(SdrObjKind eNewKind, const basegfx::B2DPolyPolygon& rPathPoly)
:	maPathPolygon(rPathPoly),
	meKind(eNewKind),
	mpDAC(0L)
{
	bClosedObj = IsClosed();
	ImpForceKind();
}

SdrPathObj::~SdrPathObj()
{
	impDeleteDAC();
}

sal_Bool ImpIsLine(const basegfx::B2DPolyPolygon& rPolyPolygon)
{
	return (1L == rPolyPolygon.count() && 2L == rPolyPolygon.getB2DPolygon(0L).count());
}

Rectangle ImpGetBoundRect(const basegfx::B2DPolyPolygon& rPolyPolygon)
{
	basegfx::B2DRange aRange(basegfx::tools::getRange(basegfx::tools::adaptiveSubdivideByAngle(rPolyPolygon)));

	return Rectangle(
		FRound(aRange.getMinX()), FRound(aRange.getMinY()), 
		FRound(aRange.getMaxX()), FRound(aRange.getMaxY()));
}

void SdrPathObj::ImpForceLineWink()
{
	if(OBJ_LINE == meKind && ImpIsLine(GetPathPoly()))
	{
		const basegfx::B2DPolygon aPoly(GetPathPoly().getB2DPolygon(0L));
		const basegfx::B2DPoint aB2DPoint0(aPoly.getB2DPoint(0L));
		const basegfx::B2DPoint aB2DPoint1(aPoly.getB2DPoint(1L));
		const Point aPoint0(FRound(aB2DPoint0.getX()), FRound(aB2DPoint0.getY()));
		const Point aPoint1(FRound(aB2DPoint1.getX()), FRound(aB2DPoint1.getY()));
		const Point aDelt(aPoint1 - aPoint0);

		aGeo.nDrehWink=GetAngle(aDelt);
		aGeo.nShearWink=0;
		aGeo.RecalcSinCos();
		aGeo.RecalcTan();

		// #101412# for SdrTextObj, keep aRect up to date
		aRect = Rectangle(aPoint0, aPoint1);
		aRect.Justify();
	}
}

void SdrPathObj::ImpForceKind()
{
	if (meKind==OBJ_PATHPLIN) meKind=OBJ_PLIN;
	if (meKind==OBJ_PATHPOLY) meKind=OBJ_POLY;

	if(GetPathPoly().areControlPointsUsed()) 
	{
		switch (meKind) 
		{
			case OBJ_LINE: meKind=OBJ_PATHLINE; break;
			case OBJ_PLIN: meKind=OBJ_PATHLINE; break;
			case OBJ_POLY: meKind=OBJ_PATHFILL; break;
			default: break;
		}
	} 
	else 
	{
		switch (meKind) 
		{
			case OBJ_PATHLINE: meKind=OBJ_PLIN; break;
			case OBJ_FREELINE: meKind=OBJ_PLIN; break;
			case OBJ_PATHFILL: meKind=OBJ_POLY; break;
			case OBJ_FREEFILL: meKind=OBJ_POLY; break;
			default: break;
		}
	}

	if (meKind==OBJ_LINE && !ImpIsLine(GetPathPoly())) meKind=OBJ_PLIN;
	if (meKind==OBJ_PLIN && ImpIsLine(GetPathPoly())) meKind=OBJ_LINE;

	bClosedObj=IsClosed();

	if (meKind==OBJ_LINE) 
	{
		ImpForceLineWink();
	}
	else
	{
		// #i10659#, similar to #101412# but for polys with more than 2 points.
		//
		// Here i again need to fix something, because when Path-Polys are Copy-Pasted
		// between Apps with different measurements (e.g. 100TH_MM and TWIPS) there is
		// a scaling loop started from SdrExchangeView::Paste. This is principally nothing
		// wrong, but aRect is wrong here and not even updated by RecalcSnapRect(). If
		// this is the case, some size needs to be set here in aRect to avoid that the cyclus
		// through Rect2Poly - Poly2Rect does something badly wrong since that cycle is
		// BASED on aRect. That cycle is triggered in SdrTextObj::NbcResize() which is called
		// from the local Resize() implementation.
		//
		// Basic problem is that the member aRect in SdrTextObj basically is a unrotated
		// text rectangle for the text object itself and methods at SdrTextObj do handle it
		// in that way. Many draw objects derived from SdrTextObj 'abuse' aRect as SnapRect
		// which is basically wrong. To make the SdrText methods which deal with aRect directly
		// work it is necessary to always keep aRect updated. This e.g. not done after a Clone()
		// command for SdrPathObj. Since adding this update mechanism with #101412# to
		// ImpForceLineWink() for lines was very successful, i add it to where ImpForceLineWink()
		// was called, once here below and once on a 2nd place below.

		// #i10659# for SdrTextObj, keep aRect up to date
		aRect = ImpGetBoundRect(GetPathPoly());
	}

	// #i75974# adapt polygon state to object type. This may include a reinterpretation
	// of a closed geometry as open one, but with identical first and last point
	for(sal_uInt32 a(0); a < maPathPolygon.count(); a++)
	{
		basegfx::B2DPolygon aCandidate(maPathPolygon.getB2DPolygon(a));

		if((bool)IsClosed() != aCandidate.isClosed())
		{
            // #i80213# really change polygon geometry; else e.g. the last point which
            // needs to be identical with the first one will be missing when opening
            // due to OBJ_PATH type
            if(aCandidate.isClosed())
            {
                basegfx::tools::openWithGeometryChange(aCandidate);
            }
            else
            {
                basegfx::tools::closeWithGeometryChange(aCandidate);
            }

            maPathPolygon.setB2DPolygon(a, aCandidate);
		}
	}
}

void SdrPathObj::ImpSetClosed(sal_Bool bClose)
{
	if(bClose) 
	{
		switch (meKind) 
		{
			case OBJ_LINE    : meKind=OBJ_POLY;     break;
			case OBJ_PLIN    : meKind=OBJ_POLY;     break;
			case OBJ_PATHLINE: meKind=OBJ_PATHFILL; break;
			case OBJ_FREELINE: meKind=OBJ_FREEFILL; break;
			case OBJ_SPLNLINE: meKind=OBJ_SPLNFILL; break;
			default: break;
		}
		
		bClosedObj = TRUE;
	} 
	else 
	{
		switch (meKind) 
		{
			case OBJ_POLY    : meKind=OBJ_PLIN;     break;
			case OBJ_PATHFILL: meKind=OBJ_PATHLINE; break;
			case OBJ_FREEFILL: meKind=OBJ_FREELINE; break;
			case OBJ_SPLNFILL: meKind=OBJ_SPLNLINE; break;
			default: break;
		}

		bClosedObj = FALSE;
	}

	ImpForceKind();
}

void SdrPathObj::TakeObjInfo(SdrObjTransformInfoRec& rInfo) const
{
	rInfo.bNoContortion=FALSE;

	FASTBOOL bCanConv = !HasText() || ImpCanConvTextToCurve();
	FASTBOOL bIsPath = IsBezier() || IsSpline();

	rInfo.bEdgeRadiusAllowed	= FALSE;
	rInfo.bCanConvToPath = bCanConv && !bIsPath;
	rInfo.bCanConvToPoly = bCanConv && bIsPath;
	rInfo.bCanConvToContour = !IsFontwork() && (rInfo.bCanConvToPoly || LineGeometryUsageIsNecessary());
}

UINT16 SdrPathObj::GetObjIdentifier() const
{
	return USHORT(meKind);
}

void SdrPathObj::RecalcBoundRect()
{
	aOutRect=GetSnapRect();
	long nLineWdt=ImpGetLineWdt();
	if (!IsClosed()) { // ggf. Linienenden beruecksichtigen
		long nLEndWdt=ImpGetLineEndAdd();
		if (nLEndWdt>nLineWdt) nLineWdt=nLEndWdt;
	}

	if(ImpAddLineGeomteryForMiteredLines())
	{
		nLineWdt = 0;
	}

	if (nLineWdt!=0) {
		aOutRect.Left  ()-=nLineWdt;
		aOutRect.Top   ()-=nLineWdt;
		aOutRect.Right ()+=nLineWdt;
		aOutRect.Bottom()+=nLineWdt;
	}
	ImpAddShadowToBoundRect();
	ImpAddTextToBoundRect();
}

sal_Bool SdrPathObj::DoPaintObject(XOutputDevice& rXOut, const SdrPaintInfoRec& rInfoRec) const
{
	const bool bHideContour(IsHideContour());

	// prepare ItemSet of this object
	const SfxItemSet& rSet = GetObjectItemSet();

#ifdef USE_JAVA
	// Fix bug 3512 by checking that the three shadow parameters exist
	SFX_ITEMSET_GET(rSet, pShadItem, SdrShadowItem, SDRATTR_SHADOW, TRUE);
	if (pShadItem)
	{
		SFX_ITEMSET_GET(rSet, pLineStartItem, XLineStartItem, XATTR_LINESTART, TRUE);
		if (!pLineStartItem)
			return sal_True;
	}
#endif	// USE_JAVA

	// perepare ItemSet to avoid old XOut line drawing
	SfxItemSet aEmptySet(*rSet.GetPool());
	aEmptySet.Put(XLineStyleItem(XLINE_NONE));
	aEmptySet.Put(XFillStyleItem(XFILL_NONE));

	// #b4899532# if not filled but fill draft, avoid object being invisible in using
	// a hair linestyle and COL_LIGHTGRAY
    SfxItemSet aItemSet(rSet);

	// #103692# prepare ItemSet for shadow fill attributes
    SfxItemSet aShadowSet(aItemSet);

	::std::auto_ptr< SdrLineGeometry > pLineGeometry( ImpPrepareLineGeometry(rXOut, aItemSet) );
	// Shadows
	if (!bHideContour && ImpSetShadowAttributes(aItemSet, aShadowSet))
	{
        if( !IsClosed() )
            rXOut.SetFillAttr(aEmptySet);
        else
            rXOut.SetFillAttr(aShadowSet);

		UINT32 nXDist=((SdrShadowXDistItem&)(aItemSet.Get(SDRATTR_SHADOWXDIST))).GetValue();
		UINT32 nYDist=((SdrShadowYDistItem&)(aItemSet.Get(SDRATTR_SHADOWYDIST))).GetValue();
		basegfx::B2DPolyPolygon aShadowPolyPolygon(GetPathPoly());
		basegfx::B2DHomMatrix aMatrix;
		aMatrix.translate(nXDist, nYDist);
		aShadowPolyPolygon.transform(aMatrix);

		// avoid shadow line drawing in XOut
		rXOut.SetLineAttr(aEmptySet);
		
        if(!IsClosed()) 
		{
            const sal_uInt32 nPolyAnz(aShadowPolyPolygon.count());

			for(sal_uInt32 nPolyNum(0L); nPolyNum < nPolyAnz; nPolyNum++) 
			{
                rXOut.DrawPolyLine(aShadowPolyPolygon.getB2DPolygon(nPolyNum));
            }
        } 
		else 
		{
            // #100127# Output original geometry for metafiles
            ImpGraphicFill aFill( *this, rXOut, aShadowSet, true );
            
            rXOut.DrawPolyPolygon(aShadowPolyPolygon);
        }

		// new shadow line drawing
		if( pLineGeometry.get() )
		{
			// draw the line geometry
			ImpDrawShadowLineGeometry(rXOut, aItemSet, *pLineGeometry);
		}
	}
	
	// Before here the LineAttr were set: if(pLineAttr) rXOut.SetLineAttr(*pLineAttr);
	// avoid line drawing in XOut
	rXOut.SetLineAttr(aEmptySet);
    rXOut.SetFillAttr( !IsClosed() ? aEmptySet : aItemSet );

	if( !bHideContour )
    {
        if( IsClosed() )
        {
            // #100127# Output original geometry for metafiles
            ImpGraphicFill aFill( *this, rXOut, !IsClosed() ? aEmptySet : aItemSet );

            rXOut.DrawPolyPolygon(GetPathPoly());
        }

        // Own line drawing
        if( pLineGeometry.get() )
        {
            // draw the line geometry
            ImpDrawColorLineGeometry(rXOut, aItemSet, *pLineGeometry);
        }
    }

	sal_Bool bOk(sal_True);
	if (HasText()) {
		bOk = SdrTextObj::DoPaintObject(rXOut,rInfoRec);
	}

	return bOk;
}

SdrObject* SdrPathObj::CheckHit(const Point& rPnt, USHORT nTol, const SetOfByte* pVisiLayer) const
{
	if(pVisiLayer && !pVisiLayer->IsSet(sal::static_int_cast< sal_uInt8 >(GetLayer()))) 
	{
		return NULL;
	}

	sal_Bool bHit(sal_False);
	const basegfx::B2DPoint aHitPoint(rPnt.X(), rPnt.Y());

	if(GetPathPoly().isClosed() && (bTextFrame || HasFill()))
	{
		// hit in filled polygon?
		if(GetPathPoly().areControlPointsUsed())
		{
			bHit = basegfx::tools::isInside(basegfx::tools::adaptiveSubdivideByAngle(GetPathPoly()), aHitPoint);
		}
		else
		{
			bHit = basegfx::tools::isInside(GetPathPoly(), aHitPoint);
		}
	}

	if(!bHit)
	{
		// hit polygon line?
		const double fHalfLineWidth(ImpGetLineWdt() / 2.0);
		double fDistance(nTol);

		if(fHalfLineWidth > fDistance)
		{
			fDistance = fHalfLineWidth;
		}

		bHit = basegfx::tools::isInEpsilonRange(GetPathPoly(), aHitPoint, fDistance);
	}

	if(!bHit && !IsTextFrame() && HasText()) 
	{
		bHit = (0L != SdrTextObj::CheckHit(rPnt,nTol,pVisiLayer));
	}

	return bHit ? (SdrObject*)this : 0L;
}

void SdrPathObj::operator=(const SdrObject& rObj)
{
	SdrTextObj::operator=(rObj);
	SdrPathObj& rPath=(SdrPathObj&)rObj;
	maPathPolygon=rPath.GetPathPoly();
}

void SdrPathObj::TakeObjNameSingul(XubString& rName) const
{
	if(OBJ_LINE == meKind) 
	{
		sal_uInt16 nId(STR_ObjNameSingulLINE);

		if(ImpIsLine(GetPathPoly()))
		{
			const basegfx::B2DPolygon aPoly(GetPathPoly().getB2DPolygon(0L));
			const basegfx::B2DPoint aB2DPoint0(aPoly.getB2DPoint(0L));
			const basegfx::B2DPoint aB2DPoint1(aPoly.getB2DPoint(1L));
			const Point aPoint0(FRound(aB2DPoint0.getX()), FRound(aB2DPoint0.getY()));
			const Point aPoint1(FRound(aB2DPoint0.getX()), FRound(aB2DPoint0.getY()));

			if(aB2DPoint0 != aB2DPoint1) 
			{
				if(aB2DPoint0.getY() == aB2DPoint1.getY()) 
				{
					nId = STR_ObjNameSingulLINE_Hori;
				} 
				else if(aB2DPoint0.getX() == aB2DPoint1.getX()) 
				{
					nId = STR_ObjNameSingulLINE_Vert;
				} 
				else 
				{
					const double fDx(fabs(aB2DPoint0.getX() - aB2DPoint1.getX()));
					const double fDy(fabs(aB2DPoint0.getY() - aB2DPoint1.getY()));

					if(fDx == fDy) 
					{
						nId = STR_ObjNameSingulLINE_Diag;
					}
				}
			}
		}

		rName = ImpGetResStr(nId);
	} 
	else if(OBJ_PLIN == meKind || OBJ_POLY == meKind) 
	{
		const sal_Bool bClosed(OBJ_POLY == meKind);
		sal_uInt16 nId(0);

		if(mpDAC && mpDAC->IsCreating()) 
		{ 
			if(bClosed) 
			{
				nId = STR_ObjNameSingulPOLY;
			} 
			else 
			{
				nId = STR_ObjNameSingulPLIN;
			}

			rName = ImpGetResStr(nId);
		} 
		else 
		{
			// get point count
			sal_uInt32 nPointCount(0L);
			const sal_uInt32 nPolyCount(GetPathPoly().count());

			for(sal_uInt32 a(0L); a < nPolyCount; a++)
			{
				nPointCount += GetPathPoly().getB2DPolygon(a).count();
			}

			if(bClosed) 
			{
				nId = STR_ObjNameSingulPOLY_PntAnz;
			} 
			else 
			{
				nId = STR_ObjNameSingulPLIN_PntAnz;
			}
			
			rName = ImpGetResStr(nId);
			sal_uInt16 nPos(rName.SearchAscii("%N"));

			if(STRING_NOTFOUND != nPos) 
			{
				rName.Erase(nPos, 2);
				rName.Insert(UniString::CreateFromInt32(nPointCount), nPos);
			}
		}
	} 
	else 
	{
		switch (meKind) 
		{
			case OBJ_PATHLINE: rName=ImpGetResStr(STR_ObjNameSingulPATHLINE); break;
			case OBJ_FREELINE: rName=ImpGetResStr(STR_ObjNameSingulFREELINE); break;
			case OBJ_SPLNLINE: rName=ImpGetResStr(STR_ObjNameSingulNATSPLN); break;
			case OBJ_PATHFILL: rName=ImpGetResStr(STR_ObjNameSingulPATHFILL); break;
			case OBJ_FREEFILL: rName=ImpGetResStr(STR_ObjNameSingulFREEFILL); break;
			case OBJ_SPLNFILL: rName=ImpGetResStr(STR_ObjNameSingulPERSPLN); break;
			default: break;
		}
	}

	String aName(GetName());
	if(aName.Len())
	{
		rName += sal_Unicode(' ');
		rName += sal_Unicode('\'');
		rName += aName;
		rName += sal_Unicode('\'');
	}
}

void SdrPathObj::TakeObjNamePlural(XubString& rName) const
{
	switch(meKind) 
	{
		case OBJ_LINE    : rName=ImpGetResStr(STR_ObjNamePluralLINE    ); break;
		case OBJ_PLIN    : rName=ImpGetResStr(STR_ObjNamePluralPLIN    ); break;
		case OBJ_POLY    : rName=ImpGetResStr(STR_ObjNamePluralPOLY    ); break;
		case OBJ_PATHLINE: rName=ImpGetResStr(STR_ObjNamePluralPATHLINE); break;
		case OBJ_FREELINE: rName=ImpGetResStr(STR_ObjNamePluralFREELINE); break;
		case OBJ_SPLNLINE: rName=ImpGetResStr(STR_ObjNamePluralNATSPLN); break;
		case OBJ_PATHFILL: rName=ImpGetResStr(STR_ObjNamePluralPATHFILL); break;
		case OBJ_FREEFILL: rName=ImpGetResStr(STR_ObjNamePluralFREEFILL); break;
		case OBJ_SPLNFILL: rName=ImpGetResStr(STR_ObjNamePluralPERSPLN); break;
		default: break;
	}
}

basegfx::B2DPolyPolygon SdrPathObj::TakeXorPoly(sal_Bool /*bDetail*/) const
{
	return GetPathPoly();
}

sal_uInt32 SdrPathObj::GetHdlCount() const
{
	sal_uInt32 nRetval(0L);
	const sal_uInt32 nPolyCount(GetPathPoly().count());

	for(sal_uInt32 a(0L); a < nPolyCount; a++)
	{
		nRetval += GetPathPoly().getB2DPolygon(a).count();
	}

	return nRetval;
}

SdrHdl* SdrPathObj::GetHdl(sal_uInt32 nHdlNum) const
{
	// #i73248#
	// Warn the user that this is ineffective and show alternatives. Should not be used at all.
	OSL_ENSURE(false, "SdrPathObj::GetHdl(): ineffective, use AddToHdlList instead (!)");

	// to have an alternative, get single handle using the ineffective way
	SdrHdl* pRetval = 0;
	SdrHdlList aLocalList(0);
	AddToHdlList(aLocalList);
	const sal_uInt32 nHdlCount(aLocalList.GetHdlCount());

	if(nHdlCount && nHdlNum < nHdlCount)
	{
		// remove and remember. The other created handles will be deleted again with the
		// destruction of the local list
		pRetval = aLocalList.RemoveHdl(nHdlNum);
	}

	return pRetval;
}

void SdrPathObj::AddToHdlList(SdrHdlList& rHdlList) const
{
	// keep old stuff to be able to keep old SdrHdl stuff, too
	const XPolyPolygon aOldPathPolygon(GetPathPoly());
	USHORT nPolyCnt=aOldPathPolygon.Count();
	FASTBOOL bClosed=IsClosed();
	USHORT nIdx=0;

	for (USHORT i=0; i<nPolyCnt; i++) {
		const XPolygon& rXPoly=aOldPathPolygon.GetObject(i);
		USHORT nPntCnt=rXPoly.GetPointCount();
		if (bClosed && nPntCnt>1) nPntCnt--;

		for (USHORT j=0; j<nPntCnt; j++) {
			if (rXPoly.GetFlags(j)!=XPOLY_CONTROL) {
				const Point& rPnt=rXPoly[j];
				SdrHdl* pHdl=new SdrHdl(rPnt,HDL_POLY);
				pHdl->SetPolyNum(i);
				pHdl->SetPointNum(j);
				pHdl->Set1PixMore(j==0);
				pHdl->SetSourceHdlNum(nIdx);
				nIdx++;
				rHdlList.AddHdl(pHdl);
			}
		}
	}
}

sal_uInt32 SdrPathObj::GetPlusHdlCount(const SdrHdl& rHdl) const
{
	// keep old stuff to be able to keep old SdrHdl stuff, too
	const XPolyPolygon aOldPathPolygon(GetPathPoly());
	sal_uInt16 nCnt = 0;
	sal_uInt16 nPnt = (sal_uInt16)rHdl.GetPointNum();
	sal_uInt16 nPolyNum = (sal_uInt16)rHdl.GetPolyNum();

	if(nPolyNum < aOldPathPolygon.Count()) 
	{
		const XPolygon& rXPoly = aOldPathPolygon[nPolyNum];
		sal_uInt16 nPntMax = rXPoly.GetPointCount();
		if (nPntMax>0) 
		{
			nPntMax--;
			if (nPnt<=nPntMax) 
			{
				if (rXPoly.GetFlags(nPnt)!=XPOLY_CONTROL) 
				{
					if (nPnt==0 && IsClosed()) nPnt=nPntMax;
					if (nPnt>0 && rXPoly.GetFlags(nPnt-1)==XPOLY_CONTROL) nCnt++;
					if (nPnt==nPntMax && IsClosed()) nPnt=0;
					if (nPnt<nPntMax && rXPoly.GetFlags(nPnt+1)==XPOLY_CONTROL) nCnt++;
				}
			}
		}
	}

	return nCnt;
}

SdrHdl* SdrPathObj::GetPlusHdl(const SdrHdl& rHdl, sal_uInt32 nPlusNum) const
{
	// keep old stuff to be able to keep old SdrHdl stuff, too
	const XPolyPolygon aOldPathPolygon(GetPathPoly());
	SdrHdl* pHdl = 0L;
	sal_uInt16 nPnt = (sal_uInt16)rHdl.GetPointNum();
	sal_uInt16 nPolyNum = (sal_uInt16)rHdl.GetPolyNum();

	if (nPolyNum<aOldPathPolygon.Count()) 
	{
		const XPolygon& rXPoly = aOldPathPolygon[nPolyNum];
		sal_uInt16 nPntMax = rXPoly.GetPointCount();

		if (nPntMax>0) 
		{
			nPntMax--;
			if (nPnt<=nPntMax) 
			{
				pHdl=new SdrHdlBezWgt(&rHdl);
				pHdl->SetPolyNum(rHdl.GetPolyNum());

				if (nPnt==0 && IsClosed()) nPnt=nPntMax;
				if (nPnt>0 && rXPoly.GetFlags(nPnt-1)==XPOLY_CONTROL && nPlusNum==0) 
				{
					pHdl->SetPos(rXPoly[nPnt-1]);
					pHdl->SetPointNum(nPnt-1);
				} 
				else 
				{
					if (nPnt==nPntMax && IsClosed()) nPnt=0;
					if (nPnt<rXPoly.GetPointCount()-1 && rXPoly.GetFlags(nPnt+1)==XPOLY_CONTROL) 
					{
						pHdl->SetPos(rXPoly[nPnt+1]);
						pHdl->SetPointNum(nPnt+1);
					}
				}

				pHdl->SetSourceHdlNum(rHdl.GetSourceHdlNum());
				pHdl->SetPlusHdl(TRUE);
			}
		}
	}
	return pHdl;
}

FASTBOOL SdrPathObj::HasSpecialDrag() const
{
	return TRUE;
}

FASTBOOL SdrPathObj::BegDrag(SdrDragStat& rDrag) const
{
	impDeleteDAC();
	return impGetDAC().BegDrag(rDrag);
}

FASTBOOL SdrPathObj::MovDrag(SdrDragStat& rDrag) const
{
	return impGetDAC().MovDrag(rDrag);
}

FASTBOOL SdrPathObj::EndDrag(SdrDragStat& rDrag)
{
	FASTBOOL bRetval(impGetDAC().EndDrag(rDrag));

	if(bRetval && mpDAC)
	{
		SetPathPoly(mpDAC->getModifiedPolyPolygon());
		impDeleteDAC();
	}

	return bRetval;
}

void SdrPathObj::BrkDrag(SdrDragStat& rDrag) const
{
	impGetDAC().BrkDrag(rDrag);
	impDeleteDAC();
}

XubString SdrPathObj::GetDragComment(const SdrDragStat& rDrag, FASTBOOL bUndoDragComment, FASTBOOL bCreateComment) const
{
	return impGetDAC().GetDragComment(rDrag, bUndoDragComment, bCreateComment);
}

basegfx::B2DPolyPolygon SdrPathObj::TakeDragPoly(const SdrDragStat& rDrag) const
{
	return impGetDAC().TakeDragPoly(rDrag);
}

FASTBOOL SdrPathObj::BegCreate(SdrDragStat& rStat)
{
	impDeleteDAC();
	return impGetDAC().BegCreate(rStat);
}

FASTBOOL SdrPathObj::MovCreate(SdrDragStat& rStat)
{
	return impGetDAC().MovCreate(rStat);
}

FASTBOOL SdrPathObj::EndCreate(SdrDragStat& rStat, SdrCreateCmd eCmd)
{
	FASTBOOL bRetval(impGetDAC().EndCreate(rStat, eCmd));

	if(bRetval && mpDAC)
	{
		SetPathPoly(mpDAC->getModifiedPolyPolygon());

		// #i75974# Check for AutoClose feature. Moved here from ImpPathForDragAndCreate::EndCreate
		// to be able to use the type-changing ImpSetClosed method
		if(!IsClosedObj())
		{
			SdrView* pView = rStat.GetView();

			if(pView && pView->IsAutoClosePolys() && !pView->IsUseIncompatiblePathCreateInterface())
			{
				OutputDevice* pOut = pView->GetFirstOutputDevice();

				if(pOut)
				{
					if(GetPathPoly().count())
					{
						const basegfx::B2DPolygon aCandidate(GetPathPoly().getB2DPolygon(0));

						if(aCandidate.count() > 2)
						{
							// check distance of first and last point
							const sal_Int32 nCloseDist(pOut->PixelToLogic(Size(pView->GetAutoCloseDistPix(), 0)).Width());
							const basegfx::B2DVector aDistVector(aCandidate.getB2DPoint(aCandidate.count() - 1) - aCandidate.getB2DPoint(0));

							if(aDistVector.getLength() <= (double)nCloseDist)
							{
								// close it
								ImpSetClosed(true);
							}
						}
					}
				}
			}
		}

		impDeleteDAC();
	}

	return bRetval;
}

FASTBOOL SdrPathObj::BckCreate(SdrDragStat& rStat)
{
	return impGetDAC().BckCreate(rStat);
}

void SdrPathObj::BrkCreate(SdrDragStat& rStat)
{
	impGetDAC().BrkCreate(rStat);
	impDeleteDAC();
}

basegfx::B2DPolyPolygon SdrPathObj::TakeCreatePoly(const SdrDragStat& rDrag) const
{
	basegfx::B2DPolyPolygon aRetval;
	
	if(mpDAC)
	{
		aRetval = mpDAC->TakeObjectPolyPolygon(rDrag);
		aRetval.append(mpDAC->TakeDragPolyPolygon(rDrag));
	}

	return aRetval;
}

// during drag or create, allow accessing the so-far created/modified polyPolygon
basegfx::B2DPolyPolygon SdrPathObj::getObjectPolyPolygon(const SdrDragStat& rDrag) const
{
	basegfx::B2DPolyPolygon aRetval;

	if(mpDAC)
	{
		aRetval = mpDAC->TakeObjectPolyPolygon(rDrag);
	}

	return aRetval;
}

basegfx::B2DPolyPolygon SdrPathObj::getDragPolyPolygon(const SdrDragStat& rDrag) const
{
	basegfx::B2DPolyPolygon aRetval;

	if(mpDAC)
	{
		aRetval = mpDAC->TakeDragPolyPolygon(rDrag);
	}

	return aRetval;
}

Pointer SdrPathObj::GetCreatePointer() const
{
	return impGetDAC().GetCreatePointer();
}

void SdrPathObj::NbcMove(const Size& rSiz)
{
	SdrTextObj::NbcMove(rSiz);

	basegfx::B2DHomMatrix aTrans;
	aTrans.translate(rSiz.Width(), rSiz.Height());
	maPathPolygon.transform(aTrans);
}

void SdrPathObj::NbcResize(const Point& rRef, const Fraction& xFact, const Fraction& yFact)
{
	SdrTextObj::NbcResize(rRef,xFact,yFact);

	basegfx::B2DHomMatrix aTrans;
	aTrans.translate(-rRef.X(), -rRef.Y());
	aTrans.scale(double(xFact), double(yFact));
	aTrans.translate(rRef.X(), rRef.Y());
	maPathPolygon.transform(aTrans);
}

void SdrPathObj::NbcRotate(const Point& rRef, long nWink, double sn, double cs)
{
	SdrTextObj::NbcRotate(rRef,nWink,sn,cs);

	basegfx::B2DHomMatrix aTrans;
	aTrans.translate(-rRef.X(), -rRef.Y());
	aTrans.rotate(-nWink * nPi180); // Thank JOE, the angles are defined mirrored to the mathematical meanings
	aTrans.translate(rRef.X(), rRef.Y());
	maPathPolygon.transform(aTrans);
}

void SdrPathObj::NbcShear(const Point& rRefPnt, long nAngle, double fTan, FASTBOOL bVShear)
{
	SdrTextObj::NbcShear(rRefPnt,nAngle,fTan,bVShear);

	basegfx::B2DHomMatrix aTrans;
	aTrans.translate(-rRefPnt.X(), -rRefPnt.Y());
	aTrans.shearX(-fTan); // Thank JOE, the angles are defined mirrored to the mathematical meanings
	aTrans.translate(rRefPnt.X(), rRefPnt.Y());
	maPathPolygon.transform(aTrans);
}

void SdrPathObj::NbcMirror(const Point& rRefPnt1, const Point& rRefPnt2)
{
	SdrTextObj::NbcMirror(rRefPnt1,rRefPnt2);

	basegfx::B2DHomMatrix aTrans;
	const double fDiffX(rRefPnt2.X() - rRefPnt1.X());
	const double fDiffY(rRefPnt2.Y() - rRefPnt1.Y());
	const double fRot(atan2(fDiffY, fDiffX));
	aTrans.translate(-rRefPnt1.X(), -rRefPnt1.Y());
	aTrans.rotate(-fRot);
	aTrans.scale(1.0, -1.0);
	aTrans.rotate(fRot);
	aTrans.translate(rRefPnt1.X(), rRefPnt1.Y());
	maPathPolygon.transform(aTrans);

	// #97538# Do Joe's special handling for lines when mirroring, too
	ImpForceKind();
}

void SdrPathObj::TakeUnrotatedSnapRect(Rectangle& rRect) const
{
	if (aGeo.nDrehWink==0) {
		rRect=GetSnapRect();
	} else {
		XPolyPolygon aXPP(GetPathPoly());
		RotateXPoly(aXPP,Point(),-aGeo.nSin,aGeo.nCos);
		rRect=aXPP.GetBoundRect();
		Point aTmp(rRect.TopLeft());
		RotatePoint(aTmp,Point(),aGeo.nSin,aGeo.nCos);
		aTmp-=rRect.TopLeft();
		rRect.Move(aTmp.X(),aTmp.Y());
	}
}

void SdrPathObj::RecalcSnapRect()
{
	maSnapRect = ImpGetBoundRect(GetPathPoly());
}

void SdrPathObj::NbcSetSnapRect(const Rectangle& rRect)
{
	Rectangle aOld(GetSnapRect());

	// #95736# Take RECT_EMPTY into account when calculating scale factors
	long nMulX = (RECT_EMPTY == rRect.Right()) ? 0 : rRect.Right()  - rRect.Left();
	
	long nDivX = aOld.Right()   - aOld.Left();
	
	// #95736# Take RECT_EMPTY into account when calculating scale factors
	long nMulY = (RECT_EMPTY == rRect.Bottom()) ? 0 : rRect.Bottom() - rRect.Top();
	
	long nDivY = aOld.Bottom()  - aOld.Top();
	if ( nDivX == 0 ) { nMulX = 1; nDivX = 1; }
	if ( nDivY == 0 ) { nMulY = 1; nDivY = 1; }
	Fraction aX(nMulX,nDivX);
	Fraction aY(nMulY,nDivY);
	NbcResize(aOld.TopLeft(), aX, aY);
	NbcMove(Size(rRect.Left() - aOld.Left(), rRect.Top() - aOld.Top()));
}

sal_uInt32 SdrPathObj::GetSnapPointCount() const
{
	return GetHdlCount();
}

Point SdrPathObj::GetSnapPoint(sal_uInt32 nSnapPnt) const
{
	sal_uInt32 nPoly,nPnt;
	if(!PolyPolygonEditor::GetRelativePolyPoint(GetPathPoly(), nSnapPnt, nPoly, nPnt)) 
	{
		DBG_ASSERT(FALSE,"SdrPathObj::GetSnapPoint: Punkt nSnapPnt nicht vorhanden!");
	}

	const basegfx::B2DPoint aB2DPoint(GetPathPoly().getB2DPolygon(nPoly).getB2DPoint(nPnt));
	return Point(FRound(aB2DPoint.getX()), FRound(aB2DPoint.getY()));
}

sal_Bool SdrPathObj::IsPolyObj() const
{
	return sal_True;
}

sal_uInt32 SdrPathObj::GetPointCount() const
{
	const sal_uInt32 nPolyCount(GetPathPoly().count());
	sal_uInt32 nRetval(0L);

	for(sal_uInt32 a(0L); a < nPolyCount; a++)
	{
		nRetval += GetPathPoly().getB2DPolygon(a).count();
	}

	return nRetval;
}

Point SdrPathObj::GetPoint(sal_uInt32 nHdlNum) const
{
	Point aRetval;
	sal_uInt32 nPoly,nPnt;

	if(PolyPolygonEditor::GetRelativePolyPoint(GetPathPoly(), nHdlNum, nPoly, nPnt))
	{
		const basegfx::B2DPolygon aPoly(GetPathPoly().getB2DPolygon(nPoly));
		const basegfx::B2DPoint aPoint(aPoly.getB2DPoint(nPnt));
		aRetval = Point(FRound(aPoint.getX()), FRound(aPoint.getY()));
	}

	return aRetval;
}

void SdrPathObj::NbcSetPoint(const Point& rPnt, sal_uInt32 nHdlNum)
{
	sal_uInt32 nPoly,nPnt;

	if(PolyPolygonEditor::GetRelativePolyPoint(GetPathPoly(), nHdlNum, nPoly, nPnt)) 
	{
		basegfx::B2DPolygon aNewPolygon(GetPathPoly().getB2DPolygon(nPoly));
		aNewPolygon.setB2DPoint(nPnt, basegfx::B2DPoint(rPnt.X(), rPnt.Y()));
		maPathPolygon.setB2DPolygon(nPoly, aNewPolygon);

		if(meKind==OBJ_LINE)
		{
			ImpForceLineWink();
		}
		else
		{
			// #i10659# for SdrTextObj, keep aRect up to date
			aRect = ImpGetBoundRect(GetPathPoly()); // fuer SdrTextObj
		}

		SetRectsDirty();
	}
}

sal_uInt32 SdrPathObj::NbcInsPointOld(const Point& rPos, sal_Bool bNewObj, sal_Bool bHideHim)
{
	sal_uInt32 nNewHdl;

	if(bNewObj) 
	{
		nNewHdl = NbcInsPoint(0L, rPos, sal_True, bHideHim);
	} 
	else 
	{
		// look for smallest distance data
		const basegfx::B2DPoint aTestPoint(rPos.X(), rPos.Y());
		sal_uInt32 nSmallestPolyIndex(0L);
		sal_uInt32 nSmallestEdgeIndex(0L);
		double fSmallestCut;
		basegfx::tools::getSmallestDistancePointToPolyPolygon(GetPathPoly(), aTestPoint, nSmallestPolyIndex, nSmallestEdgeIndex, fSmallestCut);

		// create old polygon index from it
		sal_uInt32 nPolyIndex(nSmallestEdgeIndex);

		for(sal_uInt32 a(0L); a < nSmallestPolyIndex; a++)
		{
			nPolyIndex += GetPathPoly().getB2DPolygon(a).count();
		}

		nNewHdl = NbcInsPoint(nPolyIndex, rPos, sal_False, bHideHim);
	}
	
	ImpForceKind();
	return nNewHdl;
}

sal_uInt32 SdrPathObj::NbcInsPoint(sal_uInt32 /*nHdlNum*/, const Point& rPos, sal_Bool bNewObj, sal_Bool /*bHideHim*/)
{
	sal_uInt32 nNewHdl;

	if(bNewObj) 
	{
		basegfx::B2DPolygon aNewPoly;
		const basegfx::B2DPoint aPoint(rPos.X(), rPos.Y());
		aNewPoly.append(aPoint);
		aNewPoly.setClosed(IsClosed());
		maPathPolygon.append(aNewPoly);
		SetRectsDirty();
		nNewHdl = GetHdlCount();
	} 
	else 
	{
		// look for smallest distance data
		const basegfx::B2DPoint aTestPoint(rPos.X(), rPos.Y());
		sal_uInt32 nSmallestPolyIndex(0L);
		sal_uInt32 nSmallestEdgeIndex(0L);
		double fSmallestCut;
		basegfx::tools::getSmallestDistancePointToPolyPolygon(GetPathPoly(), aTestPoint, nSmallestPolyIndex, nSmallestEdgeIndex, fSmallestCut);
		basegfx::B2DPolygon aCandidate(GetPathPoly().getB2DPolygon(nSmallestPolyIndex));
		const bool bBefore(!aCandidate.isClosed() && 0L == nSmallestEdgeIndex && 0.0 == fSmallestCut);
		const bool bAfter(!aCandidate.isClosed() && aCandidate.count() == nSmallestEdgeIndex + 2L && 1.0 == fSmallestCut);

		if(bBefore)
		{
			// before first point
			aCandidate.insert(0L, aTestPoint);
			
			if(aCandidate.areControlPointsUsed())
			{
				if(aCandidate.isNextControlPointUsed(1))
				{
					aCandidate.setNextControlPoint(0, interpolate(aTestPoint, aCandidate.getB2DPoint(1), (1.0 / 3.0)));
					aCandidate.setPrevControlPoint(1, interpolate(aTestPoint, aCandidate.getB2DPoint(1), (2.0 / 3.0)));
				}
			}
			
			nNewHdl = 0L;
		}
		else if(bAfter)
		{
			// after last point
			aCandidate.append(aTestPoint);

			if(aCandidate.areControlPointsUsed())
			{
				if(aCandidate.isPrevControlPointUsed(aCandidate.count() - 2))
				{
					aCandidate.setNextControlPoint(aCandidate.count() - 2, interpolate(aCandidate.getB2DPoint(aCandidate.count() - 2), aTestPoint, (1.0 / 3.0)));
					aCandidate.setPrevControlPoint(aCandidate.count() - 1, interpolate(aCandidate.getB2DPoint(aCandidate.count() - 2), aTestPoint, (2.0 / 3.0)));
				}
			}

			nNewHdl = aCandidate.count() - 1L;
		}
		else
		{
			// in between
			bool bSegmentSplit(false);
			const sal_uInt32 nNextIndex((nSmallestEdgeIndex + 1) % aCandidate.count());

			if(aCandidate.areControlPointsUsed())
			{
				if(aCandidate.isNextControlPointUsed(nSmallestEdgeIndex) || aCandidate.isPrevControlPointUsed(nNextIndex))
				{
					bSegmentSplit = true;
				}
			}

			if(bSegmentSplit)
			{
				// rebuild original segment to get the split data
				basegfx::B2DCubicBezier aBezierA, aBezierB;
				const basegfx::B2DCubicBezier aBezier(
					aCandidate.getB2DPoint(nSmallestEdgeIndex),
					aCandidate.getNextControlPoint(nSmallestEdgeIndex),
					aCandidate.getPrevControlPoint(nNextIndex),
					aCandidate.getB2DPoint(nNextIndex));

				// split and insert hit point
				aBezier.split(fSmallestCut, aBezierA, aBezierB);
				aCandidate.insert(nSmallestEdgeIndex + 1, aTestPoint);

				// since we inserted hit point and not split point, we need to add an offset
				// to the control points to get the C1 continuity we want to achieve
				const basegfx::B2DVector aOffset(aTestPoint - aBezierA.getEndPoint());
				aCandidate.setNextControlPoint(nSmallestEdgeIndex, aBezierA.getControlPointA() + aOffset);
				aCandidate.setPrevControlPoint(nSmallestEdgeIndex + 1, aBezierA.getControlPointB() + aOffset);
				aCandidate.setNextControlPoint(nSmallestEdgeIndex + 1, aBezierB.getControlPointA() + aOffset);
				aCandidate.setPrevControlPoint((nSmallestEdgeIndex + 2) % aCandidate.count(), aBezierB.getControlPointB() + aOffset);
			}
			else
			{
				aCandidate.insert(nSmallestEdgeIndex + 1L, aTestPoint);
			}

			nNewHdl = nSmallestEdgeIndex + 1L;
		}

		maPathPolygon.setB2DPolygon(nSmallestPolyIndex, aCandidate);

		// create old polygon index from it
		for(sal_uInt32 a(0L); a < nSmallestPolyIndex; a++)
		{
			nNewHdl += GetPathPoly().getB2DPolygon(a).count();
		}
	}

	ImpForceKind();
	return nNewHdl;
}

SdrObject* SdrPathObj::RipPoint(sal_uInt32 nHdlNum, sal_uInt32& rNewPt0Index)
{
	SdrPathObj* pNewObj = 0L;
	const basegfx::B2DPolyPolygon aLocalPolyPolygon(GetPathPoly());
	sal_uInt32 nPoly, nPnt;

	if(PolyPolygonEditor::GetRelativePolyPoint(aLocalPolyPolygon, nHdlNum, nPoly, nPnt)) 
	{
		if(0L == nPoly)
		{
			const basegfx::B2DPolygon aCandidate(aLocalPolyPolygon.getB2DPolygon(nPoly));
			const sal_uInt32 nPointCount(aCandidate.count());

			if(nPointCount)
			{
				if(IsClosed())
				{
					// when closed, RipPoint means to open the polygon at the selected point. To
					// be able to do that, it is necessary to make the selected point the first one
					basegfx::B2DPolygon aNewPolygon(basegfx::tools::makeStartPoint(aCandidate, nPnt));
					SetPathPoly(basegfx::B2DPolyPolygon(aNewPolygon));
					ToggleClosed();

					// give back new position of old start point (historical reasons)
					rNewPt0Index = (nPointCount - nPnt) % nPointCount;
				}
				else
				{
					if(nPointCount >= 3L && nPnt != 0L && nPnt + 1L < nPointCount)
					{
						// split in two objects at point nPnt
						basegfx::B2DPolygon aSplitPolyA(aCandidate, 0L, nPnt + 1L);
						SetPathPoly(basegfx::B2DPolyPolygon(aSplitPolyA));

						pNewObj = (SdrPathObj*)Clone();
						basegfx::B2DPolygon aSplitPolyB(aCandidate, nPnt, nPointCount - nPnt);
						pNewObj->SetPathPoly(basegfx::B2DPolyPolygon(aSplitPolyB));
					}
				}
			}
		}
	}

	return pNewObj;
}

SdrObject* SdrPathObj::DoConvertToPolyObj(BOOL bBezier) const
{
	SdrObject* pRet = ImpConvertMakeObj(GetPathPoly(), IsClosed(), bBezier);
	SdrPathObj* pPath = PTR_CAST(SdrPathObj, pRet);

	if(pPath) 
	{
		if(pPath->GetPathPoly().areControlPointsUsed())
		{
			if(!bBezier)
			{
				// reduce all bezier curves
				pPath->SetPathPoly(basegfx::tools::adaptiveSubdivideByAngle(pPath->GetPathPoly()));
			}
		}
		else
		{
			if(bBezier)
			{
				// create bezier curves
				pPath->SetPathPoly(basegfx::tools::expandToCurve(pPath->GetPathPoly()));
			}
		}
	}

	pRet = ImpConvertAddText(pRet, bBezier);

	return pRet;
}

SdrObjGeoData* SdrPathObj::NewGeoData() const
{
	return new SdrPathObjGeoData;
}

void SdrPathObj::SaveGeoData(SdrObjGeoData& rGeo) const
{
	SdrTextObj::SaveGeoData(rGeo);
	SdrPathObjGeoData& rPGeo = (SdrPathObjGeoData&) rGeo;
	rPGeo.maPathPolygon=GetPathPoly();
	rPGeo.meKind=meKind;
}

void SdrPathObj::RestGeoData(const SdrObjGeoData& rGeo)
{
	SdrTextObj::RestGeoData(rGeo);
	SdrPathObjGeoData& rPGeo=(SdrPathObjGeoData&)rGeo;
	maPathPolygon=rPGeo.maPathPolygon;
	meKind=rPGeo.meKind;
	ImpForceKind(); // damit u.a. bClosed gesetzt wird
}

void SdrPathObj::NbcSetPathPoly(const basegfx::B2DPolyPolygon& rPathPoly)
{
	if(GetPathPoly() != rPathPoly)
	{
		maPathPolygon=rPathPoly;
		ImpForceKind();
		SetRectsDirty();
	}
}

void SdrPathObj::SetPathPoly(const basegfx::B2DPolyPolygon& rPathPoly)
{
	if(GetPathPoly() != rPathPoly)
	{
		Rectangle aBoundRect0; if (pUserCall!=NULL) aBoundRect0=GetLastBoundRect();
		NbcSetPathPoly(rPathPoly);
		SetChanged();
		BroadcastObjectChange();
		SendUserCall(SDRUSERCALL_RESIZE,aBoundRect0);
	}
}

void SdrPathObj::ToggleClosed() // long nOpenDistance)
{
	Rectangle aBoundRect0;
	if(pUserCall != NULL)
		aBoundRect0 = GetLastBoundRect();
	ImpSetClosed(!IsClosed()); // neuen ObjKind setzen
	ImpForceKind(); // wg. Line->Poly->PolyLine statt Line->Poly->Line
	SetRectsDirty();
	SetChanged();
	BroadcastObjectChange();
	SendUserCall(SDRUSERCALL_RESIZE, aBoundRect0);
}

// fuer friend class SdrPolyEditView auf einigen Compilern:
void SdrPathObj::SetRectsDirty(sal_Bool bNotMyself) 
{ 
	SdrTextObj::SetRectsDirty(bNotMyself); 
}

ImpPathForDragAndCreate& SdrPathObj::impGetDAC() const
{
	if(!mpDAC)
	{
		((SdrPathObj*)this)->mpDAC = new ImpPathForDragAndCreate(*((SdrPathObj*)this));
	}

	return *mpDAC;
}

void SdrPathObj::impDeleteDAC() const
{
	if(mpDAC)
	{
		delete mpDAC;
		((SdrPathObj*)this)->mpDAC = 0L;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// transformation interface for StarOfficeAPI. This implements support for 
// homogen 3x3 matrices containing the transformation of the SdrObject. At the
// moment it contains a shearX, rotation and translation, but for setting all linear 
// transforms like Scale, ShearX, ShearY, Rotate and Translate are supported.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
// gets base transformation and rectangle of object. If it's an SdrPathObj it fills the PolyPolygon
// with the base geometry and returns TRUE. Otherwise it returns FALSE.
sal_Bool SdrPathObj::TRGetBaseGeometry(basegfx::B2DHomMatrix& rMatrix, basegfx::B2DPolyPolygon& rPolyPolygon) const
{
	double fRotate(0.0);
	double fShearX(0.0);
	basegfx::B2DTuple aScale(1.0, 1.0);
	basegfx::B2DTuple aTranslate(0.0, 0.0);

	if(GetPathPoly().count())
	{
		// copy geometry
		basegfx::B2DHomMatrix aMoveToZeroMatrix;
		rPolyPolygon = GetPathPoly();

		if(OBJ_LINE == meKind)
		{
			// ignore shear and rotate, just use scale and translate
			OSL_ENSURE(GetPathPoly().count() > 0L && GetPathPoly().getB2DPolygon(0L).count() > 1L, "OBJ_LINE with too less polygons (!)");
			// #i72287# use polygon without control points for range calculation. Do not change rPolyPolygon
			// itself, else this method will no longer return the full polygon information (curve will
			// be lost)
			const basegfx::B2DPolyPolygon aPolyPolygonNoCurve(basegfx::tools::adaptiveSubdivideByAngle(rPolyPolygon));
			const basegfx::B2DRange aPolyRangeNoCurve(basegfx::tools::getRange(aPolyPolygonNoCurve));
			aScale = aPolyRangeNoCurve.getRange();
			aTranslate = aPolyRangeNoCurve.getMinimum();

			// define matrix for move polygon to zero point
			aMoveToZeroMatrix.translate(-aTranslate.getX(), -aTranslate.getY());
		}
		else
		{
			if(aGeo.nShearWink || aGeo.nDrehWink)
			{
				// get rotate and shear in drawingLayer notation
				fRotate = aGeo.nDrehWink * F_PI18000;
				fShearX = aGeo.nShearWink * F_PI18000;

				// build mathematically correct (negative shear and rotate) object transform 
				// containing shear and rotate to extract unsheared, unrotated polygon
				basegfx::B2DHomMatrix aObjectMatrix;
				aObjectMatrix.shearX(tan((36000 - aGeo.nShearWink) * F_PI18000));
				aObjectMatrix.rotate((36000 - aGeo.nDrehWink) * F_PI18000);

				// create inverse from it and back-transform polygon
				basegfx::B2DHomMatrix aInvObjectMatrix(aObjectMatrix);
				aInvObjectMatrix.invert();
				rPolyPolygon.transform(aInvObjectMatrix);

				// get range from unsheared, unrotated polygon and extract scale and translate.
				// transform topLeft from it back to transformed state to get original 
				// topLeft (rotation center)
				// #i72287# use polygon without control points for range calculation. Do not change rPolyPolygon
				// itself, else this method will no longer return the full polygon information (curve will
				// be lost)
				const basegfx::B2DPolyPolygon aPolyPolygonNoCurve(basegfx::tools::adaptiveSubdivideByAngle(rPolyPolygon));
				const basegfx::B2DRange aCorrectedRangeNoCurve(basegfx::tools::getRange(aPolyPolygonNoCurve));
				aTranslate = aObjectMatrix * aCorrectedRangeNoCurve.getMinimum();
				aScale = aCorrectedRangeNoCurve.getRange();
				
				// define matrix for move polygon to zero point
				aMoveToZeroMatrix.translate(-aCorrectedRangeNoCurve.getMinX(), aCorrectedRangeNoCurve.getMinY());
			}
			else
			{
				// get scale and translate from unsheared, unrotated polygon
				// #i72287# use polygon without control points for range calculation. Do not change rPolyPolygon
				// itself, else this method will no longer return the full polygon information (curve will
				// be lost)
				const basegfx::B2DPolyPolygon aPolyPolygonNoCurve(basegfx::tools::adaptiveSubdivideByAngle(rPolyPolygon));
				const basegfx::B2DRange aPolyRangeNoCurve(basegfx::tools::getRange(aPolyPolygonNoCurve));
				aScale = aPolyRangeNoCurve.getRange();
				aTranslate = aPolyRangeNoCurve.getMinimum();

				// define matrix for move polygon to zero point
				aMoveToZeroMatrix.translate(-aTranslate.getX(), -aTranslate.getY());
			}
		}

		// move polygon to zero point with pre-defined matrix
		rPolyPolygon.transform(aMoveToZeroMatrix);
	}

	// position maybe relative to anchorpos, convert
	if( pModel->IsWriter() )
	{
		if(GetAnchorPos().X() || GetAnchorPos().Y())
		{
			aTranslate -= basegfx::B2DTuple(GetAnchorPos().X(), GetAnchorPos().Y());
		}
	}

	// force MapUnit to 100th mm
	SfxMapUnit eMapUnit = pModel->GetItemPool().GetMetric(0);
	if(eMapUnit != SFX_MAPUNIT_100TH_MM)
	{
		switch(eMapUnit)
		{
			case SFX_MAPUNIT_TWIP :
			{
				// postion
				aTranslate.setX(ImplTwipsToMM(aTranslate.getX()));
				aTranslate.setY(ImplTwipsToMM(aTranslate.getY()));

				// size
				aScale.setX(ImplTwipsToMM(aScale.getX()));
				aScale.setY(ImplTwipsToMM(aScale.getY()));

				// polygon
				basegfx::B2DHomMatrix aTwipsToMM;
				const double fFactorTwipsToMM(127.0 / 72.0);
				aTwipsToMM.scale(fFactorTwipsToMM, fFactorTwipsToMM);
				rPolyPolygon.transform(aTwipsToMM);

				break;
			}
			default:
			{
				DBG_ERROR("TRGetBaseGeometry: Missing unit translation to 100th mm!");
			}
		}
	}

	// build return value matrix
	rMatrix.identity();

    if(!basegfx::fTools::equal(aScale.getX(), 1.0) || !basegfx::fTools::equal(aScale.getY(), 1.0))
    {
    	rMatrix.scale(aScale.getX(), aScale.getY());
    }

    if(!basegfx::fTools::equalZero(fShearX))
    {
    	rMatrix.shearX(tan(fShearX));
    }

    if(!basegfx::fTools::equalZero(fRotate))
    {
        // #i78696#
        // fRotate is from the old GeoStat and thus mathematically wrong orientated. For 
        // the linear combination of matrices it needed to be fixed in the API, so it needs to
        // be mirrored here
        rMatrix.rotate(-fRotate);
    }

    if(!aTranslate.equalZero())
    {
        rMatrix.translate(aTranslate.getX(), aTranslate.getY());
    }

	return sal_True;
}

// sets the base geometry of the object using infos contained in the homogen 3x3 matrix. 
// If it's an SdrPathObj it will use the provided geometry information. The Polygon has 
// to use (0,0) as upper left and will be scaled to the given size in the matrix.
void SdrPathObj::TRSetBaseGeometry(const basegfx::B2DHomMatrix& rMatrix, const basegfx::B2DPolyPolygon& rPolyPolygon)
{
	// break up matrix
	basegfx::B2DTuple aScale;
	basegfx::B2DTuple aTranslate;
	double fRotate, fShearX;
	rMatrix.decompose(aScale, aTranslate, fRotate, fShearX);

	// #i75086# Old DrawingLayer (GeoStat and geometry) does not support holding negative scalings
	// in X and Y which equal a 180 degree rotation. Recognize it and react accordingly
	if(basegfx::fTools::less(aScale.getX(), 0.0) && basegfx::fTools::less(aScale.getY(), 0.0))
	{
		aScale.setX(fabs(aScale.getX()));
		aScale.setY(fabs(aScale.getY()));
		fRotate = fmod(fRotate + F_PI, F_2PI);
	}

	// copy poly
	basegfx::B2DPolyPolygon aNewPolyPolygon(rPolyPolygon);

	// reset object shear and rotations
	aGeo.nDrehWink = 0;
	aGeo.RecalcSinCos();
	aGeo.nShearWink = 0;
	aGeo.RecalcTan();

	// force metric to pool metric
	SfxMapUnit eMapUnit = pModel->GetItemPool().GetMetric(0);
	if(eMapUnit != SFX_MAPUNIT_100TH_MM)
	{
		switch(eMapUnit)
		{
			case SFX_MAPUNIT_TWIP :
			{
				// position
				aTranslate.setX(ImplMMToTwips(aTranslate.getX()));
				aTranslate.setY(ImplMMToTwips(aTranslate.getY()));

				// size
				aScale.setX(ImplMMToTwips(aScale.getX()));
				aScale.setY(ImplMMToTwips(aScale.getY()));
				
				// polygon
				basegfx::B2DHomMatrix aMMToTwips;
				const double fFactorMMToTwips(72.0 / 127.0);
				aMMToTwips.scale(fFactorMMToTwips, fFactorMMToTwips);
				aNewPolyPolygon.transform(aMMToTwips);

				break;
			}
			default:
			{
				DBG_ERROR("TRSetBaseGeometry: Missing unit translation to PoolMetric!");
			}
		}
	}

	if( pModel->IsWriter() )
	{
		// if anchor is used, make position relative to it
		if(GetAnchorPos().X() || GetAnchorPos().Y())
		{
			aTranslate += basegfx::B2DTuple(GetAnchorPos().X(), GetAnchorPos().Y());
		}
	}

	// create transformation for polygon, set values at aGeo direct
	basegfx::B2DHomMatrix aTransform;

	// #i75086#
	// Given polygon is already scaled (for historical reasons), but not mirrored yet.
	// Thus, when scale is negative in X or Y, apply the needed mirroring accordingly.
	if(basegfx::fTools::less(aScale.getX(), 0.0) || basegfx::fTools::less(aScale.getY(), 0.0))
	{
		aTransform.scale(
			basegfx::fTools::less(aScale.getX(), 0.0) ? -1.0 : 1.0,
			basegfx::fTools::less(aScale.getY(), 0.0) ? -1.0 : 1.0);
	}

	if(!basegfx::fTools::equalZero(fShearX))
	{
		aTransform.shearX(tan(-atan(fShearX)));
		aGeo.nShearWink = FRound(atan(fShearX) / F_PI18000);
		aGeo.RecalcTan();
	}

	if(!basegfx::fTools::equalZero(fRotate))
	{
        // #i78696#
        // fRotate is matematically correct for linear transformations, so it's
        // the one to use for the geometry change
		aTransform.rotate(fRotate);

        // #i78696# 
        // fRotate is matematically correct, but aGeoStat.nDrehWink is
        // mirrored -> mirror value here
		aGeo.nDrehWink = NormAngle360(FRound(-fRotate / F_PI18000));
		aGeo.RecalcSinCos();
	}

    if(!aTranslate.equalZero())
	{
		// #i39529# absolute positioning, so get current position (without control points (!))
		const basegfx::B2DRange aCurrentRange(basegfx::tools::getRange(basegfx::tools::adaptiveSubdivideByAngle(aNewPolyPolygon)));
		aTransform.translate(aTranslate.getX() - aCurrentRange.getMinX(), aTranslate.getY() - aCurrentRange.getMinY());
	}

	// transform polygon and trigger change
	aNewPolyPolygon.transform(aTransform);
	SetPathPoly(aNewPolyPolygon);
}

// eof
