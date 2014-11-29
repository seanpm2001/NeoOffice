/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 * 
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Portions of this file are part of the LibreOffice project.
 *
 *   This Source Code Form is subject to the terms of the Mozilla Public
 *   License, v. 2.0. If a copy of the MPL was not distributed with this
 *   file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 ************************************************************************/

#include "docxexport.hxx"
#include "docxexportfilter.hxx"

#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/i18n/ScriptType.hdl>

#include <oox/core/tokens.hxx>
#include <oox/export/drawingml.hxx>
#include <oox/export/vmlexport.hxx>

#include <map>
#include <algorithm>

#include <bookmrk.hxx>
#include <docsh.hxx>
#include <IDocumentBookmarkAccess.hxx>
#include <ndtxt.hxx>
#include <wrtww8.hxx>
#include <fltini.hxx>
#include <fmtline.hxx>
#include <fmtpdsc.hxx>
#include <frmfmt.hxx>
#include <section.hxx>

#include <docary.hxx>
#include <numrule.hxx>
#include <charfmt.hxx>

#include "ww8par.hxx"
#include "ww8scan.hxx"

#include <comphelper/string.hxx>
#include <rtl/ustrbuf.hxx>
#include <vcl/font.hxx>

using namespace ::comphelper;
using namespace ::com::sun::star;

using oox::vml::VMLExport;

using rtl::OUString;
using rtl::OUStringBuffer;

#define S( x ) OUString( RTL_CONSTASCII_USTRINGPARAM( x ) )

AttributeOutputBase& DocxExport::AttrOutput() const
{
    return *m_pAttrOutput;
}

MSWordSections& DocxExport::Sections() const
{
    return *m_pSections;
}

bool DocxExport::CollapseScriptsforWordOk( USHORT nScript, USHORT nWhich )
{
    // TODO FIXME is this actually true for docx? - this is ~copied from WW8
    if ( nScript == i18n::ScriptType::ASIAN )
    {
        // for asian in ww8, there is only one fontsize
        // and one fontstyle (posture/weight)
        switch ( nWhich )
        {
            case RES_CHRATR_FONTSIZE:
            case RES_CHRATR_POSTURE:
            case RES_CHRATR_WEIGHT:
                return false;
            default:
                break;
        }
    }
    else if ( nScript != i18n::ScriptType::COMPLEX )
    {
        // for western in ww8, there is only one fontsize
        // and one fontstyle (posture/weight)
        switch ( nWhich )
        {
            case RES_CHRATR_CJK_FONTSIZE:
            case RES_CHRATR_CJK_POSTURE:
            case RES_CHRATR_CJK_WEIGHT:
                return false;
            default:
                break;
        }
    }
    return true;
}

USHORT DocxExport::GetBookmarks( const SwTxtNode& rNd, xub_StrLen nStt,
                    xub_StrLen nEnd, SvPtrarr& rArr )
{
    const SwBookmarks& rBkmks = pDoc->getBookmarks();
    ULONG nNd = rNd.GetIndex( );

    for ( USHORT i = 0; i < rBkmks.Count( ); i++ )
    {
        SwBookmark* pBkmk = rBkmks[i];

        // Only keep the bookmarks starting or ending in this node
        if ( pBkmk->BookmarkStart()->nNode == nNd ||
                pBkmk->BookmarkEnd()->nNode == nNd )
        {
            xub_StrLen nBStart = pBkmk->BookmarkStart()->nContent.GetIndex();
            xub_StrLen nBEnd = pBkmk->BookmarkEnd()->nContent.GetIndex();

            // Keep only the bookmars starting or ending in the snippet
            bool bIsStartOk = ( nBStart >= nStt ) && ( nBStart <= nEnd );
            bool bIsEndOk = ( nBEnd >= nStt ) && ( nBEnd <= nEnd );

            if ( bIsStartOk || bIsEndOk )
            {
                rArr.Insert( pBkmk, rArr.Count( ) );
            }
        }
    }
    return rArr.Count( );
}

class CompareBkmksEnd:public
std::binary_function < const SwBookmark *, const SwBookmark *,
    bool >
{
  public:
    inline bool operator (  ) ( const SwBookmark * pOneB, const SwBookmark * pTwoB ) const
    {
        xub_StrLen nOEnd = pOneB->BookmarkEnd (  )->nContent.GetIndex (  );
        xub_StrLen nTEnd = pTwoB->BookmarkEnd (  )->nContent.GetIndex (  );

        return nOEnd < nTEnd;
    }
};

bool DocxExport::NearestBookmark( xub_StrLen& rNearest )
{
    bool bHasBookmark = false;

    if ( m_rSortedBkmksStart.size( ) > 0 )
    {
        SwBookmark* pBkmkStart = m_rSortedBkmksStart[0];
        rNearest = pBkmkStart->BookmarkStart()->nContent.GetIndex();
        bHasBookmark = true;
    }        

    if ( m_rSortedBkmksEnd.size( ) > 0 )
    {
        SwBookmark* pBkmkEnd = m_rSortedBkmksEnd[0];
        if ( !bHasBookmark )
            rNearest = pBkmkEnd->BookmarkEnd()->nContent.GetIndex();
        else
            rNearest = std::min( rNearest, pBkmkEnd->BookmarkEnd()->nContent.GetIndex() );
        bHasBookmark = true;
    }

    return bHasBookmark;
}

xub_StrLen DocxExport::GetNextPos( SwAttrIter* pAttrIter, const SwTxtNode& rNode, xub_StrLen nAktPos )
{
    // Get the bookmarks for the normal run
    xub_StrLen nNextPos = MSWordExportBase::GetNextPos( pAttrIter, rNode, nAktPos );

    GetSortedBookmarks( rNode, nAktPos, nNextPos - nAktPos );

    xub_StrLen nNextBookmark = nNextPos;
    NearestBookmark( nNextPos );
    
    return std::min( nNextPos, nNextBookmark );
}

void DocxExport::UpdatePosition( SwAttrIter* pAttrIter, xub_StrLen nAktPos, xub_StrLen nEnd )
{
    xub_StrLen nNextPos;

    // either no bookmark, or it is not at the current position
    if ( !NearestBookmark( nNextPos ) || nNextPos > nAktPos )
    {
        MSWordExportBase::UpdatePosition( pAttrIter, nAktPos, nEnd );
    }
}

void DocxExport::GetSortedBookmarks( const SwTxtNode& rNode, xub_StrLen nAktPos, xub_StrLen nLen )
{
    SvPtrarr aBkmksStart( 8 , 8 );
    if ( GetBookmarks( rNode, nAktPos, nAktPos + nLen, aBkmksStart ) ) 
    {
        std::vector<SwBookmark*> aSortedEnd;
        std::vector<SwBookmark*> aSortedStart;
        for ( USHORT i = 0; i < aBkmksStart.Count(); i++ )
        {
            SwBookmark* pBkmk = ( SwBookmark* )aBkmksStart[i];

            // Remove the positions egals to the current pos
            xub_StrLen nStart = pBkmk->BookmarkStart()->nContent.GetIndex();
            xub_StrLen nEnd = pBkmk->BookmarkEnd()->nContent.GetIndex();

            if ( nStart > nAktPos )
            {
                aSortedStart.push_back( pBkmk );
            }

            if ( nEnd > nAktPos )
            {
                aSortedEnd.push_back( pBkmk );
            }
        }

        // Sort the bookmarks by end position
        std::sort( aSortedEnd.begin(), aSortedEnd.end(), CompareBkmksEnd() );
    
        m_rSortedBkmksStart.swap( aSortedStart );
        m_rSortedBkmksEnd.swap( aSortedEnd );
    }
    else
    {
        m_rSortedBkmksStart.clear( );
        m_rSortedBkmksEnd.clear( );
    }
}

void DocxExport::AppendBookmarks( const SwTxtNode& rNode, xub_StrLen nAktPos, xub_StrLen nLen )
{
    std::vector<const String*> rStarts;
    std::vector<const String*> rEnds;

    SvPtrarr aBkmks( 8 , 8 );
    if ( GetBookmarks( rNode, nAktPos, nAktPos + nLen, aBkmks ) )
    {
        for ( USHORT i = 0; i < aBkmks.Count( ); i++ )
        {
            SwBookmark* pBkmk = ( SwBookmark* )aBkmks[i];
            
            xub_StrLen nStart = pBkmk->BookmarkStart()->nContent.GetIndex();
            xub_StrLen nEnd = pBkmk->BookmarkEnd()->nContent.GetIndex();

            if ( nStart == nAktPos ) 
            {
                rStarts.push_back( &( pBkmk->GetName( ) ) );
            }

            if ( nEnd == nAktPos )
            {
                rEnds.push_back( &( pBkmk->GetName( ) ) );
            }
        }
    }

    m_pAttrOutput->WriteBookmarks_Impl( rStarts, rEnds );
}

