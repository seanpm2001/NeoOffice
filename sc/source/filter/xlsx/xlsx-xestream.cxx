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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <utility>

#include <rtl/ustring.hxx>
#include <sax/fshelper.hxx>
#include <unotools/streamwrap.hxx>

#include "precompiled_sc.hxx"
#include "docuno.hxx"
#include "xestream.hxx"
#include "xladdress.hxx"
#include "xlstring.hxx"
#include "xeroot.hxx"
#include "xestyle.hxx"
#include "xcl97rec.hxx"
#include "rangelst.hxx"
#include "compiler.hxx"

#include <../../ui/inc/docsh.hxx>
#include <svtools/fltrcfg.hxx>
#include <svx/svxmsbas.hxx>
#include <excdoc.hxx>

#include <oox/core/tokens.hxx>
#include <formula/grammar.hxx>
#include <oox/export/drawingml.hxx>

#if SUPD == 310
#include <comphelper/processfactory.hxx>
#include <excelvbaproject.hxx>
#endif	// SUPD == 310

#define DEBUG_XL_ENCRYPTION 0

using ::std::vector;

using ::com::sun::star::beans::PropertyValue;
using ::com::sun::star::embed::XStorage;
using ::com::sun::star::io::XOutputStream;
using ::com::sun::star::io::XStream;
using ::com::sun::star::lang::XComponent;
using ::com::sun::star::lang::XMultiServiceFactory;
using ::com::sun::star::lang::XServiceInfo;
using ::com::sun::star::lang::XSingleServiceFactory;
using ::com::sun::star::registry::InvalidRegistryException;
using ::com::sun::star::registry::XRegistryKey;
using ::com::sun::star::uno::Exception;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::XInterface;
using ::oox::drawingml::DrawingML;
using ::rtl::OString;
using ::rtl::OUString;
using ::utl::OStreamWrapper;
#if SUPD == 310
using ::com::sun::star::sheet::XSpreadsheetDocument;
#endif	// SUPD == 310
using namespace formula;
// ============================================================================

XclExpStream::XclExpStream( SvStream& rOutStrm, const XclExpRoot& rRoot, sal_uInt16 nMaxRecSize ) :
    mrStrm( rOutStrm ),
    mrRoot( rRoot ),
    mnMaxRecSize( nMaxRecSize ),
    mnCurrMaxSize( 0 ),
    mnMaxSliceSize( 0 ),
    mnHeaderSize( 0 ),
    mnCurrSize( 0 ),
    mnSliceSize( 0 ),
    mnPredictSize( 0 ),
    mnLastSizePos( 0 ),
    mbInRec( false )
{
    if( mnMaxRecSize == 0 )
        mnMaxRecSize = (mrRoot.GetBiff() <= EXC_BIFF5) ? EXC_MAXRECSIZE_BIFF5 : EXC_MAXRECSIZE_BIFF8;
    mnMaxContSize = mnMaxRecSize;
}

XclExpStream::~XclExpStream()
{
    mrStrm.Flush();
}

void XclExpStream::StartRecord( sal_uInt16 nRecId, sal_Size nRecSize )
{
    DBG_ASSERT( !mbInRec, "XclExpStream::StartRecord - another record still open" );
    DisableEncryption();
    mnMaxContSize = mnCurrMaxSize = mnMaxRecSize;
    mnPredictSize = nRecSize;
    mbInRec = true;
    InitRecord( nRecId );
    SetSliceSize( 0 );
    EnableEncryption();
}

void XclExpStream::EndRecord()
{
    DBG_ASSERT( mbInRec, "XclExpStream::EndRecord - no record open" );
    DisableEncryption();
    UpdateRecSize();
    mrStrm.Seek( STREAM_SEEK_TO_END );
    mbInRec = false;
}

void XclExpStream::SetSliceSize( sal_uInt16 nSize )
{
    mnMaxSliceSize = nSize;
    mnSliceSize = 0;
}

XclExpStream& XclExpStream::operator<<( sal_Int8 nValue )
{
    PrepareWrite( 1 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, nValue);
    else
        mrStrm << nValue;
    return *this;
}

XclExpStream& XclExpStream::operator<<( sal_uInt8 nValue )
{
    PrepareWrite( 1 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, nValue);
    else
        mrStrm << nValue;
    return *this;
}

XclExpStream& XclExpStream::operator<<( sal_Int16 nValue )
{
    PrepareWrite( 2 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, nValue);
    else
        mrStrm << nValue;
    return *this;
}

XclExpStream& XclExpStream::operator<<( sal_uInt16 nValue )
{
    PrepareWrite( 2 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, nValue);
    else
        mrStrm << nValue;
    return *this;
}

XclExpStream& XclExpStream::operator<<( sal_Int32 nValue )
{
    PrepareWrite( 4 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, nValue);
    else
        mrStrm << nValue;
    return *this;
}

XclExpStream& XclExpStream::operator<<( sal_uInt32 nValue )
{
    PrepareWrite( 4 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, nValue);
    else
        mrStrm << nValue;
    return *this;
}

XclExpStream& XclExpStream::operator<<( float fValue )
{
    PrepareWrite( 4 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, fValue);
    else
        mrStrm << fValue;
    return *this;
}

XclExpStream& XclExpStream::operator<<( double fValue )
{
    PrepareWrite( 8 );
    if (mbUseEncrypter && HasValidEncrypter())
        mxEncrypter->Encrypt(mrStrm, fValue);
    else
        mrStrm << fValue;
    return *this;
}

sal_Size XclExpStream::Write( const void* pData, sal_Size nBytes )
{
    sal_Size nRet = 0;
    if( pData && (nBytes > 0) )
    {
        if( mbInRec )
        {
            const sal_uInt8* pBuffer = reinterpret_cast< const sal_uInt8* >( pData );
            sal_Size nBytesLeft = nBytes;
            bool bValid = true;

            while( bValid && (nBytesLeft > 0) )
            {
                sal_Size nWriteLen = ::std::min< sal_Size >( PrepareWrite(), nBytesLeft );
                sal_Size nWriteRet = nWriteLen;
                if (mbUseEncrypter && HasValidEncrypter())
                {
                    DBG_ASSERT(nWriteLen > 0, "XclExpStream::Write: write length is 0!");
                    vector<sal_uInt8> aBytes(nWriteLen);
                    memcpy(&aBytes[0], pBuffer, nWriteLen);
                    mxEncrypter->EncryptBytes(mrStrm, aBytes);
                    // TODO: How do I check if all the bytes have been successfully written ?
                }
                else
                {
                    nWriteRet = mrStrm.Write( pBuffer, nWriteLen );
                bValid = (nWriteLen == nWriteRet);
                DBG_ASSERT( bValid, "XclExpStream::Write - stream write error" );
                }
                pBuffer += nWriteRet;
                nRet += nWriteRet;
                nBytesLeft -= nWriteRet;
                UpdateSizeVars( nWriteRet );
            }
        }
        else
            nRet = mrStrm.Write( pData, nBytes );
    }
    return nRet;
}

void XclExpStream::WriteZeroBytes( sal_Size nBytes )
{
    if( mbInRec )
    {
        sal_Size nBytesLeft = nBytes;
        while( nBytesLeft > 0 )
        {
            sal_Size nWriteLen = ::std::min< sal_Size >( PrepareWrite(), nBytesLeft );
            WriteRawZeroBytes( nWriteLen );
            nBytesLeft -= nWriteLen;
            UpdateSizeVars( nWriteLen );
        }
    }
    else
        WriteRawZeroBytes( nBytes );
}

sal_Size XclExpStream::CopyFromStream( SvStream& rInStrm, sal_Size nBytes )
{
    sal_Size nStrmPos = rInStrm.Tell();
    rInStrm.Seek( STREAM_SEEK_TO_END );
    sal_Size nStrmSize = rInStrm.Tell();
    rInStrm.Seek( nStrmPos );

    sal_Size nBytesLeft = ::std::min( nBytes, nStrmSize - nStrmPos );
    sal_Size nRet = 0;
    if( nBytesLeft > 0 )
    {
        const sal_Size nMaxBuffer = 4096;
        sal_uInt8* pBuffer = new sal_uInt8[ ::std::min( nBytesLeft, nMaxBuffer ) ];
        bool bValid = true;

        while( bValid && (nBytesLeft > 0) )
        {
            sal_Size nWriteLen = ::std::min( nBytesLeft, nMaxBuffer );
            rInStrm.Read( pBuffer, nWriteLen );
            sal_Size nWriteRet = Write( pBuffer, nWriteLen );
            bValid = (nWriteLen == nWriteRet);
            nRet += nWriteRet;
            nBytesLeft -= nWriteRet;
        }
        delete[] pBuffer;
    }
    return nRet;
}

//UNUSED2008-05  void XclExpStream::WriteUnicodeBuffer( const sal_uInt16* pBuffer, sal_Size nChars, sal_uInt8 nFlags )
//UNUSED2008-05  {
//UNUSED2008-05      SetSliceSize( 0 );
//UNUSED2008-05      if( pBuffer && (nChars > 0) )
//UNUSED2008-05      {
//UNUSED2008-05          sal_uInt16 nCharLen = (nFlags & EXC_STRF_16BIT) ? 2 : 1;
//UNUSED2008-05          for( sal_Size nIndex = 0; nIndex < nChars; ++nIndex )
//UNUSED2008-05          {
//UNUSED2008-05              if( mbInRec && (mnCurrSize + nCharLen > mnCurrMaxSize) )
//UNUSED2008-05              {
//UNUSED2008-05                  StartContinue();
//UNUSED2008-05                  // repeat only 16bit flag
//UNUSED2008-05                  operator<<( static_cast< sal_uInt8 >( nFlags & EXC_STRF_16BIT ) );
//UNUSED2008-05              }
//UNUSED2008-05              if( nCharLen == 2 )
//UNUSED2008-05                  operator<<( pBuffer[ nIndex ] );
//UNUSED2008-05              else
//UNUSED2008-05                  operator<<( static_cast< sal_uInt8 >( pBuffer[ nIndex ] ) );
//UNUSED2008-05          }
//UNUSED2008-05      }
//UNUSED2008-05  }

void XclExpStream::WriteUnicodeBuffer( const ScfUInt16Vec& rBuffer, sal_uInt8 nFlags )
{
    SetSliceSize( 0 );
    nFlags &= EXC_STRF_16BIT;   // repeat only 16bit flag
    sal_uInt16 nCharLen = nFlags ? 2 : 1;

    ScfUInt16Vec::const_iterator aEnd = rBuffer.end();
    for( ScfUInt16Vec::const_iterator aIter = rBuffer.begin(); aIter != aEnd; ++aIter )
    {
        if( mbInRec && (mnCurrSize + nCharLen > mnCurrMaxSize) )
        {
            StartContinue();
            operator<<( nFlags );
        }
        if( nCharLen == 2 )
            operator<<( *aIter );
        else
            operator<<( static_cast< sal_uInt8 >( *aIter ) );
    }
}

//UNUSED2008-05  void XclExpStream::WriteByteStringBuffer( const ByteString& rString, sal_uInt16 nMaxLen )
//UNUSED2008-05  {
//UNUSED2008-05      SetSliceSize( 0 );
//UNUSED2008-05      Write( rString.GetBuffer(), ::std::min< sal_Size >( rString.Len(), nMaxLen ) );
//UNUSED2008-05  }

