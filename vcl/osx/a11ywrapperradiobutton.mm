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


#include "osx/salinst.h"
#include "a11ywrapperradiobutton.h"
#include "a11ytextwrapper.h"
#include "a11yvaluewrapper.h"

#ifdef USE_JAVA
#include "../java/source/app/salinst_cocoa.h"
#endif	// USE_JAVA

// Wrapper for AXRadioButton role

@implementation AquaA11yWrapperRadioButton : AquaA11yWrapper

-(id)valueAttribute {
    if ( [ self accessibleValue ] != nil ) {
        return [ AquaA11yValueWrapper valueAttributeForElement: self ];
    }
    return [ NSNumber numberWithInt: 0 ];
}

-(BOOL)accessibilityIsAttributeSettable:(NSString *)attribute {
    if ( [ attribute isEqualToString: NSAccessibilityValueAttribute ] ) {
        return NO;
    }
#ifdef USE_JAVA
    BOOL isSettable = NO;
    if ( !ImplApplicationIsRunning() )
        return isSettable;
    // Set drag lock if it has not already been set since dispatching native
    // events to windows during an accessibility call can cause crashing
    ACQUIRE_DRAGPRINTLOCK
    if ( !ImplIsValidAquaA11yWrapper( self ) || [ self isDisposed ] ) {
        RELEASE_DRAGPRINTLOCKIFNEEDED
        return isSettable;
    }
    isSettable = [ super accessibilityIsAttributeSettable: attribute ];
    RELEASE_DRAGPRINTLOCK
    return isSettable;
#else	// USE_JAVA
    return [ super accessibilityIsAttributeSettable: attribute ];
#endif	// USE_JAVA
}

-(NSArray *)accessibilityAttributeNames {
#ifdef USE_JAVA
    NSMutableArray * attributeNames = nil;
    if ( !ImplApplicationIsRunning() )
        return attributeNames;
    // Set drag lock if it has not already been set since dispatching native
    // events to windows during an accessibility call can cause crashing
    ACQUIRE_DRAGPRINTLOCK
    if ( !ImplIsValidAquaA11yWrapper( self ) || [ self isDisposed ] ) {
        RELEASE_DRAGPRINTLOCKIFNEEDED
        return attributeNames;
    }
    // Default Attributes
    attributeNames = [ NSMutableArray arrayWithArray: [ super accessibilityAttributeNames ] ];
#else	// USE_JAVA
    // Default Attributes
    NSMutableArray * attributeNames = [ NSMutableArray arrayWithArray: [ super accessibilityAttributeNames ] ];
#endif	// USE_JAVA
    // Special Attributes and removing unwanted attributes depending on role
    [ attributeNames removeObjectsInArray: [ AquaA11yTextWrapper specialAttributeNames ] ];
    [ attributeNames addObject: NSAccessibilityMinValueAttribute ];
    [ attributeNames addObject: NSAccessibilityMaxValueAttribute ];
    [ attributeNames addObject: NSAccessibilityValueAttribute ];
#ifdef USE_JAVA
    RELEASE_DRAGPRINTLOCK
    if ( !attributeNames )
        return [ NSArray array ];
    else
#endif	// USE_JAVA
    return attributeNames;
}

@end

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
