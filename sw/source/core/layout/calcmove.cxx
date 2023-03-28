/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include "rootfrm.hxx"
#include "pagefrm.hxx"
#include "viewopt.hxx"
#include "frmtool.hxx"
#include "txtftn.hxx"
#include "fmtftn.hxx"
#include <editeng/ulspitem.hxx>
#include <editeng/keepitem.hxx>
#include <svx/sdtaitm.hxx>

#include <fmtfsize.hxx>
#include <fmtanchr.hxx>
#include <fmtclbl.hxx>

#include "tabfrm.hxx"
#include "ftnfrm.hxx"
#include "txtfrm.hxx"
#include "sectfrm.hxx"
#include "dbg_lay.hxx"

#include <sortedobjs.hxx>
#include <layouter.hxx>
#include <flyfrms.hxx>

#include <DocumentSettingManager.hxx>
#include <IDocumentLayoutAccess.hxx>

#ifdef USE_JAVA

#include <list>
#include <map>

#define STOP_FORMAT_INTERVAL 1000

static sal_uInt32 nStopFormatMillis = 0;
static std::list< std::pair< SwFrm*, bool > > aStopFormatStack;
static std::map< SwFrm*, SwFrm* > aStopFormatStackMap;
static sal_uInt32 nDisableStopFormatTimer = 0;
static bool bStopFormatInvalidateSize = false;
static bool bStopFormatInvalidatePrtArea = false;
static std::map< SwFrm*, SwFrm* > aStopFormatInvalidateMap;

bool PushToStopFormatStack( SwFrm *pFrm, bool bDisableTimer, sal_uInt32 nTimerInterval )
{
    bool bRet = false;

    if ( bDisableTimer )
        nDisableStopFormatTimer++;

    if ( aStopFormatStack.size() )
        bRet = ( !nDisableStopFormatTimer && osl_getGlobalTimer() >= nStopFormatMillis );
    else
        nStopFormatMillis = osl_getGlobalTimer() + ( nTimerInterval < STOP_FORMAT_INTERVAL ? STOP_FORMAT_INTERVAL : nTimerInterval );

    aStopFormatStack.push_back( std::pair< SwFrm*, bool >( pFrm, bDisableTimer ) );
    if ( pFrm )
        aStopFormatStackMap[ pFrm ] = pFrm;

    return bRet;
}

bool PopFromStopFormatStack( bool bInvalidateSize, bool bInvalidatePrtArea )
{
    bool bRet = false;

    if ( aStopFormatStack.size() )
    {
        const std::pair< SwFrm*, bool > &rTop = aStopFormatStack.back();
        if ( rTop.second && nDisableStopFormatTimer )
        {
            nDisableStopFormatTimer--;
        }
        else if ( rTop.first && !nDisableStopFormatTimer )
        {
            bStopFormatInvalidateSize |= bInvalidateSize;
            bStopFormatInvalidatePrtArea |= bInvalidatePrtArea;
            aStopFormatInvalidateMap[ rTop.first ] = rTop.first;
        }

        bRet = ( bStopFormatInvalidateSize || bStopFormatInvalidatePrtArea );

        aStopFormatStack.pop_back();

        if ( !aStopFormatStack.size() )
        {
            if ( bStopFormatInvalidateSize || bStopFormatInvalidatePrtArea )
            {
                for ( std::map< SwFrm*, SwFrm* >::iterator it = aStopFormatInvalidateMap.begin(); it != aStopFormatInvalidateMap.end(); ++it )
                {
                    if ( it->first )
                    {
                        if ( bStopFormatInvalidateSize )
                            it->first->InvalidateSize();
                        if ( bStopFormatInvalidatePrtArea )
                            it->first->InvalidatePrt();
                    }
                }
            }

            aStopFormatStackMap.clear();
            bStopFormatInvalidateSize = false;
            bStopFormatInvalidatePrtArea = false;
            aStopFormatInvalidateMap.clear();
        }
    }

    return bRet;
}

void RemoveFromStopFormatInvalidateMap( SwFrm *pFrm )
{
    if ( pFrm )
    {
        std::map< SwFrm*, SwFrm* >::iterator it = aStopFormatStackMap.find( pFrm );
        if ( it != aStopFormatStackMap.end() )
        {
            aStopFormatStackMap.erase( it );
            for ( std::list< std::pair< SwFrm*, bool > >::iterator it = aStopFormatStack.begin(); it != aStopFormatStack.end(); ++it )
            {
                if ( it->first == pFrm )
                    it->first = NULL;
            }
        }

        it = aStopFormatInvalidateMap.find( pFrm );
        if ( it != aStopFormatInvalidateMap.end() )
            aStopFormatInvalidateMap.erase( it );
    }
}

#endif	// USE_JAVA

// Move methods

/// Return value tells whether the Frm should be moved.
bool SwCntntFrm::ShouldBwdMoved( SwLayoutFrm *pNewUpper, bool, bool & )
{
    if ( (SwFlowFrm::IsMoveBwdJump() || !IsPrevObjMove()))
    {
        // Floating back a frm uses a bit of time unfortunately.
        // The most common case is the following: The Frm wants to float to
        // somewhere where the FixSize is the same that the Frm itself has already.
        // In that case it's pretty easy to check if the Frm has enough space
        // for its VarSize. If this is NOT the case, we already know that
        // we don't need to move.
        // The Frm checks itself whether it has enough space - respecting the fact
        // that it could possibly split itself if needed.
        // If, however, the FixSize differs from the Frm or Flys are involved
        // (either in the old or the new position), checking is pointless,
        // and we have to move the Frm just to see what happens - if there's
        // some space available to do it, that is.

        // The FixSize of the containers of Cntnts is always the width.

        // If we moved more than one sheet back (for example jumping over empty
        // pages), we have to move either way. Otherwise, if the Frm doesn't fit
        // into the page, empty pages wouldn't be respected anymore.
        sal_uInt8 nMoveAnyway = 0;
        SwPageFrm * const pNewPage = pNewUpper->FindPageFrm();
        SwPageFrm *pOldPage = FindPageFrm();

        if ( SwFlowFrm::IsMoveBwdJump() )
            return true;

        if( IsInFtn() && IsInSct() )
        {
            SwFtnFrm* pFtn = FindFtnFrm();
            SwSectionFrm* pMySect = pFtn->FindSctFrm();
            if( pMySect && pMySect->IsFtnLock() )
            {
                SwSectionFrm *pSect = pNewUpper->FindSctFrm();
                while( pSect && pSect->IsInFtn() )
                    pSect = pSect->GetUpper()->FindSctFrm();
                OSL_ENSURE( pSect, "Escaping footnote" );
                if( pSect != pMySect )
                    return false;
            }
        }
        SWRECTFN( this )
        SWRECTFNX( pNewUpper )
        if( std::abs( (pNewUpper->Prt().*fnRectX->fnGetWidth)() -
                 (GetUpper()->Prt().*fnRect->fnGetWidth)() ) > 1 ) {
            // In this case, only a _WouldFit with test move is possible
            nMoveAnyway = 2;
        }

        // OD 2004-05-26 #i25904# - do *not* move backward,
        // if <nMoveAnyway> equals 3 and no space is left in new upper.
        nMoveAnyway |= BwdMoveNecessary( pOldPage, Frm() );
        {
            const IDocumentSettingAccess* pIDSA = pNewPage->GetFmt()->getIDocumentSettingAccess();
            SwTwips nSpace = 0;
            SwRect aRect( pNewUpper->Prt() );
            aRect.Pos() += pNewUpper->Frm().Pos();
            const SwFrm *pPrevFrm = pNewUpper->Lower();
            while ( pPrevFrm )
            {
                SwTwips nNewTop = (pPrevFrm->Frm().*fnRectX->fnGetBottom)();
                // OD 2004-03-01 #106629#:
                // consider lower spacing of last frame in a table cell
                {
                    // check, if last frame is inside table and if it includes
                    // its lower spacing.
                    if ( !pPrevFrm->GetNext() && pPrevFrm->IsInTab() &&
                         pIDSA->get(IDocumentSettingAccess::ADD_PARA_SPACING_TO_TABLE_CELLS) )
                    {
                        const SwFrm* pLastFrm = pPrevFrm;
                        // if last frame is a section, take its last content
                        if ( pPrevFrm->IsSctFrm() )
                        {
                            pLastFrm = static_cast<const SwSectionFrm*>(pPrevFrm)->FindLastCntnt();
                            if ( pLastFrm &&
                                 pLastFrm->FindTabFrm() != pPrevFrm->FindTabFrm() )
                            {
                                pLastFrm = pLastFrm->FindTabFrm();
                            }
                        }

                        if ( pLastFrm )
                        {
                            SwBorderAttrAccess aAccess( SwFrm::GetCache(), pLastFrm );
                            const SwBorderAttrs& rAttrs = *aAccess.Get();
                            nNewTop -= rAttrs.GetULSpace().GetLower();
                        }
                    }
                }
                (aRect.*fnRectX->fnSetTop)( nNewTop );

                pPrevFrm = pPrevFrm->GetNext();
            }

            nMoveAnyway |= BwdMoveNecessary( pNewPage, aRect);

            //determine space left in new upper frame
            nSpace = (aRect.*fnRectX->fnGetHeight)();
            const SwViewShell *pSh = pNewUpper->getRootFrm()->GetCurrShell();
            if ( IsInFtn() ||
                 (pSh && pSh->GetViewOptions()->getBrowseMode()) ||
                 pNewUpper->IsCellFrm() ||
                 ( pNewUpper->IsInSct() && ( pNewUpper->IsSctFrm() ||
                   ( pNewUpper->IsColBodyFrm() &&
                     !pNewUpper->GetUpper()->GetPrev() &&
                     !pNewUpper->GetUpper()->GetNext() ) ) ) )
                nSpace += pNewUpper->Grow( LONG_MAX, true );

            if ( nMoveAnyway < 3 )
            {
                if ( nSpace )
                {
                    // Do not notify footnotes which are stuck to the paragraph:
                    // This would require extremely confusing code, taking into
                    // account the widths
                    // and Flys, that in turn influence the footnotes, ...

                    // _WouldFit can only be used if the width is the same and
                    // ONLY self-anchored Flys are present.

                    // _WouldFit can also be used if ONLY Flys anchored
                    // somewhere else are present.
                    // In this case, the width doesn't even matter,
                    // because we're running a TestFormat in the new upper.
                    const sal_uInt8 nBwdMoveNecessaryResult =
                                            BwdMoveNecessary( pNewPage, aRect);
                    const bool bObjsInNewUpper( nBwdMoveNecessaryResult == 2 ||
                                                nBwdMoveNecessaryResult == 3 );

                    return _WouldFit( nSpace, pNewUpper, nMoveAnyway == 2,
                                      bObjsInNewUpper );
                }
                // It's impossible for _WouldFit to return a usable result if
                // we have a fresh multi-column section - so we really have to
                // float back unless there is no space.
                return pNewUpper->IsInSct() && pNewUpper->IsColBodyFrm() &&
                       !(pNewUpper->Prt().*fnRectX->fnGetWidth)() &&
                       ( pNewUpper->GetUpper()->GetPrev() ||
                         pNewUpper->GetUpper()->GetNext() );
            }

            // OD 2004-05-26 #i25904# - check for space left in new upper
            return nSpace != 0;
        }
    }
    return false;
}

