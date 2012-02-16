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
 *		 - GNU General Public License Version 2.1
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2012 by Planamesa Inc.
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
 *************************************************************************/

#import "updatei18n_cocoa.hxx"
#import <map>
#import <sfx2/app.hxx>
#import <tools/rcid.h>
#import <unotools/localedatawrapper.hxx>
#import <vcl/svapp.hxx>

static ::std::map< ::rtl::OUString, NSDictionary* > aLocalizationMap;
static NSDictionary *pDefaultLocaleDict = nil;
static NSDictionary *pPrimaryLocaleDict = nil;
static NSDictionary *pSecondaryLocaleDict = nil;
static NSDictionary *pTertiaryLocaleDict = nil;
static NSString *pDecimalSep = nil;
static ResMgr *pUPDResMgr = NULL;
static ResMgr *pVCLResMgr = NULL;

using namespace com::sun::star::lang;
using namespace rtl;

/**
 * Translated strings for de locale
 */
static const sal_Char *pEntries_de[] = {
	UPDATEBACK, "Zurück",
	nil, nil
};

/**
 * Translated strings for en_US locale
 */
static const sal_Char *pEntries_en_US[] = {
	UPDATEBACK, "Back",
	UPDATEDOWNLOADCANCELED, "Download canceled",
	UPDATEDOWNLOADFAILED, "Download failed",
	UPDATEDOWNLOADINGFILE, "Downloading file",
	UPDATEERROR, "Error:",
	UPDATEINSTALLUPDATES, "Do you want to install the updates that you downloaded now?",
	UPDATELOADING, "Loading…",
	UPDATEMEGABYTE, "MB",
	UPDATEOPENFILEFAILED, "Cannot open %@",
	UPDATEOPENINGFILE, "Opening %@…",
	UPDATEREDOWNLOADFILE, "Do you want to redownload the file?",
	nil, nil
};

/**
 * Translated strings for es locale
 */
static const sal_Char *pEntries_es[] = {
	UPDATEDOWNLOADCANCELED, "Descarga cancelada",
	UPDATEDOWNLOADFAILED, "Falló la descarga",
	UPDATEDOWNLOADINGFILE, "Descargando archivo",
	UPDATEERROR, "Error:",
	UPDATELOADING, "Cargando…",
	nil, nil
};

/**
 * Translated strings for fr locale
 */
static const sal_Char *pEntries_fr[] = {
	UPDATEBACK, "Arrière",
	UPDATEDOWNLOADCANCELED, "Téléchargement annulé",
	UPDATEDOWNLOADFAILED, "Echec du téléchargement",
	UPDATEDOWNLOADINGFILE, "Téléchargement du fichier",
	UPDATEERROR, "Erreur :",
	UPDATELOADING, "Chargement…",
	UPDATEMEGABYTE, "Mo",
	nil, nil
};

/**
 * Translated strings for it locale
 */
static const sal_Char *pEntries_it[] = {
	UPDATEDOWNLOADCANCELED, "Trasferimento cancellato ",
	UPDATEDOWNLOADFAILED, "Trasferimento fallito",
	UPDATEDOWNLOADINGFILE, "Trasferimento del file",
	UPDATEERROR, "Errore:",
	UPDATELOADING, "Caricamento…",
	nil, nil
};

/**
 * Translated strings for nl locale
 */
static const sal_Char *pEntries_nl[] = {
	UPDATEDOWNLOADCANCELED, "Ophalen geannuleerd",
	UPDATEDOWNLOADFAILED, "Ophalen mislukt",
	UPDATEDOWNLOADINGFILE, "Bestand ophalen",
	UPDATEERROR, "Fout:",
	UPDATEINSTALLUPDATES, "Wilt u de updates die u heeft opgehaald nu installeren?",
	UPDATELOADING, "Laden…",
	UPDATEOPENFILEFAILED, "Kan %@ niet openen",
	UPDATEOPENINGFILE, "%@ openen…",
	UPDATEREDOWNLOADFILE, "Wilt u het bestand opnieuw ophalen?",
	nil, nil
};

/**
 * Translated strings for pt locale
 */
static const sal_Char *pEntries_pt[] = {
	UPDATEDOWNLOADCANCELED, "Transferência de arquivo cancelada",
	UPDATEDOWNLOADFAILED, "Falha na transferência de arquivo",
	UPDATEDOWNLOADINGFILE, "Transferindo arquivo",
	UPDATEERROR, "Erro:",
	UPDATELOADING, "Carregando…",
	nil, nil
};

static OUString aDelimiter = OUString::createFromAscii( "_" );

static OUString ImplGetLocaleString( Locale aLocale )
{
	OUString aLocaleString( aLocale.Language );
	if ( aLocaleString.getLength() && aLocale.Country.getLength() )
	{
		aLocaleString += aDelimiter;
		aLocaleString += aLocale.Country;
		if ( aLocale.Variant.getLength() )
		{
			aLocaleString += aDelimiter;
			aLocaleString += aLocale.Variant;
		}
	}
	return aLocaleString;
}

static void InitializeLocale( OUString aLocale, const sal_Char **pEntries )
{
	if ( !aLocale.getLength() || !pEntries )
		return;

	::std::map< OUString, NSDictionary* >::const_iterator it = aLocalizationMap.find( aLocale );
	if ( it != aLocalizationMap.end() )
		return;

	NSMutableDictionary *pDict = [NSMutableDictionary dictionaryWithCapacity:20];
	if ( !pDict )
		return;

	// Ensure that the dictionary never gets released
	[pDict retain];

	// Iterate through entries until a nil entry is found
	for ( size_t i = 0; pEntries[ i ] && pEntries[ i + 1 ]; i += 2 )
		[pDict setObject:[NSString stringWithUTF8String:pEntries[ i + 1 ]] forKey:[NSString stringWithUTF8String:pEntries[ i ]]];

	aLocalizationMap[ aLocale ] = pDict;
}

