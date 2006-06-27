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
 *    Modified June 2006 by Edware H. Peterlin. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/
#ifndef _SV_SVIDS_HRC
#include <svids.hrc>
#endif
#ifndef _SV_SVDATA_HXX
#include <svdata.hxx>
#endif
#ifndef _SV_EVENT_HXX
#include <event.hxx>
#endif
#ifndef _SV_DECOVIEW_HXX
#include <decoview.hxx>
#endif
#ifndef _SV_SYSWIN_HXX
#include <syswin.hxx>
#endif
#ifndef _SV_DOCKWIN_HXX
#include <dockwin.hxx>
#endif
#ifndef _SV_FLOATWIN_HXX
#include <floatwin.hxx>
#endif
#ifndef _SV_BITMAP_HXX
#include <bitmap.hxx>
#endif
#ifndef _SV_GRADIENT_HXX
#include <gradient.hxx>
#endif
#ifndef _SV_IMAGE_HXX
#include <image.hxx>
#endif
#ifndef _SV_VIRDEV_HXX
#include <virdev.hxx>
#endif
#ifndef _SV_HELP_HXX
#include <help.hxx>
#endif
#ifndef _SV_EDIT_HXX
#include <edit.hxx>
#endif
#ifndef _SV_BRDWIN_HXX
#include <brdwin.hxx>
#endif
#ifndef _SV_WINDOW_H
#include <window.h>
#endif
#ifndef _SV_METRIC_HXX
#include <metric.hxx>
#endif
#include <tools/debug.hxx>

using namespace ::com::sun::star::uno;

// useful caption height for title bar buttons
#define MIN_CAPTION_HEIGHT 18

// =======================================================================

static void ImplGetPinImage( USHORT nStyle, BOOL bPinIn, Image& rImage )
{
	// ImageListe laden, wenn noch nicht vorhanden
	ImplSVData* pSVData = ImplGetSVData();
	if ( !pSVData->maCtrlData.mpPinImgList )
	{
		Bitmap aBmp;
        ResMgr* pResMgr = ImplGetResMgr();
        if( pResMgr )
            aBmp = Bitmap( ResId( SV_RESID_BITMAP_PIN, ImplGetResMgr() ) );
		pSVData->maCtrlData.mpPinImgList = new ImageList( aBmp, Color( 0x00, 0x00, 0xFF ), 4 );
	}

	// Image ermitteln und zurueckgeben
	USHORT nId;
	if ( nStyle & BUTTON_DRAW_PRESSED )
	{
		if ( bPinIn )
			nId = 4;
		else
			nId = 3;
	}
	else
	{
		if ( bPinIn )
			nId = 2;
		else
			nId = 1;
	}
	rImage = pSVData->maCtrlData.mpPinImgList->GetImage( nId );
}

// -----------------------------------------------------------------------

void Window::ImplCalcSymbolRect( Rectangle& rRect )
{
	// Den Rand den der Button in der nicht Default-Darstellung freilaesst,
	// dazuaddieren, da wir diesen bei kleinen Buttons mit ausnutzen wollen
	rRect.Left()--;
	rRect.Top()--;
	rRect.Right()++;
	rRect.Bottom()++;

	// Zwischen dem Symbol und dem Button-Rand lassen wir 5% Platz
	long nExtraWidth = ((rRect.GetWidth()*50)+500)/1000;
	long nExtraHeight = ((rRect.GetHeight()*50)+500)/1000;
	rRect.Left()	+= nExtraWidth;
	rRect.Right()	-= nExtraWidth;
	rRect.Top() 	+= nExtraHeight;
	rRect.Bottom()	-= nExtraHeight;
}

// -----------------------------------------------------------------------

static void ImplDrawBrdWinSymbol( OutputDevice* pDev,
								  const Rectangle& rRect, SymbolType eSymbol )
{
	// Zwischen dem Symbol und dem Button lassen wir 5% Platz
	DecorationView	aDecoView( pDev );
	Rectangle		aTempRect = rRect;
	Window::ImplCalcSymbolRect( aTempRect );
	aDecoView.DrawSymbol( aTempRect, eSymbol,
						  pDev->GetSettings().GetStyleSettings().GetButtonTextColor(), 0 );
}

// -----------------------------------------------------------------------

static void ImplDrawBrdWinSymbolButton( OutputDevice* pDev,
										const Rectangle& rRect,
										SymbolType eSymbol, USHORT nState )
{
    BOOL bMouseOver = (nState & BUTTON_DRAW_HIGHLIGHT) != 0;
    nState &= ~BUTTON_DRAW_HIGHLIGHT;

	Rectangle aTempRect;
    Window *pWin = dynamic_cast< Window* >(pDev);
    if( pWin )
    {
        if( bMouseOver )
        {
            // provide a bright background for selection effect
            pWin->SetFillColor( pDev->GetSettings().GetStyleSettings().GetWindowColor() );
            pWin->SetLineColor();
            pWin->DrawRect( rRect );
            pWin->DrawSelectionBackground( rRect, 2, nState & BUTTON_DRAW_PRESSED,
                                            TRUE, FALSE );
        }
        aTempRect = rRect;
        aTempRect.nLeft+=3;
        aTempRect.nRight-=4;
        aTempRect.nTop+=3;
        aTempRect.nBottom-=4;
    }
    else
    {
	    DecorationView aDecoView( pDev );
	    aTempRect = aDecoView.DrawButton( rRect, nState|BUTTON_DRAW_FLAT );
    }
	ImplDrawBrdWinSymbol( pDev, aTempRect, eSymbol );
}


// =======================================================================

// ------------------------
// - ImplBorderWindowView -
// ------------------------

ImplBorderWindowView::~ImplBorderWindowView()
{
}

// -----------------------------------------------------------------------

BOOL ImplBorderWindowView::MouseMove( const MouseEvent& rMEvt )
{
	return FALSE;
}

// -----------------------------------------------------------------------

BOOL ImplBorderWindowView::MouseButtonDown( const MouseEvent& rMEvt )
{
	return FALSE;
}

// -----------------------------------------------------------------------

BOOL ImplBorderWindowView::Tracking( const TrackingEvent& rTEvt )
{
	return FALSE;
}

// -----------------------------------------------------------------------

String ImplBorderWindowView::RequestHelp( const Point& rPos, Rectangle& rHelpRect )
{
	return String();
}

// -----------------------------------------------------------------------

Rectangle ImplBorderWindowView::GetMenuRect() const
{
    return Rectangle();
}

// -----------------------------------------------------------------------

void ImplBorderWindowView::ImplInitTitle( ImplBorderFrameData* pData )
{
	ImplBorderWindow* pBorderWindow = pData->mpBorderWindow;

	if ( !(pBorderWindow->GetStyle() & WB_MOVEABLE) ||
		  (pData->mnTitleType == BORDERWINDOW_TITLE_NONE) )
	{
		pData->mnTitleType	 = BORDERWINDOW_TITLE_NONE;
		pData->mnTitleHeight = 0;
	}
	else
	{
		const StyleSettings& rStyleSettings = pData->mpOutDev->GetSettings().GetStyleSettings();
		if ( pData->mnTitleType == BORDERWINDOW_TITLE_TEAROFF )
			pData->mnTitleHeight = rStyleSettings.GetTearOffTitleHeight();
		else
		{
			if ( pData->mnTitleType == BORDERWINDOW_TITLE_SMALL )
			{
				pBorderWindow->SetPointFont( rStyleSettings.GetFloatTitleFont() );
				pData->mnTitleHeight = rStyleSettings.GetFloatTitleHeight();
			}
			else // pData->mnTitleType == BORDERWINDOW_TITLE_NORMAL
			{
				pBorderWindow->SetPointFont( rStyleSettings.GetTitleFont() );
				pData->mnTitleHeight = rStyleSettings.GetTitleHeight();
			}
			long nTextHeight = pBorderWindow->GetTextHeight();
			if ( nTextHeight > pData->mnTitleHeight )
				pData->mnTitleHeight = nTextHeight;
		}
	}
}

// -----------------------------------------------------------------------

USHORT ImplBorderWindowView::ImplHitTest( ImplBorderFrameData* pData, const Point& rPos )
{
	ImplBorderWindow* pBorderWindow = pData->mpBorderWindow;

	if ( pData->maTitleRect.IsInside( rPos ) )
	{
		if ( pData->maCloseRect.IsInside( rPos ) )
			return BORDERWINDOW_HITTEST_CLOSE;
		else if ( pData->maRollRect.IsInside( rPos ) )
			return BORDERWINDOW_HITTEST_ROLL;
		else if ( pData->maMenuRect.IsInside( rPos ) )
			return BORDERWINDOW_HITTEST_MENU;
		else if ( pData->maDockRect.IsInside( rPos ) )
			return BORDERWINDOW_HITTEST_DOCK;
		else if ( pData->maHideRect.IsInside( rPos ) )
			return BORDERWINDOW_HITTEST_HIDE;
		else if ( pData->maHelpRect.IsInside( rPos ) )
			return BORDERWINDOW_HITTEST_HELP;
		else if ( pData->maPinRect.IsInside( rPos ) )
			return BORDERWINDOW_HITTEST_PIN;
		else
			return BORDERWINDOW_HITTEST_TITLE;
	}

	if ( (pBorderWindow->GetStyle() & WB_SIZEABLE) &&
		 !pBorderWindow->mbRollUp )
	{
		long nSizeWidth = pData->mnNoTitleTop+pData->mnTitleHeight;
		if ( nSizeWidth < 16 )
			nSizeWidth = 16;

        // no corner resize for floating toolbars, which would lead to jumps while formatting
        // setting nSizeWidth = 0 will only return pure left,top,right,bottom
        if( pBorderWindow->GetStyle() & WB_OWNERDRAWDECORATION )
            nSizeWidth = 0;

        if ( rPos.X() < pData->mnLeftBorder )
		{
			if ( rPos.Y() < nSizeWidth )
				return BORDERWINDOW_HITTEST_TOPLEFT;
			else if ( rPos.Y() >= pData->mnHeight-nSizeWidth )
				return BORDERWINDOW_HITTEST_BOTTOMLEFT;
			else
				return BORDERWINDOW_HITTEST_LEFT;
		}
		else if ( rPos.X() >= pData->mnWidth-pData->mnRightBorder )
		{
			if ( rPos.Y() < nSizeWidth )
				return BORDERWINDOW_HITTEST_TOPRIGHT;
			else if ( rPos.Y() >= pData->mnHeight-nSizeWidth )
				return BORDERWINDOW_HITTEST_BOTTOMRIGHT;
			else
				return BORDERWINDOW_HITTEST_RIGHT;
		}
		else if ( rPos.Y() < pData->mnNoTitleTop )
		{
			if ( rPos.X() < nSizeWidth )
				return BORDERWINDOW_HITTEST_TOPLEFT;
			else if ( rPos.X() >= pData->mnWidth-nSizeWidth )
				return BORDERWINDOW_HITTEST_TOPRIGHT;
			else
				return BORDERWINDOW_HITTEST_TOP;
		}
		else if ( rPos.Y() >= pData->mnHeight-pData->mnBottomBorder )
		{
			if ( rPos.X() < nSizeWidth )
				return BORDERWINDOW_HITTEST_BOTTOMLEFT;
			else if ( rPos.X() >= pData->mnWidth-nSizeWidth )
				return BORDERWINDOW_HITTEST_BOTTOMRIGHT;
			else
				return BORDERWINDOW_HITTEST_BOTTOM;
		}
	}

	return 0;
}

