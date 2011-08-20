/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified July 2007 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_lingucomponent.hxx"
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/linguistic2/XSearchableDictionaryList.hpp>

#include <com/sun/star/linguistic2/SpellFailure.hpp>
#include <cppuhelper/factory.hxx>	// helper for factories
#include <com/sun/star/registry/XRegistryKey.hpp>
#include <tools/debug.hxx>
#include <unotools/processfactory.hxx>
#include <osl/mutex.hxx>

#include <hunspell.hxx>
#include <dictmgr.hxx>

#ifndef _SPELLIMP_HXX
#include <sspellimp.hxx>
#endif

#include <linguistic/lngprops.hxx>
#include <linguistic/spelldta.hxx>
#include <i18npool/mslangid.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/lingucfg.hxx>
#include <svtools/useroptions.hxx>
#include <osl/file.hxx>
#include <rtl/ustrbuf.hxx>

#include <lingutil.hxx>

#include <list>
#include <set>

#if defined USE_JAVA && defined MACOSX
#include <unistd.h>
#endif	// USE_JAVA && MACOSX

using namespace utl;
using namespace osl;
using namespace rtl;
using namespace com::sun::star;
using namespace com::sun::star::beans;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace com::sun::star::linguistic2;
using namespace linguistic;

// XML-header of SPELLML queries
#define SPELLML_HEADER "<?xml?>"

#if defined USE_JAVA && defined MACOSX

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

#endif // USE_JAVA && MACOSX

///////////////////////////////////////////////////////////////////////////

SpellChecker::SpellChecker() :
	aEvtListeners	( GetLinguMutex() )
{
        aDicts = NULL;
	aDEncs = NULL;
	aDLocs = NULL;
	aDNames = NULL;
	bDisposing = FALSE;
	pPropHelper = NULL;
        numdict = 0;
#if defined USE_JAVA && defined MACOSX
	maLocales = NULL;
#endif	// USE_JAVA && MACOSX
}


SpellChecker::~SpellChecker()
{
#if defined USE_JAVA && defined MACOSX
	if ( maLocales )
		CFRelease( maLocales );
#endif	// USE_JAVA && MACOSX

  if (aDicts) {
     for (int i = 0; i < numdict; i++) { 
            if (aDicts[i]) delete aDicts[i];
            aDicts[i] = NULL;
     }
     delete[] aDicts;
  }
  aDicts = NULL;
  numdict = 0;	
  if (aDEncs) delete[] aDEncs;
  aDEncs = NULL;
  if (aDLocs) delete[] aDLocs;
  aDLocs = NULL;
  if (aDNames) delete[] aDNames;
  aDNames = NULL;
  if (pPropHelper)
	 pPropHelper->RemoveAsPropListener();
}


PropertyHelper_Spell & SpellChecker::GetPropHelper_Impl()
{
	if (!pPropHelper)
	{
		Reference< XPropertySet	>	xPropSet( GetLinguProperties(), UNO_QUERY );

		pPropHelper	= new PropertyHelper_Spell( (XSpellChecker *) this, xPropSet );
		xPropHelper = pPropHelper;
		pPropHelper->AddAsPropListener();	//! after a reference is established
	}
	return *pPropHelper;
}


