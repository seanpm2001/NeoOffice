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
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2002-2003 William Lachance (william.lachance@sympatico.ca)
 *  http://libwpd.sourceforge.net
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
 *  Modified November 2004 by Patrick Luby. SISSL Removed. NeoOffice is
 *  distributed under GPL only under modification term 3 of the LGPL.
 *
 *  Contributor(s): _______________________________________
 *
 ************************************************************************/


#ifndef _STYLE_H
#define _STYLE_H
#include <libwpd/libwpd.h>

class DocumentElement;

#ifndef _COM_SUN_STAR_XML_SAX_XDOCUMENTHANDLER_HPP_
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#endif

using com::sun::star::uno::Reference;
using com::sun::star::xml::sax::XDocumentHandler;

class TopLevelElementStyle
{
public:
	TopLevelElementStyle() : mpsMasterPageName(NULL) { }
	virtual ~TopLevelElementStyle() { if (mpsMasterPageName) delete mpsMasterPageName; }
	void setMasterPageName(UTF8String &sMasterPageName) { mpsMasterPageName = new UTF8String(sMasterPageName); }
	const UTF8String * getMasterPageName() const { return mpsMasterPageName; }

private:
	UTF8String *mpsMasterPageName;
};

class Style
{
 public:
	Style(const UTF8String &psName) : msName(psName) {}
	virtual ~Style() {}

	virtual void write(Reference < XDocumentHandler > &xHandler) const {};
	const UTF8String &getName() const { return msName; }

 private:
	UTF8String msName;
};
#endif