// -----------------------------------------------------------------------

BOOL ImplBorderWindowView::ImplMouseMove( ImplBorderFrameData* pData, const MouseEvent& rMEvt )
{
    USHORT oldCloseState = pData->mnCloseState;
    USHORT oldMenuState = pData->mnMenuState;
    pData->mnCloseState &= ~BUTTON_DRAW_HIGHLIGHT;
    pData->mnMenuState &= ~BUTTON_DRAW_HIGHLIGHT;

	Point aMousePos = rMEvt.GetPosPixel();
	USHORT nHitTest = ImplHitTest( pData, aMousePos );
	PointerStyle ePtrStyle = POINTER_ARROW;
	if ( nHitTest & BORDERWINDOW_HITTEST_LEFT )
		ePtrStyle = POINTER_WINDOW_WSIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_RIGHT )
		ePtrStyle = POINTER_WINDOW_ESIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_TOP )
		ePtrStyle = POINTER_WINDOW_NSIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_BOTTOM )
		ePtrStyle = POINTER_WINDOW_SSIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_TOPLEFT )
		ePtrStyle = POINTER_WINDOW_NWSIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_BOTTOMRIGHT )
		ePtrStyle = POINTER_WINDOW_SESIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_TOPRIGHT )
		ePtrStyle = POINTER_WINDOW_NESIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_BOTTOMLEFT )
		ePtrStyle = POINTER_WINDOW_SWSIZE;
	else if ( nHitTest & BORDERWINDOW_HITTEST_CLOSE )
		pData->mnCloseState |= BUTTON_DRAW_HIGHLIGHT;
	else if ( nHitTest & BORDERWINDOW_HITTEST_MENU )
		pData->mnMenuState |= BUTTON_DRAW_HIGHLIGHT;
	pData->mpBorderWindow->SetPointer( Pointer( ePtrStyle ) );

    if( pData->mnCloseState != oldCloseState )
        pData->mpBorderWindow->Invalidate( pData->maCloseRect );
    if( pData->mnMenuState != oldMenuState )
        pData->mpBorderWindow->Invalidate( pData->maMenuRect );

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL ImplBorderWindowView::ImplMouseButtonDown( ImplBorderFrameData* pData, const MouseEvent& rMEvt )
{
	ImplBorderWindow* pBorderWindow = pData->mpBorderWindow;

	if ( rMEvt.IsLeft() || rMEvt.IsRight() )
	{
		pData->maMouseOff = rMEvt.GetPosPixel();
		pData->mnHitTest = ImplHitTest( pData, pData->maMouseOff );
		USHORT nDragFullTest = 0;
		if ( pData->mnHitTest )
		{
			BOOL bTracking = TRUE;
			BOOL bHitTest = TRUE;

			if ( pData->mnHitTest & BORDERWINDOW_HITTEST_CLOSE )
			{
				pData->mnCloseState |= BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_CLOSE );
			}
			else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_ROLL )
			{
				pData->mnRollState |= BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_ROLL );
			}
			else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_DOCK )
			{
				pData->mnDockState |= BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_DOCK );
			}
			else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_MENU )
			{
				pData->mnMenuState |= BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_MENU );

                // call handler already on mouse down
				if ( pBorderWindow->ImplGetClientWindow()->IsSystemWindow() )
				{
					SystemWindow* pClientWindow = (SystemWindow*)(pBorderWindow->ImplGetClientWindow());
					pClientWindow->TitleButtonClick( TITLE_BUTTON_MENU );
				}
			}
			else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_HIDE )
			{
				pData->mnHideState |= BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_HIDE );
			}
			else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_HELP )
			{
				pData->mnHelpState |= BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_HELP );
			}
			else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_PIN )
			{
				pData->mnPinState |= BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_PIN );
			}
			else
			{
				if ( rMEvt.GetClicks() == 1 )
				{
					if ( bTracking )
					{
						Point	aPos		 = pBorderWindow->GetPosPixel();
						Size	aSize		 = pBorderWindow->GetOutputSizePixel();
						pData->mnTrackX 	 = aPos.X();
						pData->mnTrackY 	 = aPos.Y();
						pData->mnTrackWidth  = aSize.Width();
						pData->mnTrackHeight = aSize.Height();

						if ( pData->mnHitTest & BORDERWINDOW_HITTEST_TITLE )
							nDragFullTest = DRAGFULL_OPTION_WINDOWMOVE;
						else
							nDragFullTest = DRAGFULL_OPTION_WINDOWSIZE;
					}
				}
				else
				{
					bTracking = FALSE;

					if ( (pData->mnHitTest & BORDERWINDOW_DRAW_TITLE) &&
						 ((rMEvt.GetClicks() % 2) == 0) )
					{
						pData->mnHitTest = 0;
						bHitTest = FALSE;

						if ( pBorderWindow->ImplGetClientWindow()->IsSystemWindow() )
						{
							SystemWindow* pClientWindow = (SystemWindow*)(pBorderWindow->ImplGetClientWindow());
							if ( TRUE /*pBorderWindow->mbDockBtn*/ )   // always perform docking on double click, no button required
								pClientWindow->TitleButtonClick( TITLE_BUTTON_DOCKING );
							else if ( pBorderWindow->GetStyle() & WB_ROLLABLE )
							{
								if ( pClientWindow->IsRollUp() )
									pClientWindow->RollDown();
								else
									pClientWindow->RollUp();
								pClientWindow->Roll();
							}
						}
					}
				}
			}

			if ( bTracking )
			{
				pData->mbDragFull = FALSE;
				if ( nDragFullTest )
                    pData->mbDragFull = TRUE;   // always fulldrag for proper docking, ignore system settings
				pBorderWindow->StartTracking();
			}
			else if ( bHitTest )
				pData->mnHitTest = 0;
		}
	}

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL ImplBorderWindowView::ImplTracking( ImplBorderFrameData* pData, const TrackingEvent& rTEvt )
{
	ImplBorderWindow* pBorderWindow = pData->mpBorderWindow;

	if ( rTEvt.IsTrackingEnded() )
	{
		USHORT nHitTest = pData->mnHitTest;
		pData->mnHitTest = 0;

		if ( nHitTest & BORDERWINDOW_HITTEST_CLOSE )
		{
			if ( pData->mnCloseState & BUTTON_DRAW_PRESSED )
			{
				pData->mnCloseState &= ~BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_CLOSE );

				// Bei Abbruch kein Click-Handler rufen
				if ( !rTEvt.IsTrackingCanceled() )
				{
                    // dispatch to correct window type (why is Close() not virtual ??? )
                    // TODO: make Close() virtual
                    Window *pWin = pBorderWindow->ImplGetClientWindow()->ImplGetWindow();
                    SystemWindow  *pSysWin  = dynamic_cast<SystemWindow* >(pWin);
                    DockingWindow *pDockWin = dynamic_cast<DockingWindow*>(pWin);
					if ( pSysWin )
                        pSysWin->Close();
					else if ( pDockWin )
                        pDockWin->Close();
				}
			}
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_ROLL )
		{
			if ( pData->mnRollState & BUTTON_DRAW_PRESSED )
			{
				pData->mnRollState &= ~BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_ROLL );

				// Bei Abbruch kein Click-Handler rufen
				if ( !rTEvt.IsTrackingCanceled() )
				{
					if ( pBorderWindow->ImplGetClientWindow()->IsSystemWindow() )
					{
						SystemWindow* pClientWindow = (SystemWindow*)(pBorderWindow->ImplGetClientWindow());
						if ( pClientWindow->IsRollUp() )
							pClientWindow->RollDown();
						else
							pClientWindow->RollUp();
						pClientWindow->Roll();
					}
				}
			}
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_DOCK )
		{
			if ( pData->mnDockState & BUTTON_DRAW_PRESSED )
			{
				pData->mnDockState &= ~BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_DOCK );

				// Bei Abbruch kein Click-Handler rufen
				if ( !rTEvt.IsTrackingCanceled() )
				{
					if ( pBorderWindow->ImplGetClientWindow()->IsSystemWindow() )
					{
						SystemWindow* pClientWindow = (SystemWindow*)(pBorderWindow->ImplGetClientWindow());
						pClientWindow->TitleButtonClick( TITLE_BUTTON_DOCKING );
					}
				}
			}
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_MENU )
		{
			if ( pData->mnMenuState & BUTTON_DRAW_PRESSED )
			{
				pData->mnMenuState &= ~BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_MENU );

                // handler already called on mouse down
			}
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_HIDE )
		{
			if ( pData->mnHideState & BUTTON_DRAW_PRESSED )
			{
				pData->mnHideState &= ~BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_HIDE );

				// Bei Abbruch kein Click-Handler rufen
				if ( !rTEvt.IsTrackingCanceled() )
				{
					if ( pBorderWindow->ImplGetClientWindow()->IsSystemWindow() )
					{
						SystemWindow* pClientWindow = (SystemWindow*)(pBorderWindow->ImplGetClientWindow());
						pClientWindow->TitleButtonClick( TITLE_BUTTON_HIDE );
					}
				}
			}
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_HELP )
		{
			if ( pData->mnHelpState & BUTTON_DRAW_PRESSED )
			{
				pData->mnHelpState &= ~BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_HELP );

				// Bei Abbruch kein Click-Handler rufen
				if ( !rTEvt.IsTrackingCanceled() )
				{
				}
			}
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_PIN )
		{
			if ( pData->mnPinState & BUTTON_DRAW_PRESSED )
			{
				pData->mnPinState &= ~BUTTON_DRAW_PRESSED;
				DrawWindow( BORDERWINDOW_DRAW_PIN );

				// Bei Abbruch kein Click-Handler rufen
				if ( !rTEvt.IsTrackingCanceled() )
				{
					if ( pBorderWindow->ImplGetClientWindow()->IsSystemWindow() )
					{
						SystemWindow* pClientWindow = (SystemWindow*)(pBorderWindow->ImplGetClientWindow());
						pClientWindow->SetPin( !pClientWindow->IsPined() );
						pClientWindow->Pin();
					}
				}
			}
		}
		else
		{
			if ( pData->mbDragFull )
			{
				// Bei Abbruch alten Zustand wieder herstellen
				if ( rTEvt.IsTrackingCanceled() )
					pBorderWindow->SetPosSizePixel( Point( pData->mnTrackX, pData->mnTrackY ), Size( pData->mnTrackWidth, pData->mnTrackHeight ) );
			}
			else
			{
				pBorderWindow->HideTracking();
				if ( !rTEvt.IsTrackingCanceled() )
					pBorderWindow->SetPosSizePixel( Point( pData->mnTrackX, pData->mnTrackY ), Size( pData->mnTrackWidth, pData->mnTrackHeight ) );
			}

			if ( !rTEvt.IsTrackingCanceled() )
			{
				if ( pBorderWindow->ImplGetClientWindow()->ImplIsFloatingWindow() )
				{
					if ( ((FloatingWindow*)pBorderWindow->ImplGetClientWindow())->IsInPopupMode() )
						((FloatingWindow*)pBorderWindow->ImplGetClientWindow())->EndPopupMode( FLOATWIN_POPUPMODEEND_TEAROFF );
				}
			}
		}
	}
	else if ( !rTEvt.GetMouseEvent().IsSynthetic() )
	{
		Point aMousePos = rTEvt.GetMouseEvent().GetPosPixel();

		if ( pData->mnHitTest & BORDERWINDOW_HITTEST_CLOSE )
		{
			if ( pData->maCloseRect.IsInside( aMousePos ) )
			{
				if ( !(pData->mnCloseState & BUTTON_DRAW_PRESSED) )
				{
					pData->mnCloseState |= BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_CLOSE );
				}
			}
			else
			{
				if ( pData->mnCloseState & BUTTON_DRAW_PRESSED )
				{
					pData->mnCloseState &= ~BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_CLOSE );
				}
			}
		}
		else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_ROLL )
		{
			if ( pData->maRollRect.IsInside( aMousePos ) )
			{
				if ( !(pData->mnRollState & BUTTON_DRAW_PRESSED) )
				{
					pData->mnRollState |= BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_ROLL );
				}
			}
			else
			{
				if ( pData->mnRollState & BUTTON_DRAW_PRESSED )
				{
					pData->mnRollState &= ~BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_ROLL );
				}
			}
		}
		else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_DOCK )
		{
			if ( pData->maDockRect.IsInside( aMousePos ) )
			{
				if ( !(pData->mnDockState & BUTTON_DRAW_PRESSED) )
				{
					pData->mnDockState |= BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_DOCK );
				}
			}
			else
			{
				if ( pData->mnDockState & BUTTON_DRAW_PRESSED )
				{
					pData->mnDockState &= ~BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_DOCK );
				}
			}
		}
		else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_MENU )
		{
			if ( pData->maMenuRect.IsInside( aMousePos ) )
			{
				if ( !(pData->mnMenuState & BUTTON_DRAW_PRESSED) )
				{
					pData->mnMenuState |= BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_MENU );

				}
			}
			else
			{
				if ( pData->mnMenuState & BUTTON_DRAW_PRESSED )
				{
					pData->mnMenuState &= ~BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_MENU );
				}
			}
		}
		else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_HIDE )
		{
			if ( pData->maHideRect.IsInside( aMousePos ) )
			{
				if ( !(pData->mnHideState & BUTTON_DRAW_PRESSED) )
				{
					pData->mnHideState |= BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_HIDE );
				}
			}
			else
			{
				if ( pData->mnHideState & BUTTON_DRAW_PRESSED )
				{
					pData->mnHideState &= ~BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_HIDE );
				}
			}
		}
		else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_HELP )
		{
			if ( pData->maHelpRect.IsInside( aMousePos ) )
			{
				if ( !(pData->mnHelpState & BUTTON_DRAW_PRESSED) )
				{
					pData->mnHelpState |= BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_HELP );
				}
			}
			else
			{
				if ( pData->mnHelpState & BUTTON_DRAW_PRESSED )
				{
					pData->mnHelpState &= ~BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_HELP );
				}
			}
		}
		else if ( pData->mnHitTest & BORDERWINDOW_HITTEST_PIN )
		{
			if ( pData->maPinRect.IsInside( aMousePos ) )
			{
				if ( !(pData->mnPinState & BUTTON_DRAW_PRESSED) )
				{
					pData->mnPinState |= BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_PIN );
				}
			}
			else
			{
				if ( pData->mnPinState & BUTTON_DRAW_PRESSED )
				{
					pData->mnPinState &= ~BUTTON_DRAW_PRESSED;
					DrawWindow( BORDERWINDOW_DRAW_PIN );
				}
			}
		}
		else
		{
            /*
            // adjusting mousepos not required, we allow the whole screen (no desktop anymore...)
			Point	aFrameMousePos = pBorderWindow->ImplOutputToFrame( aMousePos );
			Size	aFrameSize = pBorderWindow->ImplGetFrameWindow()->GetOutputSizePixel();
			if ( aFrameMousePos.X() < 0 )
				aFrameMousePos.X() = 0;
			if ( aFrameMousePos.Y() < 0 )
				aFrameMousePos.Y() = 0;
			if ( aFrameMousePos.X() > aFrameSize.Width()-1 )
				aFrameMousePos.X() = aFrameSize.Width()-1;
			if ( aFrameMousePos.Y() > aFrameSize.Height()-1 )
				aFrameMousePos.Y() = aFrameSize.Height()-1;
			aMousePos = pBorderWindow->ImplFrameToOutput( aFrameMousePos );
            */

			aMousePos.X()	 -= pData->maMouseOff.X();
			aMousePos.Y()	 -= pData->maMouseOff.Y();

			if ( pData->mnHitTest & BORDERWINDOW_HITTEST_TITLE )
			{
	            pData->mpBorderWindow->SetPointer( Pointer( POINTER_MOVE ) );

				Point aPos = pBorderWindow->GetPosPixel();
				aPos.X() += aMousePos.X();
				aPos.Y() += aMousePos.Y();
				if ( pData->mbDragFull )
				{
					pBorderWindow->SetPosPixel( aPos );
					pBorderWindow->ImplUpdateAll();
					pBorderWindow->ImplGetFrameWindow()->ImplUpdateAll();
				}
				else
				{
					pData->mnTrackX = aPos.X();
					pData->mnTrackY = aPos.Y();
					pBorderWindow->ShowTracking( Rectangle( pBorderWindow->ScreenToOutputPixel( aPos ), pBorderWindow->GetOutputSizePixel() ), SHOWTRACK_BIG );
				}
			}
			else
			{
				Point		aOldPos			= pBorderWindow->GetPosPixel();
				Size		aSize			= pBorderWindow->GetSizePixel();
				Rectangle	aNewRect( aOldPos, aSize );
				long		nOldWidth		= aSize.Width();
				long		nOldHeight		= aSize.Height();
				long		nBorderWidth	= pData->mnLeftBorder+pData->mnRightBorder;
				long		nBorderHeight	= pData->mnTopBorder+pData->mnBottomBorder;
				long		nMinWidth		= pBorderWindow->mnMinWidth+nBorderWidth;
				long		nMinHeight		= pBorderWindow->mnMinHeight+nBorderHeight;
				long		nMinWidth2		= nBorderWidth;
				long		nMaxWidth		= pBorderWindow->mnMaxWidth+nBorderWidth;
				long		nMaxHeight		= pBorderWindow->mnMaxHeight+nBorderHeight;

				if ( pData->mnTitleHeight )
				{
					nMinWidth2 += 4;

					if ( pBorderWindow->GetStyle() & WB_CLOSEABLE )
						nMinWidth2 += pData->maCloseRect.GetWidth();
				}
				if ( nMinWidth2 > nMinWidth )
					nMinWidth = nMinWidth2;
				if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_LEFT | BORDERWINDOW_HITTEST_TOPLEFT | BORDERWINDOW_HITTEST_BOTTOMLEFT) )
				{
					aNewRect.Left() += aMousePos.X();
					if ( aNewRect.GetWidth() < nMinWidth )
						aNewRect.Left() = aNewRect.Right()-nMinWidth+1;
					else if ( aNewRect.GetWidth() > nMaxWidth )
						aNewRect.Left() = aNewRect.Right()-nMaxWidth+1;
				}
				else if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_RIGHT | BORDERWINDOW_HITTEST_TOPRIGHT | BORDERWINDOW_HITTEST_BOTTOMRIGHT) )
				{
					aNewRect.Right() += aMousePos.X();
					if ( aNewRect.GetWidth() < nMinWidth )
						aNewRect.Right() = aNewRect.Left()+nMinWidth+1;
					else if ( aNewRect.GetWidth() > nMaxWidth )
						aNewRect.Right() = aNewRect.Left()+nMaxWidth+1;
				}
				if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_TOP | BORDERWINDOW_HITTEST_TOPLEFT | BORDERWINDOW_HITTEST_TOPRIGHT) )
				{
					aNewRect.Top() += aMousePos.Y();
					if ( aNewRect.GetHeight() < nMinHeight )
						aNewRect.Top() = aNewRect.Bottom()-nMinHeight+1;
					else if ( aNewRect.GetHeight() > nMaxHeight )
						aNewRect.Top() = aNewRect.Bottom()-nMaxHeight+1;
				}
				else if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_BOTTOM | BORDERWINDOW_HITTEST_BOTTOMLEFT | BORDERWINDOW_HITTEST_BOTTOMRIGHT) )
				{
					aNewRect.Bottom() += aMousePos.Y();
					if ( aNewRect.GetHeight() < nMinHeight )
						aNewRect.Bottom() = aNewRect.Top()+nMinHeight+1;
					else if ( aNewRect.GetHeight() > nMaxHeight )
						aNewRect.Bottom() = aNewRect.Top()+nMaxHeight+1;
				}

				// call Resizing-Handler for SystemWindows
				if ( pBorderWindow->ImplGetClientWindow()->IsSystemWindow() )
				{
					// adjust size for Resizing-call
					aSize = aNewRect.GetSize();
					aSize.Width()	-= nBorderWidth;
					aSize.Height()	-= nBorderHeight;
					((SystemWindow*)pBorderWindow->ImplGetClientWindow())->Resizing( aSize );
					aSize.Width()	+= nBorderWidth;
					aSize.Height()	+= nBorderHeight;
					if ( aSize.Width() < nMinWidth )
						aSize.Width() = nMinWidth;
					if ( aSize.Height() < nMinHeight )
						aSize.Height() = nMinHeight;
					if ( aSize.Width() > nMaxWidth )
						aSize.Width() = nMaxWidth;
					if ( aSize.Height() > nMaxHeight )
						aSize.Height() = nMaxHeight;
					if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_LEFT | BORDERWINDOW_HITTEST_TOPLEFT | BORDERWINDOW_HITTEST_BOTTOMLEFT) )
						aNewRect.Left() = aNewRect.Right()-aSize.Width()+1;
					else
						aNewRect.Right() = aNewRect.Left()+aSize.Width()-1;
					if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_TOP | BORDERWINDOW_HITTEST_TOPLEFT | BORDERWINDOW_HITTEST_TOPRIGHT) )
						aNewRect.Top() = aNewRect.Bottom()-aSize.Height()+1;
					else
						aNewRect.Bottom() = aNewRect.Top()+aSize.Height()-1;
				}

				if ( pData->mbDragFull )
				{
                    // no move (only resize) if position did not change
                    if( aOldPos != aNewRect.TopLeft() )
					    pBorderWindow->SetPosSizePixel( aNewRect.Left(), aNewRect.Top(),
													aNewRect.GetWidth(), aNewRect.GetHeight(), WINDOW_POSSIZE_POSSIZE );
                    else
					    pBorderWindow->SetPosSizePixel( aNewRect.Left(), aNewRect.Top(),
													aNewRect.GetWidth(), aNewRect.GetHeight(), WINDOW_POSSIZE_SIZE );

					pBorderWindow->ImplUpdateAll();
					pBorderWindow->ImplGetFrameWindow()->ImplUpdateAll();
					if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_RIGHT | BORDERWINDOW_HITTEST_TOPRIGHT | BORDERWINDOW_HITTEST_BOTTOMRIGHT) )
						pData->maMouseOff.X() += aNewRect.GetWidth()-nOldWidth;
					if ( pData->mnHitTest & (BORDERWINDOW_HITTEST_BOTTOM | BORDERWINDOW_HITTEST_BOTTOMLEFT | BORDERWINDOW_HITTEST_BOTTOMRIGHT) )
						pData->maMouseOff.Y() += aNewRect.GetHeight()-nOldHeight;
				}
				else
				{
					pData->mnTrackX 	   = aNewRect.Left();
					pData->mnTrackY 	   = aNewRect.Top();
					pData->mnTrackWidth    = aNewRect.GetWidth();
					pData->mnTrackHeight   = aNewRect.GetHeight();
					pBorderWindow->ShowTracking( Rectangle( pBorderWindow->ScreenToOutputPixel( aNewRect.TopLeft() ), aNewRect.GetSize() ), SHOWTRACK_BIG );
				}
			}
		}
	}

	return TRUE;
}

// -----------------------------------------------------------------------

String ImplBorderWindowView::ImplRequestHelp( ImplBorderFrameData* pData,
											  const Point& rPos,
											  Rectangle& rHelpRect )
{
	USHORT nHelpId = 0;
    String aHelpStr;
	USHORT nHitTest = ImplHitTest( pData, rPos );
	if ( nHitTest )
	{
		if ( nHitTest & BORDERWINDOW_HITTEST_CLOSE )
		{
			nHelpId 	= SV_HELPTEXT_CLOSE;
			rHelpRect	= pData->maCloseRect;
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_ROLL )
		{
			if ( pData->mpBorderWindow->mbRollUp )
				nHelpId = SV_HELPTEXT_ROLLDOWN;
			else
				nHelpId = SV_HELPTEXT_ROLLUP;
			rHelpRect	= pData->maRollRect;
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_DOCK )
		{
			nHelpId 	= SV_HELPTEXT_MAXIMIZE;
			rHelpRect	= pData->maDockRect;
		}
        /* no help string available
		else if ( nHitTest & BORDERWINDOW_HITTEST_MENU )
		{
			nHelpId 	= SV_HELPTEXT_MENU;
			rHelpRect	= pData->maMenuRect;
		}*/
		else if ( nHitTest & BORDERWINDOW_HITTEST_HIDE )
		{
			nHelpId 	= SV_HELPTEXT_MINIMIZE;
			rHelpRect	= pData->maHideRect;
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_HELP )
		{
			nHelpId 	= SV_HELPTEXT_HELP;
			rHelpRect	= pData->maHelpRect;
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_PIN )
		{
			nHelpId 	= SV_HELPTEXT_ALWAYSVISIBLE;
			rHelpRect	= pData->maPinRect;
		}
		else if ( nHitTest & BORDERWINDOW_HITTEST_TITLE )
        {
            if( !pData->maTitleRect.IsEmpty() )
            {
                // tooltip only if title truncated
                if( pData->mbTitleClipped )
                {
			        rHelpRect	= pData->maTitleRect;
                    // no help id, use window title as help string
                    aHelpStr    = pData->mpBorderWindow->GetText();
                }
            }
        }
	}

    if( nHelpId && ImplGetResMgr() )
	    aHelpStr = String( ResId( nHelpId, ImplGetResMgr() ) );

    return aHelpStr;
}

