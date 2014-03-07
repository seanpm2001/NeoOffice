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
 * Modified August 2013 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svtools.hxx"
#include "acceleratorexecute.hxx"

//===============================================
// includes

#ifndef __COM_SUN_STAR_FRAME_XMODULEMANAGER_HPP_
#include <com/sun/star/frame/XModuleManager.hpp>
#endif

#ifndef __COM_SUN_STAR_FRAME_XDESKTOP_HPP_
#include <com/sun/star/frame/XDesktop.hpp>
#endif

#ifndef __COM_SUN_STAR_UI_XUICONFIGURATIONMANAGER_HPP_
#include <com/sun/star/ui/XUIConfigurationManager.hpp>
#endif

#ifndef __COM_SUN_STAR_UI_XMODULEUICONFIGURATIONMANAGERSUPPLIER_HPP_
#include <com/sun/star/ui/XModuleUIConfigurationManagerSupplier.hpp>
#endif

#ifndef __COM_SUN_STAR_UI_XUICONFIGURATIONMANAGERSUPPLIER_HPP_
#include <com/sun/star/ui/XUIConfigurationManagerSupplier.hpp>
#endif

#ifndef __COM_SUN_STAR_AWT_XTOPWINDOW_HPP_
#include <com/sun/star/awt/XTopWindow.hpp>
#endif

#ifndef __COM_SUN_STAR_AWT_KEYMODIFIER_HPP_
#include <com/sun/star/awt/KeyModifier.hpp>
#endif

#ifndef __COM_SUN_STAR_UNO_SEQUENCE_HXX_
#include <com/sun/star/uno/Sequence.hxx>
#endif

#ifndef __COM_SUN_STAR_BEANS_PROPERTYVALUE_HPP_
#include <com/sun/star/beans/PropertyValue.hpp>
#endif

#ifndef __COM_SUN_STAR_LANG_DISPOSEDEXCEPTION_HPP_
#include <com/sun/star/lang/DisposedException.hpp>
#endif
#include <toolkit/helper/vclunohelper.hxx>

#include <vcl/window.hxx>
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>
#include <comphelper/uieventslogger.hxx>

#ifdef USE_JAVA
#include <com/sun/star/uno/XReference.hpp>
#endif	// USE_JAVA

//===============================================
// namespace

namespace  css = ::com::sun::star;

namespace svt
{

//===============================================
// definitions

//-----------------------------------------------
class SVT_DLLPRIVATE AsyncAccelExec
{
    public:

        //---------------------------------------
        /** creates a new instance of this class, which can be used
            one times only!

            This instance can be forced to execute it's internal set request
            asynchronous. After that it deletes itself !
         */
        static AsyncAccelExec* createOnShotInstance(const css::uno::Reference< css::frame::XDispatch >& xDispatch,
                                                    const css::util::URL&                               aURL     );

        void execAsync();

    private:

        //---------------------------------------
        /** @short  allow creation of instances of this class
                    by using our factory only!
         */
        SVT_DLLPRIVATE AsyncAccelExec(const css::uno::Reference< css::frame::XDispatch >& xDispatch,
                                      const css::util::URL&                               aURL     );

        DECL_DLLPRIVATE_LINK(impl_ts_asyncCallback, void*);

    private:

        ::vcl::EventPoster m_aAsyncCallback;
        css::uno::Reference< css::frame::XDispatch > m_xDispatch;
        css::util::URL m_aURL;
};

//-----------------------------------------------
AcceleratorExecute::AcceleratorExecute()
    : TMutexInit      (                                                     )
    , m_aAsyncCallback(LINK(this, AcceleratorExecute, impl_ts_asyncCallback))
{
}

//-----------------------------------------------
AcceleratorExecute::AcceleratorExecute(const AcceleratorExecute&)
    : TMutexInit      (                                                     )
    , m_aAsyncCallback(LINK(this, AcceleratorExecute, impl_ts_asyncCallback))
{
    // copy construction sint supported in real ...
    // but we need this ctor to init our async callback ...
}

//-----------------------------------------------
AcceleratorExecute::~AcceleratorExecute()
{
    // does nothing real
#ifdef USE_JAVA
    // Fix memory leak reported in the following NeOffice forum post by
    // disposing global and module accelerator managers:
    // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=64433#64433
    if ( m_xGlobalCfg.is() )
    {
        css::uno::Reference< css::uno::XReference > xReference( m_xGlobalCfg, css::uno::UNO_QUERY );
        if ( xReference.is() )
            xReference->dispose();
    }
    if ( m_xModuleCfg.is() )
    {
        css::uno::Reference< css::uno::XReference > xReference( m_xModuleCfg, css::uno::UNO_QUERY );
        if ( xReference.is() )
            xReference->dispose();
    }
#endif	// USE_JAVA
}

//-----------------------------------------------
AcceleratorExecute* AcceleratorExecute::createAcceleratorHelper()
{
    AcceleratorExecute* pNew = new AcceleratorExecute();
    return pNew;
}

//-----------------------------------------------
void AcceleratorExecute::init(const css::uno::Reference< css::lang::XMultiServiceFactory >& xSMGR,
                              const css::uno::Reference< css::frame::XFrame >&              xEnv )
{
    // SAFE -> ----------------------------------
    ::osl::ResettableMutexGuard aLock(m_aLock);

    // take over the uno service manager
    m_xSMGR = xSMGR;

    // specify our internal dispatch provider
    // frame or desktop?! => document or global config.
    sal_Bool bDesktopIsUsed = sal_False;
             m_xDispatcher  = css::uno::Reference< css::frame::XDispatchProvider >(xEnv, css::uno::UNO_QUERY);
    if (!m_xDispatcher.is())
    {
        aLock.clear();
        // <- SAFE ------------------------------

        css::uno::Reference< css::frame::XDispatchProvider > xDispatcher(
                            xSMGR->createInstance(::rtl::OUString::createFromAscii("com.sun.star.frame.Desktop")),
                            css::uno::UNO_QUERY_THROW);

        // SAFE -> ------------------------------
        aLock.reset();

        m_xDispatcher  = xDispatcher;
        bDesktopIsUsed = sal_True;
    }

    aLock.clear();
    // <- SAFE ----------------------------------

    // open all needed configuration objects
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xGlobalCfg;
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xModuleCfg;
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xDocCfg   ;

    // global cfg
    xGlobalCfg = AcceleratorExecute::st_openGlobalConfig(xSMGR);
    if (!bDesktopIsUsed)
    {
        // module cfg
        xModuleCfg = AcceleratorExecute::st_openModuleConfig(xSMGR, xEnv);

        // doc cfg
        css::uno::Reference< css::frame::XController > xController;
        css::uno::Reference< css::frame::XModel >      xModel;
        xController = xEnv->getController();
        if (xController.is())
            xModel = xController->getModel();
        if (xModel.is())
            xDocCfg = AcceleratorExecute::st_openDocConfig(xModel);
    }

    // SAFE -> ------------------------------
    aLock.reset();

    m_xGlobalCfg = xGlobalCfg;
    m_xModuleCfg = xModuleCfg;
    m_xDocCfg    = xDocCfg   ;

    aLock.clear();
    // <- SAFE ----------------------------------
}

//-----------------------------------------------
sal_Bool AcceleratorExecute::execute(const KeyCode& aVCLKey)
{
    css::awt::KeyEvent aAWTKey = AcceleratorExecute::st_VCLKey2AWTKey(aVCLKey);
    return execute(aAWTKey);
}

//-----------------------------------------------
sal_Bool AcceleratorExecute::execute(const css::awt::KeyEvent& aAWTKey)
{
    ::rtl::OUString sCommand = impl_ts_findCommand(aAWTKey);

    // No Command found? Do nothing! User isnt interested on any error handling .-)
    if (!sCommand.getLength())
        return sal_False;

    // SAFE -> ----------------------------------
    ::osl::ResettableMutexGuard aLock(m_aLock);

    css::uno::Reference< css::frame::XDispatchProvider > xProvider = m_xDispatcher;

    aLock.clear();
    // <- SAFE ----------------------------------

    // convert command in URL structure
    css::uno::Reference< css::util::XURLTransformer > xParser = impl_ts_getURLParser();
#ifdef USE_JAVA
    // Fix crashing reported in the following LibreOffice bug:
    // https://bugs.freedesktop.org/show_bug.cgi?id=69162
    if ( !xParser.is() )
        return sal_False;
#endif	// USE_JAVA
    css::util::URL aURL;
    aURL.Complete = sCommand;
    xParser->parseStrict(aURL);

    // ask for dispatch object
    css::uno::Reference< css::frame::XDispatch > xDispatch = xProvider->queryDispatch(aURL, ::rtl::OUString(), 0);
    sal_Bool bRet = xDispatch.is();
    if ( bRet )
    {
        if(::comphelper::UiEventsLogger::isEnabled() && m_xSMGR.is() && m_xDispatcher.is()) //#i88653#
        {
            try
            {
                css::uno::Reference< css::frame::XModuleManager > xModuleDetection(
                    m_xSMGR->createInstance(::rtl::OUString::createFromAscii("com.sun.star.frame.ModuleManager")),
                    css::uno::UNO_QUERY_THROW);

                const ::rtl::OUString sModule = xModuleDetection->identify(m_xDispatcher);
                css::uno::Sequence<css::beans::PropertyValue> source;
                ::comphelper::UiEventsLogger::appendDispatchOrigin(source, sModule, ::rtl::OUString::createFromAscii("AcceleratorExecute"));
                ::comphelper::UiEventsLogger::logDispatch(aURL, source);
            }
            catch(const css::uno::Exception&)
                { }
        }
        // Note: Such instance can be used one times only and destroy itself afterwards .-)
        AsyncAccelExec* pExec = AsyncAccelExec::createOnShotInstance(xDispatch, aURL);
        pExec->execAsync();
    }

    return bRet;
}

//-----------------------------------------------
css::awt::KeyEvent AcceleratorExecute::st_VCLKey2AWTKey(const KeyCode& aVCLKey)
{
    css::awt::KeyEvent aAWTKey;
    aAWTKey.Modifiers = 0;
    aAWTKey.KeyCode   = (sal_Int16)aVCLKey.GetCode();
    
	if (aVCLKey.IsShift())
        aAWTKey.Modifiers |= css::awt::KeyModifier::SHIFT; 
	if (aVCLKey.IsMod1())
        aAWTKey.Modifiers |= css::awt::KeyModifier::MOD1; 
	if (aVCLKey.IsMod2())
        aAWTKey.Modifiers |= css::awt::KeyModifier::MOD2;
    
    return aAWTKey;    
}

//-----------------------------------------------
KeyCode AcceleratorExecute::st_AWTKey2VCLKey(const css::awt::KeyEvent& aAWTKey)
{
    sal_Bool bShift = ((aAWTKey.Modifiers & css::awt::KeyModifier::SHIFT) == css::awt::KeyModifier::SHIFT );
    sal_Bool bMod1  = ((aAWTKey.Modifiers & css::awt::KeyModifier::MOD1 ) == css::awt::KeyModifier::MOD1  );
    sal_Bool bMod2  = ((aAWTKey.Modifiers & css::awt::KeyModifier::MOD2 ) == css::awt::KeyModifier::MOD2  );
    USHORT   nKey   = (USHORT)aAWTKey.KeyCode;
    
    return KeyCode(nKey, bShift, bMod1, bMod2);
}
//-----------------------------------------------
::rtl::OUString AcceleratorExecute::findCommand(const css::awt::KeyEvent& aKey)
{
    return impl_ts_findCommand(aKey);
}
//-----------------------------------------------
::rtl::OUString AcceleratorExecute::impl_ts_findCommand(const css::awt::KeyEvent& aKey)
{
    // SAFE -> ----------------------------------
    ::osl::ResettableMutexGuard aLock(m_aLock);
    
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xGlobalCfg = m_xGlobalCfg;
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xModuleCfg = m_xModuleCfg;
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xDocCfg    = m_xDocCfg   ;
    
    aLock.clear();    
    // <- SAFE ----------------------------------

    ::rtl::OUString sCommand;

    try
    {
        if (xDocCfg.is())
            sCommand = xDocCfg->getCommandByKeyEvent(aKey);
        if (sCommand.getLength())
            return sCommand;
    }
    catch(const css::container::NoSuchElementException&)
        {}

    try
    {
        if (xModuleCfg.is())
            sCommand = xModuleCfg->getCommandByKeyEvent(aKey);
        if (sCommand.getLength())
            return sCommand;
    }
    catch(const css::container::NoSuchElementException&)
        {}

    try
    {
        if (xGlobalCfg.is())
            sCommand = xGlobalCfg->getCommandByKeyEvent(aKey);
        if (sCommand.getLength())
            return sCommand;
    }
    catch(const css::container::NoSuchElementException&)
        {}
        
    // fall back to functional key codes
    if( aKey.Modifiers == 0 )
    {
        switch( aKey.KeyCode )
        {
        case com::sun::star::awt::Key::DELETE_TO_BEGIN_OF_LINE:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:DelToStartOfLine" ) );
            
        case com::sun::star::awt::Key::DELETE_TO_END_OF_LINE:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:DelToEndOfLine" ) );
            
        case com::sun::star::awt::Key::DELETE_TO_BEGIN_OF_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:DelToStartOfPara" ) );
            
        case com::sun::star::awt::Key::DELETE_TO_END_OF_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:DelToEndOfPara" ) );
            
        case com::sun::star::awt::Key::DELETE_WORD_BACKWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:DelToStartOfWord" ) );
            
        case com::sun::star::awt::Key::DELETE_WORD_FORWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:DelToEndOfWord" ) );
            
        case com::sun::star::awt::Key::INSERT_LINEBREAK:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:InsertLinebreak" ) );
            
        case com::sun::star::awt::Key::INSERT_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:InsertPara" ) );
            
        case com::sun::star::awt::Key::MOVE_WORD_BACKWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToPrevWord" ) );
            
        case com::sun::star::awt::Key::MOVE_WORD_FORWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToNextWord" ) );
            
        case com::sun::star::awt::Key::MOVE_TO_BEGIN_OF_LINE:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToStartOfLine" ) );
            
        case com::sun::star::awt::Key::MOVE_TO_END_OF_LINE:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToEndOfLine" ) );
            
        case com::sun::star::awt::Key::MOVE_TO_BEGIN_OF_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToStartOfPara" ) );
            
        case com::sun::star::awt::Key::MOVE_TO_END_OF_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToEndOfPara" ) );
            
        case com::sun::star::awt::Key::MOVE_TO_BEGIN_OF_DOCUMENT:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToStartOfDoc" ) );
            
        case com::sun::star::awt::Key::MOVE_TO_END_OF_DOCUMENT:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GoToEndOfDoc" ) );
            
        case com::sun::star::awt::Key::SELECT_BACKWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:CharLeftSel" ) );
            
        case com::sun::star::awt::Key::SELECT_FORWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:CharRightSel" ) );
            
        case com::sun::star::awt::Key::SELECT_WORD_BACKWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:WordLeftSel" ) );
            
        case com::sun::star::awt::Key::SELECT_WORD_FORWARD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:WordRightSel" ) );
            
        case com::sun::star::awt::Key::SELECT_WORD:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:SelectWord" ) );
            
        case com::sun::star::awt::Key::SELECT_LINE:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "" ) );
            
        case com::sun::star::awt::Key::SELECT_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:SelectText" ) );
            
        case com::sun::star::awt::Key::SELECT_TO_BEGIN_OF_LINE:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:StartOfLineSel" ) );
            
        case com::sun::star::awt::Key::SELECT_TO_END_OF_LINE:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:EndOfLineSel" ) );

        case com::sun::star::awt::Key::SELECT_TO_BEGIN_OF_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:StartOfParaSel" ) );
            
        case com::sun::star::awt::Key::SELECT_TO_END_OF_PARAGRAPH:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:EndOfParaSel" ) );
            
        case com::sun::star::awt::Key::SELECT_TO_BEGIN_OF_DOCUMENT:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:StartOfDocumentSel" ) );
            
        case com::sun::star::awt::Key::SELECT_TO_END_OF_DOCUMENT:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:EndOfDocumentSel" ) );
            
        case com::sun::star::awt::Key::SELECT_ALL:
            return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:SelectAll" ) );
        default:
            break;
        }
    }

    return ::rtl::OUString();
}

