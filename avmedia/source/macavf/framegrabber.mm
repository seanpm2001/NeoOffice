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
 *   Modified February 2017 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "framegrabber.hxx"
#include "player.hxx"

#include <tools/stream.hxx>
#include <vcl/graph.hxx>
#include <vcl/cvtgrf.hxx>
#include <unotools/localfilehelper.hxx>

using namespace ::com::sun::star;

namespace avmedia { namespace macavf {

FrameGrabber::FrameGrabber( const uno::Reference< lang::XMultiServiceFactory >& /*rxMgr*/ )
:   mpImageGen( nullptr )
{}


FrameGrabber::~FrameGrabber()
{
    if( mpImageGen )
        CFRelease( mpImageGen );
}


bool FrameGrabber::create( const ::rtl::OUString& rURL )
{
    NSString* pNSStr = [NSString stringWithCharacters:reinterpret_cast<unichar const *>(rURL.getStr()) length:rURL.getLength()];
#ifdef USE_JAVA
    NSURL* pNSURL = [NSURL URLWithString: [pNSStr stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLPathAllowedCharacterSet]]];
#else	// USE_JAVA
    SAL_WNODEPRECATED_DECLARATIONS_PUSH
        //TODO: 10.11 stringByAddingPercentEscapesUsingEncoding
    NSURL* pNSURL = [NSURL URLWithString: [pNSStr stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    SAL_WNODEPRECATED_DECLARATIONS_POP
#endif	// USE_JAVA
    AVAsset* pMovie = [AVURLAsset URLAssetWithURL:pNSURL options:nil];
    if( !pMovie )
    {
        SAL_WARN("avmedia", "AVGrabber::create() cannot load url=" << [pNSStr UTF8String] );
        return false;
    }

    return create( pMovie );
}


bool FrameGrabber::create( AVAsset* pMovie )
{
    if( [[pMovie tracksWithMediaType:AVMediaTypeVideo] count] == 0)
    {
        SAL_WARN("avmedia", "AVGrabber::create() found no video content!" );
        return false;
    }

    mpImageGen = [AVAssetImageGenerator assetImageGeneratorWithAsset:pMovie];
    CFRetain( mpImageGen );
    return true;
}


uno::Reference< graphic::XGraphic > SAL_CALL FrameGrabber::grabFrame( double fMediaTime )
{
    uno::Reference< graphic::XGraphic > xRet;
    if( !mpImageGen )
        return xRet;

    // get the requested image from the movie
    CGImage* pCGImage = [mpImageGen copyCGImageAtTime:CMTimeMakeWithSeconds(fMediaTime,1000) actualTime:nullptr error:nullptr];

    // convert the image to a TIFF-formatted byte-array
    CFMutableDataRef pCFData = CFDataCreateMutable( kCFAllocatorDefault, 0 );
    CGImageDestination* pCGImgDest = CGImageDestinationCreateWithData( pCFData, kUTTypeTIFF, 1, nullptr );
    CGImageDestinationAddImage( pCGImgDest, pCGImage, nullptr );
    CGImageDestinationFinalize( pCGImgDest );
    CFRelease( pCGImgDest );
    const long nBitmapLen = CFDataGetLength( pCFData );
    UInt8 * pBitmapBytes = const_cast<UInt8 *>(CFDataGetBytePtr( pCFData ));

    // convert the image into the return-value type which is a graphic::XGraphic
    SvMemoryStream aMemStm( pBitmapBytes, nBitmapLen, StreamMode::READ | StreamMode::WRITE );
    Graphic aGraphic;
    if( GraphicConverter::Import( aMemStm, aGraphic, ConvertDataFormat::TIF ) == ERRCODE_NONE )
        xRet = aGraphic.GetXGraphic();

    // clean up resources
    CFRelease( pCFData );
    return xRet;
}


::rtl::OUString SAL_CALL FrameGrabber::getImplementationName(  )
{
    return ::rtl::OUString( AVMEDIA_MACAVF_FRAMEGRABBER_IMPLEMENTATIONNAME );
}

sal_Bool SAL_CALL FrameGrabber::supportsService( const ::rtl::OUString& ServiceName )
{
    return ServiceName == AVMEDIA_MACAVF_FRAMEGRABBER_SERVICENAME;
}

uno::Sequence< ::rtl::OUString > SAL_CALL FrameGrabber::getSupportedServiceNames(  )
{
    return { AVMEDIA_MACAVF_FRAMEGRABBER_SERVICENAME };
}

} // namespace macavf
} // namespace avmedia

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
