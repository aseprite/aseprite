// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

/* Some of the original code to handle PIDLs come from the
   MiniExplorer example of the Vaca library:
   https://github.com/dacap/vaca
   Copyright (C) by David Capello (MIT License)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file_system.h"

#include "base/fs.h"
#include "base/string.h"
#include "os/display.h"
#include "os/surface.h"
#include "os/system.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <utility>
#include <vector>

#ifdef _WIN32
  #include <windows.h>

  #include <shlobj.h>
  #include <shlwapi.h>

  #define MYPC_CSLID  "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
#else
  #include <dirent.h>
  #include <sys/stat.h>
#endif

//////////////////////////////////////////////////////////////////////

#ifndef MAX_PATH
  #define MAX_PATH 4096
#endif

#define NOTINITIALIZED  "{__not_initialized_path__}"

#define FS_TRACE(...)

namespace app {

// a position in the file-system
class FileItem : public IFileItem {
public:
  std::string m_keyname;
  std::string m_filename;
  std::string m_displayname;
  FileItem* m_parent;
  FileItemList m_children;
  unsigned int m_version;
  bool m_removed;
  bool m_is_folder;
#ifdef _WIN32
  LPITEMIDLIST m_pidl;            // relative to parent
  LPITEMIDLIST m_fullpidl;        // relative to the Desktop folder
                                  // (like a full path-name, because the
                                  // desktop is the root on Windows)
#endif

  FileItem(FileItem* parent);
  ~FileItem();

  void insertChildSorted(FileItem* child);
  int compare(const FileItem& that) const;

  bool operator<(const FileItem& that) const { return compare(that) < 0; }
  bool operator>(const FileItem& that) const { return compare(that) > 0; }
  bool operator==(const FileItem& that) const { return compare(that) == 0; }
  bool operator!=(const FileItem& that) const { return compare(that) != 0; }

  // IFileItem interface

  bool isFolder() const;
  bool isBrowsable() const;
  bool isHidden() const;

  std::string keyName() const;
  std::string fileName() const;
  std::string displayName() const;

  IFileItem* parent() const;
  const FileItemList& children();
  void createDirectory(const std::string& dirname);

  bool hasExtension(const base::paths& extensions);

  os::Surface* getThumbnail();
  void setThumbnail(os::Surface* thumbnail);

};

typedef std::map<std::string, FileItem*> FileItemMap;
typedef std::map<std::string, os::Surface*> ThumbnailMap;

// the root of the file-system
static FileItem* rootitem = NULL;
static FileItemMap* fileitems_map;
static ThumbnailMap* thumbnail_map;
static unsigned int current_file_system_version = 0;

#ifdef _WIN32
  static IMalloc* shl_imalloc = NULL;
  static IShellFolder* shl_idesktop = NULL;
#endif

// A more easy PIDLs interface (without using the SH* & IL* routines of W2K)
#ifdef _WIN32
  static bool is_sfgaof_folder(SFGAOF attrib);
  static void update_by_pidl(FileItem* fileitem, SFGAOF attrib);
  static LPITEMIDLIST concat_pidl(LPITEMIDLIST pidlHead, LPITEMIDLIST pidlTail);
  static UINT get_pidl_size(LPITEMIDLIST pidl);
  static LPITEMIDLIST get_next_pidl(LPITEMIDLIST pidl);
  static LPITEMIDLIST get_last_pidl(LPITEMIDLIST pidl);
  static LPITEMIDLIST clone_pidl(LPITEMIDLIST pidl);
  static LPITEMIDLIST remove_last_pidl(LPITEMIDLIST pidl);
  static void free_pidl(LPITEMIDLIST pidl);
  static std::string get_key_for_pidl(LPITEMIDLIST pidl);

  static FileItem* get_fileitem_by_fullpidl(LPITEMIDLIST pidl, bool create_if_not);
  static void put_fileitem(FileItem* fileitem);
#else
  static FileItem* get_fileitem_by_path(const std::string& path, bool create_if_not);
  static std::string remove_backslash_if_needed(const std::string& filename);
  static std::string get_key_for_filename(const std::string& filename);
  static void put_fileitem(FileItem* fileitem);
#endif

FileSystemModule* FileSystemModule::m_instance = NULL;

FileSystemModule::FileSystemModule()
{
  ASSERT(m_instance == NULL);
  m_instance = this;

  fileitems_map = new FileItemMap;
  thumbnail_map = new ThumbnailMap;

#ifdef _WIN32
  /* get the IMalloc interface */
  HRESULT hr = SHGetMalloc(&shl_imalloc);
  if (hr != S_OK)
    throw std::runtime_error("Error initializing file system. Report this problem. (SHGetMalloc failed.)");

  /* get desktop IShellFolder interface */
  hr = SHGetDesktopFolder(&shl_idesktop);
  if (hr != S_OK)
    throw std::runtime_error("Error initializing file system. Report this problem. (SHGetDesktopFolder failed.)");
