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
 *   Modified December 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tools/debug.hxx>
#include <tools/lineend.hxx>
#include <tools/poly.hxx>
#include <tools/rc.h>

#include <vcl/image.hxx>
#include <vcl/bitmap.hxx>
#include <vcl/bitmapex.hxx>
#include <vcl/decoview.hxx>
#include <vcl/event.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <vcl/dialog.hxx>
#include <vcl/fixed.hxx>
#include <vcl/button.hxx>
#include <vcl/salnativewidgets.hxx>
#include <vcl/edit.hxx>
#include <vcl/layout.hxx>

#include <svids.hrc>
#include <svdata.hxx>
#include <window.h>
#include <controldata.hxx>

#include <comphelper/processfactory.hxx>

#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/util/URL.hpp>
#include <com/sun/star/util/URLTransformer.hpp>

using namespace css;

#define PUSHBUTTON_VIEW_STYLE       (WB_3DLOOK |                        \
                                     WB_LEFT | WB_CENTER | WB_RIGHT |   \
                                     WB_TOP | WB_VCENTER | WB_BOTTOM |  \
                                     WB_WORDBREAK | WB_NOLABEL |        \
                                     WB_DEFBUTTON | WB_NOLIGHTBORDER |  \
                                     WB_RECTSTYLE | WB_SMALLSTYLE |     \
                                     WB_TOGGLE )
#define RADIOBUTTON_VIEW_STYLE      (WB_3DLOOK |                        \
                                     WB_LEFT | WB_CENTER | WB_RIGHT |   \
                                     WB_TOP | WB_VCENTER | WB_BOTTOM |  \
                                     WB_WORDBREAK | WB_NOLABEL)
#define CHECKBOX_VIEW_STYLE         (WB_3DLOOK |                        \
                                     WB_LEFT | WB_CENTER | WB_RIGHT |   \
                                     WB_TOP | WB_VCENTER | WB_BOTTOM |  \
                                     WB_WORDBREAK | WB_NOLABEL)

class ImplCommonButtonData
{
public:
    Rectangle       maFocusRect;
    long            mnSeparatorX;
    sal_uInt16      mnButtonState;
    bool            mbSmallSymbol;

    Image           maImage;
    ImageAlign      meImageAlign;
    SymbolAlign     meSymbolAlign;

public:
                    ImplCommonButtonData();
                   ~ImplCommonButtonData();
};

ImplCommonButtonData::ImplCommonButtonData() : maFocusRect(), mnSeparatorX(0), mnButtonState(0),
mbSmallSymbol(false), maImage(), meImageAlign(IMAGEALIGN_TOP), meSymbolAlign(SYMBOLALIGN_LEFT)
{
}

ImplCommonButtonData::~ImplCommonButtonData()
{
}

Button::Button( WindowType nType ) :
    Control( nType )
{
    mpButtonData = new ImplCommonButtonData;
}

Button::~Button()
{
    delete mpButtonData;
}

void Button::SetCommandHandler(const OUString& aCommand)
{
    maCommand = aCommand;
    SetClickHdl(Link(NULL, dispatchCommandHandler));
}

void Button::Click()
{
    ImplCallEventListenersAndHandler( VCLEVENT_BUTTON_CLICK, maClickHdl, this );
}

OUString Button::GetStandardText( StandardButtonType eButton )
{
    static struct
    {
        sal_uInt32 nResId;
        const char* pDefText;
    } aResIdAry[BUTTON_COUNT] =
    {
        { SV_BUTTONTEXT_OK, "~OK" },
        { SV_BUTTONTEXT_CANCEL, "~Cancel" },
        { SV_BUTTONTEXT_YES, "~Yes" },
        { SV_BUTTONTEXT_NO, "~No" },
        { SV_BUTTONTEXT_RETRY, "~Retry" },
        { SV_BUTTONTEXT_HELP, "~Help" },
        { SV_BUTTONTEXT_CLOSE, "~Close" },
        { SV_BUTTONTEXT_MORE, "~More" },
        { SV_BUTTONTEXT_IGNORE, "~Ignore" },
        { SV_BUTTONTEXT_ABORT, "~Abort" },
        { SV_BUTTONTEXT_LESS, "~Less" },
        { SV_BUTTONTEXT_RESET, "R~eset" }
    };

    ResMgr* pResMgr = ImplGetResMgr();

    if (!pResMgr)
    {
        OString aT( aResIdAry[(sal_uInt16)eButton].pDefText );
        return OStringToOUString(aT, RTL_TEXTENCODING_ASCII_US);
    }

    sal_uInt32 nResId = aResIdAry[(sal_uInt16)eButton].nResId;
#ifdef WNT
    // http://lists.freedesktop.org/archives/libreoffice/2013-January/044513.html
    // Under windows we don't want accelerators on ok/cancel but do on other
    // buttons
    if (nResId == SV_BUTTONTEXT_OK)
        nResId = SV_BUTTONTEXT_OK_NOMNEMONIC;
    else if (nResId == SV_BUTTONTEXT_CANCEL)
        nResId = SV_BUTTONTEXT_CANCEL_NOMNEMONIC;
#endif
    return ResId(nResId, *pResMgr).toString();
}

bool Button::SetModeImage( const Image& rImage )
{
    if ( rImage != mpButtonData->maImage )
    {
        mpButtonData->maImage = rImage;
        StateChanged( StateChangedType::DATA );
        queue_resize();
    }
    return true;
}

const Image Button::GetModeImage( ) const
{
    return mpButtonData->maImage;
}

bool Button::HasImage() const
{
    return !!(mpButtonData->maImage);
}

void Button::SetImageAlign( ImageAlign eAlign )
{
    if ( mpButtonData->meImageAlign != eAlign )
    {
        mpButtonData->meImageAlign = eAlign;
        StateChanged( StateChangedType::DATA );
    }
}

ImageAlign Button::GetImageAlign() const
{
    return mpButtonData->meImageAlign;
}

void Button::SetFocusRect( const Rectangle& rFocusRect )
{
    ImplSetFocusRect( rFocusRect );
}

long Button::ImplGetSeparatorX() const
{
    return mpButtonData->mnSeparatorX;
}

void Button::ImplSetSeparatorX( long nX )
{
    mpButtonData->mnSeparatorX = nX;
}

sal_uInt16 Button::ImplGetTextStyle( OUString& rText, WinBits nWinStyle,
                                 sal_uLong nDrawFlags )
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
    sal_uInt16 nTextStyle = FixedText::ImplGetTextStyle( nWinStyle & ~WB_DEFBUTTON );

    if ( nDrawFlags & WINDOW_DRAW_NOMNEMONIC )
    {
        if ( nTextStyle & TEXT_DRAW_MNEMONIC )
        {
            rText = GetNonMnemonicString( rText );
            nTextStyle &= ~TEXT_DRAW_MNEMONIC;
        }
    }

    if ( !(nDrawFlags & WINDOW_DRAW_NODISABLE) )
    {
        if ( !IsEnabled() )
#ifdef USE_JAVA
        {
            Color aColor;
            ImplControlValue aValue;

            // assume if we have native control support for pushbuttons we've
            // properly mapped disabled radio button text and other text
            // to proper color as well.
            if ( ! ( IsNativeControlSupported( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL ) && GetNativeControlTextColor( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL, 0, aValue, aColor ) ) )
#endif	// USE_JAVA
            nTextStyle |= TEXT_DRAW_DISABLE;
#ifdef USE_JAVA
       }
#endif	// USE_JAVA
    }

    if ( (nDrawFlags & WINDOW_DRAW_MONO) ||
         (rStyleSettings.GetOptions() & STYLE_OPTION_MONO) )
        nTextStyle |= TEXT_DRAW_MONO;

    return nTextStyle;
}

void Button::ImplDrawAlignedImage( OutputDevice* pDev, Point& rPos,
                                   Size& rSize, bool bLayout,
                                   sal_uLong nImageSep, sal_uLong nDrawFlags,
                                   sal_uInt16 nTextStyle, Rectangle *pSymbolRect,
                                   bool bAddImageSep )
{
    OUString        aText( GetText() );
    bool            bDrawImage = HasImage() && ! ( ImplGetButtonState() & BUTTON_DRAW_NOIMAGE );
    bool            bDrawText  = !aText.isEmpty() && ! ( ImplGetButtonState() & BUTTON_DRAW_NOTEXT );
    bool            bHasSymbol = pSymbolRect ? true : false;

    // No text and no image => nothing to do => return
    if ( !bDrawImage && !bDrawText && !bHasSymbol )
        return;

    WinBits         nWinStyle = GetStyle();
    Rectangle       aOutRect( rPos, rSize );
    MetricVector   *pVector = bLayout ? &mpControlData->mpLayoutData->m_aUnicodeBoundRects : NULL;
    OUString       *pDisplayText = bLayout ? &mpControlData->mpLayoutData->m_aDisplayText : NULL;
    ImageAlign      eImageAlign = mpButtonData->meImageAlign;
    Size            aImageSize = mpButtonData->maImage.GetSizePixel();

    if ( ( nDrawFlags & WINDOW_DRAW_NOMNEMONIC ) &&
         ( nTextStyle & TEXT_DRAW_MNEMONIC ) )
    {
        aText = GetNonMnemonicString( aText );
        nTextStyle &= ~TEXT_DRAW_MNEMONIC;
    }

    aImageSize.Width()  = CalcZoom( aImageSize.Width() );
    aImageSize.Height() = CalcZoom( aImageSize.Height() );

    // Drawing text or symbol only is simple, use style and output rectangle
    if ( bHasSymbol && !bDrawImage && !bDrawText )
    {
        *pSymbolRect = aOutRect;
        return;
    }
    else if ( bDrawText && !bDrawImage && !bHasSymbol )
    {
        DrawControlText( *pDev, aOutRect, aText, nTextStyle, pVector, pDisplayText );

        ImplSetFocusRect( aOutRect );
        rSize = aOutRect.GetSize();
        rPos = aOutRect.TopLeft();

        return;
    }

    // check for HC mode ( image only! )
    Image    *pImage    = &(mpButtonData->maImage);

    Size aTextSize;
    Size aSymbolSize;
    Size aMax;
    Point aImagePos = rPos;
    Point aTextPos = rPos;
    Rectangle aUnion = Rectangle( aImagePos, aImageSize );
    Rectangle aSymbol;
    long nSymbolHeight = 0;

    if ( bDrawText || bHasSymbol )
    {
        // Get the size of the text output area ( the symbol will be drawn in
        // this area as well, so the symbol rectangle will be calculated here, too )

        Rectangle   aRect = Rectangle( Point(), rSize );
        Size        aTSSize;

        if ( bHasSymbol )
        {
            if ( bDrawText )
            {
                nSymbolHeight = pDev->GetTextHeight();
                if ( mpButtonData->mbSmallSymbol )
                    nSymbolHeight = nSymbolHeight * 3 / 4;

                aSymbol = Rectangle( Point(), Size( nSymbolHeight, nSymbolHeight ) );
                ImplCalcSymbolRect( aSymbol );
                aRect.Left() += 3 * nSymbolHeight / 2;
                aTSSize.Width() = 3 * nSymbolHeight / 2;
            }
            else
            {
                aSymbol = Rectangle( Point(), rSize );
                ImplCalcSymbolRect( aSymbol );
                aTSSize.Width() = aSymbol.GetWidth();
            }
            aTSSize.Height() = aSymbol.GetHeight();
            aSymbolSize = aSymbol.GetSize();
        }

        if ( bDrawText )
        {
            if ( ( eImageAlign == IMAGEALIGN_LEFT_TOP     ) ||
                 ( eImageAlign == IMAGEALIGN_LEFT         ) ||
                 ( eImageAlign == IMAGEALIGN_LEFT_BOTTOM  ) ||
                 ( eImageAlign == IMAGEALIGN_RIGHT_TOP    ) ||
                 ( eImageAlign == IMAGEALIGN_RIGHT        ) ||
                 ( eImageAlign == IMAGEALIGN_RIGHT_BOTTOM ) )
            {
                aRect.Right() -= ( aImageSize.Width() + nImageSep );
            }
            else if ( ( eImageAlign == IMAGEALIGN_TOP_LEFT     ) ||
                      ( eImageAlign == IMAGEALIGN_TOP          ) ||
                      ( eImageAlign == IMAGEALIGN_TOP_RIGHT    ) ||
                      ( eImageAlign == IMAGEALIGN_BOTTOM_LEFT  ) ||
                      ( eImageAlign == IMAGEALIGN_BOTTOM       ) ||
                      ( eImageAlign == IMAGEALIGN_BOTTOM_RIGHT ) )
            {
                aRect.Bottom() -= ( aImageSize.Height() + nImageSep );
            }

            aRect = pDev->GetTextRect( aRect, aText, nTextStyle );
            aTextSize = aRect.GetSize();

            aTSSize.Width()  += aTextSize.Width();

            if ( aTSSize.Height() < aTextSize.Height() )
                aTSSize.Height() = aTextSize.Height();

            if( bAddImageSep && bDrawImage )
            {
                long nDiff = (aImageSize.Height() - aTextSize.Height()) / 3;
                if( nDiff > 0 )
                    nImageSep += nDiff;
            }
        }

        aMax.Width()  = aTSSize.Width()  > aImageSize.Width()  ? aTSSize.Width()  : aImageSize.Width();
        aMax.Height() = aTSSize.Height() > aImageSize.Height() ? aTSSize.Height() : aImageSize.Height();

        // Now calculate the output area for the image and the text according to the image align flags

        if ( ( eImageAlign == IMAGEALIGN_LEFT ) ||
             ( eImageAlign == IMAGEALIGN_RIGHT ) )
        {
            aImagePos.Y() = rPos.Y() + ( aMax.Height() - aImageSize.Height() ) / 2;
            aTextPos.Y()  = rPos.Y() + ( aMax.Height() - aTSSize.Height() ) / 2;
        }
        else if ( ( eImageAlign == IMAGEALIGN_LEFT_BOTTOM ) ||
                  ( eImageAlign == IMAGEALIGN_RIGHT_BOTTOM ) )
        {
            aImagePos.Y() = rPos.Y() + aMax.Height() - aImageSize.Height();
            aTextPos.Y()  = rPos.Y() + aMax.Height() - aTSSize.Height();
        }
        else if ( ( eImageAlign == IMAGEALIGN_TOP ) ||
                  ( eImageAlign == IMAGEALIGN_BOTTOM ) )
        {
            aImagePos.X() = rPos.X() + ( aMax.Width() - aImageSize.Width() ) / 2;
            aTextPos.X()  = rPos.X() + ( aMax.Width() - aTSSize.Width() ) / 2;
        }
        else if ( ( eImageAlign == IMAGEALIGN_TOP_RIGHT ) ||
                  ( eImageAlign == IMAGEALIGN_BOTTOM_RIGHT ) )
        {
            aImagePos.X() = rPos.X() + aMax.Width() - aImageSize.Width();
            aTextPos.X()  = rPos.X() + aMax.Width() - aTSSize.Width();
        }

        if ( ( eImageAlign == IMAGEALIGN_LEFT_TOP ) ||
             ( eImageAlign == IMAGEALIGN_LEFT ) ||
             ( eImageAlign == IMAGEALIGN_LEFT_BOTTOM ) )
        {
            aTextPos.X() = rPos.X() + aImageSize.Width() + nImageSep;
        }
        else if ( ( eImageAlign == IMAGEALIGN_RIGHT_TOP ) ||
                  ( eImageAlign == IMAGEALIGN_RIGHT ) ||
                  ( eImageAlign == IMAGEALIGN_RIGHT_BOTTOM ) )
        {
            aImagePos.X() = rPos.X() + aTSSize.Width() + nImageSep;
        }
        else if ( ( eImageAlign == IMAGEALIGN_TOP_LEFT ) ||
                  ( eImageAlign == IMAGEALIGN_TOP ) ||
                  ( eImageAlign == IMAGEALIGN_TOP_RIGHT ) )
        {
            aTextPos.Y() = rPos.Y() + aImageSize.Height() + nImageSep;
        }
        else if ( ( eImageAlign == IMAGEALIGN_BOTTOM_LEFT ) ||
                  ( eImageAlign == IMAGEALIGN_BOTTOM ) ||
                  ( eImageAlign == IMAGEALIGN_BOTTOM_RIGHT ) )
        {
            aImagePos.Y() = rPos.Y() + aTSSize.Height() + nImageSep;
        }
        else if ( eImageAlign == IMAGEALIGN_CENTER )
        {
            aImagePos.X() = rPos.X() + ( aMax.Width()  - aImageSize.Width() ) / 2;
            aImagePos.Y() = rPos.Y() + ( aMax.Height() - aImageSize.Height() ) / 2;
            aTextPos.X()  = rPos.X() + ( aMax.Width()  - aTSSize.Width() ) / 2;
            aTextPos.Y()  = rPos.Y() + ( aMax.Height() - aTSSize.Height() ) / 2;
        }
        aUnion = Rectangle( aImagePos, aImageSize );
        aUnion.Union( Rectangle( aTextPos, aTSSize ) );
    }

    // Now place the combination of text and image in the output area of the button
    // according to the window style (WinBits)
    long nXOffset = 0;
    long nYOffset = 0;

    if ( nWinStyle & WB_CENTER )
    {
        nXOffset = ( rSize.Width() - aUnion.GetWidth() ) / 2;
    }
    else if ( nWinStyle & WB_RIGHT )
    {
        nXOffset = rSize.Width() - aUnion.GetWidth();
    }

    if ( nWinStyle & WB_VCENTER )
    {
        nYOffset = ( rSize.Height() - aUnion.GetHeight() ) / 2;
    }
    else if ( nWinStyle & WB_BOTTOM )
    {
        nYOffset = rSize.Height() - aUnion.GetHeight();
    }

    // the top left corner should always be visible, so we don't allow negative offsets
    if ( nXOffset < 0 ) nXOffset = 0;
    if ( nYOffset < 0 ) nYOffset = 0;

    aImagePos.X() += nXOffset;
    aImagePos.Y() += nYOffset;
    aTextPos.X() += nXOffset;
    aTextPos.Y() += nYOffset;

    // set rPos and rSize to the union
    rSize = aUnion.GetSize();
    rPos.X() += nXOffset;
    rPos.Y() += nYOffset;

    if ( bHasSymbol )
    {
        if ( mpButtonData->meSymbolAlign == SYMBOLALIGN_RIGHT )
        {
            Point aRightPos = Point( aTextPos.X() + aTextSize.Width() + aSymbolSize.Width()/2, aTextPos.Y() );
            *pSymbolRect = Rectangle( aRightPos, aSymbolSize );
        }
        else
        {
            *pSymbolRect = Rectangle( aTextPos, aSymbolSize );
            aTextPos.X() += ( 3 * nSymbolHeight / 2 );
        }
        if ( mpButtonData->mbSmallSymbol )
        {
            nYOffset = (aUnion.GetHeight() - aSymbolSize.Height())/2;
            pSymbolRect->setY( aTextPos.Y() + nYOffset );
        }
    }

    sal_uInt16 nStyle = 0;

    if ( ! ( nDrawFlags & WINDOW_DRAW_NODISABLE ) &&
         ! IsEnabled() )
#ifdef USE_JAVA
        nStyle |= IMAGE_DRAW_SEMITRANSPARENT;
#else	// USE_JAVA
        nStyle |= IMAGE_DRAW_DISABLE;
#endif	// USE_JAVA

    if ( IsZoom() )
        pDev->DrawImage( aImagePos, aImageSize, *pImage, nStyle );
    else
        pDev->DrawImage( aImagePos, *pImage, nStyle );

    if ( bDrawText )
    {
        ImplSetFocusRect( Rectangle( aTextPos, aTextSize ) );
        pDev->DrawText( Rectangle( aTextPos, aTextSize ), aText, nTextStyle, pVector, pDisplayText );
    }
    else
    {
        ImplSetFocusRect( Rectangle( aImagePos, aImageSize ) );
    }
}

void Button::ImplSetFocusRect( const Rectangle &rFocusRect )
{
    Rectangle aFocusRect = rFocusRect;
    Rectangle aOutputRect = Rectangle( Point(), GetOutputSizePixel() );

    if ( ! aFocusRect.IsEmpty() )
    {
        aFocusRect.Left()--;
        aFocusRect.Top()--;
        aFocusRect.Right()++;
        aFocusRect.Bottom()++;
    }

    if ( aFocusRect.Left()   < aOutputRect.Left()   ) aFocusRect.Left()   = aOutputRect.Left();
    if ( aFocusRect.Top()    < aOutputRect.Top()    ) aFocusRect.Top()    = aOutputRect.Top();
    if ( aFocusRect.Right()  > aOutputRect.Right()  ) aFocusRect.Right()  = aOutputRect.Right();
    if ( aFocusRect.Bottom() > aOutputRect.Bottom() ) aFocusRect.Bottom() = aOutputRect.Bottom();

    mpButtonData->maFocusRect = aFocusRect;
}

const Rectangle& Button::ImplGetFocusRect() const
{
    return mpButtonData->maFocusRect;
}

sal_uInt16& Button::ImplGetButtonState()
{
    return mpButtonData->mnButtonState;
}

sal_uInt16 Button::ImplGetButtonState() const
{
    return mpButtonData->mnButtonState;
}

void Button::ImplSetSymbolAlign( SymbolAlign eAlign )
{
    if ( mpButtonData->meSymbolAlign != eAlign )
    {
        mpButtonData->meSymbolAlign = eAlign;
        StateChanged( StateChangedType::DATA );
    }
}

void Button::SetSmallSymbol(bool bSmall)
{
    mpButtonData->mbSmallSymbol = bSmall;
}

void Button::EnableImageDisplay( bool bEnable )
{
    if( bEnable )
        mpButtonData->mnButtonState &= ~BUTTON_DRAW_NOIMAGE;
    else
        mpButtonData->mnButtonState |= BUTTON_DRAW_NOIMAGE;
}

void Button::EnableTextDisplay( bool bEnable )
{
    if( bEnable )
        mpButtonData->mnButtonState &= ~BUTTON_DRAW_NOTEXT;
    else
        mpButtonData->mnButtonState |= BUTTON_DRAW_NOTEXT;
}

bool Button::IsSmallSymbol () const
{
    return mpButtonData->mbSmallSymbol;
}

bool Button::set_property(const OString &rKey, const OString &rValue)
{
    if (rKey == "image-position")
    {
        ImageAlign eAlign = IMAGEALIGN_LEFT;
        if (rValue == "left")
            eAlign = IMAGEALIGN_LEFT;
        else if (rValue == "right")
            eAlign = IMAGEALIGN_RIGHT;
        else if (rValue == "top")
            eAlign = IMAGEALIGN_TOP;
        else if (rValue == "bottom")
            eAlign = IMAGEALIGN_BOTTOM;
        SetImageAlign(eAlign);
    }
    else
        return Control::set_property(rKey, rValue);
    return true;
}

long Button::dispatchCommandHandler(void *, void *pCaller)
{
    const Button *pButton = reinterpret_cast<Button*>(pCaller);
    if (pButton == NULL)
        return 0;

    // Target where we will execute the .uno: command
    uno::Reference<uno::XComponentContext> xContext = ::comphelper::getProcessComponentContext();
    uno::Reference<frame::XDesktop2> xDesktop = frame::Desktop::create(xContext);

    uno::Reference<frame::XFrame> xFrame(xDesktop->getActiveFrame());
    if (!xFrame.is())
        xFrame = uno::Reference<frame::XFrame>(xDesktop, uno::UNO_QUERY);

    uno::Reference<frame::XDispatchProvider> xDispatchProvider(xFrame, uno::UNO_QUERY);
    if (!xDispatchProvider.is())
        return 0;

    util::URL aCommandURL;
    aCommandURL.Complete = pButton->maCommand;
    uno::Reference<util::XURLTransformer> xParser = util::URLTransformer::create(xContext);
    xParser->parseStrict(aCommandURL);

    uno::Reference<frame::XDispatch> xDisp = xDispatchProvider->queryDispatch(aCommandURL, OUString(), 0);
    if (!xDisp.is())
        return 0;

    // And do the work...
    xDisp->dispatch(aCommandURL, uno::Sequence<beans::PropertyValue>());

    return 1;
}



void PushButton::ImplInitPushButtonData()
{
    mpWindowImpl->mbPushButton    = true;

    meSymbol        = SymbolType::DONTKNOW;
    meState         = TRISTATE_FALSE;
    meSaveValue     = TRISTATE_FALSE;
    mnDDStyle       = 0;
    mbPressed       = false;
    mbInUserDraw    = false;
}

namespace
{
    vcl::Window* getPreviousSibling(vcl::Window *pParent)
    {
        return pParent ? pParent->GetWindow(WINDOW_LASTCHILD) : NULL;
    }
}

void PushButton::ImplInit( vcl::Window* pParent, WinBits nStyle )
{
    nStyle = ImplInitStyle(getPreviousSibling(pParent), nStyle);
    Button::ImplInit( pParent, nStyle, NULL );

    if ( nStyle & WB_NOLIGHTBORDER )
        ImplGetButtonState() |= BUTTON_DRAW_NOLIGHTBORDER;

    ImplInitSettings( true, true, true );
}

WinBits PushButton::ImplInitStyle( const vcl::Window* pPrevWindow, WinBits nStyle )
{
    if ( !(nStyle & WB_NOTABSTOP) )
        nStyle |= WB_TABSTOP;

    // if no alignment is given, default to "vertically centered". This is because since
    // #i26046#, we respect the vertical alignment flags (previously we didn't completely),
    // but we of course want to look as before when no vertical alignment is specified
    if ( ( nStyle & ( WB_TOP | WB_VCENTER | WB_BOTTOM ) ) == 0 )
        nStyle |= WB_VCENTER;

    if ( !(nStyle & WB_NOGROUP) &&
         (!pPrevWindow ||
          ((pPrevWindow->GetType() != WINDOW_PUSHBUTTON  ) &&
           (pPrevWindow->GetType() != WINDOW_OKBUTTON    ) &&
           (pPrevWindow->GetType() != WINDOW_CANCELBUTTON) &&
           (pPrevWindow->GetType() != WINDOW_HELPBUTTON  )) ) )
        nStyle |= WB_GROUP;
    return nStyle;
}

const vcl::Font& PushButton::GetCanonicalFont( const StyleSettings& _rStyle ) const
{
    return _rStyle.GetPushButtonFont();
}

const Color& PushButton::GetCanonicalTextColor( const StyleSettings& _rStyle ) const
{
    return _rStyle.GetButtonTextColor();
}

void PushButton::ImplInitSettings( bool bFont,
                                   bool bForeground, bool bBackground )
{
    Button::ImplInitSettings( bFont, bForeground );

    if ( bBackground )
    {
        SetBackground();
        // #i38498#: do not check for GetParent()->IsChildTransparentModeEnabled()
        // otherwise the formcontrol button will be overdrawn due to PARENTCLIPMODE_NOCLIP
        // for radio and checkbox this is ok as they should appear transparent in documents
        if ( IsNativeControlSupported( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL ) ||
             (GetStyle() & WB_FLATBUTTON) != 0 )
        {
            EnableChildTransparentMode( true );
            SetParentClipMode( PARENTCLIPMODE_NOCLIP );
            SetPaintTransparent( true );
            mpWindowImpl->mbUseNativeFocus = (GetStyle() & WB_FLATBUTTON)
                ? false
                : ImplGetSVData()->maNWFData.mbNoFocusRects;
        }
        else
        {
            EnableChildTransparentMode( false );
            SetParentClipMode( 0 );
            SetPaintTransparent( false );
        }
    }
}

void PushButton::ImplDrawPushButtonFrame( vcl::Window* pDev,
                                          Rectangle& rRect, sal_uInt16 nStyle )
{
    if ( !(pDev->GetStyle() & (WB_RECTSTYLE | WB_SMALLSTYLE)) )
    {
        StyleSettings aStyleSettings = pDev->GetSettings().GetStyleSettings();
        if ( pDev->IsControlBackground() )
            aStyleSettings.Set3DColors( pDev->GetControlBackground() );
    }

    DecorationView aDecoView( pDev );
    if ( pDev->IsControlBackground() )
    {
        AllSettings     aSettings = pDev->GetSettings();
        AllSettings     aOldSettings = aSettings;
        StyleSettings   aStyleSettings = aSettings.GetStyleSettings();
        aStyleSettings.Set3DColors( pDev->GetControlBackground() );
        aSettings.SetStyleSettings( aStyleSettings );
        pDev->OutputDevice::SetSettings( aSettings );
        rRect = aDecoView.DrawButton( rRect, nStyle );
        pDev->OutputDevice::SetSettings( aOldSettings );
    }
    else
        rRect = aDecoView.DrawButton( rRect, nStyle );
}

bool PushButton::ImplHitTestPushButton( vcl::Window* pDev,
                                        const Point& rPos )
{
    Point       aTempPoint;
    Rectangle   aTestRect( aTempPoint, pDev->GetOutputSizePixel() );

    return aTestRect.IsInside( rPos );
}

sal_uInt16 PushButton::ImplGetTextStyle( sal_uLong nDrawFlags ) const
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();

    sal_uInt16 nTextStyle = TEXT_DRAW_MNEMONIC | TEXT_DRAW_MULTILINE | TEXT_DRAW_ENDELLIPSIS;

    if ( ( rStyleSettings.GetOptions() & STYLE_OPTION_MONO ) ||
         ( nDrawFlags & WINDOW_DRAW_MONO ) )
        nTextStyle |= TEXT_DRAW_MONO;

    if ( GetStyle() & WB_WORDBREAK )
        nTextStyle |= TEXT_DRAW_WORDBREAK;
    if ( GetStyle() & WB_NOLABEL )
        nTextStyle &= ~TEXT_DRAW_MNEMONIC;

    if ( GetStyle() & WB_LEFT )
        nTextStyle |= TEXT_DRAW_LEFT;
    else if ( GetStyle() & WB_RIGHT )
        nTextStyle |= TEXT_DRAW_RIGHT;
    else
        nTextStyle |= TEXT_DRAW_CENTER;

    if ( GetStyle() & WB_TOP )
        nTextStyle |= TEXT_DRAW_TOP;
    else if ( GetStyle() & WB_BOTTOM )
        nTextStyle |= TEXT_DRAW_BOTTOM;
    else
        nTextStyle |= TEXT_DRAW_VCENTER;

    if ( ! ( (nDrawFlags & WINDOW_DRAW_NODISABLE) || IsEnabled() ) )
#ifdef USE_JAVA
    {
        Color aColor;
        ImplControlValue aValue;
        PushButton *nonConstButton = (PushButton *)this;

        if ( ! ( nonConstButton->IsNativeControlSupported( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL ) && nonConstButton->GetNativeControlTextColor( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL, 0, aValue, aColor ) ) )
#endif	// USE_JAVA
        nTextStyle |= TEXT_DRAW_DISABLE;
#ifdef USE_JAVA
    }
#endif	// USE_JAVA

    return nTextStyle;
}

static void ImplDrawBtnDropDownArrow( OutputDevice* pDev,
                                      long nX, long nY,
                                      Color& rColor, bool bBlack )
{
    Color aOldLineColor = pDev->GetLineColor();
    Color aOldFillColor = pDev->GetFillColor();

    pDev->SetLineColor();
    if ( bBlack )
        pDev->SetFillColor( Color( COL_BLACK ) );
    else
        pDev->SetFillColor( rColor );
    pDev->DrawRect( Rectangle( nX+0, nY+0, nX+6, nY+0 ) );
    pDev->DrawRect( Rectangle( nX+1, nY+1, nX+5, nY+1 ) );
    pDev->DrawRect( Rectangle( nX+2, nY+2, nX+4, nY+2 ) );
    pDev->DrawRect( Rectangle( nX+3, nY+3, nX+3, nY+3 ) );
    if ( bBlack )
    {
        pDev->SetFillColor( rColor );
        pDev->DrawRect( Rectangle( nX+2, nY+1, nX+4, nY+1 ) );
        pDev->DrawRect( Rectangle( nX+3, nY+2, nX+3, nY+2 ) );
    }
    pDev->SetLineColor( aOldLineColor );
    pDev->SetFillColor( aOldFillColor );
}

void PushButton::ImplDrawPushButtonContent( OutputDevice* pDev, sal_uLong nDrawFlags,
                                            const Rectangle& rRect,
                                            bool bLayout,
                                            bool bMenuBtnSep
                                            )
{
    const StyleSettings&    rStyleSettings = GetSettings().GetStyleSettings();
    Rectangle               aInRect = rRect;
    Color                   aColor;
    OUString                aText = PushButton::GetText(); // PushButton:: because of MoreButton
    sal_uInt16              nTextStyle = ImplGetTextStyle( nDrawFlags );
    sal_uInt16              nStyle;

    if( aInRect.Right() < aInRect.Left() || aInRect.Bottom() < aInRect.Top() )
        aInRect.SetEmpty();

    pDev->Push( PushFlags::CLIPREGION );
    pDev->IntersectClipRegion( aInRect );

#ifdef USE_JAVA
    bool bNativeOK = false;
    bool bDropDown = ( IsSymbol() && GetSymbol() == SymbolType::SPIN_DOWN && !GetText().getLength() );
    ControlType aCtrlType = 0;
    switch( GetParent()->GetType() )
    {
        case WINDOW_LISTBOX:
        case WINDOW_MULTILISTBOX:
        case WINDOW_TREELISTBOX:
            aCtrlType = CTRL_LISTBOX;
            break;
        case WINDOW_COMBOBOX:
        case WINDOW_PATTERNBOX:
        case WINDOW_NUMERICBOX:
        case WINDOW_METRICBOX:
        case WINDOW_CURRENCYBOX:
        case WINDOW_DATEBOX:
        case WINDOW_TIMEBOX:
        case WINDOW_LONGCURRENCYBOX:
            aCtrlType = CTRL_COMBOBOX;
            break;
        default:
            break;
    }
 
    if( bDropDown && (aCtrlType == CTRL_COMBOBOX || aCtrlType == CTRL_LISTBOX ) )
    {
        // let the theme draw it, note we then need support
        // for CTRL_LISTBOX/PART_BUTTON_DOWN and CTRL_COMBOBOX/PART_BUTTON_DOWN
        ImplControlValue aControlValue;
        vcl::Region aCtrlRegion( aInRect );
        ControlState nState = 0;

        if ( mbPressed )
            nState |= CTRL_STATE_PRESSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
            nState |= CTRL_STATE_PRESSED;
        if ( HasFocus() )
            nState |= CTRL_STATE_FOCUSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )
            nState |= CTRL_STATE_DEFAULT;
        if ( Window::IsEnabled() )
            nState |= CTRL_STATE_ENABLED;
        if ( IsMouseOver() && aInRect.IsInside( GetPointerPosPixel() ) )
            nState |= CTRL_STATE_ROLLOVER;

        bNativeOK = GetNativeControlTextColor( aCtrlType, PART_BUTTON_DOWN, nState, aControlValue, aColor );
    }
    else
    {
        ImplControlValue aControlValue;
        vcl::Region aCtrlRegion( aInRect );
        ControlState nState = 0;

        if ( mbPressed || IsChecked() )
            nState |= CTRL_STATE_PRESSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
            nState |= CTRL_STATE_PRESSED;
        if ( HasFocus() )
            nState |= CTRL_STATE_FOCUSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )
            nState |= CTRL_STATE_DEFAULT;
        if ( Window::IsEnabled() )
            nState |= CTRL_STATE_ENABLED;
        if ( IsMouseOver() && aInRect.IsInside( GetPointerPosPixel() ) )
            nState |= CTRL_STATE_ROLLOVER;

        bNativeOK = GetNativeControlTextColor( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL, nState, aControlValue, aColor );
    }
#endif	// USE_JAVA

#ifdef USE_JAVA
    // Fix bug 2534 without causing bug 2208 by not overriding the native text
    // color if there is text
    if ( !bNativeOK || !GetText().getLength() )
    {
#endif	// USE_JAVA
    if ( nDrawFlags & WINDOW_DRAW_MONO )
        aColor = Color( COL_BLACK );
    else if( (nDrawFlags & WINDOW_DRAW_ROLLOVER) && IsNativeControlSupported(CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL) )
        aColor = rStyleSettings.GetButtonRolloverTextColor();
    else if ( IsControlForeground() )
        aColor = GetControlForeground();
    else if( nDrawFlags & WINDOW_DRAW_ROLLOVER )
        aColor = rStyleSettings.GetButtonRolloverTextColor();
    else
        aColor = rStyleSettings.GetButtonTextColor();
#ifdef USE_JAVA
    }
