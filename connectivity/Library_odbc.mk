# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
# 
#   Modified October 2016 by Patrick Luby. NeoOffice is only distributed
#   under the GNU General Public License, Version 3 as allowed by Section 3.3
#   of the Mozilla Public License, v. 2.0.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

$(eval $(call gb_Library_Library,odbc))

$(eval $(call gb_Library_set_componentfile,odbc,connectivity/source/drivers/odbc/odbc))

$(eval $(call gb_Library_use_sdk_api,odbc))

$(eval $(call gb_Library_set_include,odbc,\
	$$(INCLUDE) \
	-I$(SRCDIR)/connectivity/inc \
	-I$(SRCDIR)/connectivity/source/inc \
	-I$(WORKDIR)/YaccTarget/connectivity/source/parse \
))

$(eval $(call gb_Library_add_defs,odbc,\
	-DOOO_DLLIMPLEMENTATION_ODBCBASE \
	$(if $(and $(filter MACOSX,$(OS)), $(if $(SYSTEM_ODBC_HEADERS),,TRUE)), \
		-DSQL_WCHART_CONVERT) \
))

$(eval $(call gb_Library_set_precompiled_header,odbc,$(SRCDIR)/connectivity/inc/pch/precompiled_odbc))

$(eval $(call gb_Library_use_externals,odbc,\
	boost_headers \
	odbc_headers \
))

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Library_use_system_darwin_frameworks,odbc,\
	CoreFoundation \
))
endif	# PRODUCT_BUILD_TYPE == java

$(eval $(call gb_Library_use_libraries,odbc,\
	cppu \
	cppuhelper \
	comphelper \
	dbtools \
	sal \
	salhelper \
	$(gb_UWINAPI) \
))

$(eval $(call gb_Library_add_exception_objects,odbc,\
	connectivity/source/drivers/odbc/appendsqlwchars \
	connectivity/source/drivers/odbc/oservices \
	connectivity/source/drivers/odbc/ORealDriver \
	connectivity/source/drivers/odbc/OFunctions \
	connectivity/source/drivers/odbc/OPreparedStatement \
	connectivity/source/drivers/odbc/OStatement \
	connectivity/source/drivers/odbc/OResultSetMetaData \
	connectivity/source/drivers/odbc/OResultSet \
	connectivity/source/drivers/odbc/OTools \
	connectivity/source/drivers/odbc/ODatabaseMetaDataResultSet \
	connectivity/source/drivers/odbc/ODatabaseMetaData \
	connectivity/source/drivers/odbc/ODriver \
	connectivity/source/drivers/odbc/OConnection \
))

# vim: set noet sw=4 ts=4:
