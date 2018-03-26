// SHE library
// Copyright (C) 2015-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/win/native_dialogs.h"

#include "base/fs.h"
#include "base/string.h"
#include "base/win/comptr.h"
#include "she/common/file_dialog.h"
#include "she/display.h"
#include "she/error.h"

#include <windows.h>
#include <shobjidl.h>

#include <string>
#include <vector>

namespace she {

// 32k is the limit for Win95/98/Me/NT4/2000/XP with ANSI version
#define FILENAME_BUFSIZE (1024*32)

class FileDialogWin32 : public CommonFileDialog {
public:
  FileDialogWin32()
    : m_filename(FILENAME_BUFSIZE)
    , m_defFilter(0) {
  }

  std::string fileName() override {
    return base::to_utf8(&m_filename[0]);
  }

  void getMultipleFileNames(base::paths& output) override {
    output = m_filenames;
  }

  void setFileName(const std::string& filename) override {
    wcscpy(&m_filename[0], base::from_utf8(base::get_file_name(filename)).c_str());
    m_initialDir = base::from_utf8(base::get_file_path(filename));
  }

  bool show(Display* parent) override {
    bool result = false;
    bool shown = false;

    HRESULT hr = showWithNewAPI(parent, result, shown);
    if (FAILED(hr) && !shown)
      hr = showWithOldAPI(parent, result);

    if (SUCCEEDED(hr))
      return result;

    return false;
  }

private:

  HRESULT showWithNewAPI(Display* parent, bool& result, bool& shown) {
    base::ComPtr<IFileDialog> dlg;
    HRESULT hr = CoCreateInstance(
      (m_type == Type::SaveFile ? CLSID_FileSaveDialog:
                                  CLSID_FileOpenDialog),
      nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg));
    if (FAILED(hr))
      return hr;

    FILEOPENDIALOGOPTIONS options =
      FOS_NOCHANGEDIR |
      FOS_PATHMUSTEXIST |
      FOS_FORCEFILESYSTEM;

    switch (m_type) {
      case Type::OpenFile:
        options |= FOS_FILEMUSTEXIST;
        break;
      case Type::OpenFiles:
        options |= FOS_FILEMUSTEXIST
          | FOS_ALLOWMULTISELECT;
        break;
      case Type::OpenFolder:
        options |= FOS_PICKFOLDERS;
        break;
      case Type::SaveFile:
        options |= FOS_OVERWRITEPROMPT;
        break;
    }

    hr = dlg->SetOptions(options);
    if (FAILED(hr))
      return hr;

    if (!m_title.empty()) {
      std::wstring title = base::from_utf8(m_title);
      hr = dlg->SetTitle(title.c_str());
      if (FAILED(hr))
        return hr;
    }

    if (std::wcslen(&m_filename[0]) > 0) {
      hr = dlg->SetFileName(&m_filename[0]);
      if (FAILED(hr))
        return hr;
    }

    if (std::wcslen(&m_initialDir[0]) > 0) {
      base::ComPtr<IShellItem> item;

      // The SHCreateItemFromParsingName() function is available since
      // Windows Vista in shell32.dll
      hr = ::SHCreateItemFromParsingName(&m_initialDir[0], nullptr,
                                         IID_PPV_ARGS(&item));
      if (FAILED(hr))
        return hr;

      if (item.get()) {
        hr = dlg->SetFolder(item.get());
        if (FAILED(hr))
          return hr;
      }
    }

    if (!m_defExtension.empty()) {
      std::wstring defExt = base::from_utf8(m_defExtension);
      hr = dlg->SetDefaultExtension(defExt.c_str());
      if (FAILED(hr))
        return hr;
    }

    if (m_type != Type::OpenFolder && !m_filters.empty()) {
      std::vector<COMDLG_FILTERSPEC> specs;
      getFiltersForIFileDialog(specs);
      hr = dlg->SetFileTypes(specs.size(), &specs[0]);
      freeFiltersForIFileDialog(specs);

      if (SUCCEEDED(hr))
        hr = dlg->SetFileTypeIndex(m_defFilter+1); // One-based index
      if (FAILED(hr))
        return hr;
    }

    hr = dlg->Show(parent ? (HWND)parent->nativeHandle(): nullptr);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
      shown = true;
      result = false;
      return S_OK;
    }
    if (FAILED(hr))
      return hr;
    shown = true;

    if (m_type == Type::OpenFiles) {
      base::ComPtr<IFileOpenDialog> odlg;
      hr = dlg->QueryInterface(IID_IFileOpenDialog, (void**)&odlg);
      base::ComPtr<IShellItemArray> items;
      hr = odlg->GetResults(&items);
      if (FAILED(hr))
        return hr;

      DWORD nitems = 0;
      hr = items->GetCount(&nitems);
      if (FAILED(hr))
        return hr;

      for (DWORD i=0; i<nitems; ++i) {
        base::ComPtr<IShellItem> item;
        hr = items->GetItemAt(i, &item);
        if (FAILED(hr))
          return hr;

        LPWSTR fn;
        hr = item->GetDisplayName(SIGDN_FILESYSPATH, &fn);
        if (SUCCEEDED(hr)) {
          m_filenames.push_back(base::to_utf8(fn));
          CoTaskMemFree(fn);
        }
      }
    }
    else {
      base::ComPtr<IShellItem> item;
      hr = dlg->GetResult(&item);
      if (FAILED(hr))
        return hr;

      LPWSTR fn;
      hr = item->GetDisplayName(SIGDN_FILESYSPATH, &fn);
      if (FAILED(hr))
        return hr;

      wcscpy(&m_filename[0], fn);
      m_filenames.push_back(base::to_utf8(&m_filename[0]));
      CoTaskMemFree(fn);
    }

