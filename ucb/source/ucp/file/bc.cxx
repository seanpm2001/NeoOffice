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
 * Modified December 2012 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_ucb.hxx"
#include <rtl/uri.hxx>
#include <rtl/ustrbuf.hxx>
#include <osl/file.hxx>

#include "osl/diagnose.h"
#include <com/sun/star/ucb/OpenMode.hpp>
#ifndef _COM_SUN_STAR_BEANS_PROPERTYATTRIBBUTE_HPP_
#include <com/sun/star/beans/PropertyAttribute.hpp>
#endif
#include <com/sun/star/ucb/XProgressHandler.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/io/XActiveDataStreamer.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/ucb/NumberedSortingInfo.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/beans/PropertyChangeEvent.hpp>
#include <com/sun/star/beans/PropertySetInfoChange.hpp>
#include <com/sun/star/ucb/ContentAction.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <com/sun/star/ucb/ContentInfoAttribute.hpp>
#include "filglob.hxx"
#include "filid.hxx"
#include "filrow.hxx"
#include "bc.hxx"
#include "prov.hxx"
#ifndef _FILERROR_HXX_
#include "filerror.hxx"
#endif
#include "filinsreq.hxx"

#if defined USE_JAVA && defined MACOSX

#include <dlfcn.h>

typedef id Application_acquireSecurityScopedURLFromOUString_Type( const ::rtl::OUString *pNonSecurityScopedURL, unsigned char bMustShowDialogIfNoBookmark, const ::rtl::OUString *pDialogTitle );
typedef void Application_releaseSecurityScopedURL_Type( id pSecurityScopedURLs );

static Application_acquireSecurityScopedURLFromOUString_Type *pApplication_acquireSecurityScopedURLFromOUString = NULL;
static Application_releaseSecurityScopedURL_Type *pApplication_releaseSecurityScopedURL = NULL;

#endif	// USE_JAVA && MACOSX


using namespace fileaccess;
using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::ucb;

// PropertyListeners


typedef cppu::OMultiTypeInterfaceContainerHelperVar< rtl::OUString,hashOUString,equalOUString >
PropertyListeners_impl;

class fileaccess::PropertyListeners
	: public PropertyListeners_impl
{
public:
	PropertyListeners( ::osl::Mutex& aMutex )
		: PropertyListeners_impl( aMutex )
	{
	}
};