Sequence< Locale > SAL_CALL SpellChecker::getLocales()
		throw(RuntimeException)
{
    MutexGuard  aGuard( GetLinguMutex() );

    // this routine should return the locales supported by the installed
    // dictionaries.

#if defined USE_JAVA && defined MACOSX
    if (!numdict && !maLocales)
#else	// USE_JAVA && MACOSX
    if (!numdict) 
#endif	// USE_JAVA && MACOSX
    {
        SvtLinguConfig aLinguCfg;

        // get list of extension dictionaries-to-use
		// (or better speaking: the list of dictionaries using the
		// new configuration entries).
        std::list< SvtLinguConfigDictionaryEntry > aDics;
        uno::Sequence< rtl::OUString > aFormatList;
        aLinguCfg.GetSupportedDictionaryFormatsFor( A2OU("SpellCheckers"), 
                A2OU("org.openoffice.lingu.MySpellSpellChecker"), aFormatList );
        sal_Int32 nLen = aFormatList.getLength();
        for (sal_Int32 i = 0;  i < nLen;  ++i)
        {
            std::vector< SvtLinguConfigDictionaryEntry > aTmpDic( 
                    aLinguCfg.GetActiveDictionariesByFormat( aFormatList[i] ) );
            aDics.insert( aDics.end(), aTmpDic.begin(), aTmpDic.end() );
        }

        //!! for compatibility with old dictionaries (the ones not using extensions 
        //!! or new configuration entries, but still using the dictionary.lst file)
		//!! Get the list of old style spell checking dictionaries to use...
        std::vector< SvtLinguConfigDictionaryEntry > aOldStyleDics( 
				GetOldStyleDics( "DICT" ) );

		// to prefer dictionaries with configuration entries we will only
		// use those old style dictionaries that add a language that
		// is not yet supported by the list od new style dictionaries
		MergeNewStyleDicsAndOldStyleDics( aDics, aOldStyleDics );

        numdict = aDics.size();
        if (numdict) 
        {
            // get supported locales from the dictionaries-to-use...
            sal_Int32 k = 0;
            std::set< rtl::OUString, lt_rtl_OUString > aLocaleNamesSet;
            std::list< SvtLinguConfigDictionaryEntry >::const_iterator aDictIt;
            for (aDictIt = aDics.begin();  aDictIt != aDics.end();  ++aDictIt)
            {
                uno::Sequence< rtl::OUString > aLocaleNames( aDictIt->aLocaleNames );
                sal_Int32 nLen2 = aLocaleNames.getLength();
                for (k = 0;  k < nLen2;  ++k)
                {
                    aLocaleNamesSet.insert( aLocaleNames[k] );
                }
            }    
            // ... and add them to the resulting sequence
            aSuppLocales.realloc( aLocaleNamesSet.size() );
            std::set< rtl::OUString, lt_rtl_OUString >::const_iterator aItB;
            k = 0;
            for (aItB = aLocaleNamesSet.begin();  aItB != aLocaleNamesSet.end();  ++aItB)
            {
				Locale aTmp( MsLangId::convertLanguageToLocale(
						MsLangId::convertIsoStringToLanguage( *aItB )));
                aSuppLocales[k++] = aTmp;
            }    
            
            //! For each dictionary and each locale we need a seperate entry.
            //! If this results in more than one dictionary per locale than (for now)
			//! it is undefined which dictionary gets used.
			//! In the future the implementation should support using several dictionaries
			//! for one locale. 
			numdict = 0;
            for (aDictIt = aDics.begin();  aDictIt != aDics.end();  ++aDictIt)
				numdict = numdict + aDictIt->aLocaleNames.getLength();

            // add dictionary information
            aDicts  = new Hunspell* [numdict];
            aDEncs  = new rtl_TextEncoding [numdict];
            aDLocs  = new Locale [numdict];
            aDNames = new OUString [numdict];
            k = 0;
            for (aDictIt = aDics.begin();  aDictIt != aDics.end();  ++aDictIt)
            {
                if (aDictIt->aLocaleNames.getLength() > 0 && 
                    aDictIt->aLocations.getLength() > 0)
                {
                    uno::Sequence< rtl::OUString > aLocaleNames( aDictIt->aLocaleNames );
                    sal_Int32 nLocales = aLocaleNames.getLength();

                    // currently only one language per dictionary is supported in the actual implementation...
                    // Thus here we work-around this by adding the same dictionary several times.
                    // Once for each of it's supported locales.
                    for (sal_Int32 i = 0;  i < nLocales;  ++i)
                    {
                        aDicts[k]  = NULL;
                        aDEncs[k]  = 0;
                        aDLocs[k]  = MsLangId::convertLanguageToLocale(
                                        MsLangId::convertIsoStringToLanguage( aLocaleNames[i] ));
                        // also both files have to be in the same directory and the
                        // file names must only differ in the extension (.aff/.dic).
                        // Thus we use the first location only and strip the extension part.
                        rtl::OUString aLocation = aDictIt->aLocations[0];
                        sal_Int32 nPos = aLocation.lastIndexOf( '.' );
                        aLocation = aLocation.copy( 0, nPos );
                        aDNames[k] = aLocation;
                        
                        ++k;
                    }
                }
            }
            DBG_ASSERT( k == numdict, "index mismatch?" );
        } 
        else 
        {
            /* no dictionary found so register no dictionaries */
            numdict = 0;
            aDicts  = NULL;
            aDEncs  = NULL;
            aDLocs  = NULL;
            aDNames = NULL;
            aSuppLocales.realloc(0);
        }

#if defined USE_JAVA && defined MACOSX
		::std::list< Locale > aAppLocalesList;
		CFMutableArrayRef aAppLocales = CFArrayCreateMutable( kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks );
		if ( aAppLocales )
		{
			for ( USHORT nLang = 0x0; nLang < 0xffff; nLang++ )
			{
				Locale aLocale;
				LanguageToLocale( aLocale, nLang );
				OUString aLocaleString( ImplGetLocaleString( aLocale ) );
				if ( aLocaleString.getLength() )
				{
					CFStringRef aString = CFStringCreateWithCharactersNoCopy( NULL, aLocaleString.getStr(), aLocaleString.getLength(), kCFAllocatorNull );
					if ( aString )
					{
						CFArrayAppendValue( aAppLocales, aString );
						aAppLocalesList.push_back( aLocale );
						CFRelease( aString );
					}

					if ( aLocale.Variant.getLength() )
					{
						Locale aTmpLocale( aLocale.Language, aLocale.Country, OUString() );
						OUString aTmpLocaleString( ImplGetLocaleString( aTmpLocale ) );
						if ( aTmpLocaleString.getLength() )
						{
							CFStringRef aTmpString = CFStringCreateWithCharactersNoCopy( NULL, aTmpLocaleString.getStr(), aTmpLocaleString.getLength(), kCFAllocatorNull );
							if ( aTmpString )
							{
								CFArrayAppendValue( aAppLocales, aTmpString );
								CFRelease( aTmpString );
							}
						}
					}

					if ( aLocale.Country.getLength() )
					{
						Locale aTmpLocale( aLocale.Language, OUString(), OUString() );
						OUString aTmpLocaleString( ImplGetLocaleString( aTmpLocale ) );
						if ( aTmpLocaleString.getLength() )
						{
							CFStringRef aTmpString = CFStringCreateWithCharactersNoCopy( NULL, aTmpLocaleString.getStr(), aTmpLocaleString.getLength(), kCFAllocatorNull );
							if ( aTmpString )
							{
								CFArrayAppendValue( aAppLocales, aTmpString );
								CFRelease( aTmpString );
							}
						}
					}
				}
			}
		}

		maLocales = NSSpellChecker_getLocales( aAppLocales );
		if ( maLocales )
		{
			int nStart = aSuppLocales.getLength();
			CFIndex nItems = CFArrayGetCount( maLocales );
			aSuppLocales.realloc( nStart + nItems );
			Locale *pLocaleArray = aSuppLocales.getArray();

			CFIndex nItemsAdded = nStart;
			for ( CFIndex i = 0; i < nItems; i++ )
			{
				CFStringRef aString = (CFStringRef)CFArrayGetValueAtIndex( maLocales, i );
				if ( aString )
				{
					CFIndex nLen = CFStringGetLength( aString );
					CFRange aRange = CFRangeMake( 0, nLen );
					sal_Unicode pBuffer[ nLen + 1 ];
					CFStringGetCharacters( aString, aRange, pBuffer );
					pBuffer[ nLen ] = 0;
					OUString aItem( pBuffer );

					if ( !aItem.getLength() )
						continue;

					sal_Unicode cDelimiter = (sal_Unicode)'_';
					sal_Int32 nIndex = 0;
					OUString aLang = aItem.getToken( 0, cDelimiter, nIndex );
					if ( nIndex < 0 )
					{
						// Mac OS X will sometimes use "-" as its delimiter
						cDelimiter = (sal_Unicode)'-';
						nIndex = 0;
						aLang = aItem.getToken( 0, cDelimiter, nIndex );
					}

					OUString aCountry;
					if ( nIndex >= 0 )
						aCountry = aItem.getToken( 0, cDelimiter, nIndex );

					OUString aVariant;
					if ( nIndex >= 0 )
						aVariant = aItem.getToken( 0, cDelimiter, nIndex );
	
					// TODO: Handle the cases where the country is "Hans" or
					// "Hant". Since these are only used with the "zh" locale
					// and Chinese is not likely to have spellchecking support
					// anytime soon as it is an ideographic language, we may
					// never need to worry about this case.
					Locale aLocale( aLang, aCountry, aVariant );
					OUString aLocaleString( ImplGetLocaleString( aLocale ) );
					if ( !aLocaleString.getLength() )
						continue;

					::std::map< OUString, CFStringRef >::const_iterator it = maPrimaryNativeLocaleMap.find( aLocaleString );
					if ( it == maPrimaryNativeLocaleMap.end() )
					{
						maPrimaryNativeLocaleMap[ aLocaleString ] = aString;

						// Fix bug 2532 by checking for approximate and
						// duplicate matches in the native locales
						bool bAddLocale = true;
						for ( int j = 0; j < nItemsAdded; j++ )
						{
							if ( aLocale == pLocaleArray[ j ] )
							{
								bAddLocale = false;
								break;
							}

							if ( pLocaleArray[ j ].Variant.getLength() )
							{
								Locale aTmpLocale( pLocaleArray[ j ].Language, pLocaleArray[ j ].Country, OUString() );
								if ( aLocale == aTmpLocale )
								{
									OUString aTmpLocaleString( ImplGetLocaleString( aTmpLocale ) );
									if ( aTmpLocaleString.getLength() )
										maSecondaryNativeLocaleMap[ aTmpLocaleString ] = aString;
								}
							}

							if ( pLocaleArray[ j ].Country.getLength() )
							{
								Locale aTmpLocale( pLocaleArray[ j ].Language, OUString(), OUString() );
								if ( aLocale == aTmpLocale )
								{
									OUString aTmpLocaleString( ImplGetLocaleString( aTmpLocale ) );
									if ( aTmpLocaleString.getLength() )
										maSecondaryNativeLocaleMap[ aTmpLocaleString ] = aString;
								}
							}
						}

						if ( bAddLocale )
							pLocaleArray[ nItemsAdded++ ] = aLocale;
					}
				}
			}

			aSuppLocales.realloc( nItemsAdded );
		}

		// Fix 2686 by finding partial matches to application locales
		int nStart = aSuppLocales.getLength();
		aSuppLocales.realloc( nStart + aAppLocalesList.size() );
		CFIndex nItemsAdded = nStart;
		Locale *pLocaleArray = aSuppLocales.getArray();
		for ( ::std::list< Locale >::const_iterator it = aAppLocalesList.begin(); it != aAppLocalesList.end(); ++it )
		{
			OUString aLocaleString( ImplGetLocaleString( *it ) );
			if ( !aLocaleString.getLength() )
				continue;

			bool bAddLocale = false;
			for ( int j = 0; j < nItemsAdded; j++ )
			{
				if ( pLocaleArray[ j ] == *it )
					break;

				if ( (*it).Variant.getLength() )
				{
					Locale aTmpLocale( (*it).Language, (*it).Variant, OUString() );
					if ( pLocaleArray[ j ] == aTmpLocale )
					{
						bool bFound = false;
						OUString aTmpLocaleString( ImplGetLocaleString( aTmpLocale ) );
						::std::map< OUString, CFStringRef >::const_iterator nlit = maPrimaryNativeLocaleMap.find( aTmpLocaleString );
						if ( nlit != maPrimaryNativeLocaleMap.end() )
						{
							bFound = true;
						}
						else
						{
							nlit = maSecondaryNativeLocaleMap.find( aTmpLocaleString );
							if ( nlit != maSecondaryNativeLocaleMap.end() )
								bFound = true;
						}

						if ( bFound )
						{
							maSecondaryNativeLocaleMap[ aLocaleString ] = nlit->second;
							bAddLocale = true;
							break;
						}
					}
				}

				if ( (*it).Country.getLength() )
				{
					Locale aTmpLocale( (*it).Language, OUString(), OUString() );
					if ( pLocaleArray[ j ] == aTmpLocale )
					{
						bool bFound = false;
						OUString aTmpLocaleString( ImplGetLocaleString( aTmpLocale ) );
						::std::map< OUString, CFStringRef >::const_iterator nlit = maPrimaryNativeLocaleMap.find( aTmpLocaleString );
						if ( nlit != maPrimaryNativeLocaleMap.end() )
						{
							bFound = true;
						}
						else
						{
							nlit = maSecondaryNativeLocaleMap.find( aTmpLocaleString );
							if ( nlit != maSecondaryNativeLocaleMap.end() )
								bFound = true;
						}

						if ( bFound )
						{
							maSecondaryNativeLocaleMap[ aLocaleString ] = nlit->second;
							bAddLocale = true;
							break;
						}
					}
				}
			}

			if ( bAddLocale )
				pLocaleArray[ nItemsAdded++ ] = *it;
		}

		aSuppLocales.realloc( nItemsAdded );

		if ( aAppLocales )
			CFRelease( aAppLocales );
#endif	// USE_JAVA && MACOSX
    }    

	return aSuppLocales;
}