#endif

  // first version of the file system
  ++current_file_system_version;

  // get the root element of the file system (this will create
  // the 'rootitem' FileItem)
  getRootFileItem();
}

FileSystemModule::~FileSystemModule()
{
  ASSERT(m_instance == this);

  for (FileItemMap::iterator
         it=fileitems_map->begin(); it!=fileitems_map->end(); ++it) {
    delete it->second;
  }
  fileitems_map->clear();

  for (ThumbnailMap::iterator
         it=thumbnail_map->begin(); it!=thumbnail_map->end(); ++it) {
    it->second->dispose();
  }
  thumbnail_map->clear();

#ifdef _WIN32
  // relase desktop IShellFolder interface
  shl_idesktop->Release();

  // release IMalloc interface
  shl_imalloc->Release();
  shl_imalloc = NULL;
#endif

  delete fileitems_map;
  delete thumbnail_map;

  m_instance = NULL;
}

FileSystemModule* FileSystemModule::instance()
{
  return m_instance;
}

void FileSystemModule::refresh()
{
  ++current_file_system_version;
}

IFileItem* FileSystemModule::getRootFileItem()
{
  FileItem* fileitem;

  if (rootitem)
    return rootitem;

  fileitem = new FileItem(NULL);
  rootitem = fileitem;

  //LOG("FS: Creating root fileitem %p\n", rootitem);

#ifdef _WIN32
  {
    // get the desktop PIDL
    LPITEMIDLIST pidl = NULL;

    if (SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl) != S_OK) {
      // TODO do something better
      ASSERT(false);
      exit(1);
    }
    fileitem->m_pidl = pidl;
    fileitem->m_fullpidl = pidl;

    SFGAOF attrib = SFGAO_FOLDER;
    shl_idesktop->GetAttributesOf(1, (LPCITEMIDLIST *)&pidl, &attrib);

    update_by_pidl(fileitem, attrib);
  }
#else
  {
    const char* root = "/";

    fileitem->m_filename = root;
    fileitem->m_displayname = root;
    fileitem->m_is_folder = true;
  }
#endif

  // insert the file-item in the hash-table
  put_fileitem(fileitem);
  return fileitem;
}

IFileItem* FileSystemModule::getFileItemFromPath(const std::string& path)
{
  IFileItem* fileitem = NULL;

  //LOG("FS: get_fileitem_from_path(%s)\n", path.c_str());

#ifdef _WIN32
  {
    ULONG cbEaten = 0UL;
    LPITEMIDLIST fullpidl = NULL;
    SFGAOF attrib = SFGAO_FOLDER;

    if (path.empty()) {
      fileitem = getRootFileItem();
      //LOG("FS: > %p (root)\n", fileitem);
      return fileitem;
    }

    if (shl_idesktop->ParseDisplayName
          (NULL, NULL,
           const_cast<LPWSTR>(base::from_utf8(path).c_str()),
           &cbEaten, &fullpidl, &attrib) != S_OK) {
      //LOG("FS: > (null)\n");
      return NULL;
    }

    fileitem = get_fileitem_by_fullpidl(fullpidl, true);
    free_pidl(fullpidl);
  }
#else
  {
    std::string buf = remove_backslash_if_needed(path);
    fileitem = get_fileitem_by_path(buf, true);
  }
#endif

  //LOG("FS: get_fileitem_from_path(%s) -> %p\n", path.c_str(), fileitem);

  return fileitem;
}

