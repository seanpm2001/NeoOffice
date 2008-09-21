/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to
 *  the terms of GNU General Public License Version 2.1.
 *
 *
 *    GNU General Public License Version 2.1
 *    =============================================
 *    Copyright 2005 by Sun Microsystems, Inc.
 *    901 San Antonio Road, Palo Alto, CA 94303, USA
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public
 *    License version 2.1, as published by the Free Software Foundation.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 *
 *    Modified December 2006 by Patrick Luby. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_ucbhelper.hxx"

/**************************************************************************
								TODO
 **************************************************************************

 *************************************************************************/

#include <utility>
#include <vector>
#include <list>

#ifndef _OSL_MUTEX_HXX_
#include <osl/mutex.hxx>
#endif
#ifndef _RTL_REF_HXX_
#include <rtl/ref.hxx>
#endif
#ifndef _OSL_SOCKET_HXX_
#include <osl/socket.hxx>
#endif
#ifndef _RTL_USTRBUF_HXX_
#include <rtl/ustrbuf.hxx>
#endif
#ifndef _COM_SUN_STAR_CONTAINER_XNAMEACCESS_HPP_
#include <com/sun/star/container/XNameAccess.hpp>
#endif
#ifndef _COM_SUN_STAR_LANG_XMULTISERVICEFACTORY_HPP_
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#endif
#ifndef _COM_SUN_STAR_UTIL_XCHANGESLISTENER_HPP_
#include <com/sun/star/util/XChangesListener.hpp>
#endif
#ifndef _COM_SUN_STAR_UTIL_XCHANGESNOTIFIER_HPP_
#include <com/sun/star/util/XChangesNotifier.hpp>
#endif
#ifndef _CPPUHELPER_IMPLBASE1_HXX_
#include <cppuhelper/implbase1.hxx>
#endif
#ifndef _UCBHELPER_PROXYDECIDER_HXX
#include "ucbhelper/proxydecider.hxx"
#endif

#ifdef USE_JAVA

#include <premac.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <postmac.h>

#endif	// USE_JAVA 

using namespace com::sun::star;
using namespace ucbhelper;

#define CONFIG_ROOT_KEY     "org.openoffice.Inet/Settings"
#define PROXY_TYPE_KEY      "ooInetProxyType"
#define NO_PROXY_LIST_KEY   "ooInetNoProxy"
#define HTTP_PROXY_NAME_KEY "ooInetHTTPProxyName"
#define HTTP_PROXY_PORT_KEY "ooInetHTTPProxyPort"
#define FTP_PROXY_NAME_KEY  "ooInetFTPProxyName"
#define FTP_PROXY_PORT_KEY  "ooInetFTPProxyPort"

//=========================================================================
namespace ucbhelper
{

//=========================================================================
namespace proxydecider_impl
{

// A simple case ignoring wildcard matcher.
class WildCard
{
private:
    rtl::OString m_aWildString;

public:
    WildCard( const rtl::OUString& rWildCard )
    : m_aWildString(
        rtl::OUStringToOString(
            rWildCard, RTL_TEXTENCODING_UTF8 ).toAsciiLowerCase() ) {}

    bool Matches( const rtl::OUString & rStr ) const;
};

//=========================================================================
typedef std::pair< WildCard, WildCard > NoProxyListEntry;

//=========================================================================

class HostnameCache
{
    typedef std::pair< rtl::OUString, rtl::OUString > HostListEntry;

    std::list< HostListEntry >     m_aHostList;
    sal_uInt32                     m_nCapacity;                           

public:
    explicit HostnameCache( sal_uInt32 nCapacity )
        : m_nCapacity( nCapacity ) {}

    bool get( const rtl::OUString & rKey, rtl::OUString & rValue ) const
    {
        std::list< HostListEntry >::const_iterator it
            = m_aHostList.begin();
        const std::list< HostListEntry >::const_iterator end
            = m_aHostList.end();

        while ( it != end )
        {
            if ( (*it).first == rKey )
            {
                rValue = (*it).second; 
                return true;
            }
            it++;
        }
        return false;
    }

