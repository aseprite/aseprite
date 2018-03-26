# Desktop Integration
*Copyright (C) 2017-2018 David Capello*

> Distributed under [MIT license](LICENSE.txt)

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
