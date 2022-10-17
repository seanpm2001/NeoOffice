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
 * 
 *   Modified June 2018 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef INCLUDED_EDITENG_OUTLINER_HXX
#define INCLUDED_EDITENG_OUTLINER_HXX

#include <editeng/editdata.hxx>
#include <editeng/editstat.hxx>
#include <editeng/overflowingtxt.hxx>
#include <i18nlangtag/lang.h>
#include <rtl/ustring.hxx>
#include <svl/SfxBroadcaster.hxx>
#include <svl/languageoptions.hxx>
#include <svl/undo.hxx>
#include <tools/gen.hxx>
#include <tools/color.hxx>
#include <utility>
#include <vcl/outdev.hxx>
#include <vcl/errcode.hxx>
#include <tools/link.hxx>
#include <editeng/editengdllapi.h>

#include <vcl/GraphicObject.hxx>

#include <editeng/svxfont.hxx>
#include <editeng/eedata.hxx>
#include <editeng/paragraphdata.hxx>
#include <o3tl/typed_flags_set.hxx>

#include <optional>
#include <functional>
#include <memory>
#include <vector>

class OutlinerEditEng;
class Outliner;
class EditView;
class EditUndo;
class ParagraphList;
class OutlinerParaObject;
class SvStream;
class SvxSearchItem;
class SvxFieldItem;
namespace vcl { class Window; }
namespace weld { class Widget; }
class KeyEvent;
class MouseEvent;
class CommandEvent;
class MapMode;
class SfxStyleSheetPool;
class SfxStyleSheet;
class SfxItemPool;
class SfxItemSet;
class SvxNumberFormat;
class EditEngine;
class SvKeyValueIterator;
class SvxForbiddenCharactersTable;
class OutlinerViewShell;
enum class CharCompressType;
enum class TransliterationFlags;
class SvxFieldData;
enum class PointerStyle;
class SvxNumRule;
enum class TextRotation;

namespace com::sun::star::linguistic2 {
    class XSpellChecker1;
    class XHyphenator;
}
namespace svx{
    struct SpellPortion;
    typedef std::vector<SpellPortion> SpellPortions;
}
namespace basegfx { class B2DPolyPolygon; }
namespace com::sun::star::lang { struct Locale; }



// internal use only!
enum class ParaFlag
{
    NONE               = 0x0000,
    HOLDDEPTH          = 0x4000,
    SETBULLETTEXT      = 0x8000,
    ISPAGE             = 0x0100,
};
namespace o3tl
{
    template<> struct typed_flags<ParaFlag> : is_typed_flags<ParaFlag, 0xc100> {};
}

// Undo-Action-Ids
#define OLUNDO_DEPTH            EDITUNDO_USER
// #define OLUNDO_HEIGHT           EDITUNDO_USER+1
#define OLUNDO_EXPAND           EDITUNDO_USER+2
#define OLUNDO_COLLAPSE         EDITUNDO_USER+3
// #define OLUNDO_REMOVE           EDITUNDO_USER+4
#define OLUNDO_ATTR             EDITUNDO_USER+5
#define OLUNDO_INSERT           EDITUNDO_USER+6
// #define OLUNDO_MOVEPARAGRAPHS    EDITUNDO_USER+7

class Paragraph : protected ParagraphData
{
private:
    friend class Outliner;
    friend class ParagraphList;
    friend class OutlinerView;
    friend class OutlinerParaObject;
    friend class OutlinerEditEng;
    friend class OutlinerUndoCheckPara;
    friend class OutlinerUndoChangeParaFlags;

    Paragraph& operator=(const Paragraph& rPara ) = delete;

    OUString            aBulText;
    Size                aBulSize;
    ParaFlag            nFlags;
    bool                bVisible;

    bool                IsVisible() const { return bVisible; }
    void                SetText( const OUString& rText ) { aBulText = rText; aBulSize.setWidth(-1); }
    void                Invalidate() { aBulSize.setWidth(-1); }
    void                SetDepth( sal_Int16 nNewDepth ) { nDepth = nNewDepth; aBulSize.setWidth(-1); }
    const OUString&     GetText() const { return aBulText; }

                        Paragraph( sal_Int16 nDepth );
                        Paragraph( const Paragraph& ) = delete;
                        Paragraph( const ParagraphData& );

    sal_Int16           GetDepth() const { return nDepth; }

    sal_Int16           GetNumberingStartValue() const { return mnNumberingStartValue; }
    void                SetNumberingStartValue( sal_Int16 nNumberingStartValue );

    bool                IsParaIsNumberingRestart() const { return mbParaIsNumberingRestart; }
    void                SetParaIsNumberingRestart( bool bParaIsNumberingRestart );

    void                SetFlag( ParaFlag nFlag ) { nFlags |= nFlag; }
    void                RemoveFlag( ParaFlag nFlag ) { nFlags &= ~nFlag; }
    bool                HasFlag( ParaFlag nFlag ) const { return bool(nFlags & nFlag); }
public:
                        ~Paragraph();
    void                dumpAsXml(xmlTextWriterPtr pWriter) const;
};

struct ParaRange
{
    sal_Int32  nStartPara;
    sal_Int32  nEndPara;

            ParaRange( sal_Int32 nS, sal_Int32 nE ) : nStartPara(nS), nEndPara(nE) {}

    void    Adjust();
};

