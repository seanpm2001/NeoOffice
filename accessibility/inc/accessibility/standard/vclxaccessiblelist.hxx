/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
 */

#ifndef INCLUDED_ACCESSIBILITY_INC_ACCESSIBILITY_STANDARD_VCLXACCESSIBLELIST_HXX
#define INCLUDED_ACCESSIBILITY_INC_ACCESSIBILITY_STANDARD_VCLXACCESSIBLELIST_HXX

#include <vector>
#include <functional>
#include "accessibility/standard/vclxaccessiblelistitem.hxx"
#include <accessibility/standard/vclxaccessibleedit.hxx>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/XAccessibleAction.hpp>
#include <com/sun/star/accessibility/XAccessibleEventBroadcaster.hpp>
#include <com/sun/star/accessibility/XAccessibleSelection.hpp>
#include <cppuhelper/implbase2.hxx>

typedef ::cppu::ImplHelper2<
    ::com::sun::star::accessibility::XAccessible,
    ::com::sun::star::accessibility::XAccessibleSelection
    > VCLXAccessibleList_BASE;

typedef std::vector< ::com::sun::star::uno::WeakReference< ::com::sun::star::accessibility::XAccessible > >
    ListItems;

namespace accessibility
{
    class IComboListBoxHelper;
}


/** Base class for the list contained in list- and combo boxes.  This class
    does not support selection because lists of combo boxes give no direct
    access to their underlying list implementation.  Look into derived
    classes for selection.
*/
class VCLXAccessibleList
    : public VCLXAccessibleComponent,
      public VCLXAccessibleList_BASE
{
public:
    enum BoxType {COMBOBOX, LISTBOX};

    VCLXAccessibleList (VCLXWindow* pVCLXindow, BoxType aBoxType,
                        const ::com::sun::star::uno::Reference<
                            ::com::sun::star::accessibility::XAccessible >& _xParent);

    /** The index that is passed to this method is returned on following
        calls to <member>getAccessibleIndexInParent</member>.
    */
    void SetIndexInParent (sal_Int32 nIndex);

    /** Process some of the events and delegate the rest to the base classes.
    */
    virtual void ProcessWindowEvent (const VclWindowEvent& rVclWindowEvent) SAL_OVERRIDE;
    virtual void    FillAccessibleRelationSet( utl::AccessibleRelationSetHelper& rRelationSet ) SAL_OVERRIDE;

    /** Called on reception of selection events this method checks all known
        list items for a possible change in their selection state and
        updates that accordingly.  No accessibility events are send because
        the XAccessibleSelection interface is not supported and the items
        are transient.
        @param sTextOfSelectedItem
            This string contains the text of the currently selected
            item.  It is used to retrieve the index of that item.
    */
    void UpdateSelection (const OUString& sTextOfSelectedItem);

    // XInterface
    DECLARE_XINTERFACE()

    // XTypeProvider
    DECLARE_XTYPEPROVIDER()

    // XAccessible
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessibleContext> SAL_CALL
        getAccessibleContext (void)
        throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;

    // XAccessibleContext
    virtual sal_Int32 SAL_CALL getAccessibleChildCount (void)
        throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible> SAL_CALL
        getAccessibleChild (sal_Int32 i)
        throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > SAL_CALL
        getAccessibleParent(  )
        throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;

    /** The index returned as index in parent is always the one set with the
        <member>SetIndexInParent()</member> method.
    */
    virtual sal_Int32 SAL_CALL getAccessibleIndexInParent (void)
        throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual sal_Int16 SAL_CALL getAccessibleRole (void)
        throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;

    // XAccessibleComponent
    virtual bool SAL_CALL contains (const ::com::sun::star::awt::Point& aPoint)
        throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible> SAL_CALL
        getAccessibleAt (const ::com::sun::star::awt::Point& aPoint)
        throw (::com::sun::star::uno::RuntimeException);


    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName (void)
        throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    // Return list specific services.
    virtual ::com::sun::star::uno::Sequence< OUString > SAL_CALL
        getSupportedServiceNames (void)
        throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;

    // XAccessibleSelection
    virtual void SAL_CALL selectAccessibleChild( sal_Int32 nChildIndex ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual sal_Bool SAL_CALL isAccessibleChildSelected( sal_Int32 nChildIndex ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL clearAccessibleSelection(  ) throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL selectAllAccessibleChildren(  ) throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual sal_Int32 SAL_CALL getSelectedAccessibleChildCount(  ) throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > SAL_CALL getSelectedAccessibleChild( sal_Int32 nSelectedChildIndex ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL deselectAccessibleChild( sal_Int32 nSelectedChildIndex ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;

    virtual ::com::sun::star::awt::Point SAL_CALL getLocationOnScreen(  ) throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    bool    IsInDropDown();
    void        HandleDropOpen();
    virtual void ProcessWindowEvent (const VclWindowEvent& rVclWindowEvent, bool b_IsDropDownList);
    void UpdateSelection_Acc (const ::rtl::OUString& sTextOfSelectedItem, bool b_IsDropDownList);
    void UpdateSelection_Impl_Acc (bool b_IsDropDownList);

    void UpdateFocus_Impl_Acc ( sal_uInt16 nPos, bool b_IsDropDownList) ;
    void NotifyListItem(::com::sun::star::uno::Any& val);
protected:
    BoxType     m_aBoxType;
    ::accessibility::IComboListBoxHelper* m_pListBoxHelper;
    ListItems   m_aAccessibleChildren;
    sal_Int32   m_nVisibleLineCount;
    /// Index in parent.  This is settable from the outside.
    sal_Int32   m_nIndexInParent;
    sal_Int32   m_nLastTopEntry;
    sal_Int32   m_nLastSelectedPos;
    bool        m_bDisableProcessEvent;
    bool        m_bVisible;
    sal_Int32   m_nCurSelectedPos;


    /// The currently selected item.
    ::com::sun::star::uno::WeakReference< ::com::sun::star::accessibility::XAccessible>
        m_xSelectedItem;

    virtual ~VCLXAccessibleList (void);

    /** This function is called from the implementation helper during a
        XComponent::dispose call.  Free the list of items and the items themselves.
    */
    virtual void SAL_CALL disposing (void) SAL_OVERRIDE;

    /** This method adds the states
        <const>AccessibleStateType::FOCUSABLE</const> and possibly
        <const>AccessibleStateType::MULTI_SELECTABLE</const> to the state set
        of the base classes.
    */
    virtual void FillAccessibleStateSet (utl::AccessibleStateSetHelper& rStateSet) SAL_OVERRIDE;

    /** Create the specified child and insert it into the list of children.
        Sets the child's states.
    */
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible >
        CreateChild (sal_Int32 i);

    /** Call this method when the item list has been changed, i.e. items
        have been deleted or inserted.
        @param bItemInserted
            Indicate whether items have been inserted (<TRUE/>) or removed
            (<FALSE/>).
        @param nIndex
            Index of the new or removed item.  A value of -1 indicates that
            the whole list has been cleared.
    */
    virtual void HandleChangedItemList (bool bItemInserted, sal_Int32 nIndex);

    // VCLXAccessibleComponent
    virtual ::com::sun::star::awt::Rectangle implGetBounds(  ) throw (::com::sun::star::uno::RuntimeException) SAL_OVERRIDE;

private:
    /** We need to save the accessible parent to return it in <type>getAccessibleParent()</type>,
        because this method of the base class returns the wrong parent.
    */
    ::com::sun::star::uno::Reference<
        ::com::sun::star::accessibility::XAccessible >  m_xParent;


#ifdef NO_LIBO_SIMPLIFY_CLEARITEMS_FIX
    /** dispose all items aand clears the container
    */
    void clearItems();
#endif	// NO_LIBO_SIMPLIFY_CLEARITEMS_FIX

    void UpdateEntryRange_Impl (void);
protected:
    void UpdateSelection_Impl (sal_uInt16 nPos = 0);
    bool checkEntrySelected(sal_uInt16 _nPos,
                            ::com::sun::star::uno::Any& _rNewValue,
                            ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible >& _rxNewAcc);
private:
    void notifyVisibleStates(bool _bSetNew );
    void UpdateVisibleLineCount();
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
