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
 *    Modified August 2006 by Patrick Luby. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/

// INCLUDE ---------------------------------------------------------------

#ifdef MAC
	// StackSpace
#include <mac_start.h>
#include <Memory.h>
#include <mac_end.h>
#endif

#include <svtools/zforlist.hxx>

#include "scitems.hxx"
#include "attrib.hxx"
#include "cell.hxx"
#include "compiler.hxx"
#include "interpre.hxx"
#include "document.hxx"
#include "scmatrix.hxx"
#include "dociter.hxx"
#include "docoptio.hxx"
#include "rechead.hxx"
#include "rangenam.hxx"
#include "brdcst.hxx"
#include "ddelink.hxx"
#include "validat.hxx"
#include "progress.hxx"
#include "editutil.hxx"
#include "recursionhelper.hxx"
#ifndef _EDITOBJ_HXX
#include <svx/editobj.hxx>
#endif
#define ITEMID_FIELD EE_FEATURE_FIELD
#ifndef _SFXINTITEM_HXX
#include <svtools/intitem.hxx>
#endif
#ifndef _SVX_FLDITEM_HXX
#include <svx/flditem.hxx>
#endif

#ifndef _SVT_BROADCAST_HXX
#include <svtools/broadcast.hxx>
#endif

// More or less arbitrary, of course all recursions must fit into available
// stack space (which is what on all systems we don't know yet?). Choosing a
// lower value may be better than trying a much higher value that also isn't
// sufficient but temporarily leads to high memory consumption. On the other
// hand, if the value fits all recursions, execution is quicker as no resumes
// are necessary. Could be made a configurable option.
// Allow for a year's calendar (366).
#if defined MACOSX && defined POWERPC
// Fix bug 1571 by limiting max recursion size
const USHORT MAXRECURSION = 200;
#else	// MACOSX && POWERPC
const USHORT MAXRECURSION = 400;
#endif	// MACOSX && POWERPC

// STATIC DATA -----------------------------------------------------------

#ifdef USE_MEMPOOL
// MemPools auf 4k Boundaries  - 64 Bytes ausrichten
const USHORT nMemPoolValueCell = (0x8000 - 64) / sizeof(ScValueCell);
const USHORT nMemPoolFormulaCell = (0x8000 - 64) / sizeof(ScFormulaCell);
const USHORT nMemPoolStringCell = (0x4000 - 64) / sizeof(ScStringCell);
const USHORT nMemPoolNoteCell = (0x1000 - 64) / sizeof(ScNoteCell);
IMPL_FIXEDMEMPOOL_NEWDEL( ScValueCell,   nMemPoolValueCell, nMemPoolValueCell )
IMPL_FIXEDMEMPOOL_NEWDEL( ScFormulaCell, nMemPoolFormulaCell, nMemPoolFormulaCell )
IMPL_FIXEDMEMPOOL_NEWDEL( ScStringCell,  nMemPoolStringCell, nMemPoolStringCell )
IMPL_FIXEDMEMPOOL_NEWDEL( ScNoteCell,	 nMemPoolNoteCell, nMemPoolNoteCell )
#endif

#ifndef PRODUCT
static const sal_Char __FAR_DATA msgDbgInfinity[] =
	"Formelzelle INFINITY ohne Err503 !!! (os/2?)\n"
	"NICHTS anruehren und ER bescheid sagen!";
#endif

// -----------------------------------------------------------------------

ScBaseCell* ScBaseCell::Clone(ScDocument* pDoc) const
{
	switch (eCellType)
	{
		case CELLTYPE_VALUE:
			return new ScValueCell(*(const ScValueCell*)this, pDoc);
		case CELLTYPE_STRING:
			return new ScStringCell(*(const ScStringCell*)this, pDoc);
		case CELLTYPE_EDIT:
			return new ScEditCell(*(const ScEditCell*)this, pDoc);
		case CELLTYPE_FORMULA:
			return new ScFormulaCell(pDoc, ((ScFormulaCell*)this)->aPos,
				*(const ScFormulaCell*)this);
		case CELLTYPE_NOTE:
			return new ScNoteCell(*(const ScNoteCell*)this, pDoc);
		default:
			DBG_ERROR("Unbekannter Zellentyp");
			return NULL;
	}
}

ScBaseCell::~ScBaseCell()
{
	delete pNote;
	delete pBroadcaster;
	DBG_ASSERT( eCellType == CELLTYPE_DESTROYED, "BaseCell Destructor" );
}

void ScBaseCell::Delete()
{
	DELETEZ(pNote);
	switch (eCellType)
	{
		case CELLTYPE_VALUE:
			delete (ScValueCell*) this;
			break;
		case CELLTYPE_STRING:
			delete (ScStringCell*) this;
			break;
		case CELLTYPE_EDIT:
			delete (ScEditCell*) this;
			break;
		case CELLTYPE_FORMULA:
			delete (ScFormulaCell*) this;
			break;
		case CELLTYPE_NOTE:
			delete (ScNoteCell*) this;
			break;
		default:
			DBG_ERROR("Unbekannter Zellentyp");
			break;
	}
}

void ScBaseCell::SetNote( const ScPostIt& rNote )
{
	if (!rNote.IsEmpty())
	{
		if (!pNote)
			pNote = new ScPostIt(rNote);
		else
			*pNote = rNote;
	}
	else
		DELETEZ(pNote);
}

BOOL ScBaseCell::GetNote( ScPostIt& rNote ) const
{
	if ( pNote )
		rNote = *pNote;
	else
		rNote.Clear();

	return ( pNote != NULL );
}

ScBaseCell* ScBaseCell::CreateTextCell( const String& rString, ScDocument* pDoc )
{
	if ( rString.Search('\n') != STRING_NOTFOUND || rString.Search(CHAR_CR) != STRING_NOTFOUND )
		return new ScEditCell( rString, pDoc );
	else
		return new ScStringCell( rString );
}

void ScBaseCell::LoadNote( SvStream& rStream, ScDocument* pDoc )
{
	pNote = new ScPostIt(pDoc);
	rStream >> *pNote;
}

void ScBaseCell::SetBroadcaster(SvtBroadcaster* pNew)
{
	delete pBroadcaster;
	pBroadcaster = pNew;
}

void ScBaseCell::StartListeningTo( ScDocument* pDoc )
{
	if ( eCellType == CELLTYPE_FORMULA && !pDoc->IsClipOrUndo()
			&& !pDoc->GetNoListening()
			&& !((ScFormulaCell*)this)->IsInChangeTrack()
		)
	{
		pDoc->SetDetectiveDirty(TRUE);	// es hat sich was geaendert...

		ScFormulaCell* pFormCell = (ScFormulaCell*)this;
        ScTokenArray* pArr = pFormCell->GetCode();
		if( pArr->IsRecalcModeAlways() )
			pDoc->StartListeningArea( BCA_LISTEN_ALWAYS, pFormCell );
		else
		{
			pArr->Reset();
			for( ScToken* t = pArr->GetNextReferenceRPN(); t;
						  t = pArr->GetNextReferenceRPN() )
			{
				StackVar eType = t->GetType();
				SingleRefData& rRef1 = t->GetSingleRef();
				SingleRefData& rRef2 = (eType == svDoubleRef ?
					t->GetDoubleRef().Ref2 : rRef1);
                switch( eType )
                {
                    case svSingleRef:
                        rRef1.CalcAbsIfRel( pFormCell->aPos );
                        if ( rRef1.Valid() )
                        {
                            pDoc->StartListeningCell(
                                ScAddress( rRef1.nCol,
                                           rRef1.nRow,
                                           rRef1.nTab ), pFormCell );
                        }
                    break;
                    case svDoubleRef:
                        t->CalcAbsIfRel( pFormCell->aPos );
                        if ( rRef1.Valid() && rRef2.Valid() )
                        {
                            if ( t->GetOpCode() == ocColRowNameAuto )
                            {	// automagically
                                if ( rRef1.IsColRel() )
                                {	// ColName
                                    pDoc->StartListeningArea( ScRange (
                                        0,
                                        rRef1.nRow,
                                        rRef1.nTab,
                                        MAXCOL,
                                        rRef2.nRow,
                                        rRef2.nTab ), pFormCell );
                                }
                                else
                                {	// RowName
                                    pDoc->StartListeningArea( ScRange (
                                        rRef1.nCol,
                                        0,
                                        rRef1.nTab,
                                        rRef2.nCol,
                                        MAXROW,
                                        rRef2.nTab ), pFormCell );
                                }
                            }
                            else
                            {
                                pDoc->StartListeningArea( ScRange (
                                    rRef1.nCol,
                                    rRef1.nRow,
                                    rRef1.nTab,
                                    rRef2.nCol,
                                    rRef2.nRow,
                                    rRef2.nTab ), pFormCell );
                            }
                        }
                    break;
                    default:
                        ;   // nothing
                }
			}
		}
        pFormCell->SetNeedsListening( FALSE);
	}
}

//	pArr gesetzt -> Referenzen von anderer Zelle nehmen
// dann muss auch aPos uebergeben werden!