    void put( const rtl::OUString & rKey, const rtl::OUString & rValue )
    {
        if ( m_aHostList.size() == m_nCapacity )
            m_aHostList.resize( m_nCapacity / 2 );

        m_aHostList.push_front( HostListEntry( rKey, rValue ) ); 
    }
};

//=========================================================================
class InternetProxyDecider_Impl :
    public cppu::WeakImplHelper1< util::XChangesListener >
{
    mutable osl::Mutex                       m_aMutex;
    InternetProxyServer                      m_aHttpProxy;
    InternetProxyServer                      m_aFtpProxy;
    const InternetProxyServer                m_aEmptyProxy;
#ifdef USE_JAVA
    mutable InternetProxyServer              m_aFtpSystemProxy;
    mutable InternetProxyServer              m_aHttpSystemProxy;
    mutable InternetProxyServer              m_aHttpsSystemProxy;
#endif	// USE_JAVA 
    sal_Int32                                m_nProxyType;
    uno::Reference< util::XChangesNotifier > m_xNotifier;
    std::vector< NoProxyListEntry >          m_aNoProxyList;
    mutable HostnameCache                    m_aHostnames;

private:
    bool shouldUseProxy( const rtl::OUString & rHost,
                         sal_Int32 nPort,
                         bool bUseFullyQualified ) const;
public:
    InternetProxyDecider_Impl(
        const uno::Reference< lang::XMultiServiceFactory >& rxSMgr );
    virtual ~InternetProxyDecider_Impl();

    static rtl::Reference< InternetProxyDecider_Impl > createInstance(
        const uno::Reference< lang::XMultiServiceFactory >& rxSMgr );

    void dispose();

    const InternetProxyServer & getProxy( const rtl::OUString & rProtocol,
                                          const rtl::OUString & rHost,
                                          sal_Int32 nPort ) const;

    // XChangesListener
    virtual void SAL_CALL changesOccurred( const util::ChangesEvent& Event )
        throw( uno::RuntimeException );

    // XEventListener ( base of XChangesLisetenr )
    virtual void SAL_CALL disposing( const lang::EventObject& Source )
        throw( uno::RuntimeException );

private:
    void setNoProxyList( const rtl::OUString & rNoProxyList );
};

//=========================================================================
//=========================================================================
//
// WildCard Implementation.
//
//=========================================================================
//=========================================================================

bool WildCard::Matches( const rtl::OUString& rString ) const
{
    rtl::OString aString
        = rtl::OUStringToOString(
                    rString, RTL_TEXTENCODING_UTF8 ).toAsciiLowerCase();
    const char * pStr  = aString.getStr();
    const char * pWild = m_aWildString.getStr();

    int pos  = 0;
    int flag = 0;

    while ( *pWild || flag )
    {
        switch ( *pWild )
        {
            case '?':
                if ( *pStr == '\0' )
                    return 0;
                break;

            default:
                if ( ( *pWild == '\\' ) && ( ( *( pWild + 1 ) == '?' )
                                             || ( *( pWild + 1 ) == '*') ) )
                    pWild++;
                if ( *pWild != *pStr )
                    if ( !pos )
                        return 0;
                    else
                        pWild += pos;
                else
                    break;

                // Note: fall-thru's are intended!

            case '*':
                while ( *pWild == '*' )
                    pWild++;
                if ( *pWild == '\0' )
                    return 1;
                flag = 1;
                pos  = 0;
                if ( *pStr == '\0' )
                    return ( *pWild == '\0' );
                while ( *pStr && *pStr != *pWild )
                {
                    if ( *pWild == '?' ) {
                        pWild++;
                        while ( *pWild == '*' )
                            pWild++;
                    }
                    pStr++;
                    if ( *pStr == '\0' )
                        return ( *pWild == '\0' );
                }
                break;
        }
        if ( *pWild != '\0' )
            pWild++;
        if ( *pStr != '\0' )
            pStr++;
        else
            flag = 0;
        if ( flag )
            pos--;
    }
    return ( *pStr == '\0' ) && ( *pWild == '\0' );
}

//=========================================================================
//=========================================================================
//
// InternetProxyDecider_Impl Implementation.
//
//=========================================================================
//=========================================================================

InternetProxyDecider_Impl::InternetProxyDecider_Impl(
    const uno::Reference< lang::XMultiServiceFactory >& rxSMgr )
    : m_nProxyType( 0 ),
      m_aHostnames( 256 ) // cache size
{
    try
    {
        //////////////////////////////////////////////////////////////
        // Read proxy configuration from config db.
        //////////////////////////////////////////////////////////////

        uno::Reference< lang::XMultiServiceFactory > xConfigProv(
                rxSMgr->createInstance(
                    rtl::OUString::createFromAscii(
                        "com.sun.star.configuration.ConfigurationProvider" ) ),
                uno::UNO_QUERY );

        uno::Sequence< uno::Any > aArguments( 1 );
        aArguments[ 0 ] <<= rtl::OUString::createFromAscii( CONFIG_ROOT_KEY );

        uno::Reference< uno::XInterface > xInterface(
                    xConfigProv->createInstanceWithArguments(
                        rtl::OUString::createFromAscii(
                            "com.sun.star.configuration.ConfigurationAccess" ),
                    aArguments ) );

        OSL_ENSURE( xInterface.is(),
                    "InternetProxyDecider - No config access!" );

        if ( xInterface.is() )
        {
            uno::Reference< container::XNameAccess > xNameAccess(
                                            xInterface, uno::UNO_QUERY );
            OSL_ENSURE( xNameAccess.is(),
                        "InternetProxyDecider - No name access!" );

            if ( xNameAccess.is() )
            {
                try
                {
                    if ( !( xNameAccess->getByName(
                                rtl::OUString::createFromAscii(
                                    PROXY_TYPE_KEY ) ) >>= m_nProxyType ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - "
                                    "Error getting config item value!" );
                    }
                }
                catch ( lang::WrappedTargetException const & )
                {
                }
                catch ( container::NoSuchElementException const & )
                {
                }

                rtl::OUString aNoProxyList;
                try
                {
                    if ( !( xNameAccess->getByName(
                                rtl::OUString::createFromAscii(
                                    NO_PROXY_LIST_KEY ) ) >>= aNoProxyList ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - "
                                    "Error getting config item value!" );
                    }
                }
                catch ( lang::WrappedTargetException const & )
                {
                }
                catch ( container::NoSuchElementException const & )
                {
                }

                setNoProxyList( aNoProxyList );

                try
                {
                    if ( !( xNameAccess->getByName(
                                rtl::OUString::createFromAscii(
                                    HTTP_PROXY_NAME_KEY ) )
                            >>= m_aHttpProxy.aName ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - "
                                    "Error getting config item value!" );
                    }
                }
                catch ( lang::WrappedTargetException const & )
                {
                }
                catch ( container::NoSuchElementException const & )
                {
                }

                m_aHttpProxy.nPort = -1;
                try
                {
                    uno::Any aValue = xNameAccess->getByName(
                            rtl::OUString::createFromAscii(
                                HTTP_PROXY_PORT_KEY ) );
                    if ( aValue.hasValue() &&
                         !( aValue >>= m_aHttpProxy.nPort ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - "
                                    "Error getting config item value!" );
                    }
                }
                catch ( lang::WrappedTargetException const & )
                {
                }
                catch ( container::NoSuchElementException const & )
                {
                }

                if ( m_aHttpProxy.nPort == -1 )
                    m_aHttpProxy.nPort = 80; // standard HTTP port.

                try
                {
                    if ( !( xNameAccess->getByName(
                                rtl::OUString::createFromAscii(
                                    FTP_PROXY_NAME_KEY ) )
                            >>= m_aFtpProxy.aName ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - "
                                    "Error getting config item value!" );
                    }
                }
                catch ( lang::WrappedTargetException const & )
                {
                }
                catch ( container::NoSuchElementException const & )
                {
                }

                m_aFtpProxy.nPort = -1;
                try
                {
                    uno::Any aValue = xNameAccess->getByName(
                            rtl::OUString::createFromAscii(
                                FTP_PROXY_PORT_KEY ) );
                    if ( aValue.hasValue() &&
                         !( aValue >>= m_aFtpProxy.nPort ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - "
                                    "Error getting config item value!" );
                    }
                }
                catch ( lang::WrappedTargetException const & )
                {
                }
                catch ( container::NoSuchElementException const & )
                {
                }
            }

            // Register as listener for config changes.

            m_xNotifier = uno::Reference< util::XChangesNotifier >(
                                                xInterface, uno::UNO_QUERY );

            OSL_ENSURE( m_xNotifier.is(),
                        "InternetProxyDecider - No notifier!" );

            if ( m_xNotifier.is() )
                m_xNotifier->addChangesListener( this );
        }
    }
    catch ( uno::Exception const & )
    {
        // createInstance, createInstanceWithArguments
        OSL_ENSURE( sal_False, "InternetProxyDecider - Exception!" );
    }
}

//=========================================================================
// virtual
InternetProxyDecider_Impl::~InternetProxyDecider_Impl()
{
}

//=========================================================================
void InternetProxyDecider_Impl::dispose()
{
    uno::Reference< util::XChangesNotifier > xNotifier;

    if ( m_xNotifier.is() )
    {
        osl::Guard< osl::Mutex > aGuard( m_aMutex );

        if ( m_xNotifier.is() )
        {
            xNotifier = m_xNotifier;
            m_xNotifier.clear();
        }
    }

    // Do this unguarded!
    if ( xNotifier.is() )
        xNotifier->removeChangesListener( this );
}

//=========================================================================
bool InternetProxyDecider_Impl::shouldUseProxy( const rtl::OUString & rHost,
                                                sal_Int32 nPort,
                                                bool bUseFullyQualified ) const
{
    rtl::OUStringBuffer aBuffer;

    if ( ( rHost.indexOf( ':' ) != -1 ) &&
         ( rHost[ 0 ] != sal_Unicode( '[' ) ) )
    {
        // host is given as numeric IPv6 address
        aBuffer.appendAscii( "[" );
        aBuffer.append( rHost );
        aBuffer.appendAscii( "]" );
    }
    else
    {
        // host is given either as numeric IPv4 address or non-numeric hostname
        aBuffer.append( rHost );
    }

    aBuffer.append( sal_Unicode( ':' ) );
    aBuffer.append( rtl::OUString::valueOf( nPort ) );
    const rtl::OUString aHostAndPort( aBuffer.makeStringAndClear() );

    std::vector< NoProxyListEntry >::const_iterator it
        = m_aNoProxyList.begin();
    const std::vector< NoProxyListEntry >::const_iterator end
        = m_aNoProxyList.end();

    while ( it != end )
    {
        if ( bUseFullyQualified )
        {
            if ( (*it).second.Matches( aHostAndPort ) )
                return false;
        }
        else
        {
            if ( (*it).first.Matches( aHostAndPort ) )
                return false;
        }
        it++;
    }

    return true;
}

//=========================================================================
const InternetProxyServer & InternetProxyDecider_Impl::getProxy(
                                            const rtl::OUString & rProtocol,
                                            const rtl::OUString & rHost,
                                            sal_Int32 nPort ) const
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

#ifdef USE_JAVA
    // Use system proxy even if no proxies are set
    if ( m_nProxyType == 0 || m_nProxyType == 1 )
    {
        CFDictionaryRef aDict = SCDynamicStoreCopyProxies( NULL );
        if ( aDict )
        {
            CFArrayRef aExcList = (CFArrayRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesExceptionsList );
            if ( rHost.getLength() && aExcList )
            {
                CFIndex nCount = CFArrayGetCount( aExcList );
                rtl::OUString aFullyQualifiedHost;
                for ( CFIndex i = 0; i < nCount; i++ )
                {
                    CFStringRef aHostString = (CFStringRef)CFArrayGetValueAtIndex( aExcList, i );
                    if ( !aHostString )
                        continue;

                    CFIndex nHostLen = CFStringGetLength( aHostString );
                    CFRange aHostRange = CFRangeMake( 0, nHostLen );
                    sal_Unicode pHostBuffer[ nHostLen + 1 ];
                    CFStringGetCharacters( aHostString, aHostRange, pHostBuffer );
                    pHostBuffer[ nHostLen ] = 0;
                    rtl::OUString aHostName( pHostBuffer );
                    if ( !aHostName.getLength() )
                        continue;

                    // First, try direct hostname match
                    if ( aHostName == rHost )
                        return m_aEmptyProxy;

                    // Second, try match against full qualified hostname
                    if ( !aFullyQualifiedHost.getLength() && !m_aHostnames.get( rHost, aFullyQualifiedHost ) )
                    {
                        // This might be quite expensive (DNS lookup).
                        const osl::SocketAddr aAddr( rHost, nPort );
                        aFullyQualifiedHost = aAddr.getHostname().toAsciiLowerCase();
                        m_aHostnames.put( rHost, aFullyQualifiedHost ); 
                    }

                    // Error resolving name? -> fallback.
                    if ( !aFullyQualifiedHost.getLength() )
                        aFullyQualifiedHost = rHost;

                    if ( aFullyQualifiedHost != rHost )
                    {
                        if ( aHostName == aFullyQualifiedHost )
                            return m_aEmptyProxy;
                    }

                    // Third, try match of fully qualified entries in no-proxy
                    // list against full qualified hostname
                    sal_Int32 nPos = aFullyQualifiedHost.getLength() - aHostName.getLength();
                    if ( nPos >= 0 && aFullyQualifiedHost.match( aHostName, nPos ) )
                        return m_aEmptyProxy;
                }
            }

            if ( rProtocol.toAsciiLowerCase().equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "ftp" ) ) )
            {
                int nNumber;
                CFNumberRef aNumber = (CFNumberRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesFTPEnable );
                if ( aNumber && CFNumberGetValue( aNumber, kCFNumberIntType, &nNumber) && nNumber == 1 )
                {
                    CFStringRef aProxyString = (CFStringRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesFTPProxy );
                    if ( aProxyString )
                    {
                        CFIndex nProxyLen = CFStringGetLength( aProxyString );
                        CFRange aProxyRange = CFRangeMake( 0, nProxyLen );
                        sal_Unicode pProxyBuffer[ nProxyLen + 1 ];
                        CFStringGetCharacters( aProxyString, aProxyRange, pProxyBuffer );
                        pProxyBuffer[ nProxyLen ] = 0;
                        rtl::OUString aProxyName( pProxyBuffer );
                        if ( aProxyName.getLength() )
                        {
                            m_aFtpSystemProxy.aName = aProxyName;
                            m_aFtpSystemProxy.nPort = 21;
                            aNumber = (CFNumberRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesFTPPort );
                            if ( aNumber && CFNumberGetValue( aNumber, kCFNumberIntType, &nNumber) && nNumber >= 0 )
                                m_aFtpSystemProxy.nPort = nNumber;
                            return m_aFtpSystemProxy;
                        }
                    }
                }
            }
            else if ( rProtocol.toAsciiLowerCase().equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "https" ) ) )
            {
                int nNumber;
                CFNumberRef aNumber = (CFNumberRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesHTTPSEnable );
                if ( aNumber && CFNumberGetValue( aNumber, kCFNumberIntType, &nNumber) && nNumber == 1 )
                {
                    CFStringRef aProxyString = (CFStringRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesHTTPSProxy );
                    if ( aProxyString )
                    {
                        CFIndex nProxyLen = CFStringGetLength( aProxyString );
                        CFRange aProxyRange = CFRangeMake( 0, nProxyLen );
                        sal_Unicode pProxyBuffer[ nProxyLen + 1 ];
                        CFStringGetCharacters( aProxyString, aProxyRange, pProxyBuffer );
                        pProxyBuffer[ nProxyLen ] = 0;
                        rtl::OUString aProxyName( pProxyBuffer );
                        if ( aProxyName.getLength() )
                        {
                            m_aHttpsSystemProxy.aName = aProxyName;
                            m_aHttpsSystemProxy.nPort = 443;
                            aNumber = (CFNumberRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesHTTPSPort );
                            if ( aNumber && CFNumberGetValue( aNumber, kCFNumberIntType, &nNumber) && nNumber >= 0 )
                                m_aHttpsSystemProxy.nPort = nNumber;
                            return m_aHttpsSystemProxy;
                        }
                    }
                }
            }
            else
            {
                // All other protocols use the HTTP proxy.
                int nNumber;
                CFNumberRef aNumber = (CFNumberRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesHTTPEnable );
                if ( aNumber && CFNumberGetValue( aNumber, kCFNumberIntType, &nNumber) && nNumber == 1 )
                {
                    CFStringRef aProxyString = (CFStringRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesHTTPProxy );
                    if ( aProxyString )
                    {
                        CFIndex nProxyLen = CFStringGetLength( aProxyString );
                        CFRange aProxyRange = CFRangeMake( 0, nProxyLen );
                        sal_Unicode pProxyBuffer[ nProxyLen + 1 ];
                        CFStringGetCharacters( aProxyString, aProxyRange, pProxyBuffer );
                        pProxyBuffer[ nProxyLen ] = 0;
                        rtl::OUString aProxyName( pProxyBuffer );
                        if ( aProxyName.getLength() )
                        {
                            m_aHttpSystemProxy.aName = aProxyName;
                            m_aHttpSystemProxy.nPort = 80;
                            aNumber = (CFNumberRef)CFDictionaryGetValue( aDict, kSCPropNetProxiesHTTPPort );
                            if ( aNumber && CFNumberGetValue( aNumber, kCFNumberIntType, &nNumber) && nNumber >= 0 )
                                m_aHttpSystemProxy.nPort = nNumber;
                            return m_aHttpSystemProxy;
                        }
                    }
                }
            }

            CFRelease( aDict );
        }

        return m_aEmptyProxy;
    }
