// Desktop Integration
// Copyright (c) 2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <CoreFoundation/CFPlugInCOM.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

#include "base/debug.h"
#include "thumbnail.h"

// Just as a side note: We're using the same UUID as the Windows
// Aseprite thumbnailer.
//
// If you're going to use this code, remember to change this UUID and
// change it in the Info.plist file.
#define PLUGIN_ID "A5E9417E-6E7A-4B2D-85A4-84E114D7A960"

static HRESULT Plugin_QueryInterface(void*, REFIID, LPVOID*);
static ULONG Plugin_AddRef(void*);
static ULONG Plugin_Release(void*);
static OSStatus Plugin_GenerateThumbnailForURL(void*, QLThumbnailRequestRef, CFURLRef, CFStringRef, CFDictionaryRef, CGSize);
static void Plugin_CancelThumbnailGeneration(void*, QLThumbnailRequestRef);
static OSStatus Plugin_GeneratePreviewForURL(void*, QLPreviewRequestRef, CFURLRef, CFStringRef, CFDictionaryRef);
static void Plugin_CancelPreviewGeneration(void*, QLPreviewRequestRef);

static QLGeneratorInterfaceStruct Plugin_vtbl = { // kQLGeneratorTypeID interface
  // IUnknown
  nullptr,                      // void* reserved
  Plugin_QueryInterface,
  Plugin_AddRef,
  Plugin_Release,
  // QLGeneratorInterface
  Plugin_GenerateThumbnailForURL,
  Plugin_CancelThumbnailGeneration,
  Plugin_GeneratePreviewForURL,
  Plugin_CancelPreviewGeneration
};

// TODO it would be nice to create a C++ smart pointer/wrapper for CFUUIDRef type

struct Plugin {
  QLGeneratorInterfaceStruct* interface; // Must be a pointer
  CFUUIDRef factoryID;
  ULONG refCount = 1;           // Starts with one reference when it's created

  Plugin(CFUUIDRef factoryID)
    : interface(new QLGeneratorInterfaceStruct(Plugin_vtbl))
    , factoryID(factoryID) {
    CFPlugInAddInstanceForFactory(factoryID);
  }


  ~Plugin() {
    delete interface;
    if (factoryID) {
      CFPlugInRemoveInstanceForFactory(factoryID);
      CFRelease(factoryID);
    }
  }

  // IUnknown impl

  HRESULT QueryInterface(REFIID iid, LPVOID* ppv) {
    CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);

    if (CFEqual(interfaceID, kQLGeneratorCallbacksInterfaceID)) {
      *ppv = this;
      AddRef();
      CFRelease(interfaceID);
      return S_OK;
    }
    else {
      *ppv = nullptr;
      CFRelease(interfaceID);
      return E_NOINTERFACE;
    }
  }

  ULONG AddRef() {
    return ++refCount;
  }

  ULONG Release() {
    if (refCount == 1) {
      delete this;
      return 0;
    }
    else {
      ASSERT(refCount != 0);
      return --refCount;
    }
  }

  // QLGeneratorInterfaceStruct impl

  static OSStatus GenerateThumbnailForURL(QLThumbnailRequestRef thumbnail,
                                          CFURLRef url,
                                          CFStringRef contentTypeUTI,
                                          CFDictionaryRef options,
                                          CGSize maxSize) {
    CGImageRef image = desktop::get_thumbnail(url, options, maxSize);
    if (!image)
      return -1;

    QLThumbnailRequestSetImage(thumbnail, image, nullptr);
    CGImageRelease(image);
    return 0;
  }

  static void CancelThumbnailGeneration(QLThumbnailRequestRef thumbnail) {
    // TODO
  }

  OSStatus GeneratePreviewForURL(QLPreviewRequestRef preview,
                                 CFURLRef url,
                                 CFStringRef contentTypeUTI,
                                 CFDictionaryRef options) {
    CGImageRef image = desktop::get_thumbnail(url, options, CGSizeMake(0, 0));
    if (!image)
      return -1;

    int w = CGImageGetWidth(image);
    int h = CGImageGetHeight(image);
    int wh = std::min(w, h);
    if (wh < 128) {
      w = 128 * w / wh;
      h = 128 * h / wh;
    }

    CGContextRef cg = QLPreviewRequestCreateContext(preview, CGSizeMake(w, h), YES, options);
    CGContextSetInterpolationQuality(cg, kCGInterpolationNone);
    CGContextDrawImage(cg, CGRectMake(0, 0, w, h), image);
    QLPreviewRequestFlushContext(preview, cg);
    CGImageRelease(image);
    CGContextRelease(cg);
    return 0;
  }

  void CancelPreviewGeneration(QLPreviewRequestRef preview) {
    // TODO
  }

};

static HRESULT Plugin_QueryInterface(void* p, REFIID iid, LPVOID* ppv)
{
  ASSERT(p);
  return reinterpret_cast<Plugin*>(p)->QueryInterface(iid, ppv);
}

static ULONG Plugin_AddRef(void* p)
{
  ASSERT(p);
  return reinterpret_cast<Plugin*>(p)->AddRef();
}

static ULONG Plugin_Release(void* p)
{
  ASSERT(p);
  return reinterpret_cast<Plugin*>(p)->Release();
}

static OSStatus Plugin_GenerateThumbnailForURL(void* p, QLThumbnailRequestRef thumbnail, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize)
{
  ASSERT(p);
  return reinterpret_cast<Plugin*>(p)->GenerateThumbnailForURL(thumbnail, url, contentTypeUTI, options, maxSize);
}

static void Plugin_CancelThumbnailGeneration(void* p, QLThumbnailRequestRef thumbnail)
{
  ASSERT(p);
  reinterpret_cast<Plugin*>(p)->CancelThumbnailGeneration(thumbnail);
}

static OSStatus Plugin_GeneratePreviewForURL(void* p, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
{
  ASSERT(p);
  return reinterpret_cast<Plugin*>(p)->GeneratePreviewForURL(preview, url, contentTypeUTI, options);
}

static void Plugin_CancelPreviewGeneration(void* p, QLPreviewRequestRef preview)
{
  ASSERT(p);
  reinterpret_cast<Plugin*>(p)->CancelPreviewGeneration(preview);
}

// This is the only public entry point of the framework/plugin (the
// "QuickLookGeneratorPluginFactory" name is specified in the
// Info.list file): the factory of objects. Similar than the Win32 COM
// IClassFactory::CreateInstance()
//
// This function is used to create an instance of an object of
// kQLGeneratorTypeID type, which should implement the
// QLGeneratorInterfaceStruct interface.
extern "C" void* QuickLookGeneratorPluginFactory(CFAllocatorRef allocator,
                                                 CFUUIDRef typeID)
{
  if (CFEqual(typeID, kQLGeneratorTypeID)) {
    CFUUIDRef uuid = CFUUIDCreateFromString(kCFAllocatorDefault, CFSTR(PLUGIN_ID));
    auto plugin = new Plugin(uuid);
    CFRelease(uuid);
    return plugin;
  }
  return nullptr;               // Unknown typeID
}
