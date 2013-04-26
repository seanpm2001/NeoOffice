<?xml version="1.0" encoding="UTF-8"?>


<!--***********************************************************************
  This is the main transformation style sheet for transforming.
  Only use with OOo 2.0
  Owner: fpe@openoffice.org
  =========================================================================
  Changes Log
    May 24 2004 Created
    Aug 24 2004 Fixed for help2 CWS
    Aug 27 2004 Added css link, fixed missing embed-mode for variable
                Removed width/height for images
    Sep 03 2004 Modularized xsl, added some embedded modes
    Oct 08 2004 Fixed bug wrong mode "embedded" for links
                Added embedded modes for embed and embedvar (for cascaded embeds)
                Added <p> tags around falsely embedded pars and vars
    Dec 08 2004 #i38483#, fixed wrong handling of web links
                #i37377#, fixed missing usage of Database parameter for switching
    Jan 04 2005 #i38905#, fixed buggy branding replacement template
    Mar 17 2005 #i43972#, added language info to image URL, evaluate Language parameter
                evaluate new localize attribute in images
    May 10 2005 #i48785#, fixed wrong setting of distrib variable
    Aug 16 2005 workaround for #i53365#
    Aug 19 2005 fixed missing list processing in embedded sections
    Aug 19 2005 #i53535#, fixed wrong handling of Database parameter
		Oct 17 2006 #i70462#, disabled sorting to avoid output of error messages to console
***********************************************************************//-->

<!--

  Copyright 2008 by Sun Microsystems, Inc.
 
  $RCSfile$
 
  $Revision$
 
  This file is part of NeoOffice.
 
  NeoOffice is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  only, as published by the Free Software Foundation.
 
  NeoOffice is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License version 3 for more details
  (a copy is included in the LICENSE file that accompanied this code).
 
  You should have received a copy of the GNU General Public License
  version 3 along with NeoOffice.  If not, see 
  <http://www.gnu.org/licenses/gpl-3.0.txt>
  for a copy of the GPLv3 License.
 
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output indent="yes" method="html"/>

<!--
############################
# Variables and Parameters #
############################
//-->

<!-- General Usage -->
<xsl:variable name="am" select="'&amp;'"/>
<xsl:variable name="sl" select="'/'"/>
<xsl:variable name="qt" select="'&quot;'"/>

<!-- generic Icon alt text -->
<xsl:variable name="alttext" select="'text/shared/00/icon_alt.xhp'"/>

<!-- For calculating pixel sizes -->
<xsl:variable name="dpi" select="'96'"/>
<xsl:variable name="dpcm" select="'38'"/>

<!-- Product brand variables used in the help files -->
<xsl:variable name="brand1" select="'$[officename]'"/>
<xsl:variable name="brand2" select="'$[officeversion]'"/>
<xsl:variable name="brand3" select="'%PRODUCTNAME'"/>
<xsl:variable name="brand4" select="'%PRODUCTVERSION'"/>
<xsl:variable name="brand5" select="'$(OO_PRODUCT_NAME)'"/>

<!-- meta data variables from the help file -->
<xsl:variable name="filename" select="/helpdocument/meta/topic/filename"/>
<xsl:variable name="topic_id" select="/helpdocument/meta/topic/@id"/>
<xsl:variable name="topic_status" select="/helpdocument/meta/topic/@status"/>
<xsl:variable name="title" select="/helpdocument/meta/topic/title"/>
<xsl:variable name="doclang" select="/helpdocument/meta/topic/title/@xml-lang"/>

<!-- Module and the corresponding switching values-->
<xsl:param name="Database" select="'swriter'"/>
<xsl:variable name="module" select="$Database"/>
<xsl:variable name="appl">
	<xsl:choose>
		<xsl:when test="$module = 'swriter'"><xsl:value-of select="'WRITER'"/></xsl:when>
		<xsl:when test="$module = 'scalc'"><xsl:value-of select="'CALC'"/></xsl:when>
		<xsl:when test="$module = 'sdraw'"><xsl:value-of select="'DRAW'"/></xsl:when>
		<xsl:when test="$module = 'simpress'"><xsl:value-of select="'IMPRESS'"/></xsl:when>
		<xsl:when test="$module = 'schart'"><xsl:value-of select="'CHART'"/></xsl:when>
		<xsl:when test="$module = 'sbasic'"><xsl:value-of select="'BASIC'"/></xsl:when>
		<xsl:when test="$module = 'smath'"><xsl:value-of select="'MATH'"/></xsl:when>
	</xsl:choose>
</xsl:variable>

  <!-- the other parameters given by the help caller -->
<xsl:param name="System" select="'WIN'"/>
<xsl:param name="productname" select="'Office'"/>
<xsl:param name="productversion" select="''"/>
<xsl:variable name="pversion">
	<xsl:value-of select="translate($productversion,' ','')"/>
</xsl:variable>
<!-- this is were the images are -->
<xsl:param name="imgrepos" select="''"/>
<xsl:param name="Id" />
<!-- (lame) distinction between OS and Commercial -->
<xsl:param name="distrib">
	<!-- Always use "OpenSource" -->
	<xsl:value-of select="'OpenSource'"/>
	<!--
	<xsl:choose>
		<xsl:when test="starts-with($productname,'OpenOffice')">
			<xsl:value-of select="'OpenSource'"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="'COMMERCIAL'"/>
		</xsl:otherwise>
	</xsl:choose>
	-->
</xsl:param>
<xsl:param name="Language" select="'en-US'"/>
<xsl:variable name="lang" select="$Language"/>

<xsl:param name="ExtensionId" select="''"/>
<xsl:param name="ExtensionPath" select="''"/>


  <!-- parts of help and image urls -->
<xsl:variable name="help_url_prefix" select="'vnd.sun.star.help://'"/>
<xsl:variable name="img_url_prefix" select="concat('vnd.sun.star.pkg://',$imgrepos,'/')"/>
<xsl:variable name="urlpost" select="concat('?Language=',$lang,$am,'System=',$System,$am,'UseDB=no')"/>
<xsl:variable name="urlpre" select="$help_url_prefix" /> 
<xsl:variable name="linkprefix" select="$urlpre"/>
<xsl:variable name="linkpostfix" select="$urlpost"/>

<xsl:variable name="css" select="'default.css'"/>

<!-- images for notes, tips and warnings -->
<xsl:variable name="note_img" select="concat($img_url_prefix,'res/helpimg/note.png')"/>
<xsl:variable name="tip_img" select="concat($img_url_prefix,'res/helpimg/tip.png')"/>
<xsl:variable name="warning_img" select="concat($img_url_prefix,'res/helpimg/warning.png')"/>

<!--
#############
# Templates #
#############
//-->

<!-- Create the document skeleton -->
<xsl:template match="/">
	<xsl:variable name="csslink" select="concat($urlpre,'/',$urlpost)"/>
	<html>
		<head>
			<title><xsl:value-of select="$title"/></title>
			<link href="{$csslink}" rel="Stylesheet" type="text/css" /> <!-- stylesheet link -->
  		<meta http-equiv="Content-type" content="text/html; charset=utf-8"/>
		</head>
		<body lang="{$lang}">
			<xsl:apply-templates select="/helpdocument/body"/>
		</body>
	</html>
</xsl:template>

<!-- AHELP -->
<xsl:template match="ahelp">
	<xsl:if test="not(@visibility='hidden')"><span class="avis"><xsl:apply-templates /></span></xsl:if>
</xsl:template>

<!-- ALT -->
<xsl:template match="alt"/>

<!-- BOOKMARK -->
<xsl:template match="bookmark">
	<a name="{@id}"></a>
	<xsl:choose>
		<xsl:when test="starts-with(@branch,'hid')" />
		<xsl:otherwise><xsl:apply-templates /></xsl:otherwise>
	</xsl:choose>
</xsl:template>
<xsl:template match="bookmark" mode="embedded" />

<!-- BOOKMARK_VALUE -->
<xsl:template match="bookmark_value" />

<!-- BR -->
<xsl:template match="br"><br /></xsl:template>

<!-- CAPTION -->
<xsl:template match="caption" />

<!-- CASE -->
<xsl:template match="case"><xsl:call-template name="insertcase" /></xsl:template>
<xsl:template match="case" mode="embedded">
	<xsl:call-template name="insertcase">
		<xsl:with-param name="embedded" select="'yes'"/>
	</xsl:call-template>
</xsl:template>

<!-- CASEINLINE -->
<xsl:template match="caseinline"><xsl:call-template name="insertcase" /></xsl:template>
<xsl:template match="caseinline" mode="embedded">
	<xsl:call-template name="insertcase">
		<xsl:with-param name="embedded" select="'yes'"/>
	</xsl:call-template>
</xsl:template>

<!-- COMMENT -->
<xsl:template match="comment" />
<xsl:template match="comment" mode="embedded"/>

<!-- CREATED -->
<xsl:template match="created" />

<!-- DEFAULT -->
<xsl:template match="default"><xsl:call-template name="insertdefault" /></xsl:template>
<xsl:template match="default" mode="embedded">
	<xsl:call-template name="insertdefault">
		<xsl:with-param name="embedded" select="'yes'"/>
	</xsl:call-template>
</xsl:template>

<!-- DEFAULTINLINE -->
<xsl:template match="defaultinline"><xsl:call-template name="insertdefault" /></xsl:template>
<xsl:template match="defaultinline" mode="embedded">
	<xsl:call-template name="insertdefault">
		<xsl:with-param name="embedded" select="'yes'"/>
	</xsl:call-template>
</xsl:template>

<!-- EMBED -->
<xsl:template match="embed"><xsl:call-template name="resolveembed"/></xsl:template>
<xsl:template match="embed" mode="embedded"><xsl:call-template name="resolveembed"/></xsl:template>

<!-- EMBEDVAR -->
<xsl:template match="embedvar"><xsl:call-template name="resolveembedvar"/></xsl:template>
<xsl:template match="embedvar" mode="embedded"><xsl:call-template name="resolveembedvar"/></xsl:template>

<!-- EMPH -->
<xsl:template match="emph">
	<span class="emph"><xsl:apply-templates /></span>
</xsl:template>
<xsl:template match="emph" mode="embedded">
	<span class="emph"><xsl:apply-templates /></span>
</xsl:template>

<!-- FILENAME -->
<xsl:template match="filename" />

<!-- HISTORY -->
<xsl:template match="history" />

<!-- IMAGE -->
<xsl:template match="image"><xsl:call-template name="insertimage"/></xsl:template>
<xsl:template match="image" mode="embedded"><xsl:call-template name="insertimage"/></xsl:template>

<!-- ITEM -->
<xsl:template match="item"><span class="{@type}"><xsl:apply-templates /></span></xsl:template>
<xsl:template match="item" mode="embedded"><span class="{@type}"><xsl:apply-templates /></span></xsl:template>

<!-- LASTEDITED -->
<xsl:template match="lastedited" />

<!-- LINK -->
<xsl:template match="link">
	<xsl:choose> <!-- don't insert the heading link to itself -->
		<xsl:when test="(concat('/',@href) = /helpdocument/meta/topic/filename) or (@href = /helpdocument/meta/topic/filename)">
			<xsl:apply-templates />
		</xsl:when>
		<xsl:when test="contains(child::embedvar/@href,'/00/00000004.xhp#wie')"> <!-- special treatment of howtoget links -->
			<xsl:call-template name="insert_howtoget">
				<xsl:with-param name="linkhref" select="@href"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:call-template name="createlink" /> 
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>
<xsl:template match="link" mode="embedded">
	<xsl:call-template name="createlink"/>
</xsl:template>

<!-- LIST -->
<xsl:template match="list">
	<xsl:choose>
		<xsl:when test="@type='ordered'">
			<ol>
				<xsl:if test="@startwith">
					<xsl:attribute name="start"><xsl:value-of select="@startwith"/></xsl:attribute>
				</xsl:if>
				<xsl:apply-templates />
			</ol>
		</xsl:when>
		<xsl:otherwise>
			<ul><xsl:apply-templates /></ul>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match="list" mode="embedded">
	<xsl:choose>
		<xsl:when test="@type='ordered'">
			<ol>
				<xsl:if test="@startwith">
					<xsl:attribute name="start"><xsl:value-of select="@startwith"/></xsl:attribute>
				</xsl:if>
				<xsl:apply-templates mode="embedded"/>
			</ol>
		</xsl:when>
		<xsl:otherwise>
			<ul><xsl:apply-templates mode="embedded"/></ul>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<!-- LISTITEM -->
<xsl:template match="listitem">
	<li><xsl:apply-templates /></li>
</xsl:template>

<xsl:template match="listitem" mode="embedded">
	<li><xsl:apply-templates mode="embedded"/></li>
</xsl:template>

<!-- META, SEE HEADER -->
<xsl:template match="meta" />

<!-- OBJECT (UNUSED) -->
<xsl:template match="object" />

<!-- PARAGRAPH -->
<xsl:template match="paragraph">
	<xsl:choose>
		
		<xsl:when test="@role='heading'">
			<xsl:call-template name="insertheading">
				<xsl:with-param name="level" select="@level"/>
			</xsl:call-template>
		</xsl:when>
		
		<xsl:when test="contains(' note warning tip ',@role)">
			<xsl:call-template name="insertnote">
				<xsl:with-param name="type" select="@role" />
			</xsl:call-template>
		</xsl:when>
		
		<xsl:when test="contains(descendant::embedvar/@href,'/00/00000004.xhp#wie')"> <!-- special treatment of howtoget links -->
			<xsl:apply-templates />
		</xsl:when>		
		
		<xsl:otherwise>
			<xsl:call-template name="insertpara" />
		</xsl:otherwise>
	
	</xsl:choose>
</xsl:template>

<xsl:template match="paragraph" mode="embedded">
		<xsl:choose>
		
		<xsl:when test="@role='heading'">	<!-- increase the level of headings that are embedded -->
		<!-- 
		   The internal sablotron processor does not seem to support the number function.
			 Therefore, we need a workaround for
			 <xsl:variable name="level"><xsl:value-of select="number(@level)+1"/></xsl:variable>
		-->
			<xsl:variable name="newlevel">
				<xsl:choose>
					<xsl:when test="@level='1'"><xsl:value-of select="'2'"/></xsl:when>
					<xsl:when test="@level='2'"><xsl:value-of select="'2'"/></xsl:when>
					<xsl:when test="@level='3'"><xsl:value-of select="'3'"/></xsl:when>
					<xsl:when test="@level='4'"><xsl:value-of select="'4'"/></xsl:when>
					<xsl:when test="@level='5'"><xsl:value-of select="'5'"/></xsl:when>
				</xsl:choose>
			</xsl:variable>
			
			<xsl:call-template name="insertheading">
				<xsl:with-param name="level" select="$newlevel"/>
				<xsl:with-param name="embedded" select="'yes'"/>
			</xsl:call-template>
		</xsl:when>
		
		<xsl:when test="contains(' note warning tip ',@role)">
			<xsl:call-template name="insertnote">
				<xsl:with-param name="type" select="@role" />
			</xsl:call-template>
		</xsl:when>
		
		<xsl:when test="contains(descendant::embedvar/@href,'/00/00000004.xhp#wie')"> <!-- special treatment of howtoget links -->
			<xsl:apply-templates />
		</xsl:when>		
		
		<xsl:otherwise>
			<xsl:call-template name="insertpara" />
		</xsl:otherwise>
		
	</xsl:choose>
</xsl:template>


<!-- SECTION -->
<xsl:template match="section">
	<a name="{@id}"></a>

		<xsl:choose>
			
			<xsl:when test="@id='relatedtopics'">
				<div class="relatedtopics">
					<xsl:variable name="href"><xsl:value-of select="concat($urlpre,'shared/text/shared/00/00000004.xhp',$urlpost)"/></xsl:variable>
					<xsl:variable name="anchor"><xsl:value-of select="'related'"/></xsl:variable>
					<xsl:variable name="doc" select="document($href)"/>
					<p class="related">
						<xsl:apply-templates select="$doc//variable[@id=$anchor]"/>
					</p>
					<div class="relatedbody">
						<xsl:apply-templates />
					</div>
				</div>
			</xsl:when>
			
			<xsl:when test="@id='howtoget'">
				<xsl:call-template name="insert_howtoget" />
			</xsl:when>
			
			<xsl:otherwise>
						<xsl:apply-templates/>
			</xsl:otherwise>
		
		</xsl:choose>

</xsl:template>


<!-- SECTION -->
<xsl:template match="section" mode="embedded">
	<a name="{@id}"></a>
	<xsl:apply-templates mode="embedded"/>
</xsl:template>

<!-- SORT -->
<xsl:template match="sort" >
  <!-- sorting disabled due to #i70462#
	<xsl:apply-templates><xsl:sort select="descendant::paragraph"/></xsl:apply-templates>
	//-->
	<xsl:apply-templates />
</xsl:template>
<xsl:template match="sort" mode="embedded">
<!-- sorting disabled due to #i70462#
	<xsl:apply-templates><xsl:sort select="descendant::paragraph"/></xsl:apply-templates>
	//-->
	<xsl:apply-templates />
</xsl:template>

<!-- SWITCH -->
<xsl:template match="switch"><xsl:apply-templates /></xsl:template>
<xsl:template match="switch" mode="embedded"><xsl:apply-templates /></xsl:template>

<!-- SWITCHINLINE -->
<xsl:template match="switchinline"><xsl:apply-templates /></xsl:template>
<xsl:template match="switchinline" mode="embedded"><xsl:apply-templates mode="embedded"/></xsl:template>

<!-- TABLE -->
<xsl:template match="table"><xsl:call-template name="inserttable"/></xsl:template>
<xsl:template match="table" mode="embedded"><xsl:call-template name="inserttable"/></xsl:template>

<!-- TABLECELL -->
<xsl:template match="tablecell"><td valign="top"><xsl:apply-templates /></td></xsl:template>
<xsl:template match="tablecell" mode="icontable"><td valign="top"><xsl:apply-templates/></td></xsl:template>
<xsl:template match="tablecell" mode="embedded"><td valign="top"><xsl:apply-templates mode="embedded"/></td></xsl:template>

<!-- TABLEROW -->
<xsl:template match="tablerow"><tr><xsl:apply-templates /></tr></xsl:template>
<xsl:template match="tablerow" mode="icontable"><tr><xsl:apply-templates mode="icontable"/></tr></xsl:template>
<xsl:template match="tablerow" mode="embedded"><tr><xsl:apply-templates mode="embedded"/></tr></xsl:template>

<!-- TITLE -->
<xsl:template match="title"/>

<!-- TOPIC -->
<xsl:template match="topic"/>

<!-- VARIABLE -->
<xsl:template match="variable"><a name="{@id}"></a><xsl:apply-templates /></xsl:template>
<xsl:template match="variable" mode="embedded"><a name="{@id}"></a><xsl:apply-templates mode="embedded"/></xsl:template>

<xsl:template match="text()">
	<xsl:call-template name="brand">
		<xsl:with-param name="string"><xsl:value-of select="."/></xsl:with-param>
	</xsl:call-template>
</xsl:template>

<xsl:template match="text()" mode="embedded">
	<xsl:call-template name="brand">
		<xsl:with-param name="string"><xsl:value-of select="."/></xsl:with-param>
	</xsl:call-template>
</xsl:template>

<!-- In case of missing help files -->
<xsl:template match="help-id-missing"><xsl:value-of select="$Id"/></xsl:template>

<!-- 
###################
# NAMED TEMPLATES #
###################
//-->

<!-- Branding -->
<xsl:template name="brand" >
	<xsl:param name="string"/>
	
    <xsl:choose>
		
        <xsl:when test="contains($string,$brand1)">
           <xsl:variable name="newstr">
                <xsl:value-of select="substring-before($string,$brand1)"/>
                <xsl:value-of select="$productname"/>
                <xsl:value-of select="substring-after($string,$brand1)"/>
           </xsl:variable>
			<xsl:call-template name="brand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
        
		<xsl:when test="contains($string,$brand2)">
		    <xsl:variable name="newstr">
                <xsl:value-of select="substring-before($string,$brand2)"/>
                <xsl:value-of select="$pversion"/>
                <xsl:value-of select="substring-after($string,$brand2)"/>
           </xsl:variable>
			<xsl:call-template name="brand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
        
		<xsl:when test="contains($string,$brand3)">
			<xsl:variable name="newstr">
                <xsl:value-of select="substring-before($string,$brand3)"/>
                <xsl:value-of select="$productname"/>
                <xsl:value-of select="substring-after($string,$brand3)"/>
           </xsl:variable>
			<xsl:call-template name="brand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
		
        <xsl:when test="contains($string,$brand4)">
			    <xsl:variable name="newstr">
                <xsl:value-of select="substring-before($string,$brand4)"/>
                <xsl:value-of select="$pversion"/>
                <xsl:value-of select="substring-after($string,$brand4)"/>
           </xsl:variable>
			<xsl:call-template name="brand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
		
		<!-- Replace as many OpenOffice.org references as we can -->
		<xsl:when test="contains($string,$brand5)">
			<xsl:variable name="newstr">
				<xsl:value-of select="substring-before($string,$brand5)"/>
				<xsl:value-of select="$productname"/>
				<xsl:value-of select="substring-after($string,$brand5)"/>
			</xsl:variable>
			<xsl:call-template name="brand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
		
        <xsl:otherwise>
			<xsl:value-of select="$string"/>
		</xsl:otherwise>
	</xsl:choose> 
    
</xsl:template>


<!-- Insert Paragraph -->
<xsl:template name="insertpara">
	<xsl:variable name="role">
		<xsl:choose>
			<xsl:when test="ancestor::table">
				<xsl:value-of select="concat(@role,'intable')"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="@role"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
	<p class="{$role}"><xsl:apply-templates /></p>
</xsl:template>

