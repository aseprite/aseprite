// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <Cocoa/Cocoa.h>
#include <vector>

#include "base/fs.h"
#include "she/common/file_dialog.h"
#include "she/display.h"
#include "she/keys.h"
#include "she/native_cursor.h"
#include "she/osx/native_dialogs.h"

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

class FileDialogOSX : public CommonFileDialog {
public:
  FileDialogOSX() {
  }

  std::string fileName() override {
    return m_filename;
  }

  void getMultipleFileNames(std::vector<std::string>& output) override {
    output = m_filenames;
  }

  void setFileName(const std::string& filename) override {
    m_filename = filename;
  }

  bool show(Display* display) override {
    bool retValue = false;
    @autoreleasepool {
      NSSavePanel* panel = nil;

      if (m_type == Type::SaveFile) {
        panel = [NSSavePanel new];
      }
      else {
        panel = [NSOpenPanel new];
        [(NSOpenPanel*)panel setAllowsMultipleSelection:(m_type == Type::OpenFiles ? YES: NO)];
        [(NSOpenPanel*)panel setCanChooseFiles:(m_type != Type::OpenFolder ? YES: NO)];
        [(NSOpenPanel*)panel setCanChooseDirectories:(m_type == Type::OpenFolder ? YES: NO)];
      }

      [panel setTitle:[NSString stringWithUTF8String:m_title.c_str()]];
      [panel setCanCreateDirectories:YES];

      std::string defPath = base::get_file_path(m_filename);
      std::string defName = base::get_file_name(m_filename);
      if (!defPath.empty())
        [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:defPath.c_str()]]];
      if (!defName.empty())
        [panel setNameFieldStringValue:[NSString stringWithUTF8String:defName.c_str()]];

      if (m_type != Type::OpenFolder && !m_filters.empty()) {
        NSMutableArray* types = [[NSMutableArray alloc] init];
        // The first extension in the array is used as the default one.
        if (!m_defExtension.empty())
          [types addObject:[NSString stringWithUTF8String:m_defExtension.c_str()]];
        for (const auto& filter : m_filters)
          [types addObject:[NSString stringWithUTF8String:filter.first.c_str()]];
        [panel setAllowedFileTypes:types];
        if (m_type == Type::SaveFile)
          [panel setAllowsOtherFileTypes:NO];
      }

      OpenSaveHelper* helper = [OpenSaveHelper new];
      [helper setPanel:panel];
      [helper setDisplay:display];
      [helper performSelectorOnMainThread:@selector(runModal) withObject:nil waitUntilDone:YES];

      if ([helper result] == NSFileHandlingPanelOKButton) {
        if (m_type == Type::OpenFiles) {
          for (NSURL* url in [(NSOpenPanel*)panel URLs]) {
            m_filename = [[url path] UTF8String];
            m_filenames.push_back(m_filename);
          }
        }
        else {
          NSURL* url = [panel URL];
          m_filename = [[url path] UTF8String];
          m_filenames.push_back(m_filename);
        }
        retValue = true;
      }
    }
    return retValue;
  }

private:

  std::string m_filename;
  std::vector<std::string> m_filenames;
};

NativeDialogsOSX::NativeDialogsOSX()
{
}

FileDialog* NativeDialogsOSX::createFileDialog()
{
  return new FileDialogOSX();
}

} // namespace she