// -----------------------------------------------------------------------

long ImplBorderWindowView::ImplCalcTitleWidth( const ImplBorderFrameData* pData ) const
{
	// kein sichtbarer Title, dann auch keine Breite
	if ( !pData->mnTitleHeight )
		return 0;

	ImplBorderWindow* pBorderWindow = pData->mpBorderWindow;
	long nTitleWidth = pBorderWindow->GetTextWidth( pBorderWindow->GetText() )+6;
	nTitleWidth += pData->maPinRect.GetWidth();
	nTitleWidth += pData->maCloseRect.GetWidth();
	nTitleWidth += pData->maRollRect.GetWidth();
	nTitleWidth += pData->maDockRect.GetWidth();
	nTitleWidth += pData->maMenuRect.GetWidth();
	nTitleWidth += pData->maHideRect.GetWidth();
	nTitleWidth += pData->maHelpRect.GetWidth();
	nTitleWidth += pData->mnLeftBorder+pData->mnRightBorder;
	return nTitleWidth;
}

// =======================================================================

// --------------------------
// - ImplNoBorderWindowView -
// --------------------------

ImplNoBorderWindowView::ImplNoBorderWindowView( ImplBorderWindow* pBorderWindow )
{
}

// -----------------------------------------------------------------------

void ImplNoBorderWindowView::Init( OutputDevice* pDev, long nWidth, long nHeight )
{
}

// -----------------------------------------------------------------------

void ImplNoBorderWindowView::GetBorder( sal_Int32& rLeftBorder, sal_Int32& rTopBorder,
										sal_Int32& rRightBorder, sal_Int32& rBottomBorder ) const
{
	rLeftBorder 	= 0;
	rTopBorder		= 0;
	rRightBorder	= 0;
	rBottomBorder	= 0;
}

// -----------------------------------------------------------------------

long ImplNoBorderWindowView::CalcTitleWidth() const
{
	return 0;
}

// -----------------------------------------------------------------------

void ImplNoBorderWindowView::DrawWindow( USHORT nDrawFlags, OutputDevice*, const Point* )
{
}

// =======================================================================

// -----------------------------
// - ImplSmallBorderWindowView -
// -----------------------------

// =======================================================================

ImplSmallBorderWindowView::ImplSmallBorderWindowView( ImplBorderWindow* pBorderWindow )
{
	mpBorderWindow = pBorderWindow;
}

// -----------------------------------------------------------------------

void ImplSmallBorderWindowView::Init( OutputDevice* pDev, long nWidth, long nHeight )
{
	mpOutDev	= pDev;
	mnWidth 	= nWidth;
	mnHeight	= nHeight;

	USHORT nBorderStyle = mpBorderWindow->GetBorderStyle();
	if ( nBorderStyle & WINDOW_BORDER_NOBORDER )
	{
		mnLeftBorder	= 0;
		mnTopBorder 	= 0;
		mnRightBorder	= 0;
		mnBottomBorder	= 0;
	}
	else
	{
		USHORT nStyle = FRAME_DRAW_NODRAW;
		// Wenn Border umgesetzt wurde oder BorderWindow ein Frame-Fenster
		// ist, dann Border nach aussen
		if ( (nBorderStyle & WINDOW_BORDER_DOUBLEOUT) || mpBorderWindow->mbSmallOutBorder )
			nStyle |= FRAME_DRAW_DOUBLEOUT;
		else
			nStyle |= FRAME_DRAW_DOUBLEIN;
		if ( nBorderStyle & WINDOW_BORDER_MONO )
			nStyle |= FRAME_DRAW_MONO;

		DecorationView	aDecoView( mpOutDev );
		Rectangle		aRect( 0, 0, 10, 10 );
		Rectangle		aCalcRect = aDecoView.DrawFrame( aRect, nStyle );
		mnLeftBorder	= aCalcRect.Left();
		mnTopBorder 	= aCalcRect.Top();
		mnRightBorder	= aRect.Right()-aCalcRect.Right();
		mnBottomBorder	= aRect.Bottom()-aCalcRect.Bottom();
	}
}

// -----------------------------------------------------------------------

void ImplSmallBorderWindowView::GetBorder( sal_Int32& rLeftBorder, sal_Int32& rTopBorder,
										   sal_Int32& rRightBorder, sal_Int32& rBottomBorder ) const
{
	rLeftBorder 	= mnLeftBorder;
	rTopBorder		= mnTopBorder;
	rRightBorder	= mnRightBorder;
	rBottomBorder	= mnBottomBorder;
}

// -----------------------------------------------------------------------

long ImplSmallBorderWindowView::CalcTitleWidth() const
{
	return 0;
}

// -----------------------------------------------------------------------

void ImplSmallBorderWindowView::DrawWindow( USHORT nDrawFlags, OutputDevice*, const Point* )
{
	USHORT nBorderStyle = mpBorderWindow->GetBorderStyle();
	if ( nBorderStyle & WINDOW_BORDER_NOBORDER )
		return;

    BOOL bNativeOK = FALSE;
    // for native widget drawing we must find out what
    // control this border belongs to
    Window *pWin = NULL, *pCtrl = NULL;
    if( mpOutDev->GetOutDevType() == OUTDEV_WINDOW )
        pWin = (Window*) mpOutDev;

    ControlType aCtrlType = 0;
    ControlPart aCtrlPart = PART_ENTIRE_CONTROL;

    if( pWin && (pCtrl = mpBorderWindow->GetWindow( WINDOW_CLIENT )) != NULL )
    {
        switch( pCtrl->GetType() )
        {
            case WINDOW_MULTILINEEDIT:
                aCtrlType = CTRL_MULTILINE_EDITBOX;
                break;
            case WINDOW_EDIT:
            case WINDOW_PATTERNFIELD:
            case WINDOW_METRICFIELD:
            case WINDOW_CURRENCYFIELD:
            case WINDOW_DATEFIELD:
            case WINDOW_TIMEFIELD:
            case WINDOW_LONGCURRENCYFIELD:
            case WINDOW_NUMERICFIELD:
            case WINDOW_SPINFIELD:
                if( pCtrl->GetStyle() & WB_SPIN )
                    aCtrlType = CTRL_SPINBOX;
                else
                    aCtrlType = CTRL_EDITBOX;
                break;

            case WINDOW_LISTBOX:
            case WINDOW_MULTILISTBOX:
            case WINDOW_TREELISTBOX:
                aCtrlType = CTRL_LISTBOX;
                if( pCtrl->GetStyle() & WB_DROPDOWN )
                    aCtrlPart = PART_ENTIRE_CONTROL;
                else
                    aCtrlPart = PART_WINDOW;
                break;

            case WINDOW_LISTBOXWINDOW:
                aCtrlType = CTRL_LISTBOX;
                aCtrlPart = PART_WINDOW;
                break;

            case WINDOW_COMBOBOX:
            case WINDOW_PATTERNBOX:			
            case WINDOW_NUMERICBOX:			
            case WINDOW_METRICBOX:			
            case WINDOW_CURRENCYBOX:
            case WINDOW_DATEBOX:		
            case WINDOW_TIMEBOX:			
            case WINDOW_LONGCURRENCYBOX:
                if( pCtrl->GetStyle() & WB_DROPDOWN )
                {
                    aCtrlType = CTRL_COMBOBOX;
                    aCtrlPart = PART_ENTIRE_CONTROL;
                }
                else
                {
                    aCtrlType = CTRL_LISTBOX;
                    aCtrlPart = PART_WINDOW;
                }
                break;
                break;

            default:
                break;
        }
    }

    if ( aCtrlType && pCtrl->IsNativeControlSupported(aCtrlType, aCtrlPart) )
    {
        ImplControlValue aControlValue;
        ControlState     nState = CTRL_STATE_ENABLED;

        if ( !pWin->IsEnabled() )
            nState &= ~CTRL_STATE_ENABLED;
        if ( pWin->HasFocus() )
            nState |= CTRL_STATE_FOCUSED;
#ifdef USE_JAVA
#ifdef GENESIS_OF_THE_NEW_WEAPONS
		if ( ( aCtrlType == CTRL_EDITBOX ) && ( ! ( nState & CTRL_STATE_FOCUSED ) ) )
		{
			// for edit boxes, we want to check to see if the edit itself
			// has the focus.  The border window itself will never get focused
			
			if ( pCtrl->HasFocus() )
				nState |= CTRL_STATE_FOCUSED;
		}
#endif
#endif
        BOOL bMouseOver = FALSE;
        Window *pCtrlChild = pCtrl->GetWindow( WINDOW_FIRSTCHILD );
        while( pCtrlChild && !(bMouseOver = pCtrlChild->IsMouseOver()) )
            pCtrlChild = pCtrlChild->GetWindow( WINDOW_NEXT );
    	
        if( bMouseOver )
            nState |= CTRL_STATE_ROLLOVER;

        Point aPoint;
        Region aCtrlRegion( Rectangle( aPoint, Size( mnWidth, mnHeight ) ) );
        bNativeOK = pWin->DrawNativeControl( aCtrlType, aCtrlPart, aCtrlRegion, nState,
                aControlValue, rtl::OUString() );

        // if the native theme draws the spinbuttons in one call, make sure the proper settings
        // are passed, this might force a redraw though.... (TODO: improve)
        if ( (aCtrlType == CTRL_SPINBOX) && !pCtrl->IsNativeControlSupported( CTRL_SPINBOX, PART_BUTTON_UP ) )
        {
            Edit *pEdit = ((Edit*) pCtrl)->GetSubEdit();
            if ( pEdit )
                pCtrl->Paint( Rectangle() );  // make sure the buttons are also drawn as they might overwrite the border
        }
    }

    if( bNativeOK )
        return;

	if ( nDrawFlags & BORDERWINDOW_DRAW_FRAME )
	{
		if ( nBorderStyle & WINDOW_BORDER_ACTIVE )
		{
			Color aColor = mpOutDev->GetSettings().GetStyleSettings().GetHighlightColor();
			mpOutDev->SetLineColor();
			mpOutDev->SetFillColor( aColor );
			mpOutDev->DrawRect( Rectangle( 0, 0, mnWidth-1, mnTopBorder ) );
			mpOutDev->DrawRect( Rectangle( 0, mnHeight-mnBottomBorder, mnWidth-1, mnHeight-1 ) );
			mpOutDev->DrawRect( Rectangle( 0, 0, mnLeftBorder, mnHeight-1 ) );
			mpOutDev->DrawRect( Rectangle( mnWidth-mnRightBorder, 0, mnWidth-1, mnHeight-1 ) );
		}
		else
		{
			USHORT nStyle = 0;
			// Wenn Border umgesetzt wurde oder BorderWindow ein Frame-Fenster
			// ist, dann Border nach aussen
			if ( (nBorderStyle & WINDOW_BORDER_DOUBLEOUT) || mpBorderWindow->mbSmallOutBorder )
				nStyle |= FRAME_DRAW_DOUBLEOUT;
			else
				nStyle |= FRAME_DRAW_DOUBLEIN;
			if ( nBorderStyle & WINDOW_BORDER_MONO )
				nStyle |= FRAME_DRAW_MONO;
			if ( nBorderStyle & WINDOW_BORDER_MENU )
				nStyle |= FRAME_DRAW_MENU;
            // tell DrawFrame that we're drawing a window border of a frame window to avoid round corners
            if( pWin && pWin == pWin->ImplGetFrameWindow() )
                nStyle |= FRAME_DRAW_WINDOWBORDER;  

            DecorationView	aDecoView( mpOutDev );
			Point			aTmpPoint;
			Rectangle		aInRect( aTmpPoint, Size( mnWidth, mnHeight ) );
			aDecoView.DrawFrame( aInRect, nStyle );
		}
	}
}

// =======================================================================

// ---------------------------
// - ImplStdBorderWindowView -
// ---------------------------

ImplStdBorderWindowView::ImplStdBorderWindowView( ImplBorderWindow* pBorderWindow )
{
	maFrameData.mpBorderWindow	= pBorderWindow;
	maFrameData.mbDragFull		= FALSE;
	maFrameData.mnHitTest		= 0;
	maFrameData.mnPinState		= 0;
	maFrameData.mnCloseState	= 0;
	maFrameData.mnRollState 	= 0;
	maFrameData.mnDockState 	= 0;
	maFrameData.mnMenuState 	= 0;
	maFrameData.mnHideState 	= 0;
	maFrameData.mnHelpState 	= 0;
	maFrameData.mbTitleClipped 	= 0;

	mpATitleVirDev				= NULL;
	mpDTitleVirDev				= NULL;
}

// -----------------------------------------------------------------------

ImplStdBorderWindowView::~ImplStdBorderWindowView()
{
	if ( mpATitleVirDev )
		delete mpATitleVirDev;
	if ( mpDTitleVirDev )
		delete mpDTitleVirDev;
}

// -----------------------------------------------------------------------

BOOL ImplStdBorderWindowView::MouseMove( const MouseEvent& rMEvt )
{
	return ImplMouseMove( &maFrameData, rMEvt );
}

// -----------------------------------------------------------------------

BOOL ImplStdBorderWindowView::MouseButtonDown( const MouseEvent& rMEvt )
{
	return ImplMouseButtonDown( &maFrameData, rMEvt );
}

// -----------------------------------------------------------------------

BOOL ImplStdBorderWindowView::Tracking( const TrackingEvent& rTEvt )
{
	return ImplTracking( &maFrameData, rTEvt );
}

// -----------------------------------------------------------------------

String ImplStdBorderWindowView::RequestHelp( const Point& rPos, Rectangle& rHelpRect )
{
	return ImplRequestHelp( &maFrameData, rPos, rHelpRect );
}

// -----------------------------------------------------------------------

Rectangle ImplStdBorderWindowView::GetMenuRect() const
{
    return maFrameData.maMenuRect;
}

// -----------------------------------------------------------------------

void ImplStdBorderWindowView::Init( OutputDevice* pDev, long nWidth, long nHeight )
{
	ImplBorderFrameData*	pData = &maFrameData;
	ImplBorderWindow*		pBorderWindow = maFrameData.mpBorderWindow;
	const StyleSettings&	rStyleSettings = pDev->GetSettings().GetStyleSettings();
	DecorationView			aDecoView( pDev );
	Rectangle				aRect( 0, 0, 10, 10 );
	Rectangle				aCalcRect = aDecoView.DrawFrame( aRect, FRAME_DRAW_DOUBLEOUT | FRAME_DRAW_NODRAW );

	pData->mpOutDev 		= pDev;
	pData->mnWidth			= nWidth;
	pData->mnHeight 		= nHeight;

	pData->mnTitleType		= pBorderWindow->mnTitleType;
	pData->mbFloatWindow	= pBorderWindow->mbFloatWindow;

	if ( !(pBorderWindow->GetStyle() & WB_MOVEABLE) || (pData->mnTitleType == BORDERWINDOW_TITLE_NONE) )
		pData->mnBorderSize = 0;
	else if ( pData->mnTitleType == BORDERWINDOW_TITLE_TEAROFF )
		pData->mnBorderSize = 0;
	else
		pData->mnBorderSize = rStyleSettings.GetBorderSize();
	pData->mnLeftBorder 	= aCalcRect.Left();
	pData->mnTopBorder		= aCalcRect.Top();
	pData->mnRightBorder	= aRect.Right()-aCalcRect.Right();
	pData->mnBottomBorder	= aRect.Bottom()-aCalcRect.Bottom();
	pData->mnLeftBorder    += pData->mnBorderSize;
	pData->mnTopBorder	   += pData->mnBorderSize;
	pData->mnRightBorder   += pData->mnBorderSize;
	pData->mnBottomBorder  += pData->mnBorderSize;
	pData->mnNoTitleTop 	= pData->mnTopBorder;

	ImplInitTitle( &maFrameData );
	if ( pData->mnTitleHeight )
	{
        // to improve symbol display force a minum title height
        if( pData->mnTitleHeight < MIN_CAPTION_HEIGHT )
            pData->mnTitleHeight = MIN_CAPTION_HEIGHT;

        // set a proper background for drawing
        // highlighted buttons in the title
        pBorderWindow->SetBackground( rStyleSettings.GetWindowColor() );

		pData->maTitleRect.Left()	 = pData->mnLeftBorder;
		pData->maTitleRect.Right()	 = nWidth-pData->mnRightBorder-1;
		pData->maTitleRect.Top()	 = pData->mnTopBorder;
		pData->maTitleRect.Bottom()  = pData->maTitleRect.Top()+pData->mnTitleHeight-1;

		if ( pData->mnTitleType & (BORDERWINDOW_TITLE_NORMAL | BORDERWINDOW_TITLE_SMALL) )
		{
			long nLeft			= pData->maTitleRect.Left();
			long nRight 		= pData->maTitleRect.Right();
			long nItemTop		= pData->maTitleRect.Top();
			long nItemBottom	= pData->maTitleRect.Bottom();
			nLeft			   += 1;
			nRight			   -= 3;
			nItemTop		   += 2;
			nItemBottom 	   -= 2;

			if ( pBorderWindow->GetStyle() & WB_PINABLE )
			{
				Image aImage;
				ImplGetPinImage( 0, 0, aImage );
				pData->maPinRect.Top()	  = nItemTop;
				pData->maPinRect.Bottom() = nItemBottom;
				pData->maPinRect.Left()   = nLeft;
				pData->maPinRect.Right()  = pData->maPinRect.Left()+aImage.GetSizePixel().Width();
				nLeft += pData->maPinRect.GetWidth()+3;
			}

			if ( pBorderWindow->GetStyle() & WB_CLOSEABLE )
			{
				pData->maCloseRect.Top()	= nItemTop;
				pData->maCloseRect.Bottom() = nItemBottom;
				pData->maCloseRect.Right()	= nRight;
				pData->maCloseRect.Left()	= pData->maCloseRect.Right()-pData->maCloseRect.GetHeight()+1;
				nRight -= pData->maCloseRect.GetWidth()+3;
			}

			if ( pBorderWindow->mbMenuBtn )
			{
				pData->maMenuRect.Top()    = nItemTop;
				pData->maMenuRect.Bottom() = nItemBottom;
				pData->maMenuRect.Right()  = nRight;
				pData->maMenuRect.Left()   = pData->maMenuRect.Right()-pData->maMenuRect.GetHeight()+1;
				nRight -= pData->maMenuRect.GetWidth();
			}

			if ( pBorderWindow->mbDockBtn )
			{
				pData->maDockRect.Top()    = nItemTop;
				pData->maDockRect.Bottom() = nItemBottom;
				pData->maDockRect.Right()  = nRight;
				pData->maDockRect.Left()   = pData->maDockRect.Right()-pData->maDockRect.GetHeight()+1;
				nRight -= pData->maDockRect.GetWidth();
				if ( !pBorderWindow->mbHideBtn &&
					 !(pBorderWindow->GetStyle() & WB_ROLLABLE) )
					nRight -= 3;
			}

			if ( pBorderWindow->mbHideBtn )
			{
				pData->maHideRect.Top()    = nItemTop;
				pData->maHideRect.Bottom() = nItemBottom;
				pData->maHideRect.Right()  = nRight;
				pData->maHideRect.Left()   = pData->maHideRect.Right()-pData->maHideRect.GetHeight()+1;
				nRight -= pData->maHideRect.GetWidth();
				if ( !(pBorderWindow->GetStyle() & WB_ROLLABLE) )
					nRight -= 3;
			}

			if ( pBorderWindow->GetStyle() & WB_ROLLABLE )
			{
				pData->maRollRect.Top()    = nItemTop;
				pData->maRollRect.Bottom() = nItemBottom;
				pData->maRollRect.Right()  = nRight;
				pData->maRollRect.Left()   = pData->maRollRect.Right()-pData->maRollRect.GetHeight()+1;
				nRight -= pData->maRollRect.GetWidth();
			}

			if ( pBorderWindow->mbHelpBtn )
			{
				pData->maHelpRect.Top()    = nItemTop;
				pData->maHelpRect.Bottom() = nItemBottom;
				pData->maHelpRect.Right()  = nRight;
				pData->maHelpRect.Left()   = pData->maHelpRect.Right()-pData->maHelpRect.GetHeight()+1;
				nRight -= pData->maHelpRect.GetWidth()+3;
			}
		}
		else
		{
			pData->maPinRect.SetEmpty();
			pData->maCloseRect.SetEmpty();
			pData->maDockRect.SetEmpty();
			pData->maMenuRect.SetEmpty();
			pData->maHideRect.SetEmpty();
			pData->maRollRect.SetEmpty();
			pData->maHelpRect.SetEmpty();
		}

		pData->mnTopBorder	+= pData->mnTitleHeight;
	}
	else
	{
		pData->maTitleRect.SetEmpty();
		pData->maPinRect.SetEmpty();
		pData->maCloseRect.SetEmpty();
		pData->maDockRect.SetEmpty();
		pData->maMenuRect.SetEmpty();
		pData->maHideRect.SetEmpty();
		pData->maRollRect.SetEmpty();
		pData->maHelpRect.SetEmpty();
	}
}

// -----------------------------------------------------------------------

void ImplStdBorderWindowView::GetBorder( sal_Int32& rLeftBorder, sal_Int32& rTopBorder,
										 sal_Int32& rRightBorder, sal_Int32& rBottomBorder ) const
{
	rLeftBorder 	= maFrameData.mnLeftBorder;
	rTopBorder		= maFrameData.mnTopBorder;
	rRightBorder	= maFrameData.mnRightBorder;
	rBottomBorder	= maFrameData.mnBottomBorder;
}

// -----------------------------------------------------------------------

long ImplStdBorderWindowView::CalcTitleWidth() const
{
	return ImplCalcTitleWidth( &maFrameData );
}

// -----------------------------------------------------------------------

void ImplStdBorderWindowView::DrawWindow( USHORT nDrawFlags, OutputDevice* pOutDev, const Point* pOffset )
{
	ImplBorderFrameData*	pData = &maFrameData;
	OutputDevice*			pDev = pOutDev ? pOutDev : pData->mpOutDev;
	ImplBorderWindow*		pBorderWindow = pData->mpBorderWindow;
	Point					aTmpPoint = pOffset ? Point(*pOffset) : Point();
	Rectangle				aInRect( aTmpPoint, Size( pData->mnWidth, pData->mnHeight ) );
	const StyleSettings&	rStyleSettings = pData->mpOutDev->GetSettings().GetStyleSettings();
	DecorationView			aDecoView( pDev );
    Color                   aFrameColor( rStyleSettings.GetFaceColor() );

    aFrameColor.DecreaseContrast( (UINT8) (0.50 * 255));

	// Draw Frame
	if ( nDrawFlags & BORDERWINDOW_DRAW_FRAME )
    {
        // single line frame
	    pDev->SetLineColor( aFrameColor );
        pDev->SetFillColor();
        pDev->DrawRect( aInRect );
        aInRect.nLeft++; aInRect.nRight--;
        aInRect.nTop++; aInRect.nBottom--;
    }
	else
	    aInRect = aDecoView.DrawFrame( aInRect, FRAME_DRAW_DOUBLEOUT | FRAME_DRAW_NODRAW);

	// Draw Border
	pDev->SetLineColor();
	long nBorderSize = pData->mnBorderSize;
	if ( (nDrawFlags & BORDERWINDOW_DRAW_BORDER) && nBorderSize )
	{
		pDev->SetFillColor( rStyleSettings.GetFaceColor() );
		pDev->DrawRect( Rectangle( Point( aInRect.Left(), aInRect.Top() ),
								   Size( aInRect.GetWidth(), nBorderSize ) ) );
		pDev->DrawRect( Rectangle( Point( aInRect.Left(), aInRect.Top()+nBorderSize ),
								   Size( nBorderSize, aInRect.GetHeight()-nBorderSize ) ) );
		pDev->DrawRect( Rectangle( Point( aInRect.Left(), aInRect.Bottom()-nBorderSize+1 ),
								   Size( aInRect.GetWidth(), nBorderSize ) ) );
		pDev->DrawRect( Rectangle( Point( aInRect.Right()-nBorderSize+1, aInRect.Top()+nBorderSize ),
								   Size( nBorderSize, aInRect.GetHeight()-nBorderSize ) ) );
	}

	// Draw Title
	if ( (nDrawFlags & BORDERWINDOW_DRAW_TITLE) && !pData->maTitleRect.IsEmpty() )
	{
		aInRect = pData->maTitleRect;

        // use no gradient anymore, just a static titlecolor
		pDev->SetFillColor( aFrameColor );
		pDev->SetTextColor( rStyleSettings.GetButtonTextColor() );
        Rectangle aTitleRect( pData->maTitleRect );
        if( pOffset )
            aTitleRect.Move( pOffset->X(), pOffset->Y() );
        pDev->DrawRect( aTitleRect );


		if ( pData->mnTitleType != BORDERWINDOW_TITLE_TEAROFF )
		{
			aInRect.Left()	+= 2;
			aInRect.Right() -= 2;

			if ( !pData->maPinRect.IsEmpty() )
				aInRect.Left() = pData->maPinRect.Right()+2;

			if ( !pData->maHelpRect.IsEmpty() )
				aInRect.Right() = pData->maHelpRect.Left()-2;
			else if ( !pData->maRollRect.IsEmpty() )
				aInRect.Right() = pData->maRollRect.Left()-2;
			else if ( !pData->maHideRect.IsEmpty() )
				aInRect.Right() = pData->maHideRect.Left()-2;
			else if ( !pData->maDockRect.IsEmpty() )
				aInRect.Right() = pData->maDockRect.Left()-2;
			else if ( !pData->maMenuRect.IsEmpty() )
				aInRect.Right() = pData->maMenuRect.Left()-2;
			else if ( !pData->maCloseRect.IsEmpty() )
				aInRect.Right() = pData->maCloseRect.Left()-2;

			if ( pOffset )
				aInRect.Move( pOffset->X(), pOffset->Y() );

            USHORT nTextStyle = TEXT_DRAW_LEFT | TEXT_DRAW_VCENTER | TEXT_DRAW_ENDELLIPSIS | TEXT_DRAW_CLIP;

            // must show tooltip ?
            TextRectInfo aInfo;
			pDev->GetTextRect( aInRect, pBorderWindow->GetText(), nTextStyle, &aInfo );
            pData->mbTitleClipped = aInfo.IsEllipses();

			pDev->DrawText( aInRect, pBorderWindow->GetText(), nTextStyle );
		}
	}

	if ( ((nDrawFlags & BORDERWINDOW_DRAW_CLOSE) || (nDrawFlags & BORDERWINDOW_DRAW_TITLE)) &&
		 !pData->maCloseRect.IsEmpty() )
	{
		Rectangle aSymbolRect( pData->maCloseRect );
		if ( pOffset )
			aSymbolRect.Move( pOffset->X(), pOffset->Y() );
		ImplDrawBrdWinSymbolButton( pDev, aSymbolRect, SYMBOL_CLOSE, pData->mnCloseState );
	}
	if ( ((nDrawFlags & BORDERWINDOW_DRAW_DOCK) || (nDrawFlags & BORDERWINDOW_DRAW_TITLE)) &&
		 !pData->maDockRect.IsEmpty() )
	{
		Rectangle aSymbolRect( pData->maDockRect );
		if ( pOffset )
			aSymbolRect.Move( pOffset->X(), pOffset->Y() );
		ImplDrawBrdWinSymbolButton( pDev, aSymbolRect, SYMBOL_DOCK, pData->mnDockState );
	}
	if ( ((nDrawFlags & BORDERWINDOW_DRAW_MENU) || (nDrawFlags & BORDERWINDOW_DRAW_TITLE)) &&
		 !pData->maMenuRect.IsEmpty() )
	{
		Rectangle aSymbolRect( pData->maMenuRect );
		if ( pOffset )
			aSymbolRect.Move( pOffset->X(), pOffset->Y() );
		ImplDrawBrdWinSymbolButton( pDev, aSymbolRect, SYMBOL_MENU, pData->mnMenuState );
	}
	if ( ((nDrawFlags & BORDERWINDOW_DRAW_HIDE) || (nDrawFlags & BORDERWINDOW_DRAW_TITLE)) &&
		 !pData->maHideRect.IsEmpty() )
	{
		Rectangle aSymbolRect( pData->maHideRect );
		if ( pOffset )
			aSymbolRect.Move( pOffset->X(), pOffset->Y() );
		ImplDrawBrdWinSymbolButton( pDev, aSymbolRect, SYMBOL_HIDE, pData->mnHideState );
	}
	if ( ((nDrawFlags & BORDERWINDOW_DRAW_ROLL) || (nDrawFlags & BORDERWINDOW_DRAW_TITLE)) &&
		 !pData->maRollRect.IsEmpty() )
	{
		SymbolType eType;
		if ( pBorderWindow->mbRollUp )
			eType = SYMBOL_ROLLDOWN;
		else
			eType = SYMBOL_ROLLUP;
		Rectangle aSymbolRect( pData->maRollRect );
		if ( pOffset )
			aSymbolRect.Move( pOffset->X(), pOffset->Y() );
		ImplDrawBrdWinSymbolButton( pDev, aSymbolRect, eType, pData->mnRollState );
	}

	if ( ((nDrawFlags & BORDERWINDOW_DRAW_HELP) || (nDrawFlags & BORDERWINDOW_DRAW_TITLE)) &&
		 !pData->maHelpRect.IsEmpty() )
	{
		Rectangle aSymbolRect( pData->maHelpRect );
		if ( pOffset )
			aSymbolRect.Move( pOffset->X(), pOffset->Y() );
		ImplDrawBrdWinSymbolButton( pDev, aSymbolRect, SYMBOL_HELP, pData->mnHelpState );
	}
	if ( ((nDrawFlags & BORDERWINDOW_DRAW_PIN) || (nDrawFlags & BORDERWINDOW_DRAW_TITLE)) &&
		 !pData->maPinRect.IsEmpty() )
	{
		Image aImage;
		ImplGetPinImage( pData->mnPinState, pBorderWindow->mbPined, aImage );
		Size  aImageSize = aImage.GetSizePixel();
		long  nRectHeight = pData->maPinRect.GetHeight();
		Point aPos( pData->maPinRect.TopLeft() );
		if ( pOffset )
			aPos.Move( pOffset->X(), pOffset->Y() );
		if ( nRectHeight < aImageSize.Height() )
		{
			pDev->DrawImage( aPos, Size( aImageSize.Width(), nRectHeight ), aImage );
		}
		else
		{
			aPos.Y() += (nRectHeight-aImageSize.Height())/2;
			pDev->DrawImage( aPos, aImage );
		}
	}
}