<!-- Insert "How to get Link" -->
<xsl:template name="insert_howtoget">
	<xsl:param name="linkhref" />
	<xsl:variable name="archive" select="'shared'"/>
	<xsl:variable name="tmp_href"><xsl:value-of select="concat($urlpre,'shared/text/shared/00/00000004.xhp',$urlpost)"/></xsl:variable>	
	<xsl:variable name="tmp_doc" select="document($tmp_href)"/>
	<table class="howtoget" width="100%" border="1" cellpadding="3" cellspacing="0">
		<tr>
			<td>
				<p class="howtogetheader"><xsl:apply-templates select="$tmp_doc//variable[@id='wie']"/></p>
				<div class="howtogetbody">
				<xsl:choose>
					<xsl:when test="$linkhref = ''"> <!-- new style -->
						<xsl:apply-templates/>
					</xsl:when>
					<xsl:otherwise> <!-- old style -->
						<xsl:variable name="archive1"><xsl:value-of select="concat(substring-before(substring-after($linkhref,'text/'),'/'),'/')"/></xsl:variable>
						<xsl:variable name="href"><xsl:value-of select="concat($urlpre,$archive1,substring-before($linkhref,'#'),$urlpost)"/></xsl:variable>
						<xsl:variable name="anc"><xsl:value-of select="substring-after($linkhref,'#')"/></xsl:variable>
						<xsl:variable name="docum" select="document($href)"/>
						
						<xsl:call-template name="insertembed">
							<xsl:with-param name="doc" select="$docum" />
							<xsl:with-param name="anchor" select="$anc" />
						</xsl:call-template>

					</xsl:otherwise>
				</xsl:choose>				
				</div>
			</td>
		</tr>
	</table>
	<br/>
</xsl:template>

<!-- Create a link -->
<xsl:template name="createlink">
<xsl:variable name="archive"><xsl:value-of select="concat(substring-before(substring-after(@href,'text/'),'/'),'/')"/></xsl:variable>
<xsl:variable name="dbpostfix"><xsl:call-template name="createDBpostfix"><xsl:with-param name="archive" select="$archive"/></xsl:call-template></xsl:variable>
	<xsl:choose>
		<!-- Fix bug 1120 by replacing the OOo support URL -->
		<xsl:when test="@href and parent::paragraph[@id='par_id3150667']">
			<a href="$(PRODUCT_SUPPORT_URL)">$(PRODUCT_SUPPORT_URL_TEXT)</a>
		</xsl:when>
		<!-- Replace the OOo documentation URL -->
		<xsl:when test="@href and parent::paragraph[@id='par_id3497211']">
			<a href="$(PRODUCT_DOCUMENTATION_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<!-- Replace the OOo command line options URL -->
		<xsl:when test="@href and parent::paragraph[@id='par_id3150503']">
			<a href="$(PRODUCT_DOCUMENTATION_LAUNCHSHORTCUTS_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<!-- Replace the OOo spellchecking URLs -->
		<xsl:when test="@href and parent::paragraph[@id='par_id6434522']">
			<a href="http://neowiki.neooffice.org/index.php/Activating_Dictionaries_and_Configuring_Spellcheck">NeoOffice Wiki</a>
			<a href="$(PRODUCT_DOCUMENTATION_SPELLCHECK_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<xsl:when test="@href and parent::paragraph[@id='par_id3552964']">
			<a href="$(PRODUCT_DOCUMENTATION_SPELLCHECK_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<xsl:when test="@href and parent::paragraph[@id='par_id6434522']">
			<a href="$(PRODUCT_DOCUMENTATION_SPELLCHECK_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<xsl:when test="@href and parent::paragraph[@id='par_id3552964']">
			<a href="$(PRODUCT_DOCUMENTATION_SPELLCHECK_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<xsl:when test="@href and parent::paragraph[@id='par_id9625843']">
			<a href="$(PRODUCT_DOCUMENTATION_SPELLCHECK_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<xsl:when test="@href and parent::paragraph[@id='par_id1683706']">
			<a href="$(PRODUCT_DOCUMENTATION_SPELLCHECK_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<xsl:when test="contains(@href,'#')">
			<xsl:variable name="anchor"><xsl:value-of select="concat('#',substring-after(@href,'#'))"/></xsl:variable>
			<xsl:variable name="href"><xsl:value-of select="concat($linkprefix,$archive,substring-before(@href,'#'),$linkpostfix,$dbpostfix,$anchor)"/></xsl:variable>
			<a href="{$href}"><xsl:apply-templates /></a>
		</xsl:when>
		<xsl:when test="starts-with(@href,'http://')">  <!-- web links -->
			<a href="{@href}"><xsl:apply-templates /></a>
		</xsl:when>
		<xsl:otherwise>
			<xsl:variable name="href"><xsl:value-of select="concat($linkprefix,$archive,@href,$linkpostfix,$dbpostfix)"/></xsl:variable>
			<a href="{$href}"><xsl:apply-templates /></a>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<!-- Insert Note, Warning, or Tip -->
<xsl:template name="insertnote">
	<xsl:param name="type" /> <!-- note, tip, or warning -->
	<xsl:variable name="imgsrc">
		<xsl:choose>
			<xsl:when test="$type='note'"><xsl:value-of select="$note_img"/></xsl:when>
			<xsl:when test="$type='tip'"><xsl:value-of select="$tip_img"/></xsl:when>
			<xsl:when test="$type='warning'"><xsl:value-of select="$warning_img"/></xsl:when>
		</xsl:choose>
	</xsl:variable>
	<xsl:variable name="dbpostfix"><xsl:call-template name="createDBpostfix"><xsl:with-param name="archive" select="'shared'"/></xsl:call-template></xsl:variable>
	<xsl:variable name="alt">
		<xsl:variable name="href"><xsl:value-of select="concat($urlpre,'shared/',$alttext,$urlpost,$dbpostfix)"/></xsl:variable>
		<xsl:variable name="anchor"><xsl:value-of select="concat('alt_',$type)"/></xsl:variable>
		<xsl:variable name="doc" select="document($href)"/>
		<xsl:apply-templates select="$doc//variable[@id=$anchor]" mode="embedded"/>
	</xsl:variable>
	<div class="{$type}">
		<table border="0" class="{$type}" cellspacing="0" cellpadding="5">
			<tr>
				<td><img src="{$imgsrc}" alt="{$alt}" title="{$alt}"/></td>
				<td><xsl:apply-templates /></td>
			</tr>
		</table>
	</div>
	<br/>
</xsl:template>

<!-- Insert a heading -->
<xsl:template name="insertheading">
	<xsl:param name="level" />
	<xsl:param name="embedded" />
	<xsl:text disable-output-escaping="yes">&lt;h</xsl:text><xsl:value-of select="$level"/><xsl:text disable-output-escaping="yes">&gt;</xsl:text>
		<xsl:choose>
			<xsl:when test="$embedded = 'yes'">
				<xsl:apply-templates mode="embedded"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:apply-templates />
			</xsl:otherwise>
		</xsl:choose>
	<xsl:text disable-output-escaping="yes">&lt;/h</xsl:text><xsl:value-of select="$level"/><xsl:text disable-output-escaping="yes">&gt;</xsl:text>
</xsl:template>

<!-- Evaluate a case or caseinline switch -->
<xsl:template name="insertcase">
	<xsl:param name="embedded" />
	<xsl:choose>
		<xsl:when test="parent::switch[@select='sys'] or parent::switchinline[@select='sys']">
			<xsl:if test="@select = $System">
				<xsl:choose>
					<xsl:when test="$embedded = 'yes'">
						<xsl:apply-templates mode="embedded"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates />
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
		</xsl:when>
		<xsl:when test="parent::switch[@select='appl'] or parent::switchinline[@select='appl']">
			<xsl:if test="@select = $appl">
				<xsl:choose>
					<xsl:when test="$embedded = 'yes'">
						<xsl:apply-templates mode="embedded"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates />
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
		</xsl:when>
		<xsl:when test="parent::switch[@select='distrib'] or parent::switchinline[@select='distrib']">
			<xsl:if test="@select = $distrib">
				<xsl:choose>
					<xsl:when test="$embedded = 'yes'">
						<xsl:apply-templates mode="embedded"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates />
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
		</xsl:when>
	</xsl:choose>
