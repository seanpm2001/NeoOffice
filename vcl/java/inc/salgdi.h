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
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 by Patrick Luby (patrick.luby@planamesa.com)
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

#ifndef _SV_SALGDI_H
#define _SV_SALGDI_H

#include <map>

#ifndef _SV_SV_H
#include <sv.h>
#endif 

class ImplFontSelectData;

#ifdef MACOSX
class SalATSLayout;
#endif	// MACOSX

namespace vcl
{
class com_sun_star_vcl_VCLFont;
class com_sun_star_vcl_VCLGraphics;
}

// -------------------
// - SalGraphicsData -
// ------------------- 

class SalGraphicsData
{
#ifdef MACOSX
	friend class	SalATSLayout;
#endif	// MACOSX
	friend class	SalBitmap;
	friend class	SalFrame;
	friend class	SalGraphics;
	friend class	SalInfoPrinter;
	friend class	SalInstance;
	friend class	SalOpenGL;
	friend class	SalPrinter;
	friend class	SalVirtualDevice;

	SalColor		mnFillColor;
	SalColor		mnLineColor;
	SalColor		mnTextColor;
	SalFrame*		mpFrame;
	SalPrinter*		mpPrinter;
	SalVirtualDevice*	mpVirDev;
	::vcl::com_sun_star_vcl_VCLGraphics*	mpVCLGraphics;
	::vcl::com_sun_star_vcl_VCLFont*	mpVCLFont;
	::std::map< int, ::vcl::com_sun_star_vcl_VCLFont* >	maFallbackFonts;
	::std::map< int, ImplFontSelectData* >	maFallbackFontSelectData;

					SalGraphicsData();
					~SalGraphicsData();
};

#endif // _SV_SALGDI_H