// =======================================================================
void ImplBorderWindow::ImplInit( Window* pParent,
								 WinBits nStyle, USHORT nTypeStyle,
								 const ::com::sun::star::uno::Any& aSystemToken )
{
	ImplInit( pParent, nStyle, nTypeStyle, NULL );
}

void ImplBorderWindow::ImplInit( Window* pParent,
								 WinBits nStyle, USHORT nTypeStyle,
								 SystemParentData* pSystemParentData
								 )
{
	// Alle WindowBits entfernen, die wir nicht haben wollen
	WinBits nOrgStyle = nStyle;
	WinBits nTestStyle = (WB_MOVEABLE | WB_SIZEABLE | WB_ROLLABLE | WB_PINABLE | WB_CLOSEABLE | WB_STANDALONE | WB_DIALOGCONTROL | WB_NODIALOGCONTROL | WB_SYSTEMFLOATWIN | WB_INTROWIN | WB_DEFAULTWIN | WB_TOOLTIPWIN | WB_NOSHADOW | WB_OWNERDRAWDECORATION);
	if ( nTypeStyle & BORDERWINDOW_STYLE_APP )
		nTestStyle |= WB_APP;
	nStyle &= nTestStyle;

	mpWindowImpl->mbBorderWin 		= TRUE;
	mbSmallOutBorder	= FALSE;
	if ( nTypeStyle & BORDERWINDOW_STYLE_FRAME )
	{
        if( nStyle & WB_OWNERDRAWDECORATION )
        {
		    mpWindowImpl->mbOverlapWin	= TRUE;
		    mpWindowImpl->mbFrame 		= TRUE;
		    mbFrameBorder	= TRUE;
        }
        else
        {
		    mpWindowImpl->mbOverlapWin	= TRUE;
		    mpWindowImpl->mbFrame 		= TRUE;
		    mbFrameBorder	= FALSE;
            // closeable windows may have a border as well, eg. system floating windows without caption
		    if ( (nOrgStyle & (WB_BORDER | WB_NOBORDER | WB_MOVEABLE | WB_SIZEABLE/* | WB_CLOSEABLE*/)) == WB_BORDER )
			    mbSmallOutBorder = TRUE;
        }
	}
	else if ( nTypeStyle & BORDERWINDOW_STYLE_OVERLAP )
	{
		mpWindowImpl->mbOverlapWin	= TRUE;
		mbFrameBorder	= TRUE;
	}
	else
		mbFrameBorder	= FALSE;

	if ( nTypeStyle & BORDERWINDOW_STYLE_FLOAT )
		mbFloatWindow = TRUE;
	else
		mbFloatWindow = FALSE;

	Window::ImplInit( pParent, nStyle, pSystemParentData );
	SetBackground();
	SetTextFillColor();

	mpMenuBarWindow = NULL;
	mnMinWidth		= 0;
	mnMinHeight 	= 0;
	mnMaxWidth		= SHRT_MAX;
	mnMaxHeight 	= SHRT_MAX;
	mnRollHeight	= 0;
	mnOrgMenuHeight = 0;
	mbPined 		= FALSE;
	mbRollUp		= FALSE;
	mbMenuHide		= FALSE;
	mbDockBtn		= FALSE;
	mbMenuBtn		= FALSE;
	mbHideBtn		= FALSE;
	mbHelpBtn		= FALSE;
	mbDisplayActive = IsActive();

	if ( nTypeStyle & BORDERWINDOW_STYLE_FLOAT )
		mnTitleType = BORDERWINDOW_TITLE_SMALL;
	else
		mnTitleType = BORDERWINDOW_TITLE_NORMAL;
	mnBorderStyle	= WINDOW_BORDER_NORMAL;
	InitView();
}

// =======================================================================

ImplBorderWindow::ImplBorderWindow( Window* pParent,
									SystemParentData* pSystemParentData,
									WinBits nStyle, USHORT nTypeStyle
									) :	Window( WINDOW_BORDERWINDOW )
{
	ImplInit( pParent, nStyle, nTypeStyle, pSystemParentData );
}

// -----------------------------------------------------------------------

ImplBorderWindow::ImplBorderWindow( Window* pParent, WinBits nStyle ,
									USHORT nTypeStyle ) :
	Window( WINDOW_BORDERWINDOW )
{
	ImplInit( pParent, nStyle, nTypeStyle, ::com::sun::star::uno::Any() );
}

ImplBorderWindow::ImplBorderWindow( Window* pParent,
									WinBits nStyle, USHORT nTypeStyle,
									const ::com::sun::star::uno::Any& aSystemToken ) :
	Window( WINDOW_BORDERWINDOW )
{
	ImplInit( pParent, nStyle, nTypeStyle, aSystemToken );
}

// -----------------------------------------------------------------------

ImplBorderWindow::~ImplBorderWindow()
{
	delete mpBorderView;
}

// -----------------------------------------------------------------------