</xsl:template>

<!-- Evaluate a default or defaultinline switch -->
<xsl:template name="insertdefault">
	<xsl:param name="embedded" />
	
	<xsl:choose>
		<xsl:when test="parent::switch[@select='sys'] or parent::switchinline[@select='sys']">
			<xsl:if test="not(../child::case[@select=$System]) and not(../child::caseinline[@select=$System])">
				<xsl:choose>
					<xsl:when test="$embedded = 'yes'">
						<xsl:apply-templates mode="embedded"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates />
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
		</xsl:when>
		<xsl:when test="parent::switch[@select='appl'] or parent::switchinline[@select='appl']">
			<xsl:if test="not(../child::case[@select=$appl]) and not(../child::caseinline[@select=$appl])">
				<xsl:choose>
					<xsl:when test="$embedded = 'yes'">
						<xsl:apply-templates mode="embedded"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates />
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
		</xsl:when>
		<xsl:when test="parent::switch[@select='distrib'] or parent::switchinline[@select='distrib']">
			<xsl:if test="not(../child::case[@select=$distrib]) and not(../child::caseinline[@select=$distrib])">
				<xsl:choose>
					<xsl:when test="$embedded = 'yes'">
						<xsl:apply-templates mode="embedded"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates />
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
		</xsl:when>
	</xsl:choose>
</xsl:template>

<!-- evaluate embeds -->
<xsl:template name="insertembed">
	<xsl:param name="doc" />
	<xsl:param name="anchor" />
	<!-- different embed targets (also falsely used embed instead embedvar) -->
	<xsl:choose>
		<xsl:when test="$doc//section[@id=$anchor]"> <!-- first test for a section of that name -->
			<xsl:apply-templates select="$doc//section[@id=$anchor]" mode="embedded"/>
		</xsl:when>
		<xsl:when test="$doc//paragraph[@id=$anchor]"> <!-- then test for a para of that name -->
			<p class="embedded">
				<xsl:apply-templates select="$doc//paragraph[@id=$anchor]" mode="embedded"/>
			</p>
		</xsl:when>
		<xsl:when test="$doc//variable[@id=$anchor]"> <!-- then test for a variable of that name -->
			<p class="embedded">
				<xsl:apply-templates select="$doc//variable[@id=$anchor]" mode="embedded"/>
			</p>
		</xsl:when>
		<xsl:otherwise> <!-- then give up -->
			<p class="bug">D'oh! You found a bug (<xsl:value-of select="@href"/> not found).</p> 
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<!-- Insert an image -->
<xsl:template name="insertimage">
	
	<xsl:variable name="fpath">
		<xsl:call-template name="getfpath">
			<xsl:with-param name="s"><xsl:value-of select="@src"/></xsl:with-param>
		</xsl:call-template>
	</xsl:variable>
	
	<xsl:variable name="fname">
		<xsl:call-template name="getfname">
			<xsl:with-param name="s"><xsl:value-of select="@src"/></xsl:with-param>
		</xsl:call-template>
	</xsl:variable>

  <xsl:variable name="src">
    <xsl:choose>
      <xsl:when test="not($ExtensionId='') and starts-with(@src,$ExtensionId)">
        <xsl:value-of select="concat($ExtensionPath,'/',@src)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="(@localize='true') and not($lang='en-US')">
            <xsl:value-of select="concat($img_url_prefix,$fpath,$lang,'/',$fname)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($img_url_prefix,$fpath,$fname)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  
	<!--<xsl:variable name="src"><xsl:value-of select="concat($img_url_prefix,@src)"/></xsl:variable>-->
	<xsl:variable name="alt"><xsl:value-of select="./alt"/></xsl:variable>
	<xsl:variable name="width" select="''"/> <!-- Images don't all have the correct size -->
	<xsl:variable name="height" select="''"/><!-- Image don't all have the correct size -->
	<img src="{$src}" alt="{$alt}" title="{$alt}">
		<xsl:if test="not($width='')"><xsl:attribute name="width"><xsl:value-of select="$width"/></xsl:attribute></xsl:if>
		<xsl:if test="not($height='')"><xsl:attribute name="height"><xsl:value-of select="$height"/></xsl:attribute></xsl:if>
	</img>  
</xsl:template>

<!-- Insert a Table -->
<xsl:template name="inserttable">
	<xsl:variable name="imgsrc">	<!-- see if we are in an image table -->
		<xsl:value-of select="tablerow/tablecell[1]/paragraph[1]/image/@src"/>
	</xsl:variable>
	
	<xsl:choose>
		
		<xsl:when test="count(descendant::tablecell)=1">
			<table border="0" class="onecell" cellpadding="0" cellspacing="0">
				<xsl:apply-templates />
		 </table>
		</xsl:when>
		
		<xsl:when test="descendant::tablecell[1]/descendant::image">
			<table border="0" class="icontable" cellpadding="5" cellspacing="0">
				<xsl:apply-templates mode="icontable"/>
		 </table>
		</xsl:when>
		
		<xsl:when test="@class='wide'">
			<table border="1" class="{@class}" cellpadding="0" cellspacing="0" width="100%" >
				<xsl:apply-templates />
		 </table>
		</xsl:when>
		
		<xsl:when test="not(@class='')">
			<table border="1" class="{@class}" cellpadding="0" cellspacing="0" >
				<xsl:apply-templates />
		 </table>
		</xsl:when>
		
		<xsl:otherwise>
			<table border="1" class="border" cellpadding="0" cellspacing="0" >
				<xsl:apply-templates />
		 </table>
		</xsl:otherwise>
	</xsl:choose>
	
	<br/>
</xsl:template>

<xsl:template name="resolveembed">
	<div class="embedded">
		<xsl:variable name="archive"><xsl:value-of select="concat(substring-before(substring-after(@href,'text/'),'/'),'/')"/></xsl:variable>
		<xsl:variable name="dbpostfix"><xsl:call-template name="createDBpostfix"><xsl:with-param name="archive" select="$archive"/></xsl:call-template></xsl:variable>
		<xsl:variable name="href"><xsl:value-of select="concat($urlpre,$archive,substring-before(@href,'#'),$urlpost,$dbpostfix)"/></xsl:variable>
		<xsl:variable name="anc"><xsl:value-of select="substring-after(@href,'#')"/></xsl:variable>
		<xsl:variable name="docum" select="document($href)"/>
		
		<xsl:call-template name="insertembed">
			<xsl:with-param name="doc" select="$docum" />
			<xsl:with-param name="anchor" select="$anc" />
		</xsl:call-template>

	</div>
</xsl:template>

<xsl:template name="resolveembedvar">
	<xsl:if test="not(@href='text/shared/00/00000004.xhp#wie')"> <!-- special treatment if howtoget links -->
		<xsl:variable name="archive"><xsl:value-of select="concat(substring-before(substring-after(@href,'text/'),'/'),'/')"/></xsl:variable>
		<xsl:variable name="dbpostfix"><xsl:call-template name="createDBpostfix"><xsl:with-param name="archive" select="$archive"/></xsl:call-template></xsl:variable>
		<xsl:variable name="href"><xsl:value-of select="concat($urlpre,$archive,substring-before(@href,'#'),$urlpost,$dbpostfix)"/></xsl:variable>
		<xsl:variable name="anchor"><xsl:value-of select="substring-after(@href,'#')"/></xsl:variable>
		<xsl:variable name="doc" select="document($href)"/>
		<xsl:choose>
			<xsl:when test="$doc//variable[@id=$anchor]"> <!-- test for a variable of that name -->
				<xsl:apply-templates select="$doc//variable[@id=$anchor]" mode="embedded"/>
			</xsl:when>
			<xsl:otherwise> <!-- or give up -->
				<span class="bug">[<xsl:value-of select="@href"/> not found].</span> 
			</xsl:otherwise>
		</xsl:choose>
	</xsl:if>