sal_Bool SAL_CALL SpellChecker::hasLocale(const Locale& rLocale)
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	BOOL bRes = FALSE;
	if (!aSuppLocales.getLength())
		getLocales();

	INT32 nLen = aSuppLocales.getLength();
#if defined USE_JAVA && defined MACOSX
	// Check native locales first 
	OUString aLocaleString( ImplGetLocaleString( rLocale ) );
	if ( aLocaleString.getLength() )
	{
		::std::map< OUString, CFStringRef >::const_iterator it = maPrimaryNativeLocaleMap.find( aLocaleString );
		if ( it != maPrimaryNativeLocaleMap.end() )
			return TRUE;

		it = maSecondaryNativeLocaleMap.find( aLocaleString );
		if ( it != maSecondaryNativeLocaleMap.end() )
			return TRUE;

		// Fix bug 2513 by checking for approximate matches in the native locales
		if ( rLocale.Variant.getLength() )
		{
			bool bFound = false;
			Locale aTmpLocale( rLocale.Language, rLocale.Country, OUString() );
			OUString aTmpLocaleString = ImplGetLocaleString( aTmpLocale );
			it = maPrimaryNativeLocaleMap.find( aTmpLocaleString );
			if ( it != maPrimaryNativeLocaleMap.end() )
			{
				bFound = true;
			}
			else
			{
				it = maSecondaryNativeLocaleMap.find( aTmpLocaleString );
				if ( it != maSecondaryNativeLocaleMap.end() )
					bFound = true;
			}

			if ( bFound )
			{
				aSuppLocales.realloc( ++nLen );
				Locale *pLocaleArray = aSuppLocales.getArray();
				pLocaleArray[ nLen - 1 ] = rLocale;
				maSecondaryNativeLocaleMap[ aLocaleString ] = it->second;
				return TRUE;
			}
		}

		if ( rLocale.Country.getLength() )
		{
			bool bFound = false;
			Locale aTmpLocale( rLocale.Language, OUString(), OUString() );
			OUString aTmpLocaleString = ImplGetLocaleString( aTmpLocale );
			it = maPrimaryNativeLocaleMap.find( aTmpLocaleString );
			if ( it != maPrimaryNativeLocaleMap.end() )
			{
				bFound = true;
			}
			else
			{
				it = maSecondaryNativeLocaleMap.find( aTmpLocaleString );
				if ( it != maSecondaryNativeLocaleMap.end() )
					bFound = true;
			}

			if ( bFound )
			{
				aSuppLocales.realloc( ++nLen );
				Locale *pLocaleArray = aSuppLocales.getArray();
				pLocaleArray[ nLen - 1 ] = rLocale;
				maSecondaryNativeLocaleMap[ aLocaleString ] = it->second;
				return TRUE;
			}
		}
	}