#else	// USE_JAVA
    if ( m_nProxyType == 0 )
    {
        // Never use proxy.
        return m_aEmptyProxy;
    }
#endif	// USE_JAVA

    if ( rHost.getLength() && m_aNoProxyList.size() )
    {
        //////////////////////////////////////////////////////////////////
        // First, try direct hostname match - #110515#
        //////////////////////////////////////////////////////////////////

        if ( !shouldUseProxy( rHost, nPort, false ) )
            return m_aEmptyProxy;

        //////////////////////////////////////////////////////////////////
        // Second, try match against full qualified hostname - #104401#
        //////////////////////////////////////////////////////////////////

        rtl::OUString aHost;

        if ( ( rHost[ 0 ] == sal_Unicode( '[' ) ) &&
             ( rHost.getLength() > 1 ) )
        {
            // host is given as numeric IPv6 address. name resolution
            // functions need hostname without square brackets.
            aHost = rHost.copy( 1, rHost.getLength() - 2 );
        }
        else
        {
            aHost = rHost;
        }

        rtl::OUString aFullyQualifiedHost;
        if ( !m_aHostnames.get( aHost, aFullyQualifiedHost ) )
        {
            // This might be quite expensive (DNS lookup).
            const osl::SocketAddr aAddr( aHost, nPort );
            aFullyQualifiedHost = aAddr.getHostname().toAsciiLowerCase();
            m_aHostnames.put( aHost, aFullyQualifiedHost ); 
        }

        // Error resolving name? -> fallback.
        if ( !aFullyQualifiedHost.getLength() )
            aFullyQualifiedHost = aHost;

        if ( aFullyQualifiedHost != aHost )
        {
            if ( !shouldUseProxy( aFullyQualifiedHost, nPort, false ) )
                return m_aEmptyProxy;
        }

        //////////////////////////////////////////////////////////////////
        // Third, try match of fully qualified entries in no-proxy list
        // against full qualified hostname
        //
        // Example:
        // list: staroffice-doc -> full: xyz.germany.sun.com
        // in:   staroffice-doc.germany.sun.com -> full: xyz.germany.sun.com
        //
        //////////////////////////////////////////////////////////////////

        if ( !shouldUseProxy( aFullyQualifiedHost, nPort, true ) )
            return m_aEmptyProxy;
    }

    if ( rProtocol.toAsciiLowerCase()
            .equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "ftp" ) ) )
    {
        if ( m_aFtpProxy.aName.getLength() > 0 && m_aFtpProxy.nPort >= 0 )
            return m_aFtpProxy;
    }
    else if ( m_aHttpProxy.aName.getLength() )
    {
        // All other protocols use the HTTP proxy.
        return m_aHttpProxy;
    }
    return m_aEmptyProxy;
}

