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
 * Modified November 2009 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/
#ifndef _VISCRS_HXX
#define _VISCRS_HXX
#ifndef _CURSOR_HXX //autogen
#include <vcl/cursor.hxx>
#endif
#include "swcrsr.hxx"
#include "swrect.hxx"
#include "swregion.hxx"

#ifdef USE_JAVA
#include <vector>
#endif	// USE_JAVA

class SwCrsrShell;
class SwShellCrsr;

// --------  Ab hier Klassen / Methoden fuer den nicht Text-Cursor ------

class SwVisCrsr
#ifdef SW_CRSR_TIMER
				: private Timer
#endif
{
	friend void _InitCore();
	friend void _FinitCore();

	BOOL bIsVisible : 1;
	BOOL bIsDragCrsr : 1;

#ifdef SW_CRSR_TIMER
	BOOL bTimerOn : 1;
#endif

	Cursor aTxtCrsr;
	const SwCrsrShell* pCrsrShell;

#ifdef SW_CRSR_TIMER
	virtual void Timeout();
#endif
	void _SetPosAndShow();

public:
	SwVisCrsr( const SwCrsrShell * pCShell );
	~SwVisCrsr();

	void Show();
	void Hide();

	BOOL IsVisible() const { return bIsVisible; }
    void SetDragCrsr( BOOL bFlag = TRUE ) { bIsDragCrsr = bFlag; }

#ifdef SW_CRSR_TIMER
	BOOL ChgTimerFlag( BOOL bTimerOn = TRUE );
#endif
};


// ------ Ab hier Klassen / Methoden fuer die Selectionen -------

// #i75172# predefines
namespace sdr { namespace overlay { class OverlayObject; }}

class SwSelPaintRects : public SwRects
{
	friend void _InitCore();
	friend void _FinitCore();

	static long nPixPtX, nPixPtY;
	static MapMode *pMapMode;

	// die Shell
	const SwCrsrShell* pCShell;
#ifdef USE_JAVA
	::std::vector< SwRect > aLastSelectionPixelRects;
#endif  // USE_JAVA

	void Paint( const SwRect& rRect );

	virtual void Paint( const Rectangle& rRect );
	virtual void FillRects() = 0;

	// #i75172#
	sdr::overlay::OverlayObject*	mpCursorOverlay;

	// #i75172# access to mpCursorOverlay for swapContent
	sdr::overlay::OverlayObject* getCursorOverlay() const { return mpCursorOverlay; }
	void setCursorOverlay(sdr::overlay::OverlayObject* pNew) { mpCursorOverlay = pNew; }

public:
	SwSelPaintRects( const SwCrsrShell& rCSh );
	virtual ~SwSelPaintRects();

	// #i75172# in SwCrsrShell::CreateCrsr() the content of SwSelPaintRects is exchanged. To
	// make a complete swap access to mpCursorOverlay is needed there
	void swapContent(SwSelPaintRects& rSwap);

	void Show();
	void Hide();
	void Invalidate( const SwRect& rRect );

	const SwCrsrShell* GetShell() const { return pCShell; }
	// check current MapMode of the shell and set possibly the static members.
	// Optional set the parameters pX, pY
	static void Get1PixelInLogic( const ViewShell& rSh,
									long* pX = 0, long* pY = 0 );
};


class SwShellCrsr : public virtual SwCursor, public SwSelPaintRects
{
	// Dokument-Positionen der Start/End-Charakter einer SSelection
	Point aMkPt, aPtPt;
	const SwPosition* pPt;		// fuer Zuordung vom GetPoint() zum aPtPt

	virtual void FillRects();	// fuer Table- und normalen Crsr

    using SwCursor::UpDown;

public:
	SwShellCrsr( const SwCrsrShell& rCrsrSh, const SwPosition &rPos );
	SwShellCrsr( const SwCrsrShell& rCrsrSh, const SwPosition &rPos,
					const Point& rPtPos, SwPaM* pRing = 0 );
	SwShellCrsr( SwShellCrsr& );
	virtual ~SwShellCrsr();

	virtual operator SwShellCrsr* ();

	void Show();			// Update und zeige alle Selektionen an
	void Hide();	  		// verstecke alle Selektionen
	void Invalidate( const SwRect& rRect );