// Calc methods

// Two little friendships form a secret society
inline void PrepareLock( SwFlowFrm *pTab )
{
    pTab->LockJoin();
}
inline void PrepareUnlock( SwFlowFrm *pTab )
{
    pTab->UnlockJoin();

}

// hopefully, one day this function simply will return 'false'
static bool lcl_IsCalcUpperAllowed( const SwFrm& rFrm )
{
    return !rFrm.GetUpper()->IsSctFrm() &&
           !rFrm.GetUpper()->IsFooterFrm() &&
           // #i23129#, #i36347# - no format of upper Writer fly frame
           !rFrm.GetUpper()->IsFlyFrm() &&
           !( rFrm.GetUpper()->IsTabFrm() && rFrm.GetUpper()->GetUpper()->IsInTab() ) &&
           !( rFrm.IsTabFrm() && rFrm.GetUpper()->IsInTab() );
}

/** Prepares the Frame for "formatting" (MakeAll()).
 *
 * This method serves to save stack space: To calculate the position of the Frm
 * we have to make sure that the positions of Upper and Prev respectively are
 * valid. This may require a recursive call (a loop would be quite expensive,
 * as it's not required very often).
 *
 * Every call of MakeAll requires around 500 bytes on the stack - you easily
 * see where this leads to. This method requires only a little bit of stack
 * space, so the recursive call should not be a problem here.
 *
 * Another advantage is that one nice day, this method and with it the
 * formatting of predecessors could be avoided. Then it could probably be
 * possible to jump "quickly" to the document's end.
 *
 * @see MakeAll()
 */
void SwFrm::PrepareMake()
{
    StackHack aHack;
    if ( GetUpper() )
    {
#ifndef NO_LIBO_BUG_119126_FIX
        SwFrmDeleteGuard aDeleteGuard(this);
#endif	// !NO_LIBO_BUG_119126_FIX
        if ( lcl_IsCalcUpperAllowed( *this ) )
            GetUpper()->Calc();
        OSL_ENSURE( GetUpper(), ":-( Layout unstable (Upper gone)." );
        if ( !GetUpper() )
            return;

        const bool bCnt = IsCntntFrm();
        const bool bTab = IsTabFrm();
        bool bNoSect = IsInSct();
        bool bOldTabLock = false, bFoll = false;
        SwFlowFrm* pThis = bCnt ? static_cast<SwCntntFrm*>(this) : NULL;

        if ( bTab )
        {
            pThis = static_cast<SwTabFrm*>(this);
            bOldTabLock = static_cast<SwTabFrm*>(this)->IsJoinLocked();
            ::PrepareLock( static_cast<SwTabFrm*>(this) );
            bFoll = pThis->IsFollow();
        }
        else if( IsSctFrm() )
        {
            pThis = static_cast<SwSectionFrm*>(this);
            bFoll = pThis->IsFollow();
            bNoSect = false;
        }
        else if ( bCnt && (bFoll = pThis->IsFollow()) && GetPrev() )
        {
            //Do not follow the chain when we need only one instance
            const SwTxtFrm* pMaster = static_cast<SwCntntFrm*>(this)->FindMaster();
            if ( pMaster && pMaster->IsLocked() )
            {
                MakeAll();
                return;
            }
        }

        // #i44049# - no format of previous frame, if current
        // frame is a table frame and its previous frame wants to keep with it.
        const bool bFormatPrev = !bTab ||
                                 !GetPrev() ||
                                 !GetPrev()->GetAttrSet()->GetKeep().GetValue();
        if ( bFormatPrev )
        {
            SwFrm *pFrm = GetUpper()->Lower();
            while ( pFrm != this )
            {
                OSL_ENSURE( pFrm, ":-( Layout unstable (this not found)." );
                if ( !pFrm )
                    return; //Oioioioi ...

                if ( !pFrm->IsValid() )
                {
                    // A small interference that hopefully improves on the stability:
                    // If I'm Follow AND neighbor of a Frm before me, it would delete
                    // me when formatting. This as you can see could easily become a
                    // confusing situation that we want to avoid.
                    if ( bFoll && pFrm->IsFlowFrm() &&
                         (SwFlowFrm::CastFlowFrm(pFrm))->IsAnFollow( pThis ) )
                        break;

                    pFrm->MakeAll();
                    if( IsSctFrm() && !static_cast<SwSectionFrm*>(this)->GetSection() )
                        break;
                }
                // With CntntFrms, the chain may be broken while walking through
                // it. Therefore we have to figure out the follower in a bit more
                // complicated way. However, I'll HAVE to get back to myself
                // sometime again.
                pFrm = pFrm->FindNext();

                // If we started out in a SectionFrm, it might have happened that
                // we landed in a Section Follow via the MakeAll calls.
                // FindNext only gives us the SectionFrm, not it's content - we
                // won't find ourselves anymore!
                if( bNoSect && pFrm && pFrm->IsSctFrm() )
                {
                    SwFrm* pCnt = static_cast<SwSectionFrm*>(pFrm)->ContainsAny();
                    if( pCnt )
                        pFrm = pCnt;
                }
            }
            OSL_ENSURE( GetUpper(), "Layout unstable (Upper gone II)." );
            if ( !GetUpper() )
                return;

            if ( lcl_IsCalcUpperAllowed( *this ) )
                GetUpper()->Calc();

            OSL_ENSURE( GetUpper(), "Layout unstable (Upper gone III)." );
        }

        if ( bTab && !bOldTabLock )
            ::PrepareUnlock( static_cast<SwTabFrm*>(this) );
    }
    MakeAll();
}

void SwFrm::OptPrepareMake()
{
    // #i23129#, #i36347# - no format of upper Writer fly frame
    if ( GetUpper() && !GetUpper()->IsFooterFrm() &&
         !GetUpper()->IsFlyFrm() )
    {
#ifdef NO_LIBO_BUG_91695_FIX
        GetUpper()->Calc();
#else	// NO_LIBO_BUG_91695_FIX
        {
            SwFrmDeleteGuard aDeleteGuard(this);
            GetUpper()->Calc();
        }
#endif	// NO_LIBO_BUG_91695_FIX
        OSL_ENSURE( GetUpper(), ":-( Layout unstable (Upper gone)." );
        if ( !GetUpper() )
            return;
    }
    if ( GetPrev() && !GetPrev()->IsValid() )
        PrepareMake();
    else
    {
        StackHack aHack;
        MakeAll();
    }
}

void SwFrm::PrepareCrsr()
{
    StackHack aHack;
    if( GetUpper() && !GetUpper()->IsSctFrm() )
    {
        GetUpper()->PrepareCrsr();
        GetUpper()->Calc();

        OSL_ENSURE( GetUpper(), ":-( Layout unstable (Upper gone)." );
        if ( !GetUpper() )
            return;

        const bool bCnt = IsCntntFrm();
        const bool bTab = IsTabFrm();
        bool bNoSect = IsInSct();

        bool bOldTabLock = false, bFoll;
        SwFlowFrm* pThis = bCnt ? static_cast<SwCntntFrm*>(this) : NULL;

        if ( bTab )
        {
            bOldTabLock = static_cast<SwTabFrm*>(this)->IsJoinLocked();
            ::PrepareLock( static_cast<SwTabFrm*>(this) );
            pThis = static_cast<SwTabFrm*>(this);
        }
        else if( IsSctFrm() )
        {
            pThis = static_cast<SwSectionFrm*>(this);
            bNoSect = false;
        }
        bFoll = pThis && pThis->IsFollow();

        SwFrm *pFrm = GetUpper()->Lower();
        while ( pFrm != this )
        {
            OSL_ENSURE( pFrm, ":-( Layout unstable (this not found)." );
            if ( !pFrm )
                return; //Oioioioi ...

            if ( !pFrm->IsValid() )
            {
                // A small interference that hopefully improves on the stability:
                // If I'm Follow AND neighbor of a Frm before me, it would delete
                // me when formatting. This as you can see could easily become a
                // confusing situation that we want to avoid.
                if ( bFoll && pFrm->IsFlowFrm() &&
                     (SwFlowFrm::CastFlowFrm(pFrm))->IsAnFollow( pThis ) )
                    break;

                pFrm->MakeAll();
            }
            // With CntntFrms, the chain may be broken while walking through
            // it. Therefore we have to figure out the follower in a bit more
            // complicated way. However, I'll HAVE to get back to myself
            // sometime again.
            pFrm = pFrm->FindNext();
            if( bNoSect && pFrm && pFrm->IsSctFrm() )
            {
                SwFrm* pCnt = static_cast<SwSectionFrm*>(pFrm)->ContainsAny();
                if( pCnt )
                    pFrm = pCnt;
            }
        }
        OSL_ENSURE( GetUpper(), "Layout unstable (Upper gone II)." );
        if ( !GetUpper() )
            return;

        GetUpper()->Calc();

        OSL_ENSURE( GetUpper(), "Layout unstable (Upper gone III)." );

        if ( bTab && !bOldTabLock )
            ::PrepareUnlock( static_cast<SwTabFrm*>(this) );
    }
    Calc();
}

// Here we return GetPrev(); however we will ignore empty SectionFrms
static SwFrm* lcl_Prev( SwFrm* pFrm, bool bSectPrv = true )
{
    SwFrm* pRet = pFrm->GetPrev();
    if( !pRet && pFrm->GetUpper() && pFrm->GetUpper()->IsSctFrm() &&
        bSectPrv && !pFrm->IsColumnFrm() )
        pRet = pFrm->GetUpper()->GetPrev();
    while( pRet && pRet->IsSctFrm() &&
           !static_cast<SwSectionFrm*>(pRet)->GetSection() )
        pRet = pRet->GetPrev();
    return pRet;
}

static SwFrm* lcl_NotHiddenPrev( SwFrm* pFrm )
{
    SwFrm *pRet = pFrm;
    do
    {
        pRet = lcl_Prev( pRet );
    } while ( pRet && pRet->IsTxtFrm() && static_cast<SwTxtFrm*>(pRet)->IsHiddenNow() );
    return pRet;
}

