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
 * 
 *   Modified July 2022 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "quartz/utils.h"
#include "a11ycomponentwrapper.h"
#include "a11yrolehelper.h"
#include <com/sun/star/accessibility/AccessibleRole.hpp>

using namespace ::com::sun::star::accessibility;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::uno;

// Wrapper for XAccessibleComponent and XAccessibleExtendedComponent

@implementation AquaA11yComponentWrapper : NSObject

+(id)sizeAttributeForElement:(AquaA11yWrapper *)wrapper {
    Size size = [ wrapper accessibleComponent ] -> getSize();
    NSSize nsSize = NSMakeSize ( (float) size.Width, (float) size.Height );
    return [ NSValue valueWithSize: nsSize ];
}

// TODO: should be merged with AquaSalFrame::VCLToCocoa... to a general helper method
+(id)positionAttributeForElement:(AquaA11yWrapper *)wrapper {
    // VCL coordinates are in upper-left-notation, Cocoa likes it the Cartesian way (lower-left)
    NSRect screenRect = [ [ NSScreen mainScreen ] frame ];
    Size size = [ wrapper accessibleComponent ] -> getSize();
    Point location = [ wrapper accessibleComponent ] -> getLocationOnScreen();
    NSPoint nsPoint = NSMakePoint ( (float) location.X, (float) ( screenRect.size.height - size.Height - location.Y ) );
    return [ NSValue valueWithPoint: nsPoint ];
}

+(id)descriptionAttributeForElement:(AquaA11yWrapper *)wrapper {
    if ( [ wrapper accessibleExtendedComponent ] != nil ) {
#ifdef USE_JAVA
        return [ CreateNSString ( [ wrapper accessibleExtendedComponent ] -> getToolTipText() ) autorelease ];
#else	// USE_JAVA
        return CreateNSString ( [ wrapper accessibleExtendedComponent ] -> getToolTipText() );
#endif	// USE_JAVA
    } else {
        return nil;
    }
}

+(void)addAttributeNamesTo:(NSMutableArray *)attributeNames {
    NSAutoreleasePool * pool = [ [ NSAutoreleasePool alloc ] init ];
    [ attributeNames addObjectsFromArray: [ NSArray arrayWithObjects: 
            NSAccessibilitySizeAttribute, 
            NSAccessibilityPositionAttribute, 
            NSAccessibilityFocusedAttribute, 
            NSAccessibilityEnabledAttribute, 
            nil ] ];
    [ pool release ];
}

+(BOOL)isAttributeSettable:(NSString *)attribute forElement:(AquaA11yWrapper *)wrapper {
    BOOL isSettable = NO;
    NSAutoreleasePool * pool = [ [ NSAutoreleasePool alloc ] init ];
    if ( [ attribute isEqualToString: NSAccessibilityFocusedAttribute ] 
      && ! [ [ AquaA11yRoleHelper getNativeRoleFrom: [ wrapper accessibleContext ] ] isEqualToString: NSAccessibilityScrollBarRole ] 
      && ! [ [ AquaA11yRoleHelper getNativeRoleFrom: [ wrapper accessibleContext ] ] isEqualToString: NSAccessibilityStaticTextRole ] ) {
        isSettable = YES;
    }
    [ pool release ];
    return isSettable;
}

+(void)setFocusedAttributeForElement:(AquaA11yWrapper *)wrapper to:(id)value {
    if ( [ value boolValue ] == YES ) {
        if ( [ wrapper accessibleContext ] -> getAccessibleRole() == AccessibleRole::COMBO_BOX ) {
            // special treatment for comboboxes: find the corresponding PANEL and set focus to it
            Reference < XAccessible > rxParent = [ wrapper accessibleContext ] -> getAccessibleParent();
            if ( rxParent.is() ) {
                Reference < XAccessibleContext > rxContext = rxParent->getAccessibleContext();
                if ( rxContext.is() && rxContext -> getAccessibleRole() == AccessibleRole::PANEL ) {
                    Reference < XAccessibleComponent > rxComponent = Reference < XAccessibleComponent > ( rxParent -> getAccessibleContext(), UNO_QUERY );
                    if ( rxComponent.is() ) {
                        rxComponent -> grabFocus();
                    }
                }
            }
        } else {
            [ wrapper accessibleComponent ] -> grabFocus();
        }
    }
}

@end

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
