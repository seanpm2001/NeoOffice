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
 * Modified December 2009 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

#ifndef _SVX_TABLECONTROLLER_HXX_
#define _SVX_TABLECONTROLLER_HXX_

#include <com/sun/star/util/XModifyListener.hpp>
#include <com/sun/star/table/XTable.hpp>
#include <rtl/ref.hxx>

#include <svx/sdr/overlay/overlayobjectlist.hxx>
#include <svx/selectioncontroller.hxx>
#include <svx/svdotable.hxx>
#include <svx/svdview.hxx>
#include <tablemodel.hxx>

class SdrObjEditView;
class SdrObject;

namespace sdr { namespace table {

const sal_Int16 SELTYPE_NONE = 0;
const sal_Int16 SELTYPE_MOUSE = 1;
const sal_Int16 SELTYPE_KEYS = 2;

class SvxTableController: public sdr::SelectionController
{
public:
	SvxTableController( SdrObjEditView* pView, const SdrObject* pObj );
	virtual ~SvxTableController();

	virtual bool onKeyInput(const KeyEvent& rKEvt, Window* pWin);
	virtual bool onMouseButtonDown(const MouseEvent& rMEvt, Window* pWin);
	virtual bool onMouseButtonUp(const MouseEvent& rMEvt, Window* pWin);
	virtual bool onMouseMove(const MouseEvent& rMEvt, Window* pWin);

	virtual bool DeleteMarked();

	virtual void onSelectionHasChanged();

	virtual void GetState( SfxItemSet& rSet );
	virtual void Execute( SfxRequest& rReq );

	virtual bool GetStyleSheet( SfxStyleSheet* &rpStyleSheet ) const;
	virtual bool SetStyleSheet( SfxStyleSheet* pStyleSheet, bool bDontRemoveHardAttr );

	// slots
	void onInsert( sal_uInt16 nSId );
	void onDelete( sal_uInt16 nSId );
	void onSelect( sal_uInt16 nSId );
	void onFormatTable( SfxRequest& rReq );
	void MergeMarkedCells();
	void SplitMarkedCells();
	void DistributeColumns();
	void DistributeRows();
	void SetVertical( sal_uInt16 nSId );

	static rtl::Reference< sdr::SelectionController > create( SdrObjEditView* pView, const SdrObject* pObj, const rtl::Reference< sdr::SelectionController >& xRefController );

	void MergeAttrFromSelectedCells(SfxItemSet& rAttr, bool bOnlyHardAttr) const;
	void SetAttrToSelectedCells(const SfxItemSet& rAttr, bool bReplaceAll);

	virtual bool GetAttributes(SfxItemSet& rTargetSet, bool bOnlyHardAttr) const;
	virtual bool SetAttributes(const SfxItemSet& rSet, bool bReplaceAll);

	virtual bool GetMarkedObjModel( SdrPage* pNewPage );
	virtual bool PasteObjModel( const SdrModel& rModel );

	bool hasSelectedCells() const { return mbCellSelectionMode || mpView->IsTextEdit(); }

	void getSelectedCells( CellPos& rFirstPos, CellPos& rLastPos );
	void setSelectedCells( const CellPos& rFirstPos, const CellPos& rLastPos );
	void clearSelection();
	void selectAll();

	void onTableModified();
#ifdef USE_JAVA
	Rectangle GetNativeHightlightColorRect();
#endif  // USE_JAVA

private:
    SvxTableController(SvxTableController &); // not defined
    void operator =(SvxTableController &); // not defined

	// internals
	void ApplyBorderAttr( const SfxItemSet& rAttr );
	void UpdateTableShape();

	void SetTableStyle( const SfxItemSet* pArgs );
	void SetTableStyleSettings( const SfxItemSet* pArgs );

	bool PasteObject( SdrTableObj* pPasteTableObj );

	bool checkTableObject();
	bool updateTableObject();
	const CellPos& getSelectionStart();
	void setSelectionStart( const CellPos& rPos );
	const CellPos& getSelectionEnd();
	::com::sun::star::uno::Reference< ::com::sun::star::table::XCellCursor > getSelectionCursor();
	void checkCell( CellPos& rPos );

	void MergeRange( sal_Int32 nFirstCol, sal_Int32 nFirstRow, sal_Int32 nLastCol, sal_Int32 nLastRow );

	void EditCell( const CellPos& rPos, ::Window* pWindow, const ::com::sun::star::awt::MouseEvent* pMouseEvent = 0, sal_uInt16 nAction = 0 );
	bool StopTextEdit();

	void DeleteTable();

	sal_uInt16 getKeyboardAction( const KeyEvent& rKEvt, Window* pWindow );
	bool executeAction( sal_uInt16 nAction, bool bSelect, Window* pWindow );
	void gotoCell( const CellPos& rCell, bool bSelect, Window* pWindow, sal_uInt16 nAction = 0 );

	void StartSelection( const CellPos& rPos );
	void UpdateSelection( const CellPos& rPos );
	void RemoveSelection();
	void updateSelectionOverlay();
	void destroySelectionOverlay();

	void findMergeOrigin( CellPos& rPos );

	DECL_LINK( UpdateHdl, void * );

	TableModelRef mxTable;

	CellPos maCursorFirstPos;
	CellPos maCursorLastPos;
	bool mbCellSelectionMode;
	CellPos maMouseDownPos;
	bool mbLeftButtonDown;
	::sdr::overlay::OverlayObjectList*	mpSelectionOverlay;

	SdrView* mpView;
	SdrObjectWeakRef mxTableObj;
	SdrModel* mpModel;

	::com::sun::star::uno::Reference< ::com::sun::star::util::XModifyListener > mxModifyListener;

	ULONG mnUpdateEvent;
};

} }

#endif // _SVX_TABLECONTROLLER_HXX_

