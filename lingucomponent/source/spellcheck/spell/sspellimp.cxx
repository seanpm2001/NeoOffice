/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to
 *  the terms of GNU General Public License Version 2.1.
 *
 *
 *    GNU General Public License Version 2.1
 *    =============================================
 *    Copyright 2005 by Sun Microsystems, Inc.
 *    901 San Antonio Road, Palo Alto, CA 94303, USA
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public
 *    License version 2.1, as published by the Free Software Foundation.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 *
 *    Modified July 2007 by Patrick Luby. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_lingucomponent.hxx"

#ifndef _COM_SUN_STAR_UNO_REFERENCE_H_
#include <com/sun/star/uno/Reference.h>
#endif
#ifndef _COM_SUN_STAR_LINGUISTIC2_XSEARCHABLEDICTIONARYLIST_HPP_
#include <com/sun/star/linguistic2/XSearchableDictionaryList.hpp>
#endif

#include <com/sun/star/linguistic2/SpellFailure.hpp>
#include <cppuhelper/factory.hxx>	// helper for factories
#include <com/sun/star/registry/XRegistryKey.hpp>

#ifndef _TOOLS_DEBUG_HXX //autogen wg. DBG_ASSERT
#include <tools/debug.hxx>
#endif
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX_
#include <unotools/processfactory.hxx>
#endif
#ifndef _OSL_MUTEX_HXX_
#include <osl/mutex.hxx>
#endif

#include <hunspell.hxx>
#include <dictmgr.hxx>

#ifndef _SPELLIMP_HXX
#include <sspellimp.hxx>
#endif

#include <linguistic/lngprops.hxx>
#include "spelldta.hxx"

#ifndef INCLUDED_SVTOOLS_PATHOPTIONS_HXX
#include <svtools/pathoptions.hxx>
#endif
#ifndef INCLUDED_SVTOOLS_USEROPTIONS_HXX
#include <svtools/useroptions.hxx>
#endif
#include <osl/file.hxx>
#include <rtl/ustrbuf.hxx>

#ifdef USE_JAVA
#include <unistd.h>
#endif	// USE_JAVA


using namespace utl;
using namespace osl;
using namespace rtl;
using namespace com::sun::star;
using namespace com::sun::star::beans;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace com::sun::star::linguistic2;
using namespace linguistic;

#ifdef X11_PRODUCT_NAME

#ifndef DLLPOSTFIX
#error DLLPOSTFIX must be defined in makefile.mk
#endif

#ifndef _OSL_MODULE_HXX_
#include <osl/module.hxx>
#endif

#define DOSTRING( x )			#x
#define STRING( x )				DOSTRING( x )

static bool IsX11Product()
{
    static bool bX11 = sal_False;
    static ::osl::Module aVCLModule;

    if ( !aVCLModule.is() )
    {
        ::rtl::OUString aLibName = ::rtl::OUString::createFromAscii( "libvcl" );
        aLibName += ::rtl::OUString::valueOf( (sal_Int32)SUPD, 10 );
        aLibName += ::rtl::OUString::createFromAscii( STRING( DLLPOSTFIX ) );
        aLibName += ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".dylib" ) );
		aVCLModule.load( aLibName );
        if ( aVCLModule.is() && aVCLModule.getSymbol( ::rtl::OUString::createFromAscii( "XOpenDisplay" ) ) )
            bX11 = true;
    }

    return bX11;
}

#endif	// X11_PRODUCT_NAME

#ifdef USE_JAVA

static OUString aDelimiter = OUString::createFromAscii( "_" );
 
static OUString ImplGetLocaleString( Locale aLocale )
{
	OUString aLocaleString( aLocale.Language );
	if ( aLocale.Country.getLength() )
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

#endif // USE_JAVA

///////////////////////////////////////////////////////////////////////////

BOOL operator == ( const Locale &rL1, const Locale &rL2 )
{
	return	rL1.Language ==  rL2.Language	&&
			rL1.Country  ==  rL2.Country	&&
			rL1.Variant  ==  rL2.Variant;
}

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
#ifdef USE_JAVA
	maLocales = NULL;
#endif	// USE_JAVA
}