void SwFrm::MakePos()
{
    if ( !mbValidPos )
    {
        mbValidPos = true;
        bool bUseUpper = false;
        SwFrm* pPrv = lcl_Prev( this );
        if ( pPrv &&
             ( !pPrv->IsCntntFrm() ||
               ( static_cast<SwCntntFrm*>(pPrv)->GetFollow() != this ) )
           )
        {
            if ( !StackHack::IsLocked() &&
                 ( !IsInSct() || IsSctFrm() ) &&
                 !pPrv->IsSctFrm() &&
                 !pPrv->GetAttrSet()->GetKeep().GetValue()
               )
            {
                pPrv->Calc();   // This may cause Prev to vanish!
            }
            else if ( pPrv->Frm().Top() == 0 )
            {
                bUseUpper = true;
            }
        }

        pPrv = lcl_Prev( this, false );
        const sal_uInt16 nMyType = GetType();
        SWRECTFN( ( IsCellFrm() && GetUpper() ? GetUpper() : this  ) )
        if ( !bUseUpper && pPrv )
        {
            maFrm.Pos( pPrv->Frm().Pos() );
            if( FRM_NEIGHBOUR & nMyType )
            {
                bool bR2L = IsRightToLeft();
                if( bR2L )
                    (maFrm.*fnRect->fnSetPosX)( (maFrm.*fnRect->fnGetLeft)() -
                                               (maFrm.*fnRect->fnGetWidth)() );
                else
                    (maFrm.*fnRect->fnSetPosX)( (maFrm.*fnRect->fnGetLeft)() +
                                          (pPrv->Frm().*fnRect->fnGetWidth)() );

                // cells may now leave their uppers
                if( bVert && FRM_CELL & nMyType && !mbReverse )
                    maFrm.Pos().setX(maFrm.Pos().getX() - maFrm.Width() + pPrv->Frm().Width());
            }
            else if( bVert && FRM_NOTE_VERT & nMyType )
            {
                if( mbReverse )
                    maFrm.Pos().setX(maFrm.Pos().getX() + pPrv->Frm().Width());
                else
                {
                    if ( bVertL2R )
                           maFrm.Pos().setX(maFrm.Pos().getX() + pPrv->Frm().Width());
                    else
                           maFrm.Pos().setX(maFrm.Pos().getX() - maFrm.Width());
                  }
            }
            else
                maFrm.Pos().setY(maFrm.Pos().getY() + pPrv->Frm().Height());
        }
        else if ( GetUpper() )
        {
            // OD 15.10.2002 #103517# - add safeguard for <SwFooterFrm::Calc()>
            // If parent frame is a footer frame and its <ColLocked()>, then
            // do *not* calculate it.
            // NOTE: Footer frame is <ColLocked()> during its
            //     <FormatSize(..)>, which is called from <Format(..)>, which
            //     is called from <MakeAll()>, which is called from <Calc()>.
            // #i56850#
            // - no format of upper Writer fly frame, which is anchored
            //   at-paragraph or at-character.
            if ( !GetUpper()->IsTabFrm() &&
                 !( IsTabFrm() && GetUpper()->IsInTab() ) &&
                 !GetUpper()->IsSctFrm() &&
                 !dynamic_cast<SwFlyAtCntFrm*>(GetUpper()) &&
                 !( GetUpper()->IsFooterFrm() &&
                    GetUpper()->IsColLocked() )
               )
            {
                GetUpper()->Calc();
            }
            pPrv = lcl_Prev( this, false );
            if ( !bUseUpper && pPrv )
            {
                maFrm.Pos( pPrv->Frm().Pos() );
                if( FRM_NEIGHBOUR & nMyType )
                {
                    bool bR2L = IsRightToLeft();
                    if( bR2L )
                        (maFrm.*fnRect->fnSetPosX)( (maFrm.*fnRect->fnGetLeft)() -
                                                 (maFrm.*fnRect->fnGetWidth)() );
                    else
                        (maFrm.*fnRect->fnSetPosX)( (maFrm.*fnRect->fnGetLeft)() +
                                          (pPrv->Frm().*fnRect->fnGetWidth)() );

                    // cells may now leave their uppers
                    if( bVert && FRM_CELL & nMyType && !mbReverse )
                        maFrm.Pos().setX(maFrm.Pos().getX() - maFrm.Width() + pPrv->Frm().Width());
                }
                else if( bVert && FRM_NOTE_VERT & nMyType )
                {
                    if( mbReverse )
                        maFrm.Pos().setX(maFrm.Pos().getX() + pPrv->Frm().Width());
                    else
                        maFrm.Pos().setX(maFrm.Pos().getX() - maFrm.Width());
                }
                else
                    maFrm.Pos().setY(maFrm.Pos().getY() + pPrv->Frm().Height());
            }
            else
            {
                maFrm.Pos( GetUpper()->Frm().Pos() );
                if( GetUpper()->IsFlyFrm() )
                    maFrm.Pos() += static_cast<SwFlyFrm*>(GetUpper())->ContentPos();
                else
                    maFrm.Pos() += GetUpper()->Prt().Pos();

                if( FRM_NEIGHBOUR & nMyType && IsRightToLeft() )
                {
                    if( bVert )
                        maFrm.Pos().setY(maFrm.Pos().getY() + GetUpper()->Prt().Height()
                                          - maFrm.Height());
                    else
                        maFrm.Pos().setX(maFrm.Pos().getX() + GetUpper()->Prt().Width()
                                          - maFrm.Width());
                }
                else if( bVert && !bVertL2R && FRM_NOTE_VERT & nMyType && !mbReverse )
                    maFrm.Pos().setX(maFrm.Pos().getX() - maFrm.Width() + GetUpper()->Prt().Width());
            }
        }
        else
        {
            maFrm.Pos().setX(0);
            maFrm.Pos().setY(0);
        }

        if( IsBodyFrm() && bVert && !bVertL2R && !mbReverse && GetUpper() )
            maFrm.Pos().setX(maFrm.Pos().getX() + GetUpper()->Prt().Width() - maFrm.Width());
        mbValidPos = true;
    }
}

// #i28701# - new type <SwSortedObjs>
static void lcl_CheckObjects( SwSortedObjs* pSortedObjs, SwFrm* pFrm, long& rBot )
{
    // And then there can be paragraph anchored frames that sit below their paragraph.
    long nMax = 0;
    for ( size_t i = 0; i < pSortedObjs->size(); ++i )
    {
        // #i28701# - consider changed type of <SwSortedObjs>
        // entries.
        SwAnchoredObject* pObj = (*pSortedObjs)[i];
        long nTmp = 0;
        if ( pObj->ISA(SwFlyFrm) )
        {
            SwFlyFrm *pFly = static_cast<SwFlyFrm*>(pObj);
            if( pFly->Frm().Top() != FAR_AWAY &&
                ( pFrm->IsPageFrm() ? pFly->IsFlyLayFrm() :
                  ( pFly->IsFlyAtCntFrm() &&
                    ( pFrm->IsBodyFrm() ? pFly->GetAnchorFrm()->IsInDocBody() :
                                          pFly->GetAnchorFrm()->IsInFtn() ) ) ) )
            {
                nTmp = pFly->Frm().Bottom();
            }
        }
        else
            nTmp = pObj->GetObjRect().Bottom();
        nMax = std::max( nTmp, nMax );
    }
    ++nMax; // Lower edge vs. height!
    rBot = std::max( rBot, nMax );
}