#endif	// USE_JAVA && MACOSX

	for (INT32 i = 0;  i < nLen;  ++i)
	{
		const Locale *pLocale = aSuppLocales.getConstArray();
		if (rLocale == pLocale[i])
		{
			bRes = TRUE;
			break;
		}
	}
	return bRes;
}

INT16 SpellChecker::GetSpellFailure( const OUString &rWord, const Locale &rLocale )
{       
        Hunspell * pMS;
        rtl_TextEncoding aEnc;

	// initialize a myspell object for each dictionary once 
        // (note: mutex is held higher up in isValid)


	INT16 nRes = -1;

        // first handle smart quotes both single and double
	OUStringBuffer rBuf(rWord);
        sal_Int32 n = rBuf.getLength();
        sal_Unicode c;
	for (sal_Int32 ix=0; ix < n; ix++) {
	    c = rBuf.charAt(ix);
            if ((c == 0x201C) || (c == 0x201D)) rBuf.setCharAt(ix,(sal_Unicode)0x0022);
            if ((c == 0x2018) || (c == 0x2019)) rBuf.setCharAt(ix,(sal_Unicode)0x0027);
        }
        OUString nWord(rBuf.makeStringAndClear());

	if (n)
	{
#if defined USE_JAVA && defined MACOSX
		bool bHandled = false;
		bool bFound = false;
		OUString aLocaleString( ImplGetLocaleString( rLocale ) );
		::std::map< OUString, CFStringRef >::const_iterator it = maPrimaryNativeLocaleMap.find( aLocaleString );
		if ( it != maPrimaryNativeLocaleMap.end() )
		{
			bFound = true;
		}
		else
		{
#endif	// USE_JAVA && MACOSX
            for (sal_Int32 i = 0; i < numdict; ++i) {
	        pMS = NULL;
                aEnc = 0;

	        if (rLocale == aDLocs[i])
	        { 
                   if (!aDicts[i]) 
                   {
                      OUString dicpath = aDNames[i] + A2OU(".dic");
                      OUString affpath = aDNames[i] + A2OU(".aff");
                      OUString dict;
                      OUString aff;
	              osl::FileBase::getSystemPathFromFileURL(dicpath,dict);
 	              osl::FileBase::getSystemPathFromFileURL(affpath,aff);
                      OString aTmpaff(OU2ENC(aff,osl_getThreadTextEncoding()));
                      OString aTmpdict(OU2ENC(dict,osl_getThreadTextEncoding()));

#if defined(WNT)
					  // workaround for Windows specifc problem that the 
					  // path length in calls to 'fopen' is limted to somewhat
					  // about 120+ characters which will usually be exceed when
					  // using dictionaries as extensions.
					  aTmpaff = Win_GetShortPathName( aff );
					  aTmpdict = Win_GetShortPathName( dict );
#endif

                      aDicts[i] = new Hunspell(aTmpaff.getStr(),aTmpdict.getStr());
                      aDEncs[i] = 0;
                      if (aDicts[i]) {
                        char * dic_encoding = aDicts[i]->get_dic_encoding();
			aDEncs[i] = rtl_getTextEncodingFromUnixCharset(aDicts[i]->get_dic_encoding());
                        if (aDEncs[i] == RTL_TEXTENCODING_DONTKNOW) {
			  if (strcmp("ISCII-DEVANAGARI", dic_encoding) == 0) {
			    aDEncs[i] = RTL_TEXTENCODING_ISCII_DEVANAGARI;
                          } else if (strcmp("UTF-8", dic_encoding) == 0) {
			    aDEncs[i] = RTL_TEXTENCODING_UTF8;
                          }
                        }
                      }
	           }
	           pMS = aDicts[i];
                   aEnc = aDEncs[i];
		}
	        if (pMS)
                {
#if defined USE_JAVA && defined MACOSX
				bHandled = true;
#endif	// USE_JAVA && MACOSX
		    OString aWrd(OU2ENC(nWord,aEnc));
	            int rVal = pMS->spell((char*)aWrd.getStr());
 	            if (rVal != 1)
                    {  
                        nRes = SpellFailure::SPELLING_ERROR;
                    } else {
                        return -1;
                    }
                    pMS = NULL;
	        }
	    }

#if defined USE_JAVA && defined MACOSX
		}

		if ( !bHandled )
		{
			if ( !bFound )
			{
				it = maSecondaryNativeLocaleMap.find( aLocaleString );
				if ( it != maSecondaryNativeLocaleMap.end() )
					bFound = true;
			}

			if ( bFound )
			{
				CFStringRef aString = CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, rWord.getStr(), rWord.getLength(), kCFAllocatorNull );
				if ( aString )
				{
					if ( !NSSpellChecker_checkSpellingOfString( aString, it->second ) )
						nRes = SpellFailure::SPELLING_ERROR;
					CFRelease( aString );
				}
			}
		}
#endif	// USE_JAVA && MACOSX
	}

	return nRes;
}


