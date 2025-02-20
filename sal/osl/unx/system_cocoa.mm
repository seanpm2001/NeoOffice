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

#include "system.hxx"

static NSURL *macxp_resolveAliasImpl( NSURL *url )
{
	NSURL *pRet = nil;

	if ( url )
	{
		NSURL *pOriginalURL = url;

		NSData *pData = [NSURL bookmarkDataWithContentsOfURL:url error:nil];
		if ( pData )
		{
			NSURL *pURL = [NSURL URLByResolvingBookmarkData:pData options:NSURLBookmarkResolutionWithoutUI | NSURLBookmarkResolutionWithoutMounting relativeToURL:nil bookmarkDataIsStale:nil error:nil];
			if ( pURL )
			{
				pURL = [pURL URLByStandardizingPath];
				if ( pURL )
				{
					// Recurse if the URL is also an alias
					NSNumber *pAlias = nil;
					if ( ![pURL isEqual:pOriginalURL] && [pURL getResourceValue:&pAlias forKey:NSURLIsAliasFileKey error:nil] && pAlias && [pAlias boolValue] )
					{
						NSURL *pRecursedURL = macxp_resolveAliasImpl( pURL );
						if ( pRecursedURL )
							pURL = pRecursedURL;
					}

					pRet = pURL;
				}
			}
		}
	}

	return pRet;
}