/**
 * Lookup a string and retrieve a translated string.  If no translation
 * is available, default to "en-US".
 */
NSString *UpdateGetLocalizedString( const sal_Char *key )
{
	if ( !key || !strlen( key ) )
		return @"";

	NSString *pKey = [NSString stringWithUTF8String:key];
	if ( !pKey )
		return @"";

	NSString *pRet = nil;

	if ( !aLocalizationMap.size() )
	{
		// Initialize dictionaries
		InitializeLocale( ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "de" ) ), OUString(), OUString() ) ), pEntries_de );
		InitializeLocale( ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "en" ) ), OUString( RTL_CONSTASCII_USTRINGPARAM( "US" ) ), OUString() ) ), pEntries_en_US );
		InitializeLocale( ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "es" ) ), OUString(), OUString() ) ), pEntries_es );
		InitializeLocale( ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "fr" ) ), OUString(), OUString() ) ), pEntries_fr );
		InitializeLocale( ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "it" ) ), OUString(), OUString() ) ), pEntries_it );
		InitializeLocale( ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "nl" ) ), OUString(), OUString() ) ), pEntries_nl );
		InitializeLocale( ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "pt" ) ), OUString(), OUString() ) ), pEntries_pt );

		// Set locale dictionaries based on default locale
		Locale aLocale( Application::GetSettings().GetUILocale() );

		// Check if locale exists in our list of locales. Note that we ignore
		// variant at this time as no variant-specific localizations are
		// planned for this component yet.
		OUString aDefaultLocale = ImplGetLocaleString( Locale( OUString( RTL_CONSTASCII_USTRINGPARAM( "en" ) ), OUString( RTL_CONSTASCII_USTRINGPARAM( "US" ) ), OUString() ) );
		::std::map< OUString, NSDictionary* >::const_iterator it = aLocalizationMap.find( aDefaultLocale );
		if ( it != aLocalizationMap.end() )
			pDefaultLocaleDict = it->second;

		OUString aPrimaryLocale = ImplGetLocaleString( aLocale );
		it = aLocalizationMap.find( aPrimaryLocale );
		if ( it != aLocalizationMap.end() && pDefaultLocaleDict != it->second )
			pPrimaryLocaleDict = it->second;

		OUString aSecondaryLocale = ImplGetLocaleString( Locale( aLocale.Language, aLocale.Country, OUString() ) );
		it = aLocalizationMap.find( aSecondaryLocale );
		if ( it != aLocalizationMap.end() && pDefaultLocaleDict != it->second && pPrimaryLocaleDict != it->second )
			pSecondaryLocaleDict = it->second;

		OUString aTertiaryLocale = ImplGetLocaleString( Locale( aLocale.Language, OUString(), OUString() ) );
		it = aLocalizationMap.find( aTertiaryLocale );
		if ( it != aLocalizationMap.end() && pDefaultLocaleDict != it->second && pPrimaryLocaleDict != it->second && pSecondaryLocaleDict != it->second )
			pTertiaryLocaleDict = it->second;
	}

	if ( !pRet && pPrimaryLocaleDict )
		pRet = (NSString *)[pPrimaryLocaleDict objectForKey:pKey];

	if ( !pRet && pSecondaryLocaleDict )
		pRet = (NSString *)[pSecondaryLocaleDict objectForKey:pKey];

	if ( !pRet && pTertiaryLocaleDict )
		pRet = (NSString *)[pTertiaryLocaleDict objectForKey:pKey];

	if ( !pRet && pDefaultLocaleDict )
		pRet = (NSString *)[pDefaultLocaleDict objectForKey:pKey];

	if ( !pRet )
		pRet = pKey;

	return pRet;
}

NSString *UpdateGetLocalizedDecimalSeparator()
{
	if ( !pDecimalSep )
	{
		OUString aDecimalSep( Application::GetAppLocaleDataWrapper().getNumDecimalSep() );
		if ( !aDecimalSep.getLength() )
			aDecimalSep = OUString( RTL_CONSTASCII_USTRINGPARAM( "." ) );
		pDecimalSep = [NSString stringWithCharacters:aDecimalSep.getStr() length:aDecimalSep.getLength()];
		if ( pDecimalSep )
			[pDecimalSep retain];
	}

	return pDecimalSep;
}

NSString *UpdateGetUPDResString( int nId )
{
	if ( !pUPDResMgr )
	{
		pUPDResMgr = SfxApplication::CreateResManager( "upd" );
		if ( !pUPDResMgr )
			return @"";
	}

	ResId aResId( nId, *pUPDResMgr );
	aResId.SetRT( RSC_STRING );
	if ( !pUPDResMgr->IsAvailable( aResId ) )
		return @"";
 
	XubString aResString( ResId( nId, *pUPDResMgr ) );
	aResString.EraseAllChars('~');
	return [NSString stringWithCharacters:aResString.GetBuffer() length:aResString.Len()];
}

NSString *UpdateGetVCLResString( int nId )
{
	if ( !pVCLResMgr )
	{
		pVCLResMgr = SfxApplication::CreateResManager( "vcl" );
		if ( !pVCLResMgr )
			return @"";
	}

	ResId aResId( nId, *pVCLResMgr );
	aResId.SetRT( RSC_STRING );
	if ( !pVCLResMgr->IsAvailable( aResId ) )
		return @"";
 
	XubString aResString( ResId( nId, *pVCLResMgr ) );
	aResString.EraseAllChars('~');
	return [NSString stringWithCharacters:aResString.GetBuffer() length:aResString.Len()];
}
