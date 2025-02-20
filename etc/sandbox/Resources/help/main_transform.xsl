<?xml version="1.0" encoding="UTF-8"?>

<!--***********************************************************************
  This is the main transformation style sheet for transforming.
  For use with LibreOffice 4.0+
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
    Jun 15 2009 #i101799#, fixed wrong handling of http URLs with anchors
***********************************************************************//-->

<!--
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
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
<xsl:variable name="brand5" select="'$(LIBO_PRODUCT_NAME)'"/>
<xsl:variable name="brand6" select="'LibreOffice.org'"/>
<xsl:variable name="brand7" select="'LibreOffice'"/>

<!-- meta data variables from the help file -->
<xsl:variable name="filename" select="/helpdocument/meta/topic/filename"/>
<xsl:variable name="title" select="/helpdocument/meta/topic/title"/>

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
<xsl:param name="Language" select="'en-US'"/>
<xsl:variable name="lang" select="$Language"/>

<xsl:param name="ExtensionId" select="''"/>
<xsl:param name="ExtensionPath" select="''"/>


  <!-- parts of help and image urls -->
<xsl:variable name="help_url_prefix" select="'vnd.sun.star.help://'"/>
<xsl:variable name="img_url_prefix" select="concat('vnd.sun.star.zip://',$imgrepos,'/')"/>
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

<!-- SUB -->
<xsl:template match="sub">
	<sub><xsl:apply-templates /></sub>
</xsl:template>
<xsl:template match="sub" mode="embedded">
	<sub><xsl:apply-templates /></sub>
</xsl:template>

<!-- SUP -->
<xsl:template match="sup">
	<sup><xsl:apply-templates /></sup>
</xsl:template>
<xsl:template match="sup" mode="embedded">
	<sup><xsl:apply-templates /></sup>
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
		
		<xsl:when test="@role='bascode'">
			<xsl:call-template name="insertbascode" />
		</xsl:when>

		<xsl:when test="@role='logocode'">
			<xsl:call-template name="insertlogocode" />
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
	<xsl:apply-templates><xsl:sort select="descendant::paragraph"/></xsl:apply-templates>
</xsl:template>
<xsl:template match="sort" mode="embedded">
	<xsl:apply-templates><xsl:sort select="descendant::paragraph"/></xsl:apply-templates>
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
		
		<!-- Replace as many LibreOffice references as we can -->
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
		<xsl:when test="contains($string,$brand6)">
			<xsl:variable name="newstr">
				<xsl:value-of select="substring-before($string,$brand6)"/>
				<xsl:value-of select="$productname"/>
				<xsl:value-of select="substring-after($string,$brand6)"/>
			</xsl:variable>
			<xsl:call-template name="brand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:when test="contains($string,$brand7) and parent::paragraph[not(@id='par_id2008200911583098') and not(@id='par_id5004119') and not(@id='par_id0120200910234570') and not(@id='par_id0902200809540918')]">
			<xsl:variable name="newstr">
				<xsl:value-of select="substring-before($string,$brand7)"/>
				<xsl:value-of select="$productname"/>
				<xsl:value-of select="substring-after($string,$brand7)"/>
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

<!-- Insert Basic code snippet  -->
<xsl:template name="insertbascode">
	<pre><xsl:apply-templates /></pre>
</xsl:template>

<!-- Insert Logo code snippet  -->
<xsl:template name="insertlogocode">
	<pre><xsl:apply-templates /></pre>
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
		<!-- Fix bug 1120 by replacing the LibO support URL -->
		<xsl:when test="@href[ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000001xml']]] and parent::paragraph[@id='par_id3150667']">
			<a href="$(PRODUCT_SUPPORT_URL)">$(PRODUCT_SUPPORT_URL_TEXT)</a>
		</xsl:when>
		<!-- Replace the LibO documentation URL -->
		<xsl:when test="@href[ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000001xml']]] and parent::paragraph[@id='par_id3497211']">
			<a href="$(PRODUCT_DOCUMENTATION_URL)">$(PRODUCT_DOCUMENTATION_URL_TEXT)</a>
		</xsl:when>
		<xsl:when test="starts-with(@href,'http://') or starts-with(@href,'https://')">  <!-- web links -->
			<a href="{@href}"><xsl:apply-templates /></a>
		</xsl:when>
		<xsl:when test="contains(@href,'#')">
			<xsl:variable name="anchor"><xsl:value-of select="concat('#',substring-after(@href,'#'))"/></xsl:variable>
			<xsl:variable name="href"><xsl:value-of select="concat($linkprefix,$archive,substring-before(@href,'#'),$linkpostfix,$dbpostfix,$anchor)"/></xsl:variable>
			<a href="{$href}"><xsl:apply-templates /></a>
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

<!-- Remove LibreOffice support text -->
<xsl:template match="paragraph[(@id='par_id9173253' or @id='par_id3154230' or @id='hd_id26327' or @id='par_id1318380' or @id='hd_id2611386' or @id='par_id3166335' or @id='hd_id0915200811081722' or @id='par_id0915200811081778' or @id='hd_id0804200803314150' or @id='par_id0804200803314235') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000001xml']]]" />

<!-- Remove crash reporting text -->
<xsl:template match="paragraph[@id='par_id3153345' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguideerror_reportxml']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id3150616') and not(@id='par_id3153345') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguideerror_reportxml']]]" />

<!-- Remove Download and Language Pack text -->
<xsl:template match="paragraph[(@id='hd_id3168534' or @id='par_id3028143') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000001xml']]]" />
<xsl:template match="paragraph[@id='hd_id9999694' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidelanguage_selectxml']]]" />
<xsl:template match="listitem[paragraph[(@id='par_id130619' or @id='par_id221655a' or @id='par_id7869502' or @id='par_id9852900' or @id='par_id3791924') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidelanguage_selectxml']]]]" />

<!-- Remove Help Find text -->
<xsl:template match="paragraph[@id='par_id3155555' and ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000140xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id3148523') and not(@id='par_id3155555') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000140xml']]]" />
<xsl:template match="paragraph[@id='par_id3154188' and ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000110xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>

<!-- Remove Java text -->
<xsl:template match="section[embed[@href='text/shared/00/00000406.xhp#advanced']]" />
<xsl:template match="embed[@href='text/shared/optionen/java.xhp#advanced']" />
<xsl:template match="paragraph[@id='par_idN10561' and ancestor::body/preceding-sibling::meta[topic[@id='textswriter01mailmerge02xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[@id='par_idN10572' and ancestor::body/preceding-sibling::meta[topic[@id='textswriter01mailmerge02xml']]]" />
<xsl:template match="paragraph[@id='par_id3152363' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedmain0650xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id3153089') and not(@id='par_id3152363') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedmain0650xml']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedmain0650xml']]]" />
<xsl:template match="paragraph[@id='par_idN10568' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenjavaxml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='par_idN10558') and not(@id='par_idN10568') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenjavaxml']]]" />
<xsl:template match="paragraph[@id='par_idN1056A' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenjavaclasspathxml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='par_idN10566') and not(@id='par_idN1056A') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenjavaclasspathxml']]]" />
<xsl:template match="paragraph[@id='par_id3143267' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguideassistivexml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id3147399') and not(@id='par_id3143267') and not(parent::section[@id='relatedtopics']) and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguideassistivexml']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedguideassistivexml']]]" />
<xsl:template match="paragraph[@id='par_idN10549' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenjavaparametersxml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='par_idN10545') and not(@id='par_idN10549') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenjavaparametersxml']]]" />
<xsl:template match="paragraph[@id='par_idN10600' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasedabawiz02jdbcxml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='par_idN105FC') and not(@id='par_idN10600') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasedabawiz02jdbcxml']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasedabawiz02jdbcxml']]]" />
<xsl:template match="paragraph[(@id='par_idN10569' or @id='par_idN1056D' or @id='par_idN10570' or @id='par_idN10582') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasedabawiz02mysqlxml']]]" />
<xsl:template match="paragraph[(@id='par_id3153894' or @id='par_id3152349') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguideaccessibilityxml']]]" />
<xsl:template match="paragraph[@id='par_id5248573' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabaserep_mainxml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id8773155') and not(@id='par_id5248573') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabaserep_mainxml']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabaserep_mainxml']]]" />
<xsl:template match="paragraph[(@id='hd_id3154750' or @id='par_id3155628') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionen01013000xml']]]" />
<xsl:template match="paragraph[(@id='hd_id3151172' or @id='par_id3148386') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0000000005xml']]]" />
<xsl:template match="paragraph[@id='par_idN105C1' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_reportsxhp']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[(@id='par_id4094363' or @id='hd_id8414258' or @id='par_idN105C4') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_reportsxhp']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_reportsxhp']]]" />
<xsl:template match="table[@id='tbl_id1888180' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_reportsxhp']]]" />
<xsl:template match="paragraph[@id='hd_id3145609' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_reportxml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[(@id='par_id3147265' or @id='hd_id1695608' or @id='par_id7510910' or @id='par_id8138065' or @id='par_id5086825' or @id='par_id4747154' or @id='hd_id3153104' or @id='par_id3125863' or @id='par_id3155431' or @id='par_idN107D7') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_reportxml']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_reportxml']]]" />
<xsl:template match="paragraph[@id='par_id3147102' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01040000xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="section[embed[@href='text/shared/00/00000401.xhp#autopilotagenda']]" />
<xsl:template match="paragraph[(@id='par_id3156414' or @id='par_id3147571' or @id='hd_id3147088' or @id='par_id3149177' or @id='hd_id3155391' or @id='par_id3156426' or @id='hd_id3145382' or @id='par_id3156346' or @id='par_id3149235') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01040000xml']]]" />
<xsl:template match="embed[@href='text/shared/autopi/01040100.xhp#seite1']" />
<xsl:template match="embed[@href='text/shared/autopi/01040200.xhp#seite2']" />
<xsl:template match="embed[@href='text/shared/autopi/01040300.xhp#seite3']" />
<xsl:template match="embed[@href='text/shared/autopi/01040400.xhp#seite4']" />
<xsl:template match="embed[@href='text/shared/autopi/01040500.xhp#seite5']" />
<xsl:template match="embed[@href='text/shared/autopi/01040600.xhp#seite6']" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#abbrechen' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01040000xml']]]" />
<xsl:template match="paragraph[@id='par_idN1055C' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasetablewizard00xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="embed[@href='text/shared/explorer/database/tablewizard01.xhp#selecttable']" />
<xsl:template match="embed[@href='text/shared/explorer/database/tablewizard02.xhp#settype']" />
<xsl:template match="embed[@href='text/shared/explorer/database/tablewizard03.xhp#setprimary']" />
<xsl:template match="embed[@href='text/shared/explorer/database/tablewizard04.xhp#createtable']" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#zurueckautopi' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasetablewizard00xml']]]" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#weiterautopi' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasetablewizard00xml']]]" />
<xsl:template match="paragraph[@id='par_idN105AF' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasetablewizard00xml']]]" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#finish' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasetablewizard00xml']]]" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#abbrechen' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasetablewizard00xml']]]" />
<xsl:template match="paragraph[(@id='par_idN1061A' or @id='par_idN1061E') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_queriesxhp']]]" />
<xsl:template match="list[child::listitem[paragraph[@id='par_idN10632']] and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidedata_queriesxhp']]]" />
<xsl:template match="paragraph[@id='par_id3153394' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01020000xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="section[embed[@href='text/shared/00/00000401.xhp#autopilotfax']]" />
<xsl:template match="paragraph[(@id='par_id3154824' or @id='par_id3147088' or @id='hd_id3156156' or @id='par_id3155628' or @id='hd_id3147335' or @id='par_id3156117' or @id='hd_id3152350' or @id='par_id3146948') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01020000xml']]]" />
<xsl:template match="embed[@href='text/shared/autopi/01020100.xhp#seite1']" />
<xsl:template match="embed[@href='text/shared/autopi/01020200.xhp#seite2']" />
<xsl:template match="embed[@href='text/shared/autopi/01020300.xhp#seite3']" />
<xsl:template match="embed[@href='text/shared/autopi/01020400.xhp#seite4']" />
<xsl:template match="embed[@href='text/shared/autopi/01020500.xhp#seite5']" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#abbrechen' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01020000xml']]]" />
<xsl:template match="paragraph[@id='par_id3150247' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01090000xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="section[embed[@href='text/shared/00/00000401.xhp#autopilotformular']]" />
<xsl:template match="paragraph[(@id='par_id3152801' or @id='par_idN10686') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01090000xml']]]" />
<xsl:template match="embed[@href='text/shared/autopi/01090100.xhp#fieldselection']" />
<xsl:template match="embed[@href='text/shared/autopi/01090200.xhp#setupsubform']" />
<xsl:template match="embed[@href='text/shared/autopi/01090210.xhp#addsubformfields']" />
<xsl:template match="embed[@href='text/shared/autopi/01090220.xhp#getjoinedfields']" />
<xsl:template match="embed[@href='text/shared/autopi/01090300.xhp#arrangecontrols']" />
<xsl:template match="embed[@href='text/shared/autopi/01090400.xhp#setdataentry']" />
<xsl:template match="embed[@href='text/shared/autopi/01090500.xhp#applystyles']" />
<xsl:template match="embed[@href='text/shared/autopi/01090600.xhp#setname']" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#zurueckautopi' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01090000xml']]]" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#weiterautopi' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01090000xml']]]" />
<xsl:template match="embed[@href='text/shared/00/00000001.xhp#abbrechen' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01090000xml']]]" />
<xsl:template match="paragraph[@id='par_id3093440' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01010000xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="section[embed[@href='text/shared/00/00000401.xhp#autopilotbrief']]" />
<xsl:template match="paragraph[(@id='par_id3149178' or @id='par_id3153748' or @id='par_id3153824' or @id='hd_id3159176' or @id='par_id3153543' or @id='hd_id3150254' or @id='par_id3155923' or @id='hd_id3148944' or @id='par_id3149669' or @id='par_id3144433') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01010000xml']]]" />
<xsl:template match="embed[@href='text/shared/autopi/01010100.xhp#seite1']" />
<xsl:template match="embed[@href='text/shared/autopi/01010200.xhp#seite2']" />
<xsl:template match="embed[@href='text/shared/autopi/01010300.xhp#seite3']" />
<xsl:template match="embed[@href='text/shared/autopi/01010400.xhp#seite4']" />
<xsl:template match="embed[@href='text/shared/autopi/01010500.xhp#seite5']" />
<xsl:template match="embed[@href='text/shared/autopi/01010600.xhp#seite6']" />
<xsl:template match="paragraph[@id='par_idN11C3D' and ancestor::body/preceding-sibling::meta[topic[@id='textshared0000000406xml']]]" />
<xsl:template match="paragraph[(@id='par_idN1055D' or @id='par_idN10585' or @id='par_idN10589' or @id='par_id8584246') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasedabawiz01xml']]]" />

