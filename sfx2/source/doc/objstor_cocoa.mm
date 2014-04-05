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
 *  Patrick Luby, April 2014
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2014 Planamesa Inc.
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

#include <osl/file.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/objsh.hxx>
#include <unotools/tempfile.hxx>
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>

#include <premac.h>
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>
#include <postmac.h>

#import "objstor_cocoa.h"

// Uncomment the following line to limit file coordination to only iCloud URLs
// #define FILE_COORDINATION_FOR_ICLOUD_ONLY

#ifndef NSFileCoordinatorWritingOptions
enum {
	NSFileCoordinatorWritingForDeleting = 1 << 0,
	NSFileCoordinatorWritingForReplacing = 1 << 3,
	NSFileCoordinatorWritingForMoving = 1 << 1,
	NSFileCoordinatorWritingForMerging = 1 << 2
};
typedef NSUInteger NSFileCoordinatorWritingOptions;
#endif	// !NSFileCoordinatorWritingOptions

using namespace rtl;
using namespace vos;

@interface NSFileManager (RunSFXFileCoordinator)
- (BOOL)isUbiquitousItemAtURL:(NSURL *)pURL;
@end

static NSURL *NSURLFromOUString( OUString aURL )
{
	NSURL *pRet = nil;

	if ( aURL.indexOf( OUString( RTL_CONSTASCII_USTRINGPARAM( "file://" ) ) ) == 0 )
	{
		// Exclude our own temporary files to avoid degrading performance
		OUString aPath;
		if ( ::osl::FileBase::getSystemPathFromFileURL( aURL, aPath ) == ::osl::FileBase::E_None && aPath.getLength() && aPath.indexOf( ::utl::TempFile::GetTempNameBaseDirectory() ) != 0 )
		{
			NSString *pString = [NSString stringWithCharacters:aPath.getStr() length:aPath.getLength()];
			if ( pString )
			{
				pRet = [NSURL fileURLWithPath:pString];
				if ( pRet )
				{
					pRet = [pRet URLByStandardizingPath];
					if ( pRet )
					{
						pRet = [pRet URLByResolvingSymlinksInPath];
#ifdef FILE_COORDINATION_FOR_ICLOUD_ONLY
						if ( pRet )
						{
							NSFileManager *pFileManager = [NSFileManager defaultManager];
							if ( !pFileManager || ![pFileManager respondsToSelector:@selector(isUbiquitousItemAtURL:)] || ![pFileManager isUbiquitousItemAtURL:pRet] )
								pRet = nil;
						}
#endif	// FILE_COORDINATION_FOR_ICLOUD_ONLY
					}
				}
			}
		}
	}

	return pRet;
}

static NSURL *NSURLFromSfxMedium( SfxMedium *pMedium )
{
	NSURL *pRet = nil;

	if ( pMedium )
		pRet = NSURLFromOUString( pMedium->GetBaseURL( true ) );

	return pRet;
}

@interface NSObject (NSFileCoordinator)
- (id)initWithFilePresenter:(id)pFilePresenter;
- (void)coordinateWritingItemAtURL:(NSURL *)pURL options:(NSFileCoordinatorWritingOptions)nOptions error:(NSError **)ppOutError byAccessor:(void (^)(NSURL *pNewURL))aWriter;
- (void)coordinateWritingItemAtURL:(NSURL *)pURL1 options:(NSFileCoordinatorWritingOptions)nOptions1 writingItemAtURL:(NSURL *)pURL2 options:(NSFileCoordinatorWritingOptions)nOptions2 error:(NSError **)ppOutError byAccessor:(void (^)(NSURL *pNewURL1, NSURL *pNewURL2))aWriter;
@end

@interface NSDocument (RunSFXFileCoordinator)
- (void)performSynchronousFileAccessUsingBlock:(void (^)(void))aBlock;
@end

