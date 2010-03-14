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
 * Modified May 2009 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_vcl.hxx"

#ifndef _SV_SVSYS_HXX
#include <svsys.h>
#endif
#include <vcl/salwtype.hxx>
#include <vcl/salframe.hxx>
#include <tools/debug.hxx>
#ifndef _INTN_HXX
//#include <tools/intn.hxx>
#endif
#include <vcl/i18nhelp.hxx>
#include <vcl/unohelp.hxx>
#include <unotools/localedatawrapper.hxx>
#include <vcl/svdata.hxx>
#include <vcl/dbggui.hxx>
#include <vcl/windata.hxx>
#include <vcl/timer.hxx>
#include <vcl/event.hxx>
#include <vcl/sound.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/cursor.hxx>
#include <vcl/accmgr.hxx>
#include <vcl/print.h>
#include <vcl/window.h>
#include <vcl/wrkwin.hxx>
#include <vcl/floatwin.hxx>
#include <vcl/dialog.hxx>
#include <vcl/help.hxx>
#include <vcl/helpwin.hxx>
#include <vcl/brdwin.hxx>
#include <vcl/dockwin.hxx>
#include <vcl/salgdi.hxx>
#include <vcl/menu.hxx>

#include <dndlcon.hxx>
#include <com/sun/star/datatransfer/dnd/XDragSource.hpp>
#include <com/sun/star/awt/MouseEvent.hpp>

#if OSL_DEBUG_LEVEL > 1
char dbgbuffer[1024];
#ifndef WNT
#include <stdio.h>
#define MyOutputDebugString(s) (fprintf(stderr, s ))
#else
extern void MyOutputDebugString( char *s);
#endif
#endif


// =======================================================================

#define IMPL_MIN_NEEDSYSWIN         49

// =======================================================================

long ImplCallPreNotify( NotifyEvent& rEvt )
{
    long nRet = Application::CallEventHooks( rEvt );
    if ( !nRet )
        nRet = rEvt.GetWindow()->PreNotify( rEvt );
    return nRet;
}

// =======================================================================

long ImplCallEvent( NotifyEvent& rEvt )
{
    long nRet = ImplCallPreNotify( rEvt );
    if ( !nRet )
    {
        Window* pWindow = rEvt.GetWindow();
        switch ( rEvt.GetType() )
        {
            case EVENT_MOUSEBUTTONDOWN:
                pWindow->MouseButtonDown( *rEvt.GetMouseEvent() );
                break;
            case EVENT_MOUSEBUTTONUP:
                pWindow->MouseButtonUp( *rEvt.GetMouseEvent() );
                break;
            case EVENT_MOUSEMOVE:
                pWindow->MouseMove( *rEvt.GetMouseEvent() );
                break;
            case EVENT_KEYINPUT:
                pWindow->KeyInput( *rEvt.GetKeyEvent() );
                break;
            case EVENT_KEYUP:
                pWindow->KeyUp( *rEvt.GetKeyEvent() );
                break;
            case EVENT_GETFOCUS:
                pWindow->GetFocus();
                break;
            case EVENT_LOSEFOCUS:
                pWindow->LoseFocus();
                break;
            case EVENT_COMMAND:
                pWindow->Command( *rEvt.GetCommandEvent() );
                break;
        }
    }

    return nRet;
}

// =======================================================================

static BOOL ImplHandleMouseFloatMode( Window* pChild, const Point& rMousePos,
                                      USHORT nCode, USHORT nSVEvent,
                                      BOOL bMouseLeave )
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( pSVData->maWinData.mpFirstFloat && !pSVData->maWinData.mpCaptureWin &&
         !pSVData->maWinData.mpFirstFloat->ImplIsFloatPopupModeWindow( pChild ) )
    {
        /*
         *  #93895# since floats are system windows, coordinates have
         *  to be converted to float relative for the hittest
         */
        USHORT          nHitTest = IMPL_FLOATWIN_HITTEST_OUTSIDE;
        FloatingWindow* pFloat = pSVData->maWinData.mpFirstFloat->ImplFloatHitTest( pChild, rMousePos, nHitTest );
        FloatingWindow* pLastLevelFloat;
        ULONG           nPopupFlags;
        if ( nSVEvent == EVENT_MOUSEMOVE )
        {
            if ( bMouseLeave )
                return TRUE;

            if ( !pFloat || (nHitTest & IMPL_FLOATWIN_HITTEST_RECT) )
            {
                if ( pSVData->maHelpData.mpHelpWin && !pSVData->maHelpData.mbKeyboardHelp )
                    ImplDestroyHelpWindow( true );
                pChild->ImplGetFrame()->SetPointer( POINTER_ARROW );
                return TRUE;
            }
        }
        else
        {
            if ( nCode & MOUSE_LEFT )
            {
                if ( nSVEvent == EVENT_MOUSEBUTTONDOWN )
                {
                    if ( !pFloat )
                    {
                        pLastLevelFloat = pSVData->maWinData.mpFirstFloat->ImplFindLastLevelFloat();
                        nPopupFlags = pLastLevelFloat->GetPopupModeFlags();
                        pLastLevelFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
// Erstmal ausgebaut als Hack fuer Bug 53378
//                        if ( nPopupFlags & FLOATWIN_POPUPMODE_PATHMOUSECANCELCLICK )
//                            return FALSE;
//                        else
                            return TRUE;
                    }
                    else if ( nHitTest & IMPL_FLOATWIN_HITTEST_RECT )
                    {
                        if ( !(pFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOMOUSERECTCLOSE) )
                            pFloat->ImplSetMouseDown();
                        return TRUE;
                    }
                }
                else
                {
                    if ( pFloat )
                    {
                        if ( nHitTest & IMPL_FLOATWIN_HITTEST_RECT )
                        {
                            if ( pFloat->ImplIsMouseDown() )
                                pFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL );
                            return TRUE;
                        }
                    }
                    else
                    {
                        pLastLevelFloat = pSVData->maWinData.mpFirstFloat->ImplFindLastLevelFloat();
                        nPopupFlags = pLastLevelFloat->GetPopupModeFlags();
                        if ( !(nPopupFlags & FLOATWIN_POPUPMODE_NOMOUSEUPCLOSE) )
                        {
                            pLastLevelFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
                            return TRUE;
                        }
                    }
                }
            }
            else
            {
                if ( !pFloat )
                {
                    pLastLevelFloat = pSVData->maWinData.mpFirstFloat->ImplFindLastLevelFloat();
                    nPopupFlags = pLastLevelFloat->GetPopupModeFlags();
                    if ( nPopupFlags & FLOATWIN_POPUPMODE_ALLMOUSEBUTTONCLOSE )
                    {
                        if ( (nPopupFlags & FLOATWIN_POPUPMODE_NOMOUSEUPCLOSE) &&
                             (nSVEvent == EVENT_MOUSEBUTTONUP) )
                            return TRUE;
                        pLastLevelFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
                        if ( nPopupFlags & FLOATWIN_POPUPMODE_PATHMOUSECANCELCLICK )
                            return FALSE;
                        else
                            return TRUE;
                    }
                    else
                        return TRUE;
                }
            }
        }
    }

    return FALSE;
}

// -----------------------------------------------------------------------

static void ImplHandleMouseHelpRequest( Window* pChild, const Point& rMousePos )
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( !pSVData->maHelpData.mpHelpWin ||
	     !( pSVData->maHelpData.mpHelpWin->IsWindowOrChild( pChild ) ||
	       pChild->IsWindowOrChild( pSVData->maHelpData.mpHelpWin ) ) )
    {
        USHORT nHelpMode = 0;
        if ( pSVData->maHelpData.mbQuickHelp )
            nHelpMode = HELPMODE_QUICK;
        if ( pSVData->maHelpData.mbBalloonHelp )
            nHelpMode |= HELPMODE_BALLOON;
        if ( nHelpMode )
        {
            if ( pChild->IsInputEnabled() && ! pChild->IsInModalMode() )
            {
                HelpEvent aHelpEvent( rMousePos, nHelpMode );
                pSVData->maHelpData.mbRequestingHelp = TRUE;
                pChild->RequestHelp( aHelpEvent );
                pSVData->maHelpData.mbRequestingHelp = FALSE;
            }
            // #104172# do not kill keyboard activated tooltips
            else if ( pSVData->maHelpData.mpHelpWin && !pSVData->maHelpData.mbKeyboardHelp)
            {
                ImplDestroyHelpWindow( true );
            }
        }
    }
}

// -----------------------------------------------------------------------

static void ImplSetMousePointer( Window* pChild )
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( pSVData->maHelpData.mbExtHelpMode )
        pChild->ImplGetFrame()->SetPointer( POINTER_HELP );
    else
        pChild->ImplGetFrame()->SetPointer( pChild->ImplGetMousePointer() );
}

// -----------------------------------------------------------------------

static BOOL ImplCallCommand( Window* pChild, USHORT nEvt, void* pData = NULL,
                             BOOL bMouse = FALSE, Point* pPos = NULL )
{
    Point aPos;
    if ( pPos )
        aPos = *pPos;
    else
	{
		if( bMouse )
			aPos = pChild->GetPointerPosPixel();
		else
		{
			// simulate mouseposition at center of window
			Size aSize = pChild->GetOutputSize();
			aPos = Point( aSize.getWidth()/2, aSize.getHeight()/2 );
		}
	}

    CommandEvent    aCEvt( aPos, nEvt, bMouse, pData );
    NotifyEvent     aNCmdEvt( EVENT_COMMAND, pChild, &aCEvt );
    ImplDelData     aDelData( pChild );
    BOOL            bPreNotify = (ImplCallPreNotify( aNCmdEvt ) != 0);
    if ( aDelData.IsDelete() )
        return FALSE;
    if ( !bPreNotify )
    {
        pChild->ImplGetWindowImpl()->mbCommand = FALSE;
        pChild->Command( aCEvt );

        if( aDelData.IsDelete() )
            return FALSE;
        pChild->ImplNotifyKeyMouseCommandEventListeners( aNCmdEvt );
        if ( aDelData.IsDelete() )
            return FALSE;
        if ( pChild->ImplGetWindowImpl()->mbCommand )
            return TRUE;
    }

    return FALSE;
}

// -----------------------------------------------------------------------

/*  #i34277# delayed context menu activation;
*   necessary if there already was a popup menu running.
*/

struct ContextMenuEvent
{
    Window*         pWindow;
    ImplDelData     aDelData;
    Point           aChildPos;
};

static long ContextMenuEventLink( void* pCEvent, void* )
{
    ContextMenuEvent* pEv = (ContextMenuEvent*)pCEvent;

    if( ! pEv->aDelData.IsDelete() )
    {
        pEv->pWindow->ImplRemoveDel( &pEv->aDelData );
        ImplCallCommand( pEv->pWindow, COMMAND_CONTEXTMENU, NULL, TRUE, &pEv->aChildPos );
    }
    delete pEv;

    return 0;
}