<!-- Remove Online Update text -->
<xsl:template match="section[embed[@href='text/shared/00/00000406.xhp#online_update']]" />
<xsl:template match="embed[@href='text/shared/optionen/online_update.xhp#online_update']" />
<xsl:template match="paragraph[@id='par_id8754844' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenonlineupdatexhp']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id29297') and not(@id='par_id8754844') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionenonlineupdatexhp']]]" />
<xsl:template match="paragraph[@id='par_id6797082' and ancestor::body/preceding-sibling::meta[topic[@id='textshared01online_updatexml']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id315256') and not(@id='par_id6797082') and ancestor::body/preceding-sibling::meta[topic[@id='textshared01online_updatexml']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textshared01online_updatexml']]]" />
<xsl:template match="paragraph[@id='par_id1906491' and ancestor::body/preceding-sibling::meta[topic[@id='textshared01online_update_dialogxml']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id4959257') and not(@id='par_id1906491') and not(parent::section[@id='relatedtopics']) and ancestor::body/preceding-sibling::meta[topic[@id='textshared01online_update_dialogxml']]]" />
<xsl:template match="paragraph[(@id='hd_id2926419' or @id='par_id2783898') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedmain0108xml']]]" />

<!-- Remove Python, JavaScript, and BeanShell text -->
<xsl:template match="listitem[paragraph[(@id='par_idN10739' or @id='par_idN1073D' or @id='par_id6797082') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidescriptingxml']]]]" />
<xsl:template match="paragraph[@id='par_idN1091F' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidescriptingxml']]]" />
<xsl:template match="paragraph[(@id='par_idN105AA' or @id='par_idN105BA' or @id='par_idN10622' or @id='par_idN10597' or @id='par_idN105A7' or @id='par_idN105FB') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0106130200xml']]]" />
<xsl:template match="paragraph[@id='par_idN109BB' and ancestor::body/preceding-sibling::meta[topic[@id='textshared0106130000xml']]]" />
<xsl:template match="paragraph[(@id='par_idN105E5') and ancestor::body/preceding-sibling::meta[topic[@id='textswriter01mailmerge08xml']]]">
	<xsl:apply-templates mode="securitywarning" select="." />
