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
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
# 
#   Modified December 2016 by Patrick Luby. NeoOffice is only distributed
#   under the GNU General Public License, Version 3 as allowed by Section 3.3
#   of the Mozilla Public License, v. 2.0.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

$(eval $(call gb_Library_Library,sw))

$(eval $(call gb_Library_add_sdi_headers,sw,sw/sdi/swslots))

$(eval $(call gb_Library_set_componentfile,sw,sw/util/sw))

$(eval $(call gb_Library_set_precompiled_header,sw,$(SRCDIR)/sw/inc/pch/precompiled_sw))

$(eval $(call gb_Library_set_include,sw,\
    -I$(SRCDIR)/sw/source/core/inc \
    -I$(SRCDIR)/sw/source/filter/inc \
    -I$(SRCDIR)/sw/source/uibase/inc \
    -I$(SRCDIR)/sw/inc \
    -I$(WORKDIR)/SdiTarget/sw/sdi \
    $$(INCLUDE) \
))

$(eval $(call gb_Library_use_custom_headers,sw,\
	officecfg/registry \
))

$(eval $(call gb_Library_use_sdk_api,sw))

$(eval $(call gb_Library_add_defs,sw,\
    -DSW_DLLIMPLEMENTATION \
	-DSWUI_DLL_NAME=\"$(call gb_Library_get_runtime_filename,$(call gb_Library__get_name,swui))\" \
	-DDBTOOLS_DLL_NAME=\"$(call gb_Library_get_runtime_filename,$(call gb_Library__get_name,dbtools))\" \
))

$(eval $(call gb_Library_use_libraries,sw,\
    $(call gb_Helper_optional,AVMEDIA,avmedia) \
    basegfx \
    comphelper \
    cppu \
    cppuhelper \
    drawinglayer \
    editeng \
    i18nlangtag \
    i18nutil \
    lng \
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
    tk \
    tl \
    ucbhelper \
    utl \
    $(call gb_Helper_optional,SCRIPTING, \
        vbahelper) \
    vcl \
    xmlreader \
    xo \
	$(gb_UWINAPI) \
))

$(eval $(call gb_Library_use_externals,sw,\
	boost_headers \
	icuuc \
	icu_headers \
	libxml2 \
))

$(eval $(call gb_Library_add_exception_objects,sw,\
    sw/source/core/SwNumberTree/SwNodeNum \
    sw/source/core/SwNumberTree/SwNumberTree \
    sw/source/core/access/acccell \
    sw/source/core/access/acccontext \
    sw/source/core/access/accdoc \
    sw/source/core/access/accembedded \
    sw/source/core/access/accfootnote \
    sw/source/core/access/accframe \
    sw/source/core/access/accframebase\
    sw/source/core/access/accfrmobj \
    sw/source/core/access/accfrmobjmap \
    sw/source/core/access/accfrmobjslist \
    sw/source/core/access/accgraphic \
    sw/source/core/access/accheaderfooter \
    sw/source/core/access/acchyperlink \
    sw/source/core/access/acchypertextdata \
    sw/source/core/access/accmap \
    sw/source/core/access/accnotextframe \
    sw/source/core/access/accnotexthyperlink \
    sw/source/core/access/accpage \
    sw/source/core/access/accpara \
    sw/source/core/access/accportions \
    sw/source/core/access/accpreview \
    sw/source/core/access/accselectionhelper \
    sw/source/core/access/acctable \
    sw/source/core/access/acctextframe \
    sw/source/core/access/parachangetrackinginfo \
    sw/source/core/access/textmarkuphelper \
    sw/source/core/attr/calbck \
    sw/source/core/attr/cellatr \
    sw/source/core/attr/fmtfollowtextflow \
    sw/source/core/attr/fmtwrapinfluenceonobjpos \
    sw/source/core/attr/format \
    sw/source/core/attr/hints \
    sw/source/core/attr/swatrset \
    sw/source/core/bastyp/SwSmartTagMgr \
    sw/source/core/bastyp/bparr \
    sw/source/core/bastyp/breakit \
    sw/source/core/bastyp/calc \
    sw/source/core/bastyp/checkit \
    sw/source/core/bastyp/index \
    sw/source/core/bastyp/init \
    sw/source/core/bastyp/ring \
    sw/source/core/bastyp/swcache \
    sw/source/core/bastyp/swrect \
    sw/source/core/bastyp/swregion \
    sw/source/core/bastyp/swtypes \
    sw/source/core/bastyp/tabcol \
    sw/source/core/crsr/annotationmark \
    sw/source/core/crsr/BlockCursor \
    sw/source/core/crsr/bookmrk \
    sw/source/core/crsr/callnk \
    sw/source/core/crsr/crbm \
    sw/source/core/crsr/crossrefbookmark \
    sw/source/core/crsr/crsrsh \
    sw/source/core/crsr/crstrvl \
    sw/source/core/crsr/crstrvl1 \
    sw/source/core/crsr/findattr \
    sw/source/core/crsr/findcoll \
    sw/source/core/crsr/findfmt \
    sw/source/core/crsr/findtxt \
    sw/source/core/crsr/pam \
    sw/source/core/crsr/paminit \
    sw/source/core/crsr/swcrsr \
    sw/source/core/crsr/trvlcol \
    sw/source/core/crsr/trvlfnfl \
    sw/source/core/crsr/trvlreg \
    sw/source/core/crsr/trvltbl \
    sw/source/core/crsr/unocrsr \
    sw/source/core/crsr/viscrs \
    sw/source/core/crsr/overlayrangesoutline \
    sw/source/core/doc/SwStyleNameMapper \
    sw/source/core/doc/acmplwrd \
    sw/source/core/doc/CntntIdxStore \
    sw/source/core/doc/dbgoutsw \
    sw/source/core/doc/doc \
    sw/source/core/doc/docbasic \
    sw/source/core/doc/docbm \
    sw/source/core/doc/docchart \
    sw/source/core/doc/doccomp \
    sw/source/core/doc/doccorr \
    sw/source/core/doc/docdesc \
    sw/source/core/doc/docdraw \
    sw/source/core/doc/docedt \
    sw/source/core/doc/docfld \
    sw/source/core/doc/docfly \
    sw/source/core/doc/docfmt \
    sw/source/core/doc/docftn \
    sw/source/core/doc/docglbl \
    sw/source/core/doc/docglos \
    sw/source/core/doc/doclay \
    sw/source/core/doc/docnew \
    sw/source/core/doc/docnum \
    sw/source/core/doc/docredln \
    sw/source/core/doc/docruby \
    sw/source/core/doc/docsort \
    sw/source/core/doc/docstat \
    sw/source/core/doc/doctxm \
    sw/source/core/doc/DocumentDeviceManager \
    sw/source/core/doc/docxforms \
    sw/source/core/doc/DocumentSettingManager \
    sw/source/core/doc/DocumentDrawModelManager \
    sw/source/core/doc/DocumentChartDataProviderManager \
    sw/source/core/doc/DocumentTimerManager \
    sw/source/core/doc/DocumentLinksAdministrationManager \
    sw/source/core/doc/DocumentListItemsManager \
    sw/source/core/doc/DocumentListsManager \
    sw/source/core/doc/DocumentOutlineNodesManager \
    sw/source/core/doc/DocumentContentOperationsManager \
    sw/source/core/doc/DocumentRedlineManager \
    sw/source/core/doc/DocumentFieldsManager \
    sw/source/core/doc/DocumentStatisticsManager \
    sw/source/core/doc/DocumentStateManager \
    sw/source/core/doc/DocumentLayoutManager \
    sw/source/core/doc/DocumentStylePoolManager \
    sw/source/core/doc/DocumentExternalDataManager \
    sw/source/core/doc/extinput \
    sw/source/core/doc/fmtcol \
    sw/source/core/doc/ftnidx \
    sw/source/core/doc/gctable \
    sw/source/core/doc/htmltbl \
    sw/source/core/doc/lineinfo \
    sw/source/core/doc/list \
    sw/source/core/doc/notxtfrm \
    sw/source/core/doc/number \
    sw/source/core/doc/poolfmt \
    sw/source/core/doc/sortopt \
    sw/source/core/doc/swserv \
    sw/source/core/doc/swstylemanager \
    sw/source/core/doc/tblafmt \
    sw/source/core/doc/tblcpy \
    sw/source/core/doc/tblrwcl \
    sw/source/core/doc/textboxhelper \
    sw/source/core/doc/visiturl \
    sw/source/core/docnode/cancellablejob \
    sw/source/core/docnode/finalthreadmanager \
    sw/source/core/docnode/ndcopy \
    sw/source/core/docnode/ndindex \
    sw/source/core/docnode/ndnotxt \
    sw/source/core/docnode/ndnum \
    sw/source/core/docnode/ndsect \
    sw/source/core/docnode/ndtbl \
    sw/source/core/docnode/ndtbl1 \
    sw/source/core/docnode/node \
    sw/source/core/docnode/nodedump \
    sw/source/core/docnode/node2lay \
    sw/source/core/docnode/nodes \
    sw/source/core/docnode/observablethread \
    sw/source/core/docnode/pausethreadstarting \
    sw/source/core/docnode/retrievedinputstreamdata \
    sw/source/core/docnode/retrieveinputstream \
    sw/source/core/docnode/retrieveinputstreamconsumer \
    sw/source/core/docnode/section \
    sw/source/core/docnode/swbaslnk \
    sw/source/core/docnode/swthreadjoiner \
    sw/source/core/docnode/swthreadmanager \
    sw/source/core/docnode/threadlistener \
    sw/source/core/docnode/threadmanager \
    sw/source/core/draw/dcontact \
    sw/source/core/draw/dflyobj \
    sw/source/core/draw/dobjfac \
    sw/source/core/draw/dpage \
    sw/source/core/draw/drawdoc \
    sw/source/core/draw/dview \
    sw/source/core/edit/acorrect \
    sw/source/core/edit/autofmt \
    sw/source/core/edit/edatmisc \
    sw/source/core/edit/edattr \
    sw/source/core/edit/eddel \
    sw/source/core/edit/edfcol \
    sw/source/core/edit/edfld \
    sw/source/core/edit/edfldexp \
    sw/source/core/edit/edfmt \
    sw/source/core/edit/edglbldc \
    sw/source/core/edit/edglss \
    sw/source/core/edit/editsh \
    sw/source/core/edit/edlingu \
    sw/source/core/edit/ednumber \
    sw/source/core/edit/edredln \
    sw/source/core/edit/edsect \
    sw/source/core/edit/edtab \
    sw/source/core/edit/edtox \
    sw/source/core/edit/edundo \
    sw/source/core/edit/edws \
    sw/source/core/fields/authfld \
    sw/source/core/fields/cellfml \
    sw/source/core/fields/chpfld \
    sw/source/core/fields/dbfld \
    sw/source/core/fields/ddefld \
    sw/source/core/fields/ddetbl \
    sw/source/core/fields/docufld \
    sw/source/core/fields/expfld \
    sw/source/core/fields/fldbas \
    sw/source/core/fields/flddat \
    sw/source/core/fields/flddropdown \
    sw/source/core/fields/fldlst \
    sw/source/core/fields/macrofld \
    sw/source/core/fields/postithelper \
    sw/source/core/fields/reffld \
    sw/source/core/fields/scrptfld \
    sw/source/core/fields/tblcalc \
    sw/source/core/fields/textapi \
    sw/source/core/fields/usrfld \
    sw/source/core/frmedt/fecopy \
    sw/source/core/frmedt/fedesc \
    sw/source/core/frmedt/fefly1 \
    sw/source/core/frmedt/feflyole \
    sw/source/core/frmedt/feshview \
    sw/source/core/frmedt/fetab \
    sw/source/core/frmedt/fews \
    sw/source/core/frmedt/tblsel \
    sw/source/core/graphic/grfatr \
    sw/source/core/graphic/ndgrf \
    sw/source/core/layout/anchoreddrawobject \
    sw/source/core/layout/anchoredobject \
    sw/source/core/layout/atrfrm \
    sw/source/core/layout/calcmove \
    sw/source/core/layout/colfrm \
    sw/source/core/layout/dbg_lay \
    sw/source/core/layout/dumpfilter \
    sw/source/core/layout/findfrm \
    sw/source/core/layout/flowfrm \
    sw/source/core/layout/fly \
    sw/source/core/layout/flycnt \
    sw/source/core/layout/flyincnt \
    sw/source/core/layout/flylay \
    sw/source/core/layout/flypos \
    sw/source/core/layout/frmtool \
    sw/source/core/layout/ftnfrm \
    sw/source/core/layout/hffrm \
    sw/source/core/layout/layact \
    sw/source/core/layout/laycache \
    sw/source/core/layout/layouter \
    sw/source/core/layout/movedfwdfrmsbyobjpos \
    sw/source/core/layout/newfrm \
    sw/source/core/layout/objectformatter \
    sw/source/core/layout/objectformatterlayfrm \
    sw/source/core/layout/objectformattertxtfrm \
    sw/source/core/layout/objstmpconsiderwrapinfl \
    sw/source/core/layout/pagechg \
    sw/source/core/layout/pagedesc \
    sw/source/core/layout/paintfrm \
    sw/source/core/layout/sectfrm \
    sw/source/core/layout/softpagebreak \
    sw/source/core/layout/sortedobjs \
    sw/source/core/layout/ssfrm \
    sw/source/core/layout/swselectionlist \
    sw/source/core/layout/tabfrm \
    sw/source/core/layout/trvlfrm \
    sw/source/core/layout/unusedf \
    sw/source/core/layout/virtoutp \
    sw/source/core/layout/wsfrm \
    sw/source/core/objectpositioning/anchoredobjectposition \
    sw/source/core/objectpositioning/ascharanchoredobjectposition \
    sw/source/core/objectpositioning/environmentofanchoredobject \
    sw/source/core/objectpositioning/tocntntanchoredobjectposition \
    sw/source/core/objectpositioning/tolayoutanchoredobjectposition \
    sw/source/core/ole/ndole \
    sw/source/core/para/paratr \
    sw/source/core/sw3io/sw3convert \
    sw/source/core/sw3io/swacorr \
    sw/source/core/swg/SwXMLBlockExport \
    sw/source/core/swg/SwXMLBlockImport \
    sw/source/core/swg/SwXMLSectionList \
    sw/source/core/swg/SwXMLTextBlocks \
    sw/source/core/swg/SwXMLTextBlocks1 \
    sw/source/core/swg/swblocks \
    sw/source/core/table/swnewtable \
    sw/source/core/table/swtable \
    sw/source/core/text/EnhancedPDFExportHelper \
    sw/source/core/text/SwGrammarMarkUp \
    sw/source/core/text/atrstck \
    sw/source/core/text/blink \
    sw/source/core/text/frmcrsr \
    sw/source/core/text/frmform \
    sw/source/core/text/frminf \
    sw/source/core/text/frmpaint \
    sw/source/core/text/guess \
    sw/source/core/text/inftxt \
    sw/source/core/text/itradj \
    sw/source/core/text/itratr \
    sw/source/core/text/itrcrsr \
    sw/source/core/text/itrform2 \
    sw/source/core/text/itrpaint \
    sw/source/core/text/itrtxt \
    sw/source/core/text/noteurl \
    sw/source/core/text/porexp \
    sw/source/core/text/porfld \
    sw/source/core/text/porfly \
    sw/source/core/text/porglue \
    sw/source/core/text/porlay \
    sw/source/core/text/porlin \
    sw/source/core/text/pormulti \
    sw/source/core/text/porref \
    sw/source/core/text/porrst \
    sw/source/core/text/portox \
    sw/source/core/text/portxt \
    sw/source/core/text/redlnitr \
    sw/source/core/text/txtcache \
    sw/source/core/text/txtdrop \
    sw/source/core/text/txtfld \
    sw/source/core/text/txtfly \
    sw/source/core/text/txtfrm \
    sw/source/core/text/txtftn \
    sw/source/core/text/txthyph \
    sw/source/core/text/txtinit \
    sw/source/core/text/txtio \
    sw/source/core/text/txtpaint \
    sw/source/core/text/txttab \
    sw/source/core/text/widorp \
    sw/source/core/text/wrong \
    sw/source/core/text/xmldump \
    sw/source/core/tox/tox \
    sw/source/core/tox/toxhlp \
    sw/source/core/tox/txmsrt \
    sw/source/core/tox/ToxLinkProcessor \
    sw/source/core/tox/ToxTabStopTokenHandler \
    sw/source/core/tox/ToxTextGenerator \
    sw/source/core/tox/ToxWhitespaceStripper \
    sw/source/core/txtnode/SwGrammarContact \
    sw/source/core/txtnode/atrfld \
    sw/source/core/txtnode/atrflyin \
    sw/source/core/txtnode/atrftn \
    sw/source/core/txtnode/atrref \
    sw/source/core/txtnode/atrtox \
    sw/source/core/txtnode/chrfmt \
    sw/source/core/txtnode/fmtatr2 \
    sw/source/core/txtnode/fntcache \
    sw/source/core/txtnode/fntcap \
    sw/source/core/txtnode/modeltoviewhelper \
    sw/source/core/txtnode/ndhints \
    sw/source/core/txtnode/ndtxt \
    sw/source/core/txtnode/swfntcch \
    sw/source/core/txtnode/swfont \
    sw/source/core/txtnode/thints \
    sw/source/core/txtnode/txatbase \
    sw/source/core/txtnode/txatritr \
    sw/source/core/txtnode/txtatr2 \
    sw/source/core/txtnode/txtedt \
    sw/source/core/undo/SwRewriter \
    sw/source/core/undo/SwUndoField \
    sw/source/core/undo/SwUndoFmt \
    sw/source/core/undo/SwUndoPageDesc \
    sw/source/core/undo/SwUndoTOXChange \
    sw/source/core/undo/docundo \
    sw/source/core/undo/rolbck \
    sw/source/core/undo/unattr \
    sw/source/core/undo/unbkmk \
    sw/source/core/undo/undel \
    sw/source/core/undo/undobj \
    sw/source/core/undo/undobj1 \
    sw/source/core/undo/undoflystrattr \
    sw/source/core/undo/undraw \
    sw/source/core/undo/unfmco \
    sw/source/core/undo/unins \
    sw/source/core/undo/unmove \
    sw/source/core/undo/unnum \
    sw/source/core/undo/unoutl \
    sw/source/core/undo/unovwr \
    sw/source/core/undo/unredln \
    sw/source/core/undo/unsect \
    sw/source/core/undo/unsort \
    sw/source/core/undo/unspnd \
    sw/source/core/undo/untbl \
    sw/source/core/undo/untblk \
    sw/source/core/unocore/SwXTextDefaults \
    sw/source/core/unocore/TextCursorHelper  \
    sw/source/core/unocore/XMLRangeHelper \
    sw/source/core/unocore/swunohelper \
    sw/source/core/unocore/unobkm \
    sw/source/core/unocore/unochart \
    sw/source/core/unocore/unocoll \
    sw/source/core/unocore/unocrsrhelper \
    sw/source/core/unocore/unodraw \
    sw/source/core/unocore/unoevent \
    sw/source/core/unocore/unofield \
    sw/source/core/unocore/unoflatpara \
    sw/source/core/unocore/unoframe \
    sw/source/core/unocore/unoftn \
    sw/source/core/unocore/unoidx \
    sw/source/core/unocore/unomap \
    sw/source/core/unocore/unoobj \
    sw/source/core/unocore/unoobj2 \
    sw/source/core/unocore/unoparagraph \
    sw/source/core/unocore/unoport \
    sw/source/core/unocore/unoportenum \
    sw/source/core/unocore/unoredline \
    sw/source/core/unocore/unoredlines \
    sw/source/core/unocore/unorefmk \
    sw/source/core/unocore/unosect \
    sw/source/core/unocore/unosett \
    sw/source/core/unocore/unosrch \
    sw/source/core/unocore/unostyle \
    sw/source/core/unocore/unotbl  \
    sw/source/core/unocore/unotext \
    sw/source/core/unocore/unotextmarkup \
    sw/source/core/view/pagepreviewlayout \
    sw/source/core/view/printdata \
    sw/source/core/view/vdraw \
    sw/source/core/view/viewimp \
    sw/source/core/view/viewpg \
    sw/source/core/view/viewsh \
    sw/source/core/view/vnew \
    sw/source/core/view/vprint \
    sw/source/filter/ascii/ascatr \
    sw/source/filter/ascii/parasc \
    sw/source/filter/ascii/wrtasc \
    sw/source/filter/basflt/docfact \
    sw/source/filter/basflt/fltini \
    sw/source/filter/basflt/fltshell \
    sw/source/filter/basflt/iodetect \
    sw/source/filter/basflt/shellio \
    sw/source/filter/html/SwAppletImpl \
    sw/source/filter/html/css1atr \
    sw/source/filter/html/css1kywd \
    sw/source/filter/html/htmlatr \
    sw/source/filter/html/htmlbas \
    sw/source/filter/html/htmlcss1 \
    sw/source/filter/html/htmlctxt \
    sw/source/filter/html/htmldrawreader \
    sw/source/filter/html/htmldrawwriter \
    sw/source/filter/html/htmlfld \
    sw/source/filter/html/htmlfldw \
    sw/source/filter/html/htmlfly \
    sw/source/filter/html/htmlflywriter \
    sw/source/filter/html/htmlflyt \
    sw/source/filter/html/htmlform \
    sw/source/filter/html/htmlforw \
    sw/source/filter/html/htmlftn \
    sw/source/filter/html/htmlgrin \
    sw/source/filter/html/htmlnum \
    sw/source/filter/html/htmlnumreader \
    sw/source/filter/html/htmlnumwriter \
    sw/source/filter/html/htmlplug \
    sw/source/filter/html/htmlsect \
    sw/source/filter/html/htmltab \
    sw/source/filter/html/htmltabw \
    sw/source/filter/html/parcss1 \
    sw/source/filter/html/svxcss1 \
    sw/source/filter/html/swhtml \
    sw/source/filter/html/wrthtml \
    sw/source/filter/writer/writer \
    sw/source/filter/writer/wrt_fn \
    sw/source/filter/writer/wrtswtbl \
    sw/source/filter/xml/XMLRedlineImportHelper \
    sw/source/filter/xml/swxml \
    sw/source/filter/xml/wrtxml \
    sw/source/filter/xml/xmlbrsh \
    sw/source/filter/xml/xmlexp \
    sw/source/filter/xml/xmlexpit \
    sw/source/filter/xml/xmlfmt \
    sw/source/filter/xml/xmlfmte \
    sw/source/filter/xml/xmlfonte \
    sw/source/filter/xml/xmlimp \
    sw/source/filter/xml/xmlimpit \
    sw/source/filter/xml/xmlitem \
    sw/source/filter/xml/xmliteme \
    sw/source/filter/xml/xmlitemi \
    sw/source/filter/xml/xmlitemm \
    sw/source/filter/xml/xmlithlp \
    sw/source/filter/xml/xmlitmpr \
    sw/source/filter/xml/xmlmeta \
    sw/source/filter/xml/xmlscript \
    sw/source/filter/xml/xmltble \
    sw/source/filter/xml/xmltbli \
    sw/source/filter/xml/xmltext \
    sw/source/filter/xml/xmltexte \
    sw/source/filter/xml/xmltexti \
    sw/source/uibase/app/appenv \
    sw/source/uibase/app/apphdl \
    sw/source/uibase/app/applab \
    sw/source/uibase/app/appopt \
    sw/source/uibase/app/docsh \
    sw/source/uibase/app/docsh2 \
    sw/source/uibase/app/docshdrw \
    sw/source/uibase/app/docshini \
    sw/source/uibase/app/docst \
    sw/source/uibase/app/docstyle \
    sw/source/uibase/app/mainwn \
    sw/source/uibase/app/swdll \
    sw/source/uibase/app/swmodul1 \
    sw/source/uibase/app/swmodule \
    sw/source/uibase/app/swwait \
    sw/source/uibase/cctrl/actctrl \
    sw/source/uibase/cctrl/popbox \
    sw/source/uibase/cctrl/swlbox \
    sw/source/uibase/chrdlg/ccoll \
    sw/source/uibase/config/StoredChapterNumbering \
    sw/source/uibase/config/barcfg \
    sw/source/uibase/config/caption \
    sw/source/uibase/config/cfgitems \
    sw/source/uibase/config/dbconfig \
    sw/source/uibase/config/fontcfg \
    sw/source/uibase/config/modcfg \
    sw/source/uibase/config/prtopt \
    sw/source/uibase/config/uinums \
    sw/source/uibase/config/usrpref \
    sw/source/uibase/config/viewopt \
    sw/source/uibase/dialog/SwSpellDialogChildWindow \
    sw/source/uibase/dialog/regionsw \
    sw/source/uibase/dialog/swabstdlg \
    sw/source/uibase/dialog/swwrtshitem \
    sw/source/uibase/dochdl/gloshdl \
    sw/source/uibase/dochdl/swdtflvr \
    sw/source/uibase/docvw/AnchorOverlayObject \
    sw/source/uibase/docvw/AnnotationMenuButton \
    sw/source/uibase/docvw/AnnotationWin \
    sw/source/uibase/docvw/DashedLine \
    sw/source/uibase/docvw/FrameControlsManager \
    sw/source/uibase/docvw/PageBreakWin \
    sw/source/uibase/docvw/OverlayRanges \
    sw/source/uibase/docvw/PostItMgr \
    sw/source/uibase/docvw/ShadowOverlayObject \
    sw/source/uibase/docvw/SidebarTxtControl \
    sw/source/uibase/docvw/SidebarTxtControlAcc \
    sw/source/uibase/docvw/SidebarWin \
    sw/source/uibase/docvw/SidebarWinAcc \
    sw/source/uibase/docvw/HeaderFooterWin \
    sw/source/uibase/docvw/edtdd \
    sw/source/uibase/docvw/edtwin \
    sw/source/uibase/docvw/edtwin2 \
    sw/source/uibase/docvw/edtwin3 \
    sw/source/uibase/docvw/frmsidebarwincontainer \
    sw/source/uibase/docvw/romenu \
    sw/source/uibase/docvw/srcedtw \
    sw/source/uibase/envelp/envimg \
    sw/source/uibase/envelp/labelcfg \
    sw/source/uibase/envelp/labimg \
    sw/source/uibase/envelp/syncbtn \
    sw/source/uibase/fldui/fldmgr \
    sw/source/uibase/fldui/fldwrap \
    sw/source/uibase/fldui/xfldui \
    sw/source/uibase/frmdlg/colex \
    sw/source/uibase/frmdlg/colmgr \
    sw/source/uibase/frmdlg/frmmgr \
    sw/source/uibase/globdoc/globdoc \
    sw/source/uibase/index/idxmrk \
    sw/source/uibase/dialog/wordcountwrapper \
    sw/source/uibase/index/toxmgr \
    sw/source/uibase/lingu/hhcwrp \
    sw/source/uibase/lingu/hyp \
    sw/source/uibase/lingu/olmenu \
    sw/source/uibase/lingu/sdrhhcwrap \
    sw/source/uibase/misc/glosdoc \
    sw/source/uibase/misc/glshell \
    sw/source/uibase/misc/numberingtypelistbox \
    sw/source/uibase/misc/redlndlg \
    sw/source/uibase/misc/swruler \
    sw/source/uibase/ribbar/conarc \
    sw/source/uibase/ribbar/concustomshape \
    sw/source/uibase/ribbar/conform \
    sw/source/uibase/ribbar/conpoly \
    sw/source/uibase/ribbar/conrect \
    sw/source/uibase/ribbar/drawbase \
    sw/source/uibase/ribbar/dselect \
    sw/source/uibase/ribbar/inputwin \
    sw/source/uibase/ribbar/tblctrl \
    sw/source/uibase/ribbar/tbxanchr \
    sw/source/uibase/ribbar/workctrl \
    sw/source/uibase/shells/annotsh \
    sw/source/uibase/shells/basesh \
    sw/source/uibase/shells/beziersh \
    sw/source/uibase/shells/drawdlg \
    sw/source/uibase/shells/drawsh \
    sw/source/uibase/shells/drformsh \
    sw/source/uibase/shells/drwbassh \
    sw/source/uibase/shells/drwtxtex \
    sw/source/uibase/shells/drwtxtsh \
    sw/source/uibase/shells/frmsh \
    sw/source/uibase/shells/grfsh \
    sw/source/uibase/shells/grfshex \
    sw/source/uibase/shells/langhelper \
    sw/source/uibase/shells/listsh \
    sw/source/uibase/shells/mediash \
    sw/source/uibase/shells/navsh \
    sw/source/uibase/shells/olesh \
    sw/source/uibase/shells/slotadd \
    sw/source/uibase/shells/tabsh \
    sw/source/uibase/shells/textdrw \
    sw/source/uibase/shells/textfld \
    sw/source/uibase/shells/textglos \
    sw/source/uibase/shells/textidx \
    sw/source/uibase/shells/textsh \
    sw/source/uibase/shells/textsh1 \
    sw/source/uibase/shells/textsh2 \
    sw/source/uibase/shells/txtattr \
    sw/source/uibase/shells/txtcrsr \
    sw/source/uibase/shells/txtnum \
    sw/source/uibase/sidebar/PageOrientationControl \
    sw/source/uibase/sidebar/PageMarginControl \
    sw/source/uibase/sidebar/PageSizeControl \
    sw/source/uibase/sidebar/PageColumnControl \
    sw/source/uibase/sidebar/PagePropertyPanel \
    sw/source/uibase/sidebar/WrapPropertyPanel \
    sw/source/uibase/sidebar/SwPanelFactory \
    sw/source/uibase/smartmenu/stmenu \
    sw/source/uibase/table/chartins \
    sw/source/uibase/table/swtablerep \
    sw/source/uibase/table/tablemgr \
    sw/source/uibase/uiview/formatclipboard \
    sw/source/uibase/uiview/pview \
    sw/source/uibase/uiview/scroll \
    sw/source/uibase/uiview/srcview \
    sw/source/uibase/uiview/swcli \
    sw/source/uibase/uiview/uivwimp \
    sw/source/uibase/uiview/view \
    sw/source/uibase/uiview/view0 \
    sw/source/uibase/uiview/view1 \
    sw/source/uibase/uiview/view2 \
    sw/source/uibase/uiview/viewcoll \
    sw/source/uibase/uiview/viewdlg \
    sw/source/uibase/uiview/viewdlg2 \
    sw/source/uibase/uiview/viewdraw \
    sw/source/uibase/uiview/viewling \
    sw/source/uibase/uiview/viewmdi \
    sw/source/uibase/uiview/viewport \
    sw/source/uibase/uiview/viewprt \
    sw/source/uibase/uiview/viewsrch \
    sw/source/uibase/uiview/viewstat \
    sw/source/uibase/uiview/viewtab \
    sw/source/uibase/uno/SwXDocumentSettings \
    sw/source/uibase/uno/SwXFilterOptions \
    sw/source/uibase/uno/dlelstnr \
    sw/source/uibase/uno/unoatxt \
    sw/source/uibase/uno/unodefaults \
    sw/source/uibase/uno/unodispatch \
    sw/source/uibase/uno/unodoc \
    sw/source/uibase/uno/unofreg \
    sw/source/uibase/uno/unomod \
    sw/source/uibase/uno/unomodule \
    sw/source/uibase/uno/unotxdoc \
    sw/source/uibase/uno/unotxvw \
    sw/source/uibase/utlui/attrdesc \
    sw/source/uibase/utlui/bookctrl \
    sw/source/uibase/utlui/condedit \
    sw/source/uibase/utlui/content \
    sw/source/uibase/utlui/glbltree \
    sw/source/uibase/utlui/gloslst \
    sw/source/uibase/utlui/initui \
    sw/source/uibase/utlui/navicfg \
    sw/source/uibase/utlui/navipi \
    sw/source/uibase/utlui/numfmtlb \
    sw/source/uibase/utlui/prcntfld \
    sw/source/uibase/utlui/shdwcrsr \
    sw/source/uibase/utlui/tmplctrl \
    sw/source/uibase/utlui/uiitems \
    sw/source/uibase/utlui/uitool \
    sw/source/uibase/utlui/unotools \
    sw/source/uibase/utlui/viewlayoutctrl \
    sw/source/uibase/utlui/wordcountctrl \
    sw/source/uibase/utlui/zoomctrl \
    sw/source/uibase/web/wdocsh \
    sw/source/uibase/web/wformsh \
    sw/source/uibase/web/wfrmsh \
    sw/source/uibase/web/wgrfsh \
    sw/source/uibase/web/wlistsh \
    sw/source/uibase/web/wolesh \
    sw/source/uibase/web/wtabsh \
    sw/source/uibase/web/wtextsh \
    sw/source/uibase/web/wview \
    sw/source/uibase/wrtsh/delete \
    sw/source/uibase/wrtsh/move \
    sw/source/uibase/wrtsh/navmgr \
    sw/source/uibase/wrtsh/select \
    sw/source/uibase/wrtsh/wrtsh1 \
    sw/source/uibase/wrtsh/wrtsh2 \
    sw/source/uibase/wrtsh/wrtsh3 \
    sw/source/uibase/wrtsh/wrtsh4 \
    sw/source/uibase/wrtsh/wrtundo \
))

ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))
$(eval $(call gb_Library_add_exception_objects,sw,\
    sw/source/uibase/dbui/dbmgr \
    sw/source/uibase/dbui/dbtree \
    sw/source/uibase/dbui/dbui \
    sw/source/uibase/dbui/maildispatcher \
    sw/source/uibase/dbui/mailmergechildwindow \
    sw/source/uibase/dbui/mailmergehelper \
    sw/source/uibase/dbui/mmconfigitem \
    sw/source/uibase/dbui/swdbtoolsclient \
    sw/source/uibase/uno/unomailmerge \
))
endif

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Library_add_objcxxobjects,sw,\
    sw/source/uibase/docvw/macdictlookup \
))
endif

$(eval $(call gb_SdiTarget_SdiTarget,sw/sdi/swslots,sw/sdi/swriter))

$(eval $(call gb_SdiTarget_set_include,sw/sdi/swslots,\
    -I$(SRCDIR)/sw/inc \
    -I$(SRCDIR)/sw/sdi \
    -I$(SRCDIR)/svx/sdi \
    -I$(SRCDIR)/sfx2/sdi \
    $$(INCLUDE) \
))

# Runtime dependency for unit-tests
$(eval $(call gb_Library_use_restarget,sw,sw))

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Library_use_system_darwin_frameworks,sw,\
    CoreFoundation \
    AppKit \
))
endif	# GUIBASE == java

# vim: set noet sw=4 ts=4:
