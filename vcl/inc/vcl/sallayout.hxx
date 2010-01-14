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
 * Modified May 2007 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

#ifndef _SV_SALLAYOUT_HXX
#define _SV_SALLAYOUT_HXX

#include <tools/gen.hxx>

#include <vector>
namespace basegfx {
	class B2DPolyPolygon;
	typedef std::vector<B2DPolyPolygon> B2DPolyPolygonVector;
}

#ifndef _TOOLS_LANG_HXX
typedef unsigned short LanguageType;
#endif

#include <vector>
#include <list>
#include <vcl/dllapi.h>

// for typedef sal_UCS4
#include <vcl/vclenum.hxx>

class SalGraphics;
class ImplFontData;

#define MAX_FALLBACK 16

// ----------------
// - LayoutOption -
// ----------------

#define SAL_LAYOUT_BIDI_RTL                 0x0001
#define SAL_LAYOUT_BIDI_STRONG              0x0002
#define SAL_LAYOUT_RIGHT_ALIGN              0x0004
#define SAL_LAYOUT_KERNING_PAIRS            0x0010
#define SAL_LAYOUT_KERNING_ASIAN            0x0020
#define SAL_LAYOUT_VERTICAL                 0x0040
#define SAL_LAYOUT_COMPLEX_DISABLED         0x0100
#define SAL_LAYOUT_ENABLE_LIGATURES         0x0200
#define SAL_LAYOUT_SUBSTITUTE_DIGITS        0x0400
#define SAL_LAYOUT_KASHIDA_JUSTIFICATON     0x0800
#define SAL_LAYOUT_DISABLE_GLYPH_PROCESSING 0x1000
#define SAL_LAYOUT_FOR_FALLBACK             0x2000

// -----------------

// used for managing runs e.g. for BiDi, glyph and script fallback
class VCL_DLLPUBLIC ImplLayoutRuns
{
private:
    int                 mnRunIndex;
    std::vector<int>    maRuns;

public:
            ImplLayoutRuns() { mnRunIndex = 0; maRuns.reserve(8); }

    void    Clear()             { maRuns.clear(); }
    bool    AddPos( int nCharPos, bool bRTL );
    bool    AddRun( int nMinRunPos, int nEndRunPos, bool bRTL );

    bool    IsEmpty() const     { return maRuns.empty(); }
    void    ResetPos()          { mnRunIndex = 0; }
    void    NextRun()           { mnRunIndex += 2; }
    bool    GetRun( int* nMinRunPos, int* nEndRunPos, bool* bRTL ) const;
    bool    GetNextPos( int* nCharPos, bool* bRTL );
    bool    PosIsInRun( int nCharPos ) const;
    bool    PosIsInAnyRun( int nCharPos ) const;
};

// -----------------

class ImplLayoutArgs
{
public:
    // string related inputs
    int                 mnFlags;
    int                 mnLength;
    int                 mnMinCharPos;
    int                 mnEndCharPos;
    const xub_Unicode*  mpStr;

    // positioning related inputs
    const sal_Int32*    mpDXArray;          // in pixel units
    long                mnLayoutWidth;      // in pixel units
    int                 mnOrientation;      // in 0-3600 system

    // data for bidi and glyph+script fallback
    ImplLayoutRuns      maRuns;
    ImplLayoutRuns      maReruns;

public:
                ImplLayoutArgs( const xub_Unicode* pStr, int nLength,
                    int nMinCharPos, int nEndCharPos, int nFlags );

    void        SetLayoutWidth( long nWidth )       { mnLayoutWidth = nWidth; }
    void        SetDXArray( const sal_Int32* pDXArray )  { mpDXArray = pDXArray; }
    void        SetOrientation( int nOrientation )  { mnOrientation = nOrientation; }

