/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified July 2006 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_fpicker.hxx"
#include "sal/types.h"
#include "rtl/ustring.hxx"

#ifndef _CPPUHELPER_IMPLEMENTATIONENTRY_HXX_
#include "cppuhelper/implementationentry.hxx"
#endif
#include "com/sun/star/lang/XMultiComponentFactory.hpp"

#ifdef WNT
#include <tools/prewin.h>
#include <tools/postwin.h>
#include <odma_lib.hxx>
#endif

#include "svtools/miscopt.hxx"
#include "svtools/pickerhistoryaccess.hxx"

#ifndef _SV_APP_HXX
#include "vcl/svapp.hxx"
#endif

namespace css = com::sun::star;

using css::uno::Reference;
using css::uno::Sequence;
using rtl::OUString;

/*
 * FilePicker implementation.
 */
static OUString FilePicker_getSystemPickerServiceName()
{
#ifdef UNX
	OUString aDesktopEnvironment (Application::GetDesktopEnvironment());
	if (aDesktopEnvironment.equalsIgnoreAsciiCaseAscii ("gnome"))
		return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.GtkFilePicker"));
	else if (aDesktopEnvironment.equalsIgnoreAsciiCaseAscii ("kde"))
		return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.KDEFilePicker"));
    else if (aDesktopEnvironment.equalsIgnoreAsciiCaseAscii ("macosx"))
        return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.AquaFilePicker"));
#endif
#ifdef WNT
	if (SvtMiscOptions().TryODMADialog() && ::odma::DMSsAvailable()) {
		return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.ODMAFilePicker"));
	}
	return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.Win32FilePicker"));
#endif
	return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.SystemFilePicker"));
}

static Reference< css::uno::XInterface > FilePicker_createInstance (
	Reference< css::uno::XComponentContext > const & rxContext)
{
	Reference< css::uno::XInterface > xResult;
	if (rxContext.is())
	{
		Reference< css::lang::XMultiComponentFactory > xFactory (rxContext->getServiceManager());
		if (xFactory.is())
		{
#ifndef USE_JAVA
			if (SvtMiscOptions().UseSystemFileDialog())
			{
#endif	// !USE_JAVA
				try
				{
					xResult = xFactory->createInstanceWithContext (
						FilePicker_getSystemPickerServiceName(),
						rxContext);
				}
				catch (css::uno::Exception const &)
				{
					// Handled below (see @ fallback).
				}
#ifndef USE_JAVA
			}
#endif	// !USE_JAVA
			if (!xResult.is())
			{
				// Always fall back to OfficeFilePicker.
				xResult = xFactory->createInstanceWithContext (
					OUString (RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.OfficeFilePicker")),
					rxContext);
			}
			if (xResult.is())
			{
				// Add to FilePicker history.
				svt::addFilePicker (xResult);
			}
		}
	}
	return xResult;
}

static OUString FilePicker_getImplementationName()
{
	return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.comp.fpicker.FilePicker"));
}

static Sequence< OUString > FilePicker_getSupportedServiceNames()
{
	Sequence< OUString > aServiceNames(1);
	aServiceNames.getArray()[0] =
		OUString (RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FilePicker"));
	return aServiceNames;
}

/*
 * FolderPicker implementation.
 */
static OUString FolderPicker_getSystemPickerServiceName()
{
	OUString aDesktopEnvironment (Application::GetDesktopEnvironment());
#ifdef UNX
	if (aDesktopEnvironment.equalsIgnoreAsciiCaseAscii ("gnome"))
		return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.GtkFolderPicker"));
	else if (aDesktopEnvironment.equalsIgnoreAsciiCaseAscii ("kde"))
		return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.KDEFolderPicker"));
    else if (aDesktopEnvironment.equalsIgnoreAsciiCaseAscii ("macosx"))
        return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.AquaFolderPicker"));
#endif
#ifdef WNT
	if (SvtMiscOptions().TryODMADialog() && ::odma::DMSsAvailable()) {
		return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.ODMAFolderPicker"));
	}
#endif
	return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.ui.dialogs.SystemFolderPicker"));
}

static Reference< css::uno::XInterface > FolderPicker_createInstance (
	Reference< css::uno::XComponentContext > const & rxContext)
{
	Reference< css::uno::XInterface > xResult;
	if (rxContext.is())
	{
		Reference< css::lang::XMultiComponentFactory > xFactory (rxContext->getServiceManager());
		if (xFactory.is())
		{
#ifndef USE_JAVA
			if (SvtMiscOptions().UseSystemFileDialog())
			{
#endif	// USE_JAVA
				try
				{
					xResult = xFactory->createInstanceWithContext (
						FolderPicker_getSystemPickerServiceName(),
						rxContext);
				}
				catch (css::uno::Exception const &)
				{
					// Handled below (see @ fallback).
				}
#ifndef USE_JAVA
			}
#endif	// USE_JAVA
			if (!xResult.is())
			{
				// Always fall back to OfficeFolderPicker.
				xResult = xFactory->createInstanceWithContext (
					OUString (RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.OfficeFolderPicker")),
					rxContext);
			}
			if (xResult.is())
			{
				// Add to FolderPicker history.
				svt::addFolderPicker (xResult);
			}
		}
	}
	return xResult;
}

static OUString FolderPicker_getImplementationName()
{
	return OUString (RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.comp.fpicker.FolderPicker"));
}

static Sequence< OUString > FolderPicker_getSupportedServiceNames()
{
	Sequence< OUString > aServiceNames(1);
	aServiceNames.getArray()[0] =
		OUString (RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FolderPicker"));
	return aServiceNames;
}

/*
 * Implementation entries.
 */
static cppu::ImplementationEntry g_entries[] =
{
	{
		FilePicker_createInstance,
		FilePicker_getImplementationName,
		FilePicker_getSupportedServiceNames,
		cppu::createSingleComponentFactory, 0, 0
	},
	{
		FolderPicker_createInstance,
		FolderPicker_getImplementationName,
		FolderPicker_getSupportedServiceNames,
		cppu::createSingleComponentFactory, 0, 0
	},
	{ 0, 0, 0, 0, 0, 0 }
};

/*
 * Public (exported) interface.
 */
extern "C"
{
SAL_DLLPUBLIC_EXPORT void SAL_CALL component_getImplementationEnvironment (
	const sal_Char ** ppEnvTypeName, uno_Environment ** /* ppEnv */)
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

SAL_DLLPUBLIC_EXPORT sal_Bool SAL_CALL component_writeInfo (
	void * pServiceManager, void * pRegistryKey)
{
	return cppu::component_writeInfoHelper (
		pServiceManager, pRegistryKey, g_entries);
}

SAL_DLLPUBLIC_EXPORT void * SAL_CALL component_getFactory (
	const sal_Char * pImplementationName, void * pServiceManager, void * pRegistryKey)
{
	return cppu::component_getFactoryHelper (
		pImplementationName, pServiceManager, pRegistryKey, g_entries);
}

} // extern "C"