</xsl:template>
<xsl:template match="paragraph[(@id='par_idN105E8' or @id='par_idN105EC' or @id='par_idN105EF' or @id='par_idN105F3' or @id='par_idN10600' or @id='par_idN10604' or @id='par_idN10607' or @id='par_idN1060B' or @id='par_idN1060E' or @id='par_idN10611' or @id='par_idN10615' or @id='par_idN10626' or @id='par_idN1062A' or @id='par_idN1062D' or @id='par_idN10631' or @id='par_idN10642' or @id='par_idN10646') and ancestor::body/preceding-sibling::meta[topic[@id='textswriter01mailmerge08xml']]]" />

<!-- Remove LDAP text -->
<xsl:template match="paragraph[@id='par_idN10558' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasedabawiz02ldapxml']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[(@id='par_idN10579' or @id='par_idN1057D' or @id='par_idN10594' or @id='par_idN10598' or @id='par_idN105B7' or @id='par_idN105BB' or @id='par_idN105CA' or @id='par_idN105CE') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedexplorerdatabasedabawiz02ldapxml']]]" />
<xsl:template match="paragraph[@id='par_id3153562' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionen01014000xml']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[(@id='par_id3149797' or @id='hd_id3155388' or @id='par_id3147335' or @id='hd_id3153881' or @id='par_id3148943' or @id='hd_id3153061' or @id='par_id3156343' or @id='hd_id3146795' or @id='par_id3150358' or @id='par_id3154939') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionen01014000xml']]]" />

<!-- Replace "Mac OS X" and "OS X" with "Mac" -->
<xsl:template match="paragraph[@id='hd_id4791405' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01170000xml']]]/text()">
	<xsl:call-template name="replacewithmacbrand">
		<xsl:with-param name="string"><xsl:value-of select="."/></xsl:with-param>
	</xsl:call-template>
</xsl:template>
<xsl:template match="paragraph[@id='par_id6873683' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedautopi01170000xml']]]/ahelp/text()">
	<xsl:call-template name="replacewithmacbrand">
		<xsl:with-param name="string"><xsl:value-of select="."/></xsl:with-param>
	</xsl:call-template>
</xsl:template>

