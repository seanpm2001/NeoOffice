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

#ifndef INCLUDED_XMLSECURITY_INC_XMLSECURITY_BIGINTEGER_HXX
#define INCLUDED_XMLSECURITY_INC_XMLSECURITY_BIGINTEGER_HXX

#include <sal/config.h>
#include <rtl/ustring.hxx>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.h>

OUString bigIntegerToNumericString( const ::com::sun::star::uno::Sequence< sal_Int8 >& serial );
::com::sun::star::uno::Sequence< sal_Int8 > numericStringToBigInteger ( const OUString& serialNumber );

#ifndef NO_LIBO_IS_AUTHOR_TRUSTED_FIX
namespace xmlsecurity
{
// DNs read as strings from XML files may need to be mangled for compatibility
// as NSS and MS CryptoAPI have different string serialisations; if the DN is
// from an XCertificate it's "native" already and doesn't need to be mangled.
enum EqualMode
{
    NOCOMPAT,
    COMPAT_2ND,
    COMPAT_BOTH
};
SAL_DLLPUBLIC_EXPORT bool EqualDistinguishedNames(OUString rName1,
                                                  OUString rName2, EqualMode eMode);
}
#endif	// !NO_LIBO_IS_AUTHOR_TRUSTED_FIX
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