inline void ParaRange::Adjust()
{
    if ( nStartPara > nEndPara )
    {
        std::swap(nStartPara, nEndPara);
    }
}

class EDITENG_DLLPUBLIC OutlinerView final
{
    friend class Outliner;

    Outliner*                   pOwner;
    std::unique_ptr<EditView>   pEditView;

    enum class MouseTarget {
        Text = 0,
        Bullet = 1,
        Hypertext = 2,  // Outside OutputArea
        Outside = 3     // Outside OutputArea
    };

    EDITENG_DLLPRIVATE void         ImplExpandOrCollaps( sal_Int32 nStartPara, sal_Int32 nEndPara, bool bExpand );

    EDITENG_DLLPRIVATE sal_Int32    ImpCheckMousePos( const Point& rPosPixel, MouseTarget& reTarget);
    EDITENG_DLLPRIVATE void         ImpToggleExpand( Paragraph const * pParentPara );
    EDITENG_DLLPRIVATE ParaRange    ImpGetSelectedParagraphs( bool bIncludeHiddenChildren );

    EDITENG_DLLPRIVATE sal_Int32    ImpInitPaste( sal_Int32& rStart );
    EDITENG_DLLPRIVATE void         ImpPasted( sal_Int32 nStart, sal_Int32 nPrevParaCount, sal_Int32 nSize);
    EDITENG_DLLPRIVATE sal_Int32    ImpCalcSelectedPages( bool bIncludeFirstSelected );

    Link<LinkParamNone*,void> aEndCutPasteLink;

public:
                OutlinerView( Outliner* pOut, vcl::Window* pWindow );
                ~OutlinerView();

    EditView&   GetEditView() const { return *pEditView; }

    void        Scroll( tools::Long nHorzScroll, tools::Long nVertScroll );

    void        Paint( const tools::Rectangle& rRect, OutputDevice* pTargetDevice = nullptr );
    bool        PostKeyEvent( const KeyEvent& rKEvt, vcl::Window const * pFrameWin = nullptr );
    bool        MouseButtonDown( const MouseEvent& );
    bool        MouseButtonUp( const MouseEvent& );
    void        ReleaseMouse();
    bool        MouseMove( const MouseEvent& );

    void        ShowCursor( bool bGotoCursor = true, bool bActivate = false );
    void        HideCursor( bool bDeactivate = false );

    Outliner*   GetOutliner() const { return pOwner; }

    void        SetWindow( vcl::Window* pWindow );
    vcl::Window*     GetWindow() const;

    void        SetReadOnly( bool bReadOnly );
    bool        IsReadOnly() const;

    void        SetOutputArea( const tools::Rectangle& rRect );
    tools::Rectangle const & GetOutputArea() const;

    tools::Rectangle GetVisArea() const;

    void        CreateSelectionList (std::vector<Paragraph*> &aSelList) ;

    void        Select( Paragraph const * pParagraph, bool bSelect = true);

    OUString    GetSelected() const;
    void        SelectRange( sal_Int32 nFirst, sal_Int32 nCount );
    void        SetAttribs( const SfxItemSet& );
    void        Indent( short nDiff );
    void        AdjustDepth( short nDX );   // Later replace with Indent!

    void        AdjustHeight( tools::Long nDY );

    void        Read( SvStream& rInput, EETextFormat eFormat, SvKeyValueIterator* pHTTPHeaderAttrs );

    void        InsertText( const OUString& rNew, bool bSelect = false );
    void        InsertText( const OutlinerParaObject& rParaObj );
    void        Expand();
    void        Collapse();
    void        ExpandAll();
    void        CollapseAll();

    void        SetBackgroundColor( const Color& rColor );
    Color const & GetBackgroundColor() const;

    /// Informs this edit view about which view shell contains it.
    void RegisterViewShell(OutlinerViewShell* pViewShell);

    SfxItemSet  GetAttribs();

    void        Cut();
    void        Copy();
    void        Paste( bool bUseSpecial = false );
    void        PasteSpecial();

    const SfxStyleSheet*  GetStyleSheet() const;
    SfxStyleSheet*  GetStyleSheet();

    void            SetControlWord( EVControlBits nWord );
    EVControlBits   GetControlWord() const;

    void            SetAnchorMode( EEAnchorMode eMode );
    EEAnchorMode    GetAnchorMode() const;

    PointerStyle    GetPointer( const Point& rPosPixel );
    bool            Command(const CommandEvent& rCEvt);

    void            StartSpeller(weld::Widget* pDialogParent);
    EESpellState    StartThesaurus(weld::Widget* pDialogParent);
    sal_Int32       StartSearchAndReplace( const SvxSearchItem& rSearchItem );

    // for text conversion
    void            StartTextConversion(weld::Widget* pDialogParent, LanguageType nSrcLang, LanguageType nDestLang, const vcl::Font *pDestFont, sal_Int32 nOptions, bool bIsInteractive, bool bMultipleDoc);

    void            TransliterateText( TransliterationFlags nTransliterationMode );

    ESelection      GetSelection() const;

    SvtScriptType   GetSelectedScriptType() const;

    void        SetVisArea( const tools::Rectangle& rRect );
    void        SetSelection( const ESelection& );
    void GetSelectionRectangles(std::vector<tools::Rectangle>& rLogicRects) const;

