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
 *  Sun Microsystems Inc., October, 2000
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2000 by Sun Microsystems, Inc.
 *  901 San Antonio Road, Palo Alto, CA 94303, USA
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
 *  =================================================
 *  Modified June 2004 by Patrick Luby. SISSL Removed. NeoOffice is
 *  distributed under GPL only under modification term 3 of the LGPL.
 *
 *  Contributor(s): _______________________________________
 *
 ************************************************************************/

#include <sal/types.h>

#include <premac.h>
#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <postmac.h>

/*
 * macxp_getOSXLocale
 *
 * Grab current locale from system.
 */
int macxp_getOSXLocale( char *locale, sal_uInt32 bufferLen )
{
	CFBundleRef rBundle;

	locale[0] = '\0'; 
	rBundle = CFBundleGetMainBundle();
	if ( rBundle )
	{
		CFArrayRef rAvailableLocales = CFBundleCopyBundleLocalizations( rBundle );
		if ( rAvailableLocales )
		{
			CFArrayRef rPreferredLocales = CFBundleCopyPreferredLocalizationsFromArray( rAvailableLocales );
			if ( rPreferredLocales )
			{
				CFStringRef rString = (CFStringRef)CFArrayGetValueAtIndex( rPreferredLocales, 0 );
				if ( rString )
				{
					if ( !CFStringGetCString( rString, locale, bufferLen, kCFStringEncodingUTF8 ) )
						locale[0] = '\0';
					
				}
				CFRelease( rPreferredLocales );
			}
			CFRelease( rAvailableLocales );
		}
	}

	if ( !strlen( locale ) )
		strcpy( locale, "en" );

	return( noErr );
}


/*
 * macxp_OSXConvertCFEncodingToIANACharSetName
 *
 * Convert a CoreFoundation text encoding to an IANA charset name.
 */
int macxp_OSXConvertCFEncodingToIANACharSetName( char *buffer, unsigned int bufferLen, CFStringEncoding cfEncoding )
{
	CFStringRef	sCFEncodingName;

	sCFEncodingName = CFStringConvertEncodingToIANACharSetName( cfEncoding );
	CFStringGetCString( sCFEncodingName, buffer, bufferLen, cfEncoding );

	return( noErr );
}