#endif	// USE_JAVA

    pDev->SetTextColor( aColor );

    if ( IsEnabled() || (nDrawFlags & WINDOW_DRAW_NODISABLE) )
        nStyle = 0;
    else
        nStyle = SYMBOL_DRAW_DISABLE;

    Size aSize = rRect.GetSize();
    Point aPos = rRect.TopLeft();

    sal_uLong nImageSep = 1 + (pDev->GetTextHeight()-10)/2;
    if( nImageSep < 1 )
        nImageSep = 1;
    if ( mnDDStyle == PUSHBUTTON_DROPDOWN_MENUBUTTON )
    {
        long nSeparatorX = 0;
        Rectangle aSymbolRect = aInRect;
        if ( !aText.isEmpty() && ! (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) )
        {
            // calculate symbol size
            long nSymbolSize    = pDev->GetTextHeight() / 2 + 1;

            nSeparatorX = aInRect.Right() - 2*nSymbolSize;
            aSize.Width() -= 2*nSymbolSize;

            // center symbol rectangle in the separated area
            aSymbolRect.Right() -= nSymbolSize/2;
            aSymbolRect.Left()  = aSymbolRect.Right() - nSymbolSize;

            ImplDrawAlignedImage( pDev, aPos, aSize, bLayout, nImageSep,
                                  nDrawFlags, nTextStyle, NULL, true );
        }
        else
            ImplCalcSymbolRect( aSymbolRect );

        if( ! bLayout )
        {
            long nDistance = (aSymbolRect.GetHeight() > 10) ? 2 : 1;
            DecorationView aDecoView( pDev );
            if( bMenuBtnSep && nSeparatorX > 0 )
            {
                Point aStartPt( nSeparatorX, aSymbolRect.Top()+nDistance );
                Point aEndPt( nSeparatorX, aSymbolRect.Bottom()-nDistance );
                aDecoView.DrawSeparator( aStartPt, aEndPt );
            }
            ImplSetSeparatorX( nSeparatorX );

            aDecoView.DrawSymbol( aSymbolRect, SymbolType::SPIN_DOWN, aColor, nStyle );
        }

    }
    else
    {
        Rectangle aSymbolRect;
        ImplDrawAlignedImage( pDev, aPos, aSize, bLayout, nImageSep, nDrawFlags,
                              nTextStyle, IsSymbol() ? &aSymbolRect : NULL, true );

        if ( IsSymbol() && ! bLayout )
        {
            DecorationView aDecoView( pDev );
            aDecoView.DrawSymbol( aSymbolRect, meSymbol, aColor, nStyle );
        }

        if ( mnDDStyle == PUSHBUTTON_DROPDOWN_TOOLBOX && !bLayout )
        {
            bool bBlack = false;
            Color   aArrowColor( COL_BLACK );

            if ( !(nDrawFlags & WINDOW_DRAW_MONO) )
            {
                if ( !IsEnabled() )
                    aArrowColor = rStyleSettings.GetShadowColor();
                else
                {
                    aArrowColor = Color( COL_LIGHTGREEN );
                    bBlack = true;
                }
            }

            ImplDrawBtnDropDownArrow( pDev, aInRect.Right()-6, aInRect.Top()+1,
                                      aArrowColor, bBlack );
        }
    }

    UserDrawEvent aUDEvt( this, aInRect, 0 );
    UserDraw( aUDEvt );

    pDev->Pop();  // restore clipregion
}

void PushButton::UserDraw( const UserDrawEvent& )
{
}

void PushButton::ImplDrawPushButton( bool bLayout )
{
    if( !bLayout )
        HideFocus();

    sal_uInt16                  nButtonStyle = ImplGetButtonState();
    Point                   aPoint;
    Size                    aOutSz( GetOutputSizePixel() );
    Rectangle               aRect( aPoint, aOutSz );
    Rectangle               aInRect = aRect;
    bool                    bNativeOK = false;

    // adjust style if button should be rendered 'pressed'
    if ( mbPressed )
        nButtonStyle |= BUTTON_DRAW_PRESSED;

    // TODO: move this to Window class or make it a member !!!
    ControlType aCtrlType = 0;
    switch( GetParent()->GetType() )
    {
        case WINDOW_LISTBOX:
        case WINDOW_MULTILISTBOX:
        case WINDOW_TREELISTBOX:
            aCtrlType = CTRL_LISTBOX;
            break;

        case WINDOW_COMBOBOX:
        case WINDOW_PATTERNBOX:
        case WINDOW_NUMERICBOX:
        case WINDOW_METRICBOX:
        case WINDOW_CURRENCYBOX:
        case WINDOW_DATEBOX:
        case WINDOW_TIMEBOX:
        case WINDOW_LONGCURRENCYBOX:
            aCtrlType = CTRL_COMBOBOX;
            break;
        default:
            break;
    }

    bool bDropDown = ( IsSymbol() && (GetSymbol()==SymbolType::SPIN_DOWN) && GetText().isEmpty() );

    if( bDropDown && (aCtrlType == CTRL_COMBOBOX || aCtrlType == CTRL_LISTBOX ) )
    {
        if( GetParent()->IsNativeControlSupported( aCtrlType, PART_ENTIRE_CONTROL) )
        {
            // skip painting if the button was already drawn by the theme
            if( aCtrlType == CTRL_COMBOBOX )
            {
                Edit* pEdit = static_cast<Edit*>(GetParent());
                if( pEdit->ImplUseNativeBorder( pEdit->GetStyle() ) )
                    bNativeOK = true;
            }
            else if( GetParent()->IsNativeControlSupported( aCtrlType, HAS_BACKGROUND_TEXTURE) )
            {
                bNativeOK = true;
            }
            if( !bNativeOK && GetParent()->IsNativeControlSupported( aCtrlType, PART_BUTTON_DOWN ) )
            {
                // let the theme draw it, note we then need support
                // for CTRL_LISTBOX/PART_BUTTON_DOWN and CTRL_COMBOBOX/PART_BUTTON_DOWN

                ImplControlValue    aControlValue;
                ControlState        nState = 0;

                if ( mbPressed )                        nState |= CTRL_STATE_PRESSED;
                if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )   nState |= CTRL_STATE_PRESSED;
                if ( HasFocus() )                       nState |= CTRL_STATE_FOCUSED;
                if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )   nState |= CTRL_STATE_DEFAULT;
                if ( Window::IsEnabled() )              nState |= CTRL_STATE_ENABLED;

                if ( IsMouseOver() && aInRect.IsInside( GetPointerPosPixel() ) )
                    nState |= CTRL_STATE_ROLLOVER;

                bNativeOK = DrawNativeControl( aCtrlType, PART_BUTTON_DOWN, aInRect, nState,
                                                aControlValue, OUString() );
            }
        }
    }

    if( bNativeOK )
        return;

    bool bRollOver = (IsMouseOver() && aInRect.IsInside( GetPointerPosPixel() ));
    bool bDrawMenuSep = true;
    if( (GetStyle() & WB_FLATBUTTON) )
    {
        if( ! bRollOver && ! HasFocus() )
            bDrawMenuSep = false;
    }
    if ( (bNativeOK=IsNativeControlSupported(CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL)) )
    {
        PushButtonValue aControlValue;
        Rectangle        aCtrlRegion( aInRect );
        ControlState     nState = 0;

        if ( mbPressed || IsChecked() )                   nState |= CTRL_STATE_PRESSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED ) nState |= CTRL_STATE_PRESSED;
        if ( HasFocus() )                       nState |= CTRL_STATE_FOCUSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )   nState |= CTRL_STATE_DEFAULT;
        if ( Window::IsEnabled() )              nState |= CTRL_STATE_ENABLED;

        if ( bRollOver )
            nState |= CTRL_STATE_ROLLOVER;

        if( GetStyle() & WB_BEVELBUTTON )
            aControlValue.mbBevelButton = true;

        // draw frame into invisible window to have aInRect modified correctly
        // but do not shift the inner rect for pressed buttons (ie remove BUTTON_DRAW_PRESSED)
        // this assumes the theme has enough visual cues to signalize the button was pressed
        //Window aWin( this );
        //ImplDrawPushButtonFrame( &aWin, aInRect, nButtonStyle & ~BUTTON_DRAW_PRESSED );

        // looks better this way as symbols were displaced slightly using the above approach
        aInRect.Top()+=4;
        aInRect.Bottom()-=4;
        aInRect.Left()+=4;
        aInRect.Right()-=4;

        // prepare single line hint (needed on mac to decide between normal push button and
        // rectangular bevel button look)
        Size aFontSize( Application::GetSettings().GetStyleSettings().GetPushButtonFont().GetSize() );
        aFontSize = LogicToPixel( aFontSize, MapMode( MAP_POINT ) );
        Size aInRectSize( LogicToPixel( Size( aInRect.GetWidth(), aInRect.GetHeight() ) ) );
        aControlValue.mbSingleLine = (aInRectSize.Height() < 2 * aFontSize.Height() );

        if( ((nState & CTRL_STATE_ROLLOVER)) || ! (GetStyle() & WB_FLATBUTTON) )
        {
            bNativeOK = DrawNativeControl( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL, aCtrlRegion, nState,
                            aControlValue, OUString()/*PushButton::GetText()*/ );
        }
        else
        {
            bNativeOK = true;
        }

        // draw content using the same aInRect as non-native VCL would do
        ImplDrawPushButtonContent( this,
                                   (nState&CTRL_STATE_ROLLOVER) ? WINDOW_DRAW_ROLLOVER : 0,
                                   aInRect, bLayout, bDrawMenuSep );

        if ( HasFocus() )
            ShowFocus( ImplGetFocusRect() );
    }

    if ( !bNativeOK )
    {
        // draw PushButtonFrame, aInRect has content size afterwards
        if( (GetStyle() & WB_FLATBUTTON) )
        {
            Rectangle aTempRect( aInRect );
            if( ! bLayout && bRollOver )
                ImplDrawPushButtonFrame( this, aTempRect, nButtonStyle );
            aInRect.Left()   += 2;
            aInRect.Top()    += 2;
            aInRect.Right()  -= 2;
            aInRect.Bottom() -= 2;
        }
        else
        {
            if( ! bLayout )
                ImplDrawPushButtonFrame( this, aInRect, nButtonStyle );
        }

        // draw content
        ImplDrawPushButtonContent( this, 0, aInRect, bLayout, bDrawMenuSep );

        if( ! bLayout && HasFocus() )
        {
            ShowFocus( ImplGetFocusRect() );
        }
    }
}

void PushButton::ImplSetDefButton( bool bSet )
{
    Size aSize( GetSizePixel() );
    Point aPos( GetPosPixel() );
    int dLeft(0), dRight(0), dTop(0), dBottom(0);
    bool bSetPos = false;

    if ( IsNativeControlSupported(CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL) )
    {
        Rectangle aBound, aCont;
        Rectangle aCtrlRect( 0, 0, 80, 20 ); // use a constant size to avoid accumulating
                                             // will not work if the theme has dynamic adornment sizes
        ImplControlValue aControlValue;
        Rectangle        aCtrlRegion( aCtrlRect );
        ControlState     nState = CTRL_STATE_DEFAULT|CTRL_STATE_ENABLED;

        // get native size of a 'default' button
        // and adjust the VCL button if more space for adornment is required
        if( GetNativeControlRegion( CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL, aCtrlRegion,
                                nState, aControlValue, OUString(),
                                aBound, aCont ) )
        {
            dLeft = aCont.Left() - aBound.Left();
            dTop = aCont.Top() - aBound.Top();
            dRight = aBound.Right() - aCont.Right();
            dBottom = aBound.Bottom() - aCont.Bottom();
            bSetPos = dLeft || dTop || dRight || dBottom;
        }
    }

    if ( bSet )
    {
        if( !(ImplGetButtonState() & BUTTON_DRAW_DEFAULT) && bSetPos )
        {
            // adjust pos/size when toggling from non-default to default
            aPos.Move(-dLeft, -dTop);
            aSize.Width() += dLeft + dRight;
            aSize.Height() += dTop + dBottom;
        }
        ImplGetButtonState() |= BUTTON_DRAW_DEFAULT;
    }
    else
    {
        if( (ImplGetButtonState() & BUTTON_DRAW_DEFAULT) && bSetPos )
        {
            // adjust pos/size when toggling from default to non-default
            aPos.Move(dLeft, dTop);
            aSize.Width() -= dLeft + dRight;
            aSize.Height() -= dTop + dBottom;
        }
        ImplGetButtonState() &= ~BUTTON_DRAW_DEFAULT;
    }
    if( bSetPos )
        setPosSizePixel( aPos.X(), aPos.Y(), aSize.Width(), aSize.Height(), WINDOW_POSSIZE_ALL );

    Invalidate();
}

bool PushButton::ImplIsDefButton() const
{
    return (ImplGetButtonState() & BUTTON_DRAW_DEFAULT) != 0;
}

PushButton::PushButton( WindowType nType ) :
    Button( nType )
{
    ImplInitPushButtonData();
}

PushButton::PushButton( vcl::Window* pParent, WinBits nStyle ) :
    Button( WINDOW_PUSHBUTTON )
{
    ImplInitPushButtonData();
    ImplInit( pParent, nStyle );
}

PushButton::PushButton( vcl::Window* pParent, const ResId& rResId ) :
    Button( WINDOW_PUSHBUTTON )
{
    rResId.SetRT( RSC_PUSHBUTTON );
    WinBits nStyle = ImplInitRes( rResId );
    ImplInitPushButtonData();
    ImplInit( pParent, nStyle );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE) )
        Show();
}

PushButton::~PushButton()
{
}

void PushButton::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( rMEvt.IsLeft() &&
         ImplHitTestPushButton( this, rMEvt.GetPosPixel() ) )
    {
        sal_uInt16 nTrackFlags = 0;

        if ( ( GetStyle() & WB_REPEAT ) &&
             ! ( GetStyle() & WB_TOGGLE ) )
            nTrackFlags |= STARTTRACK_BUTTONREPEAT;

        ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
        ImplDrawPushButton();
        StartTracking( nTrackFlags );

        if ( nTrackFlags & STARTTRACK_BUTTONREPEAT )
            Click();
    }
}

void PushButton::Tracking( const TrackingEvent& rTEvt )
{
    if ( rTEvt.IsTrackingEnded() )
    {
        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
        {
            if ( !(GetStyle() & WB_NOPOINTERFOCUS) && !rTEvt.IsTrackingCanceled() )
                GrabFocus();

            if ( GetStyle() & WB_TOGGLE )
            {
                // Don't toggle, when aborted
                if ( !rTEvt.IsTrackingCanceled() )
                {
                    if ( IsChecked() )
                    {
                        Check( false );
                        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
                    }
                    else
                        Check( true );
                }
            }
            else
                ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;

#ifdef USE_JAVA
            GetParent()->Invalidate( Rectangle( GetPosPixel(), GetSizePixel() ) );
            if ( GetParent()->IsInPaint() )
                GetParent()->Update();
#else	// USE_JAVA
            ImplDrawPushButton();
#endif	// USE_JAVA

            // do not call Click handler if aborted
            if ( !rTEvt.IsTrackingCanceled() )
            {
                if ( ! ( ( GetStyle() & WB_REPEAT ) &&
                         ! ( GetStyle() & WB_TOGGLE ) ) )
                    Click();
            }
        }
    }
    else
    {
        if ( ImplHitTestPushButton( this, rTEvt.GetMouseEvent().GetPosPixel() ) )
        {
            if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
            {
                if ( rTEvt.IsTrackingRepeat() && (GetStyle() & WB_REPEAT) &&
                     ! ( GetStyle() & WB_TOGGLE ) )
                    Click();
            }
            else
            {
                ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
#ifdef USE_JAVA
                GetParent()->Invalidate( Rectangle( GetPosPixel(), GetSizePixel() ) );
                if ( GetParent()->IsInPaint() )
                    GetParent()->Update();
#else	// USE_JAVA
                ImplDrawPushButton();
#endif	// USE_JAVA
            }
        }
        else
        {
            if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
            {
                ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
#ifdef USE_JAVA
                GetParent()->Invalidate( Rectangle( GetPosPixel(), GetSizePixel() ) );
                if ( GetParent()->IsInPaint() )
                    GetParent()->Update();
#else	// USE_JAVA
                ImplDrawPushButton();
#endif	// USE_JAVA
            }
        }
    }
}

void PushButton::KeyInput( const KeyEvent& rKEvt )
{
    vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

    if ( !aKeyCode.GetModifier() &&
         ((aKeyCode.GetCode() == KEY_RETURN) || (aKeyCode.GetCode() == KEY_SPACE)) )
    {
        if ( !(ImplGetButtonState() & BUTTON_DRAW_PRESSED) )
        {
            ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
            ImplDrawPushButton();
        }

        if ( ( GetStyle() & WB_REPEAT ) &&
             ! ( GetStyle() & WB_TOGGLE ) )
            Click();
    }
    else if ( (ImplGetButtonState() & BUTTON_DRAW_PRESSED) && (aKeyCode.GetCode() == KEY_ESCAPE) )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        ImplDrawPushButton();
    }
    else
        Button::KeyInput( rKEvt );
}

void PushButton::KeyUp( const KeyEvent& rKEvt )
{
    vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

    if ( (ImplGetButtonState() & BUTTON_DRAW_PRESSED) &&
         ((aKeyCode.GetCode() == KEY_RETURN) || (aKeyCode.GetCode() == KEY_SPACE)) )
    {
        if ( GetStyle() & WB_TOGGLE )
        {
            if ( IsChecked() )
            {
                Check( false );
                ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
            }
            else
                Check( true );

            Toggle();
        }
        else
            ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;

        ImplDrawPushButton();

        if ( !( ( GetStyle() & WB_REPEAT )  &&
                ! ( GetStyle() & WB_TOGGLE ) ) )
            Click();
    }
    else
        Button::KeyUp( rKEvt );
}

void PushButton::FillLayoutData() const
{
    mpControlData->mpLayoutData = new vcl::ControlLayoutData();
    const_cast<PushButton*>(this)->ImplDrawPushButton( true );
}

void PushButton::Paint( const Rectangle& )
{
    ImplDrawPushButton();
}

void PushButton::Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize,
                       sal_uLong nFlags )
{
    Point       aPos  = pDev->LogicToPixel( rPos );
    Size        aSize = pDev->LogicToPixel( rSize );
    Rectangle   aRect( aPos, aSize );
    vcl::Font   aFont = GetDrawPixelFont( pDev );

    pDev->Push();
    pDev->SetMapMode();
    pDev->SetFont( aFont );
    if ( nFlags & WINDOW_DRAW_MONO )
    {
        pDev->SetTextColor( Color( COL_BLACK ) );
    }
    else
    {
        pDev->SetTextColor( GetTextColor() );

        // DecoView uses the FaceColor...
        AllSettings aSettings = pDev->GetSettings();
        StyleSettings aStyleSettings = aSettings.GetStyleSettings();
        if ( IsControlBackground() )
            aStyleSettings.SetFaceColor( GetControlBackground() );
        else
            aStyleSettings.SetFaceColor( GetSettings().GetStyleSettings().GetFaceColor() );
        aSettings.SetStyleSettings( aStyleSettings );
        pDev->OutputDevice::SetSettings( aSettings );
    }
    pDev->SetTextFillColor();

    DecorationView aDecoView( pDev );
    sal_uInt16 nButtonStyle = 0;
    if ( nFlags & WINDOW_DRAW_MONO )
        nButtonStyle |= BUTTON_DRAW_MONO;
    if ( IsChecked() )
        nButtonStyle |= BUTTON_DRAW_CHECKED;
    aRect = aDecoView.DrawButton( aRect, nButtonStyle );

    ImplDrawPushButtonContent( pDev, nFlags, aRect, false, true );
    pDev->Pop();
}

void PushButton::Resize()
{
    Control::Resize();
    Invalidate();
}

void PushButton::GetFocus()
{
    ShowFocus( ImplGetFocusRect() );
    SetInputContext( InputContext( GetFont() ) );
    Button::GetFocus();
}

void PushButton::LoseFocus()
{
    EndSelection();
    HideFocus();
    Button::LoseFocus();
}

void PushButton::StateChanged( StateChangedType nType )
{
    Button::StateChanged( nType );

    if ( (nType == StateChangedType::ENABLE) ||
         (nType == StateChangedType::TEXT) ||
         (nType == StateChangedType::IMAGE) ||
         (nType == StateChangedType::DATA) ||
         (nType == StateChangedType::STATE) ||
         (nType == StateChangedType::UPDATEMODE) )
    {
        if ( IsReallyVisible() && IsUpdateMode() )
            Invalidate();
    }
    else if ( nType == StateChangedType::STYLE )
    {
        SetStyle( ImplInitStyle( GetWindow( WINDOW_PREV ), GetStyle() ) );

        bool bIsDefButton = ( GetStyle() & WB_DEFBUTTON ) != 0;
        bool bWasDefButton = ( GetPrevStyle() & WB_DEFBUTTON ) != 0;
        if ( bIsDefButton != bWasDefButton )
            ImplSetDefButton( bIsDefButton );

        if ( IsReallyVisible() && IsUpdateMode() )
        {
            if ( (GetPrevStyle() & PUSHBUTTON_VIEW_STYLE) !=
                 (GetStyle() & PUSHBUTTON_VIEW_STYLE) )
                Invalidate();
        }
    }
    else if ( (nType == StateChangedType::ZOOM) ||
              (nType == StateChangedType::CONTROLFONT) )
    {
        ImplInitSettings( true, false, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLFOREGROUND )
    {
        ImplInitSettings( false, true, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLBACKGROUND )
    {
        ImplInitSettings( false, false, true );
        Invalidate();
    }
}

void PushButton::DataChanged( const DataChangedEvent& rDCEvt )
{
    Button::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_FONTS) ||
         (rDCEvt.GetType() == DATACHANGED_FONTSUBSTITUTION) ||
         ((rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
          (rDCEvt.GetFlags() & SETTINGS_STYLE)) )
    {
        ImplInitSettings( true, true, true );
        Invalidate();
    }
}

bool PushButton::PreNotify( NotifyEvent& rNEvt )
{
    const MouseEvent* pMouseEvt = NULL;

    if( (rNEvt.GetType() == EVENT_MOUSEMOVE) && (pMouseEvt = rNEvt.GetMouseEvent()) != NULL )
    {
        if( pMouseEvt->IsEnterWindow() || pMouseEvt->IsLeaveWindow() )
        {
            // trigger redraw as mouse over state has changed

            // TODO: move this to Window class or make it a member !!!
            ControlType aCtrlType = 0;
            switch( GetParent()->GetType() )
            {
                case WINDOW_LISTBOX:
                case WINDOW_MULTILISTBOX:
                case WINDOW_TREELISTBOX:
                    aCtrlType = CTRL_LISTBOX;
                    break;

                case WINDOW_COMBOBOX:
                case WINDOW_PATTERNBOX:
                case WINDOW_NUMERICBOX:
                case WINDOW_METRICBOX:
                case WINDOW_CURRENCYBOX:
                case WINDOW_DATEBOX:
                case WINDOW_TIMEBOX:
                case WINDOW_LONGCURRENCYBOX:
                    aCtrlType = CTRL_COMBOBOX;
                    break;
                default:
                    break;
            }

            bool bDropDown = ( IsSymbol() && (GetSymbol()==SymbolType::SPIN_DOWN) && GetText().isEmpty() );

            if( bDropDown && GetParent()->IsNativeControlSupported( aCtrlType, PART_ENTIRE_CONTROL) &&
                   !GetParent()->IsNativeControlSupported( aCtrlType, PART_BUTTON_DOWN) )
            {
                vcl::Window *pBorder = GetParent()->GetWindow( WINDOW_BORDER );
                if(aCtrlType == CTRL_COMBOBOX)
                {
                    // only paint the button part to avoid flickering of the combobox text
                    Point aPt;
                    Rectangle aClipRect( aPt, GetOutputSizePixel() );
                    aClipRect.SetPos(pBorder->ScreenToOutputPixel(OutputToScreenPixel(aClipRect.TopLeft())));
                    pBorder->Invalidate( aClipRect );
                }
                else
                {
                    pBorder->Invalidate( INVALIDATE_NOERASE );
                    pBorder->Update();
                }
            }
            else if( (GetStyle() & WB_FLATBUTTON) ||
                     IsNativeControlSupported(CTRL_PUSHBUTTON, PART_ENTIRE_CONTROL) )
            {
                Invalidate();
            }
        }
    }

    return Button::PreNotify(rNEvt);
}

void PushButton::Toggle()
{
    ImplCallEventListenersAndHandler( VCLEVENT_PUSHBUTTON_TOGGLE, maToggleHdl, this );
}

void PushButton::SetSymbol( SymbolType eSymbol )
{
    if ( meSymbol != eSymbol )
    {
        meSymbol = eSymbol;
        StateChanged( StateChangedType::DATA );
    }
}

void PushButton::SetSymbolAlign( SymbolAlign eAlign )
{
    ImplSetSymbolAlign( eAlign );
}

void PushButton::SetDropDown( sal_uInt16 nStyle )
{
    if ( mnDDStyle != nStyle )
    {
        mnDDStyle = nStyle;
        StateChanged( StateChangedType::DATA );
    }
}

void PushButton::SetState( TriState eState )
{
    if ( meState != eState )
    {
        meState = eState;
        if ( meState == TRISTATE_FALSE )
            ImplGetButtonState() &= ~(BUTTON_DRAW_CHECKED | BUTTON_DRAW_DONTKNOW);
        else if ( meState == TRISTATE_TRUE )
        {
            ImplGetButtonState() &= ~BUTTON_DRAW_DONTKNOW;
            ImplGetButtonState() |= BUTTON_DRAW_CHECKED;
        }
        else // TRISTATE_INDET
        {
            ImplGetButtonState() &= ~BUTTON_DRAW_CHECKED;
            ImplGetButtonState() |= BUTTON_DRAW_DONTKNOW;
        }

        StateChanged( StateChangedType::STATE );
        Toggle();
    }
}

void PushButton::SetPressed( bool bPressed )
{
    if ( mbPressed != bPressed )
    {
        mbPressed = bPressed;
        StateChanged( StateChangedType::DATA );
    }
}

void PushButton::EndSelection()
{
    EndTracking( ENDTRACK_CANCEL );
    if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        if ( !mbPressed )
            ImplDrawPushButton();
    }
}

Size PushButton::CalcMinimumSize( long nMaxWidth ) const
{
    Size aSize;

    if ( IsSymbol() )
    {
        if ( IsSmallSymbol ())
            aSize = Size( 16, 12 );
        else
            aSize = Size( 26, 24 );
    }
    else if ( IsImage() && ! (ImplGetButtonState() & BUTTON_DRAW_NOIMAGE) )
        aSize = GetModeImage().GetSizePixel();
    if( mnDDStyle == PUSHBUTTON_DROPDOWN_MENUBUTTON )
    {
        long nSymbolSize = GetTextHeight() / 2 + 1;
        aSize.Width() += 2*nSymbolSize;
    }
    if ( !PushButton::GetText().isEmpty() && ! (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) )
    {
        sal_uLong nDrawFlags = 0;
        Size textSize = GetTextRect( Rectangle( Point(), Size( nMaxWidth ? nMaxWidth : 0x7fffffff, 0x7fffffff ) ),
                                     PushButton::GetText(), ImplGetTextStyle( nDrawFlags ) ).GetSize();
        aSize.Width() += textSize.Width();
        aSize.Height() = std::max( aSize.Height(), long( textSize.Height() * 1.15 ) );
    }

    // cf. ImplDrawPushButton ...
    if( (GetStyle() & WB_SMALLSTYLE) == 0 )
    {
        aSize.Width() += 24;
        aSize.Height() += 12;
    }

    return CalcWindowSize( aSize );
}

Size PushButton::GetOptimalSize() const
{
    return CalcMinimumSize();
}

bool PushButton::set_property(const OString &rKey, const OString &rValue)
{
    if (rKey == "has-default")
    {
        WinBits nBits = GetStyle();
        nBits &= ~(WB_DEFBUTTON);
        if (toBool(rValue))
            nBits |= WB_DEFBUTTON;
        SetStyle(nBits);
    }
    else
        return Button::set_property(rKey, rValue);
    return true;
}

void OKButton::ImplInit( vcl::Window* pParent, WinBits nStyle )
{
    PushButton::ImplInit( pParent, nStyle );

    SetText( Button::GetStandardText( BUTTON_OK ) );
}

OKButton::OKButton( vcl::Window* pParent, WinBits nStyle ) :
    PushButton( WINDOW_OKBUTTON )
{
    ImplInit( pParent, nStyle );
}

void OKButton::Click()
{
    // close parent if no link set
    if ( !GetClickHdl() )
    {
        vcl::Window* pParent = getNonLayoutParent(this);
        if ( pParent->IsSystemWindow() )
        {
            if ( pParent->IsDialog() )
            {
                if ( static_cast<Dialog*>(pParent)->IsInExecute() )
                    static_cast<Dialog*>(pParent)->EndDialog( RET_OK );
                // prevent recursive calls
                else if ( !static_cast<Dialog*>(pParent)->IsInClose() )
                {
                    if ( pParent->GetStyle() & WB_CLOSEABLE )
                        static_cast<Dialog*>(pParent)->Close();
                }
            }
            else
            {
                if ( pParent->GetStyle() & WB_CLOSEABLE )
                    static_cast<SystemWindow*>(pParent)->Close();
            }
        }
    }
    else
    {
        PushButton::Click();
    }
}

void CancelButton::ImplInit( vcl::Window* pParent, WinBits nStyle )
{
    PushButton::ImplInit( pParent, nStyle );

    SetText( Button::GetStandardText( BUTTON_CANCEL ) );
}

CancelButton::CancelButton( vcl::Window* pParent, WinBits nStyle ) :
    PushButton( WINDOW_CANCELBUTTON )
{
    ImplInit( pParent, nStyle );
}

void CancelButton::Click()
{
    // close parent if link not set
    if ( !GetClickHdl() )
    {
        vcl::Window* pParent = getNonLayoutParent(this);
        if ( pParent->IsSystemWindow() )
        {
            if ( pParent->IsDialog() )
            {
                if ( static_cast<Dialog*>(pParent)->IsInExecute() )
                    static_cast<Dialog*>(pParent)->EndDialog( RET_CANCEL );
                // prevent recursive calls
                else if ( !static_cast<Dialog*>(pParent)->IsInClose() )
                {
                    if ( pParent->GetStyle() & WB_CLOSEABLE )
                        static_cast<Dialog*>(pParent)->Close();
                }
            }
            else
            {
                if ( pParent->GetStyle() & WB_CLOSEABLE )
                    static_cast<SystemWindow*>(pParent)->Close();
            }
        }
    }
    else
    {
        PushButton::Click();
    }
}

CloseButton::CloseButton( vcl::Window* pParent, WinBits nStyle )
    : CancelButton(pParent, nStyle)
{
    SetText( Button::GetStandardText( BUTTON_CLOSE ) );
}

void HelpButton::ImplInit( vcl::Window* pParent, WinBits nStyle )
{
    PushButton::ImplInit( pParent, nStyle | WB_NOPOINTERFOCUS );

    SetText( Button::GetStandardText( BUTTON_HELP ) );
}

HelpButton::HelpButton( vcl::Window* pParent, WinBits nStyle ) :
    PushButton( WINDOW_HELPBUTTON )
{
    ImplInit( pParent, nStyle );
}

HelpButton::HelpButton( vcl::Window* pParent, const ResId& rResId ) :
    PushButton( WINDOW_HELPBUTTON )
{
    rResId.SetRT( RSC_HELPBUTTON );
    WinBits nStyle = ImplInitRes( rResId );
    ImplInit( pParent, nStyle );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE) )
        Show();
}

void HelpButton::Click()
{
    // trigger help if no link set
    if ( !GetClickHdl() )
    {
        vcl::Window* pFocusWin = Application::GetFocusWindow();
        if ( !pFocusWin )
            pFocusWin = this;

        HelpEvent aEvt( pFocusWin->GetPointerPosPixel(), HelpEventMode::CONTEXT );
        pFocusWin->RequestHelp( aEvt );
    }
    PushButton::Click();
}

void RadioButton::ImplInitRadioButtonData()
{
    mbChecked       = false;
    mbSaveValue     = false;
    mbRadioCheck    = true;
    mbStateChanged  = false;
}

void RadioButton::ImplInit( vcl::Window* pParent, WinBits nStyle )
{
    nStyle = ImplInitStyle(getPreviousSibling(pParent), nStyle);
    Button::ImplInit( pParent, nStyle, NULL );

    ImplInitSettings( true, true, true );
}

WinBits RadioButton::ImplInitStyle( const vcl::Window* pPrevWindow, WinBits nStyle )
{
    if ( !(nStyle & WB_NOGROUP) &&
         (!pPrevWindow || (pPrevWindow->GetType() != WINDOW_RADIOBUTTON)) )
        nStyle |= WB_GROUP;
    if ( !(nStyle & WB_NOTABSTOP) )
    {
        if ( IsChecked() )
            nStyle |= WB_TABSTOP;
        else
            nStyle &= ~WB_TABSTOP;
    }
    return nStyle;
}

const vcl::Font& RadioButton::GetCanonicalFont( const StyleSettings& _rStyle ) const
{
    return _rStyle.GetRadioCheckFont();
}

const Color& RadioButton::GetCanonicalTextColor( const StyleSettings& _rStyle ) const
{
    return _rStyle.GetRadioCheckTextColor();
}

void RadioButton::ImplInitSettings( bool bFont,
                                    bool bForeground, bool bBackground )
{
    Button::ImplInitSettings( bFont, bForeground );

    if ( bBackground )
    {
        vcl::Window* pParent = GetParent();
        if ( !IsControlBackground() &&
            (pParent->IsChildTransparentModeEnabled() || IsNativeControlSupported( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL ) ) )
        {
            EnableChildTransparentMode( true );
            SetParentClipMode( PARENTCLIPMODE_NOCLIP );
            SetPaintTransparent( true );
            SetBackground();
            if( IsNativeControlSupported( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL ) )
                mpWindowImpl->mbUseNativeFocus = ImplGetSVData()->maNWFData.mbNoFocusRects;
        }
        else
        {
            EnableChildTransparentMode( false );
            SetParentClipMode( 0 );
            SetPaintTransparent( false );

            if ( IsControlBackground() )
                SetBackground( GetControlBackground() );
            else
                SetBackground( pParent->GetBackground() );
        }
    }
}

void RadioButton::DrawRadioButtonState( )
{
    ImplDrawRadioButtonState( );
}

void RadioButton::ImplInvalidateOrDrawRadioButtonState()
{
    if( ImplGetSVData()->maNWFData.mbCheckBoxNeedsErase )
    {
        if ( IsNativeControlSupported(CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL) )
        {
#ifdef USE_JAVA
            Invalidate( maStateRect );
            if ( IsInPaint() )
                Update();
#else	// USE_JAVA
            Invalidate();
            Update();
#endif	// USE_JAVA
            return;
        }
    }
    ImplDrawRadioButtonState();
}

void RadioButton::ImplDrawRadioButtonState()
{
    bool   bNativeOK = false;

    // no native drawing for image radio buttons
    if ( !maImage && (bNativeOK=IsNativeControlSupported(CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL)) )
    {
        ImplControlValue            aControlValue( mbChecked ? BUTTONVALUE_ON : BUTTONVALUE_OFF );
        Rectangle                   aCtrlRect( maStateRect.TopLeft(), maStateRect.GetSize() );
        ControlState                nState = 0;

        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )   nState |= CTRL_STATE_PRESSED;
        if ( HasFocus() )                       nState |= CTRL_STATE_FOCUSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )   nState |= CTRL_STATE_DEFAULT;
        if ( IsEnabled() )                      nState |= CTRL_STATE_ENABLED;

        if ( IsMouseOver() && maMouseRect.IsInside( GetPointerPosPixel() ) )
            nState |= CTRL_STATE_ROLLOVER;

        bNativeOK = DrawNativeControl( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL, aCtrlRect, nState,
                    aControlValue,OUString() );

    }

    if ( !bNativeOK )
    {
        // kein Image-RadioButton
        if ( !maImage )
        {
            sal_uInt16 nStyle = ImplGetButtonState();
            if ( !IsEnabled() )
                nStyle |= BUTTON_DRAW_DISABLED;
            if ( mbChecked )
                nStyle |= BUTTON_DRAW_CHECKED;
            Image aImage = GetRadioImage( GetSettings(), nStyle );
            if ( IsZoom() )
                DrawImage( maStateRect.TopLeft(), maStateRect.GetSize(), aImage );
            else
                DrawImage( maStateRect.TopLeft(), aImage );
        }
        else
        {
            HideFocus();

            DecorationView          aDecoView( this );
            const StyleSettings&    rStyleSettings = GetSettings().GetStyleSettings();
            Rectangle               aImageRect  = maStateRect;
            Size                    aImageSize  = maImage.GetSizePixel();
            bool                    bEnabled    = IsEnabled();
            sal_uInt16 nButtonStyle = FRAME_DRAW_DOUBLEIN;

            aImageSize.Width()  = CalcZoom( aImageSize.Width() );
            aImageSize.Height() = CalcZoom( aImageSize.Height() );

            // display border and selection status
            aImageRect = aDecoView.DrawFrame( aImageRect, nButtonStyle );
            if ( (ImplGetButtonState() & BUTTON_DRAW_PRESSED) || !bEnabled )
                SetFillColor( rStyleSettings.GetFaceColor() );
            else
                SetFillColor( rStyleSettings.GetFieldColor() );
            SetLineColor();
            DrawRect( aImageRect );

            // display image
            nButtonStyle = 0;
            if ( !bEnabled )
#ifdef USE_JAVA
                nButtonStyle |= IMAGE_DRAW_SEMITRANSPARENT;
#else	// USE_JAVA
                nButtonStyle |= IMAGE_DRAW_DISABLE;
#endif	// USE_JAVA

            Image *pImage = &maImage;

            Point aImagePos( aImageRect.TopLeft() );
            aImagePos.X() += (aImageRect.GetWidth()-aImageSize.Width())/2;
            aImagePos.Y() += (aImageRect.GetHeight()-aImageSize.Height())/2;
            if ( IsZoom() )
                DrawImage( aImagePos, aImageSize, *pImage, nButtonStyle );
            else
                DrawImage( aImagePos, *pImage, nButtonStyle );

            aImageRect.Left()++;
            aImageRect.Top()++;
            aImageRect.Right()--;
            aImageRect.Bottom()--;

            ImplSetFocusRect( aImageRect );

            if ( mbChecked )
            {
                SetLineColor( rStyleSettings.GetHighlightColor() );
                SetFillColor();
                if ( (aImageSize.Width() >= 20) || (aImageSize.Height() >= 20) )
                {
                    aImageRect.Left()++;
                    aImageRect.Top()++;
                    aImageRect.Right()--;
                    aImageRect.Bottom()--;
                }
                DrawRect( aImageRect );
                aImageRect.Left()++;
                aImageRect.Top()++;
                aImageRect.Right()--;
                aImageRect.Bottom()--;
                DrawRect( aImageRect );
            }

            if ( HasFocus() )
                ShowFocus( ImplGetFocusRect() );
        }
    }
}

void RadioButton::ImplDraw( OutputDevice* pDev, sal_uLong nDrawFlags,
                            const Point& rPos, const Size& rSize,
                            const Size& rImageSize, Rectangle& rStateRect,
                            Rectangle& rMouseRect, bool bLayout )
{
    WinBits                 nWinStyle = GetStyle();
    OUString                aText( GetText() );
    MetricVector*           pVector = bLayout ? &mpControlData->mpLayoutData->m_aUnicodeBoundRects : NULL;
    OUString*               pDisplayText = bLayout ? &mpControlData->mpLayoutData->m_aDisplayText : NULL;

    pDev->Push( PushFlags::CLIPREGION );
    pDev->IntersectClipRegion( Rectangle( rPos, rSize ) );

    // no image radio button
    if ( !maImage )
    {
        if ( ( !aText.isEmpty() && ! (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) ) ||
             ( HasImage() &&  ! (ImplGetButtonState() & BUTTON_DRAW_NOIMAGE) ) )
        {
            sal_uInt16 nTextStyle = Button::ImplGetTextStyle( aText, nWinStyle, nDrawFlags );

#ifdef USE_JAVA
            if ( IsNativeControlSupported( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL ) )
            {
                ImplControlValue aControlValue;
                ControlState nState = 0;
                Color aColor;
                
                if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
                    nState |= CTRL_STATE_PRESSED;
                if ( HasFocus() )
                    nState |= CTRL_STATE_FOCUSED;
                if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )
                    nState |= CTRL_STATE_DEFAULT;
                if ( Window::IsEnabled() )
                    nState |= CTRL_STATE_ENABLED;
                if ( IsMouseOver() )
                    nState |= CTRL_STATE_ROLLOVER;
        
                if ( GetNativeControlTextColor( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL, nState, aControlValue, aColor ) )
                    SetTextColor( aColor );
            }
#endif	// USE_JAVA

            const long nImageSep = GetDrawPixel( pDev, ImplGetImageToTextDistance() );
            Size aSize( rSize );
            Point aPos( rPos );
            aPos.X() += rImageSize.Width() + nImageSep;
            aSize.Width() -= rImageSize.Width() + nImageSep;

            // if the text rect height is smaller than the height of the image
            // then for single lines the default should be centered text
            if( (nWinStyle & (WB_TOP|WB_VCENTER|WB_BOTTOM)) == 0 &&
                (rImageSize.Height() > rSize.Height() || ! (nWinStyle & WB_WORDBREAK)  ) )
            {
                nTextStyle &= ~(TEXT_DRAW_TOP|TEXT_DRAW_BOTTOM);
                nTextStyle |= TEXT_DRAW_VCENTER;
                aSize.Height() = rImageSize.Height();
            }

            ImplDrawAlignedImage( pDev, aPos, aSize, bLayout, 1,
                                  nDrawFlags, nTextStyle, NULL );

            rMouseRect          = Rectangle( aPos, aSize );
            rMouseRect.Left()   = rPos.X();

            rStateRect.Left()   = rPos.X();
            rStateRect.Top()    = rMouseRect.Top();

            if ( aSize.Height() > rImageSize.Height() )
                rStateRect.Top() += ( aSize.Height() - rImageSize.Height() ) / 2;
            else
            {
                rStateRect.Top() -= ( rImageSize.Height() - aSize.Height() ) / 2;
                if( rStateRect.Top() < 0 )
                    rStateRect.Top() = 0;
            }

            rStateRect.Right()  = rStateRect.Left() + rImageSize.Width()-1;
            rStateRect.Bottom() = rStateRect.Top() + rImageSize.Height()-1;

            if ( rStateRect.Bottom() > rMouseRect.Bottom() )
                rMouseRect.Bottom() = rStateRect.Bottom();
        }
        else
        {
            if ( mbLegacyNoTextAlign && ( nWinStyle & WB_CENTER ) )
                rStateRect.Left() = rPos.X()+((rSize.Width()-rImageSize.Width())/2);
            else if ( mbLegacyNoTextAlign && ( nWinStyle & WB_RIGHT ) )
                rStateRect.Left() = rPos.X()+rSize.Width()-rImageSize.Width(); //-1;
            else
                rStateRect.Left() = rPos.X(); //+1;
            if ( nWinStyle & WB_VCENTER )
                rStateRect.Top() = rPos.Y()+((rSize.Height()-rImageSize.Height())/2);
            else if ( nWinStyle & WB_BOTTOM )
                rStateRect.Top() = rPos.Y()+rSize.Height()-rImageSize.Height(); //-1;
            else
                rStateRect.Top() = rPos.Y(); //+1;
            rStateRect.Right()  = rStateRect.Left()+rImageSize.Width()-1;
            rStateRect.Bottom() = rStateRect.Top()+rImageSize.Height()-1;
            rMouseRect          = rStateRect;

            ImplSetFocusRect( rStateRect );

/*  and above -1 because CalcSize() does not include focus-rectangle since images would be even
    positioned higher in writer
            rFocusRect          = rStateRect;
            rFocusRect.Left()--;
            rFocusRect.Top()--;
            rFocusRect.Right()++;
            rFocusRect.Bottom()++;
*/
        }
    }
    else
    {
        bool        bTopImage   = (nWinStyle & WB_TOP) != 0;
        Size        aImageSize  = maImage.GetSizePixel();
        Rectangle   aImageRect( rPos, rSize );
        long        nTextHeight = pDev->GetTextHeight();
        long        nTextWidth  = pDev->GetCtrlTextWidth( aText );

        // calculate position and sizes
        if ( !aText.isEmpty() && ! (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) )
        {
            Size aTmpSize( (aImageSize.Width()+8), (aImageSize.Height()+8) );
            if ( bTopImage )
            {
                aImageRect.Left() = (rSize.Width()-aTmpSize.Width())/2;
                aImageRect.Top()  = (rSize.Height()-(aTmpSize.Height()+nTextHeight+6))/2;
            }
            else
                aImageRect.Top()  = (rSize.Height()-aTmpSize.Height())/2;

            aImageRect.Right()  = aImageRect.Left()+aTmpSize.Width();
            aImageRect.Bottom() = aImageRect.Top()+aTmpSize.Height();

            // display text
            Point aTxtPos = rPos;
            if ( bTopImage )
            {
                aTxtPos.X() += (rSize.Width()-nTextWidth)/2;
                aTxtPos.Y() += aImageRect.Bottom()+6;
            }
            else
            {
                aTxtPos.X() += aImageRect.Right()+8;
                aTxtPos.Y() += (rSize.Height()-nTextHeight)/2;
            }
            pDev->DrawCtrlText( aTxtPos, aText, 0, aText.getLength(), TEXT_DRAW_MNEMONIC, pVector, pDisplayText );
        }

        rMouseRect = aImageRect;
        rStateRect = aImageRect;
    }

    pDev->Pop();
}

void RadioButton::ImplDrawRadioButton( bool bLayout )
{
    if( !bLayout )
        HideFocus();

    Size aImageSize;
    if ( !maImage )
        aImageSize = ImplGetRadioImageSize();
    else
        aImageSize  = maImage.GetSizePixel();
    aImageSize.Width()  = CalcZoom( aImageSize.Width() );
    aImageSize.Height() = CalcZoom( aImageSize.Height() );

    // Draw control text
    ImplDraw( this, 0, Point(), GetOutputSizePixel(),
              aImageSize, maStateRect, maMouseRect, bLayout );

    if( !bLayout || IsNativeControlSupported(CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL) )
    {
        if ( !maImage && HasFocus() )
            ShowFocus( ImplGetFocusRect() );

        ImplDrawRadioButtonState();
    }
}

void RadioButton::group(RadioButton &rOther)
{
    if (&rOther == this)
        return;

    if (!m_xGroup)
    {
        m_xGroup.reset(new std::vector<RadioButton*>);
        m_xGroup->push_back(this);
    }

    std::vector<RadioButton*>::iterator aFind = std::find(m_xGroup->begin(), m_xGroup->end(), &rOther);
    if (aFind == m_xGroup->end())
    {
        m_xGroup->push_back(&rOther);

        if (rOther.m_xGroup)
        {
            std::vector< RadioButton* > aOthers(rOther.GetRadioButtonGroup(false));
            //make all members of the group share the same button group
            for (std::vector<RadioButton*>::iterator aI = aOthers.begin(), aEnd = aOthers.end(); aI != aEnd; ++aI)
            {
                aFind = std::find(m_xGroup->begin(), m_xGroup->end(), *aI);
                if (aFind == m_xGroup->end())
                    m_xGroup->push_back(*aI);
            }
        }

        //make all members of the group share the same button group
        for (std::vector<RadioButton*>::iterator aI = m_xGroup->begin(), aEnd = m_xGroup->end();
            aI != aEnd; ++aI)
        {
            RadioButton* pButton = *aI;
            pButton->m_xGroup = m_xGroup;
        }
    }

    //if this one is checked, uncheck all the others
    if (mbChecked)
        ImplUncheckAllOther();
}

std::vector< RadioButton* > RadioButton::GetRadioButtonGroup(bool bIncludeThis) const
{
    if (m_xGroup)
    {
        if (bIncludeThis)
            return *m_xGroup;
        std::vector< RadioButton* > aGroup;
        for (std::vector<RadioButton*>::iterator aI = m_xGroup->begin(), aEnd = m_xGroup->end(); aI != aEnd; ++aI)
        {
            RadioButton *pRadioButton = *aI;
            if (pRadioButton == this)
                continue;
            aGroup.push_back(pRadioButton);
        }
        return aGroup;
    }

    //old-school
    SAL_WARN("vcl.control", "No new-style group set on radiobutton <" << GetHelpId() << "> using old-style digging around");

    // go back to first in group;
    vcl::Window* pFirst = const_cast<RadioButton*>(this);
    while( ( pFirst->GetStyle() & WB_GROUP ) == 0 )
    {
        vcl::Window* pWindow = pFirst->GetWindow( WINDOW_PREV );
        if( pWindow )
            pFirst = pWindow;
        else
            break;
    }
    std::vector< RadioButton* > aGroup;
    // insert radiobuttons up to next group
    do
    {
        if( pFirst->GetType() == WINDOW_RADIOBUTTON )
        {
            if( pFirst != this || bIncludeThis )
                aGroup.push_back( static_cast<RadioButton*>(pFirst) );
        }
        pFirst = pFirst->GetWindow( WINDOW_NEXT );
    } while( pFirst && ( ( pFirst->GetStyle() & WB_GROUP ) == 0 ) );

    return aGroup;
}

void RadioButton::ImplUncheckAllOther()
{
    mpWindowImpl->mnStyle |= WB_TABSTOP;

    std::vector<RadioButton*> aGroup(GetRadioButtonGroup(false));
    // iterate over radio button group and checked buttons
    for (std::vector<RadioButton*>::iterator aI = aGroup.begin(), aEnd = aGroup.end(); aI != aEnd; ++aI)
    {
        RadioButton *pWindow = *aI;
        if ( pWindow->IsChecked() )
        {
            ImplDelData aDelData;
            pWindow->ImplAddDel( &aDelData );
            pWindow->SetState( false );
            if ( aDelData.IsDead() )
                return;
            pWindow->ImplRemoveDel( &aDelData );
        }

        // not inside if clause to always remove wrongly set WB_TABSTOPS
        pWindow->mpWindowImpl->mnStyle &= ~WB_TABSTOP;
    }
}

void RadioButton::ImplCallClick( bool bGrabFocus, sal_uInt16 nFocusFlags )
{
    mbStateChanged = !mbChecked;
    mbChecked = true;
    mpWindowImpl->mnStyle |= WB_TABSTOP;
    ImplInvalidateOrDrawRadioButtonState();
    ImplDelData aDelData;
    ImplAddDel( &aDelData );
    if ( mbRadioCheck )
        ImplUncheckAllOther();
    if ( aDelData.IsDead() )
        return;
    if ( bGrabFocus )
        ImplGrabFocus( nFocusFlags );
    if ( aDelData.IsDead() )
        return;
    if ( mbStateChanged )
        Toggle();
    if ( aDelData.IsDead() )
        return;
    Click();
    if ( aDelData.IsDead() )
        return;
    ImplRemoveDel( &aDelData );
    mbStateChanged = false;
}

RadioButton::RadioButton( vcl::Window* pParent, WinBits nStyle ) :
    Button( WINDOW_RADIOBUTTON ), mbLegacyNoTextAlign( false )
{
    ImplInitRadioButtonData();
    ImplInit( pParent, nStyle );
}

RadioButton::RadioButton( vcl::Window* pParent, const ResId& rResId ) :
    Button( WINDOW_RADIOBUTTON ), mbLegacyNoTextAlign( false )
{
    rResId.SetRT( RSC_RADIOBUTTON );
    WinBits nStyle = ImplInitRes( rResId );
    ImplInitRadioButtonData();
    ImplInit( pParent, nStyle );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE) )
        Show();
}

void RadioButton::ImplLoadRes( const ResId& rResId )
{
    Button::ImplLoadRes( rResId );

    //anderer Wert als Default ?
    sal_uInt16 nChecked = ReadShortRes();
    if ( nChecked )
        SetState( true );
}

RadioButton::~RadioButton()
{
    if (m_xGroup)
    {
        m_xGroup->erase(std::remove(m_xGroup->begin(), m_xGroup->end(), this),
            m_xGroup->end());
    }
}

void RadioButton::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( rMEvt.IsLeft() && maMouseRect.IsInside( rMEvt.GetPosPixel() ) )
    {
        ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
        ImplInvalidateOrDrawRadioButtonState();
        StartTracking();
        return;
    }

    Button::MouseButtonDown( rMEvt );
}

void RadioButton::Tracking( const TrackingEvent& rTEvt )
{
    if ( rTEvt.IsTrackingEnded() )
    {
        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
        {
            if ( !(GetStyle() & WB_NOPOINTERFOCUS) && !rTEvt.IsTrackingCanceled() )
                GrabFocus();

            ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;

            // do not call click handler if aborted
            if ( !rTEvt.IsTrackingCanceled() )
                ImplCallClick();
            else
                ImplInvalidateOrDrawRadioButtonState();
        }
    }
    else
    {
        if ( maMouseRect.IsInside( rTEvt.GetMouseEvent().GetPosPixel() ) )
        {
            if ( !(ImplGetButtonState() & BUTTON_DRAW_PRESSED) )
            {
                ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
                ImplInvalidateOrDrawRadioButtonState();
            }
        }
        else
        {
            if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
            {
                ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
                ImplInvalidateOrDrawRadioButtonState();
            }
        }
    }
}

void RadioButton::KeyInput( const KeyEvent& rKEvt )
{
    vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

    if ( !aKeyCode.GetModifier() && (aKeyCode.GetCode() == KEY_SPACE) )
    {
        if ( !(ImplGetButtonState() & BUTTON_DRAW_PRESSED) )
        {
            ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
            ImplInvalidateOrDrawRadioButtonState();
        }
    }
    else if ( (ImplGetButtonState() & BUTTON_DRAW_PRESSED) && (aKeyCode.GetCode() == KEY_ESCAPE) )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        ImplInvalidateOrDrawRadioButtonState();
    }
    else
        Button::KeyInput( rKEvt );
}

void RadioButton::KeyUp( const KeyEvent& rKEvt )
{
    vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

    if ( (ImplGetButtonState() & BUTTON_DRAW_PRESSED) && (aKeyCode.GetCode() == KEY_SPACE) )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        ImplCallClick();
    }
    else
        Button::KeyUp( rKEvt );
}

void RadioButton::FillLayoutData() const
{
    mpControlData->mpLayoutData = new vcl::ControlLayoutData();
    const_cast<RadioButton*>(this)->ImplDrawRadioButton( true );
}

void RadioButton::Paint( const Rectangle& )
{
    ImplDrawRadioButton();
}

void RadioButton::Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize,
                        sal_uLong nFlags )
{
    if ( !maImage )
    {
        MapMode     aResMapMode( MAP_100TH_MM );
        Point       aPos  = pDev->LogicToPixel( rPos );
        Size        aSize = pDev->LogicToPixel( rSize );
        Size        aImageSize = pDev->LogicToPixel( Size( 300, 300 ), aResMapMode );
        Size        aBrd1Size = pDev->LogicToPixel( Size( 20, 20 ), aResMapMode );
        Size        aBrd2Size = pDev->LogicToPixel( Size( 60, 60 ), aResMapMode );
        vcl::Font   aFont = GetDrawPixelFont( pDev );
        Rectangle   aStateRect;
        Rectangle   aMouseRect;

        aImageSize.Width()  = CalcZoom( aImageSize.Width() );
        aImageSize.Height() = CalcZoom( aImageSize.Height() );
        aBrd1Size.Width()   = CalcZoom( aBrd1Size.Width() );
        aBrd1Size.Height()  = CalcZoom( aBrd1Size.Height() );
        aBrd2Size.Width()   = CalcZoom( aBrd2Size.Width() );
        aBrd2Size.Height()  = CalcZoom( aBrd2Size.Height() );

        if ( !aBrd1Size.Width() )
            aBrd1Size.Width() = 1;
        if ( !aBrd1Size.Height() )
            aBrd1Size.Height() = 1;
        if ( !aBrd2Size.Width() )
            aBrd2Size.Width() = 1;
        if ( !aBrd2Size.Height() )
            aBrd2Size.Height() = 1;

        pDev->Push();
        pDev->SetMapMode();
        pDev->SetFont( aFont );
        if ( nFlags & WINDOW_DRAW_MONO )
            pDev->SetTextColor( Color( COL_BLACK ) );
        else
            pDev->SetTextColor( GetTextColor() );
        pDev->SetTextFillColor();

        ImplDraw( pDev, nFlags, aPos, aSize,
                  aImageSize, aStateRect, aMouseRect );

        Point   aCenterPos = aStateRect.Center();
        long    nRadX = aImageSize.Width()/2;
        long    nRadY = aImageSize.Height()/2;

        pDev->SetLineColor();
        pDev->SetFillColor( Color( COL_BLACK ) );
        pDev->DrawPolygon( Polygon( aCenterPos, nRadX, nRadY ) );
        nRadX -= aBrd1Size.Width();
        nRadY -= aBrd1Size.Height();
        pDev->SetFillColor( Color( COL_WHITE ) );
        pDev->DrawPolygon( Polygon( aCenterPos, nRadX, nRadY ) );
        if ( mbChecked )
        {
            nRadX -= aBrd1Size.Width();
            nRadY -= aBrd1Size.Height();
            if ( !nRadX )
                nRadX = 1;
            if ( !nRadY )
                nRadY = 1;
            pDev->SetFillColor( Color( COL_BLACK ) );
            pDev->DrawPolygon( Polygon( aCenterPos, nRadX, nRadY ) );
        }

        pDev->Pop();
    }
    else
    {
        OSL_FAIL( "RadioButton::Draw() - not implemented for RadioButton with Image" );
    }
}

void RadioButton::Resize()
{
    Control::Resize();
    Invalidate();
}

void RadioButton::GetFocus()
{
    ShowFocus( ImplGetFocusRect() );
    SetInputContext( InputContext( GetFont() ) );
    Button::GetFocus();
}

void RadioButton::LoseFocus()
{
    if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        ImplInvalidateOrDrawRadioButtonState();
    }

    HideFocus();
    Button::LoseFocus();
}

void RadioButton::StateChanged( StateChangedType nType )
{
    Button::StateChanged( nType );

    if ( nType == StateChangedType::STATE )
    {
        if ( IsReallyVisible() && IsUpdateMode() )
            Invalidate( maStateRect );
    }
    else if ( (nType == StateChangedType::ENABLE) ||
              (nType == StateChangedType::TEXT) ||
              (nType == StateChangedType::IMAGE) ||
              (nType == StateChangedType::DATA) ||
              (nType == StateChangedType::UPDATEMODE) )
    {
        if ( IsUpdateMode() )
            Invalidate();
    }
    else if ( nType == StateChangedType::STYLE )
    {
        SetStyle( ImplInitStyle( GetWindow( WINDOW_PREV ), GetStyle() ) );

        if ( (GetPrevStyle() & RADIOBUTTON_VIEW_STYLE) !=
             (GetStyle() & RADIOBUTTON_VIEW_STYLE) )
        {
            if ( IsUpdateMode() )
                Invalidate();
        }
    }
    else if ( (nType == StateChangedType::ZOOM) ||
              (nType == StateChangedType::CONTROLFONT) )
    {
        ImplInitSettings( true, false, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLFOREGROUND )
    {
        ImplInitSettings( false, true, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLBACKGROUND )
    {
        ImplInitSettings( false, false, true );
        Invalidate();
    }
}

void RadioButton::DataChanged( const DataChangedEvent& rDCEvt )
{
    Button::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_FONTS) ||
         (rDCEvt.GetType() == DATACHANGED_FONTSUBSTITUTION) ||
         ((rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
          (rDCEvt.GetFlags() & SETTINGS_STYLE)) )
    {
        ImplInitSettings( true, true, true );
        Invalidate();
    }
}

bool RadioButton::PreNotify( NotifyEvent& rNEvt )
{
    const MouseEvent* pMouseEvt = NULL;

    if( (rNEvt.GetType() == EVENT_MOUSEMOVE) && (pMouseEvt = rNEvt.GetMouseEvent()) != NULL )
    {
#ifndef USE_JAVA
        if( !pMouseEvt->GetButtons() && !pMouseEvt->IsSynthetic() && !pMouseEvt->IsModifierChanged() )
        {
            // trigger redraw if mouse over state has changed
            if( IsNativeControlSupported(CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL) )
            {
                if( ( maMouseRect.IsInside( GetPointerPosPixel()) &&
                     !maMouseRect.IsInside( GetLastPointerPosPixel()) ) ||
                    ( maMouseRect.IsInside( GetLastPointerPosPixel()) &&
                     !maMouseRect.IsInside( GetPointerPosPixel()) ) ||
                     pMouseEvt->IsLeaveWindow() || pMouseEvt->IsEnterWindow() )
                {
                    Invalidate( maStateRect );
                }
            }
        }
#endif	// !USE_JAVA
    }

    return Button::PreNotify(rNEvt);
}

void RadioButton::Toggle()
{
    ImplCallEventListenersAndHandler( VCLEVENT_RADIOBUTTON_TOGGLE, maToggleHdl, this );
}

bool RadioButton::SetModeRadioImage( const Image& rImage )
{
    if ( rImage != maImage )
    {
        maImage = rImage;
        StateChanged( StateChangedType::DATA );
        queue_resize();
    }
    return true;
}


void RadioButton::SetState( bool bCheck )
{
    // TabStop-Flag richtig mitfuehren
    if ( bCheck )
        mpWindowImpl->mnStyle |= WB_TABSTOP;
    else
        mpWindowImpl->mnStyle &= ~WB_TABSTOP;

    if ( mbChecked != bCheck )
    {
        mbChecked = bCheck;
        StateChanged( StateChangedType::STATE );
        Toggle();
    }
}

bool RadioButton::set_property(const OString &rKey, const OString &rValue)
{
    if (rKey == "active")
        SetState(toBool(rValue));
    else if (rKey == "image-position")
    {
        WinBits nBits = GetStyle();
        if (rValue == "left")
        {
            nBits &= ~(WB_CENTER | WB_RIGHT);
            nBits |= WB_LEFT;
        }
        else if (rValue == "right")
        {
            nBits &= ~(WB_CENTER | WB_LEFT);
            nBits |= WB_RIGHT;
        }
        else if (rValue == "top")
        {
            nBits &= ~(WB_VCENTER | WB_BOTTOM);
            nBits |= WB_TOP;
        }
        else if (rValue == "bottom")
        {
            nBits &= ~(WB_VCENTER | WB_TOP);
            nBits |= WB_BOTTOM;
        }
        //Its rather mad to have to set these bits when there is the other
        //image align. Looks like e.g. the radiobuttons etc weren't converted
        //over to image align fully.
        SetStyle(nBits);
        //Deliberate to set the sane ImageAlign property
        return Button::set_property(rKey, rValue);
    }
    else
        return Button::set_property(rKey, rValue);
    return true;
}

void RadioButton::Check( bool bCheck )
{
    // TabStop-Flag richtig mitfuehren
    if ( bCheck )
        mpWindowImpl->mnStyle |= WB_TABSTOP;
    else
        mpWindowImpl->mnStyle &= ~WB_TABSTOP;

    if ( mbChecked != bCheck )
    {
        mbChecked = bCheck;
        ImplDelData aDelData;
        ImplAddDel( &aDelData );
        StateChanged( StateChangedType::STATE );
        if ( aDelData.IsDead() )
            return;
        if ( bCheck && mbRadioCheck )
            ImplUncheckAllOther();
        if ( aDelData.IsDead() )
            return;
        Toggle();
        ImplRemoveDel( &aDelData );
    }
}

long RadioButton::ImplGetImageToTextDistance() const
{
    // 4 pixels, but take zoom into account, so the text doesn't "jump" relative to surrounding elements,
    // which might have been aligned with the text of the check box
    return CalcZoom( 4 );
}

Size RadioButton::ImplGetRadioImageSize() const
{
    Size aSize;
    bool bDefaultSize = true;
    if( IsNativeControlSupported( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL ) )
    {
        ImplControlValue aControlValue;
        // #i45896# workaround gcc3.3 temporary problem
        Rectangle        aCtrlRegion( Point( 0, 0 ), GetSizePixel() );
        ControlState     nState = CTRL_STATE_DEFAULT|CTRL_STATE_ENABLED;
        Rectangle aBoundingRgn, aContentRgn;

        // get native size of a radio button
        if( GetNativeControlRegion( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL, aCtrlRegion,
                                           nState, aControlValue, OUString(),
                                           aBoundingRgn, aContentRgn ) )
        {
            aSize = aContentRgn.GetSize();
            bDefaultSize = false;
        }
    }
    if( bDefaultSize )
        aSize = GetRadioImage( GetSettings(), 0 ).GetSizePixel();
    return aSize;
}

static void LoadThemedImageList (const StyleSettings &rStyleSettings,
                                 ImageList *pList, const ResId &rResId,
                                 sal_uInt16 nImages)
{
    Color aColorAry1[6];
    Color aColorAry2[6];
    aColorAry1[0] = Color( 0xC0, 0xC0, 0xC0 );
    aColorAry1[1] = Color( 0xFF, 0xFF, 0x00 );
    aColorAry1[2] = Color( 0xFF, 0xFF, 0xFF );
    aColorAry1[3] = Color( 0x80, 0x80, 0x80 );
    aColorAry1[4] = Color( 0x00, 0x00, 0x00 );
    aColorAry1[5] = Color( 0x00, 0xFF, 0x00 );
    aColorAry2[0] = rStyleSettings.GetFaceColor();
    aColorAry2[1] = rStyleSettings.GetWindowColor();
    aColorAry2[2] = rStyleSettings.GetLightColor();
    aColorAry2[3] = rStyleSettings.GetShadowColor();
    aColorAry2[4] = rStyleSettings.GetDarkShadowColor();
    aColorAry2[5] = rStyleSettings.GetWindowTextColor();

    Color aMaskColor(0x00, 0x00, 0xFF );
        DBG_ASSERT( sizeof(aColorAry1) == sizeof(aColorAry2), "aColorAry1 must match aColorAry2" );
    // FIXME: do we want the mask for the checkbox ?
    pList->InsertFromHorizontalBitmap (rResId, nImages, &aMaskColor,
        aColorAry1, aColorAry2, sizeof(aColorAry1) / sizeof(Color));
}

Image RadioButton::GetRadioImage( const AllSettings& rSettings, sal_uInt16 nFlags )
{
    ImplSVData*             pSVData = ImplGetSVData();
    const StyleSettings&    rStyleSettings = rSettings.GetStyleSettings();
    sal_uInt16              nStyle = 0;

    if ( rStyleSettings.GetOptions() & STYLE_OPTION_MONO )
        nStyle = STYLE_RADIOBUTTON_MONO;

    if ( !pSVData->maCtrlData.mpRadioImgList ||
         (pSVData->maCtrlData.mnRadioStyle != nStyle) ||
         (pSVData->maCtrlData.mnLastRadioFColor != rStyleSettings.GetFaceColor().GetColor()) ||
         (pSVData->maCtrlData.mnLastRadioWColor != rStyleSettings.GetWindowColor().GetColor()) ||
         (pSVData->maCtrlData.mnLastRadioLColor != rStyleSettings.GetLightColor().GetColor()) )
    {
        if ( pSVData->maCtrlData.mpRadioImgList )
            delete pSVData->maCtrlData.mpRadioImgList;

        pSVData->maCtrlData.mnLastRadioFColor = rStyleSettings.GetFaceColor().GetColor();
        pSVData->maCtrlData.mnLastRadioWColor = rStyleSettings.GetWindowColor().GetColor();
        pSVData->maCtrlData.mnLastRadioLColor = rStyleSettings.GetLightColor().GetColor();

        ResMgr* pResMgr = ImplGetResMgr();
        pSVData->maCtrlData.mpRadioImgList = new ImageList();
        if( pResMgr )
            LoadThemedImageList( rStyleSettings,
                 pSVData->maCtrlData.mpRadioImgList,
                 ResId( SV_RESID_BITMAP_RADIO+nStyle, *pResMgr ), 6
            );
        pSVData->maCtrlData.mnRadioStyle = nStyle;
    }

    sal_uInt16 nId;
    if ( nFlags & BUTTON_DRAW_DISABLED )
    {
        if ( nFlags & BUTTON_DRAW_CHECKED )
            nId = 6;
        else
            nId = 5;
    }
    else if ( nFlags & BUTTON_DRAW_PRESSED )
    {
        if ( nFlags & BUTTON_DRAW_CHECKED )
            nId = 4;
        else
            nId = 3;
    }
    else
    {
        if ( nFlags & BUTTON_DRAW_CHECKED )
            nId = 2;
        else
            nId = 1;
    }
    return pSVData->maCtrlData.mpRadioImgList->GetImage( nId );
}

void RadioButton::ImplSetMinimumNWFSize()
{
    Push( PushFlags::MAPMODE );
    SetMapMode( MAP_PIXEL );

    ImplControlValue aControlValue;
    Size aCurSize( GetSizePixel() );
    Rectangle aCtrlRegion( Point( 0, 0 ), aCurSize );
    Rectangle aBoundingRgn, aContentRgn;

    // get native size of a radiobutton
    if( GetNativeControlRegion( CTRL_RADIOBUTTON, PART_ENTIRE_CONTROL, aCtrlRegion,
                                CTRL_STATE_DEFAULT|CTRL_STATE_ENABLED, aControlValue, OUString(),
                                aBoundingRgn, aContentRgn ) )
    {
        Size aSize = aContentRgn.GetSize();

        if( aSize.Height() > aCurSize.Height() )
        {
            aCurSize.Height() = aSize.Height();
            SetSizePixel( aCurSize );
        }
    }

    Pop();
}

Size RadioButton::CalcMinimumSize( long nMaxWidth ) const
{
    Size aSize;
    if ( !maImage )
        aSize = ImplGetRadioImageSize();
    else
    {
        aSize = maImage.GetSizePixel();
        aSize.Width() += 8;
        aSize.Height() += 8;
    }

    OUString aText = GetText();
    if ( !aText.isEmpty() && ! (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) )
    {
        bool bTopImage = (GetStyle() & WB_TOP) != 0;

        if (!bTopImage)
        {
            nMaxWidth -= aSize.Width();
            nMaxWidth -= ImplGetImageToTextDistance();
        }

        // subtract what will be added later
        nMaxWidth-=2;

        Size aTextSize = GetTextRect( Rectangle( Point(), Size( nMaxWidth > 0 ? nMaxWidth : 0x7fffffff, 0x7fffffff ) ),
                                      aText, FixedText::ImplGetTextStyle( GetStyle() ) ).GetSize();

        aSize.Width()+=2;   // for focus rect

        if (!bTopImage)
        {
            aSize.Width() += ImplGetImageToTextDistance();
            aSize.Width() += aTextSize.Width();
            if ( aSize.Height() < aTextSize.Height() )
                aSize.Height() = aTextSize.Height();
        }
        else
        {
            aSize.Height() += 6;
            aSize.Height() += GetTextHeight();
            if ( aSize.Width() < aTextSize.Width() )
                aSize.Width() = aTextSize.Width();
        }
    }

    return CalcWindowSize( aSize );
}

Size RadioButton::GetOptimalSize() const
{
    return CalcMinimumSize();
}

void CheckBox::ImplInitCheckBoxData()
{
    meState         = TRISTATE_FALSE;
    meSaveValue     = TRISTATE_FALSE;
    mbTriState      = false;
}

void CheckBox::ImplInit( vcl::Window* pParent, WinBits nStyle )
{
    nStyle = ImplInitStyle(getPreviousSibling(pParent), nStyle);
    Button::ImplInit( pParent, nStyle, NULL );

    ImplInitSettings( true, true, true );
}

WinBits CheckBox::ImplInitStyle( const vcl::Window* pPrevWindow, WinBits nStyle )
{
    if ( !(nStyle & WB_NOTABSTOP) )
        nStyle |= WB_TABSTOP;
    if ( !(nStyle & WB_NOGROUP) &&
         (!pPrevWindow || (pPrevWindow->GetType() != WINDOW_CHECKBOX)) )
        nStyle |= WB_GROUP;
    return nStyle;
}

const vcl::Font& CheckBox::GetCanonicalFont( const StyleSettings& _rStyle ) const
{
    return _rStyle.GetRadioCheckFont();
}

const Color& CheckBox::GetCanonicalTextColor( const StyleSettings& _rStyle ) const
{
    return _rStyle.GetRadioCheckTextColor();
}

void CheckBox::ImplInitSettings( bool bFont,
                                 bool bForeground, bool bBackground )
{
    Button::ImplInitSettings( bFont, bForeground );

    if ( bBackground )
    {
        vcl::Window* pParent = GetParent();
        if ( !IsControlBackground() &&
            (pParent->IsChildTransparentModeEnabled() || IsNativeControlSupported( CTRL_CHECKBOX, PART_ENTIRE_CONTROL ) ) )
        {
            EnableChildTransparentMode( true );
            SetParentClipMode( PARENTCLIPMODE_NOCLIP );
            SetPaintTransparent( true );
            SetBackground();
            if( IsNativeControlSupported( CTRL_CHECKBOX, PART_ENTIRE_CONTROL ) )
                ImplGetWindowImpl()->mbUseNativeFocus = ImplGetSVData()->maNWFData.mbNoFocusRects;
        }
        else
        {
            EnableChildTransparentMode( false );
            SetParentClipMode( 0 );
            SetPaintTransparent( false );

            if ( IsControlBackground() )
                SetBackground( GetControlBackground() );
            else
                SetBackground( pParent->GetBackground() );
        }
    }
}

void CheckBox::ImplLoadRes( const ResId& rResId )
{
    Button::ImplLoadRes( rResId );

    sal_uInt16 nChecked = ReadShortRes();
    //anderer Wert als Default ?
    if( nChecked )
        Check( true );
}

void CheckBox::ImplInvalidateOrDrawCheckBoxState()
{
    if( ImplGetSVData()->maNWFData.mbCheckBoxNeedsErase )
    {
        if ( IsNativeControlSupported(CTRL_CHECKBOX, PART_ENTIRE_CONTROL) )
        {
#ifdef USE_JAVA
            Invalidate( maStateRect );
            if ( IsInPaint() )
                Update();
#else	// USE_JAVA
            Invalidate();
            Update();
#endif	// USE_JAVA
            return;
        }
    }
    ImplDrawCheckBoxState();
}

void CheckBox::ImplDrawCheckBoxState()
{
    bool    bNativeOK = true;

    if ( (bNativeOK=IsNativeControlSupported(CTRL_CHECKBOX, PART_ENTIRE_CONTROL)) )
    {
        ImplControlValue    aControlValue( meState == TRISTATE_TRUE ? BUTTONVALUE_ON : BUTTONVALUE_OFF );
        Rectangle           aCtrlRegion( maStateRect );
        ControlState        nState = 0;

        if ( HasFocus() )                       nState |= CTRL_STATE_FOCUSED;
        if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )   nState |= CTRL_STATE_DEFAULT;
        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )   nState |= CTRL_STATE_PRESSED;
        if ( IsEnabled() )                      nState |= CTRL_STATE_ENABLED;

        if ( meState == TRISTATE_TRUE )
            aControlValue.setTristateVal( BUTTONVALUE_ON );
        else if ( meState == TRISTATE_INDET )
            aControlValue.setTristateVal( BUTTONVALUE_MIXED );

        if ( IsMouseOver() && maMouseRect.IsInside( GetPointerPosPixel() ) )
            nState |= CTRL_STATE_ROLLOVER;

        bNativeOK = DrawNativeControl( CTRL_CHECKBOX, PART_ENTIRE_CONTROL, aCtrlRegion, nState,
                                 aControlValue, OUString() );
    }

    if ( !bNativeOK )
    {
        sal_uInt16 nStyle = ImplGetButtonState();
        if ( !IsEnabled() )
            nStyle |= BUTTON_DRAW_DISABLED;
        if ( meState == TRISTATE_INDET )
            nStyle |= BUTTON_DRAW_DONTKNOW;
        else if ( meState == TRISTATE_TRUE )
            nStyle |= BUTTON_DRAW_CHECKED;
        Image aImage = GetCheckImage( GetSettings(), nStyle );
        if ( IsZoom() )
            DrawImage( maStateRect.TopLeft(), maStateRect.GetSize(), aImage );
        else
            DrawImage( maStateRect.TopLeft(), aImage );
    }
}

