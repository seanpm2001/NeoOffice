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
 *    Modified December 2005 by Patrick Luby. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/

#include "app.hxx"
#include "officeipcthread.hxx"
#include "cmdlineargs.hxx"
#include "dispatchwatcher.hxx"
#include <stdio.h>

#ifndef _VOS_PROCESS_HXX_
#include <vos/process.hxx>
#endif
#ifndef _UTL_BOOTSTRAP_HXX
#include <unotools/bootstrap.hxx>
#endif
#ifndef _SV_SVAPP_HXX
#include <vcl/svapp.hxx>
#endif
#ifndef _SV_HELP_HXX
#include <vcl/help.hxx>
#endif
#ifndef _UTL_CONFIGMGR_HXX_
#include <unotools/configmgr.hxx>
#endif
#ifndef _THREAD_HXX_
#include <osl/thread.hxx>
#endif
#ifndef _RTL_DIGEST_H_
#include <rtl/digest.h>
#endif
#ifndef _RTL_USTRBUF_HXX_
#include <rtl/ustrbuf.hxx>
#endif
#ifndef INCLUDED_RTL_INSTANCE_HXX
#include <rtl/instance.hxx>
#endif
#ifndef _OSL_CONDITN_HXX_
#include <osl/conditn.hxx>
#endif
#ifndef INCLUDED_SVTOOLS_MODULEOPTIONS_HXX
#include <svtools/moduleoptions.hxx>
#endif
#ifndef _RTL_BOOTSTRAP_HXX_
#include <rtl/bootstrap.hxx>
#endif
#include <comphelper/processfactory.hxx>
#include <com/sun/star/uri/XExternalUriReferenceTranslator.hpp>


using namespace vos;
using namespace rtl;
using namespace desktop;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::uri;

const char  *OfficeIPCThread::sc_aTerminationSequence = "InternalIPC::TerminateThread";
const int OfficeIPCThread::sc_nTSeqLength = 28;
const char  *OfficeIPCThread::sc_aShowSequence = "-tofront";
const int OfficeIPCThread::sc_nShSeqLength = 5;
const char  *OfficeIPCThread::sc_aConfirmationSequence = "InternalIPC::ProcessingDone";
const int OfficeIPCThread::sc_nCSeqLength = 27;

// Type of pipe we use
enum PipeMode
{
	PIPEMODE_DONTKNOW,
	PIPEMODE_CREATED,
	PIPEMODE_CONNECTED
};

namespace desktop
{

String GetURL_Impl( const String& rName );

OfficeIPCThread*	OfficeIPCThread::pGlobalOfficeIPCThread = 0;
namespace { struct Security : public rtl::Static<OSecurity, Security> {}; }
::osl::Mutex*		OfficeIPCThread::pOfficeIPCThreadMutex = 0;


String CreateMD5FromString( const OUString& aMsg )
{
	// PRE: aStr "file"
	// BACK: Str "ababab....0f" Hexcode String

	rtlDigest handle = rtl_digest_create( rtl_Digest_AlgorithmMD5 );
	if ( handle > 0 )
	{
		const sal_uInt8* pData = (const sal_uInt8*)aMsg.getStr();
		sal_uInt32		 nSize = ( aMsg.getLength() * sizeof( sal_Unicode ));
		sal_uInt32		 nMD5KeyLen = rtl_digest_queryLength( handle );
		sal_uInt8*		 pMD5KeyBuffer = new sal_uInt8[ nMD5KeyLen ];

		rtl_digest_init( handle, pData, nSize );
		rtl_digest_update( handle, pData, nSize );
		rtl_digest_get( handle, pMD5KeyBuffer, nMD5KeyLen );
		rtl_digest_destroy( handle );

		// Create hex-value string from the MD5 value to keep the string size minimal
		OUStringBuffer aBuffer( nMD5KeyLen * 2 + 1 );
		for ( sal_uInt32 i = 0; i < nMD5KeyLen; i++ )
			aBuffer.append( (sal_Int32)pMD5KeyBuffer[i], 16 );

		delete [] pMD5KeyBuffer;
		return aBuffer.makeStringAndClear();
	}

	return String();
}

class ProcessEventsClass_Impl
{
public:
	DECL_STATIC_LINK( ProcessEventsClass_Impl, CallEvent, void* pEvent );
	DECL_STATIC_LINK( ProcessEventsClass_Impl, ProcessDocumentsEvent, void* pEvent );
};

IMPL_STATIC_LINK( ProcessEventsClass_Impl, CallEvent, void*, pEvent )
{
	// Application events are processed by the Desktop::HandleAppEvent implementation.
	Desktop::HandleAppEvent( *((ApplicationEvent*)pEvent) );
	delete (ApplicationEvent*)pEvent;
	return 0;
}

IMPL_STATIC_LINK( ProcessEventsClass_Impl, ProcessDocumentsEvent, void*, pEvent )
{
	// Documents requests are processed by the OfficeIPCThread implementation
	ProcessDocumentsRequest* pDocsRequest = (ProcessDocumentsRequest*)pEvent;

	if ( pDocsRequest )
	{
		OfficeIPCThread::ExecuteCmdLineRequests( *pDocsRequest );
		delete pDocsRequest;
	}
	return 0;
}

void ImplPostForeignAppEvent( ApplicationEvent* pEvent )
{
	Application::PostUserEvent( STATIC_LINK( NULL, ProcessEventsClass_Impl, CallEvent ), pEvent );
}

void ImplPostProcessDocumentsEvent( ProcessDocumentsRequest* pEvent )
{
	Application::PostUserEvent( STATIC_LINK( NULL, ProcessEventsClass_Impl, ProcessDocumentsEvent ), pEvent );
}

OSignalHandler::TSignalAction SAL_CALL SalMainPipeExchangeSignalHandler::signal(TSignalInfo *pInfo)
{
    if( pInfo->Signal == osl_Signal_Terminate )
		OfficeIPCThread::DisableOfficeIPCThread();
	return (TAction_CallNextHandler);
}

// ----------------------------------------------------------------------------

// The OfficeIPCThreadController implementation is a bookkeeper for all pending requests
// that were created by the OfficeIPCThread. The requests are waiting to be processed by
// our framework loadComponentFromURL function (e.g. open/print request).
// During shutdown the framework is asking OfficeIPCThreadController about pending requests.
// If there are pending requests framework has to stop the shutdown process. It is waiting
// for these requests because framework is not able to handle shutdown and open a document
// concurrently.


// XServiceInfo
OUString SAL_CALL OfficeIPCThreadController::getImplementationName()
throw ( RuntimeException )
{
	return OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.comp.OfficeIPCThreadController" ));
}

sal_Bool SAL_CALL OfficeIPCThreadController::supportsService( const OUString& ServiceName )
throw ( RuntimeException )
{
	return sal_False;
}

Sequence< OUString > SAL_CALL OfficeIPCThreadController::getSupportedServiceNames()
throw ( RuntimeException )
{
	Sequence< OUString > aSeq( 0 );
	return aSeq;
}

// XEventListener
void SAL_CALL OfficeIPCThreadController::disposing( const EventObject& Source )
throw( RuntimeException )
{
}

// XTerminateListener
void SAL_CALL OfficeIPCThreadController::queryTermination( const EventObject& aEvent )
throw( TerminationVetoException, RuntimeException )
{
	// Desktop ask about pending request through our office ipc pipe. We have to
	// be sure that no pending request is waiting because framework is not able to
	// handle shutdown and open a document concurrently.

	if ( OfficeIPCThread::AreRequestsPending() )
		throw TerminationVetoException();
	else
		OfficeIPCThread::BlockAllRequests();
}

void SAL_CALL OfficeIPCThreadController::notifyTermination( const EventObject& aEvent )
throw( RuntimeException )
{
}

// ----------------------------------------------------------------------------

::osl::Mutex&	OfficeIPCThread::GetMutex()
{
	// Get or create our mutex for thread-saftey
	if ( !pOfficeIPCThreadMutex )
	{
		::osl::MutexGuard aGuard( osl::Mutex::getGlobalMutex() );
		if ( !pOfficeIPCThreadMutex )
			pOfficeIPCThreadMutex = new osl::Mutex;
	}

	return *pOfficeIPCThreadMutex;
}

OfficeIPCThread* OfficeIPCThread::GetOfficeIPCThread()
{
	// Return the one and only OfficeIPCThread pointer
	::osl::MutexGuard	aGuard( GetMutex() );
	return pGlobalOfficeIPCThread;
}

void OfficeIPCThread::BlockAllRequests()
{
	// We have the order to block all incoming requests. Framework
	// wants to shutdown and we have to make sure that no loading/printing
	// requests are executed anymore.
	::osl::MutexGuard	aGuard( GetMutex() );

	if ( pGlobalOfficeIPCThread )
		pGlobalOfficeIPCThread->mbBlockRequests = sal_True;
}

sal_Bool OfficeIPCThread::AreRequestsPending()
{
	// Give info about pending requests
	::osl::MutexGuard	aGuard( GetMutex() );
	if ( pGlobalOfficeIPCThread )
		return ( pGlobalOfficeIPCThread->mnPendingRequests > 0 );
	else
		return sal_False;
}

void OfficeIPCThread::RequestsCompleted( int nCount )
{
	// Remove nCount pending requests from our internal counter
	::osl::MutexGuard	aGuard( GetMutex() );
	if ( pGlobalOfficeIPCThread )
	{
		if ( pGlobalOfficeIPCThread->mnPendingRequests > 0 )
			pGlobalOfficeIPCThread->mnPendingRequests -= nCount;
	}
}

OfficeIPCThread::Status OfficeIPCThread::EnableOfficeIPCThread()
{
	::osl::MutexGuard	aGuard( GetMutex() );

	if( pGlobalOfficeIPCThread )
		return IPC_STATUS_OK;

	::rtl::OUString aUserInstallPath;
    ::rtl::OUString aDummy;

	::vos::OStartupInfo aInfo;
	OfficeIPCThread* pThread = new OfficeIPCThread;

#ifdef PRODUCT_DIR_NAME
	pThread->maPipeIdent = OUString( RTL_CONSTASCII_USTRINGPARAM( "Single" PRODUCT_DIR_NAME "IPC_" ) );
#else
	pThread->maPipeIdent = OUString( RTL_CONSTASCII_USTRINGPARAM( "SingleOfficeIPC_" ) );
#endif

	// The name of the named pipe is created with the hashcode of the user installation directory (without /user). We have to retrieve
	// this information from a unotools implementation.
	::utl::Bootstrap::PathStatus aLocateResult = ::utl::Bootstrap::locateUserInstallation( aUserInstallPath );
	if ( aLocateResult == ::utl::Bootstrap::PATH_EXISTS || aLocateResult == ::utl::Bootstrap::PATH_VALID)
		aDummy = aUserInstallPath;
	else
	{
		delete pThread;
		return IPC_STATUS_BOOTSTRAP_ERROR;
	}

	// Try to  determine if we are the first office or not! This should prevent multiple
	// access to the user directory !
	// First we try to create our pipe if this fails we try to connect. We have to do this
	// in a loop because the the other office can crash or shutdown between createPipe
	// and connectPipe!!

    OUString            aIniName;
    
    aInfo.getExecutableFile( aIniName );
    sal_uInt32     lastIndex = aIniName.lastIndexOf('/');
    if ( lastIndex > 0 )
    {
        aIniName    = aIniName.copy( 0, lastIndex+1 );
        aIniName    += OUString( RTL_CONSTASCII_USTRINGPARAM( "perftune" ));
#ifdef WNT
        aIniName    += OUString( RTL_CONSTASCII_USTRINGPARAM( ".ini" ));
#else
        aIniName    += OUString( RTL_CONSTASCII_USTRINGPARAM( "rc" ));
#endif
    }
   
	::rtl::Bootstrap aPerfTuneIniFile( aIniName );
    
    OUString aDefault( RTL_CONSTASCII_USTRINGPARAM( "0" ));
    OUString aPreloadData;
    
    aPerfTuneIniFile.getFrom( OUString( RTL_CONSTASCII_USTRINGPARAM( "FastPipeCommunication" )), aPreloadData, aDefault );


	OUString aUserInstallPathHashCode;

    if ( aPreloadData.equalsAscii( "1" ))
    {
		sal_Char	szBuffer[32];
		sprintf( szBuffer, "%d", SUPD );
		aUserInstallPathHashCode = OUString( szBuffer, strlen(szBuffer), osl_getThreadTextEncoding() );
    }
	else
		aUserInstallPathHashCode = CreateMD5FromString( aDummy );
	

	// Check result to create a hash code from the user install path
	if ( aUserInstallPathHashCode.getLength() == 0 )
		return IPC_STATUS_BOOTSTRAP_ERROR; // Something completely broken, we cannot create a valid hash code!

	pThread->maPipeIdent = pThread->maPipeIdent + aUserInstallPathHashCode;

	PipeMode nPipeMode = PIPEMODE_DONTKNOW;
	do
	{
		OSecurity &rSecurity = Security::get();
		// Try to create pipe
		if ( pThread->maPipe.create( pThread->maPipeIdent.getStr(), OPipe::TOption_Create, rSecurity ))
		{
			// Pipe created
			nPipeMode = PIPEMODE_CREATED;
		}
		else if( pThread->maPipe.create( pThread->maPipeIdent.getStr(), OPipe::TOption_Open, rSecurity )) // Creation not successfull, now we try to connect
		{
			// Pipe connected to first office
			nPipeMode = PIPEMODE_CONNECTED;
		}
		else
		{
			OPipe::TPipeError eReason = pThread->maPipe.getError();
			if ((eReason == OPipe::E_ConnectionRefused) || (eReason == OPipe::E_invalidError))
				return IPC_STATUS_BOOTSTRAP_ERROR;

			// Wait for second office to be ready
			TimeValue aTimeValue;
			aTimeValue.Seconds = 0;
			aTimeValue.Nanosec = 10000000; // 10ms
			osl::Thread::wait( aTimeValue );
		}

	} while ( nPipeMode == PIPEMODE_DONTKNOW );

	if ( nPipeMode == PIPEMODE_CREATED )
	{
		// Seems we are the one and only, so start listening thread
		pGlobalOfficeIPCThread = pThread;
		pThread->create(); // starts thread
	}
	else
	{
		// Seems another office is running. Pipe arguments to it and self terminate
		pThread->maStreamPipe = pThread->maPipe;

		sal_Bool bWaitBeforeClose = sal_False;
		ByteString aArguments;
        ULONG nCount = aInfo.getCommandArgCount();

        if ( nCount == 0 )
		{
			// Use default argument so the first office can distinguish between a real second
			// office and another program that check the existence of the the pipe!!

			aArguments += ByteString( sc_aShowSequence );
		}
		else
		{
            sal_Bool    bPrintTo = sal_False;
            OUString    aPrintToCmd( RTL_CONSTASCII_USTRINGPARAM( "-pt" ));

            Reference< XExternalUriReferenceTranslator > xTranslator(
                comphelper::getProcessServiceFactory()->createInstance(
                OUString::createFromAscii(
                "com.sun.star.uri.ExternalUriReferenceTranslator")),
                UNO_QUERY);

            for( ULONG i=0; i < nCount; i++ )
			{
				aInfo.getCommandArg( i, aDummy );
				// Make absolute pathes from relative ones!
				// It's neccessary to use current working directory of THESE office instance and not of
				// currently running once, which get these information by using pipe.
				// Otherwhise relativ pathes are not right for his environment ...
				if( aDummy.indexOf('-',0) != 0 )
				{
                    bWaitBeforeClose = sal_True;
                    // convert to an absolut url, but don't touch extrnal file URLs here
                    if ( !bPrintTo)
                    {
                        // convert file URLs to internal form #112849#
                        if (aDummy.indexOf(OUString::createFromAscii("file:"))==0 &&
                            xTranslator.is())
                        {
                            OUString tmp(xTranslator->translateToInternal(aDummy));
                            if (tmp.getLength() > 0)
                                aDummy = tmp;
                        }
					    aDummy = GetURL_Impl( aDummy );
                    }
                    bPrintTo = sal_False;
				}
                else
                {
                    if ( aDummy.equalsIgnoreAsciiCase( aPrintToCmd ))
                        bPrintTo = sal_True;
                    else
                        bPrintTo = sal_False;
                }

				aArguments += ByteString( String( aDummy ), osl_getThreadTextEncoding() );
				aArguments += '|';
			}
		}
		// finaly, write the string onto the pipe
		pThread->maStreamPipe.write( aArguments.GetBuffer(), aArguments.Len() );
		pThread->maStreamPipe.write( "\0", 1 );

		// wait for confirmation #95361# #95425#
		ByteString aToken(sc_aConfirmationSequence);
		char *aReceiveBuffer = new char[aToken.Len()+1];
		int n = pThread->maStreamPipe.read( aReceiveBuffer, aToken.Len() );
		aReceiveBuffer[n]='\0';

		delete pThread;
		if (aToken.CompareTo(aReceiveBuffer)!= COMPARE_EQUAL) {
			// something went wrong
			delete aReceiveBuffer;
			return IPC_STATUS_BOOTSTRAP_ERROR;
		} else {
			delete aReceiveBuffer;
			return IPC_STATUS_2ND_OFFICE;
		}
	}

	return IPC_STATUS_OK;
}

void OfficeIPCThread::DisableOfficeIPCThread()
{
	osl::ClearableMutexGuard aMutex( GetMutex() );

	if( pGlobalOfficeIPCThread )
	{
        OfficeIPCThread *pOfficeIPCThread = pGlobalOfficeIPCThread;
		pGlobalOfficeIPCThread = 0;

		// send thread a termination message
		// this is done so the subsequent join will not hang
		// because the thread hangs in accept of pipe
        OPipe Pipe( pOfficeIPCThread->maPipeIdent, OPipe::TOption_Open, Security::get() );
		//Pipe.send( TERMINATION_SEQUENCE, TERMINATION_LENGTH );
        if (Pipe.isValid())
        {
    		Pipe.send( sc_aTerminationSequence, sc_nTSeqLength+1 ); // also send 0-byte

	    	// close the pipe so that the streampipe on the other
    		// side produces EOF
	    	Pipe.close();
        }

		// release mutex to avoid deadlocks
		aMutex.clear();

        OfficeIPCThread::SetReady(pOfficeIPCThread);

		// exit gracefully and join
		pOfficeIPCThread->join();
		delete pOfficeIPCThread;


	}
}

OfficeIPCThread::OfficeIPCThread() :
	mbBlockRequests( sal_False ),
	mnPendingRequests( 0 ),
	mpDispatchWatcher( 0 )
{
}

OfficeIPCThread::~OfficeIPCThread()
{
	::osl::ClearableMutexGuard	aGuard( GetMutex() );

	if ( mpDispatchWatcher )
		mpDispatchWatcher->release();
	maPipe.close();
	maStreamPipe.close();
	pGlobalOfficeIPCThread = 0;
}

static void AddURLToStringList( const rtl::OUString& aURL, rtl::OUString& aStringList )
{
	if ( aStringList.getLength() )
		aStringList += ::rtl::OUString::valueOf( (sal_Unicode)APPEVENT_PARAM_DELIMITER );
	aStringList += aURL;
}

void OfficeIPCThread::SetReady(OfficeIPCThread* pThread)
{
    if (pThread == NULL) pThread = pGlobalOfficeIPCThread;
    if (pThread != NULL)
    {
        pThread->cReady.set();
    }
}

void SAL_CALL OfficeIPCThread::run()
{
    do
	{
        OPipe::TPipeError
			nError = maPipe.accept( maStreamPipe );


		if( nError == OStreamPipe::E_None )
		{

            // #111143# and others:
            // if we receive a request while the office is displaying some dialog or error during
            // bootstrap, that dialogs event loop might get events that are dispatched by this thread
            // we have to wait for cReady to be set by the real main loop.
            // only reqests that dont dispatch events may be processed before cReady is set.
            cReady.wait();

            // we might have decided to shutdown while we were sleeping
            if (!pGlobalOfficeIPCThread) return;

            // only lock the mutex when processing starts, othewise we deadlock when the office goes
            // down during wait
            osl::ClearableMutexGuard aGuard( GetMutex() );

            ByteString aArguments;
            // test byte by byte
            const int nBufSz = 2048;
            char pBuf[nBufSz];
            int nBytes = 0;
            int nResult = 0;
            // read into pBuf until '\0' is read or read-error
            while ((nResult=maStreamPipe.recv( pBuf+nBytes, nBufSz-nBytes))>0) {
				nBytes += nResult;
				if (pBuf[nBytes-1]=='\0') {
					aArguments += pBuf;
			        break;
		        }
	        }
			// don't close pipe ...

			// #90717# Is this a lookup message from another application? if so, ignore
			if ( aArguments.Len() == 0 )
				continue;

            // is this a termination message ? if so, terminate
            if(( aArguments.CompareTo( sc_aTerminationSequence, sc_nTSeqLength ) == COMPARE_EQUAL ) ||
                    mbBlockRequests ) return;
            String           aEmpty;
            CommandLineArgs  aCmdLineArgs( OUString( aArguments.GetBuffer(),
				aArguments.Len(), gsl_getSystemTextEncoding() ));
            CommandLineArgs	*pCurrentCmdLineArgs = Desktop::GetCommandLineArgs();

			if ( aCmdLineArgs.IsQuickstart() )
			{
				// we have to use application event, because we have to start quickstart service in main thread!!
				ApplicationEvent* pAppEvent =
					new ApplicationEvent( aEmpty, aEmpty,
											"QUICKSTART", aEmpty );
				ImplPostForeignAppEvent( pAppEvent );
			}

			// handle request for acceptor
			sal_Bool bAcceptorRequest = sal_False;
			OUString aAcceptString;
            if ( aCmdLineArgs.GetAcceptString(aAcceptString) && Desktop::CheckOEM()) {
				ApplicationEvent* pAppEvent =
					new ApplicationEvent( aEmpty, aEmpty,
										  "ACCEPT", aAcceptString );
				ImplPostForeignAppEvent( pAppEvent );
				bAcceptorRequest = sal_True;
			}
			// handle acceptor removal
			OUString aUnAcceptString;
			if ( aCmdLineArgs.GetUnAcceptString(aUnAcceptString) ) {
				ApplicationEvent* pAppEvent =
					new ApplicationEvent( aEmpty, aEmpty,
										 "UNACCEPT", aUnAcceptString );
				ImplPostForeignAppEvent( pAppEvent );
				bAcceptorRequest = sal_True;
			}

#ifndef UNX
			// only in non-unix version, we need to handle a -help request
			// in a running instance in order to display  the command line help
			if ( aCmdLineArgs.IsHelp() ) {
				ApplicationEvent* pAppEvent =
					new ApplicationEvent( aEmpty, aEmpty, "HELP", aEmpty );
				ImplPostForeignAppEvent( pAppEvent );
			}
#endif

			sal_Bool bDocRequestSent = sal_False;
			ProcessDocumentsRequest* pRequest = new ProcessDocumentsRequest;
            cProcessed.reset();
            pRequest->pcProcessed = &cProcessed;

			// Print requests are not dependent on the -invisible cmdline argument as they are
			// loaded with the "hidden" flag! So they are always checked.
			bDocRequestSent |= aCmdLineArgs.GetPrintList( pRequest->aPrintList );
			bDocRequestSent |= ( aCmdLineArgs.GetPrintToList( pRequest->aPrintToList ) &&
									aCmdLineArgs.GetPrinterName( pRequest->aPrinterName )		);

			if ( !pCurrentCmdLineArgs->IsInvisible() )
			{
				// Read cmdline args that can open/create documents. As they would open a window
				// they are only allowed if the "-invisible" is currently not used!
				bDocRequestSent |= aCmdLineArgs.GetOpenList( pRequest->aOpenList );
				bDocRequestSent |= aCmdLineArgs.GetViewList( pRequest->aViewList );
                bDocRequestSent |= aCmdLineArgs.GetStartList( pRequest->aStartList );
				bDocRequestSent |= aCmdLineArgs.GetForceOpenList( pRequest->aForceOpenList );
				bDocRequestSent |= aCmdLineArgs.GetForceNewList( pRequest->aForceNewList );

				// Special command line args to create an empty document for a given module

                // #i18338# (lo)
                // we only do this if no document was specified on the command line,
                // since this would be inconsistent with the the behaviour of
                // the first process, see OpenClients() (call to OpenDefault()) in app.cxx
                if ( aCmdLineArgs.HasModuleParam() && Desktop::CheckOEM() && (!bDocRequestSent))
				{
					SvtModuleOptions aOpt;
					SvtModuleOptions::EFactory eFactory = SvtModuleOptions::E_WRITER;
					if ( aCmdLineArgs.IsWriter() )
						eFactory = SvtModuleOptions::E_WRITER;
					else if ( aCmdLineArgs.IsCalc() )
						eFactory = SvtModuleOptions::E_CALC;
					else if ( aCmdLineArgs.IsDraw() )
						eFactory = SvtModuleOptions::E_DRAW;
					else if ( aCmdLineArgs.IsImpress() )
						eFactory = SvtModuleOptions::E_IMPRESS;
					else if ( aCmdLineArgs.IsBase() )
						eFactory = SvtModuleOptions::E_DATABASE;
					else if ( aCmdLineArgs.IsMath() )
						eFactory = SvtModuleOptions::E_MATH;
					else if ( aCmdLineArgs.IsGlobal() )
						eFactory = SvtModuleOptions::E_WRITERGLOBAL;
					else if ( aCmdLineArgs.IsWeb() )
						eFactory = SvtModuleOptions::E_WRITERWEB;

                    if ( pRequest->aOpenList.getLength() )
                        pRequest->aModule = aOpt.GetFactoryName( eFactory );
                    else
                        AddURLToStringList( aOpt.GetFactoryEmptyDocumentURL( eFactory ), pRequest->aOpenList );
					bDocRequestSent = sal_True;
				}
            }

            if (!aCmdLineArgs.IsQuickstart() && Desktop::CheckOEM()) {
                sal_Bool bShowHelp = sal_False;
                rtl::OUStringBuffer aHelpURLBuffer;
                if (aCmdLineArgs.IsHelpWriter()) {
                    bShowHelp = sal_True;
                    aHelpURLBuffer.appendAscii("vnd.sun.star.help://swriter/start");
                } else if (aCmdLineArgs.IsHelpCalc()) {
                    bShowHelp = sal_True;
                    aHelpURLBuffer.appendAscii("vnd.sun.star.help://scalc/start");
                } else if (aCmdLineArgs.IsHelpDraw()) {
                    bShowHelp = sal_True;
                    aHelpURLBuffer.appendAscii("vnd.sun.star.help://sdraw/start");
                } else if (aCmdLineArgs.IsHelpImpress()) {
                    bShowHelp = sal_True;
                    aHelpURLBuffer.appendAscii("vnd.sun.star.help://simpress/start");
				} else if (aCmdLineArgs.IsHelpBase()) {
                    bShowHelp = sal_True;
                    aHelpURLBuffer.appendAscii("vnd.sun.star.help://sdatabase/start");
                } else if (aCmdLineArgs.IsHelpBasic()) {
                    bShowHelp = sal_True;
                    aHelpURLBuffer.appendAscii("vnd.sun.star.help://sbasic/start");
                } else if (aCmdLineArgs.IsHelpMath()) {
                    bShowHelp = sal_True;
                    aHelpURLBuffer.appendAscii("vnd.sun.star.help://smath/start");
                }
                if (bShowHelp) {
                    Any aRet = ::utl::ConfigManager::GetDirectConfigProperty( ::utl::ConfigManager::LOCALE );
                    rtl::OUString aTmp;
                    aRet >>= aTmp;
                    aHelpURLBuffer.appendAscii("?Language=");
                    aHelpURLBuffer.append(aTmp);
#if defined UNX
                    aHelpURLBuffer.appendAscii("&System=UNX");
#elif defined WNT
                    aHelpURLBuffer.appendAscii("&System=WIN");
#elif defined MAC
                    aHelpURLBuffer.appendAscii("&System=MAC");
#endif
                    ApplicationEvent* pAppEvent =
                        new ApplicationEvent( aEmpty, aEmpty,
                                              "OPENHELPURL", aHelpURLBuffer.makeStringAndClear());
                    ImplPostForeignAppEvent( pAppEvent );
                }
            }

            if ( bDocRequestSent && Desktop::CheckOEM())
 			{
				// Send requests to dispatch watcher if we have at least one. The receiver
				// is responsible to delete the request after processing it.
                if ( aCmdLineArgs.HasModuleParam() )
                {
                    SvtModuleOptions    aOpt;

                    // Support command line parameters to start a module (as preselection)
                    if ( aCmdLineArgs.IsWriter() && aOpt.IsModuleInstalled( SvtModuleOptions::E_SWRITER ) )
                        pRequest->aModule = aOpt.GetFactoryName( SvtModuleOptions::E_WRITER );
                    else if ( aCmdLineArgs.IsCalc() && aOpt.IsModuleInstalled( SvtModuleOptions::E_SCALC ) )
                        pRequest->aModule = aOpt.GetFactoryName( SvtModuleOptions::E_CALC );
                    else if ( aCmdLineArgs.IsImpress() && aOpt.IsModuleInstalled( SvtModuleOptions::E_SIMPRESS ) )
                        pRequest->aModule= aOpt.GetFactoryName( SvtModuleOptions::E_IMPRESS );
                    else if ( aCmdLineArgs.IsDraw() && aOpt.IsModuleInstalled( SvtModuleOptions::E_SDRAW ) )
                        pRequest->aModule= aOpt.GetFactoryName( SvtModuleOptions::E_DRAW );
                }


				ImplPostProcessDocumentsEvent( pRequest );
			}
			else
			{
				// delete not used request again
				delete pRequest;
				pRequest = NULL;
			}
			if ((( aArguments.CompareTo( sc_aShowSequence, sc_nShSeqLength ) == COMPARE_EQUAL ) ||
				!bDocRequestSent ) && !bAcceptorRequest )
			{
				// no document was sent, just bring Office to front
				ApplicationEvent* pAppEvent =
						new ApplicationEvent( aEmpty, aEmpty, "APPEAR", aEmpty );
				ImplPostForeignAppEvent( pAppEvent );
			}

			// we don't need the mutex any longer...
			aGuard.clear();
			// wait for processing to finish
            if (bDocRequestSent)
    			cProcessed.wait();
			// processing finished, inform the requesting end
			nBytes = 0;
			while (
                   (nResult = maStreamPipe.send(sc_aConfirmationSequence+nBytes, sc_nCSeqLength-nBytes))>0 &&
                   ((nBytes += nResult) < sc_nCSeqLength) );
			// now we can close, don't we?
			// maStreamPipe.close();

        }
        else
        {
#if (OSL_DEBUG_LEVEL > 1) || defined DBG_UTIL
			fprintf( stderr, "Error on accept: %d\n", (int)nError );
#endif
			TimeValue tval;
			tval.Seconds = 1;
			tval.Nanosec = 0;
			sleep( tval );
		}
	} while( schedule() );
}

static void AddToDispatchList(
	DispatchWatcher::DispatchList& rDispatchList,
	const OUString& aRequestList,
	DispatchWatcher::RequestType nType,
    const OUString& aParam,
    const OUString& aFactory )
{
	if ( aRequestList.getLength() > 0 )
	{
		sal_Int32 nIndex = 0;
		do
		{
			OUString aToken = aRequestList.getToken( 0, APPEVENT_PARAM_DELIMITER, nIndex );
			if ( aToken.getLength() > 0 )
				rDispatchList.push_back(
                    DispatchWatcher::DispatchRequest( nType, aToken, aParam, aFactory ));
		}
		while ( nIndex >= 0 );
	}
}

void OfficeIPCThread::ExecuteCmdLineRequests( ProcessDocumentsRequest& aRequest )
{
	::rtl::OUString					aEmpty;
	DispatchWatcher::DispatchList	aDispatchList;

	// Create dispatch list for dispatch watcher
    AddToDispatchList( aDispatchList, aRequest.aOpenList, DispatchWatcher::REQUEST_OPEN, aEmpty, aRequest.aModule );
    AddToDispatchList( aDispatchList, aRequest.aViewList, DispatchWatcher::REQUEST_VIEW, aEmpty, aRequest.aModule );
    AddToDispatchList( aDispatchList, aRequest.aStartList, DispatchWatcher::REQUEST_START, aEmpty, aRequest.aModule );
    AddToDispatchList( aDispatchList, aRequest.aPrintList, DispatchWatcher::REQUEST_PRINT, aEmpty, aRequest.aModule );
    AddToDispatchList( aDispatchList, aRequest.aPrintToList, DispatchWatcher::REQUEST_PRINTTO, aRequest.aPrinterName, aRequest.aModule );
    AddToDispatchList( aDispatchList, aRequest.aForceOpenList, DispatchWatcher::REQUEST_FORCEOPEN, aEmpty, aRequest.aModule );
    AddToDispatchList( aDispatchList, aRequest.aForceNewList, DispatchWatcher::REQUEST_FORCENEW, aEmpty, aRequest.aModule );

	osl::ClearableMutexGuard aGuard( GetMutex() );

	if ( pGlobalOfficeIPCThread )
	{
		pGlobalOfficeIPCThread->mnPendingRequests += aDispatchList.size();
		if ( !pGlobalOfficeIPCThread->mpDispatchWatcher )
		{
			pGlobalOfficeIPCThread->mpDispatchWatcher = DispatchWatcher::GetDispatchWatcher();
			pGlobalOfficeIPCThread->mpDispatchWatcher->acquire();
		}

		aGuard.clear();

		// Execute dispatch requests
		pGlobalOfficeIPCThread->mpDispatchWatcher->executeDispatchRequests( aDispatchList );

		// set processed flag
		if (aRequest.pcProcessed != NULL)
			aRequest.pcProcessed->set();
	}
}

}
