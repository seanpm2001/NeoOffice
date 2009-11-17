#*************************************************************************
#
# Copyright 2008 by Sun Microsystems, Inc.
#
# $RCSfile$
#
# $Revision$
#
# This file is part of NeoOffice.
#
# NeoOffice is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3
# only, as published by the Free Software Foundation.
#
# NeoOffice is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 3 for more details
# (a copy is included in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU General Public License
# version 3 along with NeoOffice.  If not, see
# <http://www.gnu.org/licenses/gpl-3.0.txt>
# for a copy of the GPLv3 License.
#
# Modified November 2009 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

PRJ=..

PRJNAME=svx
TARGET=svx
GEN_HID=TRUE
GEN_HID_OTHER=TRUE

USE_DEFFILE=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

.IF "$(OS)"=="IRIX"
LINKFLAGS+=-Wl,-LD_LAYOUT:lgot_buffer=30
.ENDIF

RSCLOCINC+=-I$(PRJ)$/source$/svdraw

# --- Svx - DLL ----------

LIB1TARGET= $(SLB)$/$(TARGET).lib
LIB1FILES=\
	$(SLB)$/animation.lib \
	$(SLB)$/overlay.lib \
	$(SLB)$/svdraw.lib \
	$(SLB)$/form.lib

.IF "(GUIBASE)" == "WIN"
LIB1FILES+=$(SLB)$/ibrwimp.lib
.ENDIF # (WIN)

LIB2TARGET= $(SLB)$/$(TARGET)_2.lib
LIB2FILES=\
	$(SLB)$/init.lib \
	$(SLB)$/items.lib     \
	$(SLB)$/svxlink.lib   \
	$(SLB)$/svxrtf.lib    \
	$(SLB)$/editeng.lib   \
	$(SLB)$/outliner.lib \
	$(SLB)$/dialogs.lib	\
	$(SLB)$/mnuctrls.lib  \
	$(SLB)$/options.lib   \
	$(SLB)$/stbctrls.lib  \
	$(SLB)$/tbxctrls.lib  \
    $(SLB)$/unoedit.lib   \
    $(SLB)$/smarttags.lib

LIB3TARGET= $(SLB)$/$(TARGET)_3.lib
LIB3FILES=\
	$(SLB)$/unodraw.lib	\
	$(SLB)$/unogallery.lib\
	$(SLB)$/gal.lib		\
	$(SLB)$/accessibility.lib	\
	$(SLB)$/customshapes.lib\
	$(SLB)$/toolbars.lib \
	$(SLB)$/properties.lib \
	$(SLB)$/contact.lib \
	$(SLB)$/mixer.lib \
        $(SLB)$/event.lib

LIB4TARGET= $(SLB)$/$(TARGET)_4.lib
LIB4FILES=\
	$(SLB)$/fmcomp.lib \
	$(SLB)$/engine3d.lib \
	$(SLB)$/msfilter.lib \
	$(SLB)$/xout.lib \
	$(SLB)$/xml.lib \
	$(SLB)$/table.lib

HELPIDFILES=    ..$/inc$/helpid.hrc

# svx
SHL1TARGET= svx$(DLLPOSTFIX)
SHL1IMPLIB= i$(TARGET)
SHL1USE_EXPORTS=name

SHL1LIBS= $(LIB1TARGET) $(LIB2TARGET) $(LIB3TARGET) $(LIB4TARGET)
SHL1STDLIBS= \
			$(AVMEDIALIB) \
            $(SFX2LIB) \
			$(XMLOFFLIB) \
            $(GOODIESLIB) \
            $(BASEGFXLIB) \
            $(BASICLIB) \
            $(LNGLIB) \
            $(SVTOOLLIB) \
            $(TKLIB) \
            $(VCLLIB) \
			$(SVLLIB) \
            $(SOTLIB) \
			$(UNOTOOLSLIB) \
            $(TOOLSLIB) \
            $(I18NISOLANGLIB) \
			$(XMLSCRIPTLIB) \
			$(COMPHELPERLIB) \
			$(UCBHELPERLIB)	\
			$(CPPUHELPERLIB)	\
			$(CPPULIB) \
            $(VOSLIB) \
            $(SALLIB) \
            $(ICUUCLIB)

.IF "$(GUI)"=="WNT"
SHL1STDLIBS+=$(SHELLLIB)
.ENDIF # WNT

.IF "$(GUIBASE)"=="java"
SHL1STDLIBS+=-framework CoreFoundation
.ENDIF		# "$(GUIBASE)"=="java"

SHL1DEF=	$(MISC)$/$(SHL1TARGET).def
DEF1NAME=	$(SHL1TARGET)
DEFLIB1NAME=$(TARGET) $(TARGET)_2 $(TARGET)_3 $(TARGET)_4


# cui
SHL2TARGET= cui$(DLLPOSTFIX)
SHL2VERSIONMAP= cui.map
SHL2IMPLIB=icui
SHL2DEPN=$(SHL1TARGETN)

SHL2DEF=	$(MISC)$/$(SHL2TARGET).def
DEF2NAME=	$(SHL2TARGET)

SHL2LIBS=	$(SLB)$/cui.lib
SHL2OBJS= \
        $(SLO)$/cuiexp.obj     \
        $(SLO)$/dlgfact.obj

SHL2STDLIBS= \
            $(SVXLIB) \
			$(AVMEDIALIB) \
            $(SFX2LIB) \
            $(GOODIESLIB) \
            $(BASEGFXLIB) \
            $(LNGLIB) \
            $(SVTOOLLIB) \
            $(TKLIB) \
            $(VCLLIB) \
			$(SVLLIB) \
            $(SOTLIB) \
			$(UNOTOOLSLIB) \
            $(TOOLSLIB) \
            $(I18NISOLANGLIB) \
			$(COMPHELPERLIB) \
			$(UCBHELPERLIB)	\
			$(CPPULIB) \
            $(VOSLIB) \
            $(SALLIB) \
            $(JVMFWKLIB) \
            $(ICUUCLIB)

.IF "$(GUI)"=="WNT"
SHL2STDLIBS+= \
             $(SHLWAPILIB) \
             $(CPPUHELPERLIB) \
             $(ADVAPI32LIB)
.ENDIF # WNT

# ------------------------------------------------------------------------------

# Resource files
SRSFILELIST=\
                $(SRS)$/svdstr.srs      \
                $(SRS)$/editeng.srs     \
                $(SRS)$/outliner.srs \
                $(SRS)$/dialogs.srs     \
                $(SRS)$/drawdlgs.srs \
                $(SRS)$/mnuctrls.srs \
                $(SRS)$/stbctrls.srs \
                $(SRS)$/tbxctrls.srs \
                $(SRS)$/options.srs     \
                $(SRS)$/svxitems.srs \
				$(SRS)$/form.srs \
				$(SRS)$/fmcomp.srs \
				$(SRS)$/engine3d.srs \
				$(SRS)$/unodraw.srs \
                $(SRS)$/svxlink.srs \
                $(SRS)$/accessibility.srs \
                $(SRS)$/toolbars.srs \
                $(SOLARCOMMONRESDIR)$/sfx.srs

RESLIB1NAME=svx
RESLIB1IMAGES=$(PRJ)$/res $(PRJ)$/source/svdraw
RESLIB1SRSFILES= $(SRSFILELIST)

# --- Targets -------------------------------------------------------

.INCLUDE :  target.mk