sal_Bool SAL_CALL 
	SpellChecker::isValid( const OUString& rWord, const Locale& rLocale, 
			const PropertyValues& rProperties ) 
		throw(IllegalArgumentException, RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

#ifdef USE_JAVA
	// If the locale is empty, spellcheck using the current locale
	if ( !rWord.getLength() )
#else	// USE_JAVA
 	if (rLocale == Locale()  ||  !rWord.getLength())
#endif	// USE_JAVA
		return TRUE;

	if (!hasLocale( rLocale ))
#ifdef LINGU_EXCEPTIONS
		throw( IllegalArgumentException() );
#else
		return TRUE;
#endif

	// Get property values to be used.
	// These are be the default values set in the SN_LINGU_PROPERTIES
	// PropertySet which are overridden by the supplied ones from the
	// last argument.
	// You'll probably like to use a simplier solution than the provided
	// one using the PropertyHelper_Spell.

	PropertyHelper_Spell &rHelper = GetPropHelper();
	rHelper.SetTmpPropVals( rProperties );

	INT16 nFailure = GetSpellFailure( rWord, rLocale );
	if (nFailure != -1 && !rWord.match(A2OU(SPELLML_HEADER), 0))
	{
		INT16 nLang = LocaleToLanguage( rLocale );
		// postprocess result for errors that should be ignored
		if (   (!rHelper.IsSpellUpperCase()  && IsUpper( rWord, nLang ))
			|| (!rHelper.IsSpellWithDigits() && HasDigits( rWord ))
			|| (!rHelper.IsSpellCapitalization()
				&&  nFailure == SpellFailure::CAPTION_ERROR)
		)
			nFailure = -1;
	}

	return (nFailure == -1);
}


