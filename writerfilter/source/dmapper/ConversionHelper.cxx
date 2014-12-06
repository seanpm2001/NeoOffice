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
#include <ConversionHelper.hxx>
#include <com/sun/star/table/BorderLine2.hpp>
#include <com/sun/star/table/BorderLineStyle.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/text/HoriOrientation.hpp>
#include <com/sun/star/style/NumberingType.hpp>
#if SUPD == 310
#include <svx/borderline.hxx>
#else	// SUPD == 310
#include <editeng/borderline.hxx>
#endif	// SUPD == 310
#include <ooxml/resourceids.hxx>
#include <rtl/ustrbuf.hxx>
#include <tools/color.hxx>
#include <tools/mapunit.hxx>
#include <algorithm>
#include <functional>

using namespace com::sun::star;

namespace writerfilter {
namespace dmapper{
namespace ConversionHelper{

#if SUPD == 310

//line definitions in 1/100 mm
#define LINE_WIDTH_0            2
#define LINE_WIDTH_1            36
#define LINE_WIDTH_2            89
#define LINE_WIDTH_3            142
#define LINE_WIDTH_4            177
#define LINE_WIDTH_5            18

#define DOUBLE_LINE0_OUT    LINE_WIDTH_0
#define DOUBLE_LINE0_IN     LINE_WIDTH_0
#define DOUBLE_LINE0_DIST   LINE_WIDTH_1

#define DOUBLE_LINE1_OUT    LINE_WIDTH_1
#define DOUBLE_LINE1_IN     LINE_WIDTH_1
#define DOUBLE_LINE1_DIST   LINE_WIDTH_1

#define DOUBLE_LINE2_OUT    LINE_WIDTH_2
#define DOUBLE_LINE2_IN     LINE_WIDTH_2
#define DOUBLE_LINE2_DIST   LINE_WIDTH_2

#define DOUBLE_LINE3_OUT    LINE_WIDTH_2
#define DOUBLE_LINE3_IN     LINE_WIDTH_1
#define DOUBLE_LINE3_DIST   LINE_WIDTH_2

#define DOUBLE_LINE4_OUT    LINE_WIDTH_1
#define DOUBLE_LINE4_IN     LINE_WIDTH_2
#define DOUBLE_LINE4_DIST   LINE_WIDTH_1

#define DOUBLE_LINE5_OUT    LINE_WIDTH_3
#define DOUBLE_LINE5_IN     LINE_WIDTH_2
#define DOUBLE_LINE5_DIST   LINE_WIDTH_2

#define DOUBLE_LINE6_OUT    LINE_WIDTH_2
#define DOUBLE_LINE6_IN     LINE_WIDTH_3
#define DOUBLE_LINE6_DIST   LINE_WIDTH_2

#define DOUBLE_LINE7_OUT    LINE_WIDTH_0
#define DOUBLE_LINE7_IN     LINE_WIDTH_0
#define DOUBLE_LINE7_DIST   LINE_WIDTH_2

#define DOUBLE_LINE8_OUT    LINE_WIDTH_1
#define DOUBLE_LINE8_IN     LINE_WIDTH_0
#define DOUBLE_LINE8_DIST   LINE_WIDTH_2

#define DOUBLE_LINE9_OUT    LINE_WIDTH_2
#define DOUBLE_LINE9_IN     LINE_WIDTH_0
#define DOUBLE_LINE9_DIST   LINE_WIDTH_2

#define DOUBLE_LINE10_OUT   LINE_WIDTH_3
#define DOUBLE_LINE10_IN    LINE_WIDTH_0
#define DOUBLE_LINE10_DIST  LINE_WIDTH_2

#endif	// SUPD == 310

sal_Int32 MakeBorderLine( sal_Int32 nSprmValue, table::BorderLine2& rToFill )
{
    //TODO: Lines are always solid
    //Border
    //borders are defined as:
    // 0x XX XX XX XX
    //    || || || ||
    //    || || ||  ---- Line width in 1/8 pt
    //    || || ||
    //    || ||  ------- Line type: 0 - none 1 - single ... 25 - engrave 3D and 64 - 230 page borders
    //    || ||
    //    ||  ---------- Line color
    //    ||
    //     ------------- seven bits line space
    //    -------------- first bit: with shading
    sal_Int16 nLineThicknessTwip = (sal_Int16)((nSprmValue & 0xff) * 20)/8L ;
    sal_Int32 nLineType       = ((nSprmValue & 0xff00) >> 8);
    sal_Int32 nLineColor    = (nSprmValue & 0xff0000)>>16;
    sal_Int32 nLineDistance = (((nSprmValue & 0x3f000000)>>24) * 2540 + 36)/72L;
    MakeBorderLine( nLineThicknessTwip, nLineType, nLineColor, rToFill, false);
    return nLineDistance;
}
void MakeBorderLine( sal_Int32 nLineThickness,   sal_Int32 nLineType,
                                            sal_Int32 nLineColor,
                                            table::BorderLine2& rToFill, bool bIsOOXML )
{
    static const sal_Int32 aBorderDefColor[] =
    {
        // The first item means automatic color (COL_AUTO), but we
        // do not use it anyway (see the next statement) .-)
        0, COL_BLACK, COL_LIGHTBLUE, COL_LIGHTCYAN, COL_LIGHTGREEN,
        COL_LIGHTMAGENTA, COL_LIGHTRED, COL_YELLOW, COL_WHITE, COL_BLUE,
        COL_CYAN, COL_GREEN, COL_MAGENTA, COL_RED, COL_BROWN, COL_GRAY,
        COL_LIGHTGRAY
    };
    //no auto color for borders
    if(!nLineColor)
        ++nLineColor;
    if(!bIsOOXML && sal::static_int_cast<sal_uInt32>(nLineColor) < SAL_N_ELEMENTS(aBorderDefColor))
        nLineColor = aBorderDefColor[nLineColor];

#if SUPD == 310
    enum eBorderCode
    {
        single0, single1, single2, single3, single4, single5,
        double0, double1, double2, double3, double4, double5, double6,
        double7, double8, double9, double10,
        none
    } eCodeIdx = none;

    // Map to our border types, we should use of one equal line
    // thickness, or one of smaller thickness. If too small we
    // can make the defecit up in additional white space or
    // object size
    switch(nLineType)
    {
        // First the single lines
        case  1: break;
        case  2:
        case  5:
        // and the unsupported special cases which we map to a single line
        case  6:
        case  7:
        case  8:
        case  9:
        case 22:
        // or if in necessary by a double line
        case 24:
        case 25:
            if( nLineThickness < 10)
                eCodeIdx = single0;//   1 Twip for us
            else if( nLineThickness < 20)
                eCodeIdx = single5;//   10 Twips for us
            else if (nLineThickness < 50)
                eCodeIdx = single1;//  20 Twips
            else if (nLineThickness < 80)
                eCodeIdx = single2;//  50
            else if (nLineThickness < 100)
                eCodeIdx = single3;//  80
            else if (nLineThickness < 150)
                eCodeIdx = single4;// 100
            // Hack: for the quite thick lines we must paint double lines,
            // because our singles lines don't come thicker than 5 points.
            else if (nLineThickness < 180)
                eCodeIdx = double2;// 150
            else
                eCodeIdx = double5;// 180
        break;
        // then the shading beams which we represent by a double line
        case 23:
            eCodeIdx = double1;
        break;
        // then the double lines, for which we have good matches
        case  3:
        case 10: //Don't have tripple so use double
            if (nLineThickness < 60)
                eCodeIdx = double0;// 22 Twips for us
            else if (nLineThickness < 135)
                eCodeIdx = double7;// some more space
            else if (nLineThickness < 180)
                eCodeIdx = double1;// 60
            else
                eCodeIdx = double2;// 150
            break;
        case 11:
            eCodeIdx = double4;//  90 Twips for us
            break;
        case 12:
        case 13: //Don't have thin thick thin, so use thick thin
            if (nLineThickness < 87)
                eCodeIdx = double8;//  71 Twips for us
            else if (nLineThickness < 117)
                eCodeIdx = double9;// 101
            else if (nLineThickness < 166)
                eCodeIdx = double10;// 131
            else
                eCodeIdx = double5;// 180
            break;
        case 14:
            if (nLineThickness < 46)
                eCodeIdx = double0;//  22 Twips for us
            else if (nLineThickness < 76)
                eCodeIdx = double1;//  60
            else if (nLineThickness < 121)
                eCodeIdx = double4;//  90
            else if (nLineThickness < 166)
                eCodeIdx = double2;// 150
            else
                eCodeIdx = double6;// 180
            break;
        case 15:
        case 16: //Don't have thin thick thin, so use thick thin
            if (nLineThickness < 46)
                eCodeIdx = double0;//  22 Twips for us
            else if (nLineThickness < 76)
                eCodeIdx = double1;//  60
            else if (nLineThickness < 121)
                eCodeIdx = double3;//  90
            else if (nLineThickness < 166)
                eCodeIdx = double2;// 150
            else
                eCodeIdx = double5;// 180
            break;
        case 17:
            if (nLineThickness < 46)
                eCodeIdx = double0;//  22 Twips for us
            else if (nLineThickness < 72)
                eCodeIdx = double7;//  52
            else if (nLineThickness < 137)
                eCodeIdx = double4;//  90
            else
                eCodeIdx = double6;// 180
        break;
        case 18:
        case 19: //Don't have thin thick thin, so use thick thin
            if (nLineThickness < 46)
                eCodeIdx = double0;//  22 Twips for us
            else if (nLineThickness < 62)
                eCodeIdx = double7;//  52
            else if (nLineThickness < 87)
                eCodeIdx = double8;//  71
            else if (nLineThickness < 117)
                eCodeIdx = double9;// 101
            else if (nLineThickness < 156)
                eCodeIdx = double10;// 131
            else
                eCodeIdx = double5;// 180
            break;
        case 20:
            if (nLineThickness < 46)
                eCodeIdx = single1; //  20 Twips for us
            else
                eCodeIdx = double1;//  60
            break;
        case 21:
            eCodeIdx = double1;//  60 Twips for us
            break;
        case 0:
        case 255:
            eCodeIdx = none;
            break;
        default:
            eCodeIdx = single0;
            break;
    }
    struct BorderDefinition
    {
        sal_Int16 nOut;
        sal_Int16 nIn;
        sal_Int16 nDist;
    };


    static const BorderDefinition aLineTab[] =
    {
        /* 0*/  { LINE_WIDTH_0, 0, 0 },
        /* 1*/  { LINE_WIDTH_1, 0, 0 },
        /* 2*/  { LINE_WIDTH_2, 0, 0 },
        /* 3*/  { LINE_WIDTH_3, 0, 0 },
        /* 4*/  { LINE_WIDTH_4, 0, 0 },
        /* 5*/  { LINE_WIDTH_5, 0, 0 },
        /* 6*/  { DOUBLE_LINE0_OUT, DOUBLE_LINE0_IN, DOUBLE_LINE0_DIST },
        /* 7*/  { DOUBLE_LINE1_OUT, DOUBLE_LINE1_IN, DOUBLE_LINE1_DIST },
        /* 8*/  { DOUBLE_LINE2_OUT, DOUBLE_LINE2_IN, DOUBLE_LINE2_DIST },
        /* 9*/  { DOUBLE_LINE3_OUT, DOUBLE_LINE3_IN, DOUBLE_LINE3_DIST },
        /*10*/  { DOUBLE_LINE4_OUT, DOUBLE_LINE4_IN, DOUBLE_LINE4_DIST },
        /*11*/  { DOUBLE_LINE5_OUT, DOUBLE_LINE5_IN, DOUBLE_LINE5_DIST },
        /*12*/  { DOUBLE_LINE6_OUT, DOUBLE_LINE6_IN, DOUBLE_LINE6_DIST },
        /*13*/  { DOUBLE_LINE7_OUT, DOUBLE_LINE7_IN, DOUBLE_LINE7_DIST },
        /*14*/  { DOUBLE_LINE8_OUT, DOUBLE_LINE8_IN, DOUBLE_LINE8_DIST },
        /*15*/  { DOUBLE_LINE9_OUT, DOUBLE_LINE9_IN, DOUBLE_LINE9_DIST },
        /*16*/  { DOUBLE_LINE10_OUT,DOUBLE_LINE10_IN,DOUBLE_LINE10_DIST},
        /*17*/  { 0, 0, 0 }
    };

    rToFill.Color = nLineColor;
    if( nLineType == 1)
    {
        rToFill.InnerLineWidth = 0;
        rToFill.OuterLineWidth = sal_Int16(nLineThickness);
        rToFill.LineDistance = 0;

    }
    else
    {
        rToFill.InnerLineWidth = aLineTab[eCodeIdx].nIn;
        rToFill.OuterLineWidth = aLineTab[eCodeIdx].nOut;
        rToFill.LineDistance = aLineTab[eCodeIdx].nDist;
    }
#endif	// SUPD == 310

    // Map to our border types, we should use of one equal line
    // thickness, or one of smaller thickness. If too small we
    // can make the defecit up in additional white space or
    // object size
    ::editeng::SvxBorderStyle const nLineStyle(
            ::editeng::ConvertBorderStyleFromWord(nLineType));
    rToFill.LineStyle = nLineStyle;
    double const fConverted( (table::BorderLineStyle::NONE == nLineStyle) ? 0.0 :
        ::editeng::ConvertBorderWidthFromWord(nLineStyle, nLineThickness,
            nLineType));
    rToFill.LineWidth = convertTwipToMM100(fConverted);
    rToFill.Color = nLineColor;
}

namespace {
void lcl_SwapQuotesInField(OUString &rFmt)
{
    //Swap unescaped " and ' with ' and "
    sal_Int32 nLen = rFmt.getLength();
    OUStringBuffer aBuffer( rFmt.getStr() );
    const sal_Unicode* pFmt = rFmt.getStr();
    for (sal_Int32 nI = 0; nI < nLen; ++nI)
    {
        if ((pFmt[nI] == '\"') && (!nI || pFmt[nI-1] != '\\'))
#if SUPD == 310
            aBuffer.setCharAt(nI, '\'');
#else	// SUPD == 310
            aBuffer[nI] = '\'';
#endif	// SUPD == 310
        else if ((pFmt[nI] == '\'') && (!nI || pFmt[nI-1] != '\\'))
#if SUPD == 310
            aBuffer.setCharAt(nI, '\"');
#else	// SUPD == 310
            aBuffer[nI] = '\"';
#endif	// SUPD == 310
    }
    rFmt = aBuffer.makeStringAndClear();
}
bool lcl_IsNotAM(OUString& rFmt, sal_Int32 nPos)
{
    return (
            (nPos == rFmt.getLength() - 1) ||
            (
            (rFmt[nPos+1] != 'M') &&
            (rFmt[nPos+1] != 'm')
            )
        );
}
}

OUString ConvertMSFormatStringToSO(
        const OUString& rFormat, lang::Locale& rLocale, bool bHijri)
{
    OUString sFormat(rFormat);
    lcl_SwapQuotesInField(sFormat);

    //#102782#, #102815#, #108341# & #111944# have to work at the same time :-)
    bool bForceJapanese(false);
    bool bForceNatNum(false);
    sal_Int32 nLen = sFormat.getLength();
    sal_Int32 nI = 0;
//    const sal_Unicode* pFormat = sFormat.getStr();
    OUStringBuffer aNewFormat( sFormat );
    while (nI < nLen)
    {
        if (aNewFormat[nI] == '\\')
            nI++;
        else if (aNewFormat[nI] == '\"')
        {
            ++nI;
            //While not at the end and not at an unescaped end quote
            while ((nI < nLen) && (!(aNewFormat[nI] == '\"') && (aNewFormat[nI-1] != '\\')))
                ++nI;
        }
        else //normal unquoted section
        {
            sal_Unicode nChar = aNewFormat[nI];
            if (nChar == 'O')
            {
#if SUPD == 310
                aNewFormat.setCharAt(nI, 'M');
#else	// SUPD == 310
                aNewFormat[nI] = 'M';
#endif	// SUPD == 310
                bForceNatNum = true;
            }
            else if (nChar == 'o')
            {
#if SUPD == 310
                aNewFormat.setCharAt(nI, 'm');
#else	// SUPD == 310
                aNewFormat[nI] = 'm';
#endif	// SUPD == 310
                bForceNatNum = true;
            }
            else if ((nChar == 'A') && lcl_IsNotAM(sFormat, nI))
            {
#if SUPD == 310
                aNewFormat.setCharAt(nI, 'D');
#else	// SUPD == 310
                aNewFormat[nI] = 'D';
#endif	// SUPD == 310
                bForceNatNum = true;
            }
            else if ((nChar == 'g') || (nChar == 'G'))
                bForceJapanese = true;
            else if ((nChar == 'a') && lcl_IsNotAM(sFormat, nI))
                bForceJapanese = true;
            else if (nChar == 'E')
            {
                if ((nI != nLen-1) && (aNewFormat[nI+1] == 'E'))
                {
                    //todo: this cannot be the right way to replace a part of the string!
#if SUPD == 310
                    aNewFormat.setCharAt(nI, 'Y');
                    aNewFormat.setCharAt(nI + 1, 'Y');
#else	// SUPD == 310
                    aNewFormat[nI] = 'Y';
                    aNewFormat[nI + 1] = 'Y';
#endif	// SUPD == 310
                    aNewFormat.insert(nI + 2, "YY");
                    nLen+=2;
                    nI+=3;
                }
                bForceJapanese = true;
            }
            else if (nChar == 'e')
            {
                if ((nI != nLen-1) && (aNewFormat[nI+1] == 'e'))
                {
                    //todo: this cannot be the right way to replace a part of the string!
#if SUPD == 310
                    aNewFormat.setCharAt(nI, 'y');
                    aNewFormat.setCharAt(nI + 1, 'y');
#else	// SUPD == 310
                    aNewFormat[nI] = 'y';
                    aNewFormat[nI + 1] = 'y';
#endif	// SUPD == 310
                    aNewFormat.insert(nI + 2, "yy");
                    nLen+=2;
                    nI+=3;
                }
                bForceJapanese = true;
            }
            else if (nChar == '/')
            {
                // MM We have to escape '/' in case it's used as a char
                //todo: this cannot be the right way to replace a part of the string!
#if SUPD == 310
                aNewFormat.setCharAt(nI, '\\');
#else	// SUPD == 310
                aNewFormat[nI] = '\\';
#endif	// SUPD == 310
                aNewFormat.insert(nI + 1, "/");
                nI++;
                nLen++;
            }
        }
        ++nI;
    }

    if (bForceNatNum)
        bForceJapanese = true;

    if (bForceJapanese)
    {
        rLocale.Language = "ja";
        rLocale.Country = "JP";
    }

    if (bForceNatNum)
    {
        aNewFormat.insert( 0, "[NatNum1][$-411]");
    }

    if (bHijri)
    {
        aNewFormat.insert( 0, "[~hijri]");
    }
    return aNewFormat.makeStringAndClear();

}

sal_Int32 convertTwipToMM100(sal_Int32 _t)
{
    // It appears that MSO handles large twip values specially, probably legacy 16bit handling,
    // anything that's bigger than 32767 appears to be simply ignored.
    if( _t >= 0x8000 )
        return 0;
    return ::convertTwipToMm100( _t );
}

sal_uInt32 convertTwipToMM100Unsigned(sal_Int32 _t)
{
    if( _t < 0 )
        return 0;
    return convertTwipToMM100( _t );
}

sal_Int32 convertEMUToMM100(sal_Int32 _t)
{
    return _t / 360;
}

sal_Int16 convertTableJustification( sal_Int32 nIntValue )
{
    sal_Int16 nOrient = text::HoriOrientation::LEFT_AND_WIDTH;
    switch( nIntValue )
    {
        case 1 : nOrient = text::HoriOrientation::CENTER; break;
        case 2 : nOrient = text::HoriOrientation::RIGHT; break;
        case 0 :
        //no break
        default:;

    }
    return nOrient;
}

sal_Int16 ConvertNumberingType(sal_Int32 nFmt)
{
    sal_Int16 nRet;
    switch(nFmt)
    {
        case NS_ooxml::LN_Value_ST_NumberFormat_decimal:
        case 0:
            nRet = style::NumberingType::ARABIC;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_upperRoman:
        case 1:
            nRet = style::NumberingType::ROMAN_UPPER;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_lowerRoman:
        case 2:
            nRet = style::NumberingType::ROMAN_LOWER;
            break;
        case 3:
            nRet = style::NumberingType::CHARS_UPPER_LETTER_N;
            break;
        case 4:
            nRet = style::NumberingType::CHARS_LOWER_LETTER_N;
            break;
        case 5:
            nRet = style::NumberingType::ARABIC;
            break;//ORDINAL
        case NS_ooxml::LN_Value_ST_NumberFormat_bullet:
        case 23:
        case 25:
            nRet = style::NumberingType::CHAR_SPECIAL;
        break;
        case NS_ooxml::LN_Value_ST_NumberFormat_none:
        case 255:
            nRet = style::NumberingType::NUMBER_NONE;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_upperLetter:
            nRet = style::NumberingType::CHARS_UPPER_LETTER;
            break;
        case  NS_ooxml::LN_Value_ST_NumberFormat_lowerLetter:
            nRet = style::NumberingType::CHARS_LOWER_LETTER;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_iroha:
            nRet = style::NumberingType::IROHA_HALFWIDTH_JA;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_irohaFullWidth:
            nRet = style::NumberingType::IROHA_FULLWIDTH_JA;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_aiueo:
            nRet = style::NumberingType::AIU_HALFWIDTH_JA;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_aiueoFullWidth:
            nRet = style::NumberingType::AIU_FULLWIDTH_JA;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_hebrew2:
            nRet = style::NumberingType::CHARS_HEBREW;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_thaiLetters:
            nRet = style::NumberingType::CHARS_THAI;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_russianLower:
            nRet = style::NumberingType::CHARS_CYRILLIC_LOWER_LETTER_RU;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_russianUpper:
            nRet = style::NumberingType::CHARS_CYRILLIC_UPPER_LETTER_RU;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_decimalEnclosedCircleChinese:
        case NS_ooxml::LN_Value_ST_NumberFormat_ideographEnclosedCircle:
            nRet = style::NumberingType::CIRCLE_NUMBER;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_ideographTraditional:
            nRet = style::NumberingType::TIAN_GAN_ZH;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_ideographZodiac:
            nRet = style::NumberingType::DI_ZI_ZH;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_ganada:
            nRet = style::NumberingType::HANGUL_SYLLABLE_KO;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_chosung:
            nRet = style::NumberingType::HANGUL_JAMO_KO;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_koreanDigital:
        case NS_ooxml::LN_Value_ST_NumberFormat_koreanCounting:
        case NS_ooxml::LN_Value_ST_NumberFormat_koreanDigital2:
            nRet = style::NumberingType::NUMBER_HANGUL_KO;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_ideographLegalTraditional:
            nRet = style::NumberingType::NUMBER_UPPER_ZH_TW;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_arabicAlpha:
            nRet = style::NumberingType::CHARS_ARABIC;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_hindiVowels:
            nRet = style::NumberingType::CHARS_NEPALI;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_japaneseLegal:
            nRet = style::NumberingType::NUMBER_TRADITIONAL_JA;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_chineseCounting:
        case NS_ooxml::LN_Value_ST_NumberFormat_japaneseCounting:
        case NS_ooxml::LN_Value_ST_NumberFormat_taiwaneseCounting:
        case NS_ooxml::LN_Value_ST_NumberFormat_ideographDigital:
        case NS_ooxml::LN_Value_ST_NumberFormat_chineseCountingThousand:
            nRet = style::NumberingType::NUMBER_LOWER_ZH;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_chineseLegalSimplified:
            nRet = style::NumberingType::NUMBER_UPPER_ZH;
            break;
        case NS_ooxml::LN_Value_ST_NumberFormat_hebrew1:
            //91726
            nRet = style::NumberingType::CHARS_HEBREW;
            break;
        default: nRet = style::NumberingType::ARABIC;
    }
/*  TODO: Lots of additional values are available - some are supported in the I18 framework
    NS_ooxml::LN_Value_ST_NumberFormat_ordinal = 91682;
    NS_ooxml::LN_Value_ST_NumberFormat_cardinalText = 91683;
    NS_ooxml::LN_Value_ST_NumberFormat_ordinalText = 91684;
    NS_ooxml::LN_Value_ST_NumberFormat_hex = 91685;
    NS_ooxml::LN_Value_ST_NumberFormat_chicago = 91686;
    NS_ooxml::LN_Value_ST_NumberFormat_decimalFullWidth = 91691;
    NS_ooxml::LN_Value_ST_NumberFormat_decimalHalfWidth = 91692;
    NS_ooxml::LN_Value_ST_NumberFormat_japaneseDigitalTenThousand = 91694;
    NS_ooxml::LN_Value_ST_NumberFormat_decimalEnclosedCircle = 91695;
    NS_ooxml::LN_Value_ST_NumberFormat_decimalFullWidth2 = 91696;
    NS_ooxml::LN_Value_ST_NumberFormat_decimalZero = 91699;
    NS_ooxml::LN_Value_ST_NumberFormat_decimalEnclosedFullstop = 91703;
    NS_ooxml::LN_Value_ST_NumberFormat_decimalEnclosedParen = 91704;
    NS_ooxml::LN_Value_ST_NumberFormat_ideographZodiacTraditional = 91709;
    NS_ooxml::LN_Value_ST_NumberFormat_taiwaneseCountingThousand = 91712;
    NS_ooxml::LN_Value_ST_NumberFormat_taiwaneseDigital = 91713;
    NS_ooxml::LN_Value_ST_NumberFormat_chineseLegalSimplified = 91715;
    NS_ooxml::LN_Value_ST_NumberFormat_chineseCountingThousand = 91716;
    NS_ooxml::LN_Value_ST_NumberFormat_koreanLegal = 91719;
    NS_ooxml::LN_Value_ST_NumberFormat_vietnameseCounting = 91721;
    NS_ooxml::LN_Value_ST_NumberFormat_numberInDash = 91725;
    NS_ooxml::LN_Value_ST_NumberFormat_arabicAbjad:
    NS_ooxml::LN_Value_ST_NumberFormat_hindiConsonants = 91731;
    NS_ooxml::LN_Value_ST_NumberFormat_hindiNumbers = 91732;
    NS_ooxml::LN_Value_ST_NumberFormat_hindiCounting = 91733;
    NS_ooxml::LN_Value_ST_NumberFormat_thaiNumbers = 91735;
    NS_ooxml::LN_Value_ST_NumberFormat_thaiCounting = 91736;*/
    return nRet;
}

com::sun::star::util::DateTime ConvertDateStringToDateTime( const OUString& rDateTime )
{
    com::sun::star::util::DateTime aDateTime;
    //xsd::DateTime in the format [-]CCYY-MM-DDThh:mm:ss[Z|(+|-)hh:mm] example: 2008-01-21T10:42:00Z
    //OUString getToken( sal_Int32 token, sal_Unicode cTok, sal_Int32& index ) const SAL_THROW(())
    sal_Int32 nIndex = 0;
    OUString sDate = rDateTime.getToken( 0, 'T', nIndex );
    // HACK: this is broken according to the spec, but MSOffice always treats the time as local,
    // and writes it as Z (=UTC+0)
    OUString sTime = rDateTime.getToken( 0, 'Z', nIndex );
    nIndex = 0;
    aDateTime.Year = sal_uInt16( sDate.getToken( 0, '-', nIndex ).toInt32() );
    aDateTime.Month = sal_uInt16( sDate.getToken( 0, '-', nIndex ).toInt32() );
    if (nIndex != -1)
        aDateTime.Day = sal_uInt16( sDate.copy( nIndex ).toInt32() );

    nIndex = 0;
    aDateTime.Hours = sal_uInt16( sTime.getToken( 0, ':', nIndex ).toInt32() );
    aDateTime.Minutes = sal_uInt16( sTime.getToken( 0, ':', nIndex ).toInt32() );
    if (nIndex != -1)
        aDateTime.Seconds = sal_uInt16( sTime.copy( nIndex ).toInt32() );

    return aDateTime;
}


} // namespace ConversionHelper
} //namespace dmapper
} //namespace writerfilter

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