    result = (hr == S_OK);
    return S_OK;
  }

  HRESULT showWithOldAPI(Display* parent, bool& result) {
    std::wstring title = base::from_utf8(m_title);
    std::wstring defExt = base::from_utf8(m_defExtension);
    std::wstring filtersWStr = getFiltersForGetOpenFileName();

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = (HWND)parent->nativeHandle();
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrFilter = filtersWStr.c_str();
    ofn.nFilterIndex = m_defFilter;
    ofn.lpstrFile = &m_filename[0];
    ofn.nMaxFile = FILENAME_BUFSIZE;
    if (!m_initialDir.empty())
      ofn.lpstrInitialDir = m_initialDir.c_str();
    ofn.lpstrTitle = title.c_str();
    ofn.lpstrDefExt = defExt.c_str();
    ofn.Flags =
      OFN_ENABLESIZING |
      OFN_EXPLORER |
      OFN_LONGNAMES |
      OFN_NOCHANGEDIR |
      OFN_PATHMUSTEXIST;

    if (m_type == Type::SaveFile) {
      ofn.Flags |= OFN_OVERWRITEPROMPT;
    }
    else {
      ofn.Flags |= OFN_FILEMUSTEXIST;
      if (m_type == Type::OpenFiles)
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }

    BOOL res;
    if (m_type == Type::SaveFile)
      res = GetSaveFileName(&ofn);
    else {
      res = GetOpenFileName(&ofn);
      if (res && m_type == Type::OpenFiles) {
        WCHAR* p = &m_filename[0];
        std::string path = base::to_utf8(p);

        for (p+=std::wcslen(p)+1; ; ++p) {
          if (*p) {
            WCHAR* q = p;
            for (++p; *p; ++p)
              ;

            m_filenames.push_back(
              base::join_path(path, base::to_utf8(q)));
          }
          else                  // Two null chars in a row
            break;
        }

        // Just one filename was selected
        if (m_filenames.empty())
          m_filenames.push_back(path);
      }
    }

    if (!res) {
      DWORD err = CommDlgExtendedError();
      if (err) {
        std::vector<char> buf(1024);
        sprintf(&buf[0], "Error using GetOpen/SaveFileName Win32 API. Code: %d", err);
        she::error_message(&buf[0]);
      }
    }

    result = (res != FALSE);
    return S_OK;
  }

  void getFiltersForIFileDialog(std::vector<COMDLG_FILTERSPEC>& specs) const {
    specs.resize(m_filters.size()+2);

    int i = 0, j = 0;
    specs[i].pszName = _wcsdup(L"All formats");
    std::wstring exts;
    bool first = true;
    for (const auto& filter : m_filters) {
      if (first)
        first = false;
      else
        exts.push_back(';');
      exts.append(L"*.");
      exts.append(base::from_utf8(filter.first));
    }
    specs[i].pszSpec = _wcsdup(exts.c_str());
    ++i;

    for (const auto& filter : m_filters) {
      specs[i].pszName = _wcsdup(base::from_utf8(filter.second).c_str());
      specs[i].pszSpec = _wcsdup(base::from_utf8("*." + filter.first).c_str());
      ++i;
    }

    specs[i].pszName = _wcsdup(L"All files");
    specs[i].pszSpec = _wcsdup(L"*.*");
    ++i;
  }

  void freeFiltersForIFileDialog(std::vector<COMDLG_FILTERSPEC>& specs) const {
    for (auto& spec : specs) {
      free((void*)spec.pszName);
      free((void*)spec.pszSpec);
    }
  }

  std::wstring getFiltersForGetOpenFileName() const {
    std::wstring filters;

    // A filter for all known types
    filters.append(L"All formats");
    filters.push_back('\0');
    bool first = true;
    for (const auto& filter : m_filters) {
      if (first)
        first = false;
      else
        filters.push_back(';');
      filters.append(L"*.");
      filters.append(base::from_utf8(filter.first));
    }
    filters.push_back('\0');

    // A specific filter for each type
    for (const auto& filter : m_filters) {
      filters.append(base::from_utf8(filter.second));
      filters.push_back('\0');
      filters.append(L"*.");
      filters.append(base::from_utf8(filter.first));
      filters.push_back('\0');
    }

    // A filter for all files
    filters.append(L"All files");
    filters.push_back('\0');
    filters.append(L"*.*");
    filters.push_back('\0');

    // End of filter string (two zeros at the end)
    filters.push_back('\0');
    return filters;
  }

  int m_defFilter;
  std::vector<WCHAR> m_filename;
  base::paths m_filenames;
  std::wstring m_initialDir;
};

NativeDialogsWin32::NativeDialogsWin32()
{
}

FileDialog* NativeDialogsWin32::createFileDialog()
{
  return new FileDialogWin32();
}

} // namespace she