void SwPageFrm::MakeAll()
{
    PROTOCOL_ENTER( this, PROT_MAKEALL, 0, 0 )

    const SwRect aOldRect( Frm() );     // Adjust root size
    const SwLayNotify aNotify( this );  // takes care of the notification in the dtor
    SwBorderAttrAccess *pAccess = 0;
    const SwBorderAttrs*pAttrs = 0;

    while ( !mbValidPos || !mbValidSize || !mbValidPrtArea )
    {
        if ( !mbValidPos )
        {
            mbValidPos = true; // positioning of the pages is taken care of by the root frame
        }

        if ( !mbValidSize || !mbValidPrtArea )
        {
            if ( IsEmptyPage() )
            {
                Frm().Width( 0 );  Prt().Width( 0 );
                Frm().Height( 0 ); Prt().Height( 0 );
                Prt().Left( 0 );   Prt().Top( 0 );
                mbValidSize = mbValidPrtArea = true;
            }
            else
            {
                if ( !pAccess )
                {
                    pAccess = new SwBorderAttrAccess( SwFrm::GetCache(), this );
                    pAttrs = pAccess->Get();
                }
                // In BrowseView, we use fixed settings
                SwViewShell *pSh = getRootFrm()->GetCurrShell();
                if ( pSh && pSh->GetViewOptions()->getBrowseMode() )
                {
                    const Size aBorder = pSh->GetOut()->PixelToLogic( pSh->GetBrowseBorder() );
                    const long nTop    = pAttrs->CalcTopLine()   + aBorder.Height();
                    const long nBottom = pAttrs->CalcBottomLine()+ aBorder.Height();

                    long nWidth = GetUpper() ? static_cast<SwRootFrm*>(GetUpper())->GetBrowseWidth() : 0;
                    if ( nWidth < pSh->GetBrowseWidth() )
                        nWidth = pSh->GetBrowseWidth();
                    nWidth += + 2 * aBorder.Width();

                    nWidth = std::max( nWidth, 2L * aBorder.Width() + 4L*MM50 );
                    Frm().Width( nWidth );

                    SwLayoutFrm *pBody = FindBodyCont();
                    if ( pBody && pBody->Lower() && pBody->Lower()->IsColumnFrm() )
                    {
                        // Columns have a fixed height
                        Frm().Height( pAttrs->GetSize().Height() );
                    }
                    else
                    {
                        // In pages without columns, the content defines the size.
                        long nBot = Frm().Top() + nTop;
                        SwFrm *pFrm = Lower();
                        while ( pFrm )
                        {
                            long nTmp = 0;
                            SwFrm *pCnt = static_cast<SwLayoutFrm*>(pFrm)->ContainsAny();
                            while ( pCnt && (pCnt->GetUpper() == pFrm ||
                                             static_cast<SwLayoutFrm*>(pFrm)->IsAnLower( pCnt )))
                            {
                                nTmp += pCnt->Frm().Height();
                                if( pCnt->IsTxtFrm() &&
                                    static_cast<SwTxtFrm*>(pCnt)->IsUndersized() )
                                    nTmp += static_cast<SwTxtFrm*>(pCnt)->GetParHeight()
                                            - pCnt->Prt().Height();
                                else if( pCnt->IsSctFrm() &&
                                         static_cast<SwSectionFrm*>(pCnt)->IsUndersized() )
                                    nTmp += static_cast<SwSectionFrm*>(pCnt)->Undersize();
                                pCnt = pCnt->FindNext();
                            }
                            // OD 29.10.2002 #97265# - consider invalid body frame properties
                            if ( pFrm->IsBodyFrm() &&
                                 ( !pFrm->GetValidSizeFlag() ||
                                   !pFrm->GetValidPrtAreaFlag() ) &&
                                 ( pFrm->Frm().Height() < pFrm->Prt().Height() )
                               )
                            {
                                nTmp = std::min( nTmp, pFrm->Frm().Height() );
                            }
                            else
                            {
                                // OD 30.10.2002 #97265# - assert invalid lower property
                                OSL_ENSURE( !(pFrm->Frm().Height() < pFrm->Prt().Height()),
                                        "SwPageFrm::MakeAll(): Lower with frame height < printing height" );
                                nTmp += pFrm->Frm().Height() - pFrm->Prt().Height();
                            }
                            if ( !pFrm->IsBodyFrm() )
                                nTmp = std::min( nTmp, pFrm->Frm().Height() );
                            nBot += nTmp;
                            // Here we check whether paragraph anchored objects
                            // protrude outside the Body/FtnCont.
                            if( pSortedObjs && !pFrm->IsHeaderFrm() &&
                                !pFrm->IsFooterFrm() )
                                lcl_CheckObjects( pSortedObjs, pFrm, nBot );
                            pFrm = pFrm->GetNext();
                        }
                        nBot += nBottom;
                        // And the page anchored ones
                        if ( pSortedObjs )
                            lcl_CheckObjects( pSortedObjs, this, nBot );
                        nBot -= Frm().Top();
                        // #i35143# - If second page frame
                        // exists, the first page doesn't have to fulfill the
                        // visible area.
                        if ( !GetPrev() && !GetNext() )
                        {
                            nBot = std::max( nBot, pSh->VisArea().Height() );
                        }
                        // #i35143# - Assure, that the page
                        // doesn't exceed the defined browse height.
                        Frm().Height( std::min( nBot, BROWSE_HEIGHT ) );
                    }
                    Prt().Left ( pAttrs->CalcLeftLine() + aBorder.Width() );
                    Prt().Top  ( nTop );
                    Prt().Width( Frm().Width() - ( Prt().Left()
                        + pAttrs->CalcRightLine() + aBorder.Width() ) );
                    Prt().Height( Frm().Height() - (nTop + nBottom) );
                    mbValidSize = mbValidPrtArea = true;
                }
                else
                {   // Set FixSize. For pages, this is not done from Upper, but from
                    // the attribute.
                    Frm().SSize( pAttrs->GetSize() );
                    Format( pAttrs );
                }
            }
        }
    } //while ( !mbValidPos || !mbValidSize || !mbValidPrtArea )
    delete pAccess;

    if ( Frm() != aOldRect && GetUpper() )
        static_cast<SwRootFrm*>(GetUpper())->CheckViewLayout( 0, 0 );

    OSL_ENSURE( !GetUpper() || GetUpper()->Prt().Width() >= maFrm.Width(),
        "Upper (Root) must be wide enough to contain the widest page");
}

void SwLayoutFrm::MakeAll()
{
    PROTOCOL_ENTER( this, PROT_MAKEALL, 0, 0 )

    // takes care of the notification in the dtor
    const SwLayNotify aNotify( this );
    bool bVert = IsVertical();

    SwRectFn fnRect = ( IsNeighbourFrm() == bVert )? fnRectHori : ( IsVertLR() ? fnRectVertL2R : fnRectVert );

    SwBorderAttrAccess *pAccess = 0;
    const SwBorderAttrs*pAttrs = 0;

    while ( !mbValidPos || !mbValidSize || !mbValidPrtArea )
    {
        if ( !mbValidPos )
            MakePos();

        if ( GetUpper() )
        {
            // NEW TABLES
            if ( IsLeaveUpperAllowed() )
            {
                if ( !mbValidSize )
                    mbValidPrtArea = false;
            }
            else
            {
                if ( !mbValidSize )
                {
                    // Set FixSize; VarSize is set by Format() after calculating the PrtArea
                    mbValidPrtArea = false;

                    SwTwips nPrtWidth = (GetUpper()->Prt().*fnRect->fnGetWidth)();
                    if( bVert && ( IsBodyFrm() || IsFtnContFrm() ) )
                    {
                        SwFrm* pNxt = GetPrev();
                        while( pNxt && !pNxt->IsHeaderFrm() )
                            pNxt = pNxt->GetPrev();
                        if( pNxt )
                            nPrtWidth -= pNxt->Frm().Height();
                        pNxt = GetNext();
                        while( pNxt && !pNxt->IsFooterFrm() )
                            pNxt = pNxt->GetNext();
                        if( pNxt )
                            nPrtWidth -= pNxt->Frm().Height();
                    }

                    const long nDiff = nPrtWidth - (Frm().*fnRect->fnGetWidth)();

                    if( IsNeighbourFrm() && IsRightToLeft() )
                        (Frm().*fnRect->fnSubLeft)( nDiff );
                    else
                        (Frm().*fnRect->fnAddRight)( nDiff );
                }
                else
                {
                    // Don't leave your upper
                    const SwTwips nDeadLine = (GetUpper()->*fnRect->fnGetPrtBottom)();
                    if( (Frm().*fnRect->fnOverStep)( nDeadLine ) )
                        mbValidSize = false;
                }
            }
        }
        if ( !mbValidSize || !mbValidPrtArea )
        {
            if ( !pAccess )
            {
                pAccess = new SwBorderAttrAccess( SwFrm::GetCache(), this );
                pAttrs  = pAccess->Get();
            }
            Format( pAttrs );
        }
    } //while ( !mbValidPos || !mbValidSize || !mbValidPrtArea )
    delete pAccess;
}

bool SwTxtNode::IsCollapse() const
{
    if (GetDoc()->GetDocumentSettingManager().get( IDocumentSettingAccess::COLLAPSE_EMPTY_CELL_PARA )
        &&  GetTxt().isEmpty())
    {
        sal_uLong nIdx=GetIndex();
        const SwEndNode *pNdBefore=GetNodes()[nIdx-1]->GetEndNode();
        const SwEndNode *pNdAfter=GetNodes()[nIdx+1]->GetEndNode();

        // The paragraph is collapsed only if the NdAfter is the end of a cell
        bool bInTable = this->FindTableNode( ) != NULL;

        SwSortedObjs* pObjs = this->getLayoutFrm( GetDoc()->getIDocumentLayoutAccess().GetCurrentLayout() )->GetDrawObjs( );
        const size_t nObjs = ( pObjs != NULL ) ? pObjs->size( ) : 0;

        return pNdBefore!=NULL && pNdAfter!=NULL && nObjs == 0 && bInTable;
    }

    return false;
}

bool SwFrm::IsCollapse() const
{
    if (!IsTxtFrm())
        return false;

    const SwTxtFrm *pTxtFrm = static_cast<const SwTxtFrm*>(this);
    const SwTxtNode *pTxtNode = pTxtFrm->GetTxtNode();
    return pTxtNode && pTxtNode->IsCollapse();
}

bool SwCntntFrm::MakePrtArea( const SwBorderAttrs &rAttrs )
{
    bool bSizeChgd = false;

    if ( !mbValidPrtArea )
    {
        mbValidPrtArea = true;

        SWRECTFN( this )
        const bool bTxtFrm = IsTxtFrm();
        SwTwips nUpper = 0;
        if ( bTxtFrm && static_cast<SwTxtFrm*>(this)->IsHiddenNow() )
        {
            if ( static_cast<SwTxtFrm*>(this)->HasFollow() )
                static_cast<SwTxtFrm*>(this)->JoinFrm();

            if( (Prt().*fnRect->fnGetHeight)() )
                static_cast<SwTxtFrm*>(this)->HideHidden();
            Prt().Pos().setX(0);
            Prt().Pos().setY(0);
            (Prt().*fnRect->fnSetWidth)( (Frm().*fnRect->fnGetWidth)() );
            (Prt().*fnRect->fnSetHeight)( 0 );
            nUpper = -( (Frm().*fnRect->fnGetHeight)() );
        }
        else
        {
            // Simplification: CntntFrms are always variable in height!

            // At the FixSize, the surrounding Frame enforces the size;
            // the borders are simply subtracted.
            const long nLeft = rAttrs.CalcLeft( this );
            const long nRight = ((SwBorderAttrs&)rAttrs).CalcRight( this );
            (this->*fnRect->fnSetXMargins)( nLeft, nRight );

            SwViewShell *pSh = getRootFrm()->GetCurrShell();
            SwTwips nWidthArea;
            if( pSh && 0!=(nWidthArea=(pSh->VisArea().*fnRect->fnGetWidth)()) &&
                GetUpper()->IsPageBodyFrm() && // but not for BodyFrms in Columns
                pSh->GetViewOptions()->getBrowseMode() )
            {
                // Do not protrude the edge of the visible area. The page may be
                // wider, because there may be objects with excess width
                // (RootFrm::ImplCalcBrowseWidth())
                long nMinWidth = 0;

                for (size_t i = 0; GetDrawObjs() && i < GetDrawObjs()->size(); ++i)
                {
                    // #i28701# - consider changed type of
                    // <SwSortedObjs> entries
                    SwAnchoredObject* pObj = (*GetDrawObjs())[i];
                    const SwFrmFmt& rFmt = pObj->GetFrmFmt();
                    const bool bFly = pObj->ISA(SwFlyFrm);
                    if ((bFly && (FAR_AWAY == pObj->GetObjRect().Width()))
                        || rFmt.GetFrmSize().GetWidthPercent())
                    {
                        continue;
                    }

                    if ( FLY_AS_CHAR == rFmt.GetAnchor().GetAnchorId() )
                    {
                        nMinWidth = std::max( nMinWidth,
                                         bFly ? rFmt.GetFrmSize().GetWidth()
                                              : pObj->GetObjRect().Width() );
                    }
                }

                const Size aBorder = pSh->GetOut()->PixelToLogic( pSh->GetBrowseBorder() );
                long nWidth = nWidthArea - 2 * ( IsVertical() ? aBorder.Height() : aBorder.Width() );
                nWidth -= (Prt().*fnRect->fnGetLeft)();
                nWidth -= rAttrs.CalcRightLine();
                nWidth = std::max( nMinWidth, nWidth );
                (Prt().*fnRect->fnSetWidth)( std::min( nWidth,
                                            (Prt().*fnRect->fnGetWidth)() ) );
            }

            if ( (Prt().*fnRect->fnGetWidth)() <= MINLAY )
            {
                // The PrtArea should already be at least MINLAY wide, matching the
                // minimal values of the UI
                (Prt().*fnRect->fnSetWidth)( std::min( long(MINLAY),
                                             (Frm().*fnRect->fnGetWidth)() ) );
                SwTwips nTmp = (Frm().*fnRect->fnGetWidth)() -
                               (Prt().*fnRect->fnGetWidth)();
                if( (Prt().*fnRect->fnGetLeft)() > nTmp )
                    (Prt().*fnRect->fnSetLeft)( nTmp );
            }

            // The following rules apply for VarSize:
            // 1. The first entry of a chain has no top border
            // 2. There is never a bottom border
            // 3. The top border is the maximum of the distance
            //    of Prev downwards and our own distance upwards
            // Those three rules apply when calculating spacings
            // that are given by UL- and LRSpace. There might be a spacing
            // in all directions however; this may be caused by borders
            // and / or shadows.
            // 4. The spacing for TextFrms corresponds to the interline lead,
            //    at a minimum.

            nUpper = CalcUpperSpace( &rAttrs, NULL );

            SwTwips nLower = CalcLowerSpace( &rAttrs );
            if (IsCollapse()) {
                nUpper=0;
                nLower=0;
            }

            (Prt().*fnRect->fnSetPosY)( (!bVert || mbReverse) ? nUpper : nLower);
            nUpper += nLower;
            nUpper -= (Frm().*fnRect->fnGetHeight)() -
                      (Prt().*fnRect->fnGetHeight)();
        }
        // If there's a difference between old and new size, call Grow() or
        // Shrink() respectively.
        if ( nUpper )
        {
            if ( nUpper > 0 )
                GrowFrm( nUpper );
            else
                ShrinkFrm( -nUpper );
            bSizeChgd = true;
        }
    }
    return bSizeChgd;
}