// ======================================================================
// FileItem class (IFileItem implementation)
// ======================================================================

bool FileItem::isFolder() const
{
  return m_is_folder;
}

bool FileItem::isBrowsable() const
{
  ASSERT(m_filename != NOTINITIALIZED);

  return m_is_folder;
}

bool FileItem::isHidden() const
{
  ASSERT(m_displayname != NOTINITIALIZED);

#ifdef _WIN32
  return false;
#else
  return m_displayname[0] == '.';
#endif
}

std::string FileItem::keyName() const
{
  ASSERT(m_keyname != NOTINITIALIZED);

  return m_keyname;
}

std::string FileItem::fileName() const
{
  ASSERT(m_filename != NOTINITIALIZED);

  return m_filename;
}

std::string FileItem::displayName() const
{
  ASSERT(m_displayname != NOTINITIALIZED);

  return m_displayname;
}

IFileItem* FileItem::parent() const
{
  if (this == rootitem)
    return NULL;
  else {
    ASSERT(m_parent);
    return m_parent;
  }
}

const FileItemList& FileItem::children()
{
  // Is the file-item a folder?
  if (isFolder() &&
      // if the children list is empty, or the file-system version
      // change (it's like to say: the current m_children list
      // is outdated)...
      (m_children.empty() ||
       current_file_system_version > m_version)) {
    FileItemList::iterator it;
    FileItem* child;

    // we have to mark current items as deprecated
    for (it=m_children.begin();
         it!=m_children.end(); ++it) {
      child = static_cast<FileItem*>(*it);
      child->m_removed = true;
    }

    //LOG("FS: Loading files for %p (%s)\n", fileitem, fileitem->displayname);
#ifdef _WIN32
    {
      IShellFolder* pFolder = NULL;
      HRESULT hr;

      if (this == rootitem)
        pFolder = shl_idesktop;
      else {
        hr = shl_idesktop->BindToObject(m_fullpidl,
          NULL, IID_IShellFolder, (LPVOID *)&pFolder);

        if (hr != S_OK)
          pFolder = NULL;
      }

      if (pFolder != NULL) {
        IEnumIDList *pEnum = NULL;
        ULONG c, fetched;

        /* get the interface to enumerate subitems */
        hr = pFolder->EnumObjects(reinterpret_cast<HWND>(os::instance()->defaultDisplay()->nativeHandle()),
          SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum);

        if (hr == S_OK && pEnum != NULL) {
          LPITEMIDLIST itempidl[256];
          SFGAOF attribs[256];

          /* enumerate the items in the folder */
          while (pEnum->Next(256, itempidl, &fetched) == S_OK && fetched > 0) {
            /* request the SFGAO_FOLDER attribute to know what of the
               item is a folder */
            for (c=0; c<fetched; ++c) {
              attribs[c] = SFGAO_FOLDER;
              pFolder->GetAttributesOf(1, (LPCITEMIDLIST *)itempidl, attribs+c);
            }

            /* generate the FileItems */
            for (c=0; c<fetched; ++c) {
              LPITEMIDLIST fullpidl = concat_pidl(m_fullpidl,
                                                  itempidl[c]);

              child = get_fileitem_by_fullpidl(fullpidl, false);
              if (!child) {
                child = new FileItem(this);

                child->m_pidl = itempidl[c];
                child->m_fullpidl = fullpidl;

                update_by_pidl(child, attribs[c]);
                put_fileitem(child);
              }
              else {
                ASSERT(child->m_parent == this);
                free_pidl(fullpidl);
                free_pidl(itempidl[c]);
              }

              insertChildSorted(child);
            }
          }

          pEnum->Release();
        }

        if (pFolder != shl_idesktop)
          pFolder->Release();
      }
    }
#else
    {
      DIR* dir = opendir(m_filename.c_str());
      if (dir) {
        dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
          FileItem* child;
          std::string fn = entry->d_name;
          std::string fullfn = base::join_path(m_filename, fn);

          if (fn == "." || fn == "..")
            continue;

          child = get_fileitem_by_path(fullfn, false);
          if (!child) {
            child = new FileItem(this);

            bool is_folder;
            struct stat fileStat;

            stat(fullfn.c_str(), &fileStat);

            if ((fileStat.st_mode & S_IFMT) == S_IFLNK) {
              is_folder = base::is_directory(fullfn);
            }
            else {
              is_folder = ((fileStat.st_mode & S_IFMT) == S_IFDIR);
            }

            child->m_filename = fullfn;
            child->m_displayname = fn;
            child->m_is_folder = is_folder;

            put_fileitem(child);
          }
          else {
            ASSERT(child->m_parent == this);
          }

          insertChildSorted(child);
        }
        closedir(dir);
      }
  }
#endif

    // check old file-items (maybe removed directories or file-items)
    for (it=m_children.begin();
         it!=m_children.end(); ) {
      child = static_cast<FileItem*>(*it);
      ASSERT(child != NULL);

      if (child && child->m_removed) {
        it = m_children.erase(it);

        fileitems_map->erase(fileitems_map->find(child->m_keyname));
        delete child;
      }
      else
        ++it;
    }

    // now this file-item is updated
    m_version = current_file_system_version;
  }

  return m_children;
}

