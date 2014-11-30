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
# Modified November 2014 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

PRJ=..

PRJNAME=sd
TARGET=sdraw3
GEN_HID=TRUE
GEN_HID_OTHER=TRUE
USE_DEFFILE=TRUE

# --- Settings -----------------------------------------------------------

.INCLUDE :  settings.mk

.IF "$(UPD)" == "310"
PREPENDLIBS=$(PRJ)$/..$/comphelper$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/oox$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/salhelper$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/sax$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/sfx2$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/svtools$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/svx$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/tools$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/vcl$/$(INPATH)$/lib \
	-L$(PRJ)$/..$/xmloff$/$(INPATH)$/lib

# Link to modified libraries
SOLARLIB:=-L$(PREPENDLIBS) $(SOLARLIB)
SOLARLIBDIR:=$(PREPENDLIBS) -L$(SOLARLIBDIR)
.ENDIF		# "$(UPD)" == "310"

# --- Resources ----------------------------------------------------

RESLIB1NAME=sd
RESLIB1IMAGES=$(PRJ)$/res/imagelst $(PRJ)$/res
RESLIB1SRSFILES=\
	$(SRS)$/app.srs				\
	$(SRS)$/dlg.srs				\
	$(SRS)$/core.srs			\
	$(SRS)$/html.srs			\
	$(SRS)$/accessibility.srs	\
	$(SRS)$/notes.srs			\
	$(SRS)$/animui.srs			\
	$(SRS)$/slideshow.srs		\
	$(SRS)$/uitable.srs			\
	$(SOLARCOMMONRESDIR)$/sfx.srs

# --- StarDraw DLL

SHL1TARGET= sd$(DLLPOSTFIX)
SHL1USE_EXPORTS=name
SHL1IMPLIB= sdi

# dynamic libraries
SHL1STDLIBS+= \
	$(SVXCORELIB) \
	$(SVXLIB) \
	$(SFXLIB) \
	$(BASICLIB) \
	$(CPPCANVASLIB) \
	$(BASEGFXLIB) \
	$(DRAWINGLAYERLIB) \
	$(GOODIESLIB) \
    $(BASEGFXLIB) \
	$(SVTOOLLIB) \
	$(TKLIB) \
	$(VCLLIB) \
	$(SVLLIB) \
	$(SOTLIB) \
    $(CANVASTOOLSLIB) \
	$(UNOTOOLSLIB) \
	$(TOOLSLIB) \
	$(I18NISOLANGLIB) \
	$(COMPHELPERLIB) \
	$(UCBHELPERLIB) \
	$(CPPUHELPERLIB) \
	$(CPPULIB) \
	$(VOSLIB) \
	$(CANVASLIB) \
	$(SALLIB) \
	$(AVMEDIALIB)

SHL1LIBS= $(LIB3TARGET) $(LIB5TARGET) $(LIB6TARGET)
SHL1DEPN+=	makefile.mk

SHL1DEF=    $(MISC)$/$(SHL1TARGET).def
DEF1DEPN        =$(MISC)$/$(SHL1TARGET).flt
DEF1NAME	=$(SHL1TARGET)
DEFLIB1NAME = $(TARGET) $(LIB5TARGET:b) $(LIB6TARGET:b)

.IF "$(GUI)" == "WNT"
SHL1RES=    $(RCTARGET)
.ENDIF

# --- Linken der Applikation ---------------------------------------

LIB2TARGET=$(SLB)$/sdmod.lib
LIB2OBJFILES=   \
			$(SLO)$/sdmod1.obj      \
			$(SLO)$/sdmod2.obj      \
			$(SLO)$/sdmod.obj

LIB3TARGET=$(SLB)$/sdraw3.lib
LIB3FILES=      \
			$(SLB)$/view.lib        \
			$(SLB)$/app.lib			\
			$(SLB)$/docshell.lib    \
			$(SLB)$/dlg.lib			\
			$(SLB)$/core.lib		\
			$(SLB)$/undo.lib		\
			$(SLB)$/helper.lib		\
			$(SLB)$/xml.lib			\
			$(SLB)$/cgm.lib			\
			$(SLB)$/uitable.lib		\
			$(SLB)$/grf.lib

LIB5TARGET=$(SLB)$/sdraw3_2.lib
LIB5FILES=      \
			$(SLB)$/html.lib		\
			$(SLB)$/filter.lib		\
			$(SLB)$/unoidl.lib		\
			$(SLB)$/animui.lib		\
			$(SLB)$/accessibility.lib	\
			$(SLB)$/toolpanel.lib		\
			$(SLB)$/uitools.lib			\
			$(SLB)$/tpcontrols.lib

LIB6TARGET=$(SLB)$/sdraw3_3.lib
LIB6FILES=      								\
			$(SLB)$/func.lib        			\
			$(SLB)$/func_2.lib        			\
			$(SLB)$/slsshell.lib				\
			$(SLB)$/slsmodel.lib				\
			$(SLB)$/slsview.lib					\
			$(SLB)$/slscontroller.lib			\
			$(SLB)$/slscache.lib				\
			$(SLB)$/slideshow.lib				\
			$(SLB)$/framework_configuration.lib	\
			$(SLB)$/framework_factories.lib		\
			$(SLB)$/framework_module.lib		\
			$(SLB)$/framework_tools.lib			\
			$(SLB)$/presenter.lib

# sdd
SHL2TARGET= sdd$(DLLPOSTFIX)
SHL2IMPLIB= sddimp
SHL2VERSIONMAP= sdd.map
SHL2DEF=$(MISC)$/$(SHL2TARGET).def
DEF2NAME=		$(SHL2TARGET)