#define STOP_FLY_FORMAT 10
// - loop prevention
const int cnStopFormat = 15;

inline void ValidateSz( SwFrm *pFrm )
{
    if ( pFrm )
    {
        pFrm->mbValidSize = true;
        pFrm->mbValidPrtArea = true;
    }
}

void SwCntntFrm::MakeAll()
{
    OSL_ENSURE( GetUpper(), "no Upper?" );
    OSL_ENSURE( IsTxtFrm(), "MakeAll(), NoTxt" );

    if ( !IsFollow() && StackHack::IsLocked() )
        return;

    if ( IsJoinLocked() )
        return;

    OSL_ENSURE( !static_cast<SwTxtFrm*>(this)->IsSwapped(), "Calculation of a swapped frame" );

    StackHack aHack;

    if ( static_cast<SwTxtFrm*>(this)->IsLocked() )
    {
        OSL_FAIL( "Format for locked TxtFrm." );
        return;
    }

#ifdef USE_JAVA
    // Fix excessively long loop times that occur when pasting huge
    // amounts of data into a table cell by applying OOo's "stop
    // formatting" loop control in this object and its children after
    // STOP_FORMAT_INTERVAL has passed
    bool bOldValidSize = mbValidSize;
    bool bOldValidPrtArea = mbValidPrtArea;
    bool bStopFormat = PushToStopFormatStack( this );
#endif	// USE_JAVA

#ifndef NO_LIBO_BUG_95974_FIX
    bool const bDeleteForbidden(IsDeleteForbidden());
    ForbidDelete();
#endif	// !NO_LIBO_BUG_95974_FIX
    LockJoin();
    long nFormatCount = 0;
    // - loop prevention
    int nConsequetiveFormatsWithoutChange = 0;
    PROTOCOL_ENTER( this, PROT_MAKEALL, 0, 0 )

#ifdef DBG_UTIL
    const SwDoc *pDoc = GetAttrSet()->GetDoc();
    if( pDoc )
    {
        static bool bWarned = false;
        if( pDoc->InXMLExport() )
        {
            SAL_WARN_IF( !bWarned, "sw", "Formatting during XML-export!" );
            bWarned = true;
        }
        else
            bWarned = false;
    }
#endif

    // takes care of the notification in the dtor
    SwCntntNotify *pNotify = new SwCntntNotify( this );

    // as long as bMakePage is true, a new page can be created (exactly once)
    bool bMakePage = true;
    // bMovedBwd gets set to true when the frame flows backwards
    bool bMovedBwd = false;
    // as long as bMovedFwd is false, the Frm may flow backwards (until
    // it has been moved forward once)
    bool bMovedFwd  = false;
    sal_Bool    bFormatted  = sal_False;    // For the widow/orphan rules, we encourage the
                                            // last CntntFrm of a chain to format. This only
                                            // needs to happen once. Every time the Frm is
                                            // moved, the flag will have to be reset.
    bool bMustFit = false;                  // Once the emergency brake is pulled,
                                            // no other prepares will be triggered
    bool bFitPromise = false;               // If a paragraph didn't fit, but promises
                                            // with WouldFit that it would adjust accordingly,
                                            // this flag is set. If it turns out that it
                                            // didn't keep it's promise, we can act in a
                                            // controlled fashion.
    bool bMoveable;
    const bool bFly = IsInFly();
    const bool bTab = IsInTab();
    const bool bFtn = IsInFtn();
    const bool bSct = IsInSct();
    Point aOldFrmPos;               // This is so we can compare with the last pos
    Point aOldPrtPos;               // and determine whether it makes sense to Prepare

    SwBorderAttrAccess aAccess( SwFrm::GetCache(), this );
    const SwBorderAttrs &rAttrs = *aAccess.Get();

    // OD 2004-02-26 #i25029#
    if ( !IsFollow() && rAttrs.JoinedWithPrev( *(this) ) )
    {
        pNotify->SetBordersJoinedWithPrev();
    }

    const bool bKeep = IsKeep( rAttrs.GetAttrSet() );

    SwSaveFtnHeight *pSaveFtn = 0;
    if ( bFtn )
    {
        SwFtnFrm *pFtn = FindFtnFrm();
        SwSectionFrm* pSct = pFtn->FindSctFrm();
        if ( !static_cast<SwTxtFrm*>(pFtn->GetRef())->IsLocked() )
        {
            SwFtnBossFrm* pBoss = pFtn->GetRef()->FindFtnBossFrm(
                                    pFtn->GetAttr()->GetFtn().IsEndNote() );
            if( !pSct || pSct->IsColLocked() || !pSct->Growable() )
                pSaveFtn = new SwSaveFtnHeight( pBoss,
                    static_cast<SwTxtFrm*>(pFtn->GetRef())->GetFtnLine( pFtn->GetAttr() ) );
        }
    }

    if ( GetUpper()->IsSctFrm() &&
         HasFollow() &&
         &GetFollow()->GetFrm() == GetNext() )
    {
        dynamic_cast<SwTxtFrm&>(*this).JoinFrm();
    }

    // #i28701# - move master forward, if it has to move,
    // because of its object positioning.
    if ( !static_cast<SwTxtFrm*>(this)->IsFollow() )
    {
        sal_uInt32 nToPageNum = 0L;
        const bool bMoveFwdByObjPos = SwLayouter::FrmMovedFwdByObjPos(
                                                    *(GetAttrSet()->GetDoc()),
                                                    *(static_cast<SwTxtFrm*>(this)),
                                                    nToPageNum );
        // #i58182#
        // Also move a paragraph forward, which is the first one inside a table cell.
        if ( bMoveFwdByObjPos &&
             FindPageFrm()->GetPhyPageNum() < nToPageNum &&
             ( lcl_Prev( this ) ||
               GetUpper()->IsCellFrm() ||
               ( GetUpper()->IsSctFrm() &&
                 GetUpper()->GetUpper()->IsCellFrm() ) ) &&
             IsMoveable() )
        {
            bMovedFwd = true;
            MoveFwd( bMakePage, false );
        }
    }

    // If a Follow sits next to it's Master and doesn't fit, we know it can
    // be moved right now.
    if ( lcl_Prev( this ) && static_cast<SwTxtFrm*>(this)->IsFollow() && IsMoveable() )
    {
        bMovedFwd = true;
        // OD 2004-03-02 #106629# - If follow frame is in table, it's master
        // will be the last in the current table cell. Thus, invalidate the
        // printing area of the master,
        if ( IsInTab() )
        {
            lcl_Prev( this )->InvalidatePrt();
        }
        MoveFwd( bMakePage, false );
    }

    // OD 08.11.2002 #104840# - check footnote content for forward move.
    // If a content of a footnote is on a prior page/column as its invalid
    // reference, it can be moved forward.
    if ( bFtn && !mbValidPos )
    {
        SwFtnFrm* pFtn = FindFtnFrm();
        SwCntntFrm* pRefCnt = pFtn ? pFtn->GetRef() : 0;
        if ( pRefCnt && !pRefCnt->IsValid() )
        {
            SwFtnBossFrm* pFtnBossOfFtn = pFtn->FindFtnBossFrm();
            SwFtnBossFrm* pFtnBossOfRef = pRefCnt->FindFtnBossFrm();
            //<loop of movefwd until condition held or no move>
            if ( pFtnBossOfFtn && pFtnBossOfRef &&
                 pFtnBossOfFtn != pFtnBossOfRef &&
                 pFtnBossOfFtn->IsBefore( pFtnBossOfRef ) )
            {
                bMovedFwd = true;
                MoveFwd( bMakePage, false );
            }
        }
    }

    SWRECTFN( this )

    while ( !mbValidPos || !mbValidSize || !mbValidPrtArea )
    {
        // - loop prevention
        SwRect aOldFrm_StopFormat( Frm() );
        SwRect aOldPrt_StopFormat( Prt() );
        if ( (bMoveable = IsMoveable()) )
        {
            SwFrm *pPre = GetIndPrev();
            if ( CheckMoveFwd( bMakePage, bKeep, bMovedBwd ) )
            {
                SWREFRESHFN( this )
                bMovedFwd = true;
                if ( bMovedBwd )
                {
                    // While flowing back, the Upper was encouraged to
                    // completely re-paint itself. We can skip this now after
                    // flowing back and forth.
                    GetUpper()->ResetCompletePaint();
                    // The predecessor was invalidated, so this is obsolete as well now.
                    OSL_ENSURE( pPre, "missing old Prev" );
                    if( !pPre->IsSctFrm() )
                        ::ValidateSz( pPre );
                }
                bMoveable = IsMoveable();
            }
        }

        aOldFrmPos = (Frm().*fnRect->fnGetPos)();
        aOldPrtPos = (Prt().*fnRect->fnGetPos)();

        if ( !mbValidPos )
            MakePos();

        //Set FixSize. VarSize is being adjusted by Format().
        if ( !mbValidSize )
        {
            // #125452#
            // invalidate printing area flag, if the following conditions are hold:
            // - current frame width is 0.
            // - current printing area width is 0.
            // - frame width is adjusted to a value greater than 0.
            // - printing area flag is true.
            // Thus, it's assured that the printing area is adjusted, if the
            // frame area width changes its width from 0 to something greater
            // than 0.
            // Note: A text frame can be in such a situation, if the format is
            //       triggered by method call <SwCrsrShell::SetCrsr()> after
            //       loading the document.
            const SwTwips nNewFrmWidth = (GetUpper()->Prt().*fnRect->fnGetWidth)();
            if ( mbValidPrtArea && nNewFrmWidth > 0 &&
                 (Frm().*fnRect->fnGetWidth)() == 0 &&
                 (Prt().*fnRect->fnGetWidth)() == 0 )
            {
                mbValidPrtArea = false;
            }

            (Frm().*fnRect->fnSetWidth)( nNewFrmWidth );

            // When a lower of a vertically aligned fly frame changes its size we need to recalculate content pos.
            if( GetUpper() && GetUpper()->IsFlyFrm() &&
                GetUpper()->GetFmt()->GetTextVertAdjust().GetValue() != SDRTEXTVERTADJUST_TOP )
            {
                static_cast<SwFlyFrm*>(GetUpper())->InvalidateContentPos();
                GetUpper()->SetCompletePaint();
            }
        }
        if ( !mbValidPrtArea )
        {
            const long nOldW = (Prt().*fnRect->fnGetWidth)();
            // #i34730# - keep current frame height
            const SwTwips nOldH = (Frm().*fnRect->fnGetHeight)();
            MakePrtArea( rAttrs );
            if ( nOldW != (Prt().*fnRect->fnGetWidth)() )
                Prepare( PREP_FIXSIZE_CHG );
            // #i34730# - check, if frame height has changed.
            // If yes, send a PREP_ADJUST_FRM and invalidate the size flag to
            // force a format. The format will check in its method
            // <SwTxtFrm::CalcPreps()>, if the already formatted lines still
            // fit and if not, performs necessary actions.
            // #i40150# - no check, if frame is undersized.
            if ( mbValidSize && !IsUndersized() &&
                 nOldH != (Frm().*fnRect->fnGetHeight)() )
            {
                // #115759# - no PREP_ADJUST_FRM and size
                // invalidation, if height decreases only by the additional
                // lower space as last content of a table cell and an existing
                // follow containing one line exists.
                const SwTwips nHDiff = nOldH - (Frm().*fnRect->fnGetHeight)();
                const bool bNoPrepAdjustFrm =
                    nHDiff > 0 && IsInTab() && GetFollow() &&
                    ( 1 == static_cast<SwTxtFrm*>(GetFollow())->GetLineCount( COMPLETE_STRING ) || (static_cast<SwTxtFrm*>(GetFollow())->Frm().*fnRect->fnGetWidth)() < 0 ) &&
                    GetFollow()->CalcAddLowerSpaceAsLastInTableCell() == nHDiff;
                if ( !bNoPrepAdjustFrm )
                {
                    Prepare( PREP_ADJUST_FRM );
                    mbValidSize = false;
                }
            }
        }

        // To make the widow and orphan rules work, we need to notify the CntntFrm.
        // Criteria:
        // - It needs to be movable (otherwise, splitting doesn't make sense)
        // - It needs to overlap with the lower edge of the PrtArea of the Upper
        if ( !bMustFit )
        {
            bool bWidow = true;
            const SwTwips nDeadLine = (GetUpper()->*fnRect->fnGetPrtBottom)();
            if ( bMoveable && !bFormatted && ( GetFollow() ||
                 ( (Frm().*fnRect->fnOverStep)( nDeadLine ) ) ) )
            {
                Prepare( PREP_WIDOWS_ORPHANS, 0, false );
                mbValidSize = bWidow = false;
            }
            if( (Frm().*fnRect->fnGetPos)() != aOldFrmPos ||
                (Prt().*fnRect->fnGetPos)() != aOldPrtPos )
            {
                // In this Prepare, an _InvalidateSize() might happen.
                // mbValidSize becomes false and Format() gets called.
                Prepare( PREP_POS_CHGD, (const void*)&bFormatted, false );
                if ( bWidow && GetFollow() )
                {
                    Prepare( PREP_WIDOWS_ORPHANS, 0, false );
                    mbValidSize = false;
                }
            }
        }
        if ( !mbValidSize )
        {
            mbValidSize = true;
            bFormatted = sal_True;
            ++nFormatCount;
            if( nFormatCount > STOP_FLY_FORMAT )
                SetFlyLock( true );
            // - loop prevention
            // No format any longer, if <cnStopFormat> consequetive formats
            // without change occur.
#ifdef USE_JAVA
            if ( !bStopFormat && nConsequetiveFormatsWithoutChange <= cnStopFormat )
#else	// USE_JAVA
            if ( nConsequetiveFormatsWithoutChange <= cnStopFormat )
#endif	// USE_JAVA
            {
                Format();
            }
#if OSL_DEBUG_LEVEL > 0
            else
            {
                OSL_FAIL( "debug assertion: <SwCntntFrm::MakeAll()> - format of text frame suppressed by fix b6448963" );
            }
#endif
        }

        // If this is the first one in a chain, check if this can flow
        // backwards (if this is movable at all).
        // To prevent oscillations/loops, check that this has not just
        // flowed forwards.
        bool bDummy;
        if ( !lcl_Prev( this ) &&
             !bMovedFwd &&
             ( bMoveable || ( bFly && !bTab ) ) &&
             ( !bFtn || !GetUpper()->FindFtnFrm()->GetPrev() )
             && MoveBwd( bDummy ) )
        {
            SWREFRESHFN( this )
            bMovedBwd = true;
            bFormatted = sal_False;
            if ( bKeep && bMoveable )
            {
                if( CheckMoveFwd( bMakePage, false, bMovedBwd ) )
                {
                    bMovedFwd = true;
                    bMoveable = IsMoveable();
                    SWREFRESHFN( this )
                }
                Point aOldPos = (Frm().*fnRect->fnGetPos)();
                MakePos();
                if( aOldPos != (Frm().*fnRect->fnGetPos)() )
                {
                    Prepare( PREP_POS_CHGD, (const void*)&bFormatted, false );
                    if ( !mbValidSize )
                    {
                        (Frm().*fnRect->fnSetWidth)( (GetUpper()->
                                                Prt().*fnRect->fnGetWidth)() );
                        if ( !mbValidPrtArea )
                        {
                            const long nOldW = (Prt().*fnRect->fnGetWidth)();
                            MakePrtArea( rAttrs );
                            if( nOldW != (Prt().*fnRect->fnGetWidth)() )
                                Prepare( PREP_FIXSIZE_CHG, 0, false );
                        }
                        if( GetFollow() )
                            Prepare( PREP_WIDOWS_ORPHANS, 0, false );
                        mbValidSize = true;
                        bFormatted = sal_True;
                        Format();
                    }
                }
                SwFrm *pNxt = HasFollow() ? NULL : FindNext();
                while( pNxt && pNxt->IsSctFrm() )
                {   // Leave empty sections out, go into the other ones.
                    if( static_cast<SwSectionFrm*>(pNxt)->GetSection() )
                    {
                        SwFrm* pTmp = static_cast<SwSectionFrm*>(pNxt)->ContainsAny();
                        if( pTmp )
                        {
                            pNxt = pTmp;
                            break;
                        }
                    }
                    pNxt = pNxt->FindNext();
                }
                if ( pNxt )
                {
                    pNxt->Calc();
                    if( mbValidPos && !GetIndNext() )
                    {
                        SwSectionFrm *pSct = FindSctFrm();
                        if( pSct && !pSct->GetValidSizeFlag() )
                        {
                            SwSectionFrm* pNxtSct = pNxt->FindSctFrm();
                            if( pNxtSct && pSct->IsAnFollow( pNxtSct ) )
                                mbValidPos = false;
                        }
                        else
                            mbValidPos = false;
                    }
                }
            }
        }

        // In footnotes, the TxtFrm may validate itself, which can lead to the
        // situation that it's position is wrong despite being "valid".
        if ( mbValidPos )
        {
            // #i59341#
            // Workaround for inadequate layout algorithm:
            // suppress invalidation and calculation of position, if paragraph
            // has formatted itself at least STOP_FLY_FORMAT times and
            // has anchored objects.
            // Thus, the anchored objects get the possibility to format itself
            // and this probably solve the layout loop.
            if ( bFtn &&
                 nFormatCount <= STOP_FLY_FORMAT &&
                 !GetDrawObjs() )
            {
                mbValidPos = false;
                MakePos();
                aOldFrmPos = (Frm().*fnRect->fnGetPos)();
                aOldPrtPos = (Prt().*fnRect->fnGetPos)();
            }
        }

        // - loop prevention
        {
            if ( aOldFrm_StopFormat == Frm() &&
                 aOldPrt_StopFormat == Prt() )
            {
                ++nConsequetiveFormatsWithoutChange;
            }
            else
            {
                nConsequetiveFormatsWithoutChange = 0;
            }
        }

        // Yet again an invalid value? Repeat from the start...
        if ( !mbValidPos || !mbValidSize || !mbValidPrtArea )
            continue;

        // Done?
        // Attention: because height == 0, it's better to use Top()+Height() instead of
        // Bottom(). This might happen with undersized TextFrms on the lower edge of a
        // multi-column section
        const long nPrtBottom = (GetUpper()->*fnRect->fnGetPrtBottom)();
        const long nBottomDist =  (Frm().*fnRect->fnBottomDist)( nPrtBottom );
        if( nBottomDist >= 0 )
        {
            if ( bKeep && bMoveable )
            {
                // We make sure the successor will be formatted the same.
                // This way, we keep control until (almost) everything is stable,
                // allowing us to avoid endless loops caused by ever repeating
                // retries.

                // bMoveFwdInvalid is required for #38407#. This was originally solved
                // in flowfrm.cxx rev 1.38, but broke the above schema and
                // preferred to play towers of hanoi (#43669#).
                SwFrm *pNxt = HasFollow() ? NULL : FindNext();
                // For sections we prefer the content, because it can change
                // the page if required.
                while( pNxt && pNxt->IsSctFrm() )
                {
                    if( static_cast<SwSectionFrm*>(pNxt)->GetSection() )
                    {
                        pNxt = static_cast<SwSectionFrm*>(pNxt)->ContainsAny();
                        break;
                    }
                    pNxt = pNxt->FindNext();
                }
                if ( pNxt )
                {
                    const bool bMoveFwdInvalid = 0 != GetIndNext();
                    const bool bNxtNew =
                        ( 0 == (pNxt->Prt().*fnRect->fnGetHeight)() ) &&
                        (!pNxt->IsTxtFrm() ||!static_cast<SwTxtFrm*>(pNxt)->IsHiddenNow());

                    pNxt->Calc();

                    if ( !bMovedBwd &&
                         ((bMoveFwdInvalid && !GetIndNext()) ||
                          bNxtNew) )
                    {
                        if( bMovedFwd )
                            pNotify->SetInvaKeep();
                        bMovedFwd = false;
                    }
                }
            }
            continue;
        }

        // I don't fit into my parents, so it's time to make changes
        // as constructively as possible.

        //If I'm NOT allowed to leave the parent Frm, I've got a problem.
        // Following Arthur Dent, we do the only thing that you can do with
        // an unsolvable problem: We ignore it with all our power.
        if ( !bMoveable || IsUndersized() )
        {
            if( !bMoveable && IsInTab() )
            {
                long nDiff = -(Frm().*fnRect->fnBottomDist)(
                                        (GetUpper()->*fnRect->fnGetPrtBottom)() );
                long nReal = GetUpper()->Grow( nDiff );
                if( nReal )
                    continue;
            }
            break;
        }

        // If there's no way I can make myself fit into my Upper, the situation
        // could still probably be mitigated by splitting up.
        // This situation arises with freshly created Follows that had been moved
        // to the next page but is still too big for it - ie. needs to be split
        // as well.

        // If I'm unable to split (WouldFit()) and can't be fitted, I'm going
        // to tell my TxtFrm part that, if possible, we still need to split despite
        // the "don't split" attribute.
        bool bMoveOrFit = false;
        bool bDontMoveMe = !GetIndPrev();
        if( bDontMoveMe && IsInSct() )
        {
            SwFtnBossFrm* pBoss = FindFtnBossFrm();
            bDontMoveMe = !pBoss->IsInSct() ||
                          ( !pBoss->Lower()->GetNext() && !pBoss->GetPrev() );
        }

        // Finally, we are able to split table rows. Therefore, bDontMoveMe
        // can be set to false:
        if( bDontMoveMe && IsInTab() &&
            0 != const_cast<SwCntntFrm*>(this)->GetNextCellLeaf( MAKEPAGE_NONE ) )
            bDontMoveMe = false;

        if ( bDontMoveMe && (Frm().*fnRect->fnGetHeight)() >
                            (GetUpper()->Prt().*fnRect->fnGetHeight)() )
        {
            if ( !bFitPromise )
            {
                SwTwips nTmp = (GetUpper()->Prt().*fnRect->fnGetHeight)() -
                               (Prt().*fnRect->fnGetTop)();
                bool bSplit = !IsFwdMoveAllowed();
                if ( nTmp > 0 && WouldFit( nTmp, bSplit, false ) )
                {
                    Prepare( PREP_WIDOWS_ORPHANS, 0, false );
                    mbValidSize = false;
                    bFitPromise = true;
                    continue;
                }
                /*
                 * In earlier days, we never tried to fit TextFrms in
                 * frames and sections using bMoveOrFit by ignoring
                 * its attributes (Widows, Keep).
                 * This should have been done at least for column frames;
                 * as it must be tried anyway with linked frames and sections.
                 * Exception: If we sit in FormatWidthCols, we must not ignore
                 * the attributes.
                 */
                else if ( !bFtn && bMoveable &&
                      ( !bFly || !FindFlyFrm()->IsColLocked() ) &&
                      ( !bSct || !FindSctFrm()->IsColLocked() ) )
                    bMoveOrFit = true;
            }
#if OSL_DEBUG_LEVEL > 0
            else
            {
                OSL_FAIL( "+TxtFrm didn't respect WouldFit promise." );
            }
#endif
        }

        // Let's see if I can find some space somewhere...
        // footnotes in the neighbourhood are moved into _MoveFtnCntFwd
        SwFrm *pPre = GetIndPrev();
        SwFrm *pOldUp = GetUpper();

/* MA 13. Oct. 98: What is this supposed to be!?
 * AMA 14. Dec 98: If a column section can't find any space for its first ContentFrm, it should be
 *                 moved not only to the next column, but probably even to the next page, creating
 *                 a section-follow there.
 */
        if( IsInSct() && bMovedFwd && bMakePage && pOldUp->IsColBodyFrm() &&
            pOldUp->GetUpper()->GetUpper()->IsSctFrm() &&
            ( pPre || pOldUp->GetUpper()->GetPrev() ) &&
            static_cast<SwSectionFrm*>(pOldUp->GetUpper()->GetUpper())->MoveAllowed(this) )
        {
            bMovedFwd = false;
        }

        const bool bCheckForGrownBody = pOldUp->IsBodyFrm();
        const long nOldBodyHeight = (pOldUp->Frm().*fnRect->fnGetHeight)();

        if ( !bMovedFwd && !MoveFwd( bMakePage, false ) )
            bMakePage = false;
        SWREFRESHFN( this )
#ifndef NO_LIBO_BUG_101821_FIX
        if (!bMovedFwd && bFtn && GetIndPrev() != pPre)
        {   // SwFlowFrame::CutTree() could have formatted and deleted pPre
            auto const pPrevFtnFrm(static_cast<SwFtnFrm const*>(GetUpper())->GetMaster());
            bool bReset = true;
            if (pPrevFtnFrm)
            {   // use GetIndNext() in case there are sections
                for (auto p = pPrevFtnFrm->Lower(); p; p = p->GetIndNext())
                {
                    if (p == pPre)
                    {
                        bReset = false;
                        break;
                    }
                }
            }
            if (bReset)
            {
                pPre = nullptr;
            }
        }
#endif	// !NO_LIBO_BUG_101821_FIX

        // If MoveFwd moves the paragraph to the next page, a following
        // paragraph, which contains footnotes can cause the old upper
        // frame to grow. In this case we explicitly allow a new check
        // for MoveBwd. Robust: We also check the bMovedBwd flag again.
        // If pOldUp was a footnote frame, it has been deleted inside MoveFwd.
        // Therefore we only check for growing body frames.
        if ( bCheckForGrownBody && ! bMovedBwd && pOldUp != GetUpper() &&
             (pOldUp->Frm().*fnRect->fnGetHeight)() > nOldBodyHeight )
        {
            bMovedFwd = false;
        }
        else
        {
            bMovedFwd = true;
        }

        bFormatted = sal_False;
        if ( bMoveOrFit && GetUpper() == pOldUp )
        {
            // FME 2007-08-30 #i81146# new loop control
#ifdef USE_JAVA
            if ( !bStopFormat && nConsequetiveFormatsWithoutChange <= cnStopFormat )
#else	// USE_JAVA
            if ( nConsequetiveFormatsWithoutChange <= cnStopFormat )
#endif	// USE_JAVA
            {
                Prepare( PREP_MUST_FIT, 0, false );
                mbValidSize = false;
                bMustFit = true;
                continue;
            }

#if OSL_DEBUG_LEVEL > 0
            OSL_FAIL( "LoopControl in SwCntntFrm::MakeAll" );
#endif
        }
        if ( bMovedBwd && GetUpper() )
        {   // Retire invalidations that have become useless.
            GetUpper()->ResetCompletePaint();
            if( pPre && !pPre->IsSctFrm() )
                ::ValidateSz( pPre );
        }

    } //while ( !mbValidPos || !mbValidSize || !mbValidPrtArea )

    // NEW: Looping Louie (Light). Should not be applied in balanced sections.
    // Should only be applied if there is no better solution!
    LOOPING_LOUIE_LIGHT( bMovedFwd && bMovedBwd && !IsInBalancedSection() &&
                            (

                                // #118572#
                                ( bFtn && !FindFtnFrm()->GetRef()->IsInSct() ) ||

                                // #i33887#
                                ( IsInSct() && bKeep )

                                // ... add your conditions here ...

                            ),
                         static_cast<SwTxtFrm&>(*this) );

    delete pSaveFtn;

    UnlockJoin();
#ifndef NO_LIBO_BUG_95974_FIX
    if (!bDeleteForbidden)
        AllowDelete();
#endif	// !NO_LIBO_BUG_95974_FIX
    if ( bMovedFwd || bMovedBwd )
        pNotify->SetInvaKeep();
    // OD 2004-02-26 #i25029#
    if ( bMovedFwd )
    {
        pNotify->SetInvalidatePrevPrtArea();
    }
    delete pNotify;
    SetFlyLock( false );

#ifdef USE_JAVA
    if ( bStopFormat )
        PopFromStopFormatStack( !bOldValidSize, !bOldValidPrtArea );
    else
        PopFromStopFormatStack();
#endif	// USE_JAVA
}