void FileItem::createDirectory(const std::string& dirname)
{
  base::make_directory(base::join_path(m_filename, dirname));

  // Invalidate the children list.
  m_version = 0;
}

bool FileItem::hasExtension(const base::paths& extensions)
{
  ASSERT(m_filename != NOTINITIALIZED);

  return base::has_file_extension(m_filename, extensions);
}

os::Surface* FileItem::getThumbnail()
{
  ThumbnailMap::iterator it = thumbnail_map->find(m_filename);
  if (it != thumbnail_map->end())
    return it->second;
  else
    return NULL;
}

void FileItem::setThumbnail(os::Surface* thumbnail)
{
  // destroy the current thumbnail of the file (if exists)
  ThumbnailMap::iterator it = thumbnail_map->find(m_filename);
  if (it != thumbnail_map->end()) {
    it->second->dispose();
    thumbnail_map->erase(it);
  }

  // insert the new one in the map
  thumbnail_map->insert(std::make_pair(m_filename, thumbnail));
}

FileItem::FileItem(FileItem* parent)
{
  FS_TRACE("FS: Creating %p fileitem with parent %p\n", this, parent);

  m_keyname = NOTINITIALIZED;
  m_filename = NOTINITIALIZED;
  m_displayname = NOTINITIALIZED;
  m_parent = parent;
  m_version = current_file_system_version;
  m_removed = false;
  m_is_folder = false;
#ifdef _WIN32
  m_pidl = NULL;
  m_fullpidl = NULL;
#endif
}

FileItem::~FileItem()
{
  FS_TRACE("FS: Destroying FileItem() with parent %p\n", m_parent);

#ifdef _WIN32
  if (m_fullpidl && m_fullpidl != m_pidl) {
    free_pidl(m_fullpidl);
    m_fullpidl = NULL;
  }

  if (m_pidl) {
    free_pidl(m_pidl);
    m_pidl = NULL;
  }
#endif
}

void FileItem::insertChildSorted(FileItem* child)
{
  // this file-item wasn't removed from the last lookup
  child->m_removed = false;

  // if the fileitem is already in the list we can go back
  if (std::find(m_children.begin(), m_children.end(), child) != m_children.end())
    return;

  for (auto it=m_children.begin(), end=m_children.end(); it!=end; ++it) {
    if (*child < *static_cast<FileItem*>(*it)) {
      m_children.insert(it, child);
      return;
    }
  }

  m_children.push_back(child);
}