void ScBaseCell::EndListeningTo( ScDocument* pDoc, ScTokenArray* pArr,
        ScAddress aPos )
{
	if ( eCellType == CELLTYPE_FORMULA && !pDoc->IsClipOrUndo()
			&& !((ScFormulaCell*)this)->IsInChangeTrack()
		)
	{
		pDoc->SetDetectiveDirty(TRUE);	// es hat sich was geaendert...

		ScFormulaCell* pFormCell = (ScFormulaCell*)this;
		if( pFormCell->GetCode()->IsRecalcModeAlways() )
			pDoc->EndListeningArea( BCA_LISTEN_ALWAYS, pFormCell );
		else
		{
			if (!pArr)
			{
				pArr = pFormCell->GetCode();
				aPos = pFormCell->aPos;
			}
			pArr->Reset();
			for( ScToken* t = pArr->GetNextReferenceRPN(); t;
						  t = pArr->GetNextReferenceRPN() )
			{
				StackVar eType = t->GetType();
				SingleRefData& rRef1 = t->GetSingleRef();
				SingleRefData& rRef2 = (eType == svDoubleRef ?
					t->GetDoubleRef().Ref2 : rRef1);
                switch( eType )
                {
                    case svSingleRef:
                        rRef1.CalcAbsIfRel( aPos );
                        if ( rRef1.Valid() )
                        {
                            pDoc->EndListeningCell(
                                ScAddress( rRef1.nCol,
                                           rRef1.nRow,
                                           rRef1.nTab ), pFormCell );
                        }
                    break;
                    case svDoubleRef:
                        t->CalcAbsIfRel( aPos );
                        if ( rRef1.Valid() && rRef2.Valid() )
                        {
                            if ( t->GetOpCode() == ocColRowNameAuto )
                            {	// automagically
                                if ( rRef1.IsColRel() )
                                {	// ColName
                                    pDoc->EndListeningArea( ScRange (
                                        0,
                                        rRef1.nRow,
                                        rRef1.nTab,
                                        MAXCOL,
                                        rRef2.nRow,
                                        rRef2.nTab ), pFormCell );
                                }
                                else
                                {	// RowName
                                    pDoc->EndListeningArea( ScRange (
                                        rRef1.nCol,
                                        0,
                                        rRef1.nTab,
                                        rRef2.nCol,
                                        MAXROW,
                                        rRef2.nTab ), pFormCell );
                                }
                            }
                            else
                            {
                                pDoc->EndListeningArea( ScRange (
                                    rRef1.nCol,
                                    rRef1.nRow,
                                    rRef1.nTab,
                                    rRef2.nCol,
                                    rRef2.nRow,
                                    rRef2.nTab ), pFormCell );
                            }
                        }
                    break;
                    default:
                        ;   // nothing
                }
			}
		}
	}
}


BOOL ScBaseCell::HasValueData() const
{
	switch ( eCellType )
	{
		case CELLTYPE_VALUE :
			return TRUE;
		case CELLTYPE_FORMULA :
			return ((ScFormulaCell*)this)->IsValue();
		default:
			return FALSE;
	}
}


BOOL ScBaseCell::HasStringData() const
{
	switch ( eCellType )
	{
		case CELLTYPE_STRING :
		case CELLTYPE_EDIT :
			return TRUE;
		case CELLTYPE_FORMULA :
			return !((ScFormulaCell*)this)->IsValue();
		default:
			return FALSE;
	}
}

String ScBaseCell::GetStringData() const
{
	String aStr;
	switch ( eCellType )
	{
		case CELLTYPE_STRING:
			((const ScStringCell*)this)->GetString( aStr );
			break;
		case CELLTYPE_EDIT:
			((const ScEditCell*)this)->GetString( aStr );
			break;
		case CELLTYPE_FORMULA:
			((ScFormulaCell*)this)->GetString( aStr );		// an der Formelzelle nicht-const
			break;
	}
	return aStr;
}

//	static
BOOL ScBaseCell::CellEqual( const ScBaseCell* pCell1, const ScBaseCell* pCell2 )
{
	CellType eType1 = CELLTYPE_NONE;
	CellType eType2 = CELLTYPE_NONE;
	if ( pCell1 )
	{
		eType1 = pCell1->GetCellType();
		if (eType1 == CELLTYPE_EDIT)
			eType1 = CELLTYPE_STRING;
		else if (eType1 == CELLTYPE_NOTE)
			eType1 = CELLTYPE_NONE;
	}
	if ( pCell2 )
	{
		eType2 = pCell2->GetCellType();
		if (eType2 == CELLTYPE_EDIT)
			eType2 = CELLTYPE_STRING;
		else if (eType2 == CELLTYPE_NOTE)
			eType2 = CELLTYPE_NONE;
	}
	if ( eType1 != eType2 )
		return FALSE;

	switch ( eType1 )				// beide Typen gleich
	{
		case CELLTYPE_NONE:			// beide leer
			return TRUE;
		case CELLTYPE_VALUE:		// wirklich Value-Zellen
			return ( ((const ScValueCell*)pCell1)->GetValue() ==
					 ((const ScValueCell*)pCell2)->GetValue() );
		case CELLTYPE_STRING:		// String oder Edit
			{
				String aText1;
				if ( pCell1->GetCellType() == CELLTYPE_STRING )
					((const ScStringCell*)pCell1)->GetString(aText1);
				else
					((const ScEditCell*)pCell1)->GetString(aText1);
				String aText2;
				if ( pCell2->GetCellType() == CELLTYPE_STRING )
					((const ScStringCell*)pCell2)->GetString(aText2);
				else
					((const ScEditCell*)pCell2)->GetString(aText2);
				return ( aText1 == aText2 );
			}
		case CELLTYPE_FORMULA:
			{
				//!	eingefuegte Zeilen / Spalten beruecksichtigen !!!!!
				//!	Vergleichsfunktion an der Formelzelle ???
				//!	Abfrage mit ScColumn::SwapRow zusammenfassen!

				ScTokenArray* pCode1 = ((ScFormulaCell*)pCell1)->GetCode();
				ScTokenArray* pCode2 = ((ScFormulaCell*)pCell2)->GetCode();

				if (pCode1->GetLen() == pCode2->GetLen())		// nicht-UPN
				{
					BOOL bEqual = TRUE;
					USHORT nLen = pCode1->GetLen();
					ScToken** ppToken1 = pCode1->GetArray();
					ScToken** ppToken2 = pCode2->GetArray();
					for (USHORT i=0; i<nLen; i++)
						if ( !ppToken1[i]->TextEqual(*(ppToken2[i])) )
						{
							bEqual = FALSE;
							break;
						}

					if (bEqual)
						return TRUE;
				}

				return FALSE;		// unterschiedlich lang oder unterschiedliche Tokens
			}
		default:
			DBG_ERROR("huch, was fuer Zellen???");
	}
	return FALSE;
}

//-----------------------------------------------------------------------------------

//
//		ScFormulaCell
//

ScFormulaCell::ScFormulaCell() :
	ScBaseCell( CELLTYPE_FORMULA ),
	nErgValue( 0.0 ),
	pCode( NULL ),
	pDocument( NULL ),
	pMatrix( NULL ),
	pPrevious(0),
	pNext(0),
	pPreviousTrack(0),
	pNextTrack(0),
	nFormatIndex(0),
	nMatCols(0),
	nMatRows(0),
    nSeenInIteration(0),
	nFormatType( NUMBERFORMAT_NUMBER ),
	bIsValue( TRUE ),
	bDirty( FALSE ),
	bChanged( FALSE ),
	bRunning( FALSE ),
	bCompile( FALSE ),
	bSubTotal( FALSE ),
	bIsIterCell( FALSE ),
	bInChangeTrack( FALSE ),
	bTableOpDirty( FALSE ),
	bNeedListening( FALSE ),
	cMatrixFlag ( MM_NONE ),
	aPos(0,0,0)
{
}

ScFormulaCell::ScFormulaCell( ScDocument* pDoc, const ScAddress& rPos,
							  const String& rFormula, BYTE cMatInd ) :
	ScBaseCell( CELLTYPE_FORMULA ),
	nErgValue( 0.0 ),
	pCode( NULL ),
	pDocument( pDoc ),
	pMatrix( NULL ),
	pPrevious(0),
	pNext(0),
	pPreviousTrack(0),
	pNextTrack(0),
	nFormatIndex(0),
	nMatCols(0),
	nMatRows(0),
    nSeenInIteration(0),
	nFormatType( NUMBERFORMAT_NUMBER ),
	bIsValue( TRUE ),
	bDirty( TRUE ), // -> wg. Benutzung im Fkt.AutoPiloten, war: cMatInd != 0
	bChanged( FALSE ),
	bRunning( FALSE ),
	bCompile( FALSE ),
	bSubTotal( FALSE ),
	bIsIterCell( FALSE ),
	bInChangeTrack( FALSE ),
	bTableOpDirty( FALSE ),
	bNeedListening( FALSE ),
	cMatrixFlag ( cMatInd ),
	aPos( rPos )
{
	Compile( rFormula, TRUE );	// bNoListening, erledigt Insert
}

// Wird von den Importfiltern verwendet