//=========================================================================
// virtual
void SAL_CALL InternetProxyDecider_Impl::changesOccurred(
                                        const util::ChangesEvent& Event )
    throw( uno::RuntimeException )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    sal_Int32 nCount = Event.Changes.getLength();
    if ( nCount )
    {
        const util::ElementChange* pElementChanges
            = Event.Changes.getConstArray();
        for ( sal_Int32 n = 0; n < nCount; ++n )
        {
            const util::ElementChange& rElem = pElementChanges[ n ];
            rtl::OUString aKey;
            if ( ( rElem.Accessor >>= aKey ) && aKey.getLength() )
            {
                if ( aKey.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM(
                                                    PROXY_TYPE_KEY ) ) )
                {
                    if ( !( rElem.Element >>= m_nProxyType ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - changesOccurred - "
                                    "Error getting config item value!" );
                    }
                }
                else if ( aKey.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM(
                                                    NO_PROXY_LIST_KEY ) ) )
                {
                    rtl::OUString aNoProxyList;
                    if ( !( rElem.Element >>= aNoProxyList ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - changesOccurred - "
                                    "Error getting config item value!" );
                    }

                    setNoProxyList( aNoProxyList );
                }
                else if ( aKey.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM(
                                                    HTTP_PROXY_NAME_KEY ) ) )
                {
                    if ( !( rElem.Element >>= m_aHttpProxy.aName ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - changesOccurred - "
                                    "Error getting config item value!" );
                    }
                }
                else if ( aKey.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM(
                                                    HTTP_PROXY_PORT_KEY ) ) )
                {
                    if ( !( rElem.Element >>= m_aHttpProxy.nPort ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - changesOccurred - "
                                    "Error getting config item value!" );
                    }

                    if ( m_aHttpProxy.nPort == -1 )
                        m_aHttpProxy.nPort = 80; // standard HTTP port.
                }
                else if ( aKey.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM(
                                                    FTP_PROXY_NAME_KEY ) ) )
                {
                    if ( !( rElem.Element >>= m_aFtpProxy.aName ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - changesOccurred - "
                                    "Error getting config item value!" );
                    }
                }
                else if ( aKey.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM(
                                                    FTP_PROXY_PORT_KEY ) ) )
                {
                    if ( !( rElem.Element >>= m_aFtpProxy.nPort ) )
                    {
                        OSL_ENSURE( sal_False,
                                    "InternetProxyDecider - changesOccurred - "
                                    "Error getting config item value!" );
                    }
                }
            }
        }
    }
}

