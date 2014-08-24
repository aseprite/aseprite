// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include "she/logger.h"

namespace she {

  class OSXLogger : public Logger {
  public:
    void logError(const char* error) override {
      NSLog([NSString stringWithUTF8String:error]);
    }
  };

  Logger* getOsxLogger() {
    return new OSXLogger;
  }

} // namespace she