@interface RunSFXFileCoordinator : NSObject
{
	SfxObjectShell*			mpObjShell;
	const String*			mpFileName;
	const String*			mpFilterName;
	const SfxItemSet*		mpSet;
	SfxMedium*				mpMedium;
	BOOL*					mpCommit;
	sal_Bool				mbResult;
}
+ (id)createWithObjectShell:(SfxObjectShell *)pObjShell fileName:(const String *)pFileName filterName:(const String *)pFilterName itemSet:(const SfxItemSet *)pSet medium:(SfxMedium *)pMedium commit:(BOOL *)pCommit;
- (NSDocument *)documentForURL:(NSURL *)pURL;
- (NSObject *)fileCoordinator;
- (id)initWithObjectShell:(SfxObjectShell *)pObjShell fileName:(const String *)pFileName filterName:(const String *)pFilterName itemSet:(const SfxItemSet *)pSet medium:(SfxMedium *)pMedium commit:(BOOL *)pCommit;
- (void)objectShellDoSave:(id)pObject;
- (void)objectShellDoSaveAs:(id)pObject;
- (void)objectShellDoSaveObjectAs:(id)pObject;
- (void)objectShellDoSave_Impl:(id)pObject;
- (void)objectShellPreDoSaveAs_Impl:(id)pObject;
- (void)objectShellSave_Impl:(id)pObject;
@end

@implementation RunSFXFileCoordinator

+ (id)createWithObjectShell:(SfxObjectShell *)pObjShell fileName:(const String *)pFileName filterName:(const String *)pFilterName itemSet:(const SfxItemSet *)pSet medium:(SfxMedium *)pMedium commit:(BOOL *)pCommit
{
	RunSFXFileCoordinator *pRet = [[RunSFXFileCoordinator alloc] initWithObjectShell:pObjShell fileName:pFileName filterName:pFilterName itemSet:pSet medium:pMedium commit:pCommit];
	[pRet autorelease];
	return pRet;
}

- (NSDocument *)documentForURL:(NSURL *)pURL
{
	NSDocument *pRet = nil;

	if ( pURL )
	{
		NSDocumentController *pDocController = [NSDocumentController sharedDocumentController];
		if ( pDocController )
			pRet = [pDocController documentForURL:pURL];
	}

	return pRet;
}

- (NSObject *)fileCoordinator
{
	NSObject *pRet = nil;

	NSBundle *pBundle = [NSBundle bundleForClass:[NSBundle class]];
	if ( pBundle )
	{
		Class aClass = [pBundle classNamed:@"NSFileCoordinator"];
		if ( aClass )
		{
			NSObject *pObject = [aClass alloc];
			if ( pObject && [pObject respondsToSelector:@selector(initWithFilePresenter:)] )
			{
				pRet = [pObject initWithFilePresenter:nil];
				if ( pRet )
					[pRet autorelease];
			}
		}
	}

	return pRet;
}

- (id)initWithObjectShell:(SfxObjectShell *)pObjShell fileName:(const String *)pFileName filterName:(const String *)pFilterName itemSet:(const SfxItemSet *)pSet medium:(SfxMedium *)pMedium commit:(BOOL *)pCommit
{
	[super init];

	mpObjShell = pObjShell;
	mpFileName = pFileName;
	mpFilterName = pFilterName;
	mpSet = pSet;
	mpMedium = pMedium;
	mpCommit = pCommit;
	mbResult = sal_False;

	return self;
}

- (void)objectShellDoSave:(id)pObject
{
	mbResult = sal_False;

	if ( !mpObjShell )
		return;

	__block sal_Bool bBlockResult = sal_False;

	IMutex& rSolarMutex = Application::GetSolarMutex();
	rSolarMutex.acquire();
	if ( !Application::IsShutDown() )
	{
		__block BOOL bBlockExecuted = NO;
		__block void (^aBlock)() = ^(void) {
			bBlockExecuted = YES;
			bBlockResult = mpObjShell->DoSave();
		};

		mpObjShell->GetMedium()->CheckForMovedFile( mpObjShell );
		NSURL *pURL = NSURLFromSfxMedium( mpObjShell->GetMedium() );
		if ( pURL )
		{
			NSDocument *pDoc = [self documentForURL:pURL];
			if ( pDoc && [pDoc respondsToSelector:@selector(performSynchronousFileAccessUsingBlock:)] )
				[pDoc performSynchronousFileAccessUsingBlock:aBlock];

			if ( !bBlockExecuted )
			{
				NSObject *pFileCoordinator = [self fileCoordinator];
				if ( pFileCoordinator && [pFileCoordinator respondsToSelector:@selector(coordinateWritingItemAtURL:options:error:byAccessor:)] )
				{
					NSError *pError = nil;
					[pFileCoordinator coordinateWritingItemAtURL:pURL options:NSFileCoordinatorWritingForReplacing error:&pError byAccessor:^(NSURL *pNewURL) {
						aBlock();
					}];
				}
			}
		}

		if ( !bBlockExecuted )
			aBlock();
	}
	rSolarMutex.release();

	mbResult = bBlockResult;
}

- (void)objectShellDoSaveAs:(id)pObject
{
	mbResult = sal_False;

	if ( !mpObjShell || !mpMedium )
		return;

	__block sal_Bool bBlockResult = sal_False;

	IMutex& rSolarMutex = Application::GetSolarMutex();
	rSolarMutex.acquire();
	if ( !Application::IsShutDown() )
	{
		__block BOOL bBlockExecuted = NO;
		__block void (^aBlock)() = ^(void) {
			bBlockExecuted = YES;
			bBlockResult = mpObjShell->DoSaveAs( *mpMedium );
		};

		mpMedium->CheckForMovedFile( mpObjShell );
		NSURL *pURL = NSURLFromSfxMedium( mpMedium );
		if ( pURL )
		{
			NSDocument *pDoc = [self documentForURL:pURL];
			if ( pDoc && [pDoc respondsToSelector:@selector(performSynchronousFileAccessUsingBlock:)] )
				[pDoc performSynchronousFileAccessUsingBlock:aBlock];

			if ( !bBlockExecuted )
			{
				NSObject *pFileCoordinator = [self fileCoordinator];
				if ( pFileCoordinator && [pFileCoordinator respondsToSelector:@selector(coordinateWritingItemAtURL:options:error:byAccessor:)] )
				{
					NSError *pError = nil;
					[pFileCoordinator coordinateWritingItemAtURL:pURL options:NSFileCoordinatorWritingForReplacing error:&pError byAccessor:^(NSURL *pNewURL) {
						aBlock();
					}];
				}
			}
		}

		if ( !bBlockExecuted )
			aBlock();
	}
	rSolarMutex.release();

	mbResult = bBlockResult;
}

- (void)objectShellDoSaveObjectAs:(id)pObject
{
	mbResult = sal_False;

	if ( !mpObjShell || !mpMedium || !mpCommit )
		return;

	__block sal_Bool bBlockResult = sal_False;

	IMutex& rSolarMutex = Application::GetSolarMutex();
	rSolarMutex.acquire();
	if ( !Application::IsShutDown() )
	{
		__block BOOL bBlockExecuted = NO;
		__block void (^aBlock)() = ^(void) {
			bBlockExecuted = YES;
			bBlockResult = mpObjShell->DoSaveObjectAs( *mpMedium, *mpCommit );
		};

		mpMedium->CheckForMovedFile( mpObjShell );
		NSURL *pURL = NSURLFromSfxMedium( mpMedium );
		if ( pURL )
		{
			NSDocument *pDoc = [self documentForURL:pURL];
			if ( pDoc && [pDoc respondsToSelector:@selector(performSynchronousFileAccessUsingBlock:)] )
				[pDoc performSynchronousFileAccessUsingBlock:aBlock];

			if ( !bBlockExecuted )
			{
				NSObject *pFileCoordinator = [self fileCoordinator];
				if ( pFileCoordinator && [pFileCoordinator respondsToSelector:@selector(coordinateWritingItemAtURL:options:error:byAccessor:)] )
				{
					NSError *pError = nil;
					[pFileCoordinator coordinateWritingItemAtURL:pURL options:NSFileCoordinatorWritingForReplacing error:&pError byAccessor:^(NSURL *pNewURL) {
						aBlock();
					}];
				}
			}
		}

		if ( !bBlockExecuted )
			aBlock();
	}
	rSolarMutex.release();

	mbResult = bBlockResult;
}

