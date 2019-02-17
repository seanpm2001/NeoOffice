# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#*************************************************************************
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
#*************************************************************************

$(eval $(call gb_CppunitTest_CppunitTest,sc_ucalc))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_ucalc, \
    sc/qa/unit/ucalc \
    sc/qa/unit/ucalc_column \
    sc/qa/unit/ucalc_formula \
    sc/qa/unit/ucalc_pivottable \
    sc/qa/unit/ucalc_sharedformula \
    sc/qa/unit/ucalc_sort \
))

$(eval $(call gb_CppunitTest_use_library_objects,sc_ucalc, \
	sc \
	scqahelper \
))

ifeq ($(ENABLE_TELEPATHY),TRUE)
$(eval $(call gb_CppunitTest_use_libraries,sc_ucalc,tubes))
endif

$(eval $(call gb_CppunitTest_use_externals,sc_ucalc,\
	boost_headers \
    icu_headers \
    icui18n \
    icuuc \
	libxml2 \
	mdds_headers \
	orcus \
	orcus-parser \
))

$(eval $(call gb_CppunitTest_use_libraries,sc_ucalc, \
	$(call gb_Helper_optional,AVMEDIA,avmedia) \
    basegfx \
    $(call gb_Helper_optional,OPENCL, \
        clew) \
    comphelper \
    cppu \
    cppuhelper \
    drawinglayer \
    editeng \
    for \
    forui \
    i18nlangtag \
    i18nutil \
	$(call gb_Helper_optional,OPENCL, \
		opencl) \
    sal \
    salhelper \
    sax \
    sb \
    sfx \
    sot \
    svl \
    svt \
    svx \
    svxcore \
	test \
    tk \
    tl \
    ucbhelper \
	unotest \
    utl \
    vbahelper \
    vcl \
    xo \
	$(gb_UWINAPI) \
))

$(eval $(call gb_CppunitTest_set_include,sc_ucalc,\
    -I$(SRCDIR)/sc/source/ui/inc \
    -I$(SRCDIR)/sc/source/core/inc \
    -I$(SRCDIR)/sc/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_api,sc_ucalc,\
    offapi \
    udkapi \
))

$(eval $(call gb_CppunitTest_use_ure,sc_ucalc))
$(eval $(call gb_CppunitTest_use_vcl,sc_ucalc))

$(eval $(call gb_CppunitTest_use_components,sc_ucalc,\
    configmgr/source/configmgr \
    framework/util/fwk \
    i18npool/source/search/i18nsearch \
    i18npool/util/i18npool \
    sax/source/expatwrap/expwrap \
    scaddins/source/analysis/analysis \
    scaddins/source/datefunc/date \
    scaddins/source/pricing/pricing \
    sfx2/util/sfx \
    ucb/source/core/ucb1 \
    ucb/source/ucp/file/ucpfile1 \
    unoxml/source/service/unoxml \
    uui/util/uui \
))

ifeq ($(OS),LINUX)
$(eval $(call gb_CppunitTest_add_libs,sc_ucalc,\
     -lrt \
))
endif

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_CppunitTest_use_system_darwin_frameworks,sc_ucalc,\
    CoreFoundation \
))
endif	# GUIBASE == java

$(eval $(call gb_CppunitTest_use_configuration,sc_ucalc))

# vim: set noet sw=4 ts=4:
