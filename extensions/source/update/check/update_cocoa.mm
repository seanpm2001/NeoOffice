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

#include <osl/objcutils.h>

#include "update_cocoa.hxx"
#include "update_java.hxx"
#include "updatei18n_cocoa.hxx"
#include "updatewebview_cocoa.h"

//========================================================================

NSString *kUpdateLastURLPref = @"updateLastURL";
NSString *kUpdateXPosPref = @"updateXPos";
NSString *kUpdateYPosPref = @"updateYPos";
NSString *kUpdateWidthPref = @"updateWidth";
NSString *kUpdateHeightPref = @"updateHeight";
NSString *kUpdateVisiblePref = @"updateVisible";
NSString *kUpdateServerTypePref = @"updateServerType";

#ifdef USE_NATIVE_WEB_VIEW
static UpdateNonRecursiveResponderWebPanel *pSharedPanel = nil;
#endif	// USE_NATIVE_WEB_VIEW

@interface UpdateCreateWebViewImpl : NSObject
{
	NSString*					mpTitle;
	NSString*					mpURL;
	NSString*					mpUserAgent;
	BOOL						mbWebViewShowing;
}
+ (id)createWithURL:(const NSString *)pURL userAgent:(const NSString *)pUserAgent title:(NSString *)pTitle;
- (id)initWithURL:(const NSString *)pURL userAgent:(const NSString *)pUserAgent title:(NSString *)pTitle;
- (BOOL)isWebViewShowing;
- (void)showWebView:(id)obj;
@end

@implementation UpdateCreateWebViewImpl

+ (id)createWithURL:(const NSString *)pURL userAgent:(const NSString *)pUserAgent title:(NSString *)pTitle
{
	UpdateCreateWebViewImpl *pRet = [[UpdateCreateWebViewImpl alloc] initWithURL:pURL userAgent:pUserAgent title:pTitle];
	[pRet autorelease];
	return pRet;
}

- (id)initWithURL:(NSString *)pURL userAgent:(NSString *)pUserAgent title:(NSString *)pTitle
{
	self = [super init];

	mpTitle = pTitle;
	mpURL = pURL;
	mpUserAgent = pUserAgent;
	mbWebViewShowing = NO;

	return(self);
}

- (BOOL)isWebViewShowing
{
	return mbWebViewShowing;
}

- (void)showWebView:(id)obj
{
	(void)obj;

	NSURL *pURL = [NSURL URLWithString:mpURL];
#ifdef USE_NATIVE_WEB_VIEW
	// If the OOo update check URL is out of sync with the server type that
	// the web view is using, do not use the web view to handle updates
	if ( !pURL || ![UpdateWebView isUpdateURL:pURL syncServer:NO] )
		return;

	if ( !pSharedPanel )
		pSharedPanel = [[UpdateNonRecursiveResponderWebPanel alloc] initWithUserAgent:mpUserAgent];

	if(pSharedPanel)
	{
		if ( mpTitle )
			[pSharedPanel setTitle:mpTitle];
		else
			[pSharedPanel setTitle:@""];

		NSUserDefaults *defaults=[NSUserDefaults standardUserDefaults];
		if(![pSharedPanel isVisible])
		{
			// Check for retained user position. If not available, make
			// relative to the primary frame.
			NSString *widthStr=[defaults stringForKey:kUpdateWidthPref];
			NSString *heightStr=[defaults stringForKey:kUpdateHeightPref];
			if(widthStr && heightStr)
			{
				NSSize contentSize={0, 0};
				contentSize.width=[widthStr intValue];
				contentSize.height=[heightStr intValue];
				[pSharedPanel setContentSize:contentSize];
			}

			NSPoint windowPos={75, 75};
			NSString *xPosStr=[defaults stringForKey:kUpdateXPosPref];
			NSString *yPosStr=[defaults stringForKey:kUpdateYPosPref];
			if(xPosStr && yPosStr)
			{
				windowPos.x=[xPosStr intValue];
				windowPos.y=[yPosStr intValue];
			}
			else
			{
				// Center on top of main window
				NSWindow *mainWindow=[NSApp mainWindow];
				if(mainWindow)
				{
					NSRect mainWindowFrame=[mainWindow frame];
					NSSize sharedPanelSize=[pSharedPanel frame].size;
					windowPos.x=mainWindowFrame.origin.x+((mainWindowFrame.size.width-sharedPanelSize.width)/2);
					windowPos.y=mainWindowFrame.origin.y+((mainWindowFrame.size.height-sharedPanelSize.height)/2);
				}
			}
			[pSharedPanel setFrameOrigin:windowPos];

			// Make sure window is visible
			[pSharedPanel makeKeyAndOrderFront:self];

			[defaults setBool:YES forKey:kUpdateVisiblePref];
			[defaults synchronize];
		}

		UpdateWebView *pWebView = [pSharedPanel webView];
		if(pWebView)
			[pWebView loadStartingURL:pURL];

		mbWebViewShowing = YES;
	}
#else	// USE_NATIVE_WEB_VIEW
	NSWorkspace *pWorkspace = [NSWorkspace sharedWorkspace];
	if ( pURL && pWorkspace )
	{
		mbWebViewShowing = [pWorkspace openURL:pURL];
		if ( !mbWebViewShowing )
		{
			NSURL *pAppURL = [pWorkspace URLForApplicationWithBundleIdentifier:@"com.apple.Safari"];
			NSWorkspaceOpenConfiguration *pConfiguration = [NSWorkspaceOpenConfiguration configuration];
			if ( pAppURL && pConfiguration )
			{
				[pWorkspace openURLs:[NSArray arrayWithObject:pURL] withApplicationAtURL:pAppURL configuration:pConfiguration completionHandler:nil];
				mbWebViewShowing = YES;
			}
		}
	}
#endif	// USE_NATIVE_WEB_VIEW
}
@end

