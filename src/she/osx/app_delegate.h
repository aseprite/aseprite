// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_APP_DELEGATE_H_INCLUDED
#define SHE_OSX_APP_DELEGATE_H_INCLUDED
#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

@interface OSXAppDelegate : NSObject
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender;
- (void)applicationWillTerminate:(NSNotification*)aNotification;
- (void)quit:(id)sender;
@end

#endif