long ImplHandleMouseEvent( Window* pWindow, USHORT nSVEvent, BOOL bMouseLeave,
                           long nX, long nY, ULONG nMsgTime,
                           USHORT nCode, USHORT nMode )
{
    ImplSVData* pSVData = ImplGetSVData();
    Point       aMousePos( nX, nY );
    Window*     pChild;
    long        nRet;
    USHORT      nClicks;
    ImplFrameData* pWinFrameData = pWindow->ImplGetFrameData();
    USHORT      nOldCode = pWinFrameData->mnMouseCode;

    // we need a mousemove event, befor we get a mousebuttondown or a
    // mousebuttonup event
    if ( (nSVEvent == EVENT_MOUSEBUTTONDOWN) ||
         (nSVEvent == EVENT_MOUSEBUTTONUP) )
    {
        if ( (nSVEvent == EVENT_MOUSEBUTTONUP) && pSVData->maHelpData.mbExtHelpMode )
            Help::EndExtHelp();
        if ( pSVData->maHelpData.mpHelpWin )
        {
            if( pWindow->ImplGetWindow() == pSVData->maHelpData.mpHelpWin )
            {
                ImplDestroyHelpWindow( false );
                return 1; // pWindow is dead now - avoid crash!
            }
            else
                ImplDestroyHelpWindow( true );
        }

        if ( (pWinFrameData->mnLastMouseX != nX) ||
             (pWinFrameData->mnLastMouseY != nY) )
        {
            ImplHandleMouseEvent( pWindow, EVENT_MOUSEMOVE, FALSE, nX, nY, nMsgTime, nCode, nMode );
        }
    }

    // update frame data
    pWinFrameData->mnBeforeLastMouseX = pWinFrameData->mnLastMouseX;
    pWinFrameData->mnBeforeLastMouseY = pWinFrameData->mnLastMouseY;
    pWinFrameData->mnLastMouseX = nX;
    pWinFrameData->mnLastMouseY = nY;
    pWinFrameData->mnMouseCode  = nCode;
    pWinFrameData->mnMouseMode  = nMode & ~(MOUSE_SYNTHETIC | MOUSE_MODIFIERCHANGED);
    if ( bMouseLeave )
    {
        pWinFrameData->mbMouseIn = FALSE;
        if ( pSVData->maHelpData.mpHelpWin && !pSVData->maHelpData.mbKeyboardHelp )
        {
            ImplDelData aDelData( pWindow );

            ImplDestroyHelpWindow( true );

            if ( aDelData.IsDelete() )
                return 1; // pWindow is dead now - avoid crash! (#122045#)
        }
    }
    else
        pWinFrameData->mbMouseIn = TRUE;

    DBG_ASSERT( !pSVData->maWinData.mpTrackWin ||
                (pSVData->maWinData.mpTrackWin == pSVData->maWinData.mpCaptureWin),
                "ImplHandleMouseEvent: TrackWin != CaptureWin" );

    // AutoScrollMode
    if ( pSVData->maWinData.mpAutoScrollWin && (nSVEvent == EVENT_MOUSEBUTTONDOWN) )
    {
        pSVData->maWinData.mpAutoScrollWin->EndAutoScroll();
        return 1;
    }

    // find mouse window
    if ( pSVData->maWinData.mpCaptureWin )
    {
        pChild = pSVData->maWinData.mpCaptureWin;

		DBG_ASSERT( pWindow == pChild->ImplGetFrameWindow(),
                    "ImplHandleMouseEvent: mouse event is not sent to capture window" );

        // java client cannot capture mouse correctly
        if ( pWindow != pChild->ImplGetFrameWindow() )
            return 0;

        if ( bMouseLeave )
            return 0;
    }
    else
    {
        if ( bMouseLeave )
            pChild = NULL;
        else
            pChild = pWindow->ImplFindWindow( aMousePos );
    }

    // test this because mouse events are buffered in the remote version
    // and size may not be in sync
    if ( !pChild && !bMouseLeave )
        return 0;

    // Ein paar Test ausfuehren und Message abfangen oder Status umsetzen
    if ( pChild )
    {
        if( pChild->ImplIsAntiparallel() )
        {
            // - RTL - re-mirror frame pos at pChild
            pChild->ImplReMirror( aMousePos );
        }
        // no mouse messages to system object windows ?
		// !!!KA: Is it OK to comment this out? !!!
//        if ( pChild->ImplGetWindowImpl()->mpSysObj )
//            return 0;

        // no mouse messages to disabled windows
        // #106845# if the window was disabed during capturing we have to pass the mouse events to release capturing
        if ( pSVData->maWinData.mpCaptureWin != pChild && (!pChild->IsEnabled() || !pChild->IsInputEnabled() || pChild->IsInModalMode() ) )
        {
            ImplHandleMouseFloatMode( pChild, aMousePos, nCode, nSVEvent, bMouseLeave );
            if ( nSVEvent == EVENT_MOUSEMOVE )
            {
                ImplHandleMouseHelpRequest( pChild, aMousePos );
                if( pWinFrameData->mpMouseMoveWin != pChild )
                    nMode |= MOUSE_ENTERWINDOW;
            }

            // Call the hook also, if Window is disabled
            Point aChildPos = pChild->ImplFrameToOutput( aMousePos );
            MouseEvent aMEvt( aChildPos, pWinFrameData->mnClickCount, nMode, nCode, nCode );
            NotifyEvent aNEvt( nSVEvent, pChild, &aMEvt );
            Application::CallEventHooks( aNEvt );
            
            if( pChild->IsCallHandlersOnInputDisabled() )
            {                
                pWinFrameData->mpMouseMoveWin = pChild;
                pChild->ImplNotifyKeyMouseCommandEventListeners( aNEvt );
            }

            if ( nSVEvent == EVENT_MOUSEBUTTONDOWN )
            {
                Sound::Beep( SOUND_DISABLE, pChild );
                return 1;
            }
            else
            {
                // Set normal MousePointer for disabled windows
                if ( nSVEvent == EVENT_MOUSEMOVE )
                    ImplSetMousePointer( pChild );

                return 0;
            }
        }

        // End ExtTextInput-Mode, if the user click in the same TopLevel Window
        if ( pSVData->maWinData.mpExtTextInputWin &&
             ((nSVEvent == EVENT_MOUSEBUTTONDOWN) ||
              (nSVEvent == EVENT_MOUSEBUTTONUP)) )
            pSVData->maWinData.mpExtTextInputWin->EndExtTextInput( EXTTEXTINPUT_END_COMPLETE );
    }

    // determine mouse event data
    if ( nSVEvent == EVENT_MOUSEMOVE )
    {
        // Testen, ob MouseMove an das gleiche Fenster geht und sich der
        // Status nicht geaendert hat
        if ( pChild )
        {
            Point aChildMousePos = pChild->ImplFrameToOutput( aMousePos );
            if ( !bMouseLeave &&
                 (pChild == pWinFrameData->mpMouseMoveWin) &&
                 (aChildMousePos.X() == pWinFrameData->mnLastMouseWinX) &&
                 (aChildMousePos.Y() == pWinFrameData->mnLastMouseWinY) &&
                 (nOldCode == pWinFrameData->mnMouseCode) )
            {
                // Mouse-Pointer neu setzen, da er sich geaendet haben
                // koennte, da ein Modus umgesetzt wurde
                ImplSetMousePointer( pChild );
                return 0;
            }

            pWinFrameData->mnLastMouseWinX = aChildMousePos.X();
            pWinFrameData->mnLastMouseWinY = aChildMousePos.Y();
        }

        // mouse click
        nClicks = pWinFrameData->mnClickCount;

        // Gegebenenfalls den Start-Drag-Handler rufen.
        // Achtung: Muss vor Move gerufen werden, da sonst bei schnellen
        // Mausbewegungen die Applikationen in den Selektionszustand gehen.
        Window* pMouseDownWin = pWinFrameData->mpMouseDownWin;
        if ( pMouseDownWin )
        {
            // Testen, ob StartDrag-Modus uebereinstimmt. Wir vergleichen nur
            // den Status der Maustasten, damit man mit Mod1 z.B. sofort
            // in den Kopiermodus gehen kann.
            const MouseSettings& rMSettings = pMouseDownWin->GetSettings().GetMouseSettings();
            if ( (nCode & (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)) ==
                 (rMSettings.GetStartDragCode() & (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)) )
            {
                if ( !pMouseDownWin->ImplGetFrameData()->mbStartDragCalled )
                {
                    long nDragW  = rMSettings.GetStartDragWidth();
                    long nDragH  = rMSettings.GetStartDragWidth();
                    //long nMouseX = nX;
                    //long nMouseY = nY;
                    long nMouseX = aMousePos.X(); // #106074# use the possibly re-mirrored coordinates (RTL) ! nX,nY are unmodified !
                    long nMouseY = aMousePos.Y();
                    if ( !(((nMouseX-nDragW) <= pMouseDownWin->ImplGetFrameData()->mnFirstMouseX) &&
                           ((nMouseX+nDragW) >= pMouseDownWin->ImplGetFrameData()->mnFirstMouseX)) ||
                         !(((nMouseY-nDragH) <= pMouseDownWin->ImplGetFrameData()->mnFirstMouseY) &&
                           ((nMouseY+nDragH) >= pMouseDownWin->ImplGetFrameData()->mnFirstMouseY)) )
                    {
                        pMouseDownWin->ImplGetFrameData()->mbStartDragCalled  = TRUE;

                        // Check if drag source provides it's own recognizer
                        if( pMouseDownWin->ImplGetFrameData()->mbInternalDragGestureRecognizer )
                        {
                            // query DropTarget from child window
                            ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::dnd::XDragGestureRecognizer > xDragGestureRecognizer =
                                ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::dnd::XDragGestureRecognizer > ( pMouseDownWin->ImplGetWindowImpl()->mxDNDListenerContainer,
                                    ::com::sun::star::uno::UNO_QUERY );

                            if( xDragGestureRecognizer.is() )
                            {
                                // retrieve mouse position relative to mouse down window
                                Point relLoc = pMouseDownWin->ImplFrameToOutput( Point(
                                    pMouseDownWin->ImplGetFrameData()->mnFirstMouseX,
                                    pMouseDownWin->ImplGetFrameData()->mnFirstMouseY ) );

                                // create a uno mouse event out of the available data
                                ::com::sun::star::awt::MouseEvent aMouseEvent(
                                    static_cast < ::com::sun::star::uno::XInterface * > ( 0 ),
#ifdef MACOSX
				    nCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2 | KEY_MOD3),
#else
                                    nCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2),
#endif
                                    nCode & (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE),
                                    nMouseX,
                                    nMouseY,
                                    nClicks,
                                    sal_False );

                                ULONG nCount = Application::ReleaseSolarMutex();

                                // FIXME: where do I get Action from ?
                                ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::dnd::XDragSource > xDragSource = pMouseDownWin->GetDragSource();

                                if( xDragSource.is() )
                                {
                                    static_cast < DNDListenerContainer * > ( xDragGestureRecognizer.get() )->fireDragGestureEvent( 0,
                                        relLoc.X(), relLoc.Y(), xDragSource, ::com::sun::star::uno::makeAny( aMouseEvent ) );
                                }

                                Application::AcquireSolarMutex( nCount );
                            }
                        }
                    }
                }
            }
            else
                pMouseDownWin->ImplGetFrameData()->mbStartDragCalled  = TRUE;
        }

        // test for mouseleave and mouseenter
        Window* pMouseMoveWin = pWinFrameData->mpMouseMoveWin;
        if ( pChild != pMouseMoveWin )
        {
            if ( pMouseMoveWin )
            {
                Point       aLeaveMousePos = pMouseMoveWin->ImplFrameToOutput( aMousePos );
                MouseEvent  aMLeaveEvt( aLeaveMousePos, nClicks, nMode | MOUSE_LEAVEWINDOW, nCode, nCode );
                NotifyEvent aNLeaveEvt( EVENT_MOUSEMOVE, pMouseMoveWin, &aMLeaveEvt );
                ImplDelData aDelData;
                ImplDelData aDelData2;
                pWinFrameData->mbInMouseMove = TRUE;
                pMouseMoveWin->ImplGetWinData()->mbMouseOver = FALSE;
                pMouseMoveWin->ImplAddDel( &aDelData );
                // Durch MouseLeave kann auch dieses Fenster zerstoert
                // werden
                if ( pChild )
                    pChild->ImplAddDel( &aDelData2 );
                if ( !ImplCallPreNotify( aNLeaveEvt ) )
                {
                    pMouseMoveWin->MouseMove( aMLeaveEvt );
                    // #82968#
                    if( !aDelData.IsDelete() )
                        aNLeaveEvt.GetWindow()->ImplNotifyKeyMouseCommandEventListeners( aNLeaveEvt );
                }

                pWinFrameData->mpMouseMoveWin = NULL;
                pWinFrameData->mbInMouseMove = FALSE;

                if ( pChild )
                {
                    if ( aDelData2.IsDelete() )
                        pChild = NULL;
                    else
                        pChild->ImplRemoveDel( &aDelData2 );
                }
                if ( aDelData.IsDelete() )
                    return 1;
                pMouseMoveWin->ImplRemoveDel( &aDelData );
            }

            nMode |= MOUSE_ENTERWINDOW;
        }
        pWinFrameData->mpMouseMoveWin = pChild;
        if( pChild )
            pChild->ImplGetWinData()->mbMouseOver = TRUE;

        // MouseLeave
        if ( !pChild )
            return 0;
    }
    else
    {
        // mouse click
        if ( nSVEvent == EVENT_MOUSEBUTTONDOWN )
        {
            const MouseSettings& rMSettings = pChild->GetSettings().GetMouseSettings();
            ULONG   nDblClkTime = rMSettings.GetDoubleClickTime();
            long    nDblClkW    = rMSettings.GetDoubleClickWidth();
            long    nDblClkH    = rMSettings.GetDoubleClickHeight();
            //long    nMouseX     = nX;
            //long    nMouseY     = nY;
            long nMouseX = aMousePos.X();   // #106074# use the possibly re-mirrored coordinates (RTL) ! nX,nY are unmodified !
            long nMouseY = aMousePos.Y();

            if ( (pChild == pChild->ImplGetFrameData()->mpMouseDownWin) &&
                 (nCode == pChild->ImplGetFrameData()->mnFirstMouseCode) &&
                 ((nMsgTime-pChild->ImplGetFrameData()->mnMouseDownTime) < nDblClkTime) &&
                 ((nMouseX-nDblClkW) <= pChild->ImplGetFrameData()->mnFirstMouseX) &&
                 ((nMouseX+nDblClkW) >= pChild->ImplGetFrameData()->mnFirstMouseX) &&
                 ((nMouseY-nDblClkH) <= pChild->ImplGetFrameData()->mnFirstMouseY) &&
                 ((nMouseY+nDblClkH) >= pChild->ImplGetFrameData()->mnFirstMouseY) )
            {
                pChild->ImplGetFrameData()->mnClickCount++;
                pChild->ImplGetFrameData()->mbStartDragCalled  = TRUE;
            }
            else
            {
                pChild->ImplGetFrameData()->mpMouseDownWin     = pChild;
                pChild->ImplGetFrameData()->mnClickCount       = 1;
                pChild->ImplGetFrameData()->mnFirstMouseX      = nMouseX;
                pChild->ImplGetFrameData()->mnFirstMouseY      = nMouseY;
                pChild->ImplGetFrameData()->mnFirstMouseCode   = nCode;
                pChild->ImplGetFrameData()->mbStartDragCalled  = !((nCode & (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)) ==
                                                            (rMSettings.GetStartDragCode() & (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)));
            }
            pChild->ImplGetFrameData()->mnMouseDownTime = nMsgTime;
        }
        nClicks = pChild->ImplGetFrameData()->mnClickCount;

        pSVData->maAppData.mnLastInputTime = Time::GetSystemTicks();
    }

    DBG_ASSERT( pChild, "ImplHandleMouseEvent: pChild == NULL" );

    // create mouse event
    Point aChildPos = pChild->ImplFrameToOutput( aMousePos );
    MouseEvent aMEvt( aChildPos, nClicks, nMode, nCode, nCode );

    // tracking window gets the mouse events
    if ( pSVData->maWinData.mpTrackWin )
        pChild = pSVData->maWinData.mpTrackWin;

    // handle FloatingMode
    if ( !pSVData->maWinData.mpTrackWin && pSVData->maWinData.mpFirstFloat )
    {
        ImplDelData aDelData;
        pChild->ImplAddDel( &aDelData );
        if ( ImplHandleMouseFloatMode( pChild, aMousePos, nCode, nSVEvent, bMouseLeave ) )
        {
            if ( !aDelData.IsDelete() )
            {
                pChild->ImplRemoveDel( &aDelData );
                pChild->ImplGetFrameData()->mbStartDragCalled = TRUE;
            }
            return 1;
        }
        else
            pChild->ImplRemoveDel( &aDelData );
    }

    // call handler
    BOOL bDrag = FALSE;
    BOOL bCallHelpRequest = TRUE;
    DBG_ASSERT( pChild, "ImplHandleMouseEvent: pChild is NULL" );

    ImplDelData aDelData;
    NotifyEvent aNEvt( nSVEvent, pChild, &aMEvt );
    pChild->ImplAddDel( &aDelData );
    if ( nSVEvent == EVENT_MOUSEMOVE )
        pChild->ImplGetFrameData()->mbInMouseMove = TRUE;

    // bring window into foreground on mouseclick
    if ( nSVEvent == EVENT_MOUSEBUTTONDOWN )
    {
        if( !pSVData->maWinData.mpFirstFloat && // totop for floating windows in popup would change the focus and would close them immediately
            !(pChild->ImplGetFrameWindow()->GetStyle() & WB_OWNERDRAWDECORATION) )    // ownerdrawdecorated windows must never grab focus
            pChild->ToTop();
        if ( aDelData.IsDelete() )
            return 1;
    }

    if ( ImplCallPreNotify( aNEvt ) || aDelData.IsDelete() )
        nRet = 1;
    else
    {
        nRet = 0;
        if ( nSVEvent == EVENT_MOUSEMOVE )
        {
            if ( pSVData->maWinData.mpTrackWin )
            {
                TrackingEvent aTEvt( aMEvt );
                pChild->Tracking( aTEvt );
                if ( !aDelData.IsDelete() )
                {
                    // When ScrollRepeat, we restart the timer
                    if ( pSVData->maWinData.mpTrackTimer &&
                         (pSVData->maWinData.mnTrackFlags & STARTTRACK_SCROLLREPEAT) )
                        pSVData->maWinData.mpTrackTimer->Start();
                }
                bCallHelpRequest = FALSE;
                nRet = 1;
            }
            else
            {
                // Auto-ToTop
                if ( !pSVData->maWinData.mpCaptureWin &&
                     (pChild->GetSettings().GetMouseSettings().GetOptions() & MOUSE_OPTION_AUTOFOCUS) )
                    pChild->ToTop( TOTOP_NOGRABFOCUS );

                if( aDelData.IsDelete() )
                    bCallHelpRequest = FALSE;
                else
                {
                    // if the MouseMove handler changes the help window's visibility
                    // the HelpRequest handler should not be called anymore
                    Window* pOldHelpTextWin = pSVData->maHelpData.mpHelpWin;
                    pChild->ImplGetWindowImpl()->mbMouseMove = FALSE;
                    pChild->MouseMove( aMEvt );
                    if ( pOldHelpTextWin != pSVData->maHelpData.mpHelpWin )
                        bCallHelpRequest = FALSE;
                }
            }
        }
        else if ( nSVEvent == EVENT_MOUSEBUTTONDOWN )
        {
            if ( pSVData->maWinData.mpTrackWin &&
                 !(pSVData->maWinData.mnTrackFlags & STARTTRACK_MOUSEBUTTONDOWN) )
                nRet = 1;
            else
            {
                pChild->ImplGetWindowImpl()->mbMouseButtonDown = FALSE;
                pChild->MouseButtonDown( aMEvt );
            }
        }
        else
        {
            if ( pSVData->maWinData.mpTrackWin )
            {
                pChild->EndTracking();
                nRet = 1;
            }
            else
            {
                pChild->ImplGetWindowImpl()->mbMouseButtonUp = FALSE;
                pChild->MouseButtonUp( aMEvt );
            }
        }

        // #82968#
        if ( !aDelData.IsDelete() )
            aNEvt.GetWindow()->ImplNotifyKeyMouseCommandEventListeners( aNEvt );
    }

    if ( aDelData.IsDelete() )
        return 1;


    if ( nSVEvent == EVENT_MOUSEMOVE )
        pChild->ImplGetWindowImpl()->mpFrameData->mbInMouseMove = FALSE;

    if ( nSVEvent == EVENT_MOUSEMOVE )
    {
        if ( bCallHelpRequest && !pSVData->maHelpData.mbKeyboardHelp )
            ImplHandleMouseHelpRequest( pChild, pChild->OutputToScreenPixel( aMEvt.GetPosPixel() ) );
        nRet = 1;
    }
    else if ( !nRet )
    {
        if ( nSVEvent == EVENT_MOUSEBUTTONDOWN )
        {
            if ( !pChild->ImplGetWindowImpl()->mbMouseButtonDown )
                nRet = 1;
        }
        else
        {
            if ( !pChild->ImplGetWindowImpl()->mbMouseButtonUp )
                nRet = 1;
        }
    }

    pChild->ImplRemoveDel( &aDelData );

    if ( nSVEvent == EVENT_MOUSEMOVE )
    {
        // set new mouse pointer
        if ( !bMouseLeave )
            ImplSetMousePointer( pChild );
    }
    else if ( (nSVEvent == EVENT_MOUSEBUTTONDOWN) || (nSVEvent == EVENT_MOUSEBUTTONUP) )
    {
        if ( !bDrag )
        {
            // Command-Events
            if ( /*(nRet == 0) &&*/ (nClicks == 1) && (nSVEvent == EVENT_MOUSEBUTTONDOWN) &&
                 (nCode == MOUSE_MIDDLE) )
            {
                USHORT nMiddleAction = pChild->GetSettings().GetMouseSettings().GetMiddleButtonAction();
                if ( nMiddleAction == MOUSE_MIDDLE_AUTOSCROLL )
                    nRet = !ImplCallCommand( pChild, COMMAND_STARTAUTOSCROLL, NULL, TRUE, &aChildPos );
                else if ( nMiddleAction == MOUSE_MIDDLE_PASTESELECTION )
                    nRet = !ImplCallCommand( pChild, COMMAND_PASTESELECTION, NULL, TRUE, &aChildPos );
            }
            else
            {
                // ContextMenu
                const MouseSettings& rMSettings = pChild->GetSettings().GetMouseSettings();
                if ( (nCode == rMSettings.GetContextMenuCode()) &&
                     (nClicks == rMSettings.GetContextMenuClicks()) )
                {
                    BOOL bContextMenu;
                    if ( rMSettings.GetContextMenuDown() )
                        bContextMenu = (nSVEvent == EVENT_MOUSEBUTTONDOWN);
                    else
                        bContextMenu = (nSVEvent == EVENT_MOUSEBUTTONUP);
                    if ( bContextMenu )
                    {
                        if( pSVData->maAppData.mpActivePopupMenu )
                        {
                            /*  #i34277# there already is a context menu open
                            *   that was probably just closed with EndPopupMode.
                            *   We need to give the eventual corresponding
                            *   PopupMenu::Execute a chance to end properly.
                            *   Therefore delay context menu command and
                            *   issue only after popping one frame of the
                            *   Yield stack.
                            */
                            ContextMenuEvent* pEv = new ContextMenuEvent;
                            pEv->pWindow = pChild;
                            pEv->aChildPos = aChildPos;
                            pChild->ImplAddDel( &pEv->aDelData );
                            Application::PostUserEvent( Link( pEv, ContextMenuEventLink ) );
                        }
                        else
                            nRet = ! ImplCallCommand( pChild, COMMAND_CONTEXTMENU, NULL, TRUE, &aChildPos );
                    }
                }
            }
        }
    }

    return nRet;
}

// -----------------------------------------------------------------------

static Window* ImplGetKeyInputWindow( Window* pWindow )
{
    ImplSVData* pSVData = ImplGetSVData();

    // determine last input time
    pSVData->maAppData.mnLastInputTime = Time::GetSystemTicks();

    // #127104# workaround for destroyed windows
    if( pWindow->ImplGetWindowImpl() == NULL )
        return 0;

    // find window - is every time the window which has currently the
    // focus or the last time the focus.
    // the first floating window always has the focus
    Window* pChild = pSVData->maWinData.mpFirstFloat;
    if( !pChild || ( pChild->ImplGetWindowImpl()->mbFloatWin && !((FloatingWindow *)pChild)->GrabsFocus() ) )
        pChild = pWindow->ImplGetWindowImpl()->mpFrameData->mpFocusWin;
    else
    {
        // allow floaters to forward keyinput to some member
        pChild = pChild->GetPreferredKeyInputWindow();
    }

    // no child - than no input
    if ( !pChild )
        return 0;

    // We call also KeyInput if we haven't the focus, because on Unix
    // system this is often the case when a Lookup Choise Window has
    // the focus - because this windows send the KeyInput directly to
    // the window without resetting the focus
    DBG_ASSERTWARNING( pChild == pSVData->maWinData.mpFocusWin,
                       "ImplHandleKey: Keyboard-Input is sent to a frame without focus" );

    // no keyinput to disabled windows
    if ( !pChild->IsEnabled() || !pChild->IsInputEnabled() || pChild->IsInModalMode() )
        return 0;

    return pChild;
}

// -----------------------------------------------------------------------