</xsl:template>

<!-- Apply -->
<xsl:template name="apply">
	<xsl:param name="embedded" />
	<xsl:choose>
		<xsl:when test="$embedded = 'yes'">
			<xsl:apply-templates mode="embedded"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:apply-templates />
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="getfpath">
	<xsl:param name="s"/>
	<xsl:param name="p"/>
	<xsl:choose>
		<xsl:when test="contains($s,'/')">
			<xsl:call-template name="getfpath">
				<xsl:with-param name="p"><xsl:value-of select="concat($p,substring-before($s,'/'),'/')"/></xsl:with-param>
				<xsl:with-param name="s"><xsl:value-of select="substring-after($s,'/')"/></xsl:with-param>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$p"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="getfname">
	<xsl:param name="s"/>
	<xsl:choose>
		<xsl:when test="contains($s,'/')">
			<xsl:call-template name="getfname">
				<xsl:with-param name="s"><xsl:value-of select="substring-after($s,'/')"/></xsl:with-param>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$s"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="createDBpostfix">
	<xsl:param name="archive"/>
	<xsl:variable name="newDB">
		<xsl:choose>
			<xsl:when test="(substring($archive,1,6) = 'shared')"><xsl:value-of select="$Database"/></xsl:when>
			<xsl:otherwise><xsl:value-of select="substring-before($archive,'/')"/></xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
	<xsl:value-of select="concat($am,'DbPAR=',$newDB)"/>
</xsl:template>

<!-- Remove OpenOffice.org support text -->
<xsl:template match="paragraph[@id='par_id9173253']" />
<xsl:template match="paragraph[@id='par_id3149140']" />
<xsl:template match="paragraph[@id='par_id3154230']" />
<xsl:template match="paragraph[@id='hd_id26327']" />
<xsl:template match="paragraph[@id='par_id1318380']" />
<xsl:template match="paragraph[@id='hd_id2611386']" />
<xsl:template match="paragraph[@id='par_id3166335']" />
<xsl:template match="paragraph[@id='hd_id0915200811081722']" />
<xsl:template match="paragraph[@id='par_id0915200811081778']" />
<xsl:template match="paragraph[@id='hd_id0804200803314150']" />
<xsl:template match="paragraph[@id='par_id0804200803314235']" />

<!-- Remove Download and Language Pack text -->
<xsl:template match="paragraph[@id='hd_id3168534']" />
<xsl:template match="paragraph[@id='par_id3028143']" />
<xsl:template match="paragraph[@id='hd_id9999694']" />
<xsl:template match="listitem[paragraph[@id='par_id2216559']]" />
<xsl:template match="listitem[paragraph[@id='par_id7869502']]" />
<xsl:template match="listitem[paragraph[@id='par_id9852900']]" />
<xsl:template match="listitem[paragraph[@id='par_id3791924']]" />

<!-- Remove Help Find text -->
<xsl:template match="paragraph[@id='par_id3155555']">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_id3152552']" />
<xsl:template match="paragraph[@id='par_id3153345']" />
<xsl:template match="paragraph[@id='par_id3155941']" />
<xsl:template match="paragraph[@id='par_id3157958']" />
<xsl:template match="paragraph[@id='par_id3147210']" />
<xsl:template match="paragraph[@id='par_id3146798']" />
<xsl:template match="paragraph[@id='par_id3149732']" />