int macxp_resolveAlias(char *path, unsigned int buflen, sal_Bool noResolveLastElement)
{
	int nRet = 0;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	NSString *pString = [NSString stringWithUTF8String:path];
	if ( pString )
	{
		NSURL *pURL = [NSURL fileURLWithPath:pString];
		if ( pURL )
		{
			pURL = [pURL URLByStandardizingPath];
			if ( pURL )
			{
				// Find lowest part of path that exists and see if it is an
				// alias
				NSURL *pTmpURL = pURL;
				while ( pTmpURL && ![pTmpURL checkResourceIsReachableAndReturnError:nil] )
				{
					NSURL *pOldTmpURL = pTmpURL;
					pTmpURL = [pTmpURL URLByDeletingLastPathComponent];
					if ( pTmpURL )
					{
						pTmpURL = [pTmpURL URLByStandardizingPath];
						if ( pTmpURL && [pTmpURL isEqual:pOldTmpURL] )
							pTmpURL = nil;
					}
				}

				if ( pTmpURL )
				{
					// We can skip checking if the URL is an alias if the
					// original URL exists and the noResolveLastElement
					// flag is true
					if ( noResolveLastElement && pTmpURL == pURL )
					{
						pURL = nil;
					}
					else
					{
						NSNumber *pAlias = nil;
						if ( [pTmpURL getResourceValue:&pAlias forKey:NSURLIsAliasFileKey error:nil] && pAlias && ![pAlias boolValue] )
							pURL = nil;
					}
            	}

				if ( pURL )
				{
					// Iterate through path and resolve any aliases
					NSArray *pPathComponents = [pURL pathComponents];
					if ( pPathComponents && [pPathComponents count] )
					{
						pURL = [NSURL fileURLWithPath:[pPathComponents objectAtIndex:0]];
						if ( pURL )
						{
							NSUInteger nCount = [pPathComponents count];
							NSUInteger i = 1;
							for ( ; i < nCount ; i++ )
							{
								pURL = [pURL URLByAppendingPathComponent:[pPathComponents objectAtIndex:i]];
								if ( !pURL || ( noResolveLastElement && i == nCount - 1 ) )
									break;

								NSNumber *pAlias = nil;
								if ( [pURL checkResourceIsReachableAndReturnError:nil] && [pURL getResourceValue:&pAlias forKey:NSURLIsAliasFileKey error:nil] && pAlias && [pAlias boolValue] )
								{
									NSURL *pResolvedURL = macxp_resolveAliasImpl( pURL );
									if ( pResolvedURL )
										pURL = pResolvedURL;
								}
							}

							if ( pURL )
							{
								NSString *pURLPath = [pURL path];
								if ( pURLPath )
								{
									const char *pURLPathString = [pURLPath UTF8String];
									if ( pURLPathString && strlen( pURLPathString ) < buflen )
									{
										strcpy( path, pURLPathString );
									}
									else
									{
										errno = ENAMETOOLONG;
										nRet = -1;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	[pPool release];

	return nRet;
}

sal_Bool macxp_getNSHomeDirectory(char *path, int buflen)
{
	sal_Bool bRet = sal_False;

	if ( path && buflen > 0 )
	{
		NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

		// NSHomeDirectory() returns the application's container directory
		// in sandboxed applications
		NSString *pHomeDir = NSHomeDirectory();
		if ( pHomeDir )
		{
			NSURL *pURL = [NSURL fileURLWithPath:pHomeDir];
			if ( pURL )
			{
				pURL = [pURL URLByStandardizingPath];
				if ( pURL )
				{
					NSString *pHomeDir = [pURL path];
					if ( pHomeDir )
					{
						const char *pHomeDirStr = [pHomeDir UTF8String];
						if ( pHomeDirStr )
						{
							int nLen = strlen( pHomeDirStr );
							if ( nLen < buflen )
							{
								strcpy( path, pHomeDirStr );
								bRet = sal_True;
							}
						}
					}
				}
			}
		}

		[pPool release];
	}

	return bRet;
}

void macxp_setFileType(const sal_Char* path)
{
#ifdef PRODUCT_FILETYPE
	if ( path && strlen( path ) )
	{
		NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

		NSString *pPath = [NSString stringWithUTF8String:path];
		NSFileManager *pFileManager = [NSFileManager defaultManager];
		if ( pPath && pFileManager )
		{
			NSDictionary *pAttributes = [pFileManager attributesOfItemAtPath:pPath error:nil];
			if ( !pAttributes || ![pAttributes fileHFSTypeCode] )
			{
				pAttributes = [NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedLong:(unsigned long)PRODUCT_FILETYPE] forKey:NSFileHFSTypeCode];
				if ( pAttributes )
					[pFileManager setAttributes:pAttributes ofItemAtPath:pPath error:nil];
			}
		}

		[pPool release];
	}
#endif	// PRODUCT_FILETYPE
}

sal_Bool macxp_isUbiquitousPath(sal_Unicode *path, sal_Int32 len)
{
	sal_Bool bRet = sal_False;

	if ( path && len > 0 )
	{
		NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

		NSFileManager *pFileManager = [NSFileManager defaultManager];
		if ( pFileManager )
		{
			NSString *pPath = [NSString stringWithCharacters:path length:len];
			if ( pPath && [pPath length] )
			{
				NSURL *pURL = [NSURL fileURLWithPath:pPath];
				if ( pURL && [pURL isFileURL] )
				{
					pURL = [pURL URLByStandardizingPath];
					if ( pURL )
					{
						pURL = [pURL URLByResolvingSymlinksInPath];
						if ( pURL )
						{
							// Don't call [NSFileManager isUbiquitousItemAtURL:]
							// as it will cause momentary hanging on OS X 10.9
							NSString *pURLPath = [pURL path];
							if ( pURLPath && [pURLPath length] )
							{
								NSArray *pLibraryFolders = NSSearchPathForDirectoriesInDomains( NSLibraryDirectory, NSUserDomainMask, NO );
								NSString *pRealHomeFolder = nil;
								struct passwd *pPasswd = getpwuid( getuid() );
								if ( pPasswd )
									pRealHomeFolder = [NSString stringWithUTF8String:pPasswd->pw_dir];
								if ( pLibraryFolders && pRealHomeFolder && [pRealHomeFolder length] )
								{
									NSUInteger nCount = [pLibraryFolders count];
									NSUInteger i = 0;
									for ( ; i < nCount; i++ )
									{
										NSString *pFolder = [pLibraryFolders objectAtIndex:i];
										if ( pFolder && [pFolder length] )
										{
											pFolder = [[pFolder stringByAppendingPathComponent:@"Mobile Documents"] stringByReplacingOccurrencesOfString:@"~" withString:pRealHomeFolder];
											if ( pFolder && [pFolder length] )
											{
												NSRange aRange = [pURLPath rangeOfString:pFolder];
												if ( !aRange.location && aRange.length )
													bRet = sal_True;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		[pPool release];
	}

	return bRet;
}

void osl_performSelectorOnMainThread( NSObject *pObj, SEL aSel, NSObject *pArg, sal_Bool bWait )
{
	if ( pObj && aSel )
		[pObj performSelectorOnMainThread:aSel withObject:pArg waitUntilDone:bWait modes:osl_getStandardRunLoopModes()];
}

NSArray<NSRunLoopMode> *osl_getStandardRunLoopModes()
{
	return [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, (NSRunLoopMode)JAVA_AWT_RUNLOOPMODE, nil];
}