	const Point& GetPtPos() const	{ return( SwPaM::GetPoint() == pPt ? aPtPt : aMkPt ); }
		  Point& GetPtPos() 		{ return( SwPaM::GetPoint() == pPt ? aPtPt : aMkPt ); }
	const Point& GetMkPos() const 	{ return( SwPaM::GetMark() == pPt ? aPtPt : aMkPt ); }
		  Point& GetMkPos() 		{ return( SwPaM::GetMark() == pPt ? aPtPt : aMkPt ); }
	const Point& GetSttPos() const	{ return( SwPaM::Start() == pPt ? aPtPt : aMkPt ); }
		  Point& GetSttPos() 		{ return( SwPaM::Start() == pPt ? aPtPt : aMkPt ); }
	const Point& GetEndPos() const	{ return( SwPaM::End() == pPt ? aPtPt : aMkPt ); }
		  Point& GetEndPos() 		{ return( SwPaM::End() == pPt ? aPtPt : aMkPt ); }

	virtual void SetMark();

	virtual SwCursor* Create( SwPaM* pRing = 0 ) const;

    virtual short MaxReplaceArived(); //returns RET_YES/RET_CANCEL/RET_NO
	virtual void SaveTblBoxCntnt( const SwPosition* pPos = 0 );

	BOOL UpDown( BOOL bUp, USHORT nCnt = 1 );

	// TRUE: an die Position kann der Cursor gesetzt werden
	virtual BOOL IsAtValidPos( BOOL bPoint = TRUE ) const;

#ifndef PRODUCT
// JP 05.03.98: zum Testen des UNO-Crsr Verhaltens hier die Implementierung
//				am sichtbaren Cursor
	virtual BOOL IsSelOvr( int eFlags =
                                ( nsSwCursorSelOverFlags::SELOVER_CHECKNODESSECTION |
                                  nsSwCursorSelOverFlags::SELOVER_TOGGLE |
                                  nsSwCursorSelOverFlags::SELOVER_CHANGEPOS ));
#endif

	DECL_FIXEDMEMPOOL_NEWDEL( SwShellCrsr )

#ifdef USE_JAVA
    virtual void GetNativeHightlightColorRects( ::std::vector< Rectangle >& rPixelRects );
#endif	// USE_JAVA
};



class SwShellTableCrsr : public virtual SwShellCrsr, public virtual SwTableCursor
{
	// die Selection hat die gleiche Reihenfolge wie die
	// TabellenBoxen. D.h., wird aus dem einen Array an einer Position
	// etwas geloescht, dann muss es auch im anderen erfolgen!!


public:
	SwShellTableCrsr( const SwCrsrShell& rCrsrSh, const SwPosition& rPos );
	SwShellTableCrsr( const SwCrsrShell& rCrsrSh,
					const SwPosition &rMkPos, const Point& rMkPt,
					const SwPosition &rPtPos, const Point& rPtPt );
	virtual ~SwShellTableCrsr();

	virtual operator SwShellTableCrsr* ();

	virtual void FillRects();	// fuer Table- und normalen Crsr

	// Pruefe, ob sich der SPoint innerhalb der Tabellen-SSelection befindet
	BOOL IsInside( const Point& rPt ) const;

	virtual void SetMark();
	virtual SwCursor* Create( SwPaM* pRing = 0 ) const;
	virtual operator SwShellCrsr* ();
	virtual operator SwTableCursor* ();
    virtual short MaxReplaceArived(); //returns RET_YES/RET_CANCEL/RET_NO
    virtual void SaveTblBoxCntnt( const SwPosition* pPos = 0 );

	// TRUE: an die Position kann der Cursor gesetzt werden
	virtual BOOL IsAtValidPos( BOOL bPoint = TRUE ) const;

#ifndef PRODUCT
// JP 05.03.98: zum Testen des UNO-Crsr Verhaltens hier die Implementierung
//				am sichtbaren Cursor
	virtual BOOL IsSelOvr( int eFlags =
                                ( nsSwCursorSelOverFlags::SELOVER_CHECKNODESSECTION |
                                  nsSwCursorSelOverFlags::SELOVER_TOGGLE |
                                  nsSwCursorSelOverFlags::SELOVER_CHANGEPOS ));
#endif
};



#endif	// _VISCRS_HXX
