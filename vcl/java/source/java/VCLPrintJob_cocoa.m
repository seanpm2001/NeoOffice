/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Patrick Luby, September 2005
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2005 Planamesa Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

#import <Cocoa/Cocoa.h>
#import "VCLPageFormat_cocoa.h"
#import "VCLPrintJob_cocoa.h"

@interface ShowPrintDialog : NSObject
{
	BOOL					mbFinished;
	NSPrintInfo*			mpInfo;
	BOOL					mbResult;
	NSWindow*				mpWindow;
}
- (void)dealloc;
- (BOOL)finished;
- (id)initWithPrintInfo:(NSPrintInfo *)pInfo window:(NSWindow *)pWindow;
- (void)printPanelDidEnd:(NSPrintPanel *)pPanel returnCode:(int)nCode contextInfo:(void *)pContextInfo;
- (BOOL)result;
- (void)showPrintDialog:(id)pObject;
@end

@implementation ShowPrintDialog

- (void)dealloc
{
	if ( mpInfo )
		[mpInfo release];

	if ( mpWindow )
		[mpWindow release];

	[super dealloc];
}

- (BOOL)finished
{
	return mbFinished;
}

- (id)initWithPrintInfo:(NSPrintInfo *)pInfo window:(NSWindow *)pWindow
{
	[super init];

	mbFinished = YES;
	mpInfo = pInfo;
	if ( mpInfo )
		[mpInfo retain];
	mbResult = NO;
	mpWindow = pWindow;
	if ( mpWindow )
		[mpWindow retain];

	return self;
}

- (void)printPanelDidEnd:(NSPrintPanel *)pPanel returnCode:(int)nCode contextInfo:(void *)pContextInfo
{
	mbFinished = YES;
	if ( nCode == NSOKButton )
	{
		mbResult = YES;

		// Cache the dialog's print info in its own dictionary as Java seems
		// to copy the dictionary into a different print info instance when
		// actually printing so we will retrieve the cached print info when
		// creating the print operation. Fix bug 2873 by making a full copy
		// of the print info and its dictionary so that Java cannot tweak
		// the settings in either object.
		NSMutableDictionary *pDictionary = [mpInfo dictionary];
		if ( pDictionary )
		{
			[pDictionary removeObjectForKey:(NSString *)VCLPrintInfo_getVCLPrintInfoDictionaryKey()];

			NSPrintInfo *pInfoCopy = [mpInfo copy];
			if ( pInfoCopy )
			{
				// Add to autorelease pool as invoking alloc disables
				// autorelease
				[pInfoCopy autorelease];

				[pDictionary setObject:pInfoCopy forKey:(NSString *)VCLPrintInfo_getVCLPrintInfoDictionaryKey()];
			}
		}
	}
	else
	{
		mbResult = NO;
	}
}

- (BOOL)result
{
	return mbResult;
}

- (void)showPrintDialog:(id)pObject
{
	// Set the job name from the window title
	PMPrintSettings aSettings = nil;
	if ( [mpInfo respondsToSelector:@selector(PMPrintSettings)] )
		aSettings = (PMPrintSettings)[mpInfo PMPrintSettings];
	else if ( [mpInfo respondsToSelector:@selector(pmPrintSettings)] )

		aSettings = (PMPrintSettings)[mpInfo pmPrintSettings];

	if ( aSettings )
	{
		NSString *pTitle = [mpWindow title];
		if ( pTitle )
			pTitle = [pTitle stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		else
			pTitle = [NSString stringWithString:@"Untitled"];

		if ( pTitle)
			PMSetJobNameCFString( aSettings, (CFStringRef)pTitle );
	}

	NSPrintPanel *pPanel = [NSPrintPanel printPanel];
	if ( pPanel )
	{
		// Fix bug 1548 by setting to print all pages
		NSMutableDictionary *pDictionary = [mpInfo dictionary];
		if ( pDictionary )
		{
			[pDictionary setObject:[NSNumber numberWithBool:YES] forKey:NSPrintAllPages];
			[pDictionary setObject:[NSNumber numberWithBool:YES] forKey:NSPrintMustCollate];
			[pDictionary removeObjectForKey:NSPrintCopies];
			[pDictionary removeObjectForKey:NSPrintFirstPage];
			[pDictionary removeObjectForKey:NSPrintLastPage];

			// Fix bug 2030 by resetting the layout
			[pDictionary setObject:[NSNumber numberWithUnsignedInt:1] forKey:@"NSPagesAcross"];
			[pDictionary setObject:[NSNumber numberWithUnsignedInt:1] forKey:@"NSPagesDown"];

			[pDictionary removeObjectForKey:(NSString *)VCLPrintInfo_getVCLPrintInfoDictionaryKey()];
		}

		mbFinished = NO;
		NSPrinter *pPrinter = [NSPrintInfo defaultPrinter];
		if ( pPrinter )
			[mpInfo setPrinter:pPrinter];
		[pPanel beginSheetWithPrintInfo:mpInfo modalForWindow:mpWindow delegate:self didEndSelector:@selector(printPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
	}
}

@end

BOOL NSPrintInfo_pageRange( id pNSPrintInfo, int *nFirst, int *nLast )
{
	BOOL bRet = NO;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pNSPrintInfo && nFirst && nLast )
	{
		NSMutableDictionary *pDictionary = [(NSPrintInfo *)pNSPrintInfo dictionary];
		if ( pDictionary )
		{
			NSNumber *pNumber = [pDictionary objectForKey:NSPrintAllPages];
			if ( !pNumber || ![pNumber boolValue] )
			{
				NSNumber *pFirst = [pDictionary objectForKey:NSPrintFirstPage];
				NSNumber *pLast = [pDictionary objectForKey:NSPrintLastPage];
				if ( pFirst && pLast )
				{
					*nFirst = [pFirst intValue];
					*nLast = [pLast intValue];
					if ( *nFirst > 0 && *nLast >= *nFirst )
						bRet = YES;
				}
			}
		}
	}

	[pPool release];

	return bRet;
}

float NSPrintInfo_scale( id pNSPrintInfo )
{
	float fRet = 1.0;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pNSPrintInfo )
	{
		NSMutableDictionary *pDictionary = [(NSPrintInfo *)pNSPrintInfo dictionary];
		if ( pDictionary )
		{
			NSNumber *pNumber = [pDictionary objectForKey:NSPrintScalingFactor];
			if ( pNumber )
				fRet = [pNumber floatValue];
		}
	}

	[pPool release];

	return fRet;
}

id NSPrintInfo_showPrintDialog( id pNSPrintInfo, id pNSWindow )
{
	ShowPrintDialog *pRet = nil;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pNSPrintInfo && pNSWindow )
	{
		// Do not retain as invoking alloc disables autorelease
		pRet = [[ShowPrintDialog alloc] initWithPrintInfo:(NSPrintInfo *)pNSPrintInfo window:(NSWindow *)pNSWindow];
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		[pRet performSelectorOnMainThread:@selector(showPrintDialog:) withObject:pRet waitUntilDone:YES modes:pModes];
	}

	[pPool release];

	return pRet;
}

BOOL NSPrintPanel_finished( id pDialog )
{
	BOOL bRet = YES;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pDialog )
		bRet = [(ShowPrintDialog *)pDialog finished];

	[pPool release];

	return bRet;
}

BOOL NSPrintPanel_result( id pDialog )
{
	BOOL bRet = NO;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pDialog )
	{
		bRet = [(ShowPrintDialog *)pDialog result];
		[(ShowPrintDialog *)pDialog release];
	}

	[pPool release];

	return bRet;
}