    void        ResetPos()
                    { maRuns.ResetPos(); }
    bool        GetNextPos( int* nCharPos, bool* bRTL )
                    { return maRuns.GetNextPos( nCharPos, bRTL ); }
    bool        GetNextRun( int* nMinRunPos, int* nEndRunPos, bool* bRTL );
    bool        NeedFallback( int nCharPos, bool bRTL )
                    { return maReruns.AddPos( nCharPos, bRTL ); }
    bool        NeedFallback( int nMinRunPos, int nEndRunPos, bool bRTL )
                    { return maReruns.AddRun( nMinRunPos, nEndRunPos, bRTL ); }
    // methods used by BiDi and glyph fallback
    bool        NeedFallback() const
                    { return !maReruns.IsEmpty(); }
    bool        PrepareFallback();

protected:
    void        AddRun( int nMinCharPos, int nEndCharPos, bool bRTL );
};

// helper functions often used with ImplLayoutArgs
int GetVerticalFlags( sal_UCS4 );
sal_UCS4 GetVerticalChar( sal_UCS4 );
// #i80090# GetMirroredChar also needed outside vcl, moved to svapp.hxx
// VCL_DLLPUBLIC sal_UCS4 GetMirroredChar( sal_UCS4 );
sal_UCS4 GetLocalizedChar( sal_UCS4, LanguageType );
VCL_DLLPUBLIC const char* GetAutofallback( sal_UCS4 ) ;

// -------------
// - SalLayout -
// -------------

typedef sal_uInt32 sal_GlyphId;

// Glyph Flags
#define GF_NONE     0x00000000
#define GF_FLAGMASK 0xFF800000
#define GF_IDXMASK  ~GF_FLAGMASK
#define GF_ISCHAR   0x00800000
#define GF_ROTL     0x01000000
// caution !!!
#define GF_VERT     0x02000000
// GF_VERT is only for windows implementation
// (win/source/gdi/salgdi3.cxx, win/source/gdi/winlayout.cxx)
// don't use this elsewhere !!!
#define GF_ROTR     0x03000000
#define GF_ROTMASK  0x03000000
#define GF_UNHINTED 0x04000000
#define GF_GSUB     0x08000000
#define GF_FONTMASK 0xF0000000
#define GF_FONTSHIFT 28

#define GF_DROPPED  0xFFFFFFFF

// all positions/widths are in font units
// one exception: drawposition is in pixel units

class VCL_DLLPUBLIC SalLayout
{
public:
    // used by upper layers
    Point&          DrawBase()                              { return maDrawBase; }
    const Point&    DrawBase() const                        { return maDrawBase; }
    Point&          DrawOffset()                            { return maDrawOffset; }
    const Point&    DrawOffset() const                      { return maDrawOffset; }
    Point           GetDrawPosition( const Point& rRelative = Point(0,0) ) const;

    virtual bool    LayoutText( ImplLayoutArgs& ) = 0;  // first step of layouting
    virtual void    AdjustLayout( ImplLayoutArgs& );    // adjusting after fallback etc.
    virtual void    InitFont() const {}
    virtual void    DrawText( SalGraphics& ) const = 0;

    int             GetUnitsPerPixel() const                { return mnUnitsPerPixel; }
    int             GetOrientation() const                  { return mnOrientation; }
    
    virtual const ImplFontData* GetFallbackFontData( sal_GlyphId ) const;

    // methods using string indexing
    virtual int     GetTextBreak( long nMaxWidth, long nCharExtra=0, int nFactor=1 ) const = 0;
    virtual long    FillDXArray( sal_Int32* pDXArray ) const = 0;
    virtual long    GetTextWidth() const { return FillDXArray( NULL ); }
    virtual void    GetCaretPositions( int nArraySize, sal_Int32* pCaretXArray ) const = 0;

    // methods using glyph indexing
    virtual int     GetNextGlyphs( int nLen, sal_GlyphId* pGlyphIdAry, Point& rPos, int&,
                        sal_Int32* pGlyphAdvAry = NULL, int* pCharPosAry = NULL ) const = 0;
    virtual bool    GetOutline( SalGraphics&, ::basegfx::B2DPolyPolygonVector& ) const;
    virtual bool    GetBoundRect( SalGraphics&, Rectangle& ) const;