- (void)objectShellDoSave_Impl:(id)pObject
{
	mbResult = sal_False;

	if ( !mpObjShell )
		return;

	__block sal_Bool bBlockResult = sal_False;

	IMutex& rSolarMutex = Application::GetSolarMutex();
	rSolarMutex.acquire();
	if ( !Application::IsShutDown() )
	{
		__block BOOL bBlockExecuted = NO;
		__block void (^aBlock)() = ^(void) {
			bBlockExecuted = YES;
			bBlockResult = mpObjShell->DoSave_Impl( mpSet );
		};

		mpObjShell->GetMedium()->CheckForMovedFile( mpObjShell );
		NSURL *pURL = NSURLFromSfxMedium( mpObjShell->GetMedium() );
		if ( pURL )
		{
			NSDocument *pDoc = [self documentForURL:pURL];
			if ( pDoc && [pDoc respondsToSelector:@selector(performSynchronousFileAccessUsingBlock:)] )
				[pDoc performSynchronousFileAccessUsingBlock:aBlock];

			if ( !bBlockExecuted )
			{
				NSObject *pFileCoordinator = [self fileCoordinator];
				if ( pFileCoordinator && [pFileCoordinator respondsToSelector:@selector(coordinateWritingItemAtURL:options:error:byAccessor:)] )
				{
					NSError *pError = nil;
					[pFileCoordinator coordinateWritingItemAtURL:pURL options:NSFileCoordinatorWritingForReplacing error:&pError byAccessor:^(NSURL *pNewURL) {
						aBlock();
					}];
				}
			}
		}

		if ( !bBlockExecuted )
			aBlock();
	}
	rSolarMutex.release();

	mbResult = bBlockResult;
}

- (void)objectShellPreDoSaveAs_Impl:(id)pObject
{
	mbResult = sal_False;

	if ( !mpObjShell || !mpFileName || !mpFilterName )
		return;

	__block sal_Bool bBlockResult = sal_False;

	IMutex& rSolarMutex = Application::GetSolarMutex();
	rSolarMutex.acquire();
	if ( !Application::IsShutDown() )
	{
		__block BOOL bBlockExecuted = NO;
		__block void (^aBlock)() = ^(void) {
			bBlockExecuted = YES;
			bBlockResult = mpObjShell->PreDoSaveAs_Impl( *mpFileName, *mpFilterName, (SfxItemSet *)mpSet );
		};

		mpObjShell->GetMedium()->CheckForMovedFile( mpObjShell );
		NSURL *pURL = NSURLFromOUString( OUString( *mpFileName ) );
		if ( pURL )
		{
			NSDocument *pDoc = [self documentForURL:pURL];
			if ( pDoc && [pDoc respondsToSelector:@selector(performSynchronousFileAccessUsingBlock:)] )
				[pDoc performSynchronousFileAccessUsingBlock:aBlock];

			if ( !bBlockExecuted )
			{
				NSObject *pFileCoordinator = [self fileCoordinator];
				if ( pFileCoordinator && [pFileCoordinator respondsToSelector:@selector(coordinateWritingItemAtURL:options:error:byAccessor:)] )
				{
					NSError *pError = nil;
					[pFileCoordinator coordinateWritingItemAtURL:pURL options:NSFileCoordinatorWritingForReplacing error:&pError byAccessor:^(NSURL *pNewURL) {
						aBlock();
					}];
				}
			}
		}

		if ( !bBlockExecuted )
			aBlock();
	}
	rSolarMutex.release();

	mbResult = bBlockResult;
}