    void        RemoveAttribs( bool bRemoveParaAttribs, bool bKeepLanguages = false );
    void        RemoveAttribsKeepLanguages( bool bRemoveParaAttribs );
    bool        HasSelection() const;

    void                InsertField( const SvxFieldItem& rFld );
    const SvxFieldItem* GetFieldUnderMousePointer() const;
    const SvxFieldItem* GetFieldAtSelection() const;
    /// Return the field at the current cursor position or nullptr if no field found
    const SvxFieldData* GetFieldAtCursor() const;
    /// Select the field at the current cursor position
    void SelectFieldAtCursor();

    /** enables bullets for the selected paragraphs if the bullets/numbering of the first paragraph is off
        or disables bullets/numbering for the selected paragraphs if the bullets/numbering of the first paragraph is on
    */
    void ToggleBullets();

    void ToggleBulletsNumbering(
        const bool bToggle,
        const bool bHandleBullets,
        const SvxNumRule* pNumRule );

    /** apply bullets/numbering for paragraphs

        @param boolean bHandleBullets
        true: handle bullets
        false: handle numbering

        @param pNewNumRule
        numbering rule which needs to be applied. can be 0.

        @param boolean bAtSelection
        true: apply bullets/numbering at selected paragraphs
        false: apply bullets/numbering at all paragraphs
    */
    void ApplyBulletsNumbering(
        const bool bHandleBullets,
        const SvxNumRule* pNewNumRule,
        const bool bCheckCurrentNumRuleBeforeApplyingNewNumRule,
        const bool bAtSelection = false );

    /** switch off bullets/numbering for paragraphs

        @param boolean bAtSelection
        true: switch off bullets/numbering at selected paragraphs
        false: switch off bullets/numbering at all paragraphs
    */
    void SwitchOffBulletsNumbering(
        const bool bAtSelection = false );

    /** enables numbering for the selected paragraphs that are not enabled and ignore all selected
        paragraphs that already have numbering enabled.
    */
    void        EnsureNumberingIsOn();

    bool        IsCursorAtWrongSpelledWord();
    bool        IsWrongSpelledWordAtPos( const Point& rPosPixel );
    void        ExecuteSpellPopup(const Point& rPosPixel, const Link<SpellCallbackInfo&,void>& rCallBack);

    void        SetInvalidateMore( sal_uInt16 nPixel );
    sal_uInt16  GetInvalidateMore() const;

    OUString    GetSurroundingText() const;
    Selection   GetSurroundingTextSelection() const;
    bool        DeleteSurroundingText(const Selection& rRange);

    void        SetEndCutPasteLinkHdl(const Link<LinkParamNone*,void> &rLink) { aEndCutPasteLink = rLink; }
};

/// Interface class to not depend on SfxViewShell in editeng.
class SAL_NO_VTABLE SAL_DLLPUBLIC_RTTI OutlinerViewShell
{
public:
    virtual void libreOfficeKitViewCallback(int nType, const char* pPayload) const = 0;
    virtual void libreOfficeKitViewCallbackWithViewId(int nType, const char* pPayload, int nViewId) const = 0;
    virtual void libreOfficeKitViewInvalidateTilesCallback(const tools::Rectangle* pRect, int nPart) const = 0;
    virtual void libreOfficeKitViewUpdatedCallback(int nType) const = 0;
    virtual void libreOfficeKitViewUpdatedCallbackPerViewId(int nType, int nViewId, int nSourceViewId) const = 0;
    virtual ViewShellId GetViewShellId() const = 0;
    virtual ViewShellDocId GetDocId() const = 0;
    /// Wrapper around SfxLokHelper::notifyOtherViews().
    virtual void NotifyOtherViews(int nType, const OString& rKey, const OString& rPayload) = 0;
    /// Wrapper around SfxLokHelper::notifyOtherView().
    virtual void NotifyOtherView(OutlinerViewShell* pOtherShell, int nType, const OString& rKey, const OString& rPayload) = 0;
    virtual vcl::Window* GetEditWindowForActiveOLEObj() const = 0;

protected:
    ~OutlinerViewShell() noexcept {}
};

// some thesaurus functionality to avoid code duplication in different projects...
bool EDITENG_DLLPUBLIC  GetStatusValueForThesaurusFromContext( OUString &rStatusVal, LanguageType &rLang, const EditView &rEditView );
void EDITENG_DLLPUBLIC  ReplaceTextWithSynonym( EditView &rEditView, const OUString &rSynonmText );

typedef ::std::vector< OutlinerView* > ViewList;

class EDITENG_DLLPUBLIC DrawPortionInfo
{
public:
    const Point&        mrStartPos;
    const OUString      maText;
    sal_Int32           mnTextStart;
    sal_Int32           mnTextLen;
    sal_Int32           mnPara;
    const SvxFont&      mrFont;
    o3tl::span<const sal_Int32> mpDXArray;

    const EEngineData::WrongSpellVector*  mpWrongSpellVector;
    const SvxFieldData* mpFieldData;
    const css::lang::Locale* mpLocale;
    const Color         maOverlineColor;
    const Color         maTextLineColor;

    sal_uInt8           mnBiDiLevel;

    bool                mbFilled;
    tools::Long                mnWidthToFill;

    bool                mbEndOfLine : 1;
    bool                mbEndOfParagraph : 1;
    bool                mbEndOfBullet : 1;