    virtual bool    IsSpacingGlyph( sal_GlyphId ) const;

    // reference counting
    void            Reference() const;
    void            Release() const;

    // used by glyph+font+script fallback
    virtual void    MoveGlyph( int nStart, long nNewXPos ) = 0;
    virtual void    DropGlyph( int nStart ) = 0;
    virtual void    Simplify( bool bIsBase ) = 0;

protected:
    // used by layout engines
                    SalLayout();
    virtual         ~SalLayout();

    // used by layout layers
    void            SetUnitsPerPixel( int n )               { mnUnitsPerPixel = n; }
    void            SetOrientation( int nOrientation )      // in 0-3600 system
                    { mnOrientation = nOrientation; }

    static int      CalcAsianKerning( sal_UCS4, bool bLeft, bool bVertical );

private:
    // enforce proper copy semantic
    SAL_DLLPRIVATE  SalLayout( const SalLayout& );
    SAL_DLLPRIVATE  SalLayout& operator=( const SalLayout& );

protected:
    int             mnMinCharPos;
    int             mnEndCharPos;
    int             mnLayoutFlags;

    int             mnUnitsPerPixel;
    int             mnOrientation;

    mutable int     mnRefCount;
    mutable Point   maDrawOffset;
    Point           maDrawBase;

#ifdef USE_JAVA
    bool            mbSpecialSpacingGlyph;
#endif	// USE_JAVA
};

// ------------------
// - MultiSalLayout -
// ------------------

class VCL_DLLPUBLIC MultiSalLayout : public SalLayout
{
public:
    virtual void    DrawText( SalGraphics& ) const;
    virtual int     GetTextBreak( long nMaxWidth, long nCharExtra, int nFactor ) const;
    virtual long    FillDXArray( sal_Int32* pDXArray ) const;
    virtual void    GetCaretPositions( int nArraySize, sal_Int32* pCaretXArray ) const;
    virtual int     GetNextGlyphs( int nLen, sal_GlyphId* pGlyphIdxAry, Point& rPos,
                        int&, sal_Int32* pGlyphAdvAry, int* pCharPosAry ) const;
    virtual bool    GetOutline( SalGraphics&, ::basegfx::B2DPolyPolygonVector& ) const;
    virtual bool    GetBoundRect( SalGraphics&, Rectangle& ) const;

    // used only by OutputDevice::ImplLayout, TODO: make friend
                    MultiSalLayout( SalLayout& rBaseLayout,
                         const ImplFontData* pBaseFont = NULL );
    virtual bool    AddFallback( SalLayout& rFallbackLayout,
                         ImplLayoutRuns&, const ImplFontData* pFallbackFont );
    virtual bool    LayoutText( ImplLayoutArgs& );
    virtual void    AdjustLayout( ImplLayoutArgs& );
    virtual void    InitFont() const;

    virtual const ImplFontData* GetFallbackFontData( sal_GlyphId ) const;

    void SetInComplete(bool bInComplete = true);

#ifdef USE_JAVA
    SalLayout*		 GetLayout( int nFallbackLevel ) const
    { return mpLayouts[ nFallbackLevel ]; }
#endif	// USE_JAVA

protected:
    virtual         ~MultiSalLayout();

private:
    // dummy implementations
    virtual void    MoveGlyph( int, long ) {}
    virtual void    DropGlyph( int ) {}
    virtual void    Simplify( bool ) {}

    // enforce proper copy semantic
    SAL_DLLPRIVATE  MultiSalLayout( const MultiSalLayout& );
    SAL_DLLPRIVATE  MultiSalLayout& operator=( const MultiSalLayout& );

private:
    SalLayout*      mpLayouts[ MAX_FALLBACK ];
    const ImplFontData* mpFallbackFonts[ MAX_FALLBACK ];
    ImplLayoutRuns  maFallbackRuns[ MAX_FALLBACK ];
    int             mnLevel;
    bool            mbInComplete;
};

// --------------------
// - GenericSalLayout -
// --------------------