Reference< XSpellAlternatives >
	SpellChecker::GetProposals( const OUString &rWord, const Locale &rLocale )
{
	// Retrieves the return values for the 'spell' function call in case
	// of a misspelled word.
	// Especially it may give a list of suggested (correct) words:
	
	Reference< XSpellAlternatives > xRes;
        // note: mutex is held by higher up by spell which covers both

        Hunspell* pMS;
        rtl_TextEncoding aEnc;
	int count;
        int numsug = 0;

        // first handle smart quotes (single and double)
	OUStringBuffer rBuf(rWord);
        sal_Int32 n = rBuf.getLength();
        sal_Unicode c;
	for (sal_Int32 ix=0; ix < n; ix++) {
	     c = rBuf.charAt(ix);
             if ((c == 0x201C) || (c == 0x201D)) rBuf.setCharAt(ix,(sal_Unicode)0x0022);
             if ((c == 0x2018) || (c == 0x2019)) rBuf.setCharAt(ix,(sal_Unicode)0x0027);
        }
        OUString nWord(rBuf.makeStringAndClear());

	if (n)
	{
	    INT16 nLang = LocaleToLanguage( rLocale );

	    Sequence< OUString > aStr( 0 );

#if defined USE_JAVA && defined MACOSX
		bool bHandled = false;
		bool bFound = false;
		OUString aLocaleString( ImplGetLocaleString( rLocale ) );
		::std::map< OUString, CFStringRef >::const_iterator it = maPrimaryNativeLocaleMap.find( aLocaleString );
		if ( it != maPrimaryNativeLocaleMap.end() )
		{
			bFound = true;
		}
		else
		{
#endif	// USE_JAVA && MACOSX
            for (int i =0; i < numdict; i++) {
	        pMS = NULL;
                aEnc = 0;
                count = 0;

	        if (rLocale == aDLocs[i])
	        { 
                    pMS = aDicts[i];
                    aEnc = aDEncs[i];
                }

	        if (pMS)
	        {
#if defined USE_JAVA && defined MACOSX
				bHandled = true;
#endif	// USE_JAVA && MACOSX
	            char ** suglst = NULL;
		    OString aWrd(OU2ENC(nWord,aEnc));
                    count = pMS->suggest(&suglst, (const char *) aWrd.getStr());

                    if (count) {
                       
	               aStr.realloc( numsug + count );
	               OUString *pStr = aStr.getArray();
                       for (int ii=0; ii < count; ii++)  
                       {  
                          // if needed add: if (suglst[ii] == NULL) continue;
                          OUString cvtwrd(suglst[ii],strlen(suglst[ii]),aEnc);  
                          pStr[numsug + ii] = cvtwrd;  
                          free(suglst[ii]);  
                       }  
                       free(suglst);
                       numsug += count;
                    }
		}
	    }
#if defined USE_JAVA && defined MACOSX
		}

		if ( !bHandled )
		{
			if ( !bFound )
			{
				it = maSecondaryNativeLocaleMap.find( aLocaleString );
				if ( it != maSecondaryNativeLocaleMap.end() )
					bFound = true;
			}

			if ( bFound )
			{
				CFStringRef aString = CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, rWord.getStr(), rWord.getLength(), kCFAllocatorNull );
				if ( aString )
				{
					CFArrayRef aGuesses = NSSpellChecker_getGuesses( aString, it->second );
					if ( aGuesses )
					{
						CFIndex nItems = CFArrayGetCount( aGuesses );
						aStr.realloc( nItems );
						OUString *pAlternativesArray = aStr.getArray();

						CFIndex nItemsAdded = 0;
						for ( CFIndex i = 0; i < nItems; i++ )
						{
							CFStringRef aGuessString = (CFStringRef)CFArrayGetValueAtIndex( aGuesses, i );
							if ( aGuessString )
							{
								CFIndex nLen = CFStringGetLength( aGuessString );
								CFRange aRange = CFRangeMake( 0, nLen );
								sal_Unicode pBuffer[ nLen + 1 ];
								CFStringGetCharacters( aGuessString, aRange, pBuffer );
								pBuffer[ nLen ] = 0;
								OUString aItem( pBuffer );

								if ( !aItem.getLength() )
									continue;

								pAlternativesArray[ nItemsAdded++ ] = aItem;
							}
						}

						aStr.realloc( nItemsAdded );

						CFRelease( aGuesses );
					}

					CFRelease( aString );
				}
			}
		}
#endif	// USE_JAVA && MACOSX
			
            // now return an empty alternative for no suggestions or the list of alternatives if some found
	    SpellAlternatives *pAlt = new SpellAlternatives;
            String aTmp(rWord);
	    pAlt->SetWordLanguage( aTmp, nLang );
	    pAlt->SetFailureType( SpellFailure::SPELLING_ERROR );
	    pAlt->SetAlternatives( aStr );
	    xRes = pAlt;
            return xRes;

	}
        return xRes;
}




