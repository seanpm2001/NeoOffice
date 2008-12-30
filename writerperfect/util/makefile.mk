PRJ=..
PRJNAME=writerperfect
TARGET=writerperfect

.INCLUDE :  settings.mk


.IF "$(GUIBASE)"=="java"
SOLARLIB := -L$(PRJ)$/..$/libwpd$/$(ROUT)$/lib $(SOLARLIB)
.ENDIF		# "$(GUIBASE)"=="java"

.IF "$(GUI)"=="UNX" || "$(GUI)$(COM)"=="WNTGCC"

.IF "$(SYSTEM_LIBWPD)" == "YES"
LIBWPD=$(LIBWPD_LIBS)
.ELSE
LIBWPD=-lwpdlib
.ENDIF

.IF "$(SYSTEM_LIBWPS)" == "YES"
LIBWPS=$(LIBWPS_LIBS)
.ELSE
LIBWPS=-lwpslib
.ENDIF

.IF "$(SYSTEM_LIBWPG)" == "YES"
LIBWPG=$(LIBWPG_LIBS)
.ELSE
LIBWPG=-lwpglib
.ENDIF

.ELSE

LIBWPD=$(LIBPRE) wpdlib.lib
LIBWPS=$(LIBPRE) wpslib.lib
LIBWPG=$(LIBPRE) wpglib.lib

.ENDIF

LIB1TARGET= $(SLB)$/wpft.lib
LIB1FILES= \
	$(SLB)$/stream.lib  \
	$(SLB)$/filter.lib  \
	$(SLB)$/wpdimp.lib
SHL1LIBS=$(LIB1TARGET)
SHL1STDLIBS+= \
	$(SVLLIB)	\
	$(SOTLIB) \
	$(SO2LIB) \
	$(SVTOOLLIB) \
	$(UNOTOOLSLIB) \
	$(TOOLSLIB) \
	$(COMPHELPERLIB) \
	$(UCBHELPERLIB) \
	$(CPPUHELPERLIB) \
	$(CPPULIB) \
	$(SALLIB) \
	$(XMLOFFLIB) \
	$(LIBWPD)

SHL1TARGET = wpft$(DLLPOSTFIX)
SHL1IMPLIB = i$(SHL1TARGET)
SHL1LIBS = $(LIB1TARGET)
SHL1VERSIONMAP=wpft.map
DEF1NAME=$(SHL1TARGET)


LIB2TARGET= $(SLB)$/msworks.lib
LIB2FILES= \
	$(SLB)$/stream.lib  \
	$(SLB)$/filter.lib  \
	$(SLB)$/wpsimp.lib
SHL2LIBS=$(LIB2TARGET)
SHL2STDLIBS+= \
	$(SVLLIB)	\
	$(SOTLIB) \
	$(SO2LIB) \
	$(SVTOOLLIB) \
	$(UNOTOOLSLIB) \
	$(TOOLSLIB) \
	$(COMPHELPERLIB) \
	$(UCBHELPERLIB) \
	$(CPPUHELPERLIB) \
	$(CPPULIB) \
	$(SALLIB) \
	$(XMLOFFLIB) \
	$(LIBWPS) \
	$(LIBWPD)

SHL2TARGET = msworks$(DLLPOSTFIX)
SHL2IMPLIB = i$(SHL2TARGET)
SHL2LIBS = $(LIB2TARGET)
SHL2VERSIONMAP=msworks.map
DEF2NAME=$(SHL2TARGET)

LIB3TARGET= $(SLB)$/wpgimport.lib
LIB3FILES= \
	$(SLB)$/stream.lib  \
	$(SLB)$/filter.lib  \
	$(SLB)$/wpgimp.lib
SHL3LIBS=$(LIB3TARGET)
SHL3STDLIBS+= \
	$(SVLLIB)	\
	$(SOTLIB) \
	$(SO2LIB) \
	$(SVTOOLLIB) \
	$(UNOTOOLSLIB) \
	$(TOOLSLIB) \
	$(COMPHELPERLIB) \
	$(UCBHELPERLIB) \
	$(CPPUHELPERLIB) \
	$(CPPULIB) \
	$(SALLIB) \
	$(XMLOFFLIB) \
	$(LIBWPD) \
	$(LIBWPG)

SHL3TARGET = wpgimport$(DLLPOSTFIX)
SHL3IMPLIB = i$(SHL3TARGET)
SHL3LIBS = $(LIB3TARGET)
SHL3VERSIONMAP=wpgimport.map
DEF3NAME=$(SHL3TARGET)

.INCLUDE :  target.mk