// ER: #71367# Xcl has an obscure sense of whether starting a new record or not,
// and crashes if it encounters the string header at the very end of a record.
// Thus we add 1 to give some room, seems like they do it that way but with another count (10?)
void XclExpStream::WriteByteString( const ByteString& rString, sal_uInt16 nMaxLen, bool b16BitCount )
{
    SetSliceSize( 0 );
    sal_Size nLen = ::std::min< sal_Size >( rString.Len(), nMaxLen );
    if( !b16BitCount )
        nLen = ::std::min< sal_Size >( nLen, 0xFF );

    sal_uInt16 nLeft = PrepareWrite();
    sal_uInt16 nLenFieldSize = b16BitCount ? 2 : 1;
    if( mbInRec && (nLeft <= nLenFieldSize) )
        StartContinue();

    if( b16BitCount )
        operator<<( static_cast< sal_uInt16 >( nLen ) );
    else
        operator<<( static_cast< sal_uInt8 >( nLen ) );
    Write( rString.GetBuffer(), nLen );
}

void XclExpStream::WriteCharBuffer( const ScfUInt8Vec& rBuffer )
{
    SetSliceSize( 0 );
    Write( &rBuffer[ 0 ], rBuffer.size() );
}

void XclExpStream::SetEncrypter( XclExpEncrypterRef xEncrypter )
{
    mxEncrypter = xEncrypter;
}

bool XclExpStream::HasValidEncrypter() const
{
    return mxEncrypter.is() && mxEncrypter->IsValid();
}

void XclExpStream::EnableEncryption( bool bEnable )
{
    mbUseEncrypter = bEnable && HasValidEncrypter();
}

void XclExpStream::DisableEncryption()
{
    EnableEncryption(false);
}

sal_Size XclExpStream::SetSvStreamPos( sal_Size nPos )
{
    DBG_ASSERT( !mbInRec, "XclExpStream::SetSvStreamPos - not allowed inside of a record" );
    return mbInRec ? 0 : mrStrm.Seek( nPos );
}

// private --------------------------------------------------------------------

void XclExpStream::InitRecord( sal_uInt16 nRecId )
{
    mrStrm.Seek( STREAM_SEEK_TO_END );
    mrStrm << nRecId;

    mnLastSizePos = mrStrm.Tell();
    mnHeaderSize = static_cast< sal_uInt16 >( ::std::min< sal_Size >( mnPredictSize, mnCurrMaxSize ) );
    mrStrm << mnHeaderSize;
    mnCurrSize = mnSliceSize = 0;
}

void XclExpStream::UpdateRecSize()
{
    if( mnCurrSize != mnHeaderSize )
    {
        mrStrm.Seek( mnLastSizePos );
        mrStrm << mnCurrSize;
    }
}

void XclExpStream::UpdateSizeVars( sal_Size nSize )
{
    DBG_ASSERT( mnCurrSize + nSize <= mnCurrMaxSize, "XclExpStream::UpdateSizeVars - record overwritten" );
    mnCurrSize = mnCurrSize + static_cast< sal_uInt16 >( nSize );

    if( mnMaxSliceSize > 0 )
    {
        DBG_ASSERT( mnSliceSize + nSize <= mnMaxSliceSize, "XclExpStream::UpdateSizeVars - slice overwritten" );
        mnSliceSize = mnSliceSize + static_cast< sal_uInt16 >( nSize );
        if( mnSliceSize >= mnMaxSliceSize )
            mnSliceSize = 0;
    }
}

void XclExpStream::StartContinue()
{
    UpdateRecSize();
    mnCurrMaxSize = mnMaxContSize;
    mnPredictSize -= mnCurrSize;
    InitRecord( EXC_ID_CONT );
}

void XclExpStream::PrepareWrite( sal_uInt16 nSize )
{
    if( mbInRec )
    {
        if( (mnCurrSize + nSize > mnCurrMaxSize) ||
            ((mnMaxSliceSize > 0) && (mnSliceSize == 0) && (mnCurrSize + mnMaxSliceSize > mnCurrMaxSize)) )
            StartContinue();
        UpdateSizeVars( nSize );
    }
}

sal_uInt16 XclExpStream::PrepareWrite()
{
    sal_uInt16 nRet = 0;
    if( mbInRec )
    {
        if( (mnCurrSize >= mnCurrMaxSize) ||
            ((mnMaxSliceSize > 0) && (mnSliceSize == 0) && (mnCurrSize + mnMaxSliceSize > mnCurrMaxSize)) )
            StartContinue();
        UpdateSizeVars( 0 );

        nRet = (mnMaxSliceSize > 0) ? (mnMaxSliceSize - mnSliceSize) : (mnCurrMaxSize - mnCurrSize);
    }
    return nRet;
}

void XclExpStream::WriteRawZeroBytes( sal_Size nBytes )
{
    const sal_uInt32 nData = 0;
    sal_Size nBytesLeft = nBytes;
    while( nBytesLeft >= sizeof( nData ) )
    {
        mrStrm << nData;
        nBytesLeft -= sizeof( nData );
    }
    if( nBytesLeft )
        mrStrm.Write( &nData, nBytesLeft );
}

// ============================================================================

XclExpBiff8Encrypter::XclExpBiff8Encrypter( const XclExpRoot& rRoot, const sal_uInt8 nDocId[16],
                                            const sal_uInt8 nSalt[16] ) :
    mrRoot(rRoot),
    mnOldPos(STREAM_SEEK_TO_END),
    mbValid(false)
{
    String aPass = rRoot.GetPassword();
    if (aPass.Len() == 0)
        // Empty password.  Get the default biff8 password.
        aPass = XclCryptoHelper::GetBiff8WbProtPassword();
    Init(aPass, nDocId, nSalt);
}

XclExpBiff8Encrypter::~XclExpBiff8Encrypter()
{
}

bool XclExpBiff8Encrypter::IsValid() const
{
    return mbValid;
}

void XclExpBiff8Encrypter::GetSaltDigest( sal_uInt8 nSaltDigest[16] ) const
{
    memcpy(nSaltDigest, mnSaltDigest, 16);
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, sal_uInt8 nData )
{
    vector<sal_uInt8> aByte(1);
    aByte[0] = nData;
    EncryptBytes(rStrm, aByte);
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, sal_uInt16 nData )
{
    ::std::vector<sal_uInt8> pnBytes(2);
    pnBytes[0] = nData & 0xFF;
    pnBytes[1] = (nData >> 8) & 0xFF;
    EncryptBytes(rStrm, pnBytes);
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, sal_uInt32 nData )
{
    ::std::vector<sal_uInt8> pnBytes(4);
    pnBytes[0] = nData & 0xFF;
    pnBytes[1] = (nData >>  8) & 0xFF;
    pnBytes[2] = (nData >> 16) & 0xFF;
    pnBytes[3] = (nData >> 24) & 0xFF;
    EncryptBytes(rStrm, pnBytes);
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, float fValue )
{
    ::std::vector<sal_uInt8> pnBytes(4);
    memcpy(&pnBytes[0], &fValue, 4);
    EncryptBytes(rStrm, pnBytes);
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, double fValue )
{
    ::std::vector<sal_uInt8> pnBytes(8);
    memcpy(&pnBytes[0], &fValue, 8);
    EncryptBytes(rStrm, pnBytes);
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, sal_Int8 nData )
{
    Encrypt(rStrm, static_cast<sal_uInt8>(nData));
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, sal_Int16 nData )
{
    Encrypt(rStrm, static_cast<sal_uInt16>(nData));
}

void XclExpBiff8Encrypter::Encrypt( SvStream& rStrm, sal_Int32 nData )
{
    Encrypt(rStrm, static_cast<sal_uInt32>(nData));
}

void XclExpBiff8Encrypter::Init( const String& aPass, const sal_uInt8 nDocId[16],
                                 const sal_uInt8 nSalt[16] )
{
    memset(mnSaltDigest, 0, sizeof(mnSaltDigest));

    xub_StrLen nLen = aPass.Len();
    bool bValid = (0 < nLen) && (nLen < 16);
    if ( bValid )
    {
        // transform String to sal_uInt16 array
        memset(mnPassw, 0, sizeof(mnPassw));
        for (xub_StrLen nChar = 0; nChar < nLen; ++nChar)
            mnPassw[nChar] = static_cast<sal_uInt16>(aPass.GetChar(nChar));

        // copy document ID
        memcpy(mnDocId, nDocId, sizeof(mnDocId));

        // init codec
        maCodec.InitKey(mnPassw, mnDocId);

        // generate salt hash.
        ::svx::MSCodec_Std97 aCodec;
        aCodec.InitKey(mnPassw, mnDocId);
        aCodec.CreateSaltDigest(nSalt, mnSaltDigest);

        // verify to make sure it's in good shape.
        bValid = maCodec.VerifyKey(nSalt, mnSaltDigest);
    }

    mbValid = bValid;
}

sal_uInt32 XclExpBiff8Encrypter::GetBlockPos( sal_Size nStrmPos ) const
{
    return static_cast<sal_uInt32>(nStrmPos / EXC_ENCR_BLOCKSIZE);
}

sal_uInt16 XclExpBiff8Encrypter::GetOffsetInBlock( sal_Size nStrmPos ) const
{
    return static_cast<sal_uInt16>(nStrmPos % EXC_ENCR_BLOCKSIZE);
}

void XclExpBiff8Encrypter::EncryptBytes( SvStream& rStrm, vector<sal_uInt8>& aBytes )
{
    sal_Size nStrmPos = rStrm.Tell();
    sal_uInt16 nBlockOffset = GetOffsetInBlock(nStrmPos);
    sal_uInt16 nBlockPos = GetBlockPos(nStrmPos);

#if DEBUG_XL_ENCRYPTION
    fprintf(stdout, "XclExpBiff8Encrypter::EncryptBytes: stream pos = %ld  offset in block = %d  block pos = %ld\n",
            nStrmPos, nBlockOffset, nBlockPos);
#endif

    sal_uInt16 nSize = aBytes.size();
    if (nSize == 0)
        return;

#if DEBUG_XL_ENCRYPTION    
    fprintf(stdout, "RAW: ");
    for (sal_uInt16 i = 0; i < nSize; ++i)
        fprintf(stdout, "%2.2X ", aBytes[i]);
    fprintf(stdout, "\n");
#endif    

    if (mnOldPos != nStrmPos)
    {
        sal_uInt16 nOldOffset = GetOffsetInBlock(mnOldPos);
        sal_uInt16 nOldBlockPos = GetBlockPos(mnOldPos);

        if ( (nBlockPos != nOldBlockPos) || (nBlockOffset < nOldOffset) )
        {
            maCodec.InitCipher(nBlockPos);
            nOldOffset = 0;
        }

        if (nBlockOffset > nOldOffset)
            maCodec.Skip(nBlockOffset - nOldOffset);
    }

    sal_uInt16 nBytesLeft = nSize;
    sal_uInt16 nPos = 0;
    while (nBytesLeft > 0)
    {
        sal_uInt16 nBlockLeft = EXC_ENCR_BLOCKSIZE - nBlockOffset;
        sal_uInt16 nEncBytes = ::std::min(nBlockLeft, nBytesLeft);

        bool bRet = maCodec.Encode(&aBytes[nPos], nEncBytes, &aBytes[nPos], nEncBytes);
        DBG_ASSERT(bRet, "XclExpBiff8Encrypter::EncryptBytes: encryption failed!!");

        sal_Size nRet = rStrm.Write(&aBytes[nPos], nEncBytes);
        DBG_ASSERT(nRet == nEncBytes, "XclExpBiff8Encrypter::EncryptBytes: fail to write to stream!!");

        nStrmPos = rStrm.Tell();
        nBlockOffset = GetOffsetInBlock(nStrmPos);
        nBlockPos = GetBlockPos(nStrmPos);
        if (nBlockOffset == 0)
            maCodec.InitCipher(nBlockPos);

        nBytesLeft -= nEncBytes;
        nPos += nEncBytes;
    }
    mnOldPos = nStrmPos;
}

static const char* lcl_GetErrorString( USHORT nScErrCode )
{
    sal_uInt8 nXclErrCode = XclTools::GetXclErrorCode( nScErrCode );
    switch( nXclErrCode )
    {
        case EXC_ERR_NULL:  return "#NULL!";
        case EXC_ERR_DIV0:  return "#DIV/0!";
        case EXC_ERR_VALUE: return "#VALUE!";
        case EXC_ERR_REF:   return "#REF!";
        case EXC_ERR_NAME:  return "#NAME?";
        case EXC_ERR_NUM:   return "#NUM!";
        case EXC_ERR_NA:    
        default:            return "#N/A";
    }
}

void XclXmlUtils::GetFormulaTypeAndValue( ScFormulaCell& rCell, const char*& rsType, OUString& rsValue )
{
    switch( rCell.GetFormatType() )
    {
        case NUMBERFORMAT_NUMBER:
        {
            // either value or error code
            USHORT nScErrCode = rCell.GetErrCode();
            if( nScErrCode )
            {
                rsType = "e";
                rsValue = ToOUString( lcl_GetErrorString( nScErrCode ) );
            }
            else
            {
                rsType = "n";
                rsValue = OUString::valueOf( rCell.GetValue() );
            }
        }
        break;

        case NUMBERFORMAT_TEXT:
        {
            rsType = "str";
            String aResult;
            rCell.GetString( aResult );
            rsValue = ToOUString( aResult );
        }
        break;

        case NUMBERFORMAT_LOGICAL:
        {
            rsType = "b";
            rsValue = ToOUString( rCell.GetValue() == 0.0 ? "0" : "1" );
        }
        break;

        default:
        {
            rsType = "inlineStr";
            String aResult;
            rCell.GetString( aResult );
            rsValue = ToOUString( aResult );
        }
        break;
    }
}

rtl::OUString XclXmlUtils::GetStreamName( const char* sStreamDir, const char* sStream, sal_Int32 nId )
{
    rtl::OUStringBuffer sBuf;
    if( sStreamDir )
        sBuf.appendAscii( sStreamDir );
    sBuf.appendAscii( sStream );
    if( nId )
        sBuf.append( nId );
    sBuf.appendAscii( ".xml" );
    return sBuf.makeStringAndClear();
}

rtl::OString XclXmlUtils::ToOString( const Color& rColor )
{
    char buf[9];
    sprintf( buf, "%.2X%.2X%.2X%.2X", rColor.GetTransparency(), rColor.GetRed(), rColor.GetGreen(), rColor.GetBlue() );
    buf[8] = '\0';
    return OString( buf );
}

rtl::OString XclXmlUtils::ToOString( const ::rtl::OUString& s )
{
    return OUStringToOString( s, RTL_TEXTENCODING_UTF8  );
}

rtl::OString XclXmlUtils::ToOString( const String& s )
{
    return rtl::OString( s.GetBuffer(), s.Len(), RTL_TEXTENCODING_UTF8 );
}

rtl::OString XclXmlUtils::ToOString( const ScAddress& rAddress )
{
    String sAddress;
    rAddress.Format( sAddress, SCA_VALID, NULL, ScAddress::Details( FormulaGrammar::CONV_XL_A1 ) );
    return ToOString( sAddress );
}

rtl::OString XclXmlUtils::ToOString( const ScfUInt16Vec& rBuffer )
{
    const sal_uInt16* pBuffer = &rBuffer [0];
    return ::rtl::OString( pBuffer, rBuffer.size(), RTL_TEXTENCODING_UTF8 );
}

rtl::OString XclXmlUtils::ToOString( const ScRange& rRange )
{
    String sRange;
    rRange.Format( sRange, SCA_VALID, NULL, ScAddress::Details( FormulaGrammar::CONV_XL_A1 ) );
    return ToOString( sRange );
}

rtl::OString XclXmlUtils::ToOString( const ScRangeList& rRangeList )
{
    String s;
    rRangeList.Format( s, SCA_VALID, NULL, FormulaGrammar::CONV_XL_A1, ' ' );
    return ToOString( s );
}

static ScAddress lcl_ToAddress( const XclAddress& rAddress )
{
    ScAddress aAddress;

    // For some reason, ScRange::Format() returns omits row numbers if 
    // the row is >= MAXROW or the column is >= MAXCOL, and Excel doesn't 
    // like "A:IV" (i.e. no row numbers).  Prevent this.
    aAddress.SetRow( std::min<sal_Int32>( rAddress.mnRow, MAXROW ) );
    aAddress.SetCol( static_cast<sal_Int16>(std::min<sal_Int32>( rAddress.mnCol, MAXCOL )) );

    return aAddress;
}

rtl::OString XclXmlUtils::ToOString( const XclAddress& rAddress )
{
    return ToOString( lcl_ToAddress( rAddress ) );
}

rtl::OString XclXmlUtils::ToOString( const XclExpString& s )
{
    DBG_ASSERT( !s.IsRich(), "XclXmlUtils::ToOString(XclExpString): rich text string found!" );
    return ToOString( s.GetUnicodeBuffer() );
}

static ScRange lcl_ToRange( const XclRange& rRange )
{
    ScRange aRange;

    aRange.aStart = lcl_ToAddress( rRange.maFirst );
    aRange.aEnd   = lcl_ToAddress( rRange.maLast );

    return aRange;
}

rtl::OString XclXmlUtils::ToOString( const XclRange& rRange )
{
    return ToOString( lcl_ToRange( rRange ) );
}

rtl::OString XclXmlUtils::ToOString( const XclRangeList& rRanges )
{
    ScRangeList aRanges;
    for( XclRangeList::const_iterator i = rRanges.begin(), end = rRanges.end();
            i != end; ++i )
    {
        aRanges.Append( lcl_ToRange( *i ) );
    }
    return ToOString( aRanges );
}

OUString XclXmlUtils::ToOUString( const char* s )
{
    return OUString( s, (sal_Int32) strlen( s ), RTL_TEXTENCODING_ASCII_US );
}

OUString XclXmlUtils::ToOUString( const ScfUInt16Vec& rBuf, sal_Int32 nStart, sal_Int32 nLength )
{
    if( nLength == -1 )
        nLength = rBuf.size();

    return OUString( &rBuf[nStart], nLength );
}

OUString XclXmlUtils::ToOUString( const String& s )
{
    return OUString( s.GetBuffer(), s.Len() );
}

rtl::OUString XclXmlUtils::ToOUString( ScDocument& rDocument, const ScAddress& rAddress, ScTokenArray* pTokenArray )
{
    ScCompiler aCompiler( &rDocument, rAddress, *pTokenArray);
    aCompiler.SetGrammar(FormulaGrammar::GRAM_ENGLISH_XL_A1);
    String s;
    aCompiler.CreateStringFromTokenArray( s );
    return ToOUString( s );
}

OUString XclXmlUtils::ToOUString( const XclExpString& s )
{
    DBG_ASSERT( !s.IsRich(), "XclXmlUtils::ToOString(XclExpString): rich text string found!" );
    return ToOUString( s.GetUnicodeBuffer() );
}

const char* XclXmlUtils::ToPsz( bool b )
{
    return b ? "true" : "false";
}

sax_fastparser::FSHelperPtr XclXmlUtils::WriteElement( sax_fastparser::FSHelperPtr pStream, sal_Int32 nElement, sal_Int32 nValue )
{
    pStream->startElement( nElement, FSEND );
    pStream->write( nValue );
    pStream->endElement( nElement );

    return pStream;
}

sax_fastparser::FSHelperPtr XclXmlUtils::WriteElement( sax_fastparser::FSHelperPtr pStream, sal_Int32 nElement, sal_Int64 nValue )
{
    pStream->startElement( nElement, FSEND );
    pStream->write( nValue );
    pStream->endElement( nElement );

    return pStream;
}

sax_fastparser::FSHelperPtr XclXmlUtils::WriteElement( sax_fastparser::FSHelperPtr pStream, sal_Int32 nElement, const char* sValue )
{
    pStream->startElement( nElement, FSEND );
    pStream->write( sValue );
    pStream->endElement( nElement );

    return pStream;
}

static void lcl_WriteValue( sax_fastparser::FSHelperPtr& rStream, sal_Int32 nElement, const char* pValue )
{
    if( !pValue )
        return;
    rStream->singleElement( nElement,
            XML_val, pValue,
            FSEND );
}

static const char* lcl_GetUnderlineStyle( FontUnderline eUnderline, bool& bHaveUnderline )
{
    bHaveUnderline = true;
    switch( eUnderline )
    {
        // OOXTODO: doubleAccounting, singleAccounting
        // OOXTODO: what should be done with the other FontUnderline values?
        case UNDERLINE_SINGLE:  return "single";
        case UNDERLINE_DOUBLE:  return "double";
        case UNDERLINE_NONE:
        default:                bHaveUnderline = false; return "none";
    }
}

static const char* lcl_ToVerticalAlignmentRun( SvxEscapement eEscapement, bool& bHaveAlignment )
{
    bHaveAlignment = true;
    switch( eEscapement )
    {
        case SVX_ESCAPEMENT_SUPERSCRIPT:    return "superscript";
        case SVX_ESCAPEMENT_SUBSCRIPT:      return "subscript";
        case SVX_ESCAPEMENT_OFF:
        default:                            bHaveAlignment = false; return "baseline";
    }
}

sax_fastparser::FSHelperPtr XclXmlUtils::WriteFontData( sax_fastparser::FSHelperPtr pStream, const XclFontData& rFontData, sal_Int32 nFontId )
{
    bool bHaveUnderline, bHaveVertAlign;
    const char* pUnderline = lcl_GetUnderlineStyle( rFontData.GetScUnderline(), bHaveUnderline );
    const char* pVertAlign = lcl_ToVerticalAlignmentRun( rFontData.GetScEscapement(), bHaveVertAlign );

    lcl_WriteValue( pStream, nFontId,        XclXmlUtils::ToOString( rFontData.maName ).getStr() );
    lcl_WriteValue( pStream, XML_charset,    rFontData.mnCharSet != 0 ? OString::valueOf( (sal_Int32) rFontData.mnCharSet ).getStr() : NULL );
    lcl_WriteValue( pStream, XML_family,     OString::valueOf( (sal_Int32) rFontData.mnFamily ).getStr() );
    lcl_WriteValue( pStream, XML_b,          rFontData.mnWeight > 400 ? XclXmlUtils::ToPsz( rFontData.mnWeight > 400 ) : NULL );
    lcl_WriteValue( pStream, XML_i,          rFontData.mbItalic ? XclXmlUtils::ToPsz( rFontData.mbItalic ) : NULL );
    lcl_WriteValue( pStream, XML_strike,     rFontData.mbStrikeout ? XclXmlUtils::ToPsz( rFontData.mbStrikeout ) : NULL );
    lcl_WriteValue( pStream, XML_outline,    rFontData.mbOutline ? XclXmlUtils::ToPsz( rFontData.mbOutline ) : NULL );
    lcl_WriteValue( pStream, XML_shadow,     rFontData.mbShadow ? XclXmlUtils::ToPsz( rFontData.mbShadow ) : NULL );
    // OOXTODO: lcl_WriteValue( rStream, XML_condense, );    // mac compatibility setting
    // OOXTODO: lcl_WriteValue( rStream, XML_extend, );      // compatibility setting
    if( rFontData.maColor != Color( 0xFF, 0xFF, 0xFF, 0xFF ) )
        pStream->singleElement( XML_color,
                // OOXTODO: XML_auto,       bool
                // OOXTODO: XML_indexed,    uint
                XML_rgb,    XclXmlUtils::ToOString( rFontData.maColor ).getStr(),
                // OOXTODO: XML_theme,      index into <clrScheme/>
                // OOXTODO: XML_tint,       double
                FSEND );
    lcl_WriteValue( pStream, XML_sz,         OString::valueOf( (double) (rFontData.mnHeight / 20.0) ) );  // Twips->Pt
    lcl_WriteValue( pStream, XML_u,          bHaveUnderline ? pUnderline : NULL );
    lcl_WriteValue( pStream, XML_vertAlign,  bHaveVertAlign ? pVertAlign : NULL );

    return pStream;
}


// ============================================================================

XclExpXmlStream::XclExpXmlStream( const Reference< XMultiServiceFactory >& rSMgr )
#if SUPD == 310
    : XmlFilterBase( comphelper::getProcessComponentContext() ),
#else	// SUPD == 310
    : XmlFilterBase( rSMgr ),
#endif	// SUPD == 310
      mpRoot( NULL )
{
}

XclExpXmlStream::~XclExpXmlStream()
{
}

sax_fastparser::FSHelperPtr& XclExpXmlStream::GetCurrentStream()
{
    DBG_ASSERT( !maStreams.empty(), "XclExpXmlStream::GetCurrentStream - no current stream" );
    return maStreams.top();
}

void XclExpXmlStream::PushStream( sax_fastparser::FSHelperPtr aStream )
{
    maStreams.push( aStream );
}

void XclExpXmlStream::PopStream()
{
    DBG_ASSERT( !maStreams.empty(), "XclExpXmlStream::PopStream - stack is empty!" );
    maStreams.pop();
}

OUString XclExpXmlStream::GetIdForPath( const OUString& sPath )
{
    if( maOpenedStreamMap.find( sPath ) == maOpenedStreamMap.end() )
        return OUString();
    return maOpenedStreamMap[ sPath ].first;
}

sax_fastparser::FSHelperPtr XclExpXmlStream::GetStreamForPath( const OUString& sPath )
{
    if( maOpenedStreamMap.find( sPath ) == maOpenedStreamMap.end() )
        return sax_fastparser::FSHelperPtr();
    return maOpenedStreamMap[ sPath ].second;
}

sax_fastparser::FSHelperPtr& XclExpXmlStream::WriteAttributes( sal_Int32 nAttribute, ... )
{
    sax_fastparser::FSHelperPtr& rStream = GetCurrentStream();

    va_list args;
    va_start( args, nAttribute );
    do {
        const char* pValue = va_arg( args, const char* );
        if( pValue )
        {
            rStream->write( " " )
                ->writeId( nAttribute )
                ->write( "=\"" )
                ->writeEscaped( pValue )
                ->write( "\"" );
        }

        nAttribute = va_arg( args, sal_Int32 );
#if SUPD == 310
        if( nAttribute == FSEND_internal )
#else	// SUPD == 310
        if( nAttribute == FSEND )
#endif	// SUPD == 310
            break;
    } while( true );
    va_end( args );

    return rStream;
}
sax_fastparser::FSHelperPtr XclExpXmlStream::CreateOutputStream ( 
    const OUString& sFullStream,
    const OUString& sRelativeStream,
    const Reference< XOutputStream >& xParentRelation, 
    const char* sContentType, 
    const char* sRelationshipType, 
    ::rtl::OUString* pRelationshipId )
{
    OUString sRelationshipId;
    if (xParentRelation.is())
        sRelationshipId = addRelation( xParentRelation, OUString::createFromAscii( sRelationshipType), sRelativeStream );
    else
        sRelationshipId = addRelation( OUString::createFromAscii( sRelationshipType ), sRelativeStream );

    if( pRelationshipId )
        *pRelationshipId = sRelationshipId;

#if SUPD == 310
    sax_fastparser::FSHelperPtr p = openFragmentStreamWithSerializer( sFullStream, OUString::createFromAscii( sContentType ) );
#else	// SUPD == 310
    sax_fastparser::FSHelperPtr p = openOutputStreamWithSerializer( sFullStream, OUString::createFromAscii( sContentType ) );
#endif	// SUPD == 310

    maOpenedStreamMap[ sFullStream ] = std::make_pair( sRelationshipId, p );

    return p;
}

bool XclExpXmlStream::importDocument() throw()
{
    return false;
}

sal_Int32 XclExpXmlStream::getSchemeClr( sal_Int32 /*nColorSchemeToken*/ ) const
{
    return -1;
}

const oox::vml::DrawingPtr XclExpXmlStream::getDrawings()
{
    return oox::vml::DrawingPtr();
}

#if SUPD == 310

oox::vml::Drawing* XclExpXmlStream::getVmlDrawing()
{
    return 0;
}

#endif	// SUPD == 310

const oox::drawingml::Theme* XclExpXmlStream::getCurrentTheme() const
{
    return NULL;
}

const oox::drawingml::table::TableStyleListPtr XclExpXmlStream::getTableStyles()
{
    return oox::drawingml::table::TableStyleListPtr();
}

oox::drawingml::chart::ChartConverter* XclExpXmlStream::getChartConverter()
{
    // just don't call...
    return NULL;
}

ScDocShell* XclExpXmlStream::getDocShell()
{
    Reference< XInterface > xModel( getModel(), UNO_QUERY );

    ScModelObj *pObj = dynamic_cast < ScModelObj* >( xModel.get() );

    if ( pObj )
        return reinterpret_cast < ScDocShell* >( pObj->GetEmbeddedObject() );

    return 0;
}

bool XclExpXmlStream::exportDocument() throw()
{
    ScDocShell* pShell = getDocShell();
    ScDocument* pDoc = pShell->GetDocument();
    SotStorageRef rStorage = dynamic_cast <SotStorage*>( Reference<XStorage>( pShell->GetStorage() ).get() );

    XclExpRootData aData( EXC_BIFF8, *pShell->GetMedium (), rStorage, *pDoc, RTL_TEXTENCODING_DONTKNOW );
    aData.meOutput = EXC_OUTPUT_XML_2007;
    XclExpRoot aRoot( aData );

    mpRoot = &aRoot;
    aRoot.GetOldRoot().pER = &aRoot;
    aRoot.GetOldRoot().eDateiTyp = Biff8;
    XclEscher escher( *aRoot.GetOldRoot ().pER, pDoc->GetTableCount () );
    aRoot.GetOldRoot().pEscher = &escher;

    if ( SvtFilterOptions* pOptions = SvtFilterOptions::Get() )
        if ( pShell && pOptions->IsLoadExcelBasicStorage() )
            if ( sal_uInt32 nError
                 = SvxImportMSVBasic( *pShell, *rStorage,
                                      pOptions->IsLoadExcelBasicCode(),
                                      pOptions->IsLoadExcelBasicStorage() )
                .SaveOrDelMSVBAStorage( true, EXC_STORAGE_VBA_PROJECT) )
            {
                pShell->SetError( nError );
            }

    OUString const workbook = CREATE_OUSTRING( "xl/workbook.xml" );
    PushStream( CreateOutputStream( workbook, workbook,
                                    Reference <XOutputStream>(),
                                    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml",
                                    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" ) );

    // destruct at the end of the block
    {
        ExcDocument aDocRoot( aRoot );
        aDocRoot.ReadDoc();
        aDocRoot.WriteXml( *this );
    }

    mpRoot = NULL;
    return true;
}

void XclExpXmlStream::Trace( const char* format, ...)
{
    va_list ap;
    va_start( ap, format );
    vfprintf( stderr, format, ap );
    va_end( ap );
}

//////////////////////////////////////////////////////////////////////////
// UNO stuff so that the filter is registered
//////////////////////////////////////////////////////////////////////////

#define IMPL_NAME "com.sun.star.comp.oox.ExcelFilterExport"

OUString XlsxExport_getImplementationName()
{
    return OUString( RTL_CONSTASCII_USTRINGPARAM( IMPL_NAME ) );
}

#if SUPD == 310

::oox::ole::VbaProject* XclExpXmlStream::implCreateVbaProject() const
{
    return new ::oox::xls::ExcelVbaProject( getComponentContext(), Reference< XSpreadsheetDocument >( getModel(), UNO_QUERY ) );
}

#endif	// SUPD == 310

::rtl::OUString XclExpXmlStream::implGetImplementationName() const
{
    return CREATE_OUSTRING( "TODO" );
}


Sequence< OUString > SAL_CALL XlsxExport_getSupportedServiceNames() throw()
{
    const OUString aServiceName( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.document.ExportFilter" ) );
    const Sequence< OUString > aSeq( &aServiceName, 1 );
    return aSeq;
}

Reference< XInterface > SAL_CALL XlsxExport_createInstance(const Reference< XMultiServiceFactory > & rSMgr ) throw( Exception )
{
    return (cppu::OWeakObject*) new XclExpXmlStream( rSMgr );
}

#ifdef __cplusplus
extern "C"
{
#endif

SAL_DLLPUBLIC_EXPORT void SAL_CALL component_getImplementationEnvironment( const sal_Char ** ppEnvTypeName, uno_Environment ** /* ppEnv */ )
{
    *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

SAL_DLLPUBLIC_EXPORT sal_Bool SAL_CALL component_writeInfo( void* /* pServiceManager */, void* pRegistryKey )
{
    sal_Bool bRet = sal_False;

    if( pRegistryKey )
    {
        try
        {
            Reference< XRegistryKey > xNewKey1(
                    static_cast< XRegistryKey* >( pRegistryKey )->createKey(                                
                        OUString::createFromAscii( IMPL_NAME "/UNO/SERVICES/" ) ) );
            xNewKey1->createKey( XlsxExport_getSupportedServiceNames().getConstArray()[0] );

            bRet = sal_True;
        }
        catch( InvalidRegistryException& )
        {
            OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
        }
    }

    return bRet;
}

// ------------------------
// - component_getFactory -
// ------------------------

SAL_DLLPUBLIC_EXPORT void* SAL_CALL component_getFactory( const sal_Char* pImplName, void* pServiceManager, void* /* pRegistryKey */ )
{
    Reference< XSingleServiceFactory > xFactory;
    void* pRet = 0;

    if ( rtl_str_compare( pImplName, IMPL_NAME ) == 0 )
    {
        const OUString aServiceName( OUString::createFromAscii( IMPL_NAME ) );

        xFactory = Reference< XSingleServiceFactory >( ::cppu::createSingleFactory(
                    reinterpret_cast< XMultiServiceFactory* >( pServiceManager ),
                    XlsxExport_getImplementationName(),
                    XlsxExport_createInstance,
                    XlsxExport_getSupportedServiceNames() ) );
    }

    if ( xFactory.is() )
    {
        xFactory->acquire();
        pRet = xFactory.get();
    }

    return pRet;
}

#ifdef __cplusplus
}
#endif
