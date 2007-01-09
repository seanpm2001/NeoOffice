#*************************************************************************
#
#   $RCSfile$
#
#   $Revision$
#
#   last change: $Author$ $Date$
#
#   The Contents of this file are made available subject to
#   the terms of GNU General Public License Version 2.1.
#
#
#     GNU General Public License Version 2.1
#     =============================================
#     Copyright 2005 by Sun Microsystems, Inc.
#     901 San Antonio Road, Palo Alto, CA 94303, USA
#
#     This library is free software; you can redistribute it and/or
#     modify it under the terms of the GNU General Public
#     License version 2.1, as published by the Free Software Foundation.
#
#     This library is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#     General Public License for more details.
#
#     You should have received a copy of the GNU General Public
#     License along with this library; if not, write to the Free Software
#     Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#     MA  02111-1307  USA
#
#     Modified April 2006 by Patrick Luby. NeoOffice is distributed under
#     GPL only under modification term 3 of the LGPL.
#
#*************************************************************************

# Builds the Java Canvas implementation.

PRJNAME	= canvas
PRJ		= ..$/..
TARGET	= javacanvas
PACKAGE = canvas

USE_JAVAVER:=TRUE 

# --- Settings -----------------------------------------------------

.INCLUDE: settings.mk

.IF "$(SOLAR_JAVA)"=="TRUE"
# Since Canvas needs newer features like
# e.g. java.awt.image.BufferStrategy,
# disabled for now for everything <1.4
.IF "$(JAVANUMVER:s/.//)" >= "000100040000" 

JAVAFILES  = \
	SpriteBase.java \
	JavaCanvas.java \
	CanvasGraphicDevice.java \
	CanvasUtils.java \
	CanvasFont.java \
	CanvasBitmap.java \
	CanvasSprite.java \
	CanvasCustomSprite.java \
	CanvasClonedSprite.java \
	TextLayout.java \
	BackBuffer.java \
	LinePolyPolygon.java \
	BezierPolyPolygon.java \
	SpriteRunner.java

.IF "$(GUIBASE)"!="unx" && "$(GUIBASE)"!="java"

JAVAFILES += win/WindowAdapter.java

.ELSE		# "$(GUIBASE)"!="unx" && "$(GUIBASE)"!="java"

JAVAFILES += x11/WindowAdapter.java

.ENDIF		# "$(GUIBASE)"!="unx" && "$(GUIBASE)"!="java"

JARFILES 		= jurt.jar unoil.jar ridl.jar juh.jar java_uno.jar
#JAVACLASSFILES	= $(foreach,i,$(JAVAFILES) $(CLASSDIR)$/$(PACKAGE)$/$(i:b).class)

JARTARGET               = $(TARGET).uno.jar
JARCOMPRESS             = TRUE
#JARCLASSDIRS            = $(PACKAGE) 
CUSTOMMANIFESTFILE      = manifest

.ENDIF # "$(JAVANUMVER:s/.//)" >= "000100040000" 
.ENDIF # "$(SOLAR_JAVA)"=="TRUE"

# --- Targets ------------------------------------------------------

.INCLUDE: target.mk

#dist: $(JAVA_FILES:b:+".class") 
#	+jar -cvfm $(CLASSDIR)/JavaCanvas.jar $(JARMANIFEST) $(JAVACLASSFILES)