<!-- Remove Exit menu item text -->
<xsl:template match="paragraph[@id='par_id3151299' and ancestor::body/preceding-sibling::meta[topic[@id='textshared0101170000xml']]]" />
<xsl:template match="paragraph[not(@id='hd_id3154545') and not(@id='par_id3151299') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0101170000xml']]]" />
<xsl:template match="paragraph[@id='par_id3155869' and ancestor::body/preceding-sibling::meta[topic[@id='textshared0000000401xml']]]" mode="embedded" />

<!-- Remove Tools :: Options warning text -->
<xsl:template match="paragraph[@id='par_id1013200911280529' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedoptionen01000000xml']]]" />

<!-- Remove Improvement Program text -->
<xsl:template match="paragraph[(@id='hd_id0120200910361765' or @id='par_id0120200910361848' or @id='par_id0120200910361874') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0500000001xml']]]" />

<!-- Remove Start Center text -->
<xsl:template match="paragraph[@id='par_id3159201' and ancestor::body/preceding-sibling::meta[topic[@id='textshared0101050000xml']]]" />
<xsl:template match="paragraph[@id='par_id0820200803204063' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidestartcenterxhp']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_id0820200802524447') and not(@id='par_id0820200803204063') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidestartcenterxhp']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='textsharedguidestartcenterxhp']]]" />

<!-- Remove LibreLogo text -->
<xsl:template match="paragraph[@id='par_180' and ancestor::body/preceding-sibling::meta[topic[@id='LibreLogo']]]">
	<xsl:apply-templates mode="notavailable" select="." />
</xsl:template>
<xsl:template match="paragraph[not(@id='hd_170') and not(@id='par_180') and ancestor::body/preceding-sibling::meta[topic[@id='LibreLogo']]]" />
<xsl:template match="list[ancestor::body/preceding-sibling::meta[topic[@id='LibreLogo']]]" />
<xsl:template match="table[ancestor::body/preceding-sibling::meta[topic[@id='LibreLogo']]]" />

<!-- Remove OpenGL text -->
<xsl:template match="paragraph[(@id='hd_id3154507' or @id='par_id3146879') and ancestor::body/preceding-sibling::meta[topic[@id='textshared0000000005xml']]]" />

<!-- Remove feedback, license, and credits text -->
<xsl:template match="paragraph[(@id='hd_id2752763' or @id='par_id443534340' or @id='hd_id4153881' or @id='par_id4144510' or @id='hd_id5153881' or @id='par_id5144510') and ancestor::body/preceding-sibling::meta[topic[@id='textsharedmain0108xml']]]" />
<xsl:template match="listitem[paragraph[@id='par_id3147008' and ancestor::body/preceding-sibling::meta[topic[@id='textsharedguideversion_numberxml']]]]" />

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

<!-- Replace "Mac OS X" and "OS X" with "Mac" in text -->
<xsl:variable name="macosxbrand" select="'Mac OS X'"/>
<xsl:variable name="osxbrand" select="'OS X'"/>
<xsl:variable name="macbrand" select="'Mac'"/>
<xsl:template name="replacewithmacbrand">
	<xsl:param name="string"/>
	<xsl:choose>
		<xsl:when test="contains($string,$macosxbrand)">
			<xsl:variable name="newstr">
				<xsl:value-of select="substring-before($string,$macosxbrand)"/>
				<xsl:value-of select="$macbrand"/>
				<xsl:value-of select="substring-after($string,$macosxbrand)"/>
			</xsl:variable>
			<xsl:call-template name="replacewithmacbrand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:when test="contains($string,$osxbrand)">
			<xsl:variable name="newstr">
				<xsl:value-of select="substring-before($string,$osxbrand)"/>
				<xsl:value-of select="$macbrand"/>
				<xsl:value-of select="substring-after($string,$osxbrand)"/>
			</xsl:variable>
			<xsl:call-template name="replacewithmacbrand">
				<xsl:with-param name="string" select="$newstr"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$string"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

</xsl:stylesheet>