SpellChecker::~SpellChecker()
{
#ifdef USE_JAVA
	if ( maLocales )
		CFRelease( maLocales );
#endif	// USE_JAVA

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
	MutexGuard	aGuard( GetLinguMutex() );

        // this routine should return the locales supported by the installed
        // dictionaries.  So here we need to parse both the user edited 
        // dictionary list and the shared dictionary list 
        // to see what dictionaries the admin/user has installed

        int numusr;          // number of user dictionary entries
        int numshr;          // number of shared dictionary entries
        dictentry * spdict;  // shared dict entry pointer
        dictentry * updict;  // user dict entry pointer
        SvtPathOptions aPathOpt;

	std::vector<dictentry *> postspdict;
	std::vector<dictentry *> postupdict;


	if (!numdict) {

            // invoke a dictionary manager to get the user dictionary list
            OUString usrlst = aPathOpt.GetUserDictionaryPath() + A2OU("/dictionary.lst");
            OUString ulst;
	    osl::FileBase::getSystemPathFromFileURL(usrlst,ulst);
            OString uTmp(OU2ENC(ulst, osl_getThreadTextEncoding()));
	    DictMgr* udMgr = new DictMgr(uTmp.getStr(),"DICT");
            numusr = 0;
            if (udMgr) 
                 numusr = udMgr->get_list(&updict);


            // invoke a second  dictionary manager to get the shared dictionary list
            OUString shrlst = aPathOpt.GetLinguisticPath() + A2OU("/ooo/dictionary.lst");
            OUString slst;
	    osl::FileBase::getSystemPathFromFileURL(shrlst,slst);
            OString sTmp(OU2ENC(slst, osl_getThreadTextEncoding()));
	    DictMgr* sdMgr = new DictMgr(sTmp.getStr(),"DICT");
            numshr = 0;
            if (sdMgr) 
                 numshr = sdMgr->get_list(&spdict);

            //Test for existence of the dictionaries
            for (int i = 0; i < numusr; i++)
	    {
            	OUString str = aPathOpt.GetUserDictionaryPath() + A2OU("/") + A2OU(updict[i].filename) + 
			A2OU(".dic");
		osl::File aTest(str);
		if (aTest.open(osl_File_OpenFlag_Read))
			continue;
		aTest.close();
		postupdict.push_back(&updict[i]);
            }

            for (int i = 0; i < numshr; i++)
	    {
                OUString str = aPathOpt.GetLinguisticPath() + A2OU("/ooo/") + A2OU(spdict[i].filename) + 
			A2OU(".dic");
		osl::File aTest(str);
		if (aTest.open(osl_File_OpenFlag_Read))
			continue;
		aTest.close();
		postspdict.push_back(&spdict[i]);
            }

	    numusr = postupdict.size();
            numshr = postspdict.size();

            // we really should merge these and remove duplicates but since 
            // users can name their dictionaries anything they want it would
            // be impossible to know if a real duplication exists unless we
            // add some unique key to each myspell dictionary
            numdict = numshr + numusr;

            if (numdict) {
	        aDicts = new Hunspell* [numdict];
	        aDEncs  = new rtl_TextEncoding [numdict];
                aDLocs = new Locale [numdict];
                aDNames = new OUString [numdict];
	        aSuppLocales.realloc(numdict);
                Locale * pLocale = aSuppLocales.getArray();
                int numlocs = 0;
                int newloc;
                int i,j;
                int k = 0;

                //first add the user dictionaries
                for (i = 0; i < numusr; i++) {
	            Locale nLoc( A2OU(postupdict[i]->lang), A2OU(postupdict[i]->region), OUString() );
                    newloc = 1;
	            for (j = 0; j < numlocs; j++) {
                        if (nLoc == pLocale[j]) newloc = 0;
                    }
                    if (newloc) {
                        pLocale[numlocs] = nLoc;
                        numlocs++;
                    }
                    aDLocs[k] = nLoc;
                    aDicts[k] = NULL;
                    aDEncs[k] = 0;
                    aDNames[k] = aPathOpt.GetUserDictionaryPath() + A2OU("/") + A2OU(postupdict[i]->filename);
#ifdef USE_JAVA
                    // Don't add dictionaries that don't exist
                    OUString dicpath = aDNames[i] + A2OU(".dic");
                    OUString affpath = aDNames[i] + A2OU(".aff");
                    OUString dict;
                    OUString aff;
                    osl::FileBase::getSystemPathFromFileURL(dicpath,dict);
                    osl::FileBase::getSystemPathFromFileURL(affpath,aff);
                    OString aTmpaff(OU2ENC(aff,osl_getThreadTextEncoding()));
                    OString aTmpdict(OU2ENC(dict,osl_getThreadTextEncoding()));

                    if (!access(aTmpaff.getStr(), R_OK) && !access(aTmpdict.getStr(), R_OK))
                        k++;
#else	// USE_JAVA
                    k++;
#endif	// USE_JAVA
                }

                // now add the shared dictionaries
                for (i = 0; i < numshr; i++) {
	            Locale nLoc( A2OU(postspdict[i]->lang), A2OU(postspdict[i]->region), OUString() );
                    newloc = 1;
	            for (j = 0; j < numlocs; j++) {
                        if (nLoc == pLocale[j]) newloc = 0;
                    }
                    if (newloc) {
                        pLocale[numlocs] = nLoc;
                        numlocs++;
                    }
                    aDLocs[k] = nLoc;
                    aDicts[k] = NULL;
                    aDEncs[k] = 0;
                    aDNames[k] = aPathOpt.GetLinguisticPath() + A2OU("/ooo/") + A2OU(postspdict[i]->filename);
#ifdef USE_JAVA
                    // Don't add dictionaries that don't exist
                    OUString dicpath = aDNames[i] + A2OU(".dic");
                    OUString affpath = aDNames[i] + A2OU(".aff");
                    OUString dict;
                    OUString aff;
                    osl::FileBase::getSystemPathFromFileURL(dicpath,dict);
                    osl::FileBase::getSystemPathFromFileURL(affpath,aff);
                    OString aTmpaff(OU2ENC(aff,osl_getThreadTextEncoding()));
                    OString aTmpdict(OU2ENC(dict,osl_getThreadTextEncoding()));
                    if (!access(aTmpaff.getStr(), R_OK) && !access(aTmpdict.getStr(), R_OK))
                        k++;
#else	// USE_JAVA
                    k++;
#endif	// USE_JAVA
                }

                aSuppLocales.realloc(numlocs);

            } else {
	      /* no dictionary.lst found so register no dictionaries */
	        numdict = 0;
	        aDicts = NULL;
                aDEncs  = NULL;
                aDLocs = NULL;
                aDNames = NULL;
	        aSuppLocales.realloc(0);
            }

            /* de-allocation of memory is handled inside the DictMgr */
            updict = NULL;
            if (udMgr) { 
                  delete udMgr;
                  udMgr = NULL;
            }
            spdict = NULL;
            if (sdMgr) { 
                  delete sdMgr;
                  sdMgr = NULL;
            }

        }

#ifdef USE_JAVA
	if ( !maLocales )
	{
		maLocales = NSSpellChecker_getLocales();
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
									maSecondaryNativeLocaleMap[ ImplGetLocaleString( aTmpLocale ) ] = aString;
									bAddLocale = false;
									break;
								}
							}

							if ( pLocaleArray[ j ].Country.getLength() )
							{
								Locale aTmpLocale( pLocaleArray[ j ].Language, OUString(), OUString() );
								if ( aLocale == aTmpLocale )
								{
									maSecondaryNativeLocaleMap[ ImplGetLocaleString( aTmpLocale ) ] = aString;
									bAddLocale = false;
									break;
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
	}
#endif	// USE_JAVA

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
#ifdef USE_JAVA
	// Check native locales first 
	OUString aLocaleString( ImplGetLocaleString( rLocale ) );
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
		Locale aLocale( rLocale.Language, rLocale.Country, OUString() );
		aLocaleString = ImplGetLocaleString( aLocale );
		it = maPrimaryNativeLocaleMap.find( aLocaleString );
		if ( it != maPrimaryNativeLocaleMap.end() )
		{
			bFound = true;
		}
		else
		{
			it = maSecondaryNativeLocaleMap.find( aLocaleString );
			if ( it != maSecondaryNativeLocaleMap.end() )
				bFound = true;
		}

		if ( bFound )
		{
			aSuppLocales.realloc( ++nLen );
			Locale *pLocaleArray = aSuppLocales.getArray();
			pLocaleArray[ nLen - 1 ] = aLocale;
			maSecondaryNativeLocaleMap[ ImplGetLocaleString( rLocale ) ] = it->second;
			return TRUE;
		}
	}

	if ( rLocale.Country.getLength() )
	{
		bool bFound = false;
		Locale aLocale( rLocale.Language, OUString(), OUString() );
		aLocaleString = ImplGetLocaleString( aLocale );
		it = maPrimaryNativeLocaleMap.find( aLocaleString );
		if ( it != maPrimaryNativeLocaleMap.end() )
		{
			bFound = true;
		}
		else
		{
			it = maSecondaryNativeLocaleMap.find( aLocaleString );
			if ( it != maSecondaryNativeLocaleMap.end() )
				bFound = true;
		}

		if ( bFound )
		{
			aSuppLocales.realloc( ++nLen );
			Locale *pLocaleArray = aSuppLocales.getArray();
			pLocaleArray[ nLen - 1 ] = aLocale;
			maSecondaryNativeLocaleMap[ ImplGetLocaleString( rLocale ) ] = it->second;
			return TRUE;
		}
	}
