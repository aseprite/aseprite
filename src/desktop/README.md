# Desktop Integration

[![MIT Licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.txt)

## Windows

On Windows we have to create a COM server in a DLL to provide
thumbnails. The DLL must provide an
[IThumbnailProvider](https://msdn.microsoft.com/en-us/library/windows/desktop/bb774614.aspx)
implementation. Our implementation is in
[desktop::ThumbnailHandler](win/thumbnail_handler.h) class, the most
interesting member function is
[ThumbnailHandler::GetThumbnail()](win/thumbnail_handler.cpp),
which should return a `HBITMAP` of the thumbnail.

### Registering the DLL

If you distribute your app in an installer remember to use
`regsvr32.exe` to register your DLL which will call your DLL's
[DllRegisterServer()](win/dllmain.cpp), a function that must
create registry keys to associate your file type extension with
your thumbnail handler.

More information [in the MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/cc144118.aspx).

## macOS

On macOS we have to create a QuickLook plugin/extension to display
thumbnails and previews. The plugin is a `.qlgenerator` bundle which
can be installed in the `~/Library/QuickLook`,
`/System/Library/QuickLook`, or `/Library/QuickLook` directories, or
included the in the same app bundle.

[macOS pre-10.15](https://developer.apple.com/documentation/quicklook/previews_or_thumbnail_images_for_macos_10_14_or_earlier?language=objc),
has a COM-like functionality to load QuickLook plugins: the QuickLook
daemon will use the information in our bundle
[Info.plist](osx/Info.plist) to know which function to load and call
from our library to create the plugin object. (The function generally
is called `QuickLookGeneratorPluginFactory` but can has any name, the
name is specified in a child element of `CFPlugInFactories` inside the
`Info.plist` file.) Then the created object is used abstractly as a
`IUnknown`, and the `QLGeneratorInterface` interface is queried to
generate then thumbnails and previews.

We can test our `.qlgenerator` without installing it using the
[`qlmanage` utility](https://developer.apple.com/library/archive/documentation/UserExperience/Conceptual/Quicklook_Programming_Guide/Articles/QLDebugTest.html).

We target to macOS 10.9, but [we should migrate](https://developer.apple.com/videos/play/wwdc2019/719?time=944)
to the new macOS 10.15 API in a near future (or provide both).