//=========================================================================
// virtual
void SAL_CALL InternetProxyDecider_Impl::disposing(const lang::EventObject&)
    throw( uno::RuntimeException )
{
    if ( m_xNotifier.is() )
    {
        osl::Guard< osl::Mutex > aGuard( m_aMutex );

        if ( m_xNotifier.is() )
            m_xNotifier.clear();
    }
}

//=========================================================================
void InternetProxyDecider_Impl::setNoProxyList(
                                        const rtl::OUString & rNoProxyList )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    m_aNoProxyList.clear();

    if ( rNoProxyList.getLength() )
    {
        // List of connection endpoints hostname[:port],
        // separated by semicolon. Wilcards allowed.

        sal_Int32 nPos = 0;
        sal_Int32 nEnd = rNoProxyList.indexOf( ';' );
        sal_Int32 nLen = rNoProxyList.getLength();

        do
        {
            if ( nEnd == -1 )
                nEnd = nLen;

            rtl::OUString aToken = rNoProxyList.copy( nPos, nEnd - nPos );

            if ( aToken.getLength() )
            {
                rtl::OUString aServer;
                rtl::OUString aPort;

                // numerical IPv6 address?
                bool bIPv6Address = false;
                sal_Int32 nClosedBracketPos = aToken.indexOf( ']' );
                if ( nClosedBracketPos == -1 )
                    nClosedBracketPos = 0;
                else
                    bIPv6Address = true;

                sal_Int32 nColonPos = aToken.indexOf( ':', nClosedBracketPos );
                if ( nColonPos == -1 )
                {
                    // No port given, server pattern equals current token
                    aPort = rtl::OUString::createFromAscii( "*" );
                    if ( aToken.indexOf( '*' ) == -1 )
                    {
                        // pattern describes exactly one server
                        aServer = aToken;
                    }

                    aToken += rtl::OUString::createFromAscii( ":*" );
                }
                else
                {
                    // Port given, extract server pattern
                    sal_Int32 nAsterixPos = aToken.indexOf( '*' );
                    aPort = aToken.copy( nColonPos + 1 );
                    if ( nAsterixPos < nColonPos )
                    {
                        // pattern describes exactly one server
                        aServer = aToken.copy( 0, nColonPos );
                    }
                }

                rtl::OUStringBuffer aFullyQualifiedHost;
                if ( aServer.getLength() )
                {
                    // Remember fully qualified server name if current list
                    // entry specifies exactly one non-fully qualified server
                    // name.

                    // remove square brackets from host name in case it's
                    // a numerical IPv6 address.
                    if ( bIPv6Address )
                        aServer = aServer.copy( 1, aServer.getLength() - 2 );

                    // This might be quite expensive (DNS lookup).
                    const osl::SocketAddr aAddr( aServer, 0 );
                    rtl::OUString aTmp = aAddr.getHostname().toAsciiLowerCase();
                    if ( aTmp != aServer.toAsciiLowerCase() )
                    {
                        if ( bIPv6Address )
                        {
                            aFullyQualifiedHost.appendAscii( "[" );
                            aFullyQualifiedHost.append( aTmp );
                            aFullyQualifiedHost.appendAscii( "]" );
                        }
                        else
                        {
                            aFullyQualifiedHost.append( aTmp );
                        }
                        aFullyQualifiedHost.appendAscii( ":" );
                        aFullyQualifiedHost.append( aPort );
                    }
                }

                m_aNoProxyList.push_back(
                    NoProxyListEntry( WildCard( aToken ),
                                      WildCard(
                                        aFullyQualifiedHost
                                            .makeStringAndClear() ) ) );
            }

            if ( nEnd != nLen )
            {
                nPos = nEnd + 1;
                nEnd = rNoProxyList.indexOf( ';', nPos );
            }
        }
        while ( nEnd != nLen );
    }
}

} // namespace proxydecider_impl