    bool IsRTL() const { return mnBiDiLevel % 2 == 1; }

    DrawPortionInfo(
        const Point& rPos,
        OUString aTxt,
        sal_Int32 nTxtStart,
        sal_Int32 nTxtLen,
        const SvxFont& rFnt,
        sal_Int32 nPar,
        o3tl::span<const sal_Int32> pDXArr,
        const EEngineData::WrongSpellVector* pWrongSpellVector,
        const SvxFieldData* pFieldData,
        const css::lang::Locale* pLocale,
        const Color& rOverlineColor,
        const Color& rTextLineColor,
        sal_uInt8 nBiDiLevel,
        bool bFilled,
        tools::Long nWidthToFill,
        bool bEndOfLine,
        bool bEndOfParagraph,
        bool bEndOfBullet)
    :   mrStartPos(rPos),
        maText(std::move(aTxt)),
        mnTextStart(nTxtStart),
        mnTextLen(nTxtLen),
        mnPara(nPar),
        mrFont(rFnt),
        mpDXArray(pDXArr),
        mpWrongSpellVector(pWrongSpellVector),
        mpFieldData(pFieldData),
        mpLocale(pLocale),
        maOverlineColor(rOverlineColor),
        maTextLineColor(rTextLineColor),
        mnBiDiLevel(nBiDiLevel),
        mbFilled( bFilled ),
        mnWidthToFill( nWidthToFill ),
        mbEndOfLine(bEndOfLine),
        mbEndOfParagraph(bEndOfParagraph),
        mbEndOfBullet(bEndOfBullet)
    {}
};

class EDITENG_DLLPUBLIC DrawBulletInfo
{
public:
    const GraphicObject maBulletGraphicObject;
    Point               maBulletPosition;
    Size                maBulletSize;

    DrawBulletInfo(
        const GraphicObject& rBulletGraphicObject,
        const Point& rBulletPosition,
        const Size& rBulletSize)
    :   maBulletGraphicObject(rBulletGraphicObject),
        maBulletPosition(rBulletPosition),
        maBulletSize(rBulletSize)
    {}
};

struct EDITENG_DLLPUBLIC PaintFirstLineInfo
{
    sal_Int32 mnPara;
    const Point& mrStartPos;
    VclPtr<OutputDevice> mpOutDev;

    PaintFirstLineInfo( sal_Int32 nPara, const Point& rStartPos, OutputDevice* pOutDev )
        : mnPara( nPara ), mrStartPos( rStartPos ), mpOutDev( pOutDev )
    {}
};

class SdrPage;

class EditFieldInfo
{
private:
    Outliner*           pOutliner;
    const SvxFieldItem& rFldItem;

    std::optional<Color> mxTxtColor;
    std::optional<Color> mxFldColor;

    OUString            aRepresentation;

    sal_Int32           nPara;
    sal_Int32           nPos;

                        EditFieldInfo( const EditFieldInfo& ) = delete;

    SdrPage*            mpSdrPage;

public:
                    EditFieldInfo( Outliner* pOutl, const SvxFieldItem& rFItem, sal_Int32 nPa, sal_Int32 nPo )
                        : rFldItem( rFItem )
                    {
                        pOutliner = pOutl;
                        nPara = nPa; nPos = nPo;
                        mpSdrPage = nullptr;
                    }

    Outliner*       GetOutliner() const { return pOutliner; }

    const SvxFieldItem& GetField() const { return rFldItem; }

    std::optional<Color> const & GetTextColor() const { return mxTxtColor; }
    void            SetTextColor( std::optional<Color> xCol ) { mxTxtColor = xCol; }

    std::optional<Color> const & GetFieldColor() const { return mxFldColor; }
    void            SetFieldColor( std::optional<Color> xCol ) { mxFldColor = xCol; }

    sal_Int32       GetPara() const { return nPara; }
    sal_Int32       GetPos() const { return nPos; }

    const OUString&     GetRepresentation() const                { return aRepresentation; }
    OUString&           GetRepresentation()                      { return aRepresentation; }
    void                SetRepresentation( const OUString& rStr ){ aRepresentation = rStr; }

    void            SetSdrPage( SdrPage* pPage ) { mpSdrPage = pPage; }
    SdrPage*        GetSdrPage() const { return mpSdrPage; }
};

 struct EBulletInfo
{
    SvxFont           aFont;
    tools::Rectangle  aBounds;
    OUString          aText;
    sal_Int32         nParagraph;
    sal_uInt16        nType;          // see SvxNumberType
    bool              bVisible;

    EBulletInfo() : nParagraph( EE_PARA_NOT_FOUND ), nType( 0 ), bVisible( false ) {}
};

enum class OutlinerMode {
    DontKnow       = 0x0000,
    TextObject     = 0x0001,
    TitleObject    = 0x0002,
    OutlineObject  = 0x0003,
    OutlineView    = 0x0004
};

class EDITENG_DLLPUBLIC Outliner : public SfxBroadcaster
{
public:
    struct ParagraphHdlParam { Outliner* pOutliner; Paragraph* pPara; };
    struct DepthChangeHdlParam { Outliner* pOutliner; Paragraph* pPara; ParaFlag nPrevFlags; };
private:
    friend class OutlinerView;
    friend class OutlinerEditEng;
    friend class OutlinerParaObject;
    friend class OLUndoExpand;
    friend class OutlinerUndoChangeDepth;
    friend class OutlinerUndoCheckPara;
    friend class OutlinerUndoChangeParaFlags;