#endif	// USE_JAVA

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
#ifdef USE_JAVA
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
#endif	// USE_JAVA

            for (int i =0; i < numdict; i++) {
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
#ifdef USE_JAVA
				bHandled = true;
#endif	// USE_JAVA
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

#ifdef USE_JAVA
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
#endif	// USE_JAVA
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
	if (nFailure != -1)
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

#ifdef USE_JAVA
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
#endif	// USE_JAVA

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
#ifdef USE_JAVA
				bHandled = true;
#endif	// USE_JAVA
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

#ifdef USE_JAVA
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
					CFMutableArrayRef aGuesses = NSSpellChecker_getGuesses( aString, it->second );
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
#endif	// USE_JAVA
			
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
#else 	// USE_JAVA
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
#ifdef X11_PRODUCT_NAME
	if ( IsX11Product() )
		return A2OU( X11_PRODUCT_NAME " Mac OS X Spellchecker with Hunspell" );
	else
#endif	// X11_PRODUCT_NAME
		return A2OU( PRODUCT_NAME " Mac OS X Spellchecker with Hunspell" );
#else	// PRODUCT_NAME
	return A2OU( "OpenOffice.org Hunspell SpellChecker" );
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
		else
			DBG_ERROR( "wrong number of arguments in sequence" );

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