@interface UpdateQuitWebViewImpl : NSObject
{
	BOOL						mbWebViewRequestedQuitApp;
}
+ (id)create;
- (id)init;
- (void)quitWebView:(id)obj;
- (BOOL)webViewRequestedQuitApp;
@end

@implementation UpdateQuitWebViewImpl

+ (id)create
{
	UpdateQuitWebViewImpl *pRet = [[UpdateQuitWebViewImpl alloc] init];
	[pRet autorelease];
	return pRet;
}

- (id)init
{
	self = [super init];

	mbWebViewRequestedQuitApp = NO;

	return(self);
}

- (void)quitWebView:(id)obj
{
	(void)obj;

#ifdef USE_NATIVE_WEB_VIEW
	if (pSharedPanel)
	{
 		if ([pSharedPanel isVisible])
			[pSharedPanel orderOut:self];

		UpdateWebView *pWebView = [pSharedPanel webView];
		if(pWebView)
			mbWebViewRequestedQuitApp = [pWebView requestedQuitApp];
	}
#endif	// USE_NATIVE_WEB_VIEW

	if (!mbWebViewRequestedQuitApp && UpdateHasPackagePaths())
	{
		NSAlert *pAlert = [[NSAlert alloc] init];
		if ( pAlert )
		{
			[pAlert autorelease];

			NSString *pMessageText = UpdateGetLocalizedString(UPDATEINSTALLUPDATES);
			if ( pMessageText )
				pAlert.messageText = pMessageText;
			NSString *pDefaultButton = UpdateGetVCLResString(SV_BUTTONTEXT_YES);
			if ( pDefaultButton )
				[pAlert addButtonWithTitle:pDefaultButton];
			NSString *pAlternateButton = UpdateGetVCLResString(SV_BUTTONTEXT_NO);
			if ( pAlternateButton )
				[pAlert addButtonWithTitle:pAlternateButton];

			if ( [pAlert runModal] == NSAlertFirstButtonReturn )
			{
				mbWebViewRequestedQuitApp = YES;
				CFPreferencesSetAppValue( kUpdateSuppressLaunchAfterInstallationPref, kCFBooleanTrue, kCFPreferencesCurrentApplication );
				CFPreferencesAppSynchronize( kCFPreferencesCurrentApplication );
			}
		}
	}
}

- (BOOL)webViewRequestedQuitApp
{
	return mbWebViewRequestedQuitApp;
}

@end

OUString UpdateNSStringToOUString( NSString *pString )
{
	if ( !pString )
		return OUString();

	unsigned int nLen = [pString length];
	if ( !nLen )
		return OUString();

	sal_Unicode aBuf[ nLen + 1 ];
	[pString getCharacters:aBuf];
	aBuf[ nLen ] = 0;

	return OUString( aBuf );
}

OUString UpdateGetOSVersion()
{
	OUString aRet;

	NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];

	NSProcessInfo *pProcessInfo = [NSProcessInfo processInfo];
	if ( pProcessInfo )
	{
		aRet = OUString::number( (sal_Int32)[pProcessInfo operatingSystemVersion].majorVersion );
        aRet += ".";
		aRet += OUString::number( (sal_Int32)[pProcessInfo operatingSystemVersion].minorVersion );
        aRet += ".";
		aRet += OUString::number( (sal_Int32)[pProcessInfo operatingSystemVersion].patchVersion );
	}

	[pool release];

	return aRet;
}

sal_Bool UpdateQuitNativeDownloadWebView()
{
	sal_Bool bRet = sal_False;

	NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];

	UpdateQuitWebViewImpl *pImp = [UpdateQuitWebViewImpl create];
	osl_performSelectorOnMainThread( pImp, @selector(quitWebView:), pImp, YES );
	bRet = (sal_Bool)[pImp webViewRequestedQuitApp];

	[pool release];

	return bRet;
}

sal_Bool UpdateShowNativeDownloadWebView( OUString aURL, OUString aUserAgent, OUString aTitle )
{
	sal_Bool bRet = sal_False;

	NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];

	NSString *pURL = [NSString stringWithCharacters:aURL.getStr() length:aURL.getLength()];
	NSString *pUserAgent = [NSString stringWithCharacters:aUserAgent.getStr() length:aUserAgent.getLength()];
	NSString *pTitle = [NSString stringWithCharacters:aTitle.getStr() length:aTitle.getLength()];
	if ( pURL )
	{
		UpdateCreateWebViewImpl *pImp = [UpdateCreateWebViewImpl createWithURL:pURL userAgent:pUserAgent title:pTitle];
		osl_performSelectorOnMainThread( pImp, @selector(showWebView:), pImp, YES );
		bRet = (sal_Bool)[pImp isWebViewShowing];
	}

	[pool release];

	return bRet;
}