    friend class TextChainingUtils;

    std::unique_ptr<OutlinerEditEng> pEditEngine;

    std::unique_ptr<ParagraphList>   pParaList;
    ViewList            aViewList;

    sal_Int32           mnFirstSelPage;
    Link<DrawPortionInfo*,void> aDrawPortionHdl;
    Link<DrawBulletInfo*,void>     aDrawBulletHdl;
    Link<ParagraphHdlParam,void>   aParaInsertedHdl;
    Link<ParagraphHdlParam,void>   aParaRemovingHdl;
    Link<DepthChangeHdlParam,void> aDepthChangedHdl;
    Link<Outliner*,void>           aBeginMovingHdl;
    Link<Outliner*,void>           aEndMovingHdl;
    Link<OutlinerView*,bool>       aIndentingPagesHdl;
    Link<OutlinerView*,bool>       aRemovingPagesHdl;
    Link<EditFieldInfo*,void>      aCalcFieldValueHdl;
    Link<PaintFirstLineInfo*,void> maPaintFirstLineHdl;
    Link<PasteOrDropInfos*,void>   maBeginPasteOrDropHdl;
    Link<PasteOrDropInfos*,void>   maEndPasteOrDropHdl;

    sal_Int32           nDepthChangedHdlPrevDepth;
    sal_Int16           nMaxDepth;
    static constexpr sal_Int16 gnMinDepth = -1;

    OutlinerMode        nOutlinerMode;

    bool                bFirstParaIsEmpty;
    sal_uInt8           nBlockInsCallback;
    bool                bStrippingPortions;
    bool                bPasting;

    DECL_DLLPRIVATE_LINK(    ParaVisibleStateChangedHdl, Paragraph&, void );
    DECL_DLLPRIVATE_LINK(    BeginMovingParagraphsHdl, MoveParagraphsInfo&, void );
    DECL_DLLPRIVATE_LINK(    EndMovingParagraphsHdl, MoveParagraphsInfo&, void );
    DECL_DLLPRIVATE_LINK(    BeginPasteOrDropHdl, PasteOrDropInfos&, void );
    DECL_DLLPRIVATE_LINK(    EndPasteOrDropHdl, PasteOrDropInfos&, void );
    DECL_DLLPRIVATE_LINK(    EditEngineNotifyHdl, EENotify&, void );
    void                ImplCheckParagraphs( sal_Int32 nStart, sal_Int32 nEnd );
    bool                ImplHasNumberFormat( sal_Int32 nPara ) const;
    Size                ImplGetBulletSize( sal_Int32 nPara );
    sal_uInt16          ImplGetNumbering( sal_Int32 nPara, const SvxNumberFormat* pParaFmt );
    void                ImplCalcBulletText( sal_Int32 nPara, bool bRecalcLevel, bool bRecalcChildren );
    OUString            ImplGetBulletText( sal_Int32 nPara );
    void                ImplCheckNumBulletItem( sal_Int32 nPara );
    void                ImplInitDepth( sal_Int32 nPara, sal_Int16 nDepth, bool bCreateUndo );
    void                ImplSetLevelDependentStyleSheet( sal_Int32 nPara );

    void                ImplBlockInsertionCallbacks( bool b );

    void        ImpFilterIndents( sal_Int32 nFirstPara, sal_Int32 nLastPara );
    bool        ImpConvertEdtToOut( sal_Int32 nPara );

    void        ImpTextPasted( sal_Int32 nStartPara, sal_Int32 nCount );
    vcl::Font   ImpCalcBulletFont( sal_Int32 nPara ) const;
    tools::Rectangle   ImpCalcBulletArea( sal_Int32 nPara, bool bAdjust, bool bReturnPaperPos );
    bool        ImpCanIndentSelectedPages( OutlinerView* pCurView );
    bool        ImpCanDeleteSelectedPages( OutlinerView* pCurView );
    bool        ImpCanDeleteSelectedPages( OutlinerView* pCurView, sal_Int32 nFirstPage, sal_Int32 nPages );

    void        ImplCheckDepth( sal_Int16& rnDepth ) const;

protected:
    void            ParagraphInserted( sal_Int32 nParagraph );
    void            ParagraphDeleted( sal_Int32 nParagraph );
    void            ParaAttribsChanged( sal_Int32 nParagraph );

    void            StyleSheetChanged( SfxStyleSheet const * pStyle );

    void            InvalidateBullet(sal_Int32 nPara);
    void            PaintBullet(sal_Int32 nPara, const Point& rStartPos,
                                const Point& rOrigin, Degree10 nOrientation,
                                OutputDevice& rOutDev);

    // used by OutlinerEditEng. Allows Outliner objects to provide
    // bullet access to the EditEngine.
    const SvxNumberFormat*  GetNumberFormat( sal_Int32 nPara ) const;

public:

                    Outliner( SfxItemPool* pPool, OutlinerMode nMode );
    virtual         ~Outliner() override;

    void            dumpAsXml(xmlTextWriterPtr pWriter) const;

    void            Init( OutlinerMode nMode );

    void            SetVertical( bool bVertical);
    void            SetRotation(TextRotation nRotation);
    bool            IsVertical() const;
    bool            IsTopToBottom() const;

    void SetTextColumns(sal_Int16 nColumns, sal_Int32 nSpacing);

    void            SetFixedCellHeight( bool bUseFixedCellHeight );

    void                        SetDefaultHorizontalTextDirection( EEHorizontalTextDirection eHTextDir );
    EEHorizontalTextDirection   GetDefaultHorizontalTextDirection() const;

    LanguageType    GetLanguage( sal_Int32 nPara, sal_Int32 nPos ) const;

    void            SetAsianCompressionMode( CharCompressType nCompressionMode );

    void            SetKernAsianPunctuation( bool bEnabled );

    void            SetAddExtLeading( bool b );

    size_t          InsertView( OutlinerView* pView, size_t nIndex = size_t(-1) );
    void            RemoveView( OutlinerView const * pView );
    void            RemoveView( size_t nIndex );
    OutlinerView*   GetView( size_t nIndex ) const;
    size_t          GetViewCount() const;

    Paragraph*      Insert( const OUString& rText, sal_Int32 nAbsPos = EE_PARA_APPEND, sal_Int16 nDepth = 0 );
    void            SetText( const OutlinerParaObject& );
    void            AddText( const OutlinerParaObject&, bool bAppend = false );
    void            SetText( const OUString& rText, Paragraph* pParagraph );
    OUString        GetText( Paragraph const * pPara, sal_Int32 nParaCount=1 ) const;

    void            SetToEmptyText();

    std::optional<OutlinerParaObject> CreateParaObject( sal_Int32 nStartPara = 0, sal_Int32 nParaCount = EE_PARA_ALL ) const;

    const SfxItemSet& GetEmptyItemSet() const;

    void            SetRefMapMode( const MapMode& );
    MapMode const & GetRefMapMode() const;

    void            SetBackgroundColor( const Color& rColor );
    Color const &   GetBackgroundColor() const;

    void            SetMaxDepth( sal_Int16 nDepth );
    sal_Int16       GetMaxDepth() const { return nMaxDepth; }

    /// @return previous value of bUpdateLayout state
    bool            SetUpdateLayout( bool bUpdate );
    bool            IsUpdateLayout() const;

    void            Clear();

    void            RemoveAttribs( const ESelection& rSelection, bool bRemoveParaAttribs, sal_uInt16 nWhich );

    sal_Int32       GetParagraphCount() const;
    Paragraph*      GetParagraph( sal_Int32 nAbsPos ) const;

    bool            HasChildren( Paragraph const * pParagraph ) const;
    sal_Int32       GetChildCount( Paragraph const * pParent ) const;
    bool            IsExpanded( Paragraph const * pPara ) const;
    Paragraph*      GetParent( Paragraph const * pParagraph ) const;
    sal_Int32       GetAbsPos( Paragraph const * pPara ) const;

    sal_Int16       GetDepth( sal_Int32 nPara ) const;
    void            SetDepth( Paragraph* pParagraph, sal_Int16 nNewDepth );

    void            EnableUndo( bool bEnable );
    bool            IsUndoEnabled() const;
    void            UndoActionStart( sal_uInt16 nId );
    void            UndoActionEnd();
    void            InsertUndo( std::unique_ptr<EditUndo> pUndo );
    bool            IsInUndo() const;

    void            ClearModifyFlag();
    bool            IsModified() const;

    void            ParagraphInsertedHdl(Paragraph*);
    void            SetParaInsertedHdl(const Link<ParagraphHdlParam,void>& rLink){aParaInsertedHdl=rLink;}
    const Link<ParagraphHdlParam,void>& GetParaInsertedHdl() const { return aParaInsertedHdl; }

    void            SetParaRemovingHdl(const Link<ParagraphHdlParam,void>& rLink){aParaRemovingHdl=rLink;}
    const Link<ParagraphHdlParam,void>& GetParaRemovingHdl() const { return aParaRemovingHdl; }

    std::optional<NonOverflowingText> GetNonOverflowingText() const;
    std::optional<OverflowingText> GetOverflowingText() const;
    void ClearOverflowingParaNum();
    bool IsPageOverflow();

    OutlinerParaObject GetEmptyParaObject() const;


    void            DepthChangedHdl(Paragraph*, ParaFlag nPrevFlags);
    void            SetDepthChangedHdl(const Link<DepthChangeHdlParam,void>& rLink){aDepthChangedHdl=rLink;}
    const Link<DepthChangeHdlParam,void>& GetDepthChangedHdl() const { return aDepthChangedHdl; }
    sal_Int16       GetPrevDepth() const { return static_cast<sal_Int16>(nDepthChangedHdlPrevDepth); }

    bool            RemovingPagesHdl( OutlinerView* );
    void            SetRemovingPagesHdl(const Link<OutlinerView*,bool>& rLink){aRemovingPagesHdl=rLink;}
    bool            IndentingPagesHdl( OutlinerView* );
    void            SetIndentingPagesHdl(const Link<OutlinerView*,bool>& rLink){aIndentingPagesHdl=rLink;}
    // valid only in the two upper handlers
    sal_Int32       GetSelPageCount() const { return nDepthChangedHdlPrevDepth; }

    void            SetCalcFieldValueHdl(const Link<EditFieldInfo*,void>& rLink ) { aCalcFieldValueHdl= rLink; }
    const Link<EditFieldInfo*,void>& GetCalcFieldValueHdl() const { return aCalcFieldValueHdl; }

    void            SetDrawPortionHdl(const Link<DrawPortionInfo*,void>& rLink){aDrawPortionHdl=rLink;}

    void            SetDrawBulletHdl(const Link<DrawBulletInfo*,void>& rLink){aDrawBulletHdl=rLink;}

    void            SetPaintFirstLineHdl(const Link<PaintFirstLineInfo*,void>& rLink) { maPaintFirstLineHdl = rLink; }

    void            SetModifyHdl( const Link<LinkParamNone*,void>& rLink );
    Link<LinkParamNone*,void> const & GetModifyHdl() const;

    void            SetNotifyHdl( const Link<EENotify&,void>& rLink );

    void            SetStatusEventHdl( const Link<EditStatus&, void>& rLink );
    Link<EditStatus&, void> const & GetStatusEventHdl() const;

    void            Draw( OutputDevice& rOutDev, const tools::Rectangle& rOutRect );
    void            Draw( OutputDevice& rOutDev, const Point& rStartPos );

    const Size&     GetPaperSize() const;
    void            SetPaperSize( const Size& rSize );

    void            SetPolygon( const basegfx::B2DPolyPolygon& rPolyPolygon );
    void            SetPolygon( const basegfx::B2DPolyPolygon& rPolyPolygon, const basegfx::B2DPolyPolygon* pLinePolyPolygon);
    void            ClearPolygon();

    const Size&     GetMinAutoPaperSize() const;
    void            SetMinAutoPaperSize( const Size& rSz );

    const Size&     GetMaxAutoPaperSize() const;
    void            SetMaxAutoPaperSize( const Size& rSz );

    void SetMinColumnWrapHeight(tools::Long nVal);

    void            SetDefTab( sal_uInt16 nTab );

    bool            IsFlatMode() const;
    void            SetFlatMode( bool bFlat );

    void            EnableAutoColor( bool b );

    void            ForceAutoColor( bool b );
    bool            IsForceAutoColor() const;

    EBulletInfo     GetBulletInfo( sal_Int32 nPara );

    void            SetWordDelimiters( const OUString& rDelimiters );
    OUString const & GetWordDelimiters() const;
    OUString        GetWord( sal_Int32 nPara, sal_Int32 nIndex );

    void            StripPortions();

    void DrawingText( const Point& rStartPos, const OUString& rText,
                              sal_Int32 nTextStart, sal_Int32 nTextLen,
                              o3tl::span<const sal_Int32> pDXArray, const SvxFont& rFont,
                              sal_Int32 nPara, sal_uInt8 nRightToLeft,
                              const EEngineData::WrongSpellVector* pWrongSpellVector,
                              const SvxFieldData* pFieldData,
                              bool bEndOfLine,
                              bool bEndOfParagraph,
                              bool bEndOfBullet,
                              const css::lang::Locale* pLocale,
                              const Color& rOverlineColor,
                              const Color& rTextLineColor);

    void DrawingTab( const Point& rStartPos, tools::Long nWidth, const OUString& rChar,
                             const SvxFont& rFont, sal_Int32 nPara, sal_uInt8 nRightToLeft,
                             bool bEndOfLine,
                             bool bEndOfParagraph,
                             const Color& rOverlineColor,
                             const Color& rTextLineColor);

    Size            CalcTextSize();
    Size            CalcTextSizeNTP();

    void                SetStyleSheetPool( SfxStyleSheetPool* pSPool );
    SfxStyleSheetPool*  GetStyleSheetPool();

    bool            IsInSelectionMode() const;

    void            SetStyleSheet( sal_Int32 nPara, SfxStyleSheet* pStyle );
    SfxStyleSheet*  GetStyleSheet( sal_Int32 nPara );

    void            SetParaAttribs( sal_Int32 nPara, const SfxItemSet& );
    SfxItemSet const & GetParaAttribs( sal_Int32 nPara ) const;

    void            Remove( Paragraph const * pPara, sal_Int32 nParaCount );
    bool            Expand( Paragraph const * );
    bool            Collapse( Paragraph const * );

    void            SetParaFlag( Paragraph* pPara,  ParaFlag nFlag );
    static bool     HasParaFlag( const Paragraph* pPara, ParaFlag nFlag );


    void            SetControlWord( EEControlBits nWord );
    EEControlBits   GetControlWord() const;

    const Link<Outliner*,void>& GetBeginMovingHdl() const { return aBeginMovingHdl; }
    void            SetBeginMovingHdl(const Link<Outliner*,void>& rLink) {aBeginMovingHdl=rLink;}
    const Link<Outliner*,void>& GetEndMovingHdl() const {return aEndMovingHdl;}
    void            SetEndMovingHdl( const Link<Outliner*,void>& rLink){aEndMovingHdl=rLink;}

    sal_uInt32           GetLineCount( sal_Int32 nParagraph ) const;
    sal_Int32           GetLineLen( sal_Int32 nParagraph, sal_Int32 nLine ) const;
    sal_uInt32           GetLineHeight( sal_Int32 nParagraph );

    ErrCode             Read( SvStream& rInput, const OUString& rBaseURL, EETextFormat, SvKeyValueIterator* pHTTPHeaderAttrs = nullptr );

    SfxUndoManager& GetUndoManager();
    SfxUndoManager* SetUndoManager(SfxUndoManager* pNew);