int FileItem::compare(const FileItem& that) const
{
  if (isFolder()) {
    if (!that.isFolder())
      return -1;
  }
  else if (that.isFolder())
    return 1;

  return base::compare_filenames(m_displayname, that.m_displayname);
}

//////////////////////////////////////////////////////////////////////
// PIDLS: Only for Win32
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32

static bool calc_is_folder(std::string filename, SFGAOF attrib)
{
  return ((attrib & SFGAO_FOLDER) == SFGAO_FOLDER)
    && (base::get_file_extension(filename) != "zip")
    && ((!filename.empty() && (*filename.begin()) != ':') || (filename == MYPC_CSLID));
}

// Updates the names of the file-item through its PIDL
static void update_by_pidl(FileItem* fileitem, SFGAOF attrib)
{
  STRRET strret;
  WCHAR pszName[MAX_PATH];
  IShellFolder* pFolder = NULL;
  HRESULT hr;

  if (fileitem == rootitem)
    pFolder = shl_idesktop;
  else {
    ASSERT(fileitem->m_parent);
    hr = shl_idesktop->BindToObject(fileitem->m_parent->m_fullpidl,
      NULL, IID_IShellFolder, (LPVOID *)&pFolder);
    if (hr != S_OK)
      pFolder = NULL;
  }

  // Get the file name

  if (pFolder != NULL &&
      pFolder->GetDisplayNameOf(fileitem->m_pidl,
                                SHGDN_NORMAL | SHGDN_FORPARSING,
                                &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->m_pidl, pszName, MAX_PATH);
    fileitem->m_filename = base::to_utf8(pszName);
  }
  else if (shl_idesktop->GetDisplayNameOf(fileitem->m_fullpidl,
                                          SHGDN_NORMAL | SHGDN_FORPARSING,
                                          &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->m_fullpidl, pszName, MAX_PATH);
    fileitem->m_filename = base::to_utf8(pszName);
  }
  else
    fileitem->m_filename = "ERR";

  // Is it a folder?

  fileitem->m_is_folder = calc_is_folder(fileitem->m_filename, attrib);

  // Get the name to display

  if (fileitem->isFolder() &&
      pFolder &&
      pFolder->GetDisplayNameOf(fileitem->m_pidl,
                                SHGDN_INFOLDER,
                                &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->m_pidl, pszName, MAX_PATH);
    fileitem->m_displayname = base::to_utf8(pszName);
  }
  else if (fileitem->isFolder() &&
           shl_idesktop->GetDisplayNameOf(fileitem->m_fullpidl,
                                          SHGDN_INFOLDER,
                                          &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->m_fullpidl, pszName, MAX_PATH);
    fileitem->m_displayname = base::to_utf8(pszName);
  }
  else {
    fileitem->m_displayname = base::get_file_name(fileitem->m_filename);
  }

  if (pFolder != NULL && pFolder != shl_idesktop) {
    pFolder->Release();
  }
}

static LPITEMIDLIST concat_pidl(LPITEMIDLIST pidlHead, LPITEMIDLIST pidlTail)
{
  LPITEMIDLIST pidlNew;
  UINT cb1, cb2;

  ASSERT(pidlHead);
  ASSERT(pidlTail);

  cb1 = get_pidl_size(pidlHead) - sizeof(pidlHead->mkid.cb);
  cb2 = get_pidl_size(pidlTail);

  pidlNew = (LPITEMIDLIST)shl_imalloc->Alloc(cb1 + cb2);
  if (pidlNew) {
    CopyMemory(pidlNew, pidlHead, cb1);
    CopyMemory(((LPSTR)pidlNew) + cb1, pidlTail, cb2);
  }

  return pidlNew;
}

static UINT get_pidl_size(LPITEMIDLIST pidl)
{
  UINT cbTotal = 0;

  if (pidl) {
    cbTotal += sizeof(pidl->mkid.cb); /* null terminator */

    while (pidl) {
      cbTotal += pidl->mkid.cb;
      pidl = get_next_pidl(pidl);
    }
  }

  return cbTotal;
}