void ImplBorderWindow::MouseMove( const MouseEvent& rMEvt )
{
	mpBorderView->MouseMove( rMEvt );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::MouseButtonDown( const MouseEvent& rMEvt )
{
	mpBorderView->MouseButtonDown( rMEvt );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::Tracking( const TrackingEvent& rTEvt )
{
	mpBorderView->Tracking( rTEvt );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::Paint( const Rectangle& rRect )
{
	mpBorderView->DrawWindow( BORDERWINDOW_DRAW_ALL );
}

void ImplBorderWindow::Draw( const Rectangle& rRect, OutputDevice* pOutDev, const Point& rPos )
{
	mpBorderView->DrawWindow( BORDERWINDOW_DRAW_ALL, pOutDev, &rPos );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::Activate()
{
	SetDisplayActive( TRUE );
	Window::Activate();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::Deactivate()
{
	// Fenster die immer Active sind, nehmen wir von dieser Regel aus,
	// genauso, wenn ein Menu aktiv wird, ignorieren wir das Deactivate
	if ( GetActivateMode() && !ImplGetSVData()->maWinData.mbNoDeactivate )
		SetDisplayActive( FALSE );
	Window::Deactivate();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::RequestHelp( const HelpEvent& rHEvt )
{
	// no keyboard help for border win
	if ( rHEvt.GetMode() & (HELPMODE_BALLOON | HELPMODE_QUICK) && !rHEvt.KeyboardActivated() )
	{
		Point		aMousePosPixel = ScreenToOutputPixel( rHEvt.GetMousePosPixel() );
		Rectangle	aHelpRect;
		String      aHelpStr( mpBorderView->RequestHelp( aMousePosPixel, aHelpRect ) );

		// Rechteck ermitteln
		if ( aHelpStr.Len() )
		{
            aHelpRect.SetPos( OutputToScreenPixel( aHelpRect.TopLeft() ) );
			if ( rHEvt.GetMode() & HELPMODE_BALLOON )
				Help::ShowBalloon( this, aHelpRect.Center(), aHelpRect, aHelpStr );
			else
				Help::ShowQuickHelp( this, aHelpRect, aHelpStr );
			return;
		}
	}

	Window::RequestHelp( rHEvt );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::Resize()
{
	Size aSize = GetOutputSizePixel();

	if ( !mbRollUp )
	{
		Window* pClientWindow = ImplGetClientWindow();

		if ( mpMenuBarWindow )
		{
			sal_Int32 nLeftBorder;
			sal_Int32 nTopBorder;
			sal_Int32 nRightBorder;
			sal_Int32 nBottomBorder;
			long nMenuHeight = mpMenuBarWindow->GetSizePixel().Height();
			if ( mbMenuHide )
			{
				if ( nMenuHeight )
					mnOrgMenuHeight = nMenuHeight;
				nMenuHeight = 0;
			}
			else
			{
				if ( !nMenuHeight )
					nMenuHeight = mnOrgMenuHeight;
			}
			mpBorderView->GetBorder( nLeftBorder, nTopBorder, nRightBorder, nBottomBorder );
			mpMenuBarWindow->SetPosSizePixel( nLeftBorder,
											  nTopBorder,
											  aSize.Width()-nLeftBorder-nRightBorder,
											  nMenuHeight,
											  WINDOW_POSSIZE_POS |
											  WINDOW_POSSIZE_WIDTH | WINDOW_POSSIZE_HEIGHT );
		}

		GetBorder( pClientWindow->mpWindowImpl->mnLeftBorder, pClientWindow->mpWindowImpl->mnTopBorder,
				   pClientWindow->mpWindowImpl->mnRightBorder, pClientWindow->mpWindowImpl->mnBottomBorder );
		pClientWindow->ImplPosSizeWindow( pClientWindow->mpWindowImpl->mnLeftBorder,
										  pClientWindow->mpWindowImpl->mnTopBorder,
										  aSize.Width()-pClientWindow->mpWindowImpl->mnLeftBorder-pClientWindow->mpWindowImpl->mnRightBorder,
										  aSize.Height()-pClientWindow->mpWindowImpl->mnTopBorder-pClientWindow->mpWindowImpl->mnBottomBorder,
										  WINDOW_POSSIZE_X | WINDOW_POSSIZE_Y |
										  WINDOW_POSSIZE_WIDTH | WINDOW_POSSIZE_HEIGHT );
	}

	// UpdateView
	mpBorderView->Init( this, aSize.Width(), aSize.Height() );
	InvalidateBorder();

	Window::Resize();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::StateChanged( StateChangedType nType )
{
	if ( (nType == STATE_CHANGE_TEXT) ||
		 (nType == STATE_CHANGE_IMAGE) ||
		 (nType == STATE_CHANGE_DATA) )
	{
		if ( IsReallyVisible() && mbFrameBorder )
		{
			if ( HasPaintEvent() )
				InvalidateBorder();
			else
				mpBorderView->DrawWindow( BORDERWINDOW_DRAW_TITLE );
		}
	}

	Window::StateChanged( nType );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( (rDCEvt.GetType() == DATACHANGED_FONTS) ||
		 (rDCEvt.GetType() == DATACHANGED_FONTSUBSTITUTION) ||
		 ((rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
		  (rDCEvt.GetFlags() & SETTINGS_STYLE)) )
	{
		if ( !mpWindowImpl->mbFrame || (GetStyle() & WB_OWNERDRAWDECORATION) )
			UpdateView( TRUE, ImplGetWindow()->GetOutputSizePixel() );
	}

	Window::DataChanged( rDCEvt );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::InitView()
{
	if ( mbSmallOutBorder )
		mpBorderView = new ImplSmallBorderWindowView( this );
	else if ( mpWindowImpl->mbFrame )
    {
        if( mbFrameBorder )
		    mpBorderView = new ImplStdBorderWindowView( this );
        else
		    mpBorderView = new ImplNoBorderWindowView( this );
    }
	else if ( !mbFrameBorder )
		mpBorderView = new ImplSmallBorderWindowView( this );
	else
		mpBorderView = new ImplStdBorderWindowView( this );
	Size aSize = GetOutputSizePixel();
	mpBorderView->Init( this, aSize.Width(), aSize.Height() );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::UpdateView( BOOL bNewView, const Size& rNewOutSize )
{
	sal_Int32 nLeftBorder;
	sal_Int32 nTopBorder;
	sal_Int32 nRightBorder;
	sal_Int32 nBottomBorder;
	Size aOldSize = GetSizePixel();
	Size aOutputSize = rNewOutSize;

	if ( bNewView )
	{
		delete mpBorderView;
		InitView();
	}
	else
	{
		Size aSize = aOutputSize;
		mpBorderView->GetBorder( nLeftBorder, nTopBorder, nRightBorder, nBottomBorder );
		aSize.Width()  += nLeftBorder+nRightBorder;
		aSize.Height() += nTopBorder+nBottomBorder;
		mpBorderView->Init( this, aSize.Width(), aSize.Height() );
	}

	Window* pClientWindow = ImplGetClientWindow();
	if ( pClientWindow )
	{
		GetBorder( pClientWindow->mpWindowImpl->mnLeftBorder, pClientWindow->mpWindowImpl->mnTopBorder,
				   pClientWindow->mpWindowImpl->mnRightBorder, pClientWindow->mpWindowImpl->mnBottomBorder );
	}
	GetBorder( nLeftBorder, nTopBorder, nRightBorder, nBottomBorder );
	if ( aOldSize.Width() || aOldSize.Height() )
	{
		aOutputSize.Width() 	+= nLeftBorder+nRightBorder;
		aOutputSize.Height()	+= nTopBorder+nBottomBorder;
		if ( aOutputSize == GetSizePixel() )
			InvalidateBorder();
		else
			SetSizePixel( aOutputSize );
	}
}

// -----------------------------------------------------------------------

void ImplBorderWindow::InvalidateBorder()
{
	if ( IsReallyVisible() )
	{
		// Nur wenn wir einen Border haben, muessen wir auch invalidieren
		sal_Int32 nLeftBorder;
		sal_Int32 nTopBorder;
		sal_Int32 nRightBorder;
		sal_Int32 nBottomBorder;
		mpBorderView->GetBorder( nLeftBorder, nTopBorder, nRightBorder, nBottomBorder );
		if ( nLeftBorder || nTopBorder || nRightBorder || nBottomBorder )
		{
			Rectangle	aWinRect( Point( 0, 0 ), GetOutputSizePixel() );
			Region		aRegion( aWinRect );
			aWinRect.Left()   += nLeftBorder;
			aWinRect.Top()	  += nTopBorder;
			aWinRect.Right()  -= nRightBorder;
			aWinRect.Bottom() -= nBottomBorder;
			// kein Output-Bereich mehr, dann alles invalidieren
			if ( (aWinRect.Right() < aWinRect.Left()) ||
				 (aWinRect.Bottom() < aWinRect.Top()) )
				Invalidate( INVALIDATE_NOCHILDREN );
			else
			{
				aRegion.Exclude( aWinRect );
				Invalidate( aRegion, INVALIDATE_NOCHILDREN );
			}
		}
	}
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetDisplayActive( BOOL bActive )
{
	if ( mbDisplayActive != bActive )
	{
		mbDisplayActive = bActive;
		if ( mbFrameBorder )
			InvalidateBorder();
	}
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetTitleType( USHORT nTitleType, const Size& rSize )
{
	mnTitleType = nTitleType;
	UpdateView( FALSE, rSize );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetBorderStyle( USHORT nStyle )
{
	if ( !mbFrameBorder && (mnBorderStyle != nStyle) )
	{
		mnBorderStyle = nStyle;
		UpdateView( FALSE, ImplGetWindow()->GetOutputSizePixel() );
	}
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetPin( BOOL bPin )
{
	mbPined = bPin;
	InvalidateBorder();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetRollUp( BOOL bRollUp, const Size& rSize )
{
	mbRollUp = bRollUp;
	mnRollHeight = rSize.Height();
	UpdateView( FALSE, rSize );
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetCloser()
{
	SetStyle( GetStyle() | WB_CLOSEABLE );
	Size aSize = GetOutputSizePixel();
	mpBorderView->Init( this, aSize.Width(), aSize.Height() );
	InvalidateBorder();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetDockButton( BOOL bDockButton )
{
	mbDockBtn = bDockButton;
	Size aSize = GetOutputSizePixel();
	mpBorderView->Init( this, aSize.Width(), aSize.Height() );
	InvalidateBorder();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetHideButton( BOOL bHideButton )
{
	mbHideBtn = bHideButton;
	Size aSize = GetOutputSizePixel();
	mpBorderView->Init( this, aSize.Width(), aSize.Height() );
	InvalidateBorder();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetHelpButton( BOOL bHelpButton )
{
	mbHelpBtn = bHelpButton;
	Size aSize = GetOutputSizePixel();
	mpBorderView->Init( this, aSize.Width(), aSize.Height() );
	InvalidateBorder();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetMenuButton( BOOL bMenuButton )
{
	mbMenuBtn = bMenuButton;
	Size aSize = GetOutputSizePixel();
	mpBorderView->Init( this, aSize.Width(), aSize.Height() );
	InvalidateBorder();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::UpdateMenuHeight()
{
	Resize();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetMenuBarWindow( Window* pWindow )
{
	mpMenuBarWindow = pWindow;
	UpdateMenuHeight();
	if ( pWindow )
		pWindow->Show();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::SetMenuBarMode( BOOL bHide )
{
	mbMenuHide = bHide;
	UpdateMenuHeight();
}

// -----------------------------------------------------------------------

void ImplBorderWindow::GetBorder( sal_Int32& rLeftBorder, sal_Int32& rTopBorder,
								  sal_Int32& rRightBorder, sal_Int32& rBottomBorder ) const
{
	mpBorderView->GetBorder( rLeftBorder, rTopBorder, rRightBorder, rBottomBorder );
	if ( mpMenuBarWindow && !mbMenuHide )
		rTopBorder += mpMenuBarWindow->GetSizePixel().Height();
}

// -----------------------------------------------------------------------

long ImplBorderWindow::CalcTitleWidth() const
{
	return mpBorderView->CalcTitleWidth();
}

Rectangle ImplBorderWindow::GetMenuRect() const
{
	return mpBorderView->GetMenuRect();
}