- (void)objectShellSave_Impl:(id)pObject
{
	mbResult = sal_False;

	if ( !mpObjShell )
		return;

	__block sal_Bool bBlockResult = sal_False;

	IMutex& rSolarMutex = Application::GetSolarMutex();
	rSolarMutex.acquire();
	if ( !Application::IsShutDown() )
	{
		__block BOOL bBlockExecuted = NO;
		__block void (^aBlock)() = ^(void) {
			bBlockExecuted = YES;
			bBlockResult = mpObjShell->Save_Impl( mpSet );
		};

		mpObjShell->GetMedium()->CheckForMovedFile( mpObjShell );
		NSURL *pURL = NSURLFromSfxMedium( mpObjShell->GetMedium() );
		if ( pURL )
		{
			NSDocument *pDoc = [self documentForURL:pURL];
			if ( pDoc && [pDoc respondsToSelector:@selector(performSynchronousFileAccessUsingBlock:)] )
				[pDoc performSynchronousFileAccessUsingBlock:aBlock];

			if ( !bBlockExecuted )
			{
				NSObject *pFileCoordinator = [self fileCoordinator];
				if ( pFileCoordinator && [pFileCoordinator respondsToSelector:@selector(coordinateWritingItemAtURL:options:error:byAccessor:)] )
				{
					NSError *pError = nil;
					[pFileCoordinator coordinateWritingItemAtURL:pURL options:NSFileCoordinatorWritingForReplacing error:&pError byAccessor:^(NSURL *pNewURL) {
						aBlock();
					}];
				}
			}
		}

		if ( !bBlockExecuted )
			aBlock();
	}
	rSolarMutex.release();

	mbResult = bBlockResult;
}

- (sal_Bool)result
{
	return mbResult;
}

@end

sal_Bool NSFileCoordinator_objectShellDoSave( SfxObjectShell *pObjShell )
{
	sal_Bool bRet = sal_False;

	if ( !pObjShell )
		return bRet;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	pObjShell->GetMedium()->CheckForMovedFile( pObjShell );
	NSURL *pURL = NSURLFromSfxMedium( pObjShell->GetMedium() );
	RunSFXFileCoordinator *pRunSFXFileCoordinator = [RunSFXFileCoordinator createWithObjectShell:pObjShell fileName:NULL filterName:NULL itemSet:NULL medium:NULL commit:NO];
	if ( pURL )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		ULONG nCount = Application::ReleaseSolarMutex();
		[pRunSFXFileCoordinator performSelectorOnMainThread:@selector(objectShellSave_Impl:) withObject:pRunSFXFileCoordinator waitUntilDone:YES modes:pModes];
		Application::AcquireSolarMutex( nCount );
	}
	else
	{
		[pRunSFXFileCoordinator objectShellSave_Impl:pRunSFXFileCoordinator];
	}
	bRet = [pRunSFXFileCoordinator result];

	[pPool release];

	return bRet;
}

sal_Bool NSFileCoordinator_objectShellDoSaveAs( SfxObjectShell *pObjShell, SfxMedium *pMedium )
{
	sal_Bool bRet = sal_False;

	if ( !pObjShell || !pMedium )
		return bRet;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	pMedium->CheckForMovedFile( pObjShell );
	NSURL *pURL = NSURLFromSfxMedium( pMedium );
	RunSFXFileCoordinator *pRunSFXFileCoordinator = [RunSFXFileCoordinator createWithObjectShell:pObjShell fileName:NULL filterName:NULL itemSet:NULL medium:pMedium commit:NO];
	if ( pURL )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		ULONG nCount = Application::ReleaseSolarMutex();
		[pRunSFXFileCoordinator performSelectorOnMainThread:@selector(objectShellDoSaveAs:) withObject:pRunSFXFileCoordinator waitUntilDone:YES modes:pModes];
		Application::AcquireSolarMutex( nCount );
	}
	else
	{
		[pRunSFXFileCoordinator objectShellDoSaveAs:pRunSFXFileCoordinator];
	}
	bRet = [pRunSFXFileCoordinator result];

	[pPool release];

	return bRet;
}

sal_Bool NSFileCoordinator_objectShellDoSaveObjectAs( SfxObjectShell *pObjShell, SfxMedium *pMedium, BOOL *pCommit )
{
	sal_Bool bRet = sal_False;

	if ( !pObjShell || !pMedium || !pCommit )
		return bRet;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	pMedium->CheckForMovedFile( pObjShell );
	NSURL *pURL = NSURLFromSfxMedium( pMedium );
	RunSFXFileCoordinator *pRunSFXFileCoordinator = [RunSFXFileCoordinator createWithObjectShell:pObjShell fileName:NULL filterName:NULL itemSet:NULL medium:pMedium commit:pCommit];
	if ( pURL )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		ULONG nCount = Application::ReleaseSolarMutex();
		[pRunSFXFileCoordinator performSelectorOnMainThread:@selector(objectShellDoSaveObjectAs:) withObject:pRunSFXFileCoordinator waitUntilDone:YES modes:pModes];
		Application::AcquireSolarMutex( nCount );
	}
	else
	{
		[pRunSFXFileCoordinator objectShellDoSaveObjectAs:pRunSFXFileCoordinator];
	}
	bRet = [pRunSFXFileCoordinator result];

	[pPool release];

	return bRet;
}

