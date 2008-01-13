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
 *  Patrick Luby, August 2007
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2007 Planamesa Inc.
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
#import "main_cocoa.h"

static AEEventHandlerUPP pOpenDocumentsHandlerUPP = nil;
static AEEventHandlerUPP pPrintDocumentsHandlerUPP = nil;
static AEEventHandlerUPP pQuitHandlerUPP = nil;

static OSErr CarbonOpenOrPrintDocuments( const AppleEvent *pEvent, AppleEvent *pReply, long nRef, BOOL bPrint )
{
	if ( pEvent )
	{
		AEDesc aDesc;
		if ( AEGetParamDesc( pEvent, keyDirectObject, typeAEList, &aDesc ) == noErr )
		{
			long nCount;
			if ( AECountItems( &aDesc, &nCount ) == noErr )
			{
				// Lists are one-based
				long i = 1;
				for ( ; i <= nCount; i++ )
				{
					FSRef aFSRef;
					AEKeyword aKeyword;
					DescType aType;
					long nSize;
					char aBuffer[ PATH_MAX + 1 ];
					if ( AEGetNthPtr( &aDesc, i, typeFSRef, &aKeyword, &aType, &aFSRef, sizeof( aFSRef ), &nSize ) == noErr && FSRefMakePath( &aFSRef, (UInt8 *)aBuffer, PATH_MAX ) == noErr )
						Application_openOrPrintFile( aBuffer, bPrint );
				}
			}
		}
	}

	return noErr;
}

static OSErr CarbonOpenDocumentsEventHandler( const AppleEvent *pEvent, AppleEvent *pReply, long nRef )
{
	return CarbonOpenOrPrintDocuments( pEvent, pReply, nRef, NO );
}

static OSErr CarbonPrintDocumentsEventHandler( const AppleEvent *pEvent, AppleEvent *pReply, long nRef )
{
	return CarbonOpenOrPrintDocuments( pEvent, pReply, nRef, YES );
}

static OSErr CarbonQuitEventHandler( const AppleEvent *pEvent, AppleEvent *pReply, long nRef )
{
	Application_queryExit();
	return noErr;
}

@interface DesktopApplicationDelegate : NSObject
- (BOOL)application:(NSApplication *)pApp openFile:(NSString *)pFilename;
- (void)application:(NSApplication *)pApp openFiles:(NSArray *)pFilenames;
- (BOOL)application:(NSApplication *)pApp printFile:(NSString *)pFilename;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)pApp;
@end

@implementation DesktopApplicationDelegate

- (BOOL)application:(NSApplication *)pApp openFile:(NSString *)pFilename
{
	if ( pFilename && [pFilename length] )
		Application_openOrPrintFile( [pFilename fileSystemRepresentation], NO );

	return YES;
}

- (void)application:(NSApplication *)pApp openFiles:(NSArray *)pFilenames
{
	if ( pFilenames )
	{
		unsigned count = [pFilenames count];
		int i = 0;
		for ( ; i < count; i++ )
			[self application:pApp openFile:[pFilenames objectAtIndex:i]];
	}
}

- (BOOL)application:(NSApplication *)pApp printFile:(NSString *)pFilename
{
	if ( pFilename && [pFilename length] )
		Application_openOrPrintFile( [pFilename fileSystemRepresentation], YES );

	return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)pApp
{
	Application_queryExit();
	return NSTerminateCancel;
}

@end

@interface DesktopApplication : NSApplication
- (void)sendEvent:(NSEvent *)pEvent;
@end

@implementation DesktopApplication

- (void)sendEvent:(NSEvent *)pEvent
{
	[super sendEvent:pEvent];

	// Handle the Command-Q event
	if ( pEvent && [pEvent type] == NSKeyDown && ( [pEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask ) == NSCommandKeyMask && [@"q" isEqualToString:[pEvent characters]] )
		[self performSelectorOnMainThread:@selector(terminate:) withObject:self waitUntilDone:NO];
}

@end

void NSApplication_run( CFRunLoopTimerRef aTimer, void *pInfo )
{
	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	// We need to override our own NSApplication methods to handle some menu
	// key equivalents in the default application menu
	[DesktopApplication poseAsClass:[NSApplication class]];

	// Load default application menu
	if ( NSApplicationLoad() )
	{
		// Make sure that X11.app is running
		NSTask *pTask = [[NSTask alloc] init];

		// Add to autorelease pool as invoking alloc disables autorelease
		[pTask autorelease];

		NSMutableArray *pArgs = [NSMutableArray array];
		if ( pArgs )
		{
			[pArgs addObject:@"-a"];
			[pArgs addObject:@"X11"];
			[pTask setArguments:pArgs];
			[pTask setLaunchPath:@"/usr/bin/open"];
			[pTask launch];
			[pTask waitUntilExit];
		}

		NSApplication *pApp = [NSApplication sharedApplication];
		if ( pApp )
		{
			// Do not retain as invoking alloc disables autorelease and do
			// not release as NSApplication does not retain the delegate
			[pApp setDelegate:[[DesktopApplicationDelegate alloc] init]];

			// Install event handler for open document events
			if ( !pOpenDocumentsHandlerUPP )
			{
				pOpenDocumentsHandlerUPP = NewAEEventHandlerUPP( CarbonOpenDocumentsEventHandler );
				if ( pOpenDocumentsHandlerUPP )
					AEInstallEventHandler( kCoreEventClass, kAEOpenDocuments, pOpenDocumentsHandlerUPP, 0, NO );
			}

			// Install event handler for print document events
			if ( !pPrintDocumentsHandlerUPP )
			{
				pPrintDocumentsHandlerUPP = NewAEEventHandlerUPP( CarbonPrintDocumentsEventHandler );
				if ( pPrintDocumentsHandlerUPP )
					AEInstallEventHandler( kCoreEventClass, kAEPrintDocuments, pPrintDocumentsHandlerUPP, 0, NO );
			}

			// Install event handler for the quit menu
			if ( !pQuitHandlerUPP )
			{
				pQuitHandlerUPP = NewAEEventHandlerUPP( CarbonQuitEventHandler );
				if ( pQuitHandlerUPP )
					AEInstallEventHandler( kCoreEventClass, kAEQuitApplication, pQuitHandlerUPP, 0, NO );
			}

			// Run native event loop
			[pApp run];
		}
	}

	if ( aTimer )
	{
		CFRunLoopRemoveTimer( CFRunLoopGetCurrent(), aTimer, kCFRunLoopDefaultMode );
		CFRelease( aTimer );
	}

	[pPool release];
}
