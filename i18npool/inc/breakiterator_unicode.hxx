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
 * Modified May 2015 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/
#ifndef _I18N_BREAKITERATOR_UNICODE_HXX_
#define _I18N_BREAKITERATOR_UNICODE_HXX_

#include <breakiteratorImpl.hxx>

#include "warnings_guard_unicode_brkiter.h"

namespace com { namespace sun { namespace star { namespace i18n {

#define LOAD_CHARACTER_BREAKITERATOR    0
#define LOAD_WORD_BREAKITERATOR         1
#define LOAD_SENTENCE_BREAKITERATOR     2
#define LOAD_LINE_BREAKITERATOR         3

//	----------------------------------------------------
//	class BreakIterator_Unicode
//	----------------------------------------------------
class BreakIterator_Unicode : public BreakIteratorImpl
{
public:
	BreakIterator_Unicode();
	~BreakIterator_Unicode();

	virtual sal_Int32 SAL_CALL previousCharacters( const rtl::OUString& Text, sal_Int32 nStartPos, 
		const com::sun::star::lang::Locale& nLocale, sal_Int16 nCharacterIteratorMode, sal_Int32 nCount, 
		sal_Int32& nDone ) throw(com::sun::star::uno::RuntimeException);
	virtual sal_Int32 SAL_CALL nextCharacters( const rtl::OUString& Text, sal_Int32 nStartPos, 
		const com::sun::star::lang::Locale& rLocale, sal_Int16 nCharacterIteratorMode, sal_Int32 nCount, 
		sal_Int32& nDone ) throw(com::sun::star::uno::RuntimeException);

	virtual Boundary SAL_CALL previousWord( const rtl::OUString& Text, sal_Int32 nStartPos, 
		const com::sun::star::lang::Locale& nLocale, sal_Int16 WordType) throw(com::sun::star::uno::RuntimeException);
	virtual Boundary SAL_CALL nextWord( const rtl::OUString& Text, sal_Int32 nStartPos, 
		const com::sun::star::lang::Locale& nLocale, sal_Int16 WordType) throw(com::sun::star::uno::RuntimeException);
	virtual Boundary SAL_CALL getWordBoundary( const rtl::OUString& Text, sal_Int32 nPos, 
		const com::sun::star::lang::Locale& nLocale, sal_Int16 WordType, sal_Bool bDirection ) 
		throw(com::sun::star::uno::RuntimeException);

	virtual sal_Int32 SAL_CALL beginOfSentence( const rtl::OUString& Text, sal_Int32 nStartPos, 
		const com::sun::star::lang::Locale& nLocale ) throw(com::sun::star::uno::RuntimeException);
	virtual sal_Int32 SAL_CALL endOfSentence( const rtl::OUString& Text, sal_Int32 nStartPos, 
		const com::sun::star::lang::Locale& nLocale ) throw(com::sun::star::uno::RuntimeException);

	virtual LineBreakResults SAL_CALL getLineBreak( const rtl::OUString& Text, sal_Int32 nStartPos, 
		const com::sun::star::lang::Locale& nLocale, sal_Int32 nMinBreakPos, 
		const LineBreakHyphenationOptions& hOptions, const LineBreakUserOptions& bOptions ) 
		throw(com::sun::star::uno::RuntimeException);

	//XServiceInfo
	virtual rtl::OUString SAL_CALL getImplementationName() throw( com::sun::star::uno::RuntimeException );
	virtual sal_Bool SAL_CALL supportsService(const rtl::OUString& ServiceName) 
		throw( com::sun::star::uno::RuntimeException );
	virtual com::sun::star::uno::Sequence< rtl::OUString > SAL_CALL getSupportedServiceNames() 
		throw( com::sun::star::uno::RuntimeException );

protected:
	const sal_Char *cBreakIterator, *wordRule, *lineRule;
	Boundary result; // for word break iterator

    struct {
        UnicodeString aICUText;
        icu::BreakIterator *aBreakIterator;
#ifdef USE_JAVA
        rtl::OUString aSrcText;
#endif	// USE_JAVA
    } character, word, sentence, line, *icuBI; 
    com::sun::star::lang::Locale aLocale;
    sal_Int16 aBreakType, aWordType;
    void SAL_CALL loadICUBreakIterator(const com::sun::star::lang::Locale& rLocale, 
        sal_Int16 rBreakType, sal_Int16 rWordType, const sal_Char* name, const rtl::OUString& rText) throw(com::sun::star::uno::RuntimeException);
};

} } } } 

#endif