    void            QuickSetAttribs( const SfxItemSet& rSet, const ESelection& rSel );
    void            QuickInsertField( const SvxFieldItem& rFld, const ESelection& rSel );
    void            QuickInsertLineBreak( const ESelection& rSel );

    // Only for EditEngine mode
    void            QuickInsertText( const OUString& rText, const ESelection& rSel );
    void            QuickDelete( const ESelection& rSel );
    /// Set attributes from rSet an all characters of nPara.
    void SetCharAttribs(sal_Int32 nPara, const SfxItemSet& rSet);
    void            RemoveCharAttribs( sal_Int32 nPara, sal_uInt16 nWhich = 0 );
    void            QuickFormatDoc();

    bool            UpdateFields();
    void            RemoveFields( const std::function<bool ( const SvxFieldData* )>& isFieldData = [] (const SvxFieldData* ){return true;} );

    virtual OUString CalcFieldValue( const SvxFieldItem& rField, sal_Int32 nPara, sal_Int32 nPos, std::optional<Color>& rTxtColor, std::optional<Color>& rFldColor );

    void            SetSpeller( css::uno::Reference< css::linguistic2::XSpellChecker1 > const &xSpeller );
    css::uno::Reference< css::linguistic2::XSpellChecker1 > const &
                    GetSpeller();
    void            SetHyphenator( css::uno::Reference< css::linguistic2::XHyphenator > const & xHyph );

    static void     SetForbiddenCharsTable(const std::shared_ptr<SvxForbiddenCharactersTable>& xForbiddenChars);

    // Deprecated
    void            SetDefaultLanguage( LanguageType eLang );

    void            CompleteOnlineSpelling();

    EESpellState    HasSpellErrors();
    bool            HasText( const SvxSearchItem& rSearchItem );
    virtual bool    SpellNextDocument();

    // for text conversion
    bool            HasConvertibleTextPortion( LanguageType nLang );
    virtual bool    ConvertNextDocument();

    void            SetEditTextObjectPool( SfxItemPool* pPool );
    SfxItemPool*    GetEditTextObjectPool() const;

    void            SetRefDevice( OutputDevice* pRefDev );
    OutputDevice*   GetRefDevice() const;

    sal_uInt32      GetTextHeight() const;
    tools::Rectangle GetParaBounds( sal_Int32 nParagraph ) const;
    Point           GetDocPos( const Point& rPaperPos ) const;
    bool            IsTextPos( const Point& rPaperPos, sal_uInt16 nBorder );
    bool            IsTextPos( const Point& rPaperPos, sal_uInt16 nBorder, bool* pbBulletPos );

    void            SetGlobalCharStretching( sal_uInt16 nX = 100, sal_uInt16 nY = 100 );
    void            GetGlobalCharStretching( sal_uInt16& rX, sal_uInt16& rY ) const;
    void            EraseVirtualDevice();

    bool            ShouldCreateBigTextObject() const;

    const EditEngine& GetEditEngine() const;

    // this is needed for StarOffice Api
    void            SetLevelDependentStyleSheet( sal_Int32 nPara );

    OutlinerMode    GetOutlinerMode() const { return nOutlinerMode; }

    // spell and return a sentence
    bool            SpellSentence(EditView const & rEditView, svx::SpellPortions& rToFill );
    // put spell position to start of current sentence
    void            PutSpellingToSentenceStart( EditView const & rEditView );
    // applies a changed sentence
    void            ApplyChangedSentence(EditView const & rEditView, const svx::SpellPortions& rNewPortions, bool bRecheck );

    /** sets a link that is called at the beginning of a drag operation at an edit view */
    void            SetBeginDropHdl( const Link<EditView*,void>& rLink );

    /** sets a link that is called at the end of a drag operation at an edit view */
    void            SetEndDropHdl( const Link<EditView*,void>& rLink );

    /** sets a link that is called before a drop or paste operation. */
    void            SetBeginPasteOrDropHdl( const Link<PasteOrDropInfos*,void>& rLink );

    /** sets a link that is called after a drop or paste operation. */
    void            SetEndPasteOrDropHdl( const Link<PasteOrDropInfos*,void>& rLink );

    sal_Int16 GetNumberingStartValue( sal_Int32 nPara ) const;
    void SetNumberingStartValue( sal_Int32 nPara, sal_Int16 nNumberingStartValue );

    bool IsParaIsNumberingRestart( sal_Int32 nPara ) const;
    void SetParaIsNumberingRestart( sal_Int32 nPara, bool bParaIsNumberingRestart );

    /** determine the bullets/numbering status of the given paragraphs

        @param nParaStart
        index of paragraph at which the check starts

        @param nParaEnd
        index of paragraph at which the check ends

        @returns
        0 : all paragraphs have bullets
        1 : all paragraphs have numbering
        2 : otherwise
    */
    sal_Int32 GetBulletsNumberingStatus(
        const sal_Int32 nParaStart,
        const sal_Int32 nParaEnd ) const;

    // convenient method to determine the bullets/numbering status for all paragraphs
    sal_Int32 GetBulletsNumberingStatus() const;
};

#ifdef USE_JAVA

bool EDITENG_DLLPUBLIC ImplIsValidOutlinerView( const OutlinerView *pOutlinerView );

#endif  // USE_JAVA

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