/****************************************************************************************/
/*                                                                                      */
/*                    BaseContent                                                       */
/*                                                                                      */
/****************************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// Private Constructor for just inserted Contents

BaseContent::BaseContent( shell* pMyShell,
						  const rtl::OUString& parentName,
						  sal_Bool bFolder )
	: m_pMyShell( pMyShell ),
	  m_xContentIdentifier( 0 ),
	  m_aUncPath( parentName ),
	  m_bFolder( bFolder ),
	  m_nState( JustInserted ),
	  m_pDisposeEventListeners( 0 ),
	  m_pContentEventListeners( 0 ),
	  m_pPropertySetInfoChangeListeners( 0 ),
	  m_pPropertyListener( 0 )
#if defined USE_JAVA && defined MACOSX
	  , m_pSecurityScopedURL( 0 )
#endif	// USE_JAVA && MACOSX
{
#if defined USE_JAVA && defined MACOSX
	if ( !pApplication_acquireSecurityScopedURLFromOUString )
		pApplication_acquireSecurityScopedURLFromOUString = (Application_acquireSecurityScopedURLFromOUString_Type *)dlsym( RTLD_DEFAULT, "Application_acquireSecurityScopedURLFromOUString" );
	if ( !pApplication_releaseSecurityScopedURL )
		pApplication_releaseSecurityScopedURL = (Application_releaseSecurityScopedURL_Type *)dlsym( RTLD_DEFAULT, "Application_releaseSecurityScopedURL" );
	if ( pApplication_acquireSecurityScopedURLFromOUString && pApplication_releaseSecurityScopedURL )
		m_pSecurityScopedURL = pApplication_acquireSecurityScopedURLFromOUString( &m_aUncPath, sal_True, NULL );
#endif	// USE_JAVA && MACOSX

	m_pMyShell->m_pProvider->acquire();
	// No registering, since we have no name
}


////////////////////////////////////////////////////////////////////////////////
// Constructor for full featured Contents

BaseContent::BaseContent( shell* pMyShell,
						  const Reference< XContentIdentifier >& xContentIdentifier,
						  const rtl::OUString& aUncPath )
	: m_pMyShell( pMyShell ),
	  m_xContentIdentifier( xContentIdentifier ),
	  m_aUncPath( aUncPath ),
	  m_bFolder( false ),
	  m_nState( FullFeatured ),
	  m_pDisposeEventListeners( 0 ),
	  m_pContentEventListeners( 0 ),
	  m_pPropertySetInfoChangeListeners( 0 ),
	  m_pPropertyListener( 0 )
#if defined USE_JAVA && defined MACOSX
	  , m_pSecurityScopedURL( 0 )
#endif	// USE_JAVA && MACOSX
{
#if defined USE_JAVA && defined MACOSX
	if ( !pApplication_acquireSecurityScopedURLFromOUString )
		pApplication_acquireSecurityScopedURLFromOUString = (Application_acquireSecurityScopedURLFromOUString_Type *)dlsym( RTLD_DEFAULT, "Application_acquireSecurityScopedURLFromOUString" );
	if ( !pApplication_releaseSecurityScopedURL )
		pApplication_releaseSecurityScopedURL = (Application_releaseSecurityScopedURL_Type *)dlsym( RTLD_DEFAULT, "Application_releaseSecurityScopedURL" );
	if ( pApplication_acquireSecurityScopedURLFromOUString && pApplication_releaseSecurityScopedURL )
		m_pSecurityScopedURL = pApplication_acquireSecurityScopedURLFromOUString( &m_aUncPath, sal_True, NULL );
#endif	// USE_JAVA && MACOSX

	m_pMyShell->m_pProvider->acquire();
	m_pMyShell->registerNotifier( m_aUncPath,this );
	m_pMyShell->insertDefaultProperties( m_aUncPath );
}


BaseContent::~BaseContent( )
{
	if( ( m_nState & FullFeatured ) || ( m_nState & Deleted ) )
	{
		m_pMyShell->deregisterNotifier( m_aUncPath,this );
	}
	m_pMyShell->m_pProvider->release();

    delete m_pDisposeEventListeners;
    delete m_pContentEventListeners;
    delete m_pPropertyListener;
    delete m_pPropertySetInfoChangeListeners;

#if defined USE_JAVA && defined MACOSX
	if ( m_pSecurityScopedURL && pApplication_releaseSecurityScopedURL )
		pApplication_releaseSecurityScopedURL( m_pSecurityScopedURL );
#endif	// USE_JAVA && MACOSX
}


//////////////////////////////////////////////////////////////////////////
// XInterface
//////////////////////////////////////////////////////////////////////////

void SAL_CALL
BaseContent::acquire( void )
	throw()
{
	OWeakObject::acquire();
}


void SAL_CALL
BaseContent::release( void )
	throw()
{
	OWeakObject::release();
}


Any SAL_CALL
BaseContent::queryInterface( const Type& rType )
	throw( RuntimeException )
{
	Any aRet = cppu::queryInterface( rType,
                                     SAL_STATIC_CAST( lang::XComponent*, this ),
                                     SAL_STATIC_CAST( lang::XTypeProvider*, this ),
                                     SAL_STATIC_CAST( lang::XServiceInfo*, this ),
                                     SAL_STATIC_CAST( XCommandProcessor*, this ),
                                     SAL_STATIC_CAST( container::XChild*, this ),
                                     SAL_STATIC_CAST( beans::XPropertiesChangeNotifier*, this ),
                                     SAL_STATIC_CAST( beans::XPropertyContainer*, this ),
                                     SAL_STATIC_CAST( XContentCreator*,this ),
                                     SAL_STATIC_CAST( beans::XPropertySetInfoChangeNotifier*, this ),
                                     SAL_STATIC_CAST( XContent*,this) );
	return aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType );
}




//////////////////////////////////////////////////////////////////////////////////////////
// XComponent
////////////////////////////////////////////////////////////////////////////////////////

void SAL_CALL
BaseContent::addEventListener( const Reference< lang::XEventListener >& Listener )
	throw( RuntimeException )
{
	osl::MutexGuard aGuard( m_aMutex );

	if ( ! m_pDisposeEventListeners )
		m_pDisposeEventListeners =
			new cppu::OInterfaceContainerHelper( m_aEventListenerMutex );

	m_pDisposeEventListeners->addInterface( Listener );
}


void SAL_CALL
BaseContent::removeEventListener( const Reference< lang::XEventListener >& Listener )
	throw( RuntimeException )
{
	osl::MutexGuard aGuard( m_aMutex );

	if ( m_pDisposeEventListeners )
		m_pDisposeEventListeners->removeInterface( Listener );
}


void SAL_CALL
BaseContent::dispose()
	throw( RuntimeException )
{
	lang::EventObject aEvt;
    cppu::OInterfaceContainerHelper* pDisposeEventListeners;
    cppu::OInterfaceContainerHelper* pContentEventListeners;
    cppu::OInterfaceContainerHelper* pPropertySetInfoChangeListeners;
    PropertyListeners* pPropertyListener;

    {
        osl::MutexGuard aGuard( m_aMutex );
        aEvt.Source = static_cast< XContent* >( this );


        pDisposeEventListeners =
            m_pDisposeEventListeners, m_pDisposeEventListeners = 0;

        pContentEventListeners =
            m_pContentEventListeners, m_pContentEventListeners = 0;

        pPropertySetInfoChangeListeners =
            m_pPropertySetInfoChangeListeners,
            m_pPropertySetInfoChangeListeners = 0;

        pPropertyListener =
            m_pPropertyListener, m_pPropertyListener = 0;
    }

	if ( pDisposeEventListeners && pDisposeEventListeners->getLength() )
		pDisposeEventListeners->disposeAndClear( aEvt );

	if ( pContentEventListeners && pContentEventListeners->getLength() )
		pContentEventListeners->disposeAndClear( aEvt );

	if( pPropertyListener )
		pPropertyListener->disposeAndClear( aEvt );

	if( pPropertySetInfoChangeListeners )
		pPropertySetInfoChangeListeners->disposeAndClear( aEvt );

    delete pDisposeEventListeners;
    delete pContentEventListeners;
    delete pPropertyListener;
    delete pPropertySetInfoChangeListeners;
}



//////////////////////////////////////////////////////////////////////////////////////////
//  XServiceInfo
//////////////////////////////////////////////////////////////////////////////////////////

rtl::OUString SAL_CALL
BaseContent::getImplementationName()
	throw( RuntimeException)
{
	return rtl::OUString::createFromAscii( "com.sun.star.comp.ucb.FileContent" );
}



sal_Bool SAL_CALL
BaseContent::supportsService( const rtl::OUString& ServiceName )
	throw( RuntimeException)
{
	if( ServiceName.compareToAscii( "com.sun.star.ucb.FileContent" ) == 0 )
		return true;
	else
		return false;
}



Sequence< rtl::OUString > SAL_CALL
BaseContent::getSupportedServiceNames()
	throw( RuntimeException )
{
	Sequence< rtl::OUString > ret( 1 );
	ret[0] = rtl::OUString::createFromAscii( "com.sun.star.ucb.FileContent" );
	return ret;
}



//////////////////////////////////////////////////////////////////////////////////////////
//  XTypeProvider
//////////////////////////////////////////////////////////////////////////////////////////

XTYPEPROVIDER_IMPL_10( BaseContent,
					   lang::XComponent,
					   lang::XTypeProvider,
					   lang::XServiceInfo,
					   XCommandProcessor,
					   XContentCreator,
					   XContent,
					   container::XChild,
					   beans::XPropertiesChangeNotifier,
					   beans::XPropertyContainer,
					   beans::XPropertySetInfoChangeNotifier )


//////////////////////////////////////////////////////////////////////////////////////////
//  XCommandProcessor
//////////////////////////////////////////////////////////////////////////////////////////

sal_Int32 SAL_CALL
BaseContent::createCommandIdentifier( void )
	throw( RuntimeException )
{
	return m_pMyShell->getCommandId();
}


void SAL_CALL
BaseContent::abort( sal_Int32 CommandId )
	throw( RuntimeException )
{
	m_pMyShell->abort( CommandId );
}


Any SAL_CALL
BaseContent::execute( const Command& aCommand,
					  sal_Int32 CommandId,
					  const Reference< XCommandEnvironment >& Environment )
	throw( Exception,
		   CommandAbortedException,
		   RuntimeException )
{
	if( ! CommandId )
		// A Command with commandid zero cannot be aborted
		CommandId = createCommandIdentifier();

	m_pMyShell->startTask( CommandId,
						   Environment );

	Any aAny;

	if( ! aCommand.Name.compareToAscii( "getPropertySetInfo" ) )  // No exceptions
	{
		aAny <<= getPropertySetInfo( CommandId );
	}
	else if( ! aCommand.Name.compareToAscii( "getCommandInfo" ) )  // no exceptions
	{
		aAny <<= getCommandInfo();
	}
	else if( ! aCommand.Name.compareToAscii( "setPropertyValues" ) )
	{
		Sequence< beans::PropertyValue > sPropertyValues;

		if( ! ( aCommand.Argument >>= sPropertyValues ) )
			m_pMyShell->installError( CommandId,
									  TASKHANDLING_WRONG_SETPROPERTYVALUES_ARGUMENT );
		else
			aAny <<= setPropertyValues( CommandId,sPropertyValues );  // calls endTask by itself
	}
	else if( ! aCommand.Name.compareToAscii( "getPropertyValues" ) )
	{
		Sequence< beans::Property > ListOfRequestedProperties;

		if( ! ( aCommand.Argument >>= ListOfRequestedProperties ) )
			m_pMyShell->installError( CommandId,
									  TASKHANDLING_WRONG_GETPROPERTYVALUES_ARGUMENT );
		else
			aAny <<= getPropertyValues( CommandId,
										ListOfRequestedProperties );
	}
	else if( ! aCommand.Name.compareToAscii( "open" ) )
	{
		OpenCommandArgument2 aOpenArgument;
		if( ! ( aCommand.Argument >>= aOpenArgument ) )
			m_pMyShell->installError( CommandId,
									  TASKHANDLING_WRONG_OPEN_ARGUMENT );
		else
		{
			Reference< XDynamicResultSet > result = open( CommandId,aOpenArgument );
			if( result.is() )
				aAny <<= result;
		}
	}
	else if( ! aCommand.Name.compareToAscii( "delete" ) )
	{
		if( ! aCommand.Argument.has< sal_Bool >() )
			m_pMyShell->installError( CommandId,
									  TASKHANDLING_WRONG_DELETE_ARGUMENT );
		else
			deleteContent( CommandId );
	}
	else if( ! aCommand.Name.compareToAscii( "transfer" ) )
	{
		TransferInfo aTransferInfo;
		if( ! ( aCommand.Argument >>= aTransferInfo ) )
			m_pMyShell->installError( CommandId,
									  TASKHANDLING_WRONG_TRANSFER_ARGUMENT );
		else
			transfer( CommandId, aTransferInfo );
	}
	else if( ! aCommand.Name.compareToAscii( "insert" ) )
	{
		InsertCommandArgument aInsertArgument;
		if( ! ( aCommand.Argument >>= aInsertArgument ) )
			m_pMyShell->installError( CommandId,
									  TASKHANDLING_WRONG_INSERT_ARGUMENT );
		else
			insert( CommandId,aInsertArgument );
	}
    else if( ! aCommand.Name.compareToAscii( "getCasePreservingURL" ) )
    {
		Sequence< beans::Property > seq(1);
		seq[0] = beans::Property(
            rtl::OUString::createFromAscii("CasePreservingURL"),
            -1,
            getCppuType( static_cast< sal_Bool* >(0) ),
            0 );
		Reference< sdbc::XRow > xRow = getPropertyValues( CommandId,seq );
        rtl::OUString CasePreservingURL = xRow->getString(1);
        if(!xRow->wasNull())
            aAny <<= CasePreservingURL;
    }
	else
		m_pMyShell->installError( CommandId,
								  TASKHANDLER_UNSUPPORTED_COMMAND );


	// This is the only function allowed to throw an exception
    endTask( CommandId );

	return aAny;
}



void SAL_CALL
BaseContent::addPropertiesChangeListener(
	const Sequence< rtl::OUString >& PropertyNames,
	const Reference< beans::XPropertiesChangeListener >& Listener )
	throw( RuntimeException )
{
	if( ! Listener.is() )
		return;

	osl::MutexGuard aGuard( m_aMutex );

	if( ! m_pPropertyListener )
		m_pPropertyListener = new PropertyListeners( m_aEventListenerMutex );


	if( PropertyNames.getLength() == 0 )
		m_pPropertyListener->addInterface( rtl::OUString(),Listener );
	else
	{
		Reference< beans::XPropertySetInfo > xProp = m_pMyShell->info_p( m_aUncPath );
		for( sal_Int32 i = 0; i < PropertyNames.getLength(); ++i )
			if( xProp->hasPropertyByName( PropertyNames[i] ) )
				m_pPropertyListener->addInterface( PropertyNames[i],Listener );
	}
}


void SAL_CALL
BaseContent::removePropertiesChangeListener( const Sequence< rtl::OUString >& PropertyNames,
											 const Reference< beans::XPropertiesChangeListener >& Listener )
	throw( RuntimeException )
{
	if( ! Listener.is() )
		return;

	osl::MutexGuard aGuard( m_aMutex );

    if( ! m_pPropertyListener )
        return;

	for( sal_Int32 i = 0; i < PropertyNames.getLength(); ++i )
		m_pPropertyListener->removeInterface( PropertyNames[i],Listener );

	m_pPropertyListener->removeInterface( rtl::OUString(), Listener );
}


/////////////////////////////////////////////////////////////////////////////////////////
// XContent
/////////////////////////////////////////////////////////////////////////////////////////

Reference< ucb::XContentIdentifier > SAL_CALL
BaseContent::getIdentifier()
	throw( RuntimeException )
{
	return m_xContentIdentifier;
}


rtl::OUString SAL_CALL
BaseContent::getContentType()
	throw( RuntimeException )
{
	if( !( m_nState & Deleted ) )
	{
		if( m_nState & JustInserted )
		{
			if ( m_bFolder )
				return m_pMyShell->FolderContentType;
			else
				return m_pMyShell->FileContentType;
		}
		else
		{
			try
			{
				// Who am I ?
				Sequence< beans::Property > seq(1);
				seq[0] = beans::Property( rtl::OUString::createFromAscii("IsDocument"),
						  		  	  	  -1,
							  	  	  	  getCppuType( static_cast< sal_Bool* >(0) ),
							  	  	  	  0 );
				Reference< sdbc::XRow > xRow = getPropertyValues( -1,seq );
				sal_Bool IsDocument = xRow->getBoolean( 1 );

				if ( !xRow->wasNull() )
				{
					if ( IsDocument )
						return m_pMyShell->FileContentType;
					else
						return m_pMyShell->FolderContentType;
				}
				else
				{
					OSL_ENSURE( false,
								"BaseContent::getContentType - Property value was null!" );
				}
			}
			catch ( sdbc::SQLException const & )
			{
				OSL_ENSURE( false,
							"BaseContent::getContentType - Caught SQLException!" );
			}
		}
	}

	return rtl::OUString();
}



void SAL_CALL
BaseContent::addContentEventListener(
	const Reference< XContentEventListener >& Listener )
	throw( RuntimeException )
{
	osl::MutexGuard aGuard( m_aMutex );

	if ( ! m_pContentEventListeners )
		m_pContentEventListeners =
			new cppu::OInterfaceContainerHelper( m_aEventListenerMutex );


	m_pContentEventListeners->addInterface( Listener );
}


void SAL_CALL
BaseContent::removeContentEventListener(
	const Reference< XContentEventListener >& Listener )
	throw( RuntimeException )
{
	osl::MutexGuard aGuard( m_aMutex );

	if ( m_pContentEventListeners )
		m_pContentEventListeners->removeInterface( Listener );
}



////////////////////////////////////////////////////////////////////////////////
// XPropertyContainer
////////////////////////////////////////////////////////////////////////////////


void SAL_CALL
BaseContent::addProperty(
	const rtl::OUString& Name,
	sal_Int16 Attributes,
	const Any& DefaultValue )
	throw( beans::PropertyExistException,
		   beans::IllegalTypeException,
		   lang::IllegalArgumentException,
		   RuntimeException)
{
	if( ( m_nState & JustInserted ) || ( m_nState & Deleted ) || Name == rtl::OUString() )
	{
		throw lang::IllegalArgumentException();
	}

	m_pMyShell->associate( m_aUncPath,Name,DefaultValue,Attributes );
}


void SAL_CALL
BaseContent::removeProperty(
	const rtl::OUString& Name )
	throw( beans::UnknownPropertyException,
		   beans::NotRemoveableException,
		   RuntimeException)
{

	if( m_nState & Deleted )
		throw beans::UnknownPropertyException();

	m_pMyShell->deassociate( m_aUncPath, Name );
}

////////////////////////////////////////////////////////////////////////////////
// XContentCreator
////////////////////////////////////////////////////////////////////////////////

Sequence< ContentInfo > SAL_CALL
BaseContent::queryCreatableContentsInfo(
	void )
	throw( RuntimeException )
{
	Sequence< ContentInfo > seq(2);

	// file
	seq[0].Type 	  = m_pMyShell->FileContentType;
	seq[0].Attributes = ContentInfoAttribute::INSERT_WITH_INPUTSTREAM
		| ContentInfoAttribute::KIND_DOCUMENT;

	Sequence< beans::Property > props( 1 );
	props[0] = beans::Property(
		rtl::OUString::createFromAscii( "Title" ),
		-1,
		getCppuType( static_cast< rtl::OUString* >( 0 ) ),
		beans::PropertyAttribute::MAYBEVOID
		| beans::PropertyAttribute::BOUND );
	seq[0].Properties = props;

	// folder
	seq[1].Type 	  = m_pMyShell->FolderContentType;
	seq[1].Attributes = ContentInfoAttribute::KIND_FOLDER;
	seq[1].Properties = props;
	return seq;
}


Reference< XContent > SAL_CALL
BaseContent::createNewContent(
	const ContentInfo& Info )
	throw( RuntimeException )
{
	// Check type.
	if ( !Info.Type.getLength() )
		return Reference< XContent >();

	sal_Bool bFolder
		= ( Info.Type.compareTo( m_pMyShell->FolderContentType ) == 0 );
	if ( !bFolder )
	{
		if ( Info.Type.compareTo( m_pMyShell->FileContentType ) != 0 )
		{
			// Neither folder nor file to create!
			return Reference< XContent >();
		}
	}

	// Who am I ?
	sal_Bool IsDocument = false;

	try
	{
		Sequence< beans::Property > seq(1);
		seq[0] = beans::Property( rtl::OUString::createFromAscii("IsDocument"),
						  		  -1,
							  	  getCppuType( static_cast< sal_Bool* >(0) ),
							  	  0 );
		Reference< sdbc::XRow > xRow = getPropertyValues( -1,seq );
		IsDocument = xRow->getBoolean( 1 );
        
		if ( xRow->wasNull() )
		{
			IsDocument = false;
// 			OSL_ENSURE( false,
// 						"BaseContent::createNewContent - Property value was null!" );
// 			return Reference< XContent >();
		}
	}
	catch ( sdbc::SQLException const & )
	{
		OSL_ENSURE( false,
					"BaseContent::createNewContent - Caught SQLException!" );
		return Reference< XContent >();
	}

	rtl::OUString dstUncPath;

	if( IsDocument )
	{
		// KSO: Why is a document a XContentCreator? This is quite unusual.
		dstUncPath = getParentName( m_aUncPath );
	}
	else
		dstUncPath = m_aUncPath;

	BaseContent* p = new BaseContent( m_pMyShell, dstUncPath, bFolder );
	return Reference< XContent >( p );
}


////////////////////////////////////////////////////////////////////////////////
// XPropertySetInfoChangeNotifier
////////////////////////////////////////////////////////////////////////////////


void SAL_CALL
BaseContent::addPropertySetInfoChangeListener(
	const Reference< beans::XPropertySetInfoChangeListener >& Listener )
	throw( RuntimeException )
{
	osl::MutexGuard aGuard( m_aMutex );
	if( ! m_pPropertySetInfoChangeListeners )
		m_pPropertySetInfoChangeListeners = new cppu::OInterfaceContainerHelper( m_aEventListenerMutex );

	m_pPropertySetInfoChangeListeners->addInterface( Listener );
}


void SAL_CALL
BaseContent::removePropertySetInfoChangeListener(
	const Reference< beans::XPropertySetInfoChangeListener >& Listener )
	throw( RuntimeException )
{
	osl::MutexGuard aGuard( m_aMutex );

	if( m_pPropertySetInfoChangeListeners )
		m_pPropertySetInfoChangeListeners->removeInterface( Listener );
}


////////////////////////////////////////////////////////////////////////////////
// XChild
////////////////////////////////////////////////////////////////////////////////

Reference< XInterface > SAL_CALL
BaseContent::getParent(
	void )
	throw( RuntimeException )
{
	rtl::OUString ParentUnq = getParentName( m_aUncPath );
	rtl::OUString ParentUrl;


	sal_Bool err = m_pMyShell->getUrlFromUnq( ParentUnq, ParentUrl );
	if( err )
		return Reference< XInterface >( 0 );

	FileContentIdentifier* p = new FileContentIdentifier( m_pMyShell,ParentUnq );
	Reference< XContentIdentifier > Identifier( p );

	try
	{
		Reference< XContent > content = m_pMyShell->m_pProvider->queryContent( Identifier );
		return Reference<XInterface>(content,UNO_QUERY);
	}
	catch( IllegalIdentifierException )
	{
		return Reference< XInterface >();
	}
}


void SAL_CALL
BaseContent::setParent(
	const Reference< XInterface >& )
	throw( lang::NoSupportException,
		   RuntimeException)
{
	throw lang::NoSupportException();
}


//////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////////////


Reference< XCommandInfo > SAL_CALL
BaseContent::getCommandInfo()
	throw( RuntimeException )
{
	if( m_nState & Deleted )
		return Reference< XCommandInfo >();

	return m_pMyShell->info_c();
}


Reference< beans::XPropertySetInfo > SAL_CALL
BaseContent::getPropertySetInfo(
	sal_Int32 )
	throw( RuntimeException )
{
	if( m_nState & Deleted )
		return Reference< beans::XPropertySetInfo >();

	return m_pMyShell->info_p( m_aUncPath );
}




Reference< sdbc::XRow > SAL_CALL
BaseContent::getPropertyValues(
	sal_Int32 nMyCommandIdentifier,
	const Sequence< beans::Property >& PropertySet )
	throw( RuntimeException )
{
	sal_Int32 nProps = PropertySet.getLength();
	if ( !nProps )
		return Reference< sdbc::XRow >();

	if( m_nState & Deleted )
	{
		Sequence< Any > aValues( nProps );
		return Reference< sdbc::XRow >( new XRow_impl( m_pMyShell, aValues ) );
	}

	if( m_nState & JustInserted )
	{
		Sequence< Any > aValues( nProps );
		Any* pValues = aValues.getArray();

		const beans::Property* pProps = PropertySet.getConstArray();

		for ( sal_Int32 n = 0; n < nProps; ++n )
		{
			const beans::Property& rProp = pProps[ n ];
			Any& rValue = pValues[ n ];

			if( rProp.Name.compareToAscii( "ContentType" ) == 0 )
			{
				rValue <<= m_bFolder ? m_pMyShell->FolderContentType
					: m_pMyShell->FileContentType;
			}
			else if( rProp.Name.compareToAscii( "IsFolder" ) == 0 )
			{
				rValue <<= m_bFolder;
			}
			else if( rProp.Name.compareToAscii( "IsDocument" ) == 0 )
			{
				rValue <<= sal_Bool( !m_bFolder );
			}
		}

		return Reference< sdbc::XRow >(
			new XRow_impl( m_pMyShell, aValues ) );
	}

	return m_pMyShell->getv( nMyCommandIdentifier,
							 m_aUncPath,
							 PropertySet );
}


Sequence< Any > SAL_CALL
BaseContent::setPropertyValues(
	sal_Int32 nMyCommandIdentifier,
	const Sequence< beans::PropertyValue >& Values )
	throw()
{
	if( m_nState & Deleted )
	{   //  To do
		return Sequence< Any >( Values.getLength() );
	}

    const rtl::OUString Title = rtl::OUString::createFromAscii( "Title" );

	// Special handling for files which have to be inserted
	if( m_nState & JustInserted )
	{
		for( sal_Int32 i = 0; i < Values.getLength(); ++i )
		{
            if( Values[i].Name == Title )
			{
				rtl::OUString NewTitle;
				if( Values[i].Value >>= NewTitle )
				{
                    if ( m_nState & NameForInsertionSet )
                    {
                        // User wants to set another Title before "insert".
                        // m_aUncPath contains previous own URI.

                        sal_Int32 nLastSlash = m_aUncPath.lastIndexOf( '/' );
                        bool bTrailingSlash = false;
                        if ( nLastSlash == m_aUncPath.getLength() - 1 )
                        {
                            bTrailingSlash = true;
                            nLastSlash
                                = m_aUncPath.lastIndexOf( '/', nLastSlash );
                        }

                        OSL_ENSURE( nLastSlash != -1,
                                    "BaseContent::setPropertyValues: "
                                    "Invalid URL!" );

                        rtl::OUStringBuffer aBuf(
                            m_aUncPath.copy( 0, nLastSlash + 1 ) );

                        if ( NewTitle.getLength() > 0 )
                        {
                            aBuf.append( NewTitle );
                            if ( bTrailingSlash )
                                aBuf.append( sal_Unicode( '/' ) );
                        }
                        else
                        {
                            m_nState &= ~NameForInsertionSet;
                        }

                        m_aUncPath = aBuf.makeStringAndClear();
                    }
                    else
                    {
                        if ( NewTitle.getLength() > 0 )
                        {
                            // Initial Title before "insert".
                            // m_aUncPath contains parent's URI.

                            if( m_aUncPath.lastIndexOf( sal_Unicode('/') ) != m_aUncPath.getLength() - 1 )
                                m_aUncPath += rtl::OUString::createFromAscii("/");

                            m_aUncPath += rtl::Uri::encode( NewTitle,
                                                            rtl_UriCharClassPchar,
                                                            rtl_UriEncodeIgnoreEscapes,
                                                            RTL_TEXTENCODING_UTF8 );
                            m_nState |= NameForInsertionSet;
                        }
                    }
				}
			}
		}

		return Sequence< Any >( Values.getLength() );
	}
	else
	{
		Sequence< Any > ret = m_pMyShell->setv( m_aUncPath,  // Does not handle Title
												Values );

		// Special handling Title: Setting Title is equivalent to a renaming of the underlying file
		for( sal_Int32 i = 0; i < Values.getLength(); ++i )
		{
			if( Values[i].Name != Title )
				continue;                  // handled by setv

			rtl::OUString NewTitle;
			if( !( Values[i].Value >>= NewTitle ) )
			{
				ret[i] <<= beans::IllegalTypeException();
				break;
			}
			else if( ! NewTitle.getLength() )
			{
				ret[i] <<= lang::IllegalArgumentException();
				break;
			}


			rtl::OUString aDstName = getParentName( m_aUncPath );
			if( aDstName.lastIndexOf( sal_Unicode('/') ) != aDstName.getLength() - 1 )
				aDstName += rtl::OUString::createFromAscii("/");

			aDstName += rtl::Uri::encode( NewTitle,
										  rtl_UriCharClassPchar,
										  rtl_UriEncodeIgnoreEscapes,
										  RTL_TEXTENCODING_UTF8 );

			m_pMyShell->move( nMyCommandIdentifier,     // move notifies the childs also;
							  m_aUncPath,
							  aDstName,
							  NameClash::KEEP );

			try
			{
                endTask( nMyCommandIdentifier );
			}
			catch( const Exception& e )
			{
				ret[i] <<= e;
			}

			// NameChanges come back trough a ContentEvent
			break; // only handling Title
		} // end for

		return ret;
	}
}



Reference< XDynamicResultSet > SAL_CALL
BaseContent::open(
	sal_Int32 nMyCommandIdentifier,
	const OpenCommandArgument2& aCommandArgument )
	throw()
{
	Reference< XDynamicResultSet > retValue( 0 );

	if( ( m_nState & Deleted ) )
	{
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_DELETED_STATE_IN_OPEN_COMMAND );
	}
	else if( m_nState & JustInserted )
	{
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_INSERTED_STATE_IN_OPEN_COMMAND );
	}
	else
	{
		if( aCommandArgument.Mode == OpenMode::DOCUMENT ||
            aCommandArgument.Mode == OpenMode::DOCUMENT_SHARE_DENY_NONE )

		{
			Reference< io::XOutputStream > outputStream( aCommandArgument.Sink,UNO_QUERY );
			if( outputStream.is() )
			{
				m_pMyShell->page( nMyCommandIdentifier,
								  m_aUncPath,
								  outputStream );
			}

            sal_Bool bLock = ( aCommandArgument.Mode != OpenMode::DOCUMENT_SHARE_DENY_NONE );

			Reference< io::XActiveDataSink > activeDataSink( aCommandArgument.Sink,UNO_QUERY );
			if( activeDataSink.is() )
			{
				activeDataSink->setInputStream( m_pMyShell->open( nMyCommandIdentifier,
																  m_aUncPath,
                                                                  bLock ) );
			}

			Reference< io::XActiveDataStreamer > activeDataStreamer( aCommandArgument.Sink,UNO_QUERY );
			if( activeDataStreamer.is() )
			{
				activeDataStreamer->setStream( m_pMyShell->open_rw( nMyCommandIdentifier,
																	m_aUncPath,
                                                                    bLock ) );
			}
		}
		else if ( aCommandArgument.Mode == OpenMode::ALL        ||
				  aCommandArgument.Mode == OpenMode::FOLDERS    ||
				  aCommandArgument.Mode == OpenMode::DOCUMENTS )
		{
			retValue = m_pMyShell->ls( nMyCommandIdentifier,
									   m_aUncPath,
									   aCommandArgument.Mode,
									   aCommandArgument.Properties,
									   aCommandArgument.SortingInfo );
		}
//  		else if(  aCommandArgument.Mode ==
//  				  OpenMode::DOCUMENT_SHARE_DENY_NONE  ||
//  				  aCommandArgument.Mode ==
//  				  OpenMode::DOCUMENT_SHARE_DENY_WRITE )
//  			m_pMyShell->installError( nMyCommandIdentifier,
//  									  TASKHANDLING_UNSUPPORTED_OPEN_MODE,
//  									  aCommandArgument.Mode);
		else
			m_pMyShell->installError( nMyCommandIdentifier,
									  TASKHANDLING_UNSUPPORTED_OPEN_MODE,
									  aCommandArgument.Mode);
	}

	return retValue;
}



void SAL_CALL
BaseContent::deleteContent( sal_Int32 nMyCommandIdentifier )
	throw()
{
	if( m_nState & Deleted )
		return;

	if( m_pMyShell->remove( nMyCommandIdentifier,m_aUncPath ) )
	{
		osl::MutexGuard aGuard( m_aMutex );
		m_nState |= Deleted;
	}
}



void SAL_CALL
BaseContent::transfer( sal_Int32 nMyCommandIdentifier,
					   const TransferInfo& aTransferInfo )
	throw()
{
	if( m_nState & Deleted )
		return;

	if( aTransferInfo.SourceURL.compareToAscii( "file:",5 ) != 0 )
	{
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_TRANSFER_INVALIDSCHEME );
		return;
	}

	rtl::OUString srcUnc;
	if( m_pMyShell->getUnqFromUrl( aTransferInfo.SourceURL,srcUnc ) )
	{
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_TRANSFER_INVALIDURL );
		return;
	}

    rtl::OUString srcUncPath = srcUnc;

	// Determine the new title !
	rtl::OUString NewTitle;
	if( aTransferInfo.NewTitle.getLength() )
		NewTitle = rtl::Uri::encode( aTransferInfo.NewTitle,
									 rtl_UriCharClassPchar,
									 rtl_UriEncodeIgnoreEscapes,
									 RTL_TEXTENCODING_UTF8 );
	else
		NewTitle = srcUncPath.copy( 1 + srcUncPath.lastIndexOf( sal_Unicode('/') ) );

	// Is destination a document or a folder ?
	Sequence< beans::Property > seq(1);
	seq[0] = beans::Property( rtl::OUString::createFromAscii("IsDocument"),
							  -1,
							  getCppuType( static_cast< sal_Bool* >(0) ),
							  0 );
	Reference< sdbc::XRow > xRow = getPropertyValues( nMyCommandIdentifier,seq );
	sal_Bool IsDocument = xRow->getBoolean( 1 );
	if( xRow->wasNull() )
	{   // Destination file type could not be determined
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_TRANSFER_DESTFILETYPE );
		return;
	}

	rtl::OUString dstUncPath;
	if( IsDocument )
	{   // as sibling
		sal_Int32 lastSlash = m_aUncPath.lastIndexOf( sal_Unicode('/') );
		dstUncPath = m_aUncPath.copy(0,lastSlash );
	}
	else
		// as child
		dstUncPath = m_aUncPath;

	dstUncPath += ( rtl::OUString::createFromAscii( "/" ) + NewTitle );

	sal_Int32 NameClash = aTransferInfo.NameClash;

	if( aTransferInfo.MoveData )
		m_pMyShell->move( nMyCommandIdentifier,srcUncPath,dstUncPath,NameClash );
	else
		m_pMyShell->copy( nMyCommandIdentifier,srcUncPath,dstUncPath,NameClash );
}




void SAL_CALL BaseContent::insert( sal_Int32 nMyCommandIdentifier,
								   const InsertCommandArgument& aInsertArgument )
	throw()
{
	if( m_nState & FullFeatured )
	{
		m_pMyShell->write( nMyCommandIdentifier,
						   m_aUncPath,
						   aInsertArgument.ReplaceExisting,
						   aInsertArgument.Data );
		return;
	}

	if( ! ( m_nState & JustInserted ) )
	{
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_NOFRESHINSERT_IN_INSERT_COMMAND );
		return;
	}

	// Inserts the content, which has the flag m_bIsFresh

	if( ! m_nState & NameForInsertionSet )
	{
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_NONAMESET_INSERT_COMMAND );
		return;
	}

	// Inserting a document or a file?
	sal_Bool bDocument = false;

	Sequence< beans::Property > seq(1);
	seq[0] = beans::Property( rtl::OUString::createFromAscii("IsDocument"),
							  -1,
							  getCppuType( static_cast< sal_Bool* >(0) ),
							  0 );

	Reference< sdbc::XRow > xRow = getPropertyValues( -1,seq );

	bool contentTypeSet = true;  // is set to false, if contentType not set
	try
	{
		bDocument = xRow->getBoolean( 1 );
		if( xRow->wasNull() )
			contentTypeSet = false;

	}
	catch ( sdbc::SQLException const & )
	{
		OSL_ENSURE( false,
					"BaseContent::insert - Caught SQLException!" );
		contentTypeSet = false;
	}

	if( ! contentTypeSet )
	{
		m_pMyShell->installError( nMyCommandIdentifier,
								  TASKHANDLING_NOCONTENTTYPE_INSERT_COMMAND );
		return;
	}


	sal_Bool success = false;
	if( bDocument )
		success = m_pMyShell->mkfil( nMyCommandIdentifier,
									 m_aUncPath,
									 aInsertArgument.ReplaceExisting,
									 aInsertArgument.Data );
	else
	{
		while( ! success )
		{
			success = m_pMyShell->mkdir( nMyCommandIdentifier,
										 m_aUncPath,
										 aInsertArgument.ReplaceExisting );
			if( success )
				break;

			XInteractionRequestImpl *aRequestImpl =
                new XInteractionRequestImpl(
                    rtl::Uri::decode(
						getTitle(m_aUncPath),
						rtl_UriDecodeWithCharset,
						RTL_TEXTENCODING_UTF8),
                    (cppu::OWeakObject*)this,
					m_pMyShell,nMyCommandIdentifier);
			uno::Reference< task::XInteractionRequest > aReq( aRequestImpl );

			m_pMyShell->handleTask( nMyCommandIdentifier,aReq );
			if(  aRequestImpl->aborted() ||
				 !aRequestImpl->newName().getLength() )
				// means aborting
				break;

			// determine new uncpath
			m_pMyShell->clearError( nMyCommandIdentifier );
			m_aUncPath = getParentName( m_aUncPath );
			if( m_aUncPath.lastIndexOf( sal_Unicode('/') ) != m_aUncPath.getLength() - 1 )
				m_aUncPath += rtl::OUString::createFromAscii("/");

			m_aUncPath += rtl::Uri::encode( aRequestImpl->newName(),
											rtl_UriCharClassPchar,
											rtl_UriEncodeIgnoreEscapes,
											RTL_TEXTENCODING_UTF8 );
		}
	}

    if ( ! success )
        return;

	FileContentIdentifier* p = new FileContentIdentifier( m_pMyShell,m_aUncPath );
	m_xContentIdentifier = Reference< XContentIdentifier >( p );

	m_pMyShell->registerNotifier( m_aUncPath,this );
	m_pMyShell->insertDefaultProperties( m_aUncPath );

	osl::MutexGuard aGuard( m_aMutex );
	m_nState = FullFeatured;
}



void SAL_CALL BaseContent::endTask( sal_Int32 CommandId )
{
	// This is the only function allowed to throw an exception
    m_pMyShell->endTask( CommandId,m_aUncPath,this );
}



ContentEventNotifier*
BaseContent::cDEL( void )
{
	osl::MutexGuard aGuard( m_aMutex );

	m_nState |= Deleted;

	ContentEventNotifier* p;
	if( m_pContentEventListeners )
		p = new ContentEventNotifier( m_pMyShell,
									  this,
									  m_xContentIdentifier,
									  m_pContentEventListeners->getElements() );
	else
		p = 0;

	return p;
}


ContentEventNotifier*
BaseContent::cEXC( const rtl::OUString aNewName )
{
	osl::MutexGuard aGuard( m_aMutex );

	Reference< XContentIdentifier > xOldRef = m_xContentIdentifier;
	m_aUncPath = aNewName;
	FileContentIdentifier* pp = new FileContentIdentifier( m_pMyShell,aNewName );
	m_xContentIdentifier = Reference< XContentIdentifier >( pp );

	ContentEventNotifier* p = 0;
	if( m_pContentEventListeners )
		p = new ContentEventNotifier( m_pMyShell,
									  this,
									  m_xContentIdentifier,
									  xOldRef,
									  m_pContentEventListeners->getElements() );

	return p;
}


ContentEventNotifier*
BaseContent::cCEL( void )
{
	osl::MutexGuard aGuard( m_aMutex );
	ContentEventNotifier* p = 0;
	if( m_pContentEventListeners )
		p = new ContentEventNotifier( m_pMyShell,
									  this,
									  m_xContentIdentifier,
									  m_pContentEventListeners->getElements() );

	return p;
}

PropertySetInfoChangeNotifier*
BaseContent::cPSL( void )
{
	osl::MutexGuard aGuard( m_aMutex );
	PropertySetInfoChangeNotifier* p = 0;
	if( m_pPropertySetInfoChangeListeners  )
		p = new PropertySetInfoChangeNotifier( m_pMyShell,
											   this,
											   m_xContentIdentifier,
											   m_pPropertySetInfoChangeListeners->getElements() );

	return p;
}



PropertyChangeNotifier*
BaseContent::cPCL( void )
{
	osl::MutexGuard aGuard( m_aMutex );

	Sequence< rtl::OUString > seqNames;

	if( m_pPropertyListener )
		seqNames = m_pPropertyListener->getContainedTypes();

	PropertyChangeNotifier* p = 0;

	sal_Int32 length = seqNames.getLength();

	if( length )
	{
		ListenerMap* listener = new ListenerMap();
		for( sal_Int32 i = 0; i < length; ++i )
		{
			(*listener)[seqNames[i]] = m_pPropertyListener->getContainer( seqNames[i] )->getElements();
		}

		p = new PropertyChangeNotifier( m_pMyShell,
										this,
										m_xContentIdentifier,
										listener );
	}

	return p;
}


rtl::OUString BaseContent::getKey( void )
{
	return m_aUncPath;
}