ScFormulaCell::ScFormulaCell( ScDocument* pDoc, const ScAddress& rPos,
							  const ScTokenArray* pArr, BYTE cInd ) :
	ScBaseCell( CELLTYPE_FORMULA ),
	nErgValue( 0.0 ),
	pCode( pArr ? new ScTokenArray( *pArr ) : new ScTokenArray ),
	pDocument( pDoc ),
	pMatrix ( NULL ),
	pPrevious(0),
	pNext(0),
	pPreviousTrack(0),
	pNextTrack(0),
	nFormatIndex(0),
	nMatCols(0),
	nMatRows(0),
    nSeenInIteration(0),
	nFormatType( NUMBERFORMAT_NUMBER ),
	bIsValue( TRUE ),
	bDirty( NULL != pArr ), // -> wg. Benutzung im Fkt.AutoPiloten, war: cInd != 0
	bChanged( FALSE ),
	bRunning( FALSE ),
	bCompile( FALSE ),
	bSubTotal( FALSE ),
	bIsIterCell( FALSE ),
	bInChangeTrack( FALSE ),
	bTableOpDirty( FALSE ),
	bNeedListening( FALSE ),
	cMatrixFlag ( cInd ),
	aPos( rPos )
{
	// UPN-Array erzeugen
	if( pCode->GetLen() && !pCode->GetError() && !pCode->GetCodeLen() )
	{
		ScCompiler aComp(pDocument, aPos, *pCode);
		bSubTotal = aComp.CompileTokenArray();
		nFormatType = aComp.GetNumFormatType();
	}
	else
	{
		pCode->Reset();
		if ( pCode->GetNextOpCodeRPN( ocSubTotal ) )
			bSubTotal = TRUE;
	}
}

ScFormulaCell::ScFormulaCell( ScDocument* pDoc, const ScAddress& rNewPos,
                              const ScFormulaCell& rScFormulaCell, USHORT nCopyFlags ) :
	ScBaseCell( rScFormulaCell, pDoc ),
	SvtListener(),
	aErgString( rScFormulaCell.aErgString ),
	nErgValue( rScFormulaCell.nErgValue ),
	pDocument( pDoc ),
	pPrevious(0),
	pNext(0),
	pPreviousTrack(0),
	pNextTrack(0),
	nFormatIndex( pDoc == rScFormulaCell.pDocument ? rScFormulaCell.nFormatIndex : 0 ),
	nMatCols( rScFormulaCell.nMatCols ),
	nMatRows( rScFormulaCell.nMatRows ),
    nSeenInIteration(0),
	nFormatType( rScFormulaCell.nFormatType ),
	bIsValue( rScFormulaCell.bIsValue ),
	bDirty( rScFormulaCell.bDirty ),
	bChanged( rScFormulaCell.bChanged ),
	bRunning( FALSE ),
	bCompile( rScFormulaCell.bCompile ),
	bSubTotal( rScFormulaCell.bSubTotal ),
	bIsIterCell( FALSE ),
	bInChangeTrack( FALSE ),
	bTableOpDirty( FALSE ),
	bNeedListening( FALSE ),
	cMatrixFlag ( rScFormulaCell.cMatrixFlag ),
	aPos( rNewPos )
{
	if (rScFormulaCell.pMatrix)
    {
		pMatrix = rScFormulaCell.pMatrix->Clone();
        pMatrix->SetEternalRef();
    }
	else
		pMatrix = NULL;
	pCode = rScFormulaCell.pCode->Clone();

    if ( nCopyFlags & 0x0001 )
        pCode->ReadjustRelative3DReferences( rScFormulaCell.aPos, aPos );

	// evtl. Fehler zuruecksetzen und neu kompilieren
	//	nicht im Clipboard - da muss das Fehlerflag erhalten bleiben
	//	Spezialfall Laenge=0: als Fehlerzelle erzeugt, dann auch Fehler behalten
	if ( pCode->GetError() && !pDocument->IsClipboard() && pCode->GetLen() )
	{
		pCode->SetError( 0 );
		bCompile = TRUE;
	}
    //! Compile ColRowNames on URM_MOVE/URM_COPY _after_ UpdateReference
    BOOL bCompileLater = FALSE;
    BOOL bClipMode = rScFormulaCell.pDocument->IsClipboard();
	if( !bCompile )
    {   // Name references with references and ColRowNames
        pCode->Reset();
		for( ScToken* t = pCode->GetNextReferenceOrName(); t && !bCompile;
					  t = pCode->GetNextReferenceOrName() )
		{
			if ( t->GetType() == svIndex )
			{
				ScRangeData* pRangeData = pDoc->GetRangeName()->FindIndex( t->GetIndex() );
				if( pRangeData )
				{
					if( pRangeData->HasReferences() )
						bCompile = TRUE;
				}
				else
                    bCompile = TRUE;    // invalid reference!
			}
			else if ( t->GetOpCode() == ocColRowName )
			{
                bCompile = TRUE;        // new lookup needed
                bCompileLater = bClipMode;
			}
		}
	}
	if( bCompile )
	{
        if ( !bCompileLater && bClipMode )
		{
			pCode->Reset();
			bCompileLater = (pCode->GetNextColRowName() != NULL);
		}
        if ( !bCompileLater )
        {
            // bNoListening, bei in Clip/Undo sowieso nicht,
            // bei aus Clip auch nicht, sondern nach Insert(Clone) und UpdateReference
            CompileTokenArray( TRUE );
        }
	}
}

// +---+---+---+---+---+---+---+---+
// |           |Str|Num|Dir|cMatrix|
// +---+---+---+---+---+---+---+---+

ScFormulaCell::ScFormulaCell( ScDocument* pDoc, const ScAddress& rPos,
							  SvStream& rStream, ScMultipleReadHeader& rHdr ) :
	ScBaseCell( CELLTYPE_FORMULA ),
	nErgValue( 0.0 ),
	pCode( new ScTokenArray ),
	pDocument( pDoc ),
	pMatrix ( NULL ),
	pPrevious(0),
	pNext(0),
	pPreviousTrack(0),
	pNextTrack(0),
	nFormatIndex(0),
	nMatCols(0),
	nMatRows(0),
    nSeenInIteration(0),
	nFormatType( 0 ),
	bIsValue( TRUE ),
	bDirty( FALSE ),
	bChanged( FALSE ),
	bRunning( FALSE ),
	bCompile( FALSE ),
	bSubTotal( FALSE ),
	bIsIterCell( FALSE ),
	bInChangeTrack( FALSE ),
	bTableOpDirty( FALSE ),
	bNeedListening( FALSE ),
	aPos( rPos )
{
//	ScReadHeader aHdr( rStream );
	rHdr.StartEntry();

	USHORT nVer = (USHORT) pDoc->GetSrcVersion();

	if( nVer >= SC_NUMFMT )
	{
		BYTE cData;
		rStream >> cData;
#ifndef PRODUCT
//		static BOOL bShown = 0;
//		if ( !bShown && SOFFICE_FILEFORMAT_NOW > SOFFICE_FILEFORMAT_50 )
//		{
//			bShown = 1;
//			DBG_ERRORFILE( "bei inkompatiblem FileFormat den FormatIndex umheben!" );
//		}
#endif
		if( cData & 0x0F )
		{
			BYTE nSkip = cData & 0x0F;
			if ( (cData & 0x10) && nSkip >= sizeof(UINT32) )
			{
				UINT32 n;
				rStream >> n;
				nFormatIndex = n;
				nSkip -= sizeof(UINT32);
			}
			if ( nSkip )
				rStream.SeekRel( nSkip );
		}
		BYTE cFlags;
		rStream >> cFlags >> nFormatType;
		cMatrixFlag = (BYTE) ( cFlags & 0x03 );
		bDirty = BOOL( ( cFlags & 0x04 ) != 0 );
		if( cFlags & 0x08 )
			rStream >> nErgValue;
		if( cFlags & 0x10 )
		{
			rStream.ReadByteString( aErgString, rStream.GetStreamCharSet() );
			bIsValue = FALSE;
		}
		pCode->Load( rStream, nVer, aPos );
		if ( (cFlags & 0x18) == 0 )
			bDirty = TRUE;		// #67161# no result stored => recalc
		if( cFlags & 0x20 )
			bSubTotal = TRUE;
		else if ( nVer < SC_SUBTOTAL_BUGFIX )
		{	// #65285# in alten Dokumenten war Flag nicht gesetzt, wenn Formel
			// manuell eingegeben wurde (nicht via Daten->Teilergebnisse)
			if ( pCode->HasOpCodeRPN( ocSubTotal ) )
			{
				bDirty = TRUE;		// neu berechnen
				bSubTotal = TRUE;
			}
		}
#if SC_ROWLIMIT_STREAM_ACCESS 
#error address types changed!
		if ( cMatrixFlag == MM_FORMULA && rHdr.BytesLeft() )
			rStream >> nMatCols >> nMatRows;
#endif
	}
	else
	{
		UINT16 nCodeLen;
		if( pDoc->GetSrcVersion() >= SC_FORMULA_LCLVER )
			rStream.SeekRel( 2 );
		rStream >> cMatrixFlag >> nCodeLen;
		if( cMatrixFlag == 5 )
			cMatrixFlag = 0;
		cMatrixFlag &= 3;
		if( nCodeLen )
			pCode->Load30( rStream, aPos );
		// Wir koennen hier bei Calc 3.0-Docs noch kein UPN-Array
		// erzeugen, da die Named Ranges noch nicht eingelesen sind
	}

	rHdr.EndEntry();

	//	after loading, it must be known if ocMacro is in any formula
	//	(for macro warning, and to insert the hidden view)
	if ( !pDoc->GetHasMacroFunc() && pCode->HasOpCodeRPN( ocMacro ) )
		pDoc->SetHasMacroFunc( TRUE );
}

BOOL lcl_IsBeyond( ScTokenArray* pCode, SCROW nMaxRow )
{
	ScToken* t;
	pCode->Reset();
	while ( t = pCode->GetNextReferenceRPN() )	// RPN -> auch in Namen
		if ( t->GetSingleRef().nRow > nMaxRow ||
				(t->GetType() == svDoubleRef &&
				t->GetDoubleRef().Ref2.nRow > nMaxRow) )
			return TRUE;
	return FALSE;
}

void ScFormulaCell::Save( SvStream& rStream, ScMultipleWriteHeader& rHdr ) const
{
	SCROW nSaveMaxRow = pDocument->GetSrcMaxRow();
	if ( nSaveMaxRow < MAXROW && lcl_IsBeyond( pCode, nSaveMaxRow ) )
	{
		//	Zelle mit Ref-Error erzeugen und speichern
		//	StartEntry/EndEntry passiert beim Speichern der neuen Zelle

		SingleRefData aRef;
		aRef.InitAddress(ScAddress());
		aRef.SetColRel(TRUE);
		aRef.SetColDeleted(TRUE);
		aRef.SetRowRel(TRUE);
		aRef.SetRowDeleted(TRUE);
		aRef.CalcRelFromAbs(aPos);
		ScTokenArray aArr;
		aArr.AddSingleReference(aRef);
		aArr.AddOpCode(ocStop);
		ScFormulaCell* pErrCell = new ScFormulaCell( pDocument, aPos, &aArr );
		pErrCell->Save( rStream, rHdr );
		delete pErrCell;

		pDocument->SetLostData();			// Warnung ausgeben
		return;
	}

	rHdr.StartEntry();

	if ( bIsValue && !pCode->GetError() && !::rtl::math::isFinite( nErgValue ) )
	{
		DBG_ERRORFILE( msgDbgInfinity );
		pCode->SetError( errIllegalFPOperation );
	}
	BYTE cFlags = cMatrixFlag & 0x03;
	if( bDirty )
		cFlags |= 0x04;
	// Daten speichern?
	if( pCode->IsRecalcModeNormal() && !pCode->GetError() )
		cFlags |= bIsValue ? 0x08 : 0x10;
	if ( bSubTotal )
		cFlags |= 0x20;
#ifndef PRODUCT
	static BOOL bShown = 0;
	if ( !bShown && rStream.GetVersion() > SOFFICE_FILEFORMAT_50 )
	{
		bShown = 1;
		DBG_ERRORFILE( "bei inkompatiblem FileFormat den FormatIndex umheben!" );
	}
//	rStream << (BYTE) 0x00;
#endif
#if SC_ROWLIMIT_STREAM_ACCESS 
#error address types changed!
	if ( nFormatIndex )
		rStream << (BYTE) (0x10 | sizeof(UINT32)) << nFormatIndex;
	else
		rStream << (BYTE) 0x00;
	rStream << cFlags << (UINT16) nFormatType;
	if( cFlags & 0x08 )
		rStream << nErgValue;
	if( cFlags & 0x10 )
		rStream.WriteByteString( aErgString, rStream.GetStreamCharSet() );
	pCode->Store( rStream, aPos );
	if ( cMatrixFlag == MM_FORMULA )
		rStream << nMatCols << nMatRows;

#endif
	rHdr.EndEntry();
}

ScBaseCell* ScFormulaCell::Clone( ScDocument* pDoc, const ScAddress& rPos,
		BOOL bNoListening ) const
{
	ScFormulaCell* pCell = new ScFormulaCell( pDoc, rPos, *this );
	if ( !bNoListening )
		pCell->StartListeningTo( pDoc );
	return pCell;
}

void ScFormulaCell::GetFormula( String& rFormula ) const
{
	if( pCode->GetError() && !pCode->GetLen() )
	{
		rFormula = ScGlobal::GetErrorString( pCode->GetError() ); return;
	}
	else if( cMatrixFlag == MM_REFERENCE )
	{
		// Referenz auf eine andere Zelle, die eine Matrixformel enthaelt
		pCode->Reset();
		ScToken* p = pCode->GetNextReferenceRPN();
		if( p )
		{
			ScBaseCell* pCell = NULL;
			if ( !IsInChangeTrack() )
			{
				SingleRefData& rRef = p->GetSingleRef();
				rRef.CalcAbsIfRel( aPos );
				if ( rRef.Valid() )
					pCell = pDocument->GetCell( ScAddress( rRef.nCol,
						rRef.nRow, rRef.nTab ) );
			}
			if (pCell && pCell->GetCellType() == CELLTYPE_FORMULA)
			{
				((ScFormulaCell*)pCell)->GetFormula(rFormula);
				return;
			}
			else
			{
				ScCompiler aComp( pDocument, aPos, *pCode );
				aComp.CreateStringFromTokenArray( rFormula );
			}
		}
		else
		{
			DBG_ERROR("ScFormulaCell::GetFormula: Keine Matrix");
		}
	}
	else
	{
		ScCompiler aComp( pDocument, aPos, *pCode );
		aComp.CreateStringFromTokenArray( rFormula );
	}

	rFormula.Insert( '=',0 );
	if( cMatrixFlag )
	{
		rFormula.Insert('{', 0);
		rFormula += '}';
	}
}

void ScFormulaCell::GetResultDimensions( SCSIZE& rCols, SCSIZE& rRows )
{
	if (IsDirtyOrInTableOpDirty() && pDocument->GetAutoCalc())
		Interpret();

	if ( !pCode->GetError() && pMatrix )
		pMatrix->GetDimensions( rCols, rRows );
	else
	{
		rCols = 0;
		rRows = 0;
	}
}

void ScFormulaCell::Compile( const String& rFormula, BOOL bNoListening )
{
	if ( pDocument->IsClipOrUndo() ) return;
	BOOL bWasInFormulaTree = pDocument->IsInFormulaTree( this );
	if ( bWasInFormulaTree )
		pDocument->RemoveFromFormulaTree( this );
	// pCode darf fuer Abfragen noch nicht geloescht, muss aber leer sein
	if ( pCode )
		pCode->Clear();
	ScTokenArray* pCodeOld = pCode;
	ScCompiler aComp(pDocument, aPos);
	if ( pDocument->IsImportingXML() )
		aComp.SetCompileEnglish( TRUE );
	pCode = aComp.CompileString( rFormula );
	if ( pCodeOld )
		delete pCodeOld;
	if( !pCode->GetError() )
	{
		if ( !pCode->GetLen() && aErgString.Len() && rFormula == aErgString )
		{	// #65994# nicht rekursiv CompileTokenArray/Compile/CompileTokenArray
			if ( rFormula.GetChar(0) == '=' )
				pCode->AddBad( rFormula.GetBuffer() + 1 );
			else
				pCode->AddBad( rFormula.GetBuffer() );
		}
		bCompile = TRUE;
		CompileTokenArray( bNoListening );
	}
	else
	{
		bChanged = TRUE;
		SetTextWidth( TEXTWIDTH_DIRTY );
		SetScriptType( SC_SCRIPTTYPE_UNKNOWN );
	}
	if ( bWasInFormulaTree )
		pDocument->PutInFormulaTree( this );
}


void ScFormulaCell::CompileTokenArray( BOOL bNoListening )
{
	// Noch nicht compiliert?
	if( !pCode->GetLen() && aErgString.Len() )
		Compile( aErgString );
	else if( bCompile && !pDocument->IsClipOrUndo() && !pCode->GetError() )
	{
		// RPN-Laenge kann sich aendern
		BOOL bWasInFormulaTree = pDocument->IsInFormulaTree( this );
		if ( bWasInFormulaTree )
			pDocument->RemoveFromFormulaTree( this );

		// Laden aus Filter? Dann noch nix machen!
		if( pDocument->IsInsertingFromOtherDoc() )
			bNoListening = TRUE;

		if( !bNoListening && pCode->GetCodeLen() )
			EndListeningTo( pDocument );
		ScCompiler aComp(pDocument, aPos, *pCode );
		bSubTotal = aComp.CompileTokenArray();
		if( !pCode->GetError() )
		{
			nFormatType = aComp.GetNumFormatType();
			nFormatIndex = 0;
			bChanged = TRUE;
			nErgValue = 0.0;
			aErgString.Erase();
			bCompile = FALSE;
			if ( !bNoListening )
				StartListeningTo( pDocument );
		}
		if ( bWasInFormulaTree )
			pDocument->PutInFormulaTree( this );
	}
}


