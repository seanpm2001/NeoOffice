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
#ifndef INCLUDED_SW_INC_NDOLE_HXX
#define INCLUDED_SW_INC_NDOLE_HXX

#include <ndnotxt.hxx>

#include <svtools/embedhlp.hxx>

class SwGrfFmtColl;
class SwDoc;
class SwOLENode;

class SwOLEListener_Impl;
#ifdef NO_LIBO_CHECK_IFRAME_TARGET
class SwEmbedObjectLink;
#else	// NO_LIBO_CHECK_IFRAME_TARGET
namespace sfx2 { class SvBaseLink; }
#endif	// NO_LIBO_CHECK_IFRAME_TARGET
class SW_DLLPUBLIC SwOLEObj
{
    friend class SwOLENode;

    const SwOLENode* pOLENd;
    SwOLEListener_Impl* pListener;

    /** Either ref or name are known. If only name is known, ref is obtained
       on demand by GetOleRef() from Sfx. */
    svt::EmbeddedObjectRef xOLERef;
    OUString aName;

    SwOLEObj( const SwOLEObj& rObj );   /// Not allowed.
    SwOLEObj();

    void SetNode( SwOLENode* pNode );

public:
    SwOLEObj( const svt::EmbeddedObjectRef& pObj );
    SwOLEObj( const OUString &rName, sal_Int64 nAspect );
    ~SwOLEObj();

    bool UnloadObject();
    static bool UnloadObject( ::com::sun::star::uno::Reference< ::com::sun::star::embed::XEmbeddedObject > xObj,
                                const SwDoc* pDoc,
                                sal_Int64 nAspect );

    OUString GetDescription();

    const com::sun::star::uno::Reference < com::sun::star::embed::XEmbeddedObject > GetOleRef();
    svt::EmbeddedObjectRef& GetObject();
    OUString GetCurrentPersistName() const { return aName; }
    OUString GetStyleString();
    bool IsOleRef() const;  ///< To avoid unnecessary loading of object.
};

// SwOLENode

class SW_DLLPUBLIC SwOLENode: public SwNoTxtNode
{
    friend class SwNodes;
    mutable SwOLEObj aOLEObj;
    OUString sChartTblName;     ///< with chart objects: name of referenced table.
    bool   bOLESizeInvalid; /**< Should be considered at SwDoc::PrtOLENotify
                                   (e.g. copied). Is not persistent. */

#ifdef NO_LIBO_CHECK_IFRAME_TARGET
    SwEmbedObjectLink*  mpObjectLink;
#else	// NO_LIBO_CHECK_IFRAME_TARGET
    sfx2::SvBaseLink*  mpObjectLink;
#endif	// NO_LIBO_CHECK_IFRAME_TARGET
    OUString maLinkURL;

    SwOLENode(  const SwNodeIndex &rWhere,
                const svt::EmbeddedObjectRef&,
                SwGrfFmtColl *pGrfColl,
                SwAttrSet* pAutoAttr = 0 );

    SwOLENode(  const SwNodeIndex &rWhere,
                const OUString &rName,
                sal_Int64 nAspect,
                SwGrfFmtColl *pGrfColl,
                SwAttrSet* pAutoAttr = 0 );

    /// aOLEObj has a private Copy-Ctor. We need one too:
    SwOLENode( const SwOLENode & );

    using SwNoTxtNode::GetGraphic;

public:
    const SwOLEObj& GetOLEObj() const { return aOLEObj; }
          SwOLEObj& GetOLEObj()       { return aOLEObj; }
    virtual ~SwOLENode();

    virtual SwCntntNode *SplitCntntNode( const SwPosition & ) SAL_OVERRIDE;

    /// Is in ndcopy.cxx.
    virtual SwCntntNode* MakeCopy( SwDoc*, const SwNodeIndex& ) const SAL_OVERRIDE;

    virtual Size GetTwipSize() const SAL_OVERRIDE;

    const Graphic* GetGraphic();

    void GetNewReplacement();

    virtual bool SavePersistentData() SAL_OVERRIDE;
    virtual bool RestorePersistentData() SAL_OVERRIDE;

    bool IsInGlobalDocSection() const;
    bool IsOLEObjectDeleted() const;

    bool IsOLESizeInvalid() const   { return bOLESizeInvalid; }
    void SetOLESizeInvalid( bool b ){ bOLESizeInvalid = b; }

    sal_Int64 GetAspect() const { return aOLEObj.GetObject().GetViewAspect(); }
    void SetAspect( sal_Int64 nAspect) { aOLEObj.GetObject().SetViewAspect( nAspect ); }

    /** Remove OLE-object from "memory".
       inline void Unload() { aOLEObj.Unload(); } */
    OUString GetDescription() const { return aOLEObj.GetDescription(); }

    bool UpdateLinkURL_Impl();
    void BreakFileLink_Impl();
    void DisconnectFileLink_Impl();

    void CheckFileLink_Impl();

    // #i99665#
    bool IsChart() const;

    OUString GetChartTblName() const { return sChartTblName; }
    void SetChartTblName( const OUString& rNm ) { sChartTblName = rNm; }

#ifndef NO_LIBO_CHECK_IFRAME_TARGET
    // react on visual change (invalidate)
    void SetChanged();
#endif	// !NO_LIBO_CHECK_IFRAME_TARGET
};

/// Inline methods from Node.hxx
inline SwOLENode *SwNode::GetOLENode()
{
     return ND_OLENODE == nNodeType ? static_cast<SwOLENode*>(this) : 0;
}

inline const SwOLENode *SwNode::GetOLENode() const
{
     return ND_OLENODE == nNodeType ? static_cast<const SwOLENode*>(this) : 0;
}

#endif  // _ INCLUDED_SW_INC_NDOLE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