<!-- Remove Java text -->
<xsl:template match="paragraph[@id='par_id3152363']">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="section[embed[@href='text/shared/00/00000406.xhp#java']]" />
<xsl:template match="paragraph[@id='par_id3154751']" />
<xsl:template match="paragraph[@id='par_id3155338']" />
<xsl:template match="list[listitem[paragraph[@id='par_id3155892']]]" />
<xsl:template match="paragraph[@id='par_id9116183']" />
<xsl:template match="paragraph[@id='par_id3153822']" />
<xsl:template match="paragraph[@id='par_idN10568' and preceding-sibling::paragraph[@id='par_idN10558']]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_idN1056B']" />
<xsl:template match="paragraph[@id='par_idN1057F']" />
<xsl:template match="paragraph[@id='par_idN10583']" />
<xsl:template match="paragraph[@id='par_idN10610']" />
<xsl:template match="paragraph[@id='par_idN10614']" />
<xsl:template match="paragraph[@id='par_idN105A5']" />
<xsl:template match="paragraph[@id='par_idN10635']" />
<xsl:template match="paragraph[@id='par_idN105A9']" />
<xsl:template match="paragraph[@id='par_idN10657']" />
<xsl:template match="paragraph[@id='par_idN105AD']" />
<xsl:template match="paragraph[@id='par_idN10686']" />
<xsl:template match="paragraph[@id='par_idN1056A']">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_idN10589']" />
<xsl:template match="paragraph[@id='par_idN1059F']" />
<xsl:template match="paragraph[@id='par_idN105D9']" />
<xsl:template match="paragraph[@id='par_idN1060A']" />
<xsl:template match="paragraph[@id='par_idN1060E']" />
<xsl:template match="paragraph[@id='par_idN10625']" />
<xsl:template match="paragraph[@id='par_idN10629']" />
<xsl:template match="paragraph[@id='par_idN10640']" />
<xsl:template match="paragraph[@id='par_idN10644' and preceding-sibling::paragraph[@id='par_idN10640']]" />
<xsl:template match="paragraph[@id='par_id3143267' and preceding-sibling::paragraph[@id='hd_id3147399']]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_id8847010']" />
<xsl:template match="paragraph[@id='hd_id3145345']" />
<xsl:template match="listitem[paragraph[@id='par_id3147209']]" />
<xsl:template match="listitem[paragraph[@id='par_id3146797']]" />
<xsl:template match="listitem[paragraph[@id='par_id3149578']]" />
<xsl:template match="listitem[paragraph[@id='par_id3149762']]" />
<xsl:template match="paragraph[@id='hd_id3149795']" />
<xsl:template match="paragraph[@id='par_id3155419']" />
<xsl:template match="listitem[paragraph[@id='par_id3156329']]" />
<xsl:template match="listitem[paragraph[@id='par_id3155628']]" />
<xsl:template match="listitem[paragraph[@id='par_id3148474']]" />
<xsl:template match="paragraph[@id='hd_id3153061']" />
<xsl:template match="paragraph[@id='par_id3156024']" />
<xsl:template match="listitem[paragraph[@id='par_id3149045']]" />
<xsl:template match="listitem[paragraph[@id='par_id3152811']]" />
<xsl:template match="listitem[paragraph[@id='par_id3153379']]" />
<xsl:template match="paragraph[@id='par_id3149808']" />
<xsl:template match="paragraph[@id='par_idN10549' and preceding-sibling::paragraph[@id='par_idN10545']]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_idN10568' and preceding-sibling::paragraph[@id='par_idN10545']]" />
<xsl:template match="paragraph[@id='par_idN1056C']" />
<xsl:template match="paragraph[@id='par_id5404522']" />
<xsl:template match="paragraph[@id='par_idN105D8']" />
<xsl:template match="paragraph[@id='par_idN105DD']" />
<xsl:template match="paragraph[@id='par_idN1057B']" />
<xsl:template match="paragraph[@id='par_idN1057E']" />
<xsl:template match="paragraph[@id='par_idN10581']" />
<xsl:template match="paragraph[@id='par_idN1060C']" />
<xsl:template match="paragraph[@id='par_idN1058C']" />
<xsl:template match="paragraph[@id='par_idN10590']" />
<xsl:template match="paragraph[@id='par_idN105A7']" />
<xsl:template match="paragraph[@id='par_idN105AB']" />
<xsl:template match="paragraph[@id='par_idN105C2']" />
<xsl:template match="paragraph[@id='par_idN105C6']" />
<xsl:template match="paragraph[@id='par_idN10600']">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_idN10623']" />
<xsl:template match="paragraph[@id='par_idN10627']" />
<xsl:template match="paragraph[@id='par_idN1062D']" />
<xsl:template match="paragraph[@id='par_idN10634']" />
<xsl:template match="paragraph[@id='par_idN10638']" />
<xsl:template match="paragraph[@id='par_idN1064B']" />
<xsl:template match="paragraph[@id='par_idN1064E']" />
<xsl:template match="paragraph[@id='par_idN10661']" />
<xsl:template match="paragraph[@id='par_idN10668']" />
<xsl:template match="paragraph[@id='par_idN1066B']" />
<xsl:template match="paragraph[@id='par_idN1066B']" />
<xsl:template match="listitem[paragraph[@id='par_idN10674']]" />
<xsl:template match="listitem[paragraph[@id='par_idN10678']]" />
<xsl:template match="listitem[paragraph[@id='par_idN1067C']]" />
<xsl:template match="paragraph[@id='par_idN1067F']" />
<xsl:template match="paragraph[@id='par_idN10683']" />
<xsl:template match="paragraph[@id='par_idN10689']" />
<xsl:template match="paragraph[@id='par_idN1068C']" />
<xsl:template match="listitem[paragraph[@id='par_idN10695']]" />
<xsl:template match="listitem[paragraph[@id='par_idN10699']]" />
<xsl:template match="listitem[paragraph[@id='par_idN1069D']]" />
<xsl:template match="paragraph[@id='par_idN106A0']" />
<xsl:template match="paragraph[@id='par_idN106A4']" />
<xsl:template match="paragraph[@id='par_idN106BB']" />
<xsl:template match="paragraph[@id='par_idN106BF']" />
<xsl:template match="paragraph[@id='par_id7953733']" />
<xsl:template match="paragraph[@id='par_idN106CE']" />
<xsl:template match="paragraph[@id='par_idN106E4']" />
<xsl:template match="paragraph[@id='par_idN106E7']" />
<xsl:template match="paragraph[@id='par_idN106F6']" />
<xsl:template match="paragraph[@id='par_idN10569']" />
<xsl:template match="paragraph[@id='par_idN1056D']" />
<xsl:template match="paragraph[@id='par_idN10570']" />
<xsl:template match="paragraph[@id='par_idN10582']" />
<xsl:template match="paragraph[@id='par_id3152349']" />
<xsl:template match="embed[@href='text/shared/optionen/java.xhp#java']" />
<xsl:template match="paragraph[@id='par_id5248573']">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_id7128818']" />
<xsl:template match="paragraph[@id='hd_id3753776']" />
<xsl:template match="listitem[paragraph[@id='par_id5284279']]" />
<xsl:template match="listitem[paragraph[@id='par_id4494766']]" />
<xsl:template match="listitem[paragraph[@id='par_id7858516']]" />
<xsl:template match="listitem[paragraph[@id='par_id973540']]" />
<xsl:template match="listitem[paragraph[@id='par_id4680928']]" />
<xsl:template match="listitem[paragraph[@id='par_id9014252']]" />
<xsl:template match="listitem[paragraph[@id='par_id6011841']]" />
<xsl:template match="listitem[paragraph[@id='par_id2591326']]" />
<xsl:template match="listitem[paragraph[@id='par_id6201666']]" />
<xsl:template match="listitem[paragraph[@id='par_id208136']]" />
<xsl:template match="paragraph[@id='hd_id556047']" />
<xsl:template match="paragraph[@id='par_id4515823']" />
<xsl:template match="listitem[paragraph[@id='par_id4044312']]" />
<xsl:template match="listitem[paragraph[@id='par_id1369060']]" />
<xsl:template match="listitem[paragraph[@id='par_id860927']]" />
<xsl:template match="listitem[paragraph[@id='par_id8581804']]" />
<xsl:template match="listitem[paragraph[@id='par_id7730033']]" />
<xsl:template match="paragraph[@id='par_id6317636']" />
<xsl:template match="paragraph[@id='hd_id9759514']" />
<xsl:template match="listitem[paragraph[@id='par_id9076509']]" />
<xsl:template match="listitem[paragraph[@id='par_id7050691']]" />
<xsl:template match="listitem[paragraph[@id='par_id7118747']]" />
<xsl:template match="paragraph[@id='par_id8283639']" />
<xsl:template match="paragraph[@id='par_id2291024']" />
<xsl:template match="paragraph[@id='par_id2100589']" />
<xsl:template match="listitem[paragraph[@id='par_id5022125']]" />
<xsl:template match="listitem[paragraph[@id='par_id6844386']]" />
<xsl:template match="listitem[paragraph[@id='par_id7018646']]" />
<xsl:template match="paragraph[@id='par_id5857112']" />
<xsl:template match="paragraph[@id='par_id6042664']" />
<xsl:template match="paragraph[@id='par_id1589098']" />
<xsl:template match="paragraph[@id='par_id1278420']" />
<xsl:template match="paragraph[@id='par_id7479476']" />
<xsl:template match="paragraph[@id='par_id3099154']" />
<xsl:template match="listitem[paragraph[@id='par_id2218390']]" />
<xsl:template match="listitem[paragraph[@id='par_id7771538']]" />
<xsl:template match="listitem[paragraph[@id='par_id366527']]" />
<xsl:template match="listitem[paragraph[@id='par_id7996459']]" />
<xsl:template match="paragraph[@id='par_id2531815']" />
<xsl:template match="paragraph[@id='par_id5927304']" />
<xsl:template match="paragraph[@id='par_id4503921']" />
<xsl:template match="listitem[paragraph[@id='par_id4051026']]" />
<xsl:template match="listitem[paragraph[@id='par_id3397320']]" />
<xsl:template match="listitem[paragraph[@id='par_id3059785']]" />
<xsl:template match="paragraph[@id='par_id7657399']" />
<xsl:template match="paragraph[@id='par_id8925138']" />
<xsl:template match="paragraph[@id='par_id5461897']" />
<xsl:template match="paragraph[@id='par_id8919339']" />
<xsl:template match="paragraph[@id='par_id4634235']" />
<xsl:template match="paragraph[@id='par_id1393475']" />
<xsl:template match="paragraph[@id='par_id6571550']" />
<xsl:template match="paragraph[@id='par_id5376140']" />
<xsl:template match="paragraph[@id='par_id9611499']" />
<xsl:template match="paragraph[@id='par_id6765953']" />
<xsl:template match="paragraph[@id='par_id1511581']" />
<xsl:template match="paragraph[@id='par_id4881740']" />
<xsl:template match="listitem[paragraph[@id='par_id8286385']]" />
<xsl:template match="paragraph[@id='par_id2354197']" />
<xsl:template match="paragraph[@id='par_id2485122']" />
<xsl:template match="paragraph[@id='hd_id8746910']" />
<xsl:template match="paragraph[@id='par_id9636524']" />
<xsl:template match="paragraph[@id='par_id5941648']" />
<xsl:template match="paragraph[@id='par_id8307138']" />
<xsl:template match="paragraph[@id='par_id7138889']" />
<xsl:template match="paragraph[@id='par_id9869380']" />
<xsl:template match="paragraph[@id='par_id12512']" />
<xsl:template match="paragraph[@id='par_id2676168']" />
<xsl:template match="paragraph[@id='par_id2626422']" />
<xsl:template match="listitem[paragraph[@id='par_id1743827']]" />
<xsl:template match="listitem[paragraph[@id='par_id4331797']]" />
<xsl:template match="listitem[paragraph[@id='par_id4191717']]" />
<xsl:template match="paragraph[@id='par_id2318796']" />
<xsl:template match="listitem[paragraph[@id='par_id399182']]" />
<xsl:template match="listitem[paragraph[@id='par_id7588732']]" />
<xsl:template match="listitem[paragraph[@id='par_id95828']]" />
<xsl:template match="listitem[paragraph[@id='par_id5675527']]" />
<xsl:template match="listitem[paragraph[@id='par_id3496200']]" />
<xsl:template match="paragraph[@id='par_id7599108']" />
<xsl:template match="paragraph[@id='par_id888698']" />
<xsl:template match="paragraph[@id='par_id3394573']" />
<xsl:template match="paragraph[@id='par_id7594225']" />
<xsl:template match="paragraph[@id='par_id8147221']" />
<xsl:template match="paragraph[@id='hd_id3154750']" />
<xsl:template match="paragraph[@id='par_id3155628']" />
<xsl:template match="paragraph[@id='par_id3143267' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidemobiledevicefiltersxml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id3147143') and not(@id='par_id3143267') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidemobiledevicefiltersxml']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidemobiledevicefiltersxml']]]" />
<xsl:template match="paragraph[(@id='hd_id3151172' or @id='par_id3148386') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0000000005xml']]]" />