void CheckBox::ImplDraw( OutputDevice* pDev, sal_uLong nDrawFlags,
                         const Point& rPos, const Size& rSize,
                         const Size& rImageSize, Rectangle& rStateRect,
                         Rectangle& rMouseRect, bool bLayout )
{
    WinBits                 nWinStyle = GetStyle();
    OUString                aText( GetText() );

    pDev->Push( PushFlags::CLIPREGION | PushFlags::LINECOLOR );
    pDev->IntersectClipRegion( Rectangle( rPos, rSize ) );

    long nLineY = rPos.Y() + (rSize.Height()-1)/2;
    if ( ( !aText.isEmpty() && ! (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) ) ||
         ( HasImage() && !  (ImplGetButtonState() & BUTTON_DRAW_NOIMAGE) ) )
    {
        sal_uInt16 nTextStyle = Button::ImplGetTextStyle( aText, nWinStyle, nDrawFlags );

#ifdef USE_JAVA
        if ( IsNativeControlSupported( CTRL_CHECKBOX, PART_ENTIRE_CONTROL ) )
        {
            ImplControlValue aControlValue;
            ControlState nState = 0;
            Color aColor;

            if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
                nState |= CTRL_STATE_PRESSED;
            if ( HasFocus() )
                nState |= CTRL_STATE_FOCUSED;
            if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )
                nState |= CTRL_STATE_DEFAULT;
            if ( Window::IsEnabled() )
                nState |= CTRL_STATE_ENABLED;
            if ( IsMouseOver() )
                nState |= CTRL_STATE_ROLLOVER;

            if ( GetNativeControlTextColor( CTRL_CHECKBOX, PART_ENTIRE_CONTROL, nState, aControlValue, aColor ) )
                SetTextColor( aColor );
        }
#endif	// USE_JAVA

        const long nImageSep = GetDrawPixel( pDev, ImplGetImageToTextDistance() );
        Size aSize( rSize );
        Point aPos( rPos );
        aPos.X() += rImageSize.Width() + nImageSep;
        aSize.Width() -= rImageSize.Width() + nImageSep;

        // if the text rect height is smaller than the height of the image
        // then for single lines the default should be centered text
        if( (nWinStyle & (WB_TOP|WB_VCENTER|WB_BOTTOM)) == 0 &&
            (rImageSize.Height() > rSize.Height() || ! (nWinStyle & WB_WORDBREAK) ) )
        {
            nTextStyle &= ~(TEXT_DRAW_TOP|TEXT_DRAW_BOTTOM);
            nTextStyle |= TEXT_DRAW_VCENTER;
            aSize.Height() = rImageSize.Height();
        }

        ImplDrawAlignedImage( pDev, aPos, aSize, bLayout, 1,
                              nDrawFlags, nTextStyle, NULL );
        nLineY = aPos.Y() + aSize.Height()/2;

        rMouseRect          = Rectangle( aPos, aSize );
        rMouseRect.Left()   = rPos.X();
        rStateRect.Left()   = rPos.X();
        rStateRect.Top()    = rMouseRect.Top();

        if ( aSize.Height() > rImageSize.Height() )
            rStateRect.Top() += ( aSize.Height() - rImageSize.Height() ) / 2;
        else
        {
            rStateRect.Top() -= ( rImageSize.Height() - aSize.Height() ) / 2;
            if( rStateRect.Top() < 0 )
                rStateRect.Top() = 0;
        }

        rStateRect.Right()  = rStateRect.Left()+rImageSize.Width()-1;
        rStateRect.Bottom() = rStateRect.Top()+rImageSize.Height()-1;
        if ( rStateRect.Bottom() > rMouseRect.Bottom() )
            rMouseRect.Bottom() = rStateRect.Bottom();
    }
    else
    {
        if ( mbLegacyNoTextAlign && ( nWinStyle & WB_CENTER ) )
            rStateRect.Left() = rPos.X()+((rSize.Width()-rImageSize.Width())/2);
        else if ( mbLegacyNoTextAlign && ( nWinStyle & WB_RIGHT ) )
            rStateRect.Left() = rPos.X()+rSize.Width()-rImageSize.Width();
        else
            rStateRect.Left() = rPos.X();
        if ( nWinStyle & WB_VCENTER )
            rStateRect.Top() = rPos.Y()+((rSize.Height()-rImageSize.Height())/2);
        else if ( nWinStyle & WB_BOTTOM )
            rStateRect.Top() = rPos.Y()+rSize.Height()-rImageSize.Height();
        else
            rStateRect.Top() = rPos.Y();
        rStateRect.Right()  = rStateRect.Left()+rImageSize.Width()-1;
        rStateRect.Bottom() = rStateRect.Top()+rImageSize.Height()-1;
        // provide space for focusrect
        // note: this assumes that the control's size was adjusted
        // accordingly in Get/LoseFocus, so the onscreen position won't change
        if( HasFocus() )
            rStateRect.Move( 1, 1 );
        rMouseRect          = rStateRect;

        ImplSetFocusRect( rStateRect );
    }

    const int nLineSpace = 4;
    if( (GetStyle() & WB_CBLINESTYLE) != 0 &&
        rMouseRect.Right()-1-nLineSpace < rPos.X()+rSize.Width() )
    {
        const StyleSettings&    rStyleSettings = GetSettings().GetStyleSettings();
        if ( rStyleSettings.GetOptions() & STYLE_OPTION_MONO )
            SetLineColor( Color( COL_BLACK ) );
        else
            SetLineColor( rStyleSettings.GetShadowColor() );
        long nLineX = rMouseRect.Right()+nLineSpace;
        DrawLine( Point( nLineX, nLineY ), Point( rPos.X() + rSize.Width()-1, nLineY ) );
        if ( !(rStyleSettings.GetOptions() & STYLE_OPTION_MONO) )
        {
            SetLineColor( rStyleSettings.GetLightColor() );
            DrawLine( Point( nLineX, nLineY+1 ), Point( rPos.X() + rSize.Width()-1, nLineY+1 ) );
        }
    }

    pDev->Pop();
}

void CheckBox::ImplDrawCheckBox( bool bLayout )
{
    Size aImageSize = ImplGetCheckImageSize();
    aImageSize.Width()  = CalcZoom( aImageSize.Width() );
    aImageSize.Height() = CalcZoom( aImageSize.Height() );

    if( !bLayout )
        HideFocus();

    ImplDraw( this, 0, Point(), GetOutputSizePixel(), aImageSize,
              maStateRect, maMouseRect, bLayout );

    if( !bLayout )
    {
        ImplDrawCheckBoxState();
        if ( HasFocus() )
            ShowFocus( ImplGetFocusRect() );
    }
}

void CheckBox::ImplCheck()
{
    TriState eNewState;
    if ( meState == TRISTATE_FALSE )
        eNewState = TRISTATE_TRUE;
    else if ( !mbTriState )
        eNewState = TRISTATE_FALSE;
    else if ( meState == TRISTATE_TRUE )
        eNewState = TRISTATE_INDET;
    else
        eNewState = TRISTATE_FALSE;
    meState = eNewState;

    ImplDelData aDelData;
    ImplAddDel( &aDelData );
    if( (GetStyle() & WB_EARLYTOGGLE) )
        Toggle();
    ImplInvalidateOrDrawCheckBoxState();
    if( ! (GetStyle() & WB_EARLYTOGGLE) )
        Toggle();
    if ( aDelData.IsDead() )
        return;
    ImplRemoveDel( &aDelData );
    Click();
}

CheckBox::CheckBox( vcl::Window* pParent, WinBits nStyle ) :
    Button( WINDOW_CHECKBOX ), mbLegacyNoTextAlign( false )
{
    ImplInitCheckBoxData();
    ImplInit( pParent, nStyle );
}

CheckBox::CheckBox( vcl::Window* pParent, const ResId& rResId ) :
    Button( WINDOW_CHECKBOX ), mbLegacyNoTextAlign( false )
{
    rResId.SetRT( RSC_CHECKBOX );
    WinBits nStyle = ImplInitRes( rResId );
    ImplInitCheckBoxData();
    ImplInit( pParent, nStyle );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE) )
        Show();
}

void CheckBox::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( rMEvt.IsLeft() && maMouseRect.IsInside( rMEvt.GetPosPixel() ) )
    {
        ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
        ImplInvalidateOrDrawCheckBoxState();
        StartTracking();
        return;
    }

    Button::MouseButtonDown( rMEvt );
}

void CheckBox::Tracking( const TrackingEvent& rTEvt )
{
    if ( rTEvt.IsTrackingEnded() )
    {
        if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
        {
            if ( !(GetStyle() & WB_NOPOINTERFOCUS) && !rTEvt.IsTrackingCanceled() )
                GrabFocus();

            ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;

            // do not call click handler if aborted
            if ( !rTEvt.IsTrackingCanceled() )
                ImplCheck();
            else
                ImplInvalidateOrDrawCheckBoxState();
        }
    }
    else
    {
        if ( maMouseRect.IsInside( rTEvt.GetMouseEvent().GetPosPixel() ) )
        {
            if ( !(ImplGetButtonState() & BUTTON_DRAW_PRESSED) )
            {
                ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
                ImplInvalidateOrDrawCheckBoxState();
            }
        }
        else
        {
            if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
            {
                ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
                ImplInvalidateOrDrawCheckBoxState();
            }
        }
    }
}

void CheckBox::KeyInput( const KeyEvent& rKEvt )
{
    vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

    if ( !aKeyCode.GetModifier() && (aKeyCode.GetCode() == KEY_SPACE) )
    {
        if ( !(ImplGetButtonState() & BUTTON_DRAW_PRESSED) )
        {
            ImplGetButtonState() |= BUTTON_DRAW_PRESSED;
            ImplInvalidateOrDrawCheckBoxState();
        }
    }
    else if ( (ImplGetButtonState() & BUTTON_DRAW_PRESSED) && (aKeyCode.GetCode() == KEY_ESCAPE) )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        ImplInvalidateOrDrawCheckBoxState();
    }
    else
        Button::KeyInput( rKEvt );
}

void CheckBox::KeyUp( const KeyEvent& rKEvt )
{
    vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

    if ( (ImplGetButtonState() & BUTTON_DRAW_PRESSED) && (aKeyCode.GetCode() == KEY_SPACE) )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        ImplCheck();
    }
    else
        Button::KeyUp( rKEvt );
}

void CheckBox::FillLayoutData() const
{
    mpControlData->mpLayoutData = new vcl::ControlLayoutData();
    const_cast<CheckBox*>(this)->ImplDrawCheckBox( true );
}

void CheckBox::Paint( const Rectangle& )
{
    ImplDrawCheckBox();
}

void CheckBox::Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize,
                     sal_uLong nFlags )
{
    MapMode     aResMapMode( MAP_100TH_MM );
    Point       aPos  = pDev->LogicToPixel( rPos );
    Size        aSize = pDev->LogicToPixel( rSize );
    Size        aImageSize = pDev->LogicToPixel( Size( 300, 300 ), aResMapMode );
    Size        aBrd1Size = pDev->LogicToPixel( Size( 20, 20 ), aResMapMode );
    Size        aBrd2Size = pDev->LogicToPixel( Size( 30, 30 ), aResMapMode );
    long        nCheckWidth = pDev->LogicToPixel( Size( 20, 20 ), aResMapMode ).Width();
    vcl::Font   aFont = GetDrawPixelFont( pDev );
    Rectangle   aStateRect;
    Rectangle   aMouseRect;

    aImageSize.Width()  = CalcZoom( aImageSize.Width() );
    aImageSize.Height() = CalcZoom( aImageSize.Height() );
    aBrd1Size.Width()   = CalcZoom( aBrd1Size.Width() );
    aBrd1Size.Height()  = CalcZoom( aBrd1Size.Height() );
    aBrd2Size.Width()   = CalcZoom( aBrd2Size.Width() );
    aBrd2Size.Height()  = CalcZoom( aBrd2Size.Height() );

    if ( !aBrd1Size.Width() )
        aBrd1Size.Width() = 1;
    if ( !aBrd1Size.Height() )
        aBrd1Size.Height() = 1;
    if ( !aBrd2Size.Width() )
        aBrd2Size.Width() = 1;
    if ( !aBrd2Size.Height() )
        aBrd2Size.Height() = 1;
    if ( !nCheckWidth )
        nCheckWidth = 1;

    pDev->Push();
    pDev->SetMapMode();
    pDev->SetFont( aFont );
    if ( nFlags & WINDOW_DRAW_MONO )
        pDev->SetTextColor( Color( COL_BLACK ) );
    else
        pDev->SetTextColor( GetTextColor() );
    pDev->SetTextFillColor();

    ImplDraw( pDev, nFlags, aPos, aSize,
              aImageSize, aStateRect, aMouseRect, false );

    pDev->SetLineColor();
    pDev->SetFillColor( Color( COL_BLACK ) );
    pDev->DrawRect( aStateRect );
    aStateRect.Left()   += aBrd1Size.Width();
    aStateRect.Top()    += aBrd1Size.Height();
    aStateRect.Right()  -= aBrd1Size.Width();
    aStateRect.Bottom() -= aBrd1Size.Height();
    if ( meState == TRISTATE_INDET )
        pDev->SetFillColor( Color( COL_LIGHTGRAY ) );
    else
        pDev->SetFillColor( Color( COL_WHITE ) );
    pDev->DrawRect( aStateRect );

    if ( meState == TRISTATE_TRUE )
    {
        aStateRect.Left()   += aBrd2Size.Width();
        aStateRect.Top()    += aBrd2Size.Height();
        aStateRect.Right()  -= aBrd2Size.Width();
        aStateRect.Bottom() -= aBrd2Size.Height();
        Point   aPos11( aStateRect.TopLeft() );
        Point   aPos12( aStateRect.BottomRight() );
        Point   aPos21( aStateRect.TopRight() );
        Point   aPos22( aStateRect.BottomLeft() );
        Point   aTempPos11( aPos11 );
        Point   aTempPos12( aPos12 );
        Point   aTempPos21( aPos21 );
        Point   aTempPos22( aPos22 );
        pDev->SetLineColor( Color( COL_BLACK ) );
        long nDX = 0;
        for ( long i = 0; i < nCheckWidth; i++ )
        {
            if ( !(i % 2) )
            {
                aTempPos11.X() = aPos11.X()+nDX;
                aTempPos12.X() = aPos12.X()+nDX;
                aTempPos21.X() = aPos21.X()+nDX;
                aTempPos22.X() = aPos22.X()+nDX;
            }
            else
            {
                nDX++;
                aTempPos11.X() = aPos11.X()-nDX;
                aTempPos12.X() = aPos12.X()-nDX;
                aTempPos21.X() = aPos21.X()-nDX;
                aTempPos22.X() = aPos22.X()-nDX;
            }
            pDev->DrawLine( aTempPos11, aTempPos12 );
            pDev->DrawLine( aTempPos21, aTempPos22 );
        }
    }

    pDev->Pop();
}

void CheckBox::Resize()
{
    Control::Resize();
    Invalidate();
}

void CheckBox::GetFocus()
{
    if ( GetText().isEmpty() || (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) )
    {
        // increase button size to have space for focus rect
        // checkboxes without text will draw focusrect around the check
        // See CheckBox::ImplDraw()
        Point aPos( GetPosPixel() );
        Size aSize( GetSizePixel() );
        aPos.Move(-1,-1);
        aSize.Height() += 2;
        aSize.Width() += 2;
        setPosSizePixel( aPos.X(), aPos.Y(), aSize.Width(), aSize.Height(), WINDOW_POSSIZE_ALL );
        ImplDrawCheckBox();
    }
    else
        ShowFocus( ImplGetFocusRect() );

    SetInputContext( InputContext( GetFont() ) );
    Button::GetFocus();
}

void CheckBox::LoseFocus()
{
    if ( ImplGetButtonState() & BUTTON_DRAW_PRESSED )
    {
        ImplGetButtonState() &= ~BUTTON_DRAW_PRESSED;
        ImplInvalidateOrDrawCheckBoxState();
    }

    HideFocus();
    Button::LoseFocus();

    if ( GetText().isEmpty() || (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) )
    {
        // decrease button size again (see GetFocus())
        // checkboxes without text will draw focusrect around the check
        Point aPos( GetPosPixel() );
        Size aSize( GetSizePixel() );
        aPos.Move(1,1);
        aSize.Height() -= 2;
        aSize.Width() -= 2;
        setPosSizePixel( aPos.X(), aPos.Y(), aSize.Width(), aSize.Height(), WINDOW_POSSIZE_ALL );
        ImplDrawCheckBox();
    }
}

void CheckBox::StateChanged( StateChangedType nType )
{
    Button::StateChanged( nType );

    if ( nType == StateChangedType::STATE )
    {
        if ( IsReallyVisible() && IsUpdateMode() )
            Invalidate( maStateRect );
    }
    else if ( (nType == StateChangedType::ENABLE) ||
              (nType == StateChangedType::TEXT) ||
              (nType == StateChangedType::IMAGE) ||
              (nType == StateChangedType::DATA) ||
              (nType == StateChangedType::UPDATEMODE) )
    {
        if ( IsUpdateMode() )
            Invalidate();
    }
    else if ( nType == StateChangedType::STYLE )
    {
        SetStyle( ImplInitStyle( GetWindow( WINDOW_PREV ), GetStyle() ) );

        if ( (GetPrevStyle() & CHECKBOX_VIEW_STYLE) !=
             (GetStyle() & CHECKBOX_VIEW_STYLE) )
        {
            if ( IsUpdateMode() )
                Invalidate();
        }
    }
    else if ( (nType == StateChangedType::ZOOM) ||
              (nType == StateChangedType::CONTROLFONT) )
    {
        ImplInitSettings( true, false, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLFOREGROUND )
    {
        ImplInitSettings( false, true, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLBACKGROUND )
    {
        ImplInitSettings( false, false, true );
        Invalidate();
    }
}

void CheckBox::DataChanged( const DataChangedEvent& rDCEvt )
{
    Button::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_FONTS) ||
         (rDCEvt.GetType() == DATACHANGED_FONTSUBSTITUTION) ||
         ((rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
          (rDCEvt.GetFlags() & SETTINGS_STYLE)) )
    {
        ImplInitSettings( true, true, true );
        Invalidate();
    }
}

bool CheckBox::PreNotify( NotifyEvent& rNEvt )
{
    const MouseEvent* pMouseEvt = NULL;

    if( (rNEvt.GetType() == EVENT_MOUSEMOVE) && (pMouseEvt = rNEvt.GetMouseEvent()) != NULL )
    {
#ifndef USE_JAVA
        if( !pMouseEvt->GetButtons() && !pMouseEvt->IsSynthetic() && !pMouseEvt->IsModifierChanged() )
        {
            // trigger redraw if mouse over state has changed
            if( IsNativeControlSupported(CTRL_CHECKBOX, PART_ENTIRE_CONTROL) )
            {
                if( ( maMouseRect.IsInside( GetPointerPosPixel()) &&
                     !maMouseRect.IsInside( GetLastPointerPosPixel()) ) ||
                    ( maMouseRect.IsInside( GetLastPointerPosPixel()) &&
                     !maMouseRect.IsInside( GetPointerPosPixel()) ) ||
                    pMouseEvt->IsLeaveWindow() || pMouseEvt->IsEnterWindow() )
                {
                    Invalidate( maStateRect );
                }
            }
        }
#endif     // !USE_JAVA
    }

    return Button::PreNotify(rNEvt);
}

void CheckBox::Toggle()
{
    ImplCallEventListenersAndHandler( VCLEVENT_CHECKBOX_TOGGLE, maToggleHdl, this );
}

void CheckBox::SetState( TriState eState )
{
    if ( !mbTriState && (eState == TRISTATE_INDET) )
        eState = TRISTATE_FALSE;

    if ( meState != eState )
    {
        meState = eState;
        StateChanged( StateChangedType::STATE );
        Toggle();
    }
}

bool CheckBox::set_property(const OString &rKey, const OString &rValue)
{
    if (rKey == "active")
        SetState(toBool(rValue) ? TRISTATE_TRUE : TRISTATE_FALSE);
    else
        return Button::set_property(rKey, rValue);
    return true;
}

void CheckBox::EnableTriState( bool bTriState )
{
    if ( mbTriState != bTriState )
    {
        mbTriState = bTriState;

        if ( !bTriState && (meState == TRISTATE_INDET) )
            SetState( TRISTATE_FALSE );
    }
}

long CheckBox::ImplGetImageToTextDistance() const
{
    // 4 pixels, but take zoom into account, so the text doesn't "jump" relative to surrounding elements,
    // which might have been aligned with the text of the check box
    return CalcZoom( 4 );
}

Size CheckBox::ImplGetCheckImageSize() const
{
    Size aSize;
    bool bDefaultSize = true;
    if( IsNativeControlSupported( CTRL_CHECKBOX, PART_ENTIRE_CONTROL ) )
    {
        ImplControlValue aControlValue;
        // #i45896# workaround gcc3.3 temporary problem
        Rectangle        aCtrlRegion( Point( 0, 0 ), GetSizePixel() );
        ControlState     nState = CTRL_STATE_DEFAULT|CTRL_STATE_ENABLED;
        Rectangle aBoundingRgn, aContentRgn;

        // get native size of a check box
        if( GetNativeControlRegion( CTRL_CHECKBOX, PART_ENTIRE_CONTROL, aCtrlRegion,
                                           nState, aControlValue, OUString(),
                                           aBoundingRgn, aContentRgn ) )
        {
            aSize = aContentRgn.GetSize();
            bDefaultSize = false;
        }
    }
    if( bDefaultSize )
        aSize = GetCheckImage( GetSettings(), 0 ).GetSizePixel();
    return aSize;
}

Image CheckBox::GetCheckImage( const AllSettings& rSettings, sal_uInt16 nFlags )
{
    ImplSVData*             pSVData = ImplGetSVData();
    const StyleSettings&    rStyleSettings = rSettings.GetStyleSettings();
    sal_uInt16              nStyle = 0;

    if ( rStyleSettings.GetOptions() & STYLE_OPTION_MONO )
        nStyle = STYLE_CHECKBOX_MONO;

    if ( !pSVData->maCtrlData.mpCheckImgList ||
         (pSVData->maCtrlData.mnCheckStyle != nStyle) ||
         (pSVData->maCtrlData.mnLastCheckFColor != rStyleSettings.GetFaceColor().GetColor()) ||
         (pSVData->maCtrlData.mnLastCheckWColor != rStyleSettings.GetWindowColor().GetColor()) ||
         (pSVData->maCtrlData.mnLastCheckLColor != rStyleSettings.GetLightColor().GetColor()) )
    {
        if ( pSVData->maCtrlData.mpCheckImgList )
            delete pSVData->maCtrlData.mpCheckImgList;

        pSVData->maCtrlData.mnLastCheckFColor = rStyleSettings.GetFaceColor().GetColor();
        pSVData->maCtrlData.mnLastCheckWColor = rStyleSettings.GetWindowColor().GetColor();
        pSVData->maCtrlData.mnLastCheckLColor = rStyleSettings.GetLightColor().GetColor();

        ResMgr* pResMgr = ImplGetResMgr();
        pSVData->maCtrlData.mpCheckImgList = new ImageList();
        if( pResMgr )
            LoadThemedImageList( rStyleSettings,
                pSVData->maCtrlData.mpCheckImgList,
                ResId( SV_RESID_BITMAP_CHECK+nStyle, *pResMgr ), 9 );
        pSVData->maCtrlData.mnCheckStyle = nStyle;
    }

    sal_uInt16 nId;
    if ( nFlags & BUTTON_DRAW_DISABLED )
    {
        if ( nFlags & BUTTON_DRAW_DONTKNOW )
            nId = 9;
        else if ( nFlags & BUTTON_DRAW_CHECKED )
            nId = 6;
        else
            nId = 5;
    }
    else if ( nFlags & BUTTON_DRAW_PRESSED )
    {
        if ( nFlags & BUTTON_DRAW_DONTKNOW )
            nId = 8;
        else if ( nFlags & BUTTON_DRAW_CHECKED )
            nId = 4;
        else
            nId = 3;
    }
    else
    {
        if ( nFlags & BUTTON_DRAW_DONTKNOW )
            nId = 7;
        else if ( nFlags & BUTTON_DRAW_CHECKED )
            nId = 2;
        else
            nId = 1;
    }
    return pSVData->maCtrlData.mpCheckImgList->GetImage( nId );
}

void CheckBox::ImplSetMinimumNWFSize()
{
    Push( PushFlags::MAPMODE );
    SetMapMode( MAP_PIXEL );

    ImplControlValue aControlValue;
    Size aCurSize( GetSizePixel() );
    Rectangle aCtrlRegion( Point( 0, 0 ), aCurSize );
    Rectangle aBoundingRgn, aContentRgn;

    // get native size of a radiobutton
    if( GetNativeControlRegion( CTRL_CHECKBOX, PART_ENTIRE_CONTROL, aCtrlRegion,
                                CTRL_STATE_DEFAULT|CTRL_STATE_ENABLED, aControlValue, OUString(),
                                aBoundingRgn, aContentRgn ) )
    {
        Size aSize = aContentRgn.GetSize();

        if( aSize.Height() > aCurSize.Height() )
        {
            aCurSize.Height() = aSize.Height();
            SetSizePixel( aCurSize );
        }
    }

    Pop();
}

Size CheckBox::CalcMinimumSize( long nMaxWidth ) const
{
    Size aSize = ImplGetCheckImageSize();
    nMaxWidth -= aSize.Width();

    OUString aText = GetText();
    if ( !aText.isEmpty() && ! (ImplGetButtonState() & BUTTON_DRAW_NOTEXT) )
    {
        // subtract what will be added later
        nMaxWidth-=2;
        nMaxWidth -= ImplGetImageToTextDistance();

        Size aTextSize = GetTextRect( Rectangle( Point(), Size( nMaxWidth > 0 ? nMaxWidth : 0x7fffffff, 0x7fffffff ) ),
                                      aText, FixedText::ImplGetTextStyle( GetStyle() ) ).GetSize();
        aSize.Width()+=2;    // for focus rect
        aSize.Width() += ImplGetImageToTextDistance();
        aSize.Width() += aTextSize.Width();
        if ( aSize.Height() < aTextSize.Height() )
            aSize.Height() = aTextSize.Height();
    }
    else
    {
        // is this still correct ? since the checkbox now
        // shows a focus rect it should be 2 pixels wider and longer
/* da ansonsten im Writer die Control zu weit oben haengen
        aSize.Width() += 2;
        aSize.Height() += 2;
*/
    }

    return CalcWindowSize( aSize );
}

Size CheckBox::GetOptimalSize() const
{
    return CalcMinimumSize();
}

ImageButton::ImageButton( vcl::Window* pParent, WinBits nStyle ) :
    PushButton( pParent, nStyle )
{
    ImplInitStyle();
}

ImageButton::ImageButton( vcl::Window* pParent, const ResId& rResId ) :
    PushButton( pParent, rResId.SetRT( RSC_IMAGEBUTTON ) )
{
    sal_uLong nObjMask = ReadLongRes();

    if ( RSC_IMAGEBUTTON_IMAGE & nObjMask )
    {
        SetModeImage( Image( ResId( (RSHEADER_TYPE*)GetClassRes(), *rResId.GetResMgr() ) ) );
        IncrementRes( GetObjSizeRes( (RSHEADER_TYPE*)GetClassRes() ) );
    }

    if ( RSC_IMAGEBUTTON_SYMBOL & nObjMask )
        SetSymbol( (SymbolType)ReadLongRes() );

    if ( RSC_IMAGEBUTTON_STATE & nObjMask )
        SetState( (TriState)ReadLongRes() );

    ImplInitStyle();
}

ImageButton::~ImageButton()
{
}

void ImageButton::ImplInitStyle()
{
    WinBits nStyle = GetStyle();

    if ( ! ( nStyle & ( WB_RIGHT | WB_LEFT ) ) )
        nStyle |= WB_CENTER;

    if ( ! ( nStyle & ( WB_TOP | WB_BOTTOM ) ) )
        nStyle |= WB_VCENTER;

    SetStyle( nStyle );
}

ImageRadioButton::ImageRadioButton( vcl::Window* pParent, WinBits nStyle ) :
    RadioButton( pParent, nStyle )
{
}

ImageRadioButton::~ImageRadioButton()
{
}

TriStateBox::TriStateBox( vcl::Window* pParent, WinBits nStyle ) :
    CheckBox( pParent, nStyle )
{
    EnableTriState( true );
}

TriStateBox::~TriStateBox()
{
}

DisclosureButton::DisclosureButton( vcl::Window* pParent, WinBits nStyle ) :
    CheckBox( pParent, nStyle )
{
}

void DisclosureButton::ImplDrawCheckBoxState()
{
    /* HACK: DisclosureButton is currently assuming, that the disclosure sign
       will fit into the rectangle occupied by a normal checkbox on all themes.
       If this does not hold true for some theme, ImplGetCheckImageSize
       would have to be overloaded for DisclosureButton; also GetNativeControlRegion
       for CTRL_LISTNODE would have to be implemented and taken into account
    */

    Rectangle aStateRect( GetStateRect() );

    ImplControlValue    aControlValue( GetState() == TRISTATE_TRUE ? BUTTONVALUE_ON : BUTTONVALUE_OFF );
    Rectangle           aCtrlRegion( aStateRect );
    ControlState        nState = 0;

    if ( HasFocus() )                       nState |= CTRL_STATE_FOCUSED;
    if ( ImplGetButtonState() & BUTTON_DRAW_DEFAULT )   nState |= CTRL_STATE_DEFAULT;
    if ( Window::IsEnabled() )              nState |= CTRL_STATE_ENABLED;
    if ( IsMouseOver() && GetMouseRect().IsInside( GetPointerPosPixel() ) )
        nState |= CTRL_STATE_ROLLOVER;

    if( ! DrawNativeControl( CTRL_LISTNODE, PART_ENTIRE_CONTROL, aCtrlRegion, nState,
                           aControlValue, OUString() ) )
    {
        ImplSVCtrlData& rCtrlData( ImplGetSVData()->maCtrlData );
        if( ! rCtrlData.mpDisclosurePlus )
            rCtrlData.mpDisclosurePlus = new Image( BitmapEx( VclResId( SV_DISCLOSURE_PLUS ) ) );
        if( ! rCtrlData.mpDisclosureMinus )
            rCtrlData.mpDisclosureMinus = new Image( BitmapEx( VclResId( SV_DISCLOSURE_MINUS ) ) );

        Image* pImg = NULL;
        pImg = IsChecked() ? rCtrlData.mpDisclosureMinus : rCtrlData.mpDisclosurePlus;

        DBG_ASSERT( pImg, "no disclosure image" );
        if( ! pImg )
            return;

        sal_uInt16 nStyle = 0;
        if( ! IsEnabled() )
            nStyle |= IMAGE_DRAW_DISABLE;

        Size aSize( aStateRect.GetSize() );
        Size aImgSize( pImg->GetSizePixel() );
        Point aOff( (aSize.Width() - aImgSize.Width())/2,
                    (aSize.Height() - aImgSize.Height())/2 );
        aOff += aStateRect.TopLeft();
        DrawImage( aOff, *pImg, nStyle );
    }
}

void DisclosureButton::KeyInput( const KeyEvent& rKEvt )
{
    vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

    if( !aKeyCode.GetModifier()  &&
        ( ( aKeyCode.GetCode() == KEY_ADD ) ||
          ( aKeyCode.GetCode() == KEY_SUBTRACT ) )
        )
    {
        Check( aKeyCode.GetCode() == KEY_ADD );
    }
    else
        CheckBox::KeyInput( rKEvt );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
