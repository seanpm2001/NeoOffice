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

#ifndef _LINGU2_SPELLIMP_HXX_
#define _LINGU2_SPELLIMP_HXX_

#include <uno/lbnames.h>			// CPPU_CURRENT_LANGUAGE_BINDING_NAME macro, which specify the environment type
#include <cppuhelper/implbase1.hxx>	// helper for implementations
#include <cppuhelper/implbase6.hxx>	// helper for implementations
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XServiceDisplayName.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValues.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/linguistic2/XSpellChecker.hpp>
#include <com/sun/star/linguistic2/XSearchableDictionaryList.hpp>
#include <com/sun/star/linguistic2/XLinguServiceEventBroadcaster.hpp>
#include <tools/table.hxx>

#include <lingutil.hxx>
#include <linguistic/misc.hxx>
#include "sprophelp.hxx"

#if defined USE_JAVA && defined MACOSX

#include <map>
#include "sspellimp_cocoa.h"

#endif	// USE_JAVA && MACOSX

using namespace ::rtl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::linguistic2;


///////////////////////////////////////////////////////////////////////////


class SpellChecker :
	public cppu::WeakImplHelper6
	<
		XSpellChecker,
		XLinguServiceEventBroadcaster,
		XInitialization,
		XComponent,
		XServiceInfo,
		XServiceDisplayName
	>
{
	Sequence< Locale >                 aSuppLocales;
        Hunspell **                         aDicts;
        rtl_TextEncoding *                 aDEncs;
        Locale *                           aDLocs;
        OUString *                         aDNames;
        sal_Int32                          numdict;

#if defined USE_JAVA && defined MACOSX
	CFArrayRef								maLocales;
	::std::map< ::rtl::OUString, CFStringRef >	maPrimaryNativeLocaleMap;
	::std::map< ::rtl::OUString, CFStringRef >	maSecondaryNativeLocaleMap;
#endif	// USE_JAVA && MACOSX
	::cppu::OInterfaceContainerHelper		aEvtListeners;
	Reference< XPropertyChangeListener >	xPropHelper;
	PropertyHelper_Spell *			 		pPropHelper;
	BOOL									bDisposing;

	// disallow copy-constructor and assignment-operator for now
	SpellChecker(const SpellChecker &);
	SpellChecker & operator = (const SpellChecker &);

	PropertyHelper_Spell &	GetPropHelper_Impl();
	PropertyHelper_Spell &	GetPropHelper()
	{
		return pPropHelper ? *pPropHelper : GetPropHelper_Impl();
	}

	INT16	GetSpellFailure( const OUString &rWord, const Locale &rLocale );
	Reference< XSpellAlternatives >
			GetProposals( const OUString &rWord, const Locale &rLocale );

public:
	SpellChecker();
	virtual ~SpellChecker();

	// XSupportedLocales (for XSpellChecker)
    virtual Sequence< Locale > SAL_CALL 
		getLocales() 
			throw(RuntimeException);
    virtual sal_Bool SAL_CALL 
		hasLocale( const Locale& rLocale ) 
			throw(RuntimeException);

	// XSpellChecker
    virtual sal_Bool SAL_CALL 
		isValid( const OUString& rWord, const Locale& rLocale, 
				const PropertyValues& rProperties ) 
			throw(IllegalArgumentException, 
				  RuntimeException);
    virtual Reference< XSpellAlternatives > SAL_CALL 
		spell( const OUString& rWord, const Locale& rLocale, 
				const PropertyValues& rProperties ) 
			throw(IllegalArgumentException, 
				  RuntimeException);

    // XLinguServiceEventBroadcaster
    virtual sal_Bool SAL_CALL 
		addLinguServiceEventListener( 
			const Reference< XLinguServiceEventListener >& rxLstnr ) 
			throw(RuntimeException);
    virtual sal_Bool SAL_CALL 
		removeLinguServiceEventListener( 
			const Reference< XLinguServiceEventListener >& rxLstnr ) 
			throw(RuntimeException);
	
	// XServiceDisplayName
    virtual OUString SAL_CALL 
		getServiceDisplayName( const Locale& rLocale ) 
			throw(RuntimeException);

	// XInitialization
    virtual void SAL_CALL 
		initialize( const Sequence< Any >& rArguments ) 
			throw(Exception, RuntimeException);

	// XComponent
	virtual void SAL_CALL 
		dispose() 
			throw(RuntimeException);
    virtual void SAL_CALL 
		addEventListener( const Reference< XEventListener >& rxListener ) 
			throw(RuntimeException);
    virtual void SAL_CALL 
		removeEventListener( const Reference< XEventListener >& rxListener ) 
			throw(RuntimeException);

	////////////////////////////////////////////////////////////
	// Service specific part
	//

	// XServiceInfo
    virtual OUString SAL_CALL 
		getImplementationName() 
			throw(RuntimeException);
    virtual sal_Bool SAL_CALL 
		supportsService( const OUString& rServiceName ) 
			throw(RuntimeException);
    virtual Sequence< OUString > SAL_CALL 
		getSupportedServiceNames() 
			throw(RuntimeException);


	static inline OUString	
		getImplementationName_Static() throw();
    static Sequence< OUString >	
		getSupportedServiceNames_Static() throw();
};

inline OUString SpellChecker::getImplementationName_Static() throw()
{
	return A2OU( "org.openoffice.lingu.MySpellSpellChecker" );
}



///////////////////////////////////////////////////////////////////////////

#endif

