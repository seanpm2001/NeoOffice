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
#   Patrick Luby, June 2003
# 
#   GNU General Public License Version 2.1
#   =============================================
#   Copyright 2003 by Patrick Luby (patrick.luby@planamesa.com)
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
##########################################################################

# Set the shell to tcsh since the OpenOffice.org build requires it
SHELL:=/bin/tcsh

# Build location macros
BUILD_HOME:=build
INSTALL_HOME:=install
SOURCE_HOME:=source
OO_PATCHES_HOME:=patches/openoffice
OO_ENV_X11:=$(BUILD_HOME)/MacosxEnv.Set
OO_ENV_JAVA:=$(BUILD_HOME)/MacosxEnvJava.Set
OO_LANGUAGES=ALL

# Product information
PRODUCT_NAME=NeoOffice/J
PRODUCT_DIR_NAME=NeoOfficeJ
# Important: Note that there are escape characters in the PRODUCT_NAME for the
# UTF-8 trademark symbol. Don't replace these with "\x##" literal strings!
PRODUCT_TRADEMARKED_NAME=NeoOffice™/J
PRODUCT_VERSION=0.0.1
PRODUCT_FILETYPE=no%f

# CVS macros
OO_CVSROOT:=:pserver:anoncvs@anoncvs.services.openoffice.org:/cvs
OO_PACKAGE:=all
OO_TAG:=OOO_STABLE_1_PORTS
NEO_CVSROOT:=:pserver:anoncvs@anoncvs.neooffice.org:/cvs
NEO_PACKAGE:=NeoOfficeJ
NEO_TAG:=NeoOfficeJ-0_0_1

all: build.all

# Create the build directory and checkout the OpenOffice.org sources
build.oo_checkout:
	mkdir -p "$(BUILD_HOME)"
	cd "$(BUILD_HOME)" ; cvs -d "$(OO_CVSROOT)" co -r "$(OO_TAG)" "$(OO_PACKAGE)"
	chmod -Rf u+w "$(BUILD_HOME)"
	touch "$@"

build.oo_patches: build.oo_checkout \
	build.oo_external_patch
	touch "$@"

build.oo_external_patch: build.oo_checkout
	chmod -Rf u+w "$(BUILD_HOME)/external/gpc"
	gnutar zxf "$(OO_PATCHES_HOME)/gpc231.tar.Z" -C "$(BUILD_HOME)/external/gpc"
	chmod -Rf u+w "$(BUILD_HOME)/external/gpc"
	mv -f "$(BUILD_HOME)/external/gpc/gpc231"/* "$(BUILD_HOME)/external/gpc"
	rm -Rf "$(BUILD_HOME)/external/gpc/gpc231"
	-( cd "$(BUILD_HOME)/external" ; patch -R -p0 -N -r "$(PWD)/patch.rej" ) < "$(OO_PATCHES_HOME)/external.patch"
	cp "$(OO_PATCHES_HOME)/dlcompat-20020709.tar.gz" "$(BUILD_HOME)/external/download"
	cp "$(OO_PATCHES_HOME)/dlcompat.pat.tar.gz" "$(BUILD_HOME)/external/dlcompat"
	( cd "$(BUILD_HOME)/external" ; patch -p0 -N -r "$(PWD)/patch.rej" ) < "$(OO_PATCHES_HOME)/external.patch"
	touch "$@"

build.oo_%_patch: $(BUILD_HOME)/% build.oo_checkout
	-( cd "$<" ; patch -R -p0 -N -r "$(PWD)/patch.rej" ) < "$(OO_PATCHES_HOME)/"`basename "$<"`".patch"
	( cd "$<" ; patch -p0 -N -r "$(PWD)/patch.rej" ) < "$(OO_PATCHES_HOME)/"`basename "$<"`".patch"
	touch "$@"

build.configure: build.oo_patches
	cd "$(BUILD_HOME)/config_office" ; autoconf
	( cd "$(BUILD_HOME)/config_office" ; ./configure CC=cc --with-x --with-lang="$(OO_LANGUAGES)" )
	rm -f "$(OO_ENV_JAVA)"
	sed 's#^setenv GUIBASE .*$$#setenv GUIBASE "java"#' "$(OO_ENV_X11)" | sed 's#^setenv ENVCDEFS "#&-DUSE_JAVA#' | sed 's#^setenv CLASSPATH .*$$#setenv CLASSPATH "$$SOLARVER/$$UPD/$$INPATH/bin/vcl.jar"#' | sed 's#^setenv DELIVER .*$$#setenv DELIVER "true"#' | sed 's#^alias deliver .*$$#alias deliver "echo The deliver command has been disabled"#' > "$(OO_ENV_JAVA)"
	echo "setenv PRODUCT_NAME '$(PRODUCT_NAME)'" >> "$(OO_ENV_JAVA)"
	echo "setenv PRODUCT_DIR_NAME '$(PRODUCT_DIR_NAME)'" >> "$(OO_ENV_JAVA)"
	echo "setenv PRODUCT_TRADEMARKED_NAME '$(PRODUCT_TRADEMARKED_NAME)'" >> "$(OO_ENV_JAVA)"
	echo "setenv PRODUCT_VERSION '$(PRODUCT_VERSION)'" >> "$(OO_ENV_JAVA)"
	echo "setenv PRODUCT_FILETYPE '$(PRODUCT_FILETYPE)'" >> "$(OO_ENV_JAVA)"
	( cd "$(BUILD_HOME)" ; ./bootstrap )
	touch "$@"

build.oo_all: build.configure
	source "$(OO_ENV_X11)" ; cd "$(BUILD_HOME)/instsetoo" ; `alias build` -all $(OO_BUILD_ARGS)
	touch "$@"

build.neo_%_patch: % build.oo_all
	cd "$<" ; sh -e -c 'for i in `cd "$(PWD)/$(BUILD_HOME)/$<" ; find . -type d | grep -v /CVS$$ | grep -v /unxmacxp.pro` ; do mkdir -p "$$i" ; done'
	cd "$<" ; sh -e -c 'for i in `cd "$(PWD)/$(BUILD_HOME)/$<" ; find . ! -type d | grep -v /CVS/ | grep -v /unxmacxp.pro` ; do if [ ! -f "$$i" ] ; then ln -sf "$(PWD)/$(BUILD_HOME)/$</$$i" "$$i" 2>/dev/null ; fi ; done'
	sh -e -c 'if [ ! -d "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro.oo" -a -d "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro" ] ; then rm -Rf "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro.oo" ; mv -f "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro" "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro.oo" ; fi'
	rm -Rf "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro"
	mkdir -p "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro"
	cd "$<" ; ln -sf "$(PWD)/$(BUILD_HOME)/$</unxmacxp.pro"
	source "$(OO_ENV_JAVA)" ; cd "$<" ; `alias build` $(NEO_BUILD_ARGS)
	touch "$@"

build.neo_patches: \
	build.neo_desktop_patch \
	build.neo_dtrans_patch \
	build.neo_forms_patch \
	build.neo_readlicense_patch \
	build.neo_offmgr_patch \
	build.neo_sal_patch \
	build.neo_setup2_patch \
	build.neo_sfx2_patch \
	build.neo_svtools_patch \
	build.neo_sysui_patch \
	build.neo_toolkit_patch \
	build.neo_vcl_patch \
	build.neo_instsetoo_patch
	touch "$@"

build.package: build.neo_patches
	if ( -d "$(INSTALL_HOME)" ) chmod -Rf u+rw "$(INSTALL_HOME)"
	rm -Rf "$(INSTALL_HOME)"
	mkdir -p "$(INSTALL_HOME)/package"
	echo `source "$(OO_ENV_JAVA)" ; cd "instsetoo/util" ; dmake language_numbers` > "$(INSTALL_HOME)/language_numbers"
	echo `source "$(OO_ENV_JAVA)" ; cd "instsetoo/util" ; dmake language_names` > "$(INSTALL_HOME)/language_names"
	echo "[ENVIRONMENT]" > "$(INSTALL_HOME)/response"
	echo "INSTALLATIONMODE=INSTALL_NETWORK" >> "$(INSTALL_HOME)/response"
	echo "INSTALLATIONTYPE=STANDARD" >> "$(INSTALL_HOME)/response"
	echo "DESTINATIONPATH=$(PWD)/$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" >> "$(INSTALL_HOME)/response"
	echo "OUTERPATH=" >> "$(INSTALL_HOME)/response"
	echo "LOGFILE=" >> "$(INSTALL_HOME)/response"
	echo "LANGUAGELIST="`cat "$(INSTALL_HOME)/language_numbers"` >> "$(INSTALL_HOME)/response"
	echo "[JAVA]" >> "$(INSTALL_HOME)/response"
	echo "JavaSupport=preinstalled_or_none" >> "$(INSTALL_HOME)/response"
# Eliminate duplicate help directories since only English is available
	mkdir -p "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents/help/en"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents/help" ; sh -e -c 'for i in `cat "$(PWD)/$(INSTALL_HOME)/language_names"` ; do ln -s "en" "$${i}" ; done'
	source "$(OO_ENV_JAVA)" ; "$(BUILD_HOME)/instsetoo/unxmacxp.pro/"`cat "$(INSTALL_HOME)/language_numbers"`"/normal/setup" -v "-r:$(PWD)/$(INSTALL_HOME)/response"
	source "$(OO_ENV_JAVA)" ; cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/dtrans/unxmacxp.pro/lib/libdtransjava$${UPD}$${DLLSUFFIX}.dylib" "$(PWD)/$(BUILD_HOME)/forms/unxmacxp.pro/lib/libfrm$${UPD}$${DLLSUFFIX}.dylib" "$(PWD)/$(BUILD_HOME)/sal/unxmacxp.pro/lib/libsalextra_x11osx_mxp.dylib" "$(PWD)/$(BUILD_HOME)/sfx2/unxmacxp.pro/lib/libsfx$${UPD}$${DLLSUFFIX}.dylib" "$(PWD)/$(BUILD_HOME)/svtools/unxmacxp.pro/lib/libsvt$${UPD}$${DLLSUFFIX}.dylib" "$(PWD)/$(BUILD_HOME)/toolkit/unxmacxp.pro/lib/libtk$${UPD}$${DLLSUFFIX}.dylib" "$(PWD)/$(BUILD_HOME)/vcl/unxmacxp.pro/lib/libvcl$${UPD}$${DLLSUFFIX}.dylib" "program"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/readlicense/source/license/unx/LICENSE" "$(PWD)/$(BUILD_HOME)/readlicense/source/readme/unxmacxp/README" "."
	source "$(OO_ENV_JAVA)" ; cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; sh -e -c 'for i in `cat "$(PWD)/$(INSTALL_HOME)/language_numbers" | sed "s#,# #g"` ; do cp "$(PWD)/$(BUILD_HOME)/offmgr/unxmacxp.pro/bin/neojava$${UPD}$${i}.res" "program/resource/iso$${UPD}$${i}.res" ; done'
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/desktop/unxmacxp.pro/bin/soffice" "program/soffice.bin" ; chmod a+x "program/soffice.bin"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/sysui/unxmacxp.pro/misc/soffice.sh" "program/soffice" ; chmod a+x "program/soffice"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/setup2/unxmacxp.pro/misc/setup.sh" "program/setup" ; chmod a+x "program/setup"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/sysui/unxmacxp.pro/misc/nswrapper.sh" "program/nswrapper" ; chmod a+x "program/nswrapper"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/sysui/unxmacxp.pro/misc/Info.plist" "."
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/sysui/unxmacxp.pro/misc/PkgInfo" "."
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/vcl/unxmacxp.pro/class/vcl.jar" "program/classes"
	rm -Rf "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents/Resources"
	mkdir -p "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents/Resources"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; cp "$(PWD)/$(BUILD_HOME)/sysui/unxmacxp.pro/misc/ship.icns" "Resources"
	source "$(OO_ENV_JAVA)" ; cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents/program" ; regcomp -revoke -r applicat.rdb -c "libdtransX11$${UPD}$${DLLSUFFIX}.dylib"
	source "$(OO_ENV_JAVA)" ; cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents/program" ; regcomp -register -r applicat.rdb -c "libdtransjava$${UPD}$${DLLSUFFIX}.dylib"
	source "$(OO_ENV_JAVA)" ; cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; rm -Rf "license.html" "readme.html" "spadmin" "setup" "program/libdtransX11$${UPD}$${DLLSUFFIX}.dylib" "program/libpsp$${UPD}$${DLLSUFFIX}.dylib" "program/libspa$${UPD}$${DLLSUFFIX}.dylib" "program/setup.bin" "program/spadmin" "program/spadmin.bin"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; sed 's#ProductPatch=.*$$#ProductPatch=($(PRODUCT_VERSION))#' "program/bootstraprc" | sed '/Location=.*$$/d' | sed 's#UserInstallation=.*$$#UserInstallation=$$SYSUSERCONFIG/Library/$(PRODUCT_DIR_NAME)#' | sed 's#ProductKey=.*$$#ProductKey=$(PRODUCT_NAME) $(PRODUCT_VERSION)#' > "../../../out" ; mv -f "../../../out" "program/bootstraprc"
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; rm -Rf "share/config/registry/cache" ; sh -e -c 'for i in "share/config/registry/instance/org/openoffice/Setup.xml" "share/config/registry/instance/org/openoffice/Office/Common.xml" ; do sed "s#\"string\">.*</ooName>#\"string\">$(PRODUCT_NAME)</ooName>#g" "$${i}" | sed "s#\"string\">.*</ooSetupVersion>#\"string\">$(PRODUCT_VERSION)</ooSetupVersion>#g" | sed "s#$(PWD)/$(INSTALL_HOME)/package#/Applications#g" | sed "s#>OpenOffice\.org [0-9\.]* #>$(PRODUCT_NAME) $(PRODUCT_VERSION) #g" | sed "s#/work#/../../../Documents#g" > "../../../out" ; mv -f "../../../out" "$${i}" ; done'
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; sh -e -c 'if [ ! -d "MacOS" ] ; then rm -Rf "MacOS" ; mv -f "program" "MacOS" ; ln -s "MacOS" "program" ; fi'
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; sh -e -c 'for i in `cd "$(PWD)/etc" ; find . -type d | grep -v /CVS$$` ; do mkdir -p "$$i" ; done'
	cd "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app/Contents" ; sh -e -c 'for i in `cd "$(PWD)/etc" ; find . ! -type d | grep -v /CVS/` ; do cp "$(PWD)/etc/$${i}" "$${i}" ; done'
	chmod -Rf a-w,a+r "$(INSTALL_HOME)/package/$(PRODUCT_DIR_NAME).app"
	sed 's#$$(PRODUCT_NAME)#$(PRODUCT_NAME)#g' etc/neojava.info | sed 's#$$(PRODUCT_VERSION)#$(PRODUCT_VERSION)#g' > "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).info"
	/usr/bin/package "$(INSTALL_HOME)/package" "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).info" -d "$(INSTALL_HOME)"
	sh -e -c 'if [ ! -d "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg/Contents/Resources" ] ; then mv "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg" "$(INSTALL_HOME)/Resources" ; mkdir -p "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg/Contents" ; mv "$(INSTALL_HOME)/Resources" "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg/Contents" ; fi'
	cp "etc/gpl.html" "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg/Contents/Resources/License.html"
	cd "bin" ; sh -e -c 'for i in preflight postflight ; do sed "s#\$$(PRODUCT_DIR_NAME)#$(PRODUCT_DIR_NAME)#g" "$${i}" > "$(PWD)/$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg/Contents/Resources/$${i}" ; chmod 755 "$(PWD)/$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg/Contents/Resources/$${i}" ; done'
	chmod -Rf u+w,og-w,a+r "$(INSTALL_HOME)/$(PRODUCT_DIR_NAME).pkg"
	touch "$@"

build.source_zip:
	$(RM) -Rf "$(SOURCE_HOME)"
	mkdir -p "$(SOURCE_HOME)/$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION)"
	cd "$(SOURCE_HOME)/$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION)" ; cvs -d "$(NEO_CVSROOT)" co -r "$(NEO_TAG)" "$(NEO_PACKAGE)"
# Prune out empty directories
	cd "$(SOURCE_HOME)/$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION)" ; sh -e -c 'for i in `ls -1`; do cd "$${i}" ; cvs update -P -d ; done'
	cp "$(SOURCE_HOME)/$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION)/neojava/etc/gpl.html" "$(SOURCE_HOME)/$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION)/LICENSE.html"
	chmod -Rf u+w,og-w,a+r "$(SOURCE_HOME)/$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION)"
	cd "$(SOURCE_HOME)" ; gnutar zcf "$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION).src.tar.gz" "$(PRODUCT_DIR_NAME)-$(PRODUCT_VERSION)"
	touch "$@"

build.all: build.oo_all build.package build.source_zip
	touch "$@"