void DocxExport::AppendBookmark( const String& rName, bool /*bSkip*/ )
{
    std::vector<const String*> rStarts;
    std::vector<const String*> rEnds;

    rStarts.push_back( &rName );
    rEnds.push_back( &rName );

    m_pAttrOutput->WriteBookmarks_Impl( rStarts, rEnds );
}

::rtl::OString DocxExport::AddRelation( const OUString& rType, const OUString& rTarget, const OUString& rMode )
{
    OUString sId = m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
           rType, rTarget, rMode );

    return ::rtl::OUStringToOString( sId, RTL_TEXTENCODING_UTF8 );
}

bool DocxExport::DisallowInheritingOutlineNumbering( const SwFmt& rFmt )
{
    bool bRet( false );

    if (SFX_ITEM_SET != rFmt.GetItemState(RES_PARATR_NUMRULE, false))
    {
        if (const SwFmt *pParent = rFmt.DerivedFrom())
        {
			if (((const SwTxtFmtColl*)pParent)->IsAssignedToListLevelOfOutlineStyle())
            {
                ::sax_fastparser::FSHelperPtr pSerializer = m_pAttrOutput->GetSerializer( );
                // Level 9 disables the outline
                pSerializer->singleElementNS( XML_w, XML_outlineLvl,
                        FSNS( XML_w, XML_val ), "9" ,
                        FSEND );

                bRet = true;
            }
        }
    }

    return bRet;
}

void DocxExport::WriteHeadersFooters( BYTE nHeadFootFlags,
        const SwFrmFmt& rFmt, const SwFrmFmt& rLeftFmt, const SwFrmFmt& rFirstPageFmt )
{
    // headers
    if ( nHeadFootFlags & nsHdFtFlags::WW8_HEADER_EVEN )
        WriteHeaderFooter( rLeftFmt, true, "even" );

    if ( nHeadFootFlags & nsHdFtFlags::WW8_HEADER_ODD )
        WriteHeaderFooter( rFmt, true, "default" );

    if ( nHeadFootFlags & nsHdFtFlags::WW8_HEADER_FIRST )
        WriteHeaderFooter( rFirstPageFmt, true, "first" );

    // footers
    if ( nHeadFootFlags & nsHdFtFlags::WW8_FOOTER_EVEN )
        WriteHeaderFooter( rLeftFmt, false, "even" );

    if ( nHeadFootFlags & nsHdFtFlags::WW8_FOOTER_ODD )
        WriteHeaderFooter( rFmt, false, "default" );

    if ( nHeadFootFlags & nsHdFtFlags::WW8_FOOTER_FIRST )
        WriteHeaderFooter( rFirstPageFmt, false, "first" );
}

void DocxExport::OutputField( const SwField* pFld, ww::eField eFldType, const String& rFldCmd, BYTE nMode )
{
    m_pAttrOutput->WriteField_Impl( pFld, eFldType, rFldCmd, nMode );
}

void DocxExport::WriteFormData( SwFieldBookmark& /*rFieldmark*/ )
{
#if OSL_DEBUG_LEVEL > 0
    fprintf( stderr, "TODO DocxExport::WriteFormData()\n" );
#endif
}

void DocxExport::DoComboBox(const rtl::OUString& rName,
                             const rtl::OUString& rHelp,
                             const rtl::OUString& rToolTip,
                             const rtl::OUString& rSelected,
                             uno::Sequence<rtl::OUString>& rListItems)
{
    m_pDocumentFS->startElementNS( XML_w, XML_ffData, FSEND );

    m_pDocumentFS->singleElementNS( XML_w, XML_name, 
            FSNS( XML_w, XML_val ), OUStringToOString( rName, RTL_TEXTENCODING_UTF8 ).getStr(),
            FSEND );

    m_pDocumentFS->singleElementNS( XML_w, XML_enabled, FSEND );

    if ( rHelp.getLength( ) > 0 )
        m_pDocumentFS->singleElementNS( XML_w, XML_helpText, 
            FSNS( XML_w, XML_val ), OUStringToOString( rHelp, RTL_TEXTENCODING_UTF8 ).getStr(),
            FSEND );
    
    if ( rToolTip.getLength( ) > 0 )
        m_pDocumentFS->singleElementNS( XML_w, XML_statusText, 
            FSNS( XML_w, XML_val ), OUStringToOString( rToolTip, RTL_TEXTENCODING_UTF8 ).getStr(),
            FSEND );

    m_pDocumentFS->startElementNS( XML_w, XML_ddList, FSEND );
  
    // Output the 0-based index of the selected value
    sal_uInt32 nListItems = rListItems.getLength();
    sal_Int32 nId = 0;
    sal_uInt32 nI = 0;
    while ( ( nI < nListItems ) && ( nId == 0 ) )
    {
        if ( rListItems[nI] == rSelected ) 
            nId = nI;
        nI++;
    }

    m_pDocumentFS->singleElementNS( XML_w, XML_result, 
            FSNS( XML_w, XML_val ), rtl::OString::valueOf( nId ).getStr( ),
            FSEND );

    // Loop over the entries
    
    for (sal_uInt32 i = 0; i < nListItems; i++)
    {
        m_pDocumentFS->singleElementNS( XML_w, XML_listEntry,
                FSNS( XML_w, XML_val ), OUStringToOString( rListItems[i], RTL_TEXTENCODING_UTF8 ).getStr(),
               FSEND );
    }

    m_pDocumentFS->endElementNS( XML_w, XML_ddList );

    m_pDocumentFS->endElementNS( XML_w, XML_ffData );
}

void DocxExport::DoFormText(const SwInputField* /*pFld*/)
{
#if OSL_DEBUG_LEVEL > 0
    fprintf( stderr, "TODO DocxExport::ForFormText()\n" );
#endif
}

void DocxExport::ExportDocument_Impl()
{
    InitStyles();

    // init sections
    m_pSections = new MSWordSections( *this );

    WriteMainText();

    WriteFootnotesEndnotes();
    
    WriteNumbering();

    WriteFonts();

    delete pStyles, pStyles = NULL;
    delete m_pSections, m_pSections = NULL;
}

void DocxExport::OutputPageSectionBreaks( const SwTxtNode& )
{
#if OSL_DEBUG_LEVEL > 0
    fprintf( stderr, "TODO DocxExport::OutputPageSectionBreaks( const SwTxtNode& )\n" );
#endif
}


void DocxExport::AppendSection( const SwPageDesc *pPageDesc, const SwSectionFmt* pFmt, ULONG nLnNum )
{
    AttrOutput().SectionBreak( msword::PageBreak, m_pSections->CurrentSectionInfo() );
    m_pSections->AppendSep( pPageDesc, pFmt, nLnNum );
}

void DocxExport::OutputEndNode( const SwEndNode& rEndNode )
{
    MSWordExportBase::OutputEndNode( rEndNode );

    if ( TXT_MAINTEXT == nTxtTyp && rEndNode.StartOfSectionNode()->IsSectionNode() )
    {
        // this originally comes from WW8Export::WriteText(), and looks like it
        // could have some code common with SectionNode()...

        const SwSection& rSect = rEndNode.StartOfSectionNode()->GetSectionNode()->GetSection();
        if ( bStartTOX && TOX_CONTENT_SECTION == rSect.GetType() )
            bStartTOX = false;

        SwNodeIndex aIdx( rEndNode, 1 );
        const SwNode& rNd = aIdx.GetNode();
        if ( rNd.IsEndNode() && rNd.StartOfSectionNode()->IsSectionNode() )
            return;

        if ( !rNd.IsSectionNode() && !bIsInTable ) // No sections in table
        {
            const SwSectionFmt* pParentFmt = rSect.GetFmt()->GetParent();
            if( !pParentFmt )
                pParentFmt = (SwSectionFmt*)0xFFFFFFFF;

            ULONG nRstLnNum;
            if( rNd.IsCntntNode() )
                nRstLnNum = const_cast< SwCntntNode* >( rNd.GetCntntNode() )->GetSwAttrSet().GetLineNumber().GetStartValue();
            else
                nRstLnNum = 0;

            AttrOutput().SectionBreak( msword::PageBreak, m_pSections->CurrentSectionInfo( ) );
            m_pSections->AppendSep( pAktPageDesc, pParentFmt, nRstLnNum ); 
        }
    }
}

void DocxExport::OutputTableNode( const SwTableNode& )
{
#if OSL_DEBUG_LEVEL > 0
    fprintf( stderr, "TODO DocxExport::OutputTableNode( const SwTableNode& )\n" );
#endif
}

void DocxExport::OutputGrfNode( const SwGrfNode& )
{
#if OSL_DEBUG_LEVEL > 0
    fprintf( stderr, "TODO DocxExport::OutputGrfNode( const SwGrfNode& )\n" );
#endif
}

void DocxExport::OutputOLENode( const SwOLENode& )
{
#if OSL_DEBUG_LEVEL > 0
    fprintf( stderr, "TODO DocxExport::OutputOLENode( const SwOLENode& )\n" );
#endif
}

ULONG DocxExport::ReplaceCr( BYTE )
{
    // Completely unused for Docx export... only here for code sharing 
    // purpose with binary export
    return 0;
}

void DocxExport::PrepareNewPageDesc( const SfxItemSet* pSet,
        const SwNode& rNd, const SwFmtPageDesc* pNewPgDescFmt,
        const SwPageDesc* pNewPgDesc )
{
    // tell the attribute output that we are ready to write the section
    // break [has to be output inside paragraph properties]
    AttrOutput().SectionBreak( msword::PageBreak, m_pSections->CurrentSectionInfo() );

    const SwSectionFmt* pFmt = GetSectionFormat( rNd );
    const ULONG nLnNm = GetSectionLineNo( pSet, rNd );

    ASSERT( pNewPgDescFmt || pNewPgDesc, "Neither page desc format nor page desc provided." );

    if ( pNewPgDescFmt )
    {
        m_pSections->AppendSep( *pNewPgDescFmt, rNd, pFmt, nLnNm );
    }
    else if ( pNewPgDesc )
    {
        m_pSections->AppendSep( pNewPgDesc, rNd, pFmt, nLnNm );
    }

}

void DocxExport::InitStyles()
{
    pStyles = new MSWordStyles( *this );

    // setup word/styles.xml and the relations + content type
    m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
            S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" ),
            S( "styles.xml" ) );

    ::sax_fastparser::FSHelperPtr pStylesFS =
#if SUPD == 310
        m_pFilter->openFragmentStreamWithSerializer( S( "word/styles.xml" ),
#else	// SUPD == 310
        m_pFilter->openOutputStreamWithSerializer( S( "word/styles.xml" ),
#endif	// SUPD == 310
            S( "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml" ) );

    // switch the serializer to redirect the output to word/styles.xml
    m_pAttrOutput->SetSerializer( pStylesFS );

    // do the work
    pStyles->OutputStylesTable();

    // switch the serializer back
    m_pAttrOutput->SetSerializer( m_pDocumentFS );
}

void DocxExport::WriteFootnotesEndnotes()
{
    if ( m_pAttrOutput->HasFootnotes() )
    {
        // setup word/styles.xml and the relations + content type
        m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
                S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes" ),
                S( "footnotes.xml" ) );

        ::sax_fastparser::FSHelperPtr pFootnotesFS =
#if SUPD == 310
            m_pFilter->openFragmentStreamWithSerializer( S( "word/footnotes.xml" ),
#else	// SUPD == 310
            m_pFilter->openOutputStreamWithSerializer( S( "word/footnotes.xml" ),
#endif	// SUPD == 310
                    S( "application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml" ) );

        // switch the serializer to redirect the output to word/footnotes.xml
        m_pAttrOutput->SetSerializer( pFootnotesFS );

        // do the work
        m_pAttrOutput->FootnotesEndnotes( true );

        // switch the serializer back
        m_pAttrOutput->SetSerializer( m_pDocumentFS );
    }

    if ( m_pAttrOutput->HasEndnotes() )
    {
        // setup word/styles.xml and the relations + content type
        m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
                S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes" ),
                S( "endnotes.xml" ) );

        ::sax_fastparser::FSHelperPtr pEndnotesFS =
#if SUPD == 310
            m_pFilter->openFragmentStreamWithSerializer( S( "word/endnotes.xml" ),
#else	// SUPD == 310
            m_pFilter->openOutputStreamWithSerializer( S( "word/endnotes.xml" ),
#endif	// SUPD == 310
                    S( "application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml" ) );

        // switch the serializer to redirect the output to word/endnotes.xml
        m_pAttrOutput->SetSerializer( pEndnotesFS );

        // do the work
        m_pAttrOutput->FootnotesEndnotes( false );

        // switch the serializer back
        m_pAttrOutput->SetSerializer( m_pDocumentFS );
    }
}

void DocxExport::WriteNumbering()
{
    if ( !pUsedNumTbl )
        return; // no numbering is used

    m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
        S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" ),
        S( "numbering.xml" ) );

#if SUPD == 310
    ::sax_fastparser::FSHelperPtr pNumberingFS = m_pFilter->openFragmentStreamWithSerializer( S( "word/numbering.xml" ),
#else	// SUPD == 310
    ::sax_fastparser::FSHelperPtr pNumberingFS = m_pFilter->openOutputStreamWithSerializer( S( "word/numbering.xml" ),
#endif	// SUPD == 310
        S( "application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml" ) );

    // switch the serializer to redirect the output to word/nubering.xml
    m_pAttrOutput->SetSerializer( pNumberingFS );

    pNumberingFS->startElementNS( XML_w, XML_numbering,
            FSNS( XML_xmlns, XML_w ), "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
            FSEND );

    AbstractNumberingDefinitions();

    NumberingDefinitions();

    pNumberingFS->endElementNS( XML_w, XML_numbering );

    // switch the serializer back
    m_pAttrOutput->SetSerializer( m_pDocumentFS );
}

void DocxExport::WriteHeaderFooter( const SwFmt& rFmt, bool bHeader, const char* pType )
{
    // setup the xml stream
    OUString aRelId;
    ::sax_fastparser::FSHelperPtr pFS;
    if ( bHeader )
    {
        OUString aName( OUStringBuffer().appendAscii( "header" ).append( ++m_nHeaders ).appendAscii( ".xml" ).makeStringAndClear() );

        aRelId = m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
                S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" ),
                aName );
        
#if SUPD == 310
        pFS = m_pFilter->openFragmentStreamWithSerializer( OUStringBuffer().appendAscii( "word/" ).append( aName ).makeStringAndClear(),
#else	// SUPD == 310
        pFS = m_pFilter->openOutputStreamWithSerializer( OUStringBuffer().appendAscii( "word/" ).append( aName ).makeStringAndClear(),
#endif	// SUPD == 310
                    S( "application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml" ) );

        pFS->startElementNS( XML_w, XML_hdr,
                FSNS( XML_xmlns, XML_w ), "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
                FSEND );
    }
    else
    {
        OUString aName( OUStringBuffer().appendAscii( "footer" ).append( ++m_nFooters ).appendAscii( ".xml" ).makeStringAndClear() );

        aRelId = m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
                S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer" ),
                aName );
        
#if SUPD == 310
        pFS = m_pFilter->openFragmentStreamWithSerializer( OUStringBuffer().appendAscii( "word/" ).append( aName ).makeStringAndClear(),
#else	// SUPD == 310
        pFS = m_pFilter->openOutputStreamWithSerializer( OUStringBuffer().appendAscii( "word/" ).append( aName ).makeStringAndClear(),
#endif	// SUPD == 310
                    S( "application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml" ) );

        pFS->startElementNS( XML_w, XML_ftr,
                FSNS( XML_xmlns, XML_w ), "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
                FSEND );
    }

    // switch the serializer to redirect the output to word/styles.xml
    m_pAttrOutput->SetSerializer( pFS );

    // do the work
    WriteHeaderFooterText( rFmt, bHeader );

    // switch the serializer back
    m_pAttrOutput->SetSerializer( m_pDocumentFS );

    // close the tag
    sal_Int32 nReference;
    if ( bHeader )
    {
        pFS->endElementNS( XML_w, XML_hdr );
        nReference = XML_headerReference;
    }
    else
    {
        pFS->endElementNS( XML_w, XML_ftr );
        nReference = XML_footerReference;
    }

    // and write the reference
    m_pDocumentFS->singleElementNS( XML_w, nReference,
            FSNS( XML_w, XML_type ), pType,
            FSNS( XML_r, XML_id ), rtl::OUStringToOString( aRelId, RTL_TEXTENCODING_UTF8 ).getStr(),
            FSEND );
}

void DocxExport::WriteFonts()
{
    m_pFilter->addRelation( m_pDocumentFS->getOutputStream(),
            S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/fontTable" ),
            S( "fontTable.xml" ) );

#if SUPD == 310
    ::sax_fastparser::FSHelperPtr pFS = m_pFilter->openFragmentStreamWithSerializer(
#else	// SUPD == 310
    ::sax_fastparser::FSHelperPtr pFS = m_pFilter->openOutputStreamWithSerializer(
#endif	// SUPD == 310
            S( "word/fontTable.xml" ),
            S( "application/vnd.openxmlformats-officedocument.wordprocessingml.fontTable+xml" ) );

    pFS->startElementNS( XML_w, XML_fonts,
            FSNS( XML_xmlns, XML_w ), "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
            FSEND );

    // switch the serializer to redirect the output to word/styles.xml
    m_pAttrOutput->SetSerializer( pFS );

    // do the work
    maFontHelper.WriteFontTable( *m_pAttrOutput );

    // switch the serializer back
    m_pAttrOutput->SetSerializer( m_pDocumentFS );

    pFS->endElementNS( XML_w, XML_fonts );
}


void DocxExport::WriteProperties( ) 
{
    // Write the core properties
    SwDocShell* pDocShell( pDoc->GetDocShell( ) );
    uno::Reference<document::XDocumentProperties> xDocProps;
    if ( pDocShell )
    {
        uno::Reference<document::XDocumentPropertiesSupplier> xDPS( 
               pDocShell->GetModel( ), uno::UNO_QUERY );
        xDocProps = xDPS->getDocumentProperties();
    }

    m_pFilter->exportDocumentProperties( xDocProps );
}

VMLExport& DocxExport::VMLExporter()
{
    return *m_pVMLExport;
}

void DocxExport::WriteMainText()
{
    // setup the namespaces
    m_pDocumentFS->startElementNS( XML_w, XML_document,
            FSNS( XML_xmlns, XML_o ), "urn:schemas-microsoft-com:office:office",
            FSNS( XML_xmlns, XML_r ), "http://schemas.openxmlformats.org/officeDocument/2006/relationships",
            FSNS( XML_xmlns, XML_v ), "urn:schemas-microsoft-com:vml",
            FSNS( XML_xmlns, XML_w ), "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
            FSNS( XML_xmlns, XML_w10 ), "urn:schemas-microsoft-com:office:word",
            FSNS( XML_xmlns, XML_wp ), "http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing",
            FSEND );

    // body
    m_pDocumentFS->startElementNS( XML_w, XML_body, FSEND );
    
    pCurPam->GetPoint()->nNode = pDoc->GetNodes().GetEndOfContent().StartOfSectionNode()->GetIndex();

    // the text
    WriteText();

    // the last section info
    const WW8_SepInfo *pSectionInfo = m_pSections? m_pSections->CurrentSectionInfo(): NULL;
    if ( pSectionInfo )
        SectionProperties( *pSectionInfo );

    // finish body and document
    m_pDocumentFS->endElementNS( XML_w, XML_body );
    m_pDocumentFS->endElementNS( XML_w, XML_document );
}

DocxExport::DocxExport( DocxExportFilter *pFilter, SwDoc *pDocument, SwPaM *pCurrentPam, SwPaM *pOriginalPam )
    : MSWordExportBase( pDocument, pCurrentPam, pOriginalPam ),
      m_pFilter( pFilter ),
      m_pAttrOutput( NULL ),
      m_pSections( NULL ),
      m_nHeaders( 0 ),
      m_nFooters( 0 ),
      m_pVMLExport( NULL )
{
    // Write the document properies
    WriteProperties( );

    // relations for the document
    m_pFilter->addRelation( S( "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" ),
            S( "word/document.xml" ) );

    // the actual document
#if SUPD == 310
    m_pDocumentFS = m_pFilter->openFragmentStreamWithSerializer( S( "word/document.xml" ),
#else	// SUPD == 310
    m_pDocumentFS = m_pFilter->openOutputStreamWithSerializer( S( "word/document.xml" ),
#endif	// SUPD == 310
            S( "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml" ) );

    // the DrawingML access
    m_pDrawingML = new oox::drawingml::DrawingML( m_pDocumentFS, m_pFilter, oox::drawingml::DrawingML::DOCUMENT_DOCX );

    // the attribute output for the document
    m_pAttrOutput = new DocxAttributeOutput( *this, m_pDocumentFS, m_pDrawingML );

    // the related VMLExport
    m_pVMLExport = new VMLExport( m_pDocumentFS );
}

DocxExport::~DocxExport()
{
    delete m_pVMLExport, m_pVMLExport = NULL;
    delete m_pAttrOutput, m_pAttrOutput = NULL;
    delete m_pDrawingML, m_pDrawingML = NULL;
}

/* vi:set tabstop=4 shiftwidth=4 expandtab: */