static long ImplHandleKey( Window* pWindow, USHORT nSVEvent,
                           USHORT nKeyCode, USHORT nCharCode, USHORT nRepeat, BOOL bForward )
{
    ImplSVData* pSVData = ImplGetSVData();
    KeyCode     aKeyCode( nKeyCode, nKeyCode );
    USHORT      nEvCode = aKeyCode.GetCode();

    // allow application key listeners to remove the key event
    // but make sure we're not forwarding external KeyEvents, (ie where bForward is FALSE)
    // becasue those are coming back from the listener itself and MUST be processed
    KeyEvent aKeyEvent( (xub_Unicode)nCharCode, aKeyCode, nRepeat );
    if( bForward )
    {
        USHORT nVCLEvent;
        switch( nSVEvent )
        {
            case EVENT_KEYINPUT:
                nVCLEvent = VCLEVENT_WINDOW_KEYINPUT;
                break;
            case EVENT_KEYUP:
                nVCLEvent = VCLEVENT_WINDOW_KEYUP;
                break;
            default:
                nVCLEvent = 0;
                break;
        }
        if( nVCLEvent && pSVData->mpApp->HandleKey( nVCLEvent, pWindow, &aKeyEvent ) )
            return 1;
    }

    // #i1820# use locale specific decimal separator
    if( nEvCode == KEY_DECIMAL )
    {
        if( Application::GetSettings().GetMiscSettings().GetEnableLocalizedDecimalSep() )
        {
            String aSep( pWindow->GetSettings().GetLocaleDataWrapper().getNumDecimalSep() );
            nCharCode = (USHORT) aSep.GetChar(0);
        }
    }

	BOOL bCtrlF6 = (aKeyCode.GetCode() == KEY_F6) && aKeyCode.IsMod1();

    // determine last input time
    pSVData->maAppData.mnLastInputTime = Time::GetSystemTicks();

    // handle tracking window
    if ( nSVEvent == EVENT_KEYINPUT )
    {
#ifdef DBG_UTIL
        // #105224# use Ctrl-Alt-Shift-D, Ctrl-Shift-D must be useable by app
        if ( aKeyCode.IsShift() && aKeyCode.IsMod1() && (aKeyCode.IsMod2() || aKeyCode.IsMod3()) && (aKeyCode.GetCode() == KEY_D) )
        {
            DBGGUI_START();
            return 1;
        }
#endif

        if ( pSVData->maHelpData.mbExtHelpMode )
        {
            Help::EndExtHelp();
            if ( nEvCode == KEY_ESCAPE )
                return 1;
        }
        if ( pSVData->maHelpData.mpHelpWin )
            ImplDestroyHelpWindow( false );

        // AutoScrollMode
        if ( pSVData->maWinData.mpAutoScrollWin )
        {
            pSVData->maWinData.mpAutoScrollWin->EndAutoScroll();
            if ( nEvCode == KEY_ESCAPE )
                return 1;
        }

        if ( pSVData->maWinData.mpTrackWin )
        {
            USHORT nOrigCode = aKeyCode.GetCode();

            if ( (nOrigCode == KEY_ESCAPE) && !(pSVData->maWinData.mnTrackFlags & STARTTRACK_NOKEYCANCEL) )
            {
                pSVData->maWinData.mpTrackWin->EndTracking( ENDTRACK_CANCEL | ENDTRACK_KEY );
                if ( pSVData->maWinData.mpFirstFloat )
                {
                    FloatingWindow* pLastLevelFloat = pSVData->maWinData.mpFirstFloat->ImplFindLastLevelFloat();
                    if ( !(pLastLevelFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOKEYCLOSE) )
                    {
                        USHORT nEscCode = aKeyCode.GetCode();

                        if ( nEscCode == KEY_ESCAPE )
                            pLastLevelFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
                    }
                }
                return 1;
            }
            else if ( nOrigCode == KEY_RETURN )
            {
                pSVData->maWinData.mpTrackWin->EndTracking( ENDTRACK_KEY );
                return 1;
            }
            else if ( !(pSVData->maWinData.mnTrackFlags & STARTTRACK_KEYINPUT) )
                return 1;
        }

        // handle FloatingMode
        if ( pSVData->maWinData.mpFirstFloat )
        {
            FloatingWindow* pLastLevelFloat = pSVData->maWinData.mpFirstFloat->ImplFindLastLevelFloat();
            if ( !(pLastLevelFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOKEYCLOSE) )
            {
                USHORT nCode = aKeyCode.GetCode();

                if ( (nCode == KEY_ESCAPE) || bCtrlF6)
                {
                    pLastLevelFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
                    if( !bCtrlF6 )
						return 1;
                }
            }
        }

        // test for accel
        if ( pSVData->maAppData.mpAccelMgr )
        {
            if ( pSVData->maAppData.mpAccelMgr->IsAccelKey( aKeyCode, nRepeat ) )
                return 1;
        }
    }

    // find window
    Window* pChild = ImplGetKeyInputWindow( pWindow );
    if ( !pChild )
        return 0;

    // --- RTL --- mirror cursor keys
    if( (aKeyCode.GetCode() == KEY_LEFT || aKeyCode.GetCode() == KEY_RIGHT) &&
      pChild->ImplHasMirroredGraphics() && pChild->IsRTLEnabled() )
        aKeyCode = KeyCode( aKeyCode.GetCode() == KEY_LEFT ? KEY_RIGHT : KEY_LEFT, aKeyCode.GetModifier() );

    // call handler
    ImplDelData aDelData;
    pChild->ImplAddDel( &aDelData );

	KeyEvent    aKeyEvt( (xub_Unicode)nCharCode, aKeyCode, nRepeat );
    NotifyEvent aNotifyEvt( nSVEvent, pChild, &aKeyEvt );
    BOOL        bKeyPreNotify = (ImplCallPreNotify( aNotifyEvt ) != 0);
    long        nRet = 1;

    if ( !bKeyPreNotify && !aDelData.IsDelete() )
    {
        if ( nSVEvent == EVENT_KEYINPUT )
        {
            pChild->ImplGetWindowImpl()->mbKeyInput = FALSE;
            pChild->KeyInput( aKeyEvt );
        }
        else
        {
            pChild->ImplGetWindowImpl()->mbKeyUp = FALSE;
            pChild->KeyUp( aKeyEvt );
        }
        // #82968#
        if( !aDelData.IsDelete() )
            aNotifyEvt.GetWindow()->ImplNotifyKeyMouseCommandEventListeners( aNotifyEvt );
    }

    if ( aDelData.IsDelete() )
        return 1;

    pChild->ImplRemoveDel( &aDelData );

    if ( nSVEvent == EVENT_KEYINPUT )
    {
        if ( !bKeyPreNotify && pChild->ImplGetWindowImpl()->mbKeyInput )
        {
            USHORT nCode = aKeyCode.GetCode();

            // #101999# is focus in or below toolbox
            BOOL bToolboxFocus=FALSE;
            if( (nCode == KEY_F1) && aKeyCode.IsShift() )
            {
                Window *pWin = pWindow->ImplGetWindowImpl()->mpFrameData->mpFocusWin;
                while( pWin )
                {
                    if( pWin->ImplGetWindowImpl()->mbToolBox )
                    {
                        bToolboxFocus = TRUE;
                        break;
                    }
                    else
                        pWin = pWin->GetParent();
                }
            }

            // ContextMenu
            if ( (nCode == KEY_CONTEXTMENU) || ((nCode == KEY_F10) && aKeyCode.IsShift() && !aKeyCode.IsMod1() && !aKeyCode.IsMod2() ) )
                nRet = !ImplCallCommand( pChild, COMMAND_CONTEXTMENU, NULL, FALSE );
            else if ( ( (nCode == KEY_F2) && aKeyCode.IsShift() ) || ( (nCode == KEY_F1) && aKeyCode.IsMod1() ) ||
                // #101999# no active help when focus in toolbox, simulate BallonHelp instead
                ( (nCode == KEY_F1) && aKeyCode.IsShift() && bToolboxFocus ) )
			{
                // TipHelp via Keyboard (Shift-F2 or Ctrl-F1)
				// simulate mouseposition at center of window

				Size aSize = pChild->GetOutputSize();
				Point aPos = Point( aSize.getWidth()/2, aSize.getHeight()/2 );
				aPos = pChild->OutputToScreenPixel( aPos );

                HelpEvent aHelpEvent( aPos, HELPMODE_BALLOON );
				aHelpEvent.SetKeyboardActivated( TRUE );
				pSVData->maHelpData.mbSetKeyboardHelp = TRUE;
                pChild->RequestHelp( aHelpEvent );
				pSVData->maHelpData.mbSetKeyboardHelp = FALSE;
			}
            else if ( (nCode == KEY_F1) || (nCode == KEY_HELP) )
            {
                if ( !aKeyCode.GetModifier() )
                {
                    if ( pSVData->maHelpData.mbContextHelp )
                    {
                        Point       aMousePos = pChild->OutputToScreenPixel( pChild->GetPointerPosPixel() );
                        HelpEvent   aHelpEvent( aMousePos, HELPMODE_CONTEXT );
                        pChild->RequestHelp( aHelpEvent );
                    }
                    else
                        nRet = 0;
                }
                else if ( aKeyCode.IsShift() )
                {
                    if ( pSVData->maHelpData.mbExtHelp )
                        Help::StartExtHelp();
                    else
                        nRet = 0;
                }
            }
            else
            {
                if ( ImplCallHotKey( aKeyCode ) )
                    nRet = 1;
                else
                    nRet = 0;
            }
        }
    }
    else
    {
        if ( !bKeyPreNotify && pChild->ImplGetWindowImpl()->mbKeyUp )
            nRet = 0;
    }

#ifdef USE_JAVA
    // Fix crashing bug when Impress' Slide Pane is undocked and
    // Command-Shift-F10 is pressed to dock it
    if( !nRet && !pWindow->ImplGetWindowImpl() )
        nRet = 1;
#endif	// USE_JAVA

    // #105591# send keyinput to parent if we are a floating window and the key was not pocessed yet
    if( !nRet && pWindow->ImplGetWindowImpl()->mbFloatWin && pWindow->GetParent() && (pWindow->ImplGetWindowImpl()->mpFrame != pWindow->GetParent()->ImplGetWindowImpl()->mpFrame) )
    {
        pChild = pWindow->GetParent();

        // call handler
        ImplDelData aChildDelData( pChild );
        KeyEvent    aKEvt( (xub_Unicode)nCharCode, aKeyCode, nRepeat );
        NotifyEvent aNEvt( nSVEvent, pChild, &aKEvt );
        BOOL        bPreNotify = (ImplCallPreNotify( aNEvt ) != 0);
        if ( aChildDelData.IsDelete() )
            return 1;

        if ( !bPreNotify )
        {
            if ( nSVEvent == EVENT_KEYINPUT )
            {
                pChild->ImplGetWindowImpl()->mbKeyInput = FALSE;
                pChild->KeyInput( aKEvt );
            }
            else
            {
                pChild->ImplGetWindowImpl()->mbKeyUp = FALSE;
                pChild->KeyUp( aKEvt );
            }
            // #82968#
            if( !aChildDelData.IsDelete() )
                aNEvt.GetWindow()->ImplNotifyKeyMouseCommandEventListeners( aNEvt );
            if ( aChildDelData.IsDelete() )
                return 1;
        }

        if( bPreNotify || !pChild->ImplGetWindowImpl()->mbKeyInput )
            nRet = 1;
    }

    return nRet;
}

// -----------------------------------------------------------------------

static long ImplHandleExtTextInput( Window* pWindow,
                                    const XubString& rText,
                                    const USHORT* pTextAttr,
                                    ULONG nCursorPos, USHORT nCursorFlags )
{
    ImplSVData* pSVData = ImplGetSVData();
    Window*     pChild = NULL;
    
    int nTries = 200;
    while( nTries-- )
    {
        pChild = pSVData->maWinData.mpExtTextInputWin;
        if ( !pChild )
        {
            pChild = ImplGetKeyInputWindow( pWindow );
            if ( !pChild )
                return 0;
        }
        if( !pChild->ImplGetWindowImpl()->mpFrameData->mnFocusId )
            break;
        Application::Yield();
    }

    // If it is the first ExtTextInput call, we inform the information
    // and allocate the data, which we must store in this mode
    ImplWinData* pWinData = pChild->ImplGetWinData();
    if ( !pChild->ImplGetWindowImpl()->mbExtTextInput )
    {
        pChild->ImplGetWindowImpl()->mbExtTextInput = TRUE;
        if ( !pWinData->mpExtOldText )
            pWinData->mpExtOldText = new UniString;
        else
            pWinData->mpExtOldText->Erase();
        if ( pWinData->mpExtOldAttrAry )
        {
            delete [] pWinData->mpExtOldAttrAry;
            pWinData->mpExtOldAttrAry = NULL;
        }
        pSVData->maWinData.mpExtTextInputWin = pChild;
        ImplCallCommand( pChild, COMMAND_STARTEXTTEXTINPUT );
    }

    // be aware of being recursively called in StartExtTextInput
    if ( !pChild->ImplGetWindowImpl()->mbExtTextInput )
        return 0;

    // Test for changes
    BOOL        bOnlyCursor = FALSE;
    xub_StrLen  nMinLen = Min( pWinData->mpExtOldText->Len(), rText.Len() );
    xub_StrLen  nDeltaStart = 0;
    while ( nDeltaStart < nMinLen )
    {
        if ( pWinData->mpExtOldText->GetChar( nDeltaStart ) != rText.GetChar( nDeltaStart ) )
            break;
        nDeltaStart++;
    }
    if ( pWinData->mpExtOldAttrAry || pTextAttr )
    {
        if ( !pWinData->mpExtOldAttrAry || !pTextAttr )
            nDeltaStart = 0;
        else
        {
            xub_StrLen i = 0;
            while ( i < nDeltaStart )
            {
                if ( pWinData->mpExtOldAttrAry[i] != pTextAttr[i] )
                {
                    nDeltaStart = i;
                    break;
                }
                i++;
            }
        }
    }
    if ( (nDeltaStart >= nMinLen) &&
         (pWinData->mpExtOldText->Len() == rText.Len()) )
        bOnlyCursor = TRUE;

    // Call Event and store the information
    CommandExtTextInputData aData( rText, pTextAttr,
                                   (xub_StrLen)nCursorPos, nCursorFlags,
                                   nDeltaStart, pWinData->mpExtOldText->Len(),
                                   bOnlyCursor );
    *pWinData->mpExtOldText = rText;
    if ( pWinData->mpExtOldAttrAry )
    {
        delete [] pWinData->mpExtOldAttrAry;
        pWinData->mpExtOldAttrAry = NULL;
    }
    if ( pTextAttr )
    {
        pWinData->mpExtOldAttrAry = new USHORT[rText.Len()];
        memcpy( pWinData->mpExtOldAttrAry, pTextAttr, rText.Len()*sizeof( USHORT ) );
    }
    return !ImplCallCommand( pChild, COMMAND_EXTTEXTINPUT, &aData );
}

// -----------------------------------------------------------------------

static long ImplHandleEndExtTextInput( Window* /* pWindow */ )
{
    ImplSVData* pSVData = ImplGetSVData();
    Window*     pChild = pSVData->maWinData.mpExtTextInputWin;
    long        nRet = 0;

    if ( pChild )
    {
        pChild->ImplGetWindowImpl()->mbExtTextInput = FALSE;
        pSVData->maWinData.mpExtTextInputWin = NULL;
        ImplWinData* pWinData = pChild->ImplGetWinData();
        if ( pWinData->mpExtOldText )
        {
            delete pWinData->mpExtOldText;
            pWinData->mpExtOldText = NULL;
        }
        if ( pWinData->mpExtOldAttrAry )
        {
            delete [] pWinData->mpExtOldAttrAry;
            pWinData->mpExtOldAttrAry = NULL;
        }
        nRet = !ImplCallCommand( pChild, COMMAND_ENDEXTTEXTINPUT );
    }

    return nRet;
}

// -----------------------------------------------------------------------

static void ImplHandleExtTextInputPos( Window* pWindow,
                                       Rectangle& rRect, long& rInputWidth,
                                       bool * pVertical )
{
    ImplSVData* pSVData = ImplGetSVData();
    Window*     pChild = pSVData->maWinData.mpExtTextInputWin;

    if ( !pChild )
        pChild = ImplGetKeyInputWindow( pWindow );
    else
    {
        // Test, if the Window is related to the frame
        if ( !pWindow->ImplIsWindowOrChild( pChild ) )
            pChild = ImplGetKeyInputWindow( pWindow );
    }

    if ( pChild )
    {
        ImplCallCommand( pChild, COMMAND_CURSORPOS );
        const Rectangle* pRect = pChild->GetCursorRect();
        if ( pRect )
            rRect = pChild->ImplLogicToDevicePixel( *pRect );
        else
        {
            Cursor* pCursor = pChild->GetCursor();
            if ( pCursor )
            {
                Point aPos = pChild->ImplLogicToDevicePixel( pCursor->GetPos() );
                Size aSize = pChild->LogicToPixel( pCursor->GetSize() );
                if ( !aSize.Width() )
                    aSize.Width() = pChild->GetSettings().GetStyleSettings().GetCursorSize();
                rRect = Rectangle( aPos, aSize );
            }
            else
                rRect = Rectangle( Point( pChild->GetOutOffXPixel(), pChild->GetOutOffYPixel() ), Size() );
        }
        rInputWidth = pChild->ImplLogicWidthToDevicePixel( pChild->GetCursorExtTextInputWidth() );
        if ( !rInputWidth )
            rInputWidth = rRect.GetWidth();
    }
    if (pVertical != 0)
        *pVertical
            = pChild != 0 && pChild->GetInputContext().GetFont().IsVertical();
}

// -----------------------------------------------------------------------

static long ImplHandleInputContextChange( Window* pWindow, LanguageType eNewLang )
{
    Window* pChild = ImplGetKeyInputWindow( pWindow );
    CommandInputContextData aData( eNewLang );
    return !ImplCallCommand( pChild, COMMAND_INPUTCONTEXTCHANGE, &aData );
}

// -----------------------------------------------------------------------

static BOOL ImplCallWheelCommand( Window* pWindow, const Point& rPos,
                                  const CommandWheelData* pWheelData )
{
    Point               aCmdMousePos = pWindow->ImplFrameToOutput( rPos );
    CommandEvent        aCEvt( aCmdMousePos, COMMAND_WHEEL, TRUE, pWheelData );
    NotifyEvent         aNCmdEvt( EVENT_COMMAND, pWindow, &aCEvt );
    ImplDelData         aDelData( pWindow );
    BOOL                bPreNotify = (ImplCallPreNotify( aNCmdEvt ) != 0);
    if ( aDelData.IsDelete() )
        return FALSE;
    if ( !bPreNotify )
    {
        pWindow->ImplGetWindowImpl()->mbCommand = FALSE;
        pWindow->Command( aCEvt );
        if ( aDelData.IsDelete() )
            return FALSE;
        if ( pWindow->ImplGetWindowImpl()->mbCommand )
            return TRUE;
    }
    return FALSE;
}

// -----------------------------------------------------------------------

static long ImplHandleWheelEvent( Window* pWindow, const SalWheelMouseEvent& rEvt )
{
    ImplDelData aDogTag( pWindow );

    ImplSVData* pSVData = ImplGetSVData();
    if ( pSVData->maWinData.mpAutoScrollWin )
        pSVData->maWinData.mpAutoScrollWin->EndAutoScroll();
    if ( pSVData->maHelpData.mpHelpWin )
        ImplDestroyHelpWindow( true );
    if( aDogTag.IsDelete() )
        return 0;

    USHORT nMode;
    USHORT nCode = rEvt.mnCode;
    bool bHorz = rEvt.mbHorz;
    if ( nCode & KEY_MOD1 )
        nMode = COMMAND_WHEEL_ZOOM;
    else if ( nCode & KEY_MOD2 )
        nMode = COMMAND_WHEEL_DATAZOOM;
    else
    {
        nMode = COMMAND_WHEEL_SCROLL;
        // #i85450# interpret shift-wheel as horizontal wheel action
        if( (nCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2 | KEY_MOD3)) == KEY_SHIFT )
            bHorz = true;
    }

    CommandWheelData    aWheelData( rEvt.mnDelta, rEvt.mnNotchDelta, rEvt.mnScrollLines, nMode, nCode, bHorz );
    Point               aMousePos( rEvt.mnX, rEvt.mnY );
    BOOL                bRet = TRUE;

    // first check any floating window ( eg. drop down listboxes)
    bool bIsFloat = false;
    Window *pMouseWindow = NULL;
    if ( pSVData->maWinData.mpFirstFloat && !pSVData->maWinData.mpCaptureWin &&
         !pSVData->maWinData.mpFirstFloat->ImplIsFloatPopupModeWindow( pWindow ) )
    {
        USHORT nHitTest = IMPL_FLOATWIN_HITTEST_OUTSIDE;
        pMouseWindow = pSVData->maWinData.mpFirstFloat->ImplFloatHitTest( pWindow, aMousePos, nHitTest );
    }
    // then try the window directly beneath the mouse
    if( !pMouseWindow )
        pMouseWindow = pWindow->ImplFindWindow( aMousePos );
	else
    {
        // transform coordinates to float window frame coordinates
		pMouseWindow = pMouseWindow->ImplFindWindow(
                 pMouseWindow->OutputToScreenPixel(
                  pMouseWindow->AbsoluteScreenToOutputPixel(
				   pWindow->OutputToAbsoluteScreenPixel(
                    pWindow->ScreenToOutputPixel( aMousePos ) ) ) ) );
        bIsFloat = true;
    }

    if ( pMouseWindow &&
         pMouseWindow->IsEnabled() && pMouseWindow->IsInputEnabled() && ! pMouseWindow->IsInModalMode() )
    {
        // transform coordinates to float window frame coordinates
        Point aRelMousePos( pMouseWindow->OutputToScreenPixel(
                             pMouseWindow->AbsoluteScreenToOutputPixel(
                              pWindow->OutputToAbsoluteScreenPixel(
                               pWindow->ScreenToOutputPixel( aMousePos ) ) ) ) );
        bRet = ImplCallWheelCommand( pMouseWindow, aRelMousePos, &aWheelData );
    }

    // if the commad was not handled try the focus window
    if ( bRet )
    {
        Window* pFocusWindow = pWindow->ImplGetWindowImpl()->mpFrameData->mpFocusWin;
        if ( pFocusWindow && (pFocusWindow != pMouseWindow) &&
             (pFocusWindow == pSVData->maWinData.mpFocusWin) )
        {
            // no wheel-messages to disabled windows
            if ( pFocusWindow->IsEnabled() && pFocusWindow->IsInputEnabled() && ! pFocusWindow->IsInModalMode() )
            {
                // transform coordinates to focus window frame coordinates
                Point aRelMousePos( pFocusWindow->OutputToScreenPixel(
                                     pFocusWindow->AbsoluteScreenToOutputPixel(
									  pWindow->OutputToAbsoluteScreenPixel(
                                       pWindow->ScreenToOutputPixel( aMousePos ) ) ) ) );
                bRet = ImplCallWheelCommand( pFocusWindow, aRelMousePos, &aWheelData );
            }
        }
    }
    
    // close floaters
    if( ! bIsFloat && pSVData->maWinData.mpFirstFloat )
    {
        FloatingWindow* pLastLevelFloat = pSVData->maWinData.mpFirstFloat->ImplFindLastLevelFloat();
        if( pLastLevelFloat )
        {
            ULONG nPopupFlags = pLastLevelFloat->GetPopupModeFlags();
            if ( nPopupFlags & FLOATWIN_POPUPMODE_ALLMOUSEBUTTONCLOSE )
            {
                pLastLevelFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
            }
        }
    }

    return !bRet;
}

// -----------------------------------------------------------------------
#define IMPL_PAINT_CHECKRTL         ((USHORT)0x0020)

static void ImplHandlePaint( Window* pWindow, const Rectangle& rBoundRect, bool bImmediateUpdate )
{
    // give up background save when sytem paints arrive
    Window* pSaveBackWin = pWindow->ImplGetWindowImpl()->mpFrameData->mpFirstBackWin;
    while ( pSaveBackWin )
    {
        Window* pNext = pSaveBackWin->ImplGetWindowImpl()->mpOverlapData->mpNextBackWin;
        Rectangle aRect( Point( pSaveBackWin->GetOutOffXPixel(), pSaveBackWin->GetOutOffYPixel() ),
                         Size( pSaveBackWin->GetOutputWidthPixel(), pSaveBackWin->GetOutputHeightPixel() ) );
        if ( aRect.IsOver( rBoundRect ) )
            pSaveBackWin->ImplDeleteOverlapBackground();
        pSaveBackWin = pNext;
    }

    // system paint events must be checked for re-mirroring
    pWindow->ImplGetWindowImpl()->mnPaintFlags |= IMPL_PAINT_CHECKRTL;

    // trigger paint for all windows that live in the new paint region
    Region aRegion( rBoundRect );
    pWindow->ImplInvalidateOverlapFrameRegion( aRegion );
    if( bImmediateUpdate )
    {
        // #i87663# trigger possible pending resize notifications
        // (GetSizePixel does that for us)
        pWindow->GetSizePixel();
        // force drawing inmmediately
        pWindow->Update();
    }
}

// -----------------------------------------------------------------------

static void KillOwnPopups( Window* pWindow )
{
    ImplSVData* pSVData = ImplGetSVData();
	Window *pParent = pWindow->ImplGetWindowImpl()->mpFrameWindow;
    Window *pChild = pSVData->maWinData.mpFirstFloat;
    if ( pChild && pParent->ImplIsWindowOrChild( pChild, TRUE ) )
    {
        if ( !(pSVData->maWinData.mpFirstFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOAPPFOCUSCLOSE) )
            pSVData->maWinData.mpFirstFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
    }
}

// -----------------------------------------------------------------------

void ImplHandleResize( Window* pWindow, long nNewWidth, long nNewHeight )
{
    if( pWindow->GetStyle() & (WB_MOVEABLE|WB_SIZEABLE) )
    {
        KillOwnPopups( pWindow );
        if( pWindow->ImplGetWindow() != ImplGetSVData()->maHelpData.mpHelpWin )
            ImplDestroyHelpWindow( true );
    }

    if ( (nNewWidth > 0) && (nNewHeight > 0) ||
         pWindow->ImplGetWindow()->ImplGetWindowImpl()->mbAllResize )
    {
        if ( (nNewWidth != pWindow->GetOutputWidthPixel()) || (nNewHeight != pWindow->GetOutputHeightPixel()) )
        {
            pWindow->mnOutWidth  = nNewWidth;
            pWindow->mnOutHeight = nNewHeight;
            pWindow->ImplGetWindowImpl()->mbWaitSystemResize = FALSE;
            if ( pWindow->IsReallyVisible() )
                pWindow->ImplSetClipFlag();
            if ( pWindow->IsVisible() || pWindow->ImplGetWindow()->ImplGetWindowImpl()->mbAllResize ||
                ( pWindow->ImplGetWindowImpl()->mbFrame && pWindow->ImplGetWindowImpl()->mpClientWindow ) )   // propagate resize for system border windows
            {
                bool bStartTimer = true;
                // use resize buffering for user resizes
                // ownerdraw decorated windows and floating windows can be resized immediately (i.e. synchronously)
                if( pWindow->ImplGetWindowImpl()->mbFrame && (pWindow->GetStyle() & WB_SIZEABLE)
                    && !(pWindow->GetStyle() & WB_OWNERDRAWDECORATION)  // synchronous resize for ownerdraw decorated windows (toolbars)
                    && !pWindow->ImplGetWindowImpl()->mbFloatWin )             // synchronous resize for floating windows, #i43799#
                {
                    if( pWindow->ImplGetWindowImpl()->mpClientWindow )
                    {
                        // #i42750# presentation wants to be informed about resize
                        // as early as possible
                        WorkWindow* pWorkWindow = dynamic_cast<WorkWindow*>(pWindow->ImplGetWindowImpl()->mpClientWindow);
                        if( pWorkWindow && pWorkWindow->IsPresentationMode() )
                            bStartTimer = false;
                    }
                }
                else
                    bStartTimer = false;

                if( bStartTimer )
                    pWindow->ImplGetWindowImpl()->mpFrameData->maResizeTimer.Start();
                else
                    pWindow->ImplCallResize(); // otherwise menues cannot be positioned
            }
            else
                pWindow->ImplGetWindowImpl()->mbCallResize = TRUE;
        }
    }

    pWindow->ImplGetWindowImpl()->mpFrameData->mbNeedSysWindow = (nNewWidth < IMPL_MIN_NEEDSYSWIN) ||
                                            (nNewHeight < IMPL_MIN_NEEDSYSWIN);
    BOOL bMinimized = (nNewWidth <= 0) || (nNewHeight <= 0);
    if( bMinimized != pWindow->ImplGetWindowImpl()->mpFrameData->mbMinimized )
        pWindow->ImplGetWindowImpl()->mpFrameWindow->ImplNotifyIconifiedState( bMinimized );
    pWindow->ImplGetWindowImpl()->mpFrameData->mbMinimized = bMinimized;
}

// -----------------------------------------------------------------------

static void ImplHandleMove( Window* pWindow )
{
    if( pWindow->ImplGetWindowImpl()->mbFrame && pWindow->ImplIsFloatingWindow() && pWindow->IsReallyVisible() )
    {
        static_cast<FloatingWindow*>(pWindow)->EndPopupMode( FLOATWIN_POPUPMODEEND_TEAROFF );
        pWindow->ImplCallMove();
    }

    if( pWindow->GetStyle() & (WB_MOVEABLE|WB_SIZEABLE) )
    {
        KillOwnPopups( pWindow );
        if( pWindow->ImplGetWindow() != ImplGetSVData()->maHelpData.mpHelpWin )
            ImplDestroyHelpWindow( true );
    }

    if ( pWindow->IsVisible() )
		pWindow->ImplCallMove();
    else
        pWindow->ImplGetWindowImpl()->mbCallMove = TRUE; // make sure the framepos will be updated on the next Show()

    if ( pWindow->ImplGetWindowImpl()->mbFrame && pWindow->ImplGetWindowImpl()->mpClientWindow )
		pWindow->ImplGetWindowImpl()->mpClientWindow->ImplCallMove();	// notify client to update geometry

}

// -----------------------------------------------------------------------

static void ImplHandleMoveResize( Window* pWindow, long nNewWidth, long nNewHeight )
{
    ImplHandleMove( pWindow );
    ImplHandleResize( pWindow, nNewWidth, nNewHeight );
}

// -----------------------------------------------------------------------

static void ImplActivateFloatingWindows( Window* pWindow, BOOL bActive )
{
    // Zuerst alle ueberlappenden Fenster ueberpruefen
    Window* pTempWindow = pWindow->ImplGetWindowImpl()->mpFirstOverlap;
    while ( pTempWindow )
    {
        if ( !pTempWindow->GetActivateMode() )
        {
            if ( (pTempWindow->GetType() == WINDOW_BORDERWINDOW) &&
                 (pTempWindow->ImplGetWindow()->GetType() == WINDOW_FLOATINGWINDOW) )
                ((ImplBorderWindow*)pTempWindow)->SetDisplayActive( bActive );
        }

        ImplActivateFloatingWindows( pTempWindow, bActive );
        pTempWindow = pTempWindow->ImplGetWindowImpl()->mpNext;
    }
}


// -----------------------------------------------------------------------

IMPL_LINK( Window, ImplAsyncFocusHdl, void*, EMPTYARG )
{
    ImplGetWindowImpl()->mpFrameData->mnFocusId = 0;

    // Wenn Status erhalten geblieben ist, weil wir den Focus in der
    // zwischenzeit schon wiederbekommen haben, brauchen wir auch
    // nichts machen
    BOOL bHasFocus = ImplGetWindowImpl()->mpFrameData->mbHasFocus || ImplGetWindowImpl()->mpFrameData->mbSysObjFocus;

    // Dann die zeitverzoegerten Funktionen ausfuehren
    if ( bHasFocus )
    {
        // Alle FloatingFenster deaktiv zeichnen
        if ( ImplGetWindowImpl()->mpFrameData->mbStartFocusState != bHasFocus )
            ImplActivateFloatingWindows( this, bHasFocus );

        if ( ImplGetWindowImpl()->mpFrameData->mpFocusWin )
        {
            BOOL bHandled = FALSE;
            if ( ImplGetWindowImpl()->mpFrameData->mpFocusWin->IsInputEnabled() &&
                 ! ImplGetWindowImpl()->mpFrameData->mpFocusWin->IsInModalMode() )
            {
                if ( ImplGetWindowImpl()->mpFrameData->mpFocusWin->IsEnabled() )
                {
                    ImplGetWindowImpl()->mpFrameData->mpFocusWin->GrabFocus();
                    bHandled = TRUE;
                }
                else if( ImplGetWindowImpl()->mpFrameData->mpFocusWin->ImplHasDlgCtrl() )
                {
                // #109094# if the focus is restored to a disabled dialog control (was disabled meanwhile)
                // try to move it to the next control
                    ImplGetWindowImpl()->mpFrameData->mpFocusWin->ImplDlgCtrlNextWindow();
                    bHandled = TRUE;
                }
            }
            if ( !bHandled )
            {
                ImplSVData* pSVData = ImplGetSVData();
                Window*     pTopLevelWindow = ImplGetWindowImpl()->mpFrameData->mpFocusWin->ImplGetFirstOverlapWindow();
                if ( ( ! pTopLevelWindow->IsInputEnabled() || pTopLevelWindow->IsInModalMode() )
                     && pSVData->maWinData.mpLastExecuteDlg )
                    pSVData->maWinData.mpLastExecuteDlg->ToTop( TOTOP_RESTOREWHENMIN | TOTOP_GRABFOCUSONLY);
                else
                    pTopLevelWindow->GrabFocus();
            }
        }
        else
            GrabFocus();
    }
    else
    {
        Window* pFocusWin = ImplGetWindowImpl()->mpFrameData->mpFocusWin;
        if ( pFocusWin )
        {
            ImplSVData* pSVData = ImplGetSVData();

            if ( pSVData->maWinData.mpFocusWin == pFocusWin )
            {
                // FocusWindow umsetzen
                Window* pOverlapWindow = pFocusWin->ImplGetFirstOverlapWindow();
                pOverlapWindow->ImplGetWindowImpl()->mpLastFocusWindow = pFocusWin;
                pSVData->maWinData.mpFocusWin = NULL;

                if ( pFocusWin->ImplGetWindowImpl()->mpCursor )
                    pFocusWin->ImplGetWindowImpl()->mpCursor->ImplHide();

                // Deaktivate rufen
                Window* pOldFocusWindow = pFocusWin;
                if ( pOldFocusWindow )
                {
                    Window* pOldOverlapWindow = pOldFocusWindow->ImplGetFirstOverlapWindow();
                    Window* pOldRealWindow = pOldOverlapWindow->ImplGetWindow();

                    pOldOverlapWindow->ImplGetWindowImpl()->mbActive = FALSE;
                    pOldOverlapWindow->Deactivate();
                    if ( pOldRealWindow != pOldOverlapWindow )
                    {
                        pOldRealWindow->ImplGetWindowImpl()->mbActive = FALSE;
                        pOldRealWindow->Deactivate();
                    }
                }

                // TrackingMode is ended in ImplHandleLoseFocus
// To avoid problems with the Unix IME
//                pFocusWin->EndExtTextInput( EXTTEXTINPUT_END_COMPLETE );

                // XXX #102010# hack for accessibility: do not close the menu,
                // even after focus lost
                static const char* pEnv = getenv("SAL_FLOATWIN_NOAPPFOCUSCLOSE");
                if( !(pEnv && *pEnv) )
                {
                    NotifyEvent aNEvt( EVENT_LOSEFOCUS, pFocusWin );
                    if ( !ImplCallPreNotify( aNEvt ) )
                        pFocusWin->LoseFocus();
                    pFocusWin->ImplCallDeactivateListeners( NULL );
                    GetpApp()->FocusChanged();
                }
                // XXX
            }
        }

        // Alle FloatingFenster deaktiv zeichnen
        if ( ImplGetWindowImpl()->mpFrameData->mbStartFocusState != bHasFocus )
            ImplActivateFloatingWindows( this, bHasFocus );
    }

    return 0;
}

// -----------------------------------------------------------------------

static void ImplHandleGetFocus( Window* pWindow )
{
    pWindow->ImplGetWindowImpl()->mpFrameData->mbHasFocus = TRUE;

    // Focus-Events zeitverzoegert ausfuehren, damit bei SystemChildFenstern
    // nicht alles flackert, wenn diese den Focus bekommen
    if ( !pWindow->ImplGetWindowImpl()->mpFrameData->mnFocusId )
    {
        bool bCallDirect = ImplGetSVData()->mbIsTestTool;
        pWindow->ImplGetWindowImpl()->mpFrameData->mbStartFocusState = !pWindow->ImplGetWindowImpl()->mpFrameData->mbHasFocus;
        if( ! bCallDirect )
            Application::PostUserEvent( pWindow->ImplGetWindowImpl()->mpFrameData->mnFocusId, LINK( pWindow, Window, ImplAsyncFocusHdl ) );
		Window* pFocusWin = pWindow->ImplGetWindowImpl()->mpFrameData->mpFocusWin;
		if ( pFocusWin && pFocusWin->ImplGetWindowImpl()->mpCursor )
			pFocusWin->ImplGetWindowImpl()->mpCursor->ImplShow();
        if( bCallDirect )
            pWindow->ImplAsyncFocusHdl( NULL );
    }
}

// -----------------------------------------------------------------------

static void ImplHandleLoseFocus( Window* pWindow )
{
    ImplSVData* pSVData = ImplGetSVData();

    // Wenn Frame den Focus verliert, brechen wir auch ein AutoScroll ab
    if ( pSVData->maWinData.mpAutoScrollWin )
        pSVData->maWinData.mpAutoScrollWin->EndAutoScroll();

    // Wenn Frame den Focus verliert, brechen wir auch ein Tracking ab
    if ( pSVData->maWinData.mpTrackWin )
    {
        if ( pSVData->maWinData.mpTrackWin->ImplGetWindowImpl()->mpFrameWindow == pWindow )
            pSVData->maWinData.mpTrackWin->EndTracking( ENDTRACK_CANCEL );
    }

    // handle FloatingMode
    // hier beenden wir immer den PopupModus, auch dann, wenn NOFOCUSCLOSE
    // gesetzt ist, damit wir nicht beim Wechsel noch Fenster stehen lassen
    if ( pSVData->maWinData.mpFirstFloat )
    {
        if ( !(pSVData->maWinData.mpFirstFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOAPPFOCUSCLOSE) )
            pSVData->maWinData.mpFirstFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
    }

    pWindow->ImplGetWindowImpl()->mpFrameData->mbHasFocus = FALSE;

    // Focus-Events zeitverzoegert ausfuehren, damit bei SystemChildFenstern
    // nicht alles flackert, wenn diese den Focus bekommen
    bool bCallDirect = ImplGetSVData()->mbIsTestTool;
    if ( !pWindow->ImplGetWindowImpl()->mpFrameData->mnFocusId )
    {
        pWindow->ImplGetWindowImpl()->mpFrameData->mbStartFocusState = !pWindow->ImplGetWindowImpl()->mpFrameData->mbHasFocus;
        if( ! bCallDirect )
            Application::PostUserEvent( pWindow->ImplGetWindowImpl()->mpFrameData->mnFocusId, LINK( pWindow, Window, ImplAsyncFocusHdl ) );
    }

	Window* pFocusWin = pWindow->ImplGetWindowImpl()->mpFrameData->mpFocusWin;
	if ( pFocusWin && pFocusWin->ImplGetWindowImpl()->mpCursor )
		pFocusWin->ImplGetWindowImpl()->mpCursor->ImplHide();
    if( bCallDirect )
        pWindow->ImplAsyncFocusHdl( NULL );
}

// -----------------------------------------------------------------------
struct DelayedCloseEvent
{
    Window*         pWindow;
    ImplDelData     aDelData;
};

static long DelayedCloseEventLink( void* pCEvent, void* )
{
    DelayedCloseEvent* pEv = (DelayedCloseEvent*)pCEvent;

    if( ! pEv->aDelData.IsDelete() )
    {
        pEv->pWindow->ImplRemoveDel( &pEv->aDelData );
        // dispatch to correct window type
        if( pEv->pWindow->IsSystemWindow() )
            ((SystemWindow*)pEv->pWindow)->Close();
        else if( pEv->pWindow->ImplIsDockingWindow() )
            ((DockingWindow*)pEv->pWindow)->Close();
    }
    delete pEv;

    return 0;
}

void ImplHandleClose( Window* pWindow )
{
    ImplSVData* pSVData = ImplGetSVData();

    bool bWasPopup = false;
    if( pWindow->ImplIsFloatingWindow() &&
        static_cast<FloatingWindow*>(pWindow)->ImplIsInPrivatePopupMode() )
    {
        bWasPopup = true;
    }

    // on Close stop all floating modes and end popups
    if ( pSVData->maWinData.mpFirstFloat )
    {
        FloatingWindow* pLastLevelFloat;
        pLastLevelFloat = pSVData->maWinData.mpFirstFloat->ImplFindLastLevelFloat();
        pLastLevelFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
    }
    if ( pSVData->maHelpData.mbExtHelpMode )
        Help::EndExtHelp();
    if ( pSVData->maHelpData.mpHelpWin )
        ImplDestroyHelpWindow( false );
    // AutoScrollMode
    if ( pSVData->maWinData.mpAutoScrollWin )
        pSVData->maWinData.mpAutoScrollWin->EndAutoScroll();

    if ( pSVData->maWinData.mpTrackWin )
        pSVData->maWinData.mpTrackWin->EndTracking( ENDTRACK_CANCEL | ENDTRACK_KEY );

    if( ! bWasPopup )
    {
        Window *pWin = pWindow->ImplGetWindow();
        // check whether close is allowed
        if ( !pWin->IsEnabled() || !pWin->IsInputEnabled() || pWin->IsInModalMode() )
            Sound::Beep( SOUND_DISABLE, pWin );
        else
        {
            DelayedCloseEvent* pEv = new DelayedCloseEvent;
            pEv->pWindow = pWin;
            pWin->ImplAddDel( &pEv->aDelData );
            Application::PostUserEvent( Link( pEv, DelayedCloseEventLink ) );
        }
    }
}

// -----------------------------------------------------------------------

static void ImplHandleUserEvent( ImplSVEvent* pSVEvent )
{
    if ( pSVEvent )
    {
        if ( pSVEvent->mbCall && !pSVEvent->maDelData.IsDelete() )
        {
            if ( pSVEvent->mpWindow )
            {
                pSVEvent->mpWindow->ImplRemoveDel( &(pSVEvent->maDelData) );
                if ( pSVEvent->mpLink )
                    pSVEvent->mpLink->Call( pSVEvent->mpData );
                else
                    pSVEvent->mpWindow->UserEvent( pSVEvent->mnEvent, pSVEvent->mpData );
            }
            else
            {
                if ( pSVEvent->mpLink )
                    pSVEvent->mpLink->Call( pSVEvent->mpData );
                else
                    GetpApp()->UserEvent( pSVEvent->mnEvent, pSVEvent->mpData );
            }
        }

        delete pSVEvent->mpLink;
        delete pSVEvent;
    }
}

// =======================================================================

static USHORT ImplGetMouseMoveMode( SalMouseEvent* pEvent )
{
    USHORT nMode = 0;
    if ( !pEvent->mnCode )
        nMode |= MOUSE_SIMPLEMOVE;
    if ( (pEvent->mnCode & MOUSE_LEFT) && !(pEvent->mnCode & KEY_MOD1) )
        nMode |= MOUSE_DRAGMOVE;
    if ( (pEvent->mnCode & MOUSE_LEFT) && (pEvent->mnCode & KEY_MOD1) )
        nMode |= MOUSE_DRAGCOPY;
    return nMode;
}

// -----------------------------------------------------------------------

static USHORT ImplGetMouseButtonMode( SalMouseEvent* pEvent )
{
    USHORT nMode = 0;
    if ( pEvent->mnButton == MOUSE_LEFT )
        nMode |= MOUSE_SIMPLECLICK;
    if ( (pEvent->mnButton == MOUSE_LEFT) && !(pEvent->mnCode & (MOUSE_MIDDLE | MOUSE_RIGHT)) )
        nMode |= MOUSE_SELECT;
    if ( (pEvent->mnButton == MOUSE_LEFT) && (pEvent->mnCode & KEY_MOD1) &&
         !(pEvent->mnCode & (MOUSE_MIDDLE | MOUSE_RIGHT | KEY_SHIFT)) )
        nMode |= MOUSE_MULTISELECT;
    if ( (pEvent->mnButton == MOUSE_LEFT) && (pEvent->mnCode & KEY_SHIFT) &&
         !(pEvent->mnCode & (MOUSE_MIDDLE | MOUSE_RIGHT | KEY_MOD1)) )
        nMode |= MOUSE_RANGESELECT;
    return nMode;
}

// -----------------------------------------------------------------------

inline long ImplHandleSalMouseLeave( Window* pWindow, SalMouseEvent* pEvent )
{
    return ImplHandleMouseEvent( pWindow, EVENT_MOUSEMOVE, TRUE,
                                 pEvent->mnX, pEvent->mnY,
                                 pEvent->mnTime, pEvent->mnCode,
                                 ImplGetMouseMoveMode( pEvent ) );
}

// -----------------------------------------------------------------------

inline long ImplHandleSalMouseMove( Window* pWindow, SalMouseEvent* pEvent )
{
    return ImplHandleMouseEvent( pWindow, EVENT_MOUSEMOVE, FALSE,
                                 pEvent->mnX, pEvent->mnY,
                                 pEvent->mnTime, pEvent->mnCode,
                                 ImplGetMouseMoveMode( pEvent ) );
}

// -----------------------------------------------------------------------

inline long ImplHandleSalMouseButtonDown( Window* pWindow, SalMouseEvent* pEvent )
{
    return ImplHandleMouseEvent( pWindow, EVENT_MOUSEBUTTONDOWN, FALSE,
                                 pEvent->mnX, pEvent->mnY,
                                 pEvent->mnTime,
#ifdef MACOSX
				 pEvent->mnButton | (pEvent->mnCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2 | KEY_MOD3)),
#else
                                 pEvent->mnButton | (pEvent->mnCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2)),
#endif
                                 ImplGetMouseButtonMode( pEvent ) );
}

// -----------------------------------------------------------------------

inline long ImplHandleSalMouseButtonUp( Window* pWindow, SalMouseEvent* pEvent )
{
    return ImplHandleMouseEvent( pWindow, EVENT_MOUSEBUTTONUP, FALSE,
                                 pEvent->mnX, pEvent->mnY,
                                 pEvent->mnTime,
#ifdef MACOSX
				 pEvent->mnButton | (pEvent->mnCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2 | KEY_MOD3)),
#else
                                 pEvent->mnButton | (pEvent->mnCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2)),
#endif
                                 ImplGetMouseButtonMode( pEvent ) );
}

// -----------------------------------------------------------------------

static long ImplHandleSalMouseActivate( Window* /*pWindow*/, SalMouseActivateEvent* /*pEvent*/ )
{
    return FALSE;
}

// -----------------------------------------------------------------------

static long ImplHandleMenuEvent( Window* pWindow, SalMenuEvent* pEvent, USHORT nEvent )
{
    // Find SystemWindow and its Menubar and let it dispatch the command
    long nRet = 0;
    Window *pWin = pWindow->ImplGetWindowImpl()->mpFirstChild;
    while ( pWin )
    {
        if ( pWin->ImplGetWindowImpl()->mbSysWin )
            break;
        pWin = pWin->ImplGetWindowImpl()->mpNext;
    }
    if( pWin )
    {
        MenuBar *pMenuBar = ((SystemWindow*) pWin)->GetMenuBar();
        if( pMenuBar )
        {
            switch( nEvent )
            {
                case SALEVENT_MENUACTIVATE:
                    nRet = pMenuBar->HandleMenuActivateEvent( (Menu*) pEvent->mpMenu ) ? 1 : 0;
                    break;
                case SALEVENT_MENUDEACTIVATE:
                    nRet = pMenuBar->HandleMenuDeActivateEvent( (Menu*) pEvent->mpMenu ) ? 1 : 0;
                    break;
                case SALEVENT_MENUHIGHLIGHT:
                    nRet = pMenuBar->HandleMenuHighlightEvent( (Menu*) pEvent->mpMenu, pEvent->mnId ) ? 1 : 0;
                    break;
                case SALEVENT_MENUBUTTONCOMMAND:
                    nRet = pMenuBar->HandleMenuButtonEvent( (Menu*) pEvent->mpMenu, pEvent->mnId ) ? 1 : 0;
                    break;
                case SALEVENT_MENUCOMMAND:
                    nRet = pMenuBar->HandleMenuCommandEvent( (Menu*) pEvent->mpMenu, pEvent->mnId ) ? 1 : 0;
                    break;
                default:
                    break;
            }
        }
    }
    return nRet;
}

// -----------------------------------------------------------------------

static void ImplHandleSalKeyMod( Window* pWindow, SalKeyModEvent* pEvent )
{
    ImplSVData* pSVData = ImplGetSVData();
    Window* pTrackWin = pSVData->maWinData.mpTrackWin;
    if ( pTrackWin )
        pWindow = pTrackWin;
#ifdef MACOSX
    USHORT nOldCode = pWindow->ImplGetWindowImpl()->mpFrameData->mnMouseCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2 | KEY_MOD3);
#else
    USHORT nOldCode = pWindow->ImplGetWindowImpl()->mpFrameData->mnMouseCode & (KEY_SHIFT | KEY_MOD1 | KEY_MOD2);
#endif
    USHORT nNewCode = pEvent->mnCode;
    if ( nOldCode != nNewCode )
    {
#ifdef MACOSX
	nNewCode |= pWindow->ImplGetWindowImpl()->mpFrameData->mnMouseCode & ~(KEY_SHIFT | KEY_MOD1 | KEY_MOD2 | KEY_MOD3);
#else
        nNewCode |= pWindow->ImplGetWindowImpl()->mpFrameData->mnMouseCode & ~(KEY_SHIFT | KEY_MOD1 | KEY_MOD2);
#endif
        pWindow->ImplGetWindowImpl()->mpFrameWindow->ImplCallMouseMove( nNewCode, TRUE );
    }

    // #105224# send commandevent to allow special treatment of Ctrl-LeftShift/Ctrl-RightShift etc.

    // find window
    Window* pChild = ImplGetKeyInputWindow( pWindow );
    if ( !pChild )
        return;

    // send modkey events only if useful data is available
    if( pEvent->mnModKeyCode != 0 )
    {
        CommandModKeyData data( pEvent->mnModKeyCode );
        ImplCallCommand( pChild, COMMAND_MODKEYCHANGE, &data );
    }
}

// -----------------------------------------------------------------------

static void ImplHandleInputLanguageChange( Window* pWindow )
{
    // find window
    Window* pChild = ImplGetKeyInputWindow( pWindow );
    if ( !pChild )
        return;

    ImplCallCommand( pChild, COMMAND_INPUTLANGUAGECHANGE );
}

// -----------------------------------------------------------------------

static void ImplHandleSalSettings( Window* pWindow, USHORT nEvent )
{
    // Application Notification werden nur fuer das erste Window ausgeloest
    ImplSVData* pSVData = ImplGetSVData();
    if ( pWindow != pSVData->maWinData.mpFirstFrame )
        return;

    Application* pApp = GetpApp();
    if ( !pApp )
        return;

    if ( nEvent == SALEVENT_SETTINGSCHANGED )
    {
        AllSettings aSettings = pApp->GetSettings();
        pApp->MergeSystemSettings( aSettings );
        pApp->SystemSettingsChanging( aSettings, pWindow );
        pApp->SetSettings( aSettings );
    }
    else
    {
        USHORT nType;
        switch ( nEvent )
        {
            case SALEVENT_VOLUMECHANGED:
                nType = 0;
                break;
            case SALEVENT_PRINTERCHANGED:
                ImplDeletePrnQueueList();
                nType = DATACHANGED_PRINTER;
                break;
            case SALEVENT_DISPLAYCHANGED:
                nType = DATACHANGED_DISPLAY;
                break;
            case SALEVENT_FONTCHANGED:
                OutputDevice::ImplUpdateAllFontData( TRUE );
                nType = DATACHANGED_FONTS;
                break;
            case SALEVENT_DATETIMECHANGED:
                nType = DATACHANGED_DATETIME;
                break;
            case SALEVENT_KEYBOARDCHANGED:
                nType = 0;
                break;
            default:
                nType = 0;
                break;
        }

        if ( nType )
        {
            DataChangedEvent aDCEvt( nType );
            pApp->DataChanged( aDCEvt );
            pApp->NotifyAllWindows( aDCEvt );
        }
    }
}

// -----------------------------------------------------------------------

static void ImplHandleSalExtTextInputPos( Window* pWindow, SalExtTextInputPosEvent* pEvt )
{
    Rectangle aCursorRect;
    ImplHandleExtTextInputPos( pWindow, aCursorRect, pEvt->mnExtWidth, &pEvt->mbVertical );
    if ( aCursorRect.IsEmpty() )
    {
        pEvt->mnX       = -1;
        pEvt->mnY       = -1;
        pEvt->mnWidth   = -1;
        pEvt->mnHeight  = -1;
    }
    else
    {
        pEvt->mnX       = aCursorRect.Left();
        pEvt->mnY       = aCursorRect.Top();
        pEvt->mnWidth   = aCursorRect.GetWidth();
        pEvt->mnHeight  = aCursorRect.GetHeight();
    }
}

// -----------------------------------------------------------------------

static long ImplHandleShowDialog( Window* pWindow, int nDialogId )
{
    if( ! pWindow )
        return FALSE;
    
    if( pWindow->GetType() == WINDOW_BORDERWINDOW )
    {
        Window* pWrkWin = pWindow->GetWindow( WINDOW_CLIENT );
        if( pWrkWin )
            pWindow = pWrkWin;
    }
    CommandDialogData aCmdData( nDialogId );
    return ImplCallCommand( pWindow, COMMAND_SHOWDIALOG, &aCmdData );
}

// -----------------------------------------------------------------------

static void ImplHandleSurroundingTextRequest( Window *pWindow,
					      XubString& rText,
					      Selection &rSelRange )
{
    Window* pChild = ImplGetKeyInputWindow( pWindow );

    if ( !pChild )
    {
	rText = XubString::EmptyString();
	rSelRange.setMin( 0 );
	rSelRange.setMax( 0 );
    }
    else
    {

	rText = pChild->GetSurroundingText();
	Selection aSel = pChild->GetSurroundingTextSelection();
	rSelRange.setMin( aSel.Min() );
	rSelRange.setMax( aSel.Max() );
    }
}

// -----------------------------------------------------------------------

static void ImplHandleSalSurroundingTextRequest( Window *pWindow,
						 SalSurroundingTextRequestEvent *pEvt )
{
	Selection aSelRange;
	ImplHandleSurroundingTextRequest( pWindow, pEvt->maText, aSelRange );

	aSelRange.Justify();

	if( aSelRange.Min() < 0 )
		pEvt->mnStart = 0;
	else if( aSelRange.Min() > pEvt->maText.Len() )
		pEvt->mnStart = pEvt->maText.Len();
	else
		pEvt->mnStart = aSelRange.Min();

	if( aSelRange.Max() < 0 )
		pEvt->mnStart = 0;
	else if( aSelRange.Max() > pEvt->maText.Len() )
		pEvt->mnEnd = pEvt->maText.Len();
	else
		pEvt->mnEnd = aSelRange.Max();
}

// -----------------------------------------------------------------------

static void ImplHandleSurroundingTextSelectionChange( Window *pWindow,
						      ULONG nStart,
						      ULONG nEnd )
{
    Window* pChild = ImplGetKeyInputWindow( pWindow );
    if( pChild )
    {
        CommandSelectionChangeData data( nStart, nEnd );
        ImplCallCommand( pChild, COMMAND_SELECTIONCHANGE, &data );
    }
}

// -----------------------------------------------------------------------

static void ImplHandleStartReconversion( Window *pWindow )
{
    Window* pChild = ImplGetKeyInputWindow( pWindow );
    if( pChild )
	ImplCallCommand( pChild, COMMAND_PREPARERECONVERSION );
}

// -----------------------------------------------------------------------

long ImplWindowFrameProc( Window* pWindow, SalFrame* /*pFrame*/,
                          USHORT nEvent, const void* pEvent )
{
    DBG_TESTSOLARMUTEX();

    long nRet = 0;

    // #119709# for some unknown reason it is possible to receive events (in this case key events)
    // although the corresponding VCL window must have been destroyed already
    // at least ImplGetWindowImpl() was NULL in these cases, so check this here
    if( pWindow->ImplGetWindowImpl() == NULL )
        return 0;

    switch ( nEvent )
    {
        case SALEVENT_MOUSEMOVE:
            nRet = ImplHandleSalMouseMove( pWindow, (SalMouseEvent*)pEvent );
            break;
        case SALEVENT_EXTERNALMOUSEMOVE:
        {
            MouseEvent*     pMouseEvt = (MouseEvent*) pEvent;
            SalMouseEvent   aSalMouseEvent;

            aSalMouseEvent.mnTime = Time::GetSystemTicks();
            aSalMouseEvent.mnX = pMouseEvt->GetPosPixel().X();
            aSalMouseEvent.mnY = pMouseEvt->GetPosPixel().Y();
            aSalMouseEvent.mnButton = 0;
            aSalMouseEvent.mnCode = pMouseEvt->GetButtons() | pMouseEvt->GetModifier();

            nRet = ImplHandleSalMouseMove( pWindow, &aSalMouseEvent );
        }
        break;
        case SALEVENT_MOUSELEAVE:
            nRet = ImplHandleSalMouseLeave( pWindow, (SalMouseEvent*)pEvent );
            break;
        case SALEVENT_MOUSEBUTTONDOWN:
            nRet = ImplHandleSalMouseButtonDown( pWindow, (SalMouseEvent*)pEvent );
            break;
        case SALEVENT_EXTERNALMOUSEBUTTONDOWN:
        {
            MouseEvent*     pMouseEvt = (MouseEvent*) pEvent;
            SalMouseEvent   aSalMouseEvent;

            aSalMouseEvent.mnTime = Time::GetSystemTicks();
            aSalMouseEvent.mnX = pMouseEvt->GetPosPixel().X();
            aSalMouseEvent.mnY = pMouseEvt->GetPosPixel().Y();
            aSalMouseEvent.mnButton = pMouseEvt->GetButtons();
            aSalMouseEvent.mnCode = pMouseEvt->GetButtons() | pMouseEvt->GetModifier();

            nRet = ImplHandleSalMouseButtonDown( pWindow, &aSalMouseEvent );
        }
        break;
        case SALEVENT_MOUSEBUTTONUP:
            nRet = ImplHandleSalMouseButtonUp( pWindow, (SalMouseEvent*)pEvent );
            break;
        case SALEVENT_EXTERNALMOUSEBUTTONUP:
        {
            MouseEvent*     pMouseEvt = (MouseEvent*) pEvent;
            SalMouseEvent   aSalMouseEvent;

            aSalMouseEvent.mnTime = Time::GetSystemTicks();
            aSalMouseEvent.mnX = pMouseEvt->GetPosPixel().X();
            aSalMouseEvent.mnY = pMouseEvt->GetPosPixel().Y();
            aSalMouseEvent.mnButton = pMouseEvt->GetButtons();
            aSalMouseEvent.mnCode = pMouseEvt->GetButtons() | pMouseEvt->GetModifier();

            nRet = ImplHandleSalMouseButtonUp( pWindow, &aSalMouseEvent );
        }
        break;
        case SALEVENT_MOUSEACTIVATE:
            nRet = ImplHandleSalMouseActivate( pWindow, (SalMouseActivateEvent*)pEvent );
            break;
        case SALEVENT_KEYINPUT:
            {
            SalKeyEvent* pKeyEvt = (SalKeyEvent*)pEvent;
            nRet = ImplHandleKey( pWindow, EVENT_KEYINPUT,
                pKeyEvt->mnCode, pKeyEvt->mnCharCode, pKeyEvt->mnRepeat, TRUE );
            }
            break;
        case SALEVENT_EXTERNALKEYINPUT:
            {
            KeyEvent* pKeyEvt = (KeyEvent*) pEvent;
            nRet = ImplHandleKey( pWindow, EVENT_KEYINPUT,
                pKeyEvt->GetKeyCode().GetFullCode(), pKeyEvt->GetCharCode(), pKeyEvt->GetRepeat(), FALSE );
            }
            break;
        case SALEVENT_KEYUP:
            {
            SalKeyEvent* pKeyEvt = (SalKeyEvent*)pEvent;
            nRet = ImplHandleKey( pWindow, EVENT_KEYUP,
                pKeyEvt->mnCode, pKeyEvt->mnCharCode, pKeyEvt->mnRepeat, TRUE );
            }
            break;
        case SALEVENT_EXTERNALKEYUP:
            {
            KeyEvent* pKeyEvt = (KeyEvent*) pEvent;
            nRet = ImplHandleKey( pWindow, EVENT_KEYUP,
                pKeyEvt->GetKeyCode().GetFullCode(), pKeyEvt->GetCharCode(), pKeyEvt->GetRepeat(), FALSE );
            }
            break;
        case SALEVENT_KEYMODCHANGE:
            ImplHandleSalKeyMod( pWindow, (SalKeyModEvent*)pEvent );
            break;

        case SALEVENT_INPUTLANGUAGECHANGE:
            ImplHandleInputLanguageChange( pWindow );
            break;

        case SALEVENT_MENUACTIVATE:
        case SALEVENT_MENUDEACTIVATE:
        case SALEVENT_MENUHIGHLIGHT:
        case SALEVENT_MENUCOMMAND:
        case SALEVENT_MENUBUTTONCOMMAND:
            nRet = ImplHandleMenuEvent( pWindow, (SalMenuEvent*)pEvent, nEvent );
            break;

        case SALEVENT_WHEELMOUSE:
            nRet = ImplHandleWheelEvent( pWindow, *(const SalWheelMouseEvent*)pEvent);
            break;

        case SALEVENT_PAINT:
            {
            SalPaintEvent* pPaintEvt = (SalPaintEvent*)pEvent;

            if( Application::GetSettings().GetLayoutRTL() )
            {
                // --- RTL --- (mirror paint rect)
                SalFrame* pSalFrame = pWindow->ImplGetWindowImpl()->mpFrame;
                pPaintEvt->mnBoundX = pSalFrame->maGeometry.nWidth-pPaintEvt->mnBoundWidth-pPaintEvt->mnBoundX;
            }

            Rectangle aBoundRect( Point( pPaintEvt->mnBoundX, pPaintEvt->mnBoundY ),
                                  Size( pPaintEvt->mnBoundWidth, pPaintEvt->mnBoundHeight ) );
            ImplHandlePaint( pWindow, aBoundRect, pPaintEvt->mbImmediateUpdate );
            }
            break;

        case SALEVENT_MOVE:
            ImplHandleMove( pWindow );
            break;

        case SALEVENT_RESIZE:
            {
            long nNewWidth;
            long nNewHeight;
            pWindow->ImplGetWindowImpl()->mpFrame->GetClientSize( nNewWidth, nNewHeight );
            ImplHandleResize( pWindow, nNewWidth, nNewHeight );
            }
            break;

        case SALEVENT_MOVERESIZE:
            {
            SalFrameGeometry g = pWindow->ImplGetWindowImpl()->mpFrame->GetGeometry();
            ImplHandleMoveResize( pWindow, g.nWidth, g.nHeight );
            }
            break;

        case SALEVENT_CLOSEPOPUPS:
            {
            KillOwnPopups( pWindow );
            }
            break;

        case SALEVENT_GETFOCUS:
            ImplHandleGetFocus( pWindow );
            break;
        case SALEVENT_LOSEFOCUS:
            ImplHandleLoseFocus( pWindow );
            break;

        case SALEVENT_CLOSE:
            ImplHandleClose( pWindow );
            break;

        case SALEVENT_SHUTDOWN:
			{
				static bool bInQueryExit = false;
				if( !bInQueryExit )
				{
					bInQueryExit = true;
					if ( GetpApp()->QueryExit() )
					{
						// Message-Schleife beenden
						Application::Quit();
						return FALSE;
					}
					else
					{
						bInQueryExit = false;
						return TRUE;
					}
				}
                return FALSE;
			}

        case SALEVENT_SETTINGSCHANGED:
        case SALEVENT_VOLUMECHANGED:
        case SALEVENT_PRINTERCHANGED:
        case SALEVENT_DISPLAYCHANGED:
        case SALEVENT_FONTCHANGED:
        case SALEVENT_DATETIMECHANGED:
        case SALEVENT_KEYBOARDCHANGED:
            ImplHandleSalSettings( pWindow, nEvent );
            break;

        case SALEVENT_USEREVENT:
            ImplHandleUserEvent( (ImplSVEvent*)pEvent );
            break;

        case SALEVENT_EXTTEXTINPUT:
            {
            SalExtTextInputEvent* pEvt = (SalExtTextInputEvent*)pEvent;
            nRet = ImplHandleExtTextInput( pWindow,
                                           pEvt->maText, pEvt->mpTextAttr,
                                           pEvt->mnCursorPos, pEvt->mnCursorFlags );
            }
            break;
        case SALEVENT_ENDEXTTEXTINPUT:
            nRet = ImplHandleEndExtTextInput( pWindow );
            break;
        case SALEVENT_EXTTEXTINPUTPOS:
            ImplHandleSalExtTextInputPos( pWindow, (SalExtTextInputPosEvent*)pEvent );
            break;
        case SALEVENT_INPUTCONTEXTCHANGE:
            nRet = ImplHandleInputContextChange( pWindow, ((SalInputContextChangeEvent*)pEvent)->meLanguage );
            break;
        case SALEVENT_SHOWDIALOG:
            {
                int nDialogID = static_cast<int>(reinterpret_cast<sal_IntPtr>(pEvent));
                nRet = ImplHandleShowDialog( pWindow, nDialogID );
            }
            break;
        case SALEVENT_SURROUNDINGTEXTREQUEST:
            ImplHandleSalSurroundingTextRequest( pWindow, (SalSurroundingTextRequestEvent*)pEvent );
            break;
        case SALEVENT_SURROUNDINGTEXTSELECTIONCHANGE:
        {
            SalSurroundingTextSelectionChangeEvent* pEvt
             = (SalSurroundingTextSelectionChangeEvent*)pEvent;
            ImplHandleSurroundingTextSelectionChange( pWindow,
						      pEvt->mnStart,
						      pEvt->mnEnd );
        }
        case SALEVENT_STARTRECONVERSION:
            ImplHandleStartReconversion( pWindow );
            break;
#ifdef DBG_UTIL
        default:
            DBG_ERROR1( "ImplWindowFrameProc(): unknown event (%lu)", (ULONG)nEvent );
            break;
#endif
    }

    return nRet;
}
