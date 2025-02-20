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

#ifndef INCLUDED_SW_SOURCE_CORE_INC_FTNFRM_HXX
#define INCLUDED_SW_SOURCE_CORE_INC_FTNFRM_HXX

#include "layfrm.hxx"

class SwCntntFrm;
class SwTxtFtn;
class SwBorderAttrs;
class SwFtnFrm;

void sw_RemoveFtns( SwFtnBossFrm* pBoss, bool bPageOnly, bool bEndNotes );

// There exists a special section on a page for footnotes. It's called
// SwFtnContFrm. Each footnote is separated by a SwFtnFrm which contains
// the paragraphs of a footnote. SwFtnFrm can be splitted and will then
// continue on another page.
class SwFtnContFrm: public SwLayoutFrm
{
public:
    SwFtnContFrm( SwFrmFmt*, SwFrm* );

    const SwFtnFrm* FindFootNote() const;

    virtual SwTwips ShrinkFrm( SwTwips, bool bTst = false, bool bInfo = false ) SAL_OVERRIDE;
    virtual SwTwips GrowFrm  ( SwTwips, bool bTst = false, bool bInfo = false ) SAL_OVERRIDE;
    virtual void    Format( const SwBorderAttrs *pAttrs = 0 ) SAL_OVERRIDE;
    virtual void    PaintBorder( const SwRect &, const SwPageFrm *pPage,
                                 const SwBorderAttrs & ) const SAL_OVERRIDE;
    virtual void PaintSubsidiaryLines( const SwPageFrm*, const SwRect& ) const SAL_OVERRIDE;
            void    PaintLine( const SwRect &, const SwPageFrm * ) const;
};

class SwFtnFrm: public SwLayoutFrm
{
    // Pointer to FtnFrm in which the footnote will be continued:
    //  - 0     no following existent
    //  - this  for the last one
    //  - otherwise the following FtnFrm
    SwFtnFrm     *pFollow;
    SwFtnFrm     *pMaster;      // FtnFrm from which I am the following
    SwCntntFrm   *pRef;         // in this CntntFrm is the footnote reference
    SwTxtFtn     *pAttr;        // footnote attribute (for recognition)

    // if true paragraphs in this footnote are NOT permitted to flow backwards
    bool bBackMoveLocked : 1;
    // #i49383# - control unlock of position of lower anchored objects.
    bool mbUnlockPosOfLowerObjs : 1;
#ifdef DBG_UTIL
protected:
    virtual SwTwips ShrinkFrm( SwTwips, bool bTst = false, bool bInfo = false ) SAL_OVERRIDE;
    virtual SwTwips GrowFrm  ( SwTwips, bool bTst = false, bool bInfo = false ) SAL_OVERRIDE;
#endif

public:
    SwFtnFrm( SwFrmFmt*, SwFrm*, SwCntntFrm*, SwTxtFtn* );

#ifndef NO_LIBO_BUG_119126_FIX
    virtual bool IsDeleteForbidden() const override;
#endif	// !NO_LIBO_BUG_119126_FIX
    virtual void Cut() SAL_OVERRIDE;
    virtual void Paste( SwFrm* pParent, SwFrm* pSibling = 0 ) SAL_OVERRIDE;

    virtual void PaintSubsidiaryLines( const SwPageFrm*, const SwRect& ) const SAL_OVERRIDE;

    bool operator<( const SwTxtFtn* pTxtFtn ) const;

#ifdef DBG_UTIL
    const SwCntntFrm *GetRef() const;
         SwCntntFrm  *GetRef();
#else
    const SwCntntFrm *GetRef() const    { return pRef; }
         SwCntntFrm  *GetRef()          { return pRef; }
#endif
    const SwCntntFrm *GetRefFromAttr()  const;
          SwCntntFrm *GetRefFromAttr();

    const SwFtnFrm *GetFollow() const   { return pFollow; }
          SwFtnFrm *GetFollow()         { return pFollow; }

    const SwFtnFrm *GetMaster() const   { return pMaster; }
          SwFtnFrm *GetMaster()         { return pMaster; }

    const SwTxtFtn   *GetAttr() const   { return pAttr; }
          SwTxtFtn   *GetAttr()         { return pAttr; }

    void SetFollow( SwFtnFrm *pNew ) { pFollow = pNew; }
    void SetMaster( SwFtnFrm *pNew ) { pMaster = pNew; }
    void SetRef   ( SwCntntFrm *pNew ) { pRef = pNew; }

    void InvalidateNxtFtnCnts( SwPageFrm* pPage );

    void LockBackMove()     { bBackMoveLocked = true; }
    void UnlockBackMove()   { bBackMoveLocked = false;}
    bool IsBackMoveLocked() { return bBackMoveLocked; }

    // prevents that the last content deletes the SwFtnFrm as well (Cut())
    inline void ColLock()       { mbColLocked = true; }
    inline void ColUnlock()     { mbColLocked = false; }

    // #i49383#
    inline void UnlockPosOfLowerObjs()
    {
        mbUnlockPosOfLowerObjs = true;
    }
    inline void KeepLockPosOfLowerObjs()
    {
        mbUnlockPosOfLowerObjs = false;
    }
    inline bool IsUnlockPosOfLowerObjs()
    {
        return mbUnlockPosOfLowerObjs;
    }

    /** search for last content in the current footnote frame

        OD 2005-12-02 #i27138#

        @return SwCntntFrm*
        pointer to found last content frame. NULL, if none is found.
    */
    SwCntntFrm* FindLastCntnt();
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