void ScFormulaCell::CompileXML( ScProgress& rProgress )
{
	if ( cMatrixFlag == MM_REFERENCE )
	{	// is already token code via ScDocFunc::EnterMatrix, ScDocument::InsertMatrixFormula
		// just establish listeners
		StartListeningTo( pDocument );
		return ;
	}

	ScCompiler aComp( pDocument, aPos, *pCode );
	aComp.SetCompileEnglish( TRUE );
	aComp.SetImportXML( TRUE );
	String aFormula;
	aComp.CreateStringFromTokenArray( aFormula );
    pDocument->DecXMLImportedFormulaCount( aFormula.Len() );
    rProgress.SetStateCountDownOnPercent( pDocument->GetXMLImportedFormulaCount() );
	// pCode darf fuer Abfragen noch nicht geloescht, muss aber leer sein
	if ( pCode )
		pCode->Clear();
	ScTokenArray* pCodeOld = pCode;
	pCode = aComp.CompileString( aFormula );
	delete pCodeOld;
	if( !pCode->GetError() )
	{
		if ( !pCode->GetLen() )
		{
			if ( aFormula.GetChar(0) == '=' )
				pCode->AddBad( aFormula.GetBuffer() + 1 );
			else
				pCode->AddBad( aFormula.GetBuffer() );
		}
		bSubTotal = aComp.CompileTokenArray();
		if( !pCode->GetError() )
		{
			nFormatType = aComp.GetNumFormatType();
			nFormatIndex = 0;
			bChanged = TRUE;
			bCompile = FALSE;
			StartListeningTo( pDocument );
		}
	}
	else
	{
		bChanged = TRUE;
		SetTextWidth( TEXTWIDTH_DIRTY );
		SetScriptType( SC_SCRIPTTYPE_UNKNOWN );
	}

	//	Same as in Load: after loading, it must be known if ocMacro is in any formula
	//	(for macro warning, CompileXML is called at the end of loading XML file)
	if ( !pDocument->GetHasMacroFunc() && pCode->HasOpCodeRPN( ocMacro ) )
		pDocument->SetHasMacroFunc( TRUE );
}


void ScFormulaCell::CalcAfterLoad()
{
	BOOL bNewCompiled = FALSE;
	// Falls ein Calc 1.0-Doc eingelesen wird, haben wir ein Ergebnis,
	// aber kein TokenArray
	if( !pCode->GetLen() && aErgString.Len() )
	{
		Compile( aErgString, TRUE );
		aErgString.Erase();
		bDirty = TRUE;
		bNewCompiled = TRUE;
	}
	// Das UPN-Array wird nicht erzeugt, wenn ein Calc 3.0-Doc eingelesen
	// wurde, da die RangeNames erst jetzt existieren.
	if( pCode->GetLen() && !pCode->GetCodeLen() && !pCode->GetError() )
	{
		ScCompiler aComp(pDocument, aPos, *pCode);
		bSubTotal = aComp.CompileTokenArray();
		nFormatType = aComp.GetNumFormatType();
		nFormatIndex = 0;
		bDirty = TRUE;
		bCompile = FALSE;
		bNewCompiled = TRUE;
	}
	// irgendwie koennen unter os/2 mit rotter FPU-Exception /0 ohne Err503
	// gespeichert werden, woraufhin spaeter im NumberFormatter die BLC Lib
	// bei einem fabs(-NAN) abstuerzt (#32739#)
	// hier fuer alle Systeme ausbuegeln, damit da auch Err503 steht
	if ( bIsValue && !::rtl::math::isFinite( nErgValue ) )
	{
		DBG_ERRORFILE("Formelzelle INFINITY !!! Woher kommt das Dokument?");
		nErgValue = 0.0;
		pCode->SetError( errIllegalFPOperation );
		bDirty = TRUE;
	}
	// DoubleRefs bei binaeren Operatoren waren vor v5.0 immer Matrix,
	// jetzt nur noch wenn in Matrixformel, sonst implizite Schnittmenge
	if ( pDocument->GetSrcVersion() < SC_MATRIX_DOUBLEREF &&
			GetMatrixFlag() == MM_NONE && pCode->HasMatrixDoubleRefOps() )
	{
		cMatrixFlag = MM_FORMULA;
		nMatCols = 1;
		nMatRows = 1;
	}
	// Muss die Zelle berechnet werden?
	// Nach Load koennen Zellen einen Fehlercode enthalten, auch dann
	// Listener starten und ggbf. neu berechnen wenn nicht RECALCMODE_NORMAL
	if( !bNewCompiled || !pCode->GetError() )
	{
		StartListeningTo( pDocument );
		if( !pCode->IsRecalcModeNormal() )
			bDirty = TRUE;
	}
	if ( pCode->IsRecalcModeAlways() )
	{	// zufall(), heute(), jetzt() bleiben immer im FormulaTree, damit sie
		// auch bei jedem F9 berechnet werden.
		bDirty = TRUE;
	}
	// Noch kein SetDirty weil noch nicht alle Listener bekannt, erst in
	// SetDirtyAfterLoad.
}


// FIXME: set to 0
#define erDEBUGDOT 0
// If set to 1, write output that's suitable for graphviz tools like dot.
// Only node1 -> node2 entries are written, you'll have to manually surround
// the file content with [strict] digraph name { ... }
// The ``strict'' keyword might be necessary in case of multiple identical
// paths like they occur in iterations, otherwise dot may consume too much
// memory when generating the layout, or you'll get unreadable output. On the
// other hand, information about recurring calculation is lost then.
// Generates output only if variable nDebug is set in debugger, see below.
// FIXME: currently doesn't cope with iterations and recursions. Code fragments
// are a leftover from a previous debug session, meant as a pointer.
#if erDEBUGDOT
#include <cstdio>
using ::std::fopen;
using ::std::fprintf;
#include <vector>
static const char aDebugDotFile[] = "ttt_debug.dot";
#endif

void ScFormulaCell::Interpret()
{

#if erDEBUGDOT
    static int nDebug = 0;
    static const int erDEBUGDOTRUN = 3;
    static FILE* pDebugFile = 0;
    static sal_Int32 nDebugRootCount = 0;
    static unsigned int nDebugPathCount = 0;
    static ScAddress aDebugLastPos( ScAddress::INITIALIZE_INVALID);
    static ScAddress aDebugThisPos( ScAddress::INITIALIZE_INVALID);
    typedef ::std::vector< ByteString > DebugVector;
    static DebugVector aDebugVec;
    class DebugElement
    {
        public:
            static void push( ScFormulaCell* pCell )
            {
                aDebugThisPos = pCell->aPos;
                if (aDebugVec.empty())
                {
                    ByteString aR( "root_");
                    aR += ByteString::CreateFromInt32( ++nDebugRootCount);
                    aDebugVec.push_back( aR);
                }
                String aStr;
                pCell->aPos.Format( aStr, SCA_VALID | SCA_TAB_3D, pCell->GetDocument());
                ByteString aB( aStr, RTL_TEXTENCODING_UTF8);
                aDebugVec.push_back( aB);
            }
            static void pop()
            {
                aDebugLastPos = aDebugThisPos;
                if (!aDebugVec.empty())
                {
                    aDebugVec.pop_back();
                    if (aDebugVec.size() == 1)
                    {
                        aDebugVec.pop_back();
                        aDebugLastPos = ScAddress( ScAddress::INITIALIZE_INVALID);
                    }
                }
            }
            DebugElement( ScFormulaCell* p ) { push(p); }
            ~DebugElement() { pop(); }
    };
    class DebugDot
    {
        public:
            static void out( const char* pColor )
            {
                if (nDebug != erDEBUGDOTRUN)
                    return;
                char pColorString[256];
                sprintf( pColorString, (*pColor ?
                            ",color=\"%s\",fontcolor=\"%s\"" : "%s%s"), pColor,
                        pColor);
                size_t n = aDebugVec.size();
                fprintf( pDebugFile,
                        "\"%s\" -> \"%s\" [label=\"%u\"%s];  // v:%d\n",
                        aDebugVec[n-2].GetBuffer(), aDebugVec[n-1].GetBuffer(),
                        ++nDebugPathCount, pColorString, n-1);
                fflush( pDebugFile);
            }
    };
    #define erDEBUGDOT_OUT( p )             (DebugDot::out(p))
    #define erDEBUGDOT_ELEMENT_PUSH( p )    (DebugElement::push(p))
    #define erDEBUGDOT_ELEMENT_POP()        (DebugElement::pop())
#else
    #define erDEBUGDOT_OUT( p )
    #define erDEBUGDOT_ELEMENT_PUSH( p )
    #define erDEBUGDOT_ELEMENT_POP()
#endif

	if (!IsDirtyOrInTableOpDirty() || pDocument->GetRecursionHelper().IsInReturn())
        return;     // no double/triple processing

	//!	HACK:
	//	Wenn der Aufruf aus einem Reschedule im DdeLink-Update kommt, dirty stehenlassen
	//	Besser: Dde-Link Update ohne Reschedule oder ganz asynchron !!!

    if ( pDocument->IsInDdeLinkUpdate() )
		return;

#if erDEBUGDOT
    // set nDebug=1 in debugger to init things
    if (nDebug == 1)
    {
        ++nDebug;
        pDebugFile = fopen( aDebugDotFile, "a");
        if (!pDebugFile)
            nDebug = 0;
        else
            nDebug = erDEBUGDOTRUN;
    }
    // set nDebug=3 (erDEBUGDOTRUN) in debugger to get any output
    DebugElement aDebugElem( this);
    // set nDebug=5 in debugger to close output
    if (nDebug == 5)
    {
        nDebug = 0;
        fclose( pDebugFile);
        pDebugFile = 0;
    }
#endif

	if (bRunning)
	{

#if erDEBUGDOT
        if (!pDocument->GetRecursionHelper().IsDoingIteration() ||
                aDebugThisPos != aDebugLastPos)
            erDEBUGDOT_OUT(aDebugThisPos == aDebugLastPos ? "orange" :
                    (pDocument->GetRecursionHelper().GetIteration() ? "blue" :
                     "red"));
#endif

        if (!pDocument->GetDocOptions().IsIter())
        {
			pCode->SetError( errCircularReference );
            return;
        }

        if (pCode->GetError() == errCircularReference)
            pCode->SetError( 0 );

        // Start or add to iteration list.
        if (!pDocument->GetRecursionHelper().IsDoingIteration() ||
                !pDocument->GetRecursionHelper().GetRecursionInIterationStack().top()->bIsIterCell)
            pDocument->GetRecursionHelper().SetInIterationReturn( true);

        return;
	}
    // #63038# no multiple interprets for GetErrCode, IsValue, GetValue and
    // different entry point recursions. Would also lead to premature
    // convergence in iterations.
    if (pDocument->GetRecursionHelper().GetIteration() && nSeenInIteration ==
            pDocument->GetRecursionHelper().GetIteration())
		return ;

    erDEBUGDOT_OUT( pDocument->GetRecursionHelper().GetIteration() ? "magenta" : "");

    ScRecursionHelper& rRecursionHelper = pDocument->GetRecursionHelper();
    BOOL bOldRunning = bRunning;
    if (rRecursionHelper.GetRecursionCount() > MAXRECURSION)
    {
        bRunning = TRUE;
        rRecursionHelper.SetInRecursionReturn( true);
    }
    else
    {
        InterpretTail( SCITP_NORMAL);
    }

    // While leaving a recursion or iteration stack, insert its cells to the
    // recursion list in reverse order.
    if (rRecursionHelper.IsInReturn())
    {
        if (rRecursionHelper.GetRecursionCount() > 0 ||
                !rRecursionHelper.IsDoingRecursion())
            rRecursionHelper.Insert( this, bOldRunning, bIsValue, nErgValue,
                    aErgString);
        bool bIterationFromRecursion = false;
        bool bResumeIteration = false;
        do
        {
            if ((rRecursionHelper.IsInIterationReturn() &&
                        rRecursionHelper.GetRecursionCount() == 0 &&
                        !rRecursionHelper.IsDoingIteration()) ||
                    bIterationFromRecursion || bResumeIteration)
            {
                ScFormulaCell* pIterCell = this; // scope for debug convenience
                bool & rDone = rRecursionHelper.GetConvergingReference();
                rDone = false;
                if (!bIterationFromRecursion && bResumeIteration)
                {
                    bResumeIteration = false;
                    // Resuming iteration expands the range.
                    ScFormulaRecursionList::const_iterator aOldStart(
                            rRecursionHelper.GetLastIterationStart());
                    rRecursionHelper.ResumeIteration();
                    // Mark new cells being in iteration.
                    for (ScFormulaRecursionList::const_iterator aIter(
                                rRecursionHelper.GetIterationStart()); aIter !=
                            aOldStart; ++aIter)
                    {
                        pIterCell = (*aIter).pCell;
                        pIterCell->bIsIterCell = TRUE;
                    }
                    // Mark older cells dirty again, in case they converted
                    // without accounting for all remaining cells in the circle
                    // that weren't touched so far, e.g. conditional. Restore
                    // backuped result.
                    USHORT nIteration = rRecursionHelper.GetIteration();
                    for (ScFormulaRecursionList::const_iterator aIter(
                                aOldStart); aIter !=
                            rRecursionHelper.GetIterationEnd(); ++aIter)
                    {
                        pIterCell = (*aIter).pCell;
                        if (pIterCell->nSeenInIteration == nIteration)
                        {
                            if (!pIterCell->bDirty || aIter == aOldStart)
                            {
                                pIterCell->bIsValue =
                                    (*aIter).bPreviousNumeric;
                                pIterCell->nErgValue =
                                    (*aIter).fPreviousResult;
                                pIterCell->aErgString =
                                    (*aIter).aPreviousString;
                            }
                            --pIterCell->nSeenInIteration;
                        }
                        pIterCell->bDirty = TRUE;
                    }
                }
                else
                {
                    bResumeIteration = false;
                    // Close circle once.
                    rRecursionHelper.GetList().back().pCell->InterpretTail(
                            SCITP_CLOSE_ITERATION_CIRCLE);
                    // Start at 1, init things.
                    rRecursionHelper.StartIteration();
                    // Mark all cells being in iteration.
                    for (ScFormulaRecursionList::const_iterator aIter(
                                rRecursionHelper.GetIterationStart()); aIter !=
                            rRecursionHelper.GetIterationEnd(); ++aIter)
                    {
                        pIterCell = (*aIter).pCell;
                        pIterCell->bIsIterCell = TRUE;
                    }
                }
                bIterationFromRecursion = false;
                USHORT nIterMax = pDocument->GetDocOptions().GetIterCount();
                for ( ; rRecursionHelper.GetIteration() <= nIterMax && !rDone;
                        rRecursionHelper.IncIteration())
                {
                    rDone = true;
                    for ( ScFormulaRecursionList::iterator aIter(
                                rRecursionHelper.GetIterationStart()); aIter !=
                            rRecursionHelper.GetIterationEnd() &&
                            !rRecursionHelper.IsInReturn(); ++aIter)
                    {
                        pIterCell = (*aIter).pCell;
                        if (pIterCell->IsDirtyOrInTableOpDirty() &&
                                rRecursionHelper.GetIteration() !=
                                pIterCell->GetSeenInIteration())
                        {
                            (*aIter).bPreviousNumeric = pIterCell->bIsValue;
                            (*aIter).fPreviousResult = pIterCell->nErgValue;
                            (*aIter).aPreviousString = pIterCell->aErgString;
                            pIterCell->InterpretTail( SCITP_FROM_ITERATION);
                        }
                        rDone = rDone && !pIterCell->IsDirtyOrInTableOpDirty();
                    }
                    if (rRecursionHelper.IsInReturn())
                    {
                        bResumeIteration = true;
                        break;  // for
                        // Don't increment iteration.
                    }
                }
                if (!bResumeIteration)
                {
                    if (rDone)
                    {
                        for (ScFormulaRecursionList::const_iterator aIter(
                                    rRecursionHelper.GetIterationStart());
                                aIter != rRecursionHelper.GetIterationEnd();
                                ++aIter)
                        {
                            pIterCell = (*aIter).pCell;
                            pIterCell->bIsIterCell = FALSE;
                            pIterCell->nSeenInIteration = 0;
                            pIterCell->bRunning = (*aIter).bOldRunning;
                        }
                    }
                    else
                    {
                        for (ScFormulaRecursionList::const_iterator aIter(
                                    rRecursionHelper.GetIterationStart());
                                aIter != rRecursionHelper.GetIterationEnd();
                                ++aIter)
                        {
                            pIterCell = (*aIter).pCell;
                            pIterCell->bIsIterCell = FALSE;
                            pIterCell->nSeenInIteration = 0;
                            if (pIterCell->IsDirtyOrInTableOpDirty())
                            {
                                pIterCell->bRunning = (*aIter).bOldRunning;
                                pIterCell->bDirty = FALSE;
                                pIterCell->bTableOpDirty = FALSE;
                                pIterCell->pCode->SetError( errNoConvergence);
                                pIterCell->bChanged = TRUE;
                                pIterCell->SetTextWidth( TEXTWIDTH_DIRTY);
                                pIterCell->SetScriptType( SC_SCRIPTTYPE_UNKNOWN);
                            }
                        }
                    }
                    // End this iteration and remove entries.
                    rRecursionHelper.EndIteration();
                    bResumeIteration = rRecursionHelper.IsDoingIteration();
                }
            }
            if (rRecursionHelper.IsInRecursionReturn() &&
                    rRecursionHelper.GetRecursionCount() == 0 &&
                    !rRecursionHelper.IsDoingRecursion())
            {
                bIterationFromRecursion = false;
                // Iterate over cells known so far, start with the last cell
                // encountered, inserting new cells if another recursion limit
                // is reached. Repeat until solved.
                rRecursionHelper.SetDoingRecursion( true);
                do
                {
                    rRecursionHelper.SetInRecursionReturn( false);
                    for (ScFormulaRecursionList::const_iterator aIter(
                                rRecursionHelper.GetStart());
                            !rRecursionHelper.IsInReturn() && aIter !=
                            rRecursionHelper.GetEnd(); ++aIter)
                    {
                        ScFormulaCell* pCell = (*aIter).pCell;
                        if (pCell->IsDirtyOrInTableOpDirty())
                        {
                            pCell->InterpretTail( SCITP_NORMAL);
                            if (!pCell->IsDirtyOrInTableOpDirty() && !pCell->IsIterCell())
                                pCell->bRunning = (*aIter).bOldRunning;
                        }
                    }
                } while (rRecursionHelper.IsInRecursionReturn());
                rRecursionHelper.SetDoingRecursion( false);
                if (rRecursionHelper.IsInIterationReturn())
                {
                    if (!bResumeIteration)
                        bIterationFromRecursion = true;
                }
                else if (bResumeIteration ||
                        rRecursionHelper.IsDoingIteration())
                    rRecursionHelper.GetList().erase(
                            rRecursionHelper.GetStart(),
                            rRecursionHelper.GetLastIterationStart());
                else
                    rRecursionHelper.Clear();
            }
        } while (bIterationFromRecursion || bResumeIteration);
    }
}

void ScFormulaCell::InterpretTail( ScInterpretTailParameter eTailParam )
{
    class RecursionCounter
    {
        ScRecursionHelper&  rRec;
        bool                bStackedInIteration;
        public:
        RecursionCounter( ScRecursionHelper& r, ScFormulaCell* p ) : rRec(r)
        {
            bStackedInIteration = rRec.IsDoingIteration();
            if (bStackedInIteration)
                rRec.GetRecursionInIterationStack().push( p);
            rRec.IncRecursionCount();
        }
        ~RecursionCounter()
        {
            rRec.DecRecursionCount();
            if (bStackedInIteration)
                rRec.GetRecursionInIterationStack().pop();
        }
    } aRecursionCounter( pDocument->GetRecursionHelper(), this);
    nSeenInIteration = pDocument->GetRecursionHelper().GetIteration();
    if( !pCode->GetCodeLen() && !pCode->GetError() )
    {
        // #i11719# no UPN and no error and no token code but result string present
        // => interpretation of this cell during name-compilation and unknown names
        // => can't exchange underlying code array in CompileTokenArray() /
        // Compile() because interpreter's token iterator would crash.
        // This should only be a temporary condition and, since we set an
        // error, if ran into it again we'd bump into the dirty-clearing
        // condition further down.
        if ( !pCode->GetLen() && aErgString.Len() )
        {
            pCode->SetError( errNoCode );
            // This is worth an assertion; if encountered in daily work
            // documents we might need another solution. Or just confirm correctness.
            DBG_ERRORFILE( "ScFormulaCell::Interpret: no UPN, no error, no token, but string" );
            return;
        }
        CompileTokenArray();
    }

    if( pCode->GetCodeLen() && pDocument )
    {
        class StackCleaner
        {
            ScDocument*     pDoc;
            ScInterpreter*  pInt;
            public:
            StackCleaner( ScDocument* pD, ScInterpreter* pI )
                : pDoc(pD), pInt(pI)
                {}
            ~StackCleaner()
            {
                delete pInt;
                pDoc->DecInterpretLevel();
            }
        };
        pDocument->IncInterpretLevel();
        ScInterpreter* p = new ScInterpreter( this, pDocument, aPos, *pCode );
        StackCleaner aStackCleaner( pDocument, p);
        USHORT nOldErrCode = pCode->GetError();
        if ( nSeenInIteration == 0 )
        {   // Only the first time
            // With bChanged=FALSE, if a newly compiled cell has a result of
            // 0.0, no change is detected and the cell will not be repainted.
            // bChanged = FALSE;
            if (nOldErrCode == errNoConvergence &&
                    pDocument->GetDocOptions().IsIter())
                pCode->SetError( 0 );
        }
        if ( pMatrix )
        {
            DBG_ASSERT( pMatrix->IsEternalRef(), "ScFormulaCell.pMatrix is not eternal");
            pMatrix->Delete();
            pMatrix = NULL;
        }

        switch ( pCode->GetError() )
        {
            case errCircularReference :     // wird neu festgestellt
                pCode->SetError( 0 );
            break;
        }

        BOOL bOldRunning = bRunning;
        bRunning = TRUE;
        p->Interpret();
        if (pDocument->GetRecursionHelper().IsInReturn() && eTailParam != SCITP_CLOSE_ITERATION_CIRCLE)
        {
            if (nSeenInIteration > 0)
                --nSeenInIteration;     // retry when iteration is resumed
            return;
        }
        bRunning = bOldRunning;
        // Do not create a HyperLink() cell if the formula results in an error.
        if( pCode->GetError() && pCode->IsHyperLink())
            pCode->SetHyperLink(FALSE);

        if( pCode->GetError() && pCode->GetError() != errCircularReference)
        {
            bDirty = FALSE;
            bTableOpDirty = FALSE;
            bChanged = TRUE;
            bIsValue = TRUE;
        }
        if (eTailParam == SCITP_FROM_ITERATION && IsDirtyOrInTableOpDirty())
        {
            // Did it converge?
            if ((bIsValue && p->GetResultType() == svDouble && fabs(
                            p->GetNumResult() - nErgValue) <=
                        pDocument->GetDocOptions().GetIterEps()) ||
                    (!bIsValue && p->GetResultType() == svString &&
                     p->GetStringResult() == aErgString))
            {
                // A convergence in the first iteration doesn't necessarily
                // mean that it's done, it may be because not all related cells
                // of a circle changed their values yet. If the set really
                // converges it will do so also during the next iteration. This
                // fixes situations like of #i44115#. If this wasn't wanted an
                // initial "uncalculated" value would be needed for all cells
                // of a circular dependency => graph needed before calculation.
                if (nSeenInIteration > 1 ||
                        pDocument->GetDocOptions().GetIterCount() == 1)
                {
                    bDirty = FALSE;
                    bTableOpDirty = FALSE;
                }
            }
        }

        switch( p->GetResultType() )
        {
            case svDouble:
                if( nErgValue != p->GetNumResult() || !bIsValue )
                {
                    bChanged = TRUE;
                    bIsValue = TRUE;
                    nErgValue = p->GetNumResult();
                }
            break;
            case svString:
                if( aErgString != p->GetStringResult() || bIsValue )
                {
                    bChanged = TRUE;
                    bIsValue = FALSE;
                    aErgString = p->GetStringResult();
                }
            break;
            default:
                ;   // nothing
        }

        // Neuer Fehlercode?
        if( !bChanged && pCode->GetError() != nOldErrCode )
            bChanged = TRUE;
        // Anderes Zahlenformat?
        if( nFormatType != p->GetRetFormatType() )
        {
            nFormatType = p->GetRetFormatType();
            bChanged = TRUE;
        }
        if( nFormatIndex != p->GetRetFormatIndex() )
        {
            nFormatIndex = p->GetRetFormatIndex();
            bChanged = TRUE;
        }
        // Genauigkeit wie angezeigt?
        if ( bIsValue && !pCode->GetError()
          && pDocument->GetDocOptions().IsCalcAsShown()
          && nFormatType != NUMBERFORMAT_DATE
          && nFormatType != NUMBERFORMAT_TIME
          && nFormatType != NUMBERFORMAT_DATETIME )
        {
            ULONG nFormat = pDocument->GetNumberFormat( aPos );
            if ( nFormatIndex && (nFormat % SV_COUNTRY_LANGUAGE_OFFSET) == 0 )
                nFormat = nFormatIndex;
            if ( (nFormat % SV_COUNTRY_LANGUAGE_OFFSET) == 0 )
                nFormat = ScGlobal::GetStandardFormat(
                    *pDocument->GetFormatTable(), nFormat, nFormatType );
            nErgValue = pDocument->RoundValueAsShown( nErgValue, nFormat );
        }
        if (eTailParam == SCITP_NORMAL)
        {
            bDirty = FALSE;
            bTableOpDirty = FALSE;
        }
        pMatrix = p->GetMatrixResult();
        if( pMatrix )
        {
            // If the formula wasn't entered as a matrix formula, live on with
            // the upper left corner and let the interpreter delete the matrix.
            if( cMatrixFlag != MM_FORMULA && !pCode->IsHyperLink() )
                pMatrix = NULL;
            else
                pMatrix->SetEternalRef();   // it's mine now
        }
        if( bChanged )
        {
            SetTextWidth( TEXTWIDTH_DIRTY );
            SetScriptType( SC_SCRIPTTYPE_UNKNOWN );
        }
        if ( !pCode->IsRecalcModeAlways() )
            pDocument->RemoveFromFormulaTree( this );
#ifndef PRODUCT
        if ( bIsValue && !pCode->GetError() && !::rtl::math::isFinite( nErgValue ) )
        {
            DBG_ERRORFILE( msgDbgInfinity );
            nErgValue = 0.0;
            pCode->SetError( errIllegalFPOperation );
        }
#endif

        //  FORCED Zellen auch sofort auf Gueltigkeit testen (evtl. Makro starten)

        if ( pCode->IsRecalcModeForced() )
        {
            ULONG nValidation = ((const SfxUInt32Item*) pDocument->GetAttr(
                    aPos.Col(), aPos.Row(), aPos.Tab(), ATTR_VALIDDATA ))->GetValue();
            if ( nValidation )
            {
                const ScValidationData* pData = pDocument->GetValidationEntry( nValidation );
                if ( pData && !pData->IsDataValid( this, aPos ) )
                    pData->DoCalcError( this );
            }
        }

        // Reschedule verlangsamt das ganze erheblich, nur bei Prozentaenderung ausfuehren
        ScProgress::GetInterpretProgress()->SetStateCountDownOnPercent(
            pDocument->GetFormulaCodeInTree() );
    }
    else
    {
        //  Zelle bei Compiler-Fehlern nicht ewig auf dirty stehenlassen
        DBG_ASSERT( pCode->GetError(), "kein UPN-Code und kein Fehler ?!?!" );
        bDirty = FALSE;
        bTableOpDirty = FALSE;
    }
}


ULONG ScFormulaCell::GetStandardFormat( SvNumberFormatter& rFormatter, ULONG nFormat ) const
{
	if ( nFormatIndex && (nFormat % SV_COUNTRY_LANGUAGE_OFFSET) == 0 )
		return nFormatIndex;
	if ( bIsValue )		//! nicht IsValue()
		return ScGlobal::GetStandardFormat(	nErgValue, rFormatter, nFormat, nFormatType );
	else
		return ScGlobal::GetStandardFormat(	rFormatter, nFormat, nFormatType );
}


void __EXPORT ScFormulaCell::Notify( SvtBroadcaster& rBC, const SfxHint& rHint)
{
	if ( !pDocument->IsInDtorClear() && !pDocument->GetHardRecalcState() )
	{
		const ScHint* p = PTR_CAST( ScHint, &rHint );
		if( p && (p->GetId() & (SC_HINT_DATACHANGED | SC_HINT_DYING |
				SC_HINT_TABLEOPDIRTY)) )
		{
            BOOL bForceTrack = FALSE;
			if ( p->GetId() & SC_HINT_TABLEOPDIRTY )
            {
                bForceTrack = !bTableOpDirty;
                if ( !bTableOpDirty )
                {
                    pDocument->AddTableOpFormulaCell( this );
                    bTableOpDirty = TRUE;
                }
            }
			else
            {
                bForceTrack = !bDirty;
				bDirty = TRUE;
            }
            // #35962# Don't remove from FormulaTree to put in FormulaTrack to
            // put in FormulaTree again and again, only if necessary.
            // Any other means except RECALCMODE_ALWAYS by which a cell could
            // be in FormulaTree if it would notify other cells through
            // FormulaTrack which weren't in FormulaTrack/FormulaTree before?!?
            // #87866# Yes. The new TableOpDirty made it necessary to have a
            // forced mode where formulas may still be in FormulaTree from
            // TableOpDirty but have to notify dependents for normal dirty.
            if ( (bForceTrack || !pDocument->IsInFormulaTree( this )
					|| pCode->IsRecalcModeAlways())
					&& !pDocument->IsInFormulaTrack( this ) )
				pDocument->AppendToFormulaTrack( this );
		}
	}
}

void ScFormulaCell::SetDirty()
{
	if ( !IsInChangeTrack() )
	{
		if ( pDocument->GetHardRecalcState() )
			bDirty = TRUE;
		else
		{
			// Mehrfach-FormulaTracking in Load und in CompileAll
			// nach CopyScenario und CopyBlockFromClip vermeiden.
			// Wenn unbedingtes FormulaTracking noetig, vor SetDirty bDirty=FALSE
			// setzen, z.B. in CompileTokenArray
			if ( !bDirty || !pDocument->IsInFormulaTree( this ) )
			{
				bDirty = TRUE;
				pDocument->AppendToFormulaTrack( this );
				pDocument->TrackFormulas();
			}
		}
	}
}

void ScFormulaCell::SetTableOpDirty()
{
	if ( !IsInChangeTrack() )
	{
		if ( pDocument->GetHardRecalcState() )
			bTableOpDirty = TRUE;
		else
		{
			if ( !bTableOpDirty || !pDocument->IsInFormulaTree( this ) )
			{
                if ( !bTableOpDirty )
                {
                    pDocument->AddTableOpFormulaCell( this );
                    bTableOpDirty = TRUE;
                }
				pDocument->AppendToFormulaTrack( this );
				pDocument->TrackFormulas( SC_HINT_TABLEOPDIRTY );
			}
		}
	}
}


BOOL ScFormulaCell::IsDirtyOrInTableOpDirty() const
{
	return bDirty || (bTableOpDirty && pDocument->IsInInterpreterTableOp());
}


void ScFormulaCell::SetErrCode( USHORT n )
{
	pCode->SetError( n );
	bIsValue = FALSE;
}

void ScFormulaCell::AddRecalcMode( ScRecalcMode nBits )
{
	if ( (nBits & RECALCMODE_EMASK) != RECALCMODE_NORMAL )
		bDirty = TRUE;
	if ( nBits & RECALCMODE_ONLOAD_ONCE )
	{	// OnLoadOnce nur zum Dirty setzen nach Filter-Import
		nBits = (nBits & ~RECALCMODE_EMASK) | RECALCMODE_NORMAL;
	}
	pCode->AddRecalcMode( nBits );
}

// Dynamically create the URLField on a mouse-over action on a hyperlink() cell.
void ScFormulaCell::GetURLResult( String& rURL, String& rCellText )
{
    String aCellString;
        
    Color* pColor;

    // Cell Text uses the Cell format while the URL uses 
    // the default format for the type.
    ULONG nCellFormat = pDocument->GetNumberFormat( aPos );
    SvNumberFormatter* pFormatter = pDocument->GetFormatTable();
			         
    if ( (nCellFormat % SV_COUNTRY_LANGUAGE_OFFSET) == 0 )
        nCellFormat = GetStandardFormat( *pFormatter,nCellFormat );

   ULONG nURLFormat = ScGlobal::GetStandardFormat( *pFormatter,nCellFormat, NUMBERFORMAT_NUMBER);

    if ( IsValue() )
    {
        double fValue = GetValue();
        pFormatter->GetOutputString( fValue, nCellFormat, rCellText, &pColor );
    }
    else
    {
        GetString( aCellString );
        pFormatter->GetOutputString( aCellString, nCellFormat, rCellText, &pColor );
    }
        
    if(pMatrix)
    {
        ScMatValType nMatValType;
        // determine if the matrix result is a string or value.
        const ScMatrixValue* pMatVal = pMatrix->Get(0, 1, nMatValType);
        if (pMatVal)
        {
            if (nMatValType != SC_MATVAL_VALUE)
                rURL = pMatVal->GetString();
            else
                pFormatter->GetOutputString( pMatVal->fVal, nURLFormat, rURL, &pColor );
        }
    }

    if(!rURL.Len())
    {
        if(IsValue())
            pFormatter->GetOutputString( GetValue(), nURLFormat, rURL, &pColor );
        else
            pFormatter->GetOutputString( aCellString, nURLFormat, rURL, &pColor );
    }
}

EditTextObject* ScFormulaCell::CreateURLObject() 
{
    String aCellText;
    String aURL;
    GetURLResult( aURL, aCellText );

    SvxURLField aUrlField( aURL, aCellText, SVXURLFORMAT_APPDEFAULT);
    EditEngine& rEE = pDocument->GetEditEngine();
    rEE.SetText( EMPTY_STRING );
    rEE.QuickInsertField( SvxFieldItem( aUrlField ), ESelection( 0xFFFF, 0xFFFF ) );
    
    return rEE.CreateTextObject();
}

//------------------------------------------------------------------------

ScDetectiveRefIter::ScDetectiveRefIter( ScFormulaCell* pCell )
{
	pCode = pCell->GetCode();
	pCode->Reset();
	aPos = pCell->aPos;
}

BOOL lcl_ScDetectiveRefIter_SkipRef( ScToken* p )
{
	SingleRefData& rRef1 = p->GetSingleRef();
	if ( rRef1.IsColDeleted() || rRef1.IsRowDeleted() || rRef1.IsTabDeleted()
			|| !rRef1.Valid() )
		return TRUE;
	if ( p->GetType() == svDoubleRef )
	{
		SingleRefData& rRef2 = p->GetDoubleRef().Ref2;
		if ( rRef2.IsColDeleted() || rRef2.IsRowDeleted() || rRef2.IsTabDeleted()
				|| !rRef2.Valid() )
			return TRUE;
	}
	return FALSE;
}

BOOL ScDetectiveRefIter::GetNextRef( ScRange& rRange )
{
	BOOL bRet = FALSE;

	ScToken* p = pCode->GetNextReferenceRPN();
	if (p)
		p->CalcAbsIfRel( aPos );

	while ( p && lcl_ScDetectiveRefIter_SkipRef( p ) )
	{
		p = pCode->GetNextReferenceRPN();
		if (p)
			p->CalcAbsIfRel( aPos );
	}

	if( p )
	{
		SingleDoubleRefProvider aProv( *p );
		rRange.aStart.Set( aProv.Ref1.nCol, aProv.Ref1.nRow, aProv.Ref1.nTab );
		rRange.aEnd.Set( aProv.Ref2.nCol, aProv.Ref2.nRow, aProv.Ref2.nTab );
		bRet = TRUE;
	}

	return bRet;
}

//-----------------------------------------------------------------------------------

ScFormulaCell::~ScFormulaCell()
{
	pDocument->RemoveFromFormulaTree( this );
	delete pCode;
    if ( pMatrix )
    {
        DBG_ASSERT( pMatrix->IsEternalRef(), "~ScFormulaCell:: pMatrix is not eternal");
	    pMatrix->Delete();
	    pMatrix = NULL;
    }
#ifdef DBG_UTIL
	eCellType = CELLTYPE_DESTROYED;
#endif
}


#ifdef DBG_UTIL

ScStringCell::~ScStringCell()
{
	eCellType = CELLTYPE_DESTROYED;
}
#endif
									//! ValueCell auch nur bei DBG_UTIL,
									//! auch in cell.hxx aendern !!!!!!!!!!!!!!!!!!!!

ScValueCell::~ScValueCell()
{
	eCellType = CELLTYPE_DESTROYED;
}

#ifdef DBG_UTIL

ScNoteCell::~ScNoteCell()
{
	eCellType = CELLTYPE_DESTROYED;
}
#endif




