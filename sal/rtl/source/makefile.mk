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
#     Modified July 2006 by Patrick Luby. NeoOffice is distributed under
#     GPL only under modification term 3 of the LGPL.
#
#*************************************************************************

PRJ=..$/..

PRJNAME=sal
TARGET=cpprtl
USE_LDUMP2=TRUE

PROJECTPCH4DLL=TRUE
PROJECTPCH=cont_pch
PROJECTPCHSOURCE=cont_pch

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

.IF "$(ALLOC)" == "SYS_ALLOC" || "$(ALLOC)" == "TCMALLOC" || "$(GUIBASE)" == "java"
CDEFS+= -DFORCE_SYSALLOC
.ENDIF

# --- Files --------------------------------------------------------

.IF "$(header)" == ""

ALWAYSDBGFILES=$(SLO)$/debugprint.obj

.IF "$(ALWAYSDBGFILES)" != ""
ALWAYSDBGTARGET=do_it_alwaysdebug
.ENDIF

SLOFILES=   \
	$(SLO)$/alloc_global.obj     \
	$(SLO)$/alloc_cache.obj      \
	$(SLO)$/alloc_arena.obj      \
            $(SLO)$/memory.obj      \
            $(SLO)$/cipher.obj      \
            $(SLO)$/crc.obj         \
            $(SLO)$/digest.obj      \
            $(SLO)$/random.obj      \
            $(SLO)$/locale.obj      \
            $(SLO)$/strimp.obj      \
            $(SLO)$/string.obj      \
            $(SLO)$/ustring.obj     \
            $(SLO)$/strbuf.obj      \
            $(SLO)$/ustrbuf.obj     \
            $(SLO)$/uuid.obj        \
            $(SLO)$/rtl_process.obj \
            $(SLO)$/byteseq.obj     \
            $(SLO)$/uri.obj			\
            $(SLO)$/bootstrap.obj  	\
            $(SLO)$/cmdargs.obj		\
            $(SLO)$/macro.obj		\
            $(SLO)$/unload.obj		\
            $(SLO)$/logfile.obj     \
            $(SLO)$/tres.obj        \
            $(SLO)$/debugprint.obj        \
            $(SLO)$/math.obj

#.IF "$(UPDATER)"=="YES"
OBJFILES=   \
	$(OBJ)$/alloc_global.obj     \
	$(OBJ)$/alloc_cache.obj      \
	$(OBJ)$/alloc_arena.obj      \
            $(OBJ)$/memory.obj      \
            $(OBJ)$/cipher.obj      \
            $(OBJ)$/crc.obj         \
            $(OBJ)$/digest.obj      \
            $(OBJ)$/random.obj      \
            $(OBJ)$/locale.obj      \
            $(OBJ)$/strimp.obj      \
            $(OBJ)$/string.obj      \
            $(OBJ)$/ustring.obj     \
            $(OBJ)$/strbuf.obj      \
            $(OBJ)$/ustrbuf.obj     \
            $(OBJ)$/uuid.obj        \
            $(OBJ)$/rtl_process.obj \
            $(OBJ)$/byteseq.obj     \
            $(OBJ)$/uri.obj			\
            $(OBJ)$/bootstrap.obj  	\
            $(OBJ)$/cmdargs.obj		\
            $(OBJ)$/macro.obj		\
            $(OBJ)$/unload.obj		\
            $(OBJ)$/logfile.obj     \
            $(OBJ)$/tres.obj        \
            $(OBJ)$/math.obj

# --- Extra objs ----------------------------------------------------

.IF "$(OS)"=="LINUX"

#
# This part builds a second version of alloc.c, with 
# FORCE_SYSALLOC defined. Is later used in util/makefile.mk
# to build a tiny replacement lib to LD_PRELOAD into the 
# office, enabling e.g. proper valgrinding.
#

SECOND_BUILD=SYSALLOC
SYSALLOC_SLOFILES=	$(SLO)$/alloc_global.obj
SYSALLOCCDEFS+=-DFORCE_SYSALLOC

.ENDIF # .IF "$(OS)"=="LINUX"

#.ENDIF

.ENDIF

# --- Targets ------------------------------------------------------

.IF "$(ALWAYSDBG_FLAG)"==""
TARGETDEPS+=$(ALWAYSDBGTARGET)
.ENDIF

.INCLUDE :  target.mk

.IF "$(ALWAYSDBGTARGET)" != ""
.IF "$(ALWAYSDBG_FLAG)" == ""
# --------------------------------------------------
# - ALWAYSDBG - files always compiled with debugging
# --------------------------------------------------
$(ALWAYSDBGTARGET):
	@+echo --- ALWAYSDBGFILES ---
	@dmake $(MFLAGS) $(MAKEFILE) debug=true $(ALWAYSDBGFILES) ALWAYSDBG_FLAG=TRUE $(CALLMACROS)
	@+echo --- ALWAYSDBGFILES OVER ---

$(ALWAYSDBGFILES):
	@+echo --- ALWAYSDBG ---
	@dmake $(MFLAGS) $(MAKEFILE) debug=true ALWAYSDBG_FLAG=TRUE $(CALLMACROS) $@
	@+echo --- ALWAYSDBG OVER ---
.ENDIF
.ENDIF