struct GlyphItem
{
    int     mnFlags;
    int     mnCharPos;      // index in string
    int     mnOrigWidth;    // original glyph width
    int     mnNewWidth;     // width after adjustments
    sal_GlyphId mnGlyphIndex;
    Point   maLinearPos;    // absolute position of non rotated string

public:
            GlyphItem() {}

            GlyphItem( int nCharPos, sal_GlyphId nGlyphIndex, const Point& rLinearPos,
                long nFlags, int nOrigWidth )
            :   mnFlags(nFlags), mnCharPos(nCharPos),
                mnOrigWidth(nOrigWidth), mnNewWidth(nOrigWidth),
                mnGlyphIndex(nGlyphIndex), maLinearPos(rLinearPos)
            {}

#ifdef USE_JAVA
    enum{ FALLBACK_MASK=0xFF, IS_IN_CLUSTER=0x100, IS_RTL_GLYPH=0x200, IS_KASHIDA_ALLOWED_AFTER_GLYPH=0x400, IS_NONPRINTING_CHAR=0x800 };
#else	// USE_JAVA
    enum{ FALLBACK_MASK=0xFF, IS_IN_CLUSTER=0x100, IS_RTL_GLYPH=0x200 };
#endif	// USE_JAVA

    bool    IsClusterStart() const { return !(mnFlags & IS_IN_CLUSTER); }
    bool    IsRTLGlyph() const { return ((mnFlags & IS_RTL_GLYPH) != 0); }
#ifdef USE_JAVA
    bool    IsKashidaAllowedAfterGlyph() const { return ((mnFlags & IS_KASHIDA_ALLOWED_AFTER_GLYPH) != 0); }
    bool    IsNonprintingChar() const { return ((mnFlags & IS_NONPRINTING_CHAR) != 0); }
#endif	// USE_JAVA
};

// ---------------

typedef std::list<GlyphItem> GlyphList;
typedef std::vector<GlyphItem> GlyphVector;

// ---------------

class VCL_DLLPUBLIC GenericSalLayout : public SalLayout
{
public:
    // used by layout engines
    void            AppendGlyph( const GlyphItem& );
    virtual void    AdjustLayout( ImplLayoutArgs& );
    virtual void    ApplyDXArray( ImplLayoutArgs& );
    virtual void    Justify( long nNewWidth );
    void            KashidaJustify( long nIndex, int nWidth );
    void            ApplyAsianKerning( const sal_Unicode*, int nLength );
    void            SortGlyphItems();

    // used by upper layers
    virtual long    GetTextWidth() const;
    virtual long    FillDXArray( sal_Int32* pDXArray ) const;
    virtual int     GetTextBreak( long nMaxWidth, long nCharExtra, int nFactor ) const;
    virtual void    GetCaretPositions( int nArraySize, sal_Int32* pCaretXArray ) const;

    // used by display layers
    virtual int     GetNextGlyphs( int nLen, sal_GlyphId* pGlyphIdxAry, Point& rPos, int&,
                        sal_Int32* pGlyphAdvAry = NULL, int* pCharPosAry = NULL ) const;

protected:
                    GenericSalLayout();
    virtual         ~GenericSalLayout();

    // for glyph+font+script fallback
    virtual void    MoveGlyph( int nStart, long nNewXPos );
    virtual void    DropGlyph( int nStart );
    virtual void    Simplify( bool bIsBase );

    bool            GetCharWidths( sal_Int32* pCharWidths ) const;
#ifdef USE_JAVA
    void            SetGlyphCapacity( int nGlyphCapacity );
#endif	// USE_JAVA

private:
    GlyphItem*      mpGlyphItems;   // TODO: change to GlyphList
    int             mnGlyphCount;
    int             mnGlyphCapacity;
    mutable Point   maBasePoint;

    // enforce proper copy semantic
    SAL_DLLPRIVATE  GenericSalLayout( const GenericSalLayout& );
    SAL_DLLPRIVATE  GenericSalLayout& operator=( const GenericSalLayout& );
};

#undef SalGraphics

#endif // _SV_SALLAYOUT_HXX