static LPITEMIDLIST get_next_pidl(LPITEMIDLIST pidl)
{
  if (pidl != NULL && pidl->mkid.cb > 0) {
    pidl = (LPITEMIDLIST)(((LPBYTE)(pidl)) + pidl->mkid.cb);
    if (pidl->mkid.cb > 0)
      return pidl;
  }

  return NULL;
}

static LPITEMIDLIST get_last_pidl(LPITEMIDLIST pidl)
{
  LPITEMIDLIST pidlLast = pidl;
  LPITEMIDLIST pidlNew = NULL;

  while (pidl) {
    pidlLast = pidl;
    pidl = get_next_pidl(pidl);
  }

  if (pidlLast) {
    ULONG sz = get_pidl_size(pidlLast);
    pidlNew = (LPITEMIDLIST)shl_imalloc->Alloc(sz);
    CopyMemory(pidlNew, pidlLast, sz);
  }

  return pidlNew;
}

static LPITEMIDLIST clone_pidl(LPITEMIDLIST pidl)
{
  ULONG sz = get_pidl_size(pidl);
  LPITEMIDLIST pidlNew = (LPITEMIDLIST)shl_imalloc->Alloc(sz);

  CopyMemory(pidlNew, pidl, sz);

  return pidlNew;
}

static LPITEMIDLIST remove_last_pidl(LPITEMIDLIST pidl)
{
  LPITEMIDLIST pidlFirst = pidl;
  LPITEMIDLIST pidlLast = pidl;

  while (pidl) {
    pidlLast = pidl;
    pidl = get_next_pidl(pidl);
  }

  if (pidlLast)
    pidlLast->mkid.cb = 0;

  return pidlFirst;
}

static void free_pidl(LPITEMIDLIST pidl)
{
  shl_imalloc->Free(pidl);
}

static std::string get_key_for_pidl(LPITEMIDLIST pidl)
{
#if 0
  char *key = base_malloc(get_pidl_size(pidl)+1);
  UINT c, i = 0;

  while (pidl) {
    for (c=0; c<pidl->mkid.cb; ++c) {
      if (pidl->mkid.abID[c])
        key[i++] = pidl->mkid.abID[c];
      else
        key[i++] = 1;
    }
    pidl = get_next_pidl(pidl);
  }
  key[i] = 0;

  return key;
#else
  STRRET strret;
  WCHAR pszName[MAX_PATH];
  WCHAR key[4096] = { 0 };
  int len;

  // Go pidl by pidl from the fullpidl to the root (desktop)
  //LOG("FS: ***\n");
  pidl = clone_pidl(pidl);
  while (pidl->mkid.cb > 0) {
    if (shl_idesktop->GetDisplayNameOf(pidl,
                                       SHGDN_INFOLDER | SHGDN_FORPARSING,
                                       &strret) == S_OK) {
      if (StrRetToBuf(&strret, pidl, pszName, MAX_PATH) != S_OK)
        pszName[0] = 0;

      //LOG("FS: + %s\n", pszName);

      len = wcslen(pszName);
      if (len > 0) {
        if (*key) {
          if (pszName[len-1] != L'\\') {
            memmove(key+len+1, key, sizeof(WCHAR)*(wcslen(key)+1));
            key[len] = L'\\';
          }
          else
            memmove(key+len, key, sizeof(WCHAR)*(wcslen(key)+1));
        }
        else
          key[len] = 0;

        memcpy(key, pszName, sizeof(WCHAR)*len);
      }
    }
    remove_last_pidl(pidl);
  }
  free_pidl(pidl);

  //LOG("FS: =%s\n***\n", key);
  return base::to_utf8(key);
#endif
}

