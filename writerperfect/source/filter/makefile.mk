##########################################################################
# 
#   $RCSfile$
# 
#   $Revision$
# 
#   last change: $Author$ $Date$
# 
#   The Contents of this file are made available subject to the terms of
#   either of the following licenses
# 
#          - GNU General Public License Version 2.1
# 
#   GNU General Public License Version 2.1
#   =============================================
#   Copyright 2002-2003 William Lachance (william.lachance@sympatico.ca)
#   http://libwpd.sourceforge.net
# 
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public
#   License version 2.1, as published by the Free Software Foundation.
# 
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
# 
#   You should have received a copy of the GNU General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#   MA  02111-1307  USA
#   
#   =================================================
#   Modified November 2004 by Patrick Luby. SISSL Removed. NeoOffice is
#   distributed under GPL only under modification term 3 of the LGPL.
# 
#   Contributor(s): _______________________________________
# 
##########################################################################

PRJ=..$/..

PRJNAME=filter
TARGET=filter
ENABLE_EXCEPTIONS=true

.INCLUDE :  settings.mk

# broken but ... necessary, internal include shafted ...
INCPRE+=$(SOLARVER)$/$(UPD)$/$(INPATH)$/inc$/libwpd -I..

SLOFILES= \
	$(SLO)$/DocumentElement.obj		\
	$(SLO)$/FontMap.obj			\
	$(SLO)$/FontStyle.obj			\
	$(SLO)$/ListStyle.obj			\
	$(SLO)$/PageSpan.obj			\
	$(SLO)$/SectionStyle.obj		\
	$(SLO)$/TableStyle.obj			\
	$(SLO)$/TextRunStyle.obj 		\
	$(SLO)$/WordPerfectCollector.obj 	\
	$(SLO)$/WordPerfectImportFilter.obj 	\
	$(SLO)$/genericfilter.obj		

.INCLUDE :  target.mk