//-----------------------------------------------
css::uno::Reference< css::ui::XAcceleratorConfiguration > AcceleratorExecute::st_openGlobalConfig(const css::uno::Reference< css::lang::XMultiServiceFactory >& xSMGR)
{
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xAccCfg(
        xSMGR->createInstance(::rtl::OUString::createFromAscii("com.sun.star.ui.GlobalAcceleratorConfiguration")),
        css::uno::UNO_QUERY_THROW);
    return xAccCfg;
}

//-----------------------------------------------
css::uno::Reference< css::ui::XAcceleratorConfiguration > AcceleratorExecute::st_openModuleConfig(const css::uno::Reference< css::lang::XMultiServiceFactory >& xSMGR ,
                                                                                                   const css::uno::Reference< css::frame::XFrame >&              xFrame)
{
    css::uno::Reference< css::frame::XModuleManager > xModuleDetection(
        xSMGR->createInstance(::rtl::OUString::createFromAscii("com.sun.star.frame.ModuleManager")),
        css::uno::UNO_QUERY_THROW);

    ::rtl::OUString sModule;
    try
    {
        sModule = xModuleDetection->identify(xFrame);
    }
    catch(const css::uno::RuntimeException&)
        { throw; }
    catch(const css::uno::Exception&)
        { return css::uno::Reference< css::ui::XAcceleratorConfiguration >(); }

    css::uno::Reference< css::ui::XModuleUIConfigurationManagerSupplier > xUISupplier(
        xSMGR->createInstance(::rtl::OUString::createFromAscii("com.sun.star.ui.ModuleUIConfigurationManagerSupplier")),
        css::uno::UNO_QUERY_THROW);

    css::uno::Reference< css::ui::XUIConfigurationManager >   xUIManager = xUISupplier->getUIConfigurationManager(sModule);
    css::uno::Reference< css::ui::XAcceleratorConfiguration > xAccCfg    (xUIManager->getShortCutManager(), css::uno::UNO_QUERY_THROW);
    return xAccCfg;
}