SHL2STDLIBS= \
			$(SFX2LIB) \
			$(SVXCORELIB) \
			$(SVTOOLLIB) \
			$(SVLLIB) \
			$(VCLLIB) \
                        $(SOTLIB) \
			$(TOOLSLIB) \
			$(UCBHELPERLIB) \
			$(COMPHELPERLIB) \
			$(CPPUHELPERLIB) \
			$(CPPULIB) \
			$(SALLIB)

SHL2OBJS=   $(SLO)$/sddetect.obj \
	    $(SLO)$/detreg.obj

SHL2DEPN+=  makefile.mk

# sdui
SHL4TARGET= sdui$(DLLPOSTFIX)
SHL4IMPLIB= sduiimp
SHL4VERSIONMAP= sdui.map
SHL4DEF=$(MISC)$/$(SHL4TARGET).def
DEF4NAME=       $(SHL4TARGET)
SHL4LIBS=   $(SLB)$/sdui_all.lib

LIB4TARGET=	$(SLB)$/sdui_all.lib
LIB4FILES=	\
	$(SLB)$/sdui.lib \
	$(SLB)$/func_ui.lib \
	$(SLB)$/html_ui.lib

SHL4STDLIBS= \
	$(ISDLIB) \
	$(SVXCORELIB) \
	$(SVXLIB) \
	$(SFXLIB) \
	$(BASICLIB) \
	$(BASEGFXLIB) \
	$(DRAWINGLAYERLIB) \
	$(GOODIESLIB) \
	$(SO2LIB) \
	$(SVTOOLLIB) \
	$(TKLIB) \
	$(VCLLIB) \
	$(SVLLIB) \
	$(SOTLIB) \
	$(UNOTOOLSLIB) \
	$(TOOLSLIB) \
	$(I18NISOLANGLIB) \
	$(COMPHELPERLIB) \
	$(UCBHELPERLIB) \
	$(CPPUHELPERLIB) \
	$(CPPULIB) \
	$(VOSLIB) \
	$(CANVASLIB) \
	$(SALLIB)

.IF "$(GUI)$(COM)" == "WNTMSC"
.IF "$(ENABLE_PCH)" != ""
#target sd
SHL1OBJS += $(SLO)$/pchname.obj \
            $(SLO)$/pchname_ex.obj
#target sdd
SHL2OBJS += $(SLO)$/pchname.obj \
            $(SLO)$/pchname_ex.obj
#target sdui
SHL4OBJS += $(SLO)$/pchname.obj \
            $(SLO)$/pchname_ex.obj
.ENDIF # "$(ENABLE_PCH)" != ""
.ENDIF # "$(GUI)$(COM)" == "WNTMSC"

# $(ISDLIB) is build in SHL1TARGET
.IF "$(GUI)" == "UNX"
SHL4DEPN=$(SHL1TARGETN)
SHL5DEPN=$(SHL1TARGETN)
.ELSE
SHL4DEPN=$(SHL1IMPLIBN)
SHL5DEPN=$(SHL1IMPLIBN)
.ENDIF

# ppt import/export library
SHL5TARGET    = sdfilt$(DLLPOSTFIX)
SHL5IMPLIB    = sdfilti
SHL5VERSIONMAP= sdfilt.map
SHL5DEF       = $(MISC)$/$(SHL5TARGET).def
SHL5LIBS      = $(SLB)$/ppt.lib $(SLB)$/eppt.lib

DEF5NAME=$(SHL5TARGET)

SHL5STDLIBS = $(ISDLIB) \
              $(SVXCORELIB)       \
              $(SVXMSFILTERLIB)   \
              $(SFX2LIB)          \
              $(SVTOOLLIB)        \
              $(SOTLIB)           \
              $(GOODIESLIB)       \
              $(VCLLIB)           \
              $(SVLLIB)           \
              $(SOTLIB)           \
              $(UNOTOOLSLIB)      \
              $(TOOLSLIB)         \
              $(UCBHELPERLIB)     \
              $(CPPUHELPERLIB)    \
              $(CPPULIB)          \
              $(SALLIB)           \
              $(COMPHELPERLIB)    \
              $(I18NISOLANGLIB)

# pptx export library
SHL6TARGET    = pptx$(DLLPOSTFIX)
SHL6IMPLIB    = pptxi
SHL6VERSIONMAP= pptx.map
SHL6DEF       = $(MISC)$/$(SHL6TARGET).def
SHL6LIBS      = $(SLB)$/pptx.lib
SHL6DEPN      = $(SHL1TARGETN)
DEF6NAME      = $(SHL6TARGET)

SHL6STDLIBS = $(ISDLIB) \
              $(SVXCORELIB)       \
              $(SVXMSFILTERLIB)   \
              $(SFX2LIB)          \
              $(SVTOOLLIB)        \
              $(SOTLIB)           \
              $(GOODIESLIB)       \
              $(VCLLIB)           \
              $(SVLLIB)           \
              $(SOTLIB)           \
              $(UNOTOOLSLIB)      \
              $(TOOLSLIB)         \
              $(UCBHELPERLIB)     \
              $(CPPUHELPERLIB)    \
              $(CPPULIB)          \
              $(SALLIB)           \
              $(COMPHELPERLIB)    \
              $(I18NISOLANGLIB)   \
              $(OOXLIB)           \
              $(SAXLIB)

# --- Targets -------------------------------------------------------------

.INCLUDE :  target.mk

$(MISC)$/$(SHL1TARGET).flt: makefile.mk
    @echo ------------------------------
    @echo Making: $@
    @$(TYPE) sd.flt > $@

