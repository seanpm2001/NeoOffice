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

#include <dlfcn.h>
#import <Cocoa/Cocoa.h>
#import "VCLPageFormat_cocoa.h"
#import "VCLPrintJob_cocoa.h"

typedef OSStatus PMSetJobNameCFString_Type( PMPrintSettings aSettings, CFStringRef aName );

@interface ShowPrintDialog : NSObject
{
	BOOL					mbFinished;
	NSPrintInfo*			mpInfo;
	CFStringRef				maJobName;
	NSPrintOperation*		mpPrintOperation;
	NSWindow*				mpWindow;
}
- (void)cleanUpPrintOperation:(id)pObject;
- (void)dealloc;
- (BOOL)finished;
- (id)initWithPrintInfo:(NSPrintInfo *)pInfo window:(NSWindow *)pWindow jobName:(CFStringRef)aJobName;
- (NSPrintOperation *)printOperation;
- (void)printPanelDidEnd:(NSPrintPanel *)pPanel returnCode:(NSInteger)nCode contextInfo:(void *)pContextInfo;
- (void)setJobTitle;
- (void)showPrintDialog:(id)pObject;
@end

@implementation ShowPrintDialog

- (void)cleanUpPrintOperation:(id)pObject
{
	if ( mpPrintOperation )
		[mpPrintOperation cleanUpOperation];
}

- (void)dealloc
{
	if ( mpInfo )
		[mpInfo release];

	if ( maJobName )
		CFRelease( maJobName );

	if ( mpPrintOperation )
		[mpPrintOperation release];

	if ( mpWindow )
		[mpWindow release];

	[super dealloc];
}

- (BOOL)finished
{
	return mbFinished;
}

- (id)initWithPrintInfo:(NSPrintInfo *)pInfo window:(NSWindow *)pWindow jobName:(CFStringRef)aJobName
{
	[super init];

	mbFinished = NO;
	mpInfo = pInfo;
	if ( mpInfo )
		[mpInfo retain];
	maJobName = aJobName;
	if ( !maJobName )
		maJobName = CFSTR( "Untitled" );
	if ( maJobName )
		CFRetain( maJobName );
	mpPrintOperation = mpPrintOperation;
	mpWindow = pWindow;
	if ( mpWindow )
		[mpWindow retain];

	return self;
}

- (NSPrintOperation *)printOperation
{
	return mpPrintOperation;
}

- (void)printPanelDidEnd:(NSPrintPanel *)pPanel returnCode:(NSInteger)nCode contextInfo:(void *)pContextInfo
{
	mbFinished = YES;

	if ( nCode == NSOKButton )
	{
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

		if ( !mpPrintOperation )
		{
			NSView *pView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
			if ( pView )
			{
				[pView autorelease];

				mpPrintOperation = [VCLPrintOperation printOperationWithView:pView printInfo:mpInfo];
				if ( mpPrintOperation )
				{
					[mpPrintOperation retain];
					[mpPrintOperation setJobTitle:(NSString *)maJobName];
				}
			}
		}
	}
}

- (void)setJobTitle
{
	// Also set the job name via the PMPrintSettings so that the Save As
	// dialog gets the job name
	void *pLib = dlopen( NULL, RTLD_LAZY | RTLD_LOCAL );
	if ( pLib )
	{
		PMSetJobNameCFString_Type *pPMSetJobNameCFString = (PMSetJobNameCFString_Type *)dlsym( pLib, "PMSetJobNameCFString" );
		if ( pPMSetJobNameCFString )
		{
			PMPrintSettings aSettings = (PMPrintSettings)[mpInfo PMPrintSettings];
			if ( aSettings )
				pPMSetJobNameCFString( aSettings, maJobName );
		}

		dlclose( pLib );
	}
}

- (void)showPrintDialog:(id)pObject
{
	if ( mbFinished )
		return;

	// Set job title
	[self setJobTitle];

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

		NSPrinter *pPrinter = [NSPrintInfo defaultPrinter];
		if ( pPrinter )
			[mpInfo setPrinter:pPrinter];

		// Fix bug 3614 by displaying a modal print dialog if there is no
		// window to attach a sheet to
		if ( !mpWindow )
		{
			NSView *pView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
			if ( pView )
			{
				[pView autorelease];

				NSPrintOperation *pOperation = [VCLPrintOperation printOperationWithView:pView printInfo:mpInfo];
				if ( pOperation )
				{
					[pOperation setJobTitle:(NSString *)maJobName];

					NSPrintOperation *pOldOperation = [NSPrintOperation currentOperation];
					[NSPrintOperation setCurrentOperation:pOperation];

					NSInteger nButton = [pPanel runModal];
					if ( nButton == NSOKButton )
					{
						// Copy any dictionary changes
						NSPrintInfo *pInfo = [pOperation printInfo];
						if ( pInfo && pInfo != mpInfo )
						{
							NSMutableDictionary *pSrcDict = [pInfo dictionary];
							NSMutableDictionary *pDestDict = [mpInfo dictionary];
							if ( pSrcDict && pDestDict )
							{
								NSEnumerator *pSrcKeys = [pSrcDict keyEnumerator];
								if ( pSrcKeys )
								{
									id pKey;
									while ( ( pKey = [pSrcKeys nextObject] ) )
									{
										id pValue = [pSrcDict objectForKey:pKey];
										if ( pValue )
											[pDestDict setObject:pValue forKey:pKey];
									}
								}
							}
						}
					}

					[self printPanelDidEnd:pPanel returnCode:nButton contextInfo:nil];
					[pOperation cleanUpOperation];
					[NSPrintOperation setCurrentOperation:pOldOperation];
				}
			}
		}
		else
		{
			[pPanel beginSheetWithPrintInfo:mpInfo modalForWindow:mpWindow delegate:self didEndSelector:@selector(printPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
		}
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

id NSPrintInfo_showPrintDialog( id pNSPrintInfo, id pNSWindow, CFStringRef aJobName )
{
	ShowPrintDialog *pRet = nil;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pNSPrintInfo )
	{
		// Do not retain as invoking alloc disables autorelease
		pRet = [[ShowPrintDialog alloc] initWithPrintInfo:(NSPrintInfo *)pNSPrintInfo window:(NSWindow *)pNSWindow jobName:aJobName];
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

id NSPrintPanel_printOperation( id pDialog )
{
	id pRet = nil;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pDialog )
		pRet = [(ShowPrintDialog *)pDialog printOperation];

	[pPool release];

	return pRet;
}

void NSPrintPanel_release( id pDialog )
{
	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pDialog )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		[(ShowPrintDialog *)pDialog performSelectorOnMainThread:@selector(cleanUpPrintOperation:) withObject:pDialog waitUntilDone:YES modes:pModes];
		[pDialog release];
	}

	[pPool release];
}