//=========================================================================
//=========================================================================
//
// InternetProxyDecider Implementation.
//
//=========================================================================
//=========================================================================

InternetProxyDecider::InternetProxyDecider(
    const uno::Reference< lang::XMultiServiceFactory >& rxSMgr )
: m_pImpl( new proxydecider_impl::InternetProxyDecider_Impl( rxSMgr ) )
{
    m_pImpl->acquire();
}

//=========================================================================
InternetProxyDecider::~InternetProxyDecider()
{
    // Break circular reference between config listener and notifier.
    m_pImpl->dispose();

    // Let him go...
    m_pImpl->release();
}

//=========================================================================
bool InternetProxyDecider::shouldUseProxy( const rtl::OUString & rProtocol,
                                           const rtl::OUString & rHost,
                                           sal_Int32 nPort ) const
{
    const InternetProxyServer & rData = m_pImpl->getProxy( rProtocol,
                                                           rHost,
                                                           nPort );
    return ( rData.aName.getLength() > 0 );
}

//=========================================================================
const InternetProxyServer & InternetProxyDecider::getProxy(
                                            const rtl::OUString & rProtocol,
                                            const rtl::OUString & rHost,
                                            sal_Int32 nPort ) const
{
    return m_pImpl->getProxy( rProtocol, rHost, nPort );
}

} // namespace ucbhelper
