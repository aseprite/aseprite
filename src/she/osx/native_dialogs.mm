// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <Cocoa/Cocoa.h>
#include <vector>

#include "she/display.h"
#include "she/keys.h"
#include "she/native_cursor.h"
#include "she/osx/native_dialogs.h"

#include "base/path.h"

@interface OpenSaveHelper : NSObject {
@private
  NSSavePanel* panel;
  she::Display* display;
  int result;
}
- (id)init;
- (void)setPanel:(NSSavePanel*)panel;
- (void)setDisplay:(she::Display*)display;
- (void)runModal;
- (int)result;
@end

@implementation OpenSaveHelper

- (id)init
{
  if (self = [super init]) {
    result = NSFileHandlingPanelCancelButton;
  }
  return self;
}

- (void)setPanel:(NSSavePanel*)newPanel
{
  panel = newPanel;
}

- (void)setDisplay:(she::Display*)newDisplay
{
  display = newDisplay;
}

// This is executed in the main thread.
- (void)runModal
{
  she::NativeCursor oldCursor = display->nativeMouseCursor();
  display->setNativeMouseCursor(she::kArrowCursor);

#ifndef __MAC_10_6              // runModalForTypes is deprecated in 10.6
  if ([panel isKindOfClass:[NSOpenPanel class]]) {
    // As we're using OS X 10.4 framework, it looks like runModal
    // doesn't recognize the allowedFileTypes property. So we force it
    // using runModalForTypes: selector.

    result = [(NSOpenPanel*)panel runModalForTypes:[panel allowedFileTypes]];
  }
  else
#endif
  {
    result = [panel runModal];
  }

  display->setNativeMouseCursor(oldCursor);
}

- (int)result
{
  return result;
}

@end

namespace she {

class FileDialogOSX : public FileDialog {
public:
  FileDialogOSX()
    : m_save(false)
  {
  }

  void dispose() override {
    delete this;
  }

  void toOpenFile() override {
    m_save = false;
  }

  void toSaveFile() override {
    m_save = true;
  }

  void setTitle(const std::string& title) override {
    m_title = title;
  }

  void setDefaultExtension(const std::string& extension) override {
    m_defExtension = extension;
  }

  void addFilter(const std::string& extension, const std::string& description) override {
    if (m_defExtension.empty())
      m_defExtension = extension;

    m_filters.push_back(std::make_pair(description, extension));
  }

  std::string getFileName() override {
    return m_filename;
  }

  void setFileName(const std::string& filename) override {
    m_filename = filename;
  }

  bool show(Display* display) override {
    NSSavePanel* panel = nil;

    if (m_save) {
      panel = [NSSavePanel savePanel];
    }
    else {
      panel = [NSOpenPanel openPanel];
      [(NSOpenPanel*)panel setAllowsMultipleSelection:NO];
      [(NSOpenPanel*)panel setCanChooseDirectories:NO];
    }

    [panel setTitle:[NSString stringWithUTF8String:m_title.c_str()]];
    [panel setCanCreateDirectories:YES];

    std::string defPath = base::get_file_path(m_filename);
    std::string defName = base::get_file_name(m_filename);
    if (!defPath.empty())
      [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:defPath.c_str()]]];
    if (!defName.empty())
      [panel setNameFieldStringValue:[NSString stringWithUTF8String:defName.c_str()]];

    NSMutableArray* types = [[NSMutableArray alloc] init];
    // The first extension in the array is used as the default one.
    if (!m_defExtension.empty())
      [types addObject:[NSString stringWithUTF8String:m_defExtension.c_str()]];
    for (const auto& filter : m_filters)
      [types addObject:[NSString stringWithUTF8String:filter.second.c_str()]];
    [panel setAllowedFileTypes:types];

    OpenSaveHelper* helper = [[OpenSaveHelper alloc] init];
    [helper setPanel:panel];
    [helper setDisplay:display];
    [helper performSelectorOnMainThread:@selector(runModal) withObject:nil waitUntilDone:YES];

    bool retValue;
    if ([helper result] == NSFileHandlingPanelOKButton) {
      NSURL* url = [panel URL];
      m_filename = [[url path] UTF8String];
      retValue = true;
    }
    else
      retValue = false;

#if !__has_feature(objc_arc)
    [helper release];
    [types release];
#endif
    return retValue;
  }

private:

  std::vector<std::pair<std::string, std::string>> m_filters;
  std::string m_defExtension;
  std::string m_filename;
  std::string m_title;
  bool m_save;
};

NativeDialogsOSX::NativeDialogsOSX()
{
}

FileDialog* NativeDialogsOSX::createFileDialog()
{
  return new FileDialogOSX();
}

} // namespace she