void MakeNxt( SwFrm *pFrm, SwFrm *pNxt )
{
    // fix(25455): Validate, otherwise this leads to a recursion.
    // The first try, cancelling with pFrm = 0 if !Valid, leads to a problem, as
    // the Keep may not be considered properly anymore (27417).
    const bool bOldPos = pFrm->GetValidPosFlag();
    const bool bOldSz  = pFrm->GetValidSizeFlag();
    const bool bOldPrt = pFrm->GetValidPrtAreaFlag();
    pFrm->mbValidPos = pFrm->mbValidPrtArea = pFrm->mbValidSize = true;

    // fix(29272): Don't call MakeAll - there, pFrm might be invalidated again, and
    // we recursively end up in here again.
    if ( pNxt->IsCntntFrm() )
    {
        SwCntntNotify aNotify( static_cast<SwCntntFrm*>(pNxt) );
        SwBorderAttrAccess aAccess( SwFrm::GetCache(), pNxt );
        const SwBorderAttrs &rAttrs = *aAccess.Get();
        if ( !pNxt->GetValidSizeFlag() )
        {
            if( pNxt->IsVertical() )
                pNxt->Frm().Height( pNxt->GetUpper()->Prt().Height() );
            else
                pNxt->Frm().Width( pNxt->GetUpper()->Prt().Width() );
        }
        static_cast<SwCntntFrm*>(pNxt)->MakePrtArea( rAttrs );
        pNxt->Format( &rAttrs );
    }
    else
    {
        SwLayNotify aNotify( static_cast<SwLayoutFrm*>(pNxt) );
        SwBorderAttrAccess aAccess( SwFrm::GetCache(), pNxt );
        const SwBorderAttrs &rAttrs = *aAccess.Get();
        if ( !pNxt->GetValidSizeFlag() )
        {
            if( pNxt->IsVertical() )
                pNxt->Frm().Height( pNxt->GetUpper()->Prt().Height() );
            else
                pNxt->Frm().Width( pNxt->GetUpper()->Prt().Width() );
        }
        pNxt->Format( &rAttrs );
    }

    pFrm->mbValidPos      = bOldPos;
    pFrm->mbValidSize     = bOldSz;
    pFrm->mbValidPrtArea  = bOldPrt;
}

