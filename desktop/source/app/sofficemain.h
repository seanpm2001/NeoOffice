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
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 * Modified August 2008 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

#ifndef INCLUDED_DESKTOP_SOURCE_APP_SOFFICEMAIN_H
#define INCLUDED_DESKTOP_SOURCE_APP_SOFFICEMAIN_H

#include "sal/config.h"

#if defined __cplusplus
extern "C" {
#endif

#ifdef USE_JAVA
int soffice_main( int argc, char **argv );
#else	// USE_JAVA
int soffice_main(void);
#endif	// USE_JAVA

#if defined __cplusplus
}
#endif

#endif
