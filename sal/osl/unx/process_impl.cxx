/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Sun Microsystems Inc., October, 2000
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2000 by Sun Microsystems, Inc.
 *  901 San Antonio Road, Palo Alto, CA 94303, USA
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *  
 *  =================================================
 *  Modified June 2004 by Patrick Luby. SISSL Removed. NeoOffice is
 *  distributed under GPL only under modification term 3 of the LGPL.
 *
 *  Contributor(s): _______________________________________
 *
 ************************************************************************/

 #ifndef _STRINGS_H
 #include <strings.h>
 #endif
 
 #ifndef _STDLIB_H
 #include <stdlib.h>
 #endif
 
 #ifndef _OSL_PROCESS_H_
 #include <osl/process.h>
 #endif
 
 #ifndef _RTL_USTRING_HXX_
 #include <rtl/ustring.hxx>
 #endif
 
 #ifndef _OSL_THREAD_H_
 #include <osl/thread.h>
 #endif
 
 #ifndef _OSL_FILE_HXX_
 #include <osl/file.hxx>
 #endif
 
 #ifndef _OSL_UUNXAPI_HXX_
 #include "uunxapi.hxx"
 #endif
 
 #ifndef _OSL_FILE_PATH_HELPER_HXX_
 #include "file_path_helper.hxx"
 #endif
 
 #include <string.h>

 #ifdef MACOSX
 /* All Mac OS X paths are UTF-8 */
 #define osl_getThreadTextEncoding() RTL_TEXTENCODING_UTF8
 #endif

 //------------------------------------
 // forward 
 //------------------------------------
 
 extern "C" sal_Char *getCmdLine();
 extern "C" char* osl_impl_getExecutableName(char * buffer, size_t n);
 
 //------------------------------------
 // private stuff 
 //------------------------------------
 
 namespace /* private */
 {
 
	const rtl::OUString UNICHAR_SLASH = rtl::OUString::createFromAscii("/");
  	
	//-------------------------------------------------------
 	// if the command line arg 0 contains a '/' somewhere it 
	// has been probably invoked relatively to the current 
	// working dir
	//-------------------------------------------------------
	
 	inline bool is_relative_to_cwd(const rtl::OUString& path)
	{
		return (path.indexOf(UNICHAR_SLASH) > -1);
	}	
	
	//---------------------------------------------------------
	// make a relative path absolute using the current working 
	// directory
	// no return value because osl_getExecutableFile either
	// returns osl_Process_E_None or osl_Process_E_Unknown
	//---------------------------------------------------------

	void make_absolute_to_cwd(const rtl::OUString& relative_path, rtl::OUString& absolute_path)
	{
		rtl::OUString cwd_url;		
 		
		if (osl_Process_E_None == osl_getProcessWorkingDir(&cwd_url.pData))
		{
			rtl::OUString cwd;
			osl::FileBase::RC rc = osl::FileBase::getSystemPathFromFileURL(cwd_url, cwd);
			OSL_ASSERT(osl::FileBase::E_None == rc);
			
			osl::systemPathMakeAbsolutePath(cwd, relative_path, absolute_path);
		}
	}
	
	//---------------------------------------------------------
	// search for the file using the PATH environment variable
	// no return value because osl_getExecutableFile either
	// returns osl_Process_E_None or osl_Process_E_Unknown
	//---------------------------------------------------------
	
	void find_in_PATH(const rtl::OUString& file_path, rtl::OUString& path_found)
	{
		rtl::OUString   PATH   = rtl::OUString::createFromAscii("PATH");
		rtl::OUString   env_path;		
		
		if (osl_getEnvironment(PATH.pData, &env_path.pData) == osl_Process_E_None)
			osl::searchPath(file_path, env_path, path_found);		
	}
	
 } // namespace /private */
 
 
/***************************************
  osl_getExecutableFile
 **************************************/
 
oslProcessError SAL_CALL osl_getExecutableFile(rtl_uString** ppustrFile)
{	
	sal_Char*       p_cmdline = getCmdLine();	
	oslProcessError osl_error = osl_Process_E_Unknown;
	
	if (p_cmdline != 0)
	{
		rtl::OUString cmdline(p_cmdline, strlen(p_cmdline), osl_getThreadTextEncoding());
		
		free(p_cmdline);
		
		rtl::OUString abs_path; 
				
		if (osl::systemPathIsRelativePath(cmdline))
		{				
			if (is_relative_to_cwd(cmdline))
				make_absolute_to_cwd(cmdline, abs_path);
			else
				find_in_PATH(cmdline, abs_path);					
		}
		else
		{
			abs_path = cmdline;
		}
		
		rtl::OUString path_resolved;
		
		if ((abs_path.getLength() > 0) && osl::realpath(abs_path, path_resolved))
		{
			rtl::OUString furl;
			osl::FileBase::RC rc = osl::FileBase::getFileURLFromSystemPath(path_resolved.pData, furl);
			OSL_ASSERT(osl::FileBase::E_None == rc);
			
			rtl_uString_assign(ppustrFile, furl.pData);
			osl_error = osl_Process_E_None;
		}		
	}
	return osl_error;
}

/***************************************
 Necessary for signal.c
 **************************************/
 
char* osl_impl_getExecutableName(char * buffer, size_t n)
{
	sal_Char*     p_cmdline = getCmdLine();
	char*         pChrRet   = NULL;
	
	if (p_cmdline != 0)
	{
		rtl::OUString cmdline(p_cmdline, strlen(p_cmdline), osl_getThreadTextEncoding());
		
		free(p_cmdline);
		
		rtl::OUString exename_u;	
		osl::systemPathGetFileNameOrLastDirectoryPart(cmdline, exename_u);
		
		rtl::OString exename_a = rtl::OUStringToOString(exename_u, osl_getThreadTextEncoding());
		
		if (exename_a.getLength() < n)
		{
			strcpy(buffer, exename_a.getStr());
			pChrRet = buffer;
		}
	}
	return pChrRet;
}