/// This routine checks whether there are no other FtnBosses
/// between the pFrm's FtnBoss and the pNxt's FtnBoss.
static bool lcl_IsNextFtnBoss( const SwFrm *pFrm, const SwFrm* pNxt )
{
    assert(pFrm && pNxt && "lcl_IsNextFtnBoss: No Frames?");
    pFrm = pFrm->FindFtnBossFrm();
    pNxt = pNxt->FindFtnBossFrm();
    // If pFrm is a last column, we use the page instead.
    while( pFrm && pFrm->IsColumnFrm() && !pFrm->GetNext() )
        pFrm = pFrm->GetUpper()->FindFtnBossFrm();
    // If pNxt is a first column, we use the page instead.
    while( pNxt && pNxt->IsColumnFrm() && !pNxt->GetPrev() )
        pNxt = pNxt->GetUpper()->FindFtnBossFrm();
    // So.. now pFrm and pNxt are either two adjacent pages or columns.
    return pFrm && pNxt && pFrm->GetNext() == pNxt;
}

bool SwCntntFrm::_WouldFit( SwTwips nSpace,
                            SwLayoutFrm *pNewUpper,
                            bool bTstMove,
                            const bool bObjsInNewUpper )
{
    // To have the footnote select it's place carefully, it needs
    // to be moved in any case if there is at least one page/column
    // between the footnote and the new Upper.
    SwFtnFrm* pFtnFrm = 0;
    if ( IsInFtn() )
    {
        if( !lcl_IsNextFtnBoss( pNewUpper, this ) )
            return true;
        pFtnFrm = FindFtnFrm();
    }

    bool bRet;
    bool bSplit = !pNewUpper->Lower();
    SwCntntFrm *pFrm = this;
    const SwFrm *pTmpPrev = pNewUpper->Lower();
    if( pTmpPrev && pTmpPrev->IsFtnFrm() )
        pTmpPrev = static_cast<const SwFtnFrm*>(pTmpPrev)->Lower();
    while ( pTmpPrev && pTmpPrev->GetNext() )
        pTmpPrev = pTmpPrev->GetNext();
    do
    {
        // #i46181#
        SwTwips nSecondCheck = 0;
        SwTwips nOldSpace = nSpace;
        bool bOldSplit = bSplit;

        if ( bTstMove || IsInFly() || ( IsInSct() &&
             ( pFrm->GetUpper()->IsColBodyFrm() || ( pFtnFrm &&
               pFtnFrm->GetUpper()->GetUpper()->IsColumnFrm() ) ) ) )
        {
            // This is going to get a bit insidious now. If you're faint of heart,
            // you'd better look away here. If a Fly contains columns, then the Cntnts
            // are movable, except ones in the last column (see SwFrm::IsMoveable()).
            // Of course they're allowed to float back. WouldFit() only returns a usable
            // value if the Frm is movable. To fool WouldFit() into believing there's
            // a movable Frm, I'm just going to hang it somewhere else for the time.
            // The same procedure applies for column sections to make SwSectionFrm::Growable()
            // return the proper value.
            // Within footnotes, we may even need to put the SwFtnFrm somewhere else, if
            // there's no SwFtnFrm there.
            SwFrm* pTmpFrm = pFrm->IsInFtn() && !pNewUpper->FindFtnFrm() ?
                             (SwFrm*)pFrm->FindFtnFrm() : pFrm;
            SwLayoutFrm *pUp = pTmpFrm->GetUpper();
            SwFrm *pOldNext = pTmpFrm->GetNext();
            pTmpFrm->Remove();
            pTmpFrm->InsertBefore( pNewUpper, 0 );
#ifndef NO_LIBO_BUG_107126_FIX
            // tdf#107126 for a section in a footnote, we have only inserted
            // the SwTextFrame but no SwSectionFrame - reset mbInfSct flag
            // to avoid crashing (but perhaps we should create a temp
            // SwSectionFrame here because WidowsAndOrphans checks for that?)
            pTmpFrm->InvalidateInfFlags();
#endif	// !NO_LIBO_BUG_107126_FIX
            if ( pFrm->IsTxtFrm() &&
                 ( bTstMove ||
                   static_cast<SwTxtFrm*>(pFrm)->HasFollow() ||
                   ( !static_cast<SwTxtFrm*>(pFrm)->HasPara() &&
                     !static_cast<SwTxtFrm*>(pFrm)->IsEmpty()
                   )
                 )
               )
            {
                bTstMove = true;
                bRet = static_cast<SwTxtFrm*>(pFrm)->TestFormat( pTmpPrev, nSpace, bSplit );
            }
            else
                bRet = pFrm->WouldFit( nSpace, bSplit, false );

            pTmpFrm->Remove();
            pTmpFrm->InsertBefore( pUp, pOldNext );
#ifndef NO_LIBO_BUG_107126_FIX
            pTmpFrm->InvalidateInfFlags(); // restore flags
#endif	// !NO_LIBO_BUG_107126_FIX
        }
        else
        {
            bRet = pFrm->WouldFit( nSpace, bSplit, false );
            nSecondCheck = !bSplit ? 1 : 0;
        }

        SwBorderAttrAccess aAccess( SwFrm::GetCache(), pFrm );
        const SwBorderAttrs &rAttrs = *aAccess.Get();

        // Sad but true: We need to consider the spacing in our calculation.
        // This already happened in TestFormat.
        if ( bRet && !bTstMove )
        {
            SwTwips nUpper;

            if ( pTmpPrev )
            {
                nUpper = CalcUpperSpace( NULL, pTmpPrev );

                // in balanced columned section frames we do not want the
                // common border
                bool bCommonBorder = true;
                if ( pFrm->IsInSct() && pFrm->GetUpper()->IsColBodyFrm() )
                {
                    const SwSectionFrm* pSct = pFrm->FindSctFrm();
                    bCommonBorder = pSct->GetFmt()->GetBalancedColumns().GetValue();
                }

                // #i46181#
                nSecondCheck = ( 1 == nSecondCheck &&
                                 pFrm == this &&
                                 IsTxtFrm() &&
                                 bCommonBorder &&
                                 !static_cast<const SwTxtFrm*>(this)->IsEmpty() ) ?
                                 nUpper :
                                 0;

                nUpper += bCommonBorder ?
                          rAttrs.GetBottomLine( *(pFrm) ) :
                          rAttrs.CalcBottomLine();

            }
            else
            {
                // #i46181#
                nSecondCheck = 0;

                if( pFrm->IsVertical() )
                    nUpper = pFrm->Frm().Width() - pFrm->Prt().Width();
                else
                    nUpper = pFrm->Frm().Height() - pFrm->Prt().Height();
            }

            nSpace -= nUpper;

            if ( nSpace < 0 )
            {
                bRet = false;

                // #i46181#
                if ( nSecondCheck > 0 )
                {
                    // The following code is intended to solve a (rare) problem
                    // causing some frames not to move backward:
                    // SwTxtFrm::WouldFit() claims that the whole paragraph
                    // fits into the given space and subtracts the height of
                    // all lines from nSpace. nSpace - nUpper is not a valid
                    // indicator if the frame should be allowed to move backward.
                    // We do a second check with the original remaining space
                    // reduced by the required upper space:
                    nOldSpace -= nSecondCheck;
                    const bool bSecondRet = nOldSpace >= 0 && pFrm->WouldFit( nOldSpace, bOldSplit, false );
                    if ( bSecondRet && bOldSplit && nOldSpace >= 0 )
                    {
                        bRet = true;
                        bSplit = true;
                    }
                }
            }
        }

        // OD 2004-03-01 #106629# - also consider lower spacing in table cells
        if ( bRet && IsInTab() &&
             pNewUpper->GetFmt()->getIDocumentSettingAccess()->get(IDocumentSettingAccess::ADD_PARA_SPACING_TO_TABLE_CELLS) )
        {
            nSpace -= rAttrs.GetULSpace().GetLower();
            if ( nSpace < 0 )
            {
                bRet = false;
            }
        }

        if ( bRet && !bSplit && pFrm->IsKeep( rAttrs.GetAttrSet() ) )
        {
            if( bTstMove )
            {
                while( pFrm->IsTxtFrm() && static_cast<SwTxtFrm*>(pFrm)->HasFollow() )
                {
                    pFrm = static_cast<SwTxtFrm*>(pFrm)->GetFollow();
                }
                // OD 11.04.2003 #108824# - If last follow frame of <this> text
                // frame isn't valid, a formatting of the next content frame
                // doesn't makes sense. Thus, return true.
                if ( IsAnFollow( pFrm ) && !pFrm->IsValid() )
                {
                    OSL_FAIL( "Only a warning for task 108824:/n<SwCntntFrm::_WouldFit(..) - follow not valid!" );
                    return true;
                }
            }
            SwFrm *pNxt;
            if( 0 != (pNxt = pFrm->FindNext()) && pNxt->IsCntntFrm() &&
                ( !pFtnFrm || ( pNxt->IsInFtn() &&
                  pNxt->FindFtnFrm()->GetAttr() == pFtnFrm->GetAttr() ) ) )
            {
                // TestFormat(?) does not like paragraph- or character anchored objects.

                // current solution for the test formatting doesn't work, if
                // objects are present in the remaining area of the new upper
                if ( bTstMove &&
                     ( pNxt->GetDrawObjs() || bObjsInNewUpper ) )
                {
                    return true;
                }

                if ( !pNxt->IsValid() )
                    MakeNxt( pFrm, pNxt );

                // Little trick: if the next has a predecessor, then the paragraph
                // spacing has been calculated already, and we don't need to re-calculate
                // it in an expensive way.
                if( lcl_NotHiddenPrev( pNxt ) )
                    pTmpPrev = 0;
                else
                {
                    if( pFrm->IsTxtFrm() && static_cast<SwTxtFrm*>(pFrm)->IsHiddenNow() )
                        pTmpPrev = lcl_NotHiddenPrev( pFrm );
                    else
                        pTmpPrev = pFrm;
                }
                pFrm = static_cast<SwCntntFrm*>(pNxt);
            }
            else
                pFrm = 0;
        }
        else
            pFrm = 0;

    } while ( bRet && pFrm );

    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