//-----------------------------------------------
css::uno::Reference< css::ui::XAcceleratorConfiguration > AcceleratorExecute::st_openDocConfig(const css::uno::Reference< css::frame::XModel >& xModel)
{
    css::uno::Reference< css::ui::XAcceleratorConfiguration >       xAccCfg;
    css::uno::Reference< css::ui::XUIConfigurationManagerSupplier > xUISupplier(xModel, css::uno::UNO_QUERY);
    if (xUISupplier.is())
    {
        css::uno::Reference< css::ui::XUIConfigurationManager >     xUIManager = xUISupplier->getUIConfigurationManager();
        xAccCfg.set(xUIManager->getShortCutManager(), css::uno::UNO_QUERY_THROW);
    }
    return xAccCfg;
}

//-----------------------------------------------
css::uno::Reference< css::util::XURLTransformer > AcceleratorExecute::impl_ts_getURLParser()
{
    // SAFE -> ----------------------------------
    ::osl::ResettableMutexGuard aLock(m_aLock);

    if (m_xURLParser.is())
        return m_xURLParser;
#ifdef USE_JAVA
    // Fix crashing reported in the following LibreOffice bug:
    // https://bugs.freedesktop.org/show_bug.cgi?id=69162
    else if (!m_xSMGR.is())
        return m_xURLParser;
#endif	// USE_JAVA
    css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR = m_xSMGR;

    aLock.clear();
    // <- SAFE ----------------------------------

    css::uno::Reference< css::util::XURLTransformer > xParser(
                xSMGR->createInstance(::rtl::OUString::createFromAscii("com.sun.star.util.URLTransformer")),
                css::uno::UNO_QUERY_THROW);

    // SAFE -> ----------------------------------
    aLock.reset();
    m_xURLParser = xParser;
    aLock.clear();
    // <- SAFE ----------------------------------

    return xParser;
}