<!-- Remove Online Registration text -->
<xsl:template match="paragraph[@id='par_id3153882']">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="section[embed[@href='text/shared/00/00000408.xhp#online']]" />
<xsl:template match="paragraph[@id='hd_id3153624']" />
<xsl:template match="paragraph[@id='par_id3150445']" />
<xsl:template match="paragraph[@id='hd_id3145629']" />
<xsl:template match="paragraph[@id='par_id3149999']" />
<xsl:template match="paragraph[@id='hd_id3149760']" />
<xsl:template match="paragraph[@id='par_id3151234']" />
<xsl:template match="paragraph[@id='hd_id3147557']" />
<xsl:template match="paragraph[@id='par_id3148548']" />
<xsl:template match="embed[@href='text/shared/optionen/online_update.xhp#online_update']" />

<!-- Remove Online Update text -->
<xsl:template match="paragraph[@id='par_id8754844']">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="section[embed[@href='text/shared/00/00000406.xhp#online_update']]" />
<xsl:template match="paragraph[@id='hd_id2189397']" />
<xsl:template match="paragraph[@id='par_id7523728']" />
<xsl:template match="paragraph[@id='par_id8994109']" />
<xsl:template match="paragraph[@id='par_id476699']" />
<xsl:template match="paragraph[@id='par_id4057130']" />
<xsl:template match="paragraph[@id='hd_id266426']" />
<xsl:template match="paragraph[@id='par_id3031098']" />
<xsl:template match="paragraph[@id='hd_id8276619']" />
<xsl:template match="paragraph[@id='par_id7824030']" />
<xsl:template match="paragraph[@id='hd_id7534104']" />
<xsl:template match="paragraph[@id='par_id209051']" />
<xsl:template match="paragraph[@id='hd_id1418805']" />
<xsl:template match="paragraph[@id='par_id1743522']" />
<xsl:template match="paragraph[@id='hd_id5994140']" />
<xsl:template match="paragraph[@id='par_id7870113']" />
<xsl:template match="paragraph[@id='hd_id3051545']" />
<xsl:template match="paragraph[@id='par_id3061311']" />
<xsl:template match="paragraph[@id='hd_id4814905']" />
<xsl:template match="paragraph[@id='par_id2143925']" />
<xsl:template match="paragraph[@id='par_id6797082']">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_id4218878']" />
<xsl:template match="paragraph[@id='par_id8132267']" />
<xsl:template match="paragraph[@id='par_id702230']" />
<xsl:template match="listitem[paragraph[@id='par_id3422345']]" />
<xsl:template match="paragraph[@id='par_id9313638']" />
<xsl:template match="listitem[paragraph[@id='par_id9951780']]" />
<xsl:template match="listitem[paragraph[@id='par_id6479384']]" />
<xsl:template match="listitem[paragraph[@id='par_id3639027']]" />
<xsl:template match="paragraph[@id='par_id3722342']" />
<xsl:template match="listitem[paragraph[@id='par_id5106662']]" />
<xsl:template match="listitem[paragraph[@id='par_id4931485']]" />
<xsl:template match="listitem[paragraph[@id='par_id9168980']]" />
<xsl:template match="paragraph[@id='par_id9766533']" />
<xsl:template match="paragraph[@id='par_id927152']" />
<xsl:template match="paragraph[@id='par_id6081728']" />
<xsl:template match="paragraph[@id='par_id9219641']" />

<!-- Remove Python, JavaScript, and BeanShell text -->
<xsl:template match="listitem[paragraph[@id='par_idN10739']]" />
<xsl:template match="listitem[paragraph[@id='par_idN1073D']]" />
<xsl:template match="listitem[paragraph[@id='par_id6797082']]" />
<xsl:template match="paragraph[@id='par_idN1091F']" />
<xsl:template match="paragraph[@id='par_idN105AA']" />
<xsl:template match="paragraph[@id='par_idN105BA']" />
<xsl:template match="paragraph[@id='par_idN10622']" />
<xsl:template match="paragraph[@id='par_idN10597']" />
<xsl:template match="paragraph[@id='par_idN109BB']" />

<!-- Replace paragraph with "not available" warning -->
<xsl:template match="paragraph" mode="notavailable">
	<xsl:choose>
		<xsl:when test="$lang='de'">Diese Funktion ist nicht verfügbar.</xsl:when>
		<xsl:when test="$lang='fr'">Cette fonctionnalité n'est pas disponible.</xsl:when>
		<xsl:when test="$lang='it'">Questa funzione non è disponibile.</xsl:when>
		<xsl:when test="$lang='nl'">Deze functie is niet beschikbaar.</xsl:when>
		<xsl:otherwise>This feature is not available.</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<!-- Replace paragraph with "not available due to security risk" warning -->
<xsl:template match="paragraph" mode="securitywarning">
	<xsl:choose>
		<xsl:when test="$lang='de'">Diese Funktion ist nicht verfügbar, da externe Software, die als Sicherheitsrisiko identifiziert wurde benötigt werden.</xsl:when>
		<xsl:when test="$lang='fr'">Cette fonctionnalité n'est pas disponible, car il nécessite un logiciel externe qui a été identifié comme un risque de sécurité.</xsl:when>
		<xsl:when test="$lang='it'">Questa funzione non è disponibile perché richiede software esterno che è stato identificato come un rischio per la sicurezza.</xsl:when>
		<xsl:when test="$lang='nl'">Deze functie is niet beschikbaar, omdat het vereist externe software die is geïdentificeerd als een veiligheidsrisico.</xsl:when>
		<xsl:otherwise>This feature is not available because it requires external software that has been identified as a security risk.</xsl:otherwise>
	</xsl:choose>
</xsl:template>

</xsl:stylesheet>