Reference< XSpellAlternatives > SAL_CALL 
	SpellChecker::spell( const OUString& rWord, const Locale& rLocale, 
			const PropertyValues& rProperties ) 
		throw(IllegalArgumentException, RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

#ifdef USE_JAVA
	// If the locale is empty, spellcheck using the current locale
 	if ( !rWord.getLength() )
#else	// USE_JAVA
 	if (rLocale == Locale()  ||  !rWord.getLength())
#endif	// USE_JAVA
		return NULL;

	if (!hasLocale( rLocale ))
#ifdef LINGU_EXCEPTIONS
		throw( IllegalArgumentException() );
#else
		return NULL;
#endif

	Reference< XSpellAlternatives > xAlt;
	if (!isValid( rWord, rLocale, rProperties ))
	{
		xAlt =  GetProposals( rWord, rLocale );
	}
	return xAlt;
}


Reference< XInterface > SAL_CALL SpellChecker_CreateInstance( 
            const Reference< XMultiServiceFactory > & /*rSMgr*/ )
		throw(Exception)
{

	Reference< XInterface > xService = (cppu::OWeakObject*) new SpellChecker;
	return xService;
}
    
	
sal_Bool SAL_CALL 
	SpellChecker::addLinguServiceEventListener( 
			const Reference< XLinguServiceEventListener >& rxLstnr ) 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	BOOL bRes = FALSE;   
	if (!bDisposing && rxLstnr.is())
	{
		bRes = GetPropHelper().addLinguServiceEventListener( rxLstnr );
	}
	return bRes;
}


sal_Bool SAL_CALL 
	SpellChecker::removeLinguServiceEventListener( 
			const Reference< XLinguServiceEventListener >& rxLstnr ) 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	BOOL bRes = FALSE;   
	if (!bDisposing && rxLstnr.is())
	{
		DBG_ASSERT( xPropHelper.is(), "xPropHelper non existent" );
		bRes = GetPropHelper().removeLinguServiceEventListener( rxLstnr );
	}
	return bRes;
}


OUString SAL_CALL 
    SpellChecker::getServiceDisplayName( const Locale& /*rLocale*/ ) 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );
#ifdef PRODUCT_NAME
	return A2OU( PRODUCT_NAME " Mac OS X Spellchecker with Hunspell" );
#else	// PRODUCT_NAME
	return A2OU( "Hunspell SpellChecker" );
#endif	// PRODUCT_NAME
}


void SAL_CALL 
	SpellChecker::initialize( const Sequence< Any >& rArguments ) 
		throw(Exception, RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (!pPropHelper)
	{
		INT32 nLen = rArguments.getLength();
		if (2 == nLen)
		{
			Reference< XPropertySet	>	xPropSet;
			rArguments.getConstArray()[0] >>= xPropSet;
			//rArguments.getConstArray()[1] >>= xDicList;

			//! Pointer allows for access of the non-UNO functions.
			//! And the reference to the UNO-functions while increasing
			//! the ref-count and will implicitly free the memory
			//! when the object is not longer used.
			pPropHelper = new PropertyHelper_Spell( (XSpellChecker *) this, xPropSet );
			xPropHelper = pPropHelper;
			pPropHelper->AddAsPropListener();	//! after a reference is established
		}
		else {
			DBG_ERROR( "wrong number of arguments in sequence" );
        }

	}
}


void SAL_CALL 
	SpellChecker::dispose() 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (!bDisposing)
	{
		bDisposing = TRUE;
		EventObject	aEvtObj( (XSpellChecker *) this );
		aEvtListeners.disposeAndClear( aEvtObj );
	}
}


void SAL_CALL 
	SpellChecker::addEventListener( const Reference< XEventListener >& rxListener ) 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (!bDisposing && rxListener.is())
		aEvtListeners.addInterface( rxListener );
}


void SAL_CALL 
	SpellChecker::removeEventListener( const Reference< XEventListener >& rxListener ) 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );
	
	if (!bDisposing && rxListener.is())
		aEvtListeners.removeInterface( rxListener );
}


///////////////////////////////////////////////////////////////////////////
// Service specific part
//

OUString SAL_CALL SpellChecker::getImplementationName() 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	return getImplementationName_Static();
}


sal_Bool SAL_CALL SpellChecker::supportsService( const OUString& ServiceName )
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	Sequence< OUString > aSNL = getSupportedServiceNames();
	const OUString * pArray = aSNL.getConstArray();
	for( INT32 i = 0; i < aSNL.getLength(); i++ )
		if( pArray[i] == ServiceName )
			return TRUE;
	return FALSE;
}


Sequence< OUString > SAL_CALL SpellChecker::getSupportedServiceNames()
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	return getSupportedServiceNames_Static();
}


Sequence< OUString > SpellChecker::getSupportedServiceNames_Static() 
		throw()
{
	MutexGuard	aGuard( GetLinguMutex() );

	Sequence< OUString > aSNS( 1 );	// auch mehr als 1 Service moeglich
	aSNS.getArray()[0] = A2OU( SN_SPELLCHECKER );
	return aSNS;
}


sal_Bool SAL_CALL SpellChecker_writeInfo(
			void * /*pServiceManager*/, registry::XRegistryKey * pRegistryKey )
{

	try
	{
		String aImpl( '/' );
		aImpl += SpellChecker::getImplementationName_Static().getStr();
		aImpl.AppendAscii( "/UNO/SERVICES" );
		Reference< registry::XRegistryKey > xNewKey =
				pRegistryKey->createKey( aImpl );
		Sequence< OUString > aServices =
				SpellChecker::getSupportedServiceNames_Static();
		for( INT32 i = 0; i < aServices.getLength(); i++ )
			xNewKey->createKey( aServices.getConstArray()[i] );

		return sal_True;
	}
	catch(Exception &)
	{
		return sal_False;
	}
}


void * SAL_CALL SpellChecker_getFactory( const sal_Char * pImplName,
			XMultiServiceFactory * pServiceManager, void *  )
{
	void * pRet = 0;
	if ( !SpellChecker::getImplementationName_Static().compareToAscii( pImplName ) )
	{
		Reference< XSingleServiceFactory > xFactory =
			cppu::createOneInstanceFactory(
				pServiceManager,
				SpellChecker::getImplementationName_Static(),
				SpellChecker_CreateInstance,
				SpellChecker::getSupportedServiceNames_Static());
		// acquire, because we return an interface pointer instead of a reference
		xFactory->acquire();
		pRet = xFactory.get();
	}
	return pRet;
}


///////////////////////////////////////////////////////////////////////////