sal_Bool NSFileCoordinator_objectShellDoSave_Impl( SfxObjectShell *pObjShell, const SfxItemSet* pSet )
{
	sal_Bool bRet = sal_False;

	if ( !pObjShell )
		return bRet;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	pObjShell->GetMedium()->CheckForMovedFile( pObjShell );
	NSURL *pURL = NSURLFromSfxMedium( pObjShell->GetMedium() );
	RunSFXFileCoordinator *pRunSFXFileCoordinator = [RunSFXFileCoordinator createWithObjectShell:pObjShell fileName:NULL filterName:NULL itemSet:pSet medium:NULL commit:NO];
	if ( pURL )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		ULONG nCount = Application::ReleaseSolarMutex();
		[pRunSFXFileCoordinator performSelectorOnMainThread:@selector(objectShellDoSave_Impl:) withObject:pRunSFXFileCoordinator waitUntilDone:YES modes:pModes];
		Application::AcquireSolarMutex( nCount );
	}
	else
	{
		[pRunSFXFileCoordinator objectShellDoSave_Impl:pRunSFXFileCoordinator];
	}
	bRet = [pRunSFXFileCoordinator result];

	[pPool release];

	return bRet;
}

sal_Bool NSFileCoordinator_objectShellPreDoSaveAs_Impl( SfxObjectShell *pObjShell, const String *pFileName, const String *pFilterName, SfxItemSet *pSet )
{
	sal_Bool bRet = sal_False;

	if ( !pObjShell || !pFileName || !pFilterName )
		return bRet;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	pObjShell->GetMedium()->CheckForMovedFile( pObjShell );
	NSURL *pURL = NSURLFromOUString( OUString( *pFileName ) );
	RunSFXFileCoordinator *pRunSFXFileCoordinator = [RunSFXFileCoordinator createWithObjectShell:pObjShell fileName:pFileName filterName:pFilterName itemSet:pSet medium:NULL commit:NO];
	if ( pURL )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		ULONG nCount = Application::ReleaseSolarMutex();
		[pRunSFXFileCoordinator performSelectorOnMainThread:@selector(objectShellPreDoSaveAs_Impl:) withObject:pRunSFXFileCoordinator waitUntilDone:YES modes:pModes];
		Application::AcquireSolarMutex( nCount );
	}
	else
	{
		[pRunSFXFileCoordinator objectShellPreDoSaveAs_Impl:pRunSFXFileCoordinator];
	}
	bRet = [pRunSFXFileCoordinator result];

	[pPool release];

	return bRet;
}

sal_Bool NSFileCoordinator_objectShellSave_Impl( SfxObjectShell *pObjShell, const SfxItemSet *pSet )
{
	sal_Bool bRet = sal_False;

	if ( !pObjShell )
		return bRet;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	pObjShell->GetMedium()->CheckForMovedFile( pObjShell );
	NSURL *pURL = NSURLFromSfxMedium( pObjShell->GetMedium() );
	RunSFXFileCoordinator *pRunSFXFileCoordinator = [RunSFXFileCoordinator createWithObjectShell:pObjShell fileName:NULL filterName:NULL itemSet:pSet medium:NULL commit:NO];
	if ( pURL )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		ULONG nCount = Application::ReleaseSolarMutex();
		[pRunSFXFileCoordinator performSelectorOnMainThread:@selector(objectShellSave_Impl:) withObject:pRunSFXFileCoordinator waitUntilDone:YES modes:pModes];
		Application::AcquireSolarMutex( nCount );
	}
	else
	{
		[pRunSFXFileCoordinator objectShellSave_Impl:pRunSFXFileCoordinator];
	}
	bRet = [pRunSFXFileCoordinator result];

	[pPool release];

	return bRet;
}
