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
 *	 - GNU General Public License Version 2.1
 *
 *  Edward Peterlin, September 2004
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2004 by Edward Peterlin (OPENSTEP@neooffice.org)
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

#ifndef _SV_COM_SUN_STAR_VCL_VCLMENUBAR_HXX
#define	_SV_COM_SUN_STAR_VCL_VCLMENUBAR_HXX

#ifndef _SV_JAVA_LANG_OBJECT_HXX
#include <java/lang/Object.hxx>
#endif

class SalMenu;

namespace vcl {

class com_sun_star_vcl_VCLFrame;
class com_sun_star_vcl_VCLMenuItemData;

class com_sun_star_vcl_VCLMenuBar : public java_lang_Object
{
protected:
	static jclass		theClass;

public:
	friend class SalMenu;
	
	static jclass		getMyClass();
				
						com_sun_star_vcl_VCLMenuBar( jobject myObj ) : java_lang_Object( myObj ) {}
				
						com_sun_star_vcl_VCLMenuBar();
	virtual				~com_sun_star_vcl_VCLMenuBar() {}
	
	void				setFrame( com_sun_star_vcl_VCLFrame *_par0 );
	void				dispose();
	void				addMenuItem( com_sun_star_vcl_VCLMenuItemData *_par0, int _par1 );
	void				removeMenu( int _par0 );
	void				changeMenu( com_sun_star_vcl_VCLMenuItemData *_par0, int _par1 );
	void				enableMenu( int _par0, bool _par1 );
	void				syncCheckboxMenuItemState( void );
};

} // namespace vcl

#endif // _SV_COM_SUN_STAR_VCL_VCLMENUBAR_HXX