//-----------------------------------------------
IMPL_LINK(AcceleratorExecute, impl_ts_asyncCallback, void*, EMPTYARG)
{
    // replaced by AsyncAccelExec!
    return 0;
}

//-----------------------------------------------
AsyncAccelExec::AsyncAccelExec(const css::uno::Reference< css::frame::XDispatch >& xDispatch,
                               const css::util::URL&                               aURL     )
    : m_aAsyncCallback(LINK(this, AsyncAccelExec, impl_ts_asyncCallback))
    , m_xDispatch     (xDispatch                                        )
    , m_aURL          (aURL                                             )
{
}

//-----------------------------------------------
AsyncAccelExec* AsyncAccelExec::createOnShotInstance(const css::uno::Reference< css::frame::XDispatch >& xDispatch,
                                                     const css::util::URL&                               aURL     )
{
    AsyncAccelExec* pExec = new AsyncAccelExec(xDispatch, aURL);
    return pExec;
}

//-----------------------------------------------
void AsyncAccelExec::execAsync()
{
    m_aAsyncCallback.Post(0);
}

//-----------------------------------------------
IMPL_LINK(AsyncAccelExec, impl_ts_asyncCallback, void*,)
{
    if (! m_xDispatch.is())
        return 0;

    try
    {
        m_xDispatch->dispatch(m_aURL, css::uno::Sequence< css::beans::PropertyValue >());
    }
    catch(const css::lang::DisposedException&)
        {}
    catch(const css::uno::RuntimeException&)
        { throw; }
    catch(const css::uno::Exception&)
        {}

    delete this;

    return 0;
}

} // namespace svt