static FileItem* get_fileitem_by_fullpidl(LPITEMIDLIST fullpidl, bool create_if_not)
{
  FileItemMap::iterator it = fileitems_map->find(get_key_for_pidl(fullpidl));
  if (it != fileitems_map->end())
    return it->second;

  if (!create_if_not)
    return NULL;

  // new file-item
  FileItem* fileitem = new FileItem(NULL);
  fileitem->m_fullpidl = clone_pidl(fullpidl);

  SFGAOF attrib = SFGAO_FOLDER;
  HRESULT hr = shl_idesktop->GetAttributesOf(1, (LPCITEMIDLIST *)&fileitem->m_fullpidl, &attrib);
  if (hr == S_OK) {
    LPITEMIDLIST parent_fullpidl = clone_pidl(fileitem->m_fullpidl);
    remove_last_pidl(parent_fullpidl);

    fileitem->m_pidl = get_last_pidl(fileitem->m_fullpidl);
    fileitem->m_parent = get_fileitem_by_fullpidl(parent_fullpidl, true);

    free_pidl(parent_fullpidl);
  }

  update_by_pidl(fileitem, attrib);
  put_fileitem(fileitem);

  //LOG("FS: fileitem %p created %s with parent %p\n", fileitem, fileitem->keyname.c_str(), fileitem->parent);

  return fileitem;
}

/**
 * Inserts the @a fileitem in the hash map of items.
 */
static void put_fileitem(FileItem* fileitem)
{
  ASSERT(fileitem->m_filename != NOTINITIALIZED);
  ASSERT(fileitem->m_keyname == NOTINITIALIZED);

  fileitem->m_keyname = get_key_for_pidl(fileitem->m_fullpidl);

  ASSERT(fileitem->m_keyname != NOTINITIALIZED);

#ifdef _DEBUG
  auto it = fileitems_map->find(get_key_for_pidl(fileitem->m_fullpidl));
  ASSERT(it == fileitems_map->end());
#endif

  // insert this file-item in the hash-table
  fileitems_map->insert(std::make_pair(fileitem->m_keyname, fileitem));
}

#else

//////////////////////////////////////////////////////////////////////
// POSIX functions
//////////////////////////////////////////////////////////////////////

static FileItem* get_fileitem_by_path(const std::string& path, bool create_if_not)
{
  if (path.empty())
    return rootitem;

  FileItemMap::iterator it = fileitems_map->find(get_key_for_filename(path));
  if (it != fileitems_map->end())
    return it->second;

  if (!create_if_not)
    return NULL;

  // get the attributes of the file
  bool is_folder = false;
  if (!base::is_file(path)) {
    if (!base::is_directory(path))
      return NULL;

    is_folder = true;
  }

  // new file-item
  FileItem* fileitem = new FileItem(NULL);

  fileitem->m_filename = path;
  fileitem->m_displayname = base::get_file_name(path);
  fileitem->m_is_folder = is_folder;

  // get the parent
  {
    std::string parent_path = remove_backslash_if_needed(base::join_path(base::get_file_path(path), ""));
    fileitem->m_parent = get_fileitem_by_path(parent_path, true);
  }

  put_fileitem(fileitem);

  return fileitem;
}

static std::string remove_backslash_if_needed(const std::string& filename)
{
  if (!filename.empty() && base::is_path_separator(*(filename.end()-1))) {
    int len = filename.size();

    // This is just the root '/' slash
    if (len == 1)
      return filename;
    else
      return base::remove_path_separator(filename);
  }
  return filename;
}

static std::string get_key_for_filename(const std::string& filename)
{
  std::string buf(filename);
  buf = base::fix_path_separators(buf);
  return buf;
}

static void put_fileitem(FileItem* fileitem)
{
  ASSERT(fileitem->m_filename != NOTINITIALIZED);
  ASSERT(fileitem->m_keyname == NOTINITIALIZED);

  fileitem->m_keyname = get_key_for_filename(fileitem->m_filename);

  ASSERT(fileitem->m_keyname != NOTINITIALIZED);

  // insert this file-item in the hash-table
  fileitems_map->insert(std::make_pair(fileitem->m_keyname, fileitem));
}

#endif

} // namespace app
