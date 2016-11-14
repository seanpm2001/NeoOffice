# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
# 
#   Modified November 2016 by Patrick Luby. NeoOffice is only distributed
#   under the GNU General Public License, Version 3 as allowed by Section 3.3
#   of the Mozilla Public License, v. 2.0.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

$(eval $(call gb_Library_Library,jvmfwk))

$(eval $(call gb_Library_add_defs,jvmfwk,\
    -DJVMFWK_DLLIMPLEMENTATION \
))

ifneq ($(JVM_ONE_PATH_CHECK),)
$(eval $(call gb_Library_add_defs,jvmfwk,\
    -DJVM_ONE_PATH_CHECK=\"$(JVM_ONE_PATH_CHECK)\" \
))
endif

$(eval $(call gb_Library_set_include,jvmfwk,\
    -I$(SRCDIR)/jvmfwk/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_Library_use_api,jvmfwk,\
    udkapi \
))

$(eval $(call gb_Library_use_libraries,jvmfwk,\
    cppu \
    cppuhelper \
    sal \
    salhelper \
	$(gb_UWINAPI) \
))

ifeq ($(OS),ANDROID)
$(eval $(call gb_Library_use_libraries,jvmfwk,\
    lo-bootstrap \
))
endif

ifeq ($(OS),WNT)
$(eval $(call gb_Library_use_system_win32_libs,jvmfwk,\
	advapi32 \
))
endif

$(eval $(call gb_Library_use_externals,jvmfwk,\
    boost_headers \
    libxml2 \
    valgrind \
))

$(eval $(call gb_Library_add_exception_objects,jvmfwk,\
    jvmfwk/plugins/sunmajor/pluginlib/gnujre \
    jvmfwk/plugins/sunmajor/pluginlib/otherjre \
    jvmfwk/plugins/sunmajor/pluginlib/sunjavaplugin \
    jvmfwk/plugins/sunmajor/pluginlib/sunjre \
    jvmfwk/plugins/sunmajor/pluginlib/sunversion \
    jvmfwk/plugins/sunmajor/pluginlib/util \
    jvmfwk/plugins/sunmajor/pluginlib/vendorbase \
    jvmfwk/plugins/sunmajor/pluginlib/vendorlist \
    jvmfwk/source/elements \
    jvmfwk/source/framework \
    jvmfwk/source/fwkbase \
    jvmfwk/source/fwkutil \
    jvmfwk/source/libxmlutil \
))

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Library_add_objcxxobjects,jvmfwk,\
    jvmfwk/plugins/sunmajor/pluginlib/util_cocoa \
))

$(eval $(call gb_Library_use_system_darwin_frameworks,jvmfwk,\
    Foundation \
))
endif	# GUIBASE == java

# vim:set noet sw=4 ts=4:
