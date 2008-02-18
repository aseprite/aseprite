/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Some of the original code to handle PIDLs come from the
   MiniExplorer example of the Vaca library:
   http://vaca.sourceforge.net/
 */

#include "config.h"

#include <stdio.h>
#include <assert.h>
#include <allegro.h>

/* in Windows we can use PIDLS */
#if defined ALLEGRO_WINDOWS
  /* uncomment this if you don't want to use PIDLs in windows */
  #define USE_PIDLS
#endif

#if defined ALLEGRO_UNIX || defined ALLEGRO_DJGPP || defined ALLEGRO_MINGW32
  #include <sys/stat.h>
#endif
#if defined ALLEGRO_UNIX || defined ALLEGRO_MINGW32
  #include <sys/unistd.h>
#endif

#if defined USE_PIDLS
  #define COBJMACROS
  #include <winalleg.h>
  #include <shlobj.h>
  #include <shlwapi.h>
#endif

#include "jinete/jlist.h"
#include "jinete/jfilesel.h"

#include "core/file_system.h"
#include "util/hash.h"

/********************************************************************/

#ifdef USE_PIDLS
  /* ..using Win32 Shell (PIDLs) */

  #define IS_FOLDER(fi)					\
    (((fi)->attrib & SFGAO_FOLDER) == SFGAO_FOLDER)

  #define MYPC_CSLID  "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"

#else
  /* ..using Allegro (for_each_file) */

  #define IS_FOLDER(fi)					\
    (((fi)->attrib & FA_DIREC) == FA_DIREC)

  #if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')
    #define HAVE_DRIVES
  #else
    #define CASE_SENSITIVE
  #endif

  #ifndef FA_ALL
    #define FA_ALL     FA_RDONLY | FA_DIREC | FA_ARCH | FA_HIDDEN | FA_SYSTEM
  #endif
  #define FA_TO_SHOW   FA_RDONLY | FA_DIREC | FA_ARCH | FA_SYSTEM

#endif

/********************************************************************/

#ifndef MAX_PATH
#  define MAX_PATH 4096
#endif

/* a position in the file-system */
struct FileItem
{
  char *keyname;
  char *filename;
  char *displayname;
  FileItem *parent;
  JList children;
  int last_update;
#ifdef USE_PIDLS
  LPITEMIDLIST pidl;		/* relative to parent */
  LPITEMIDLIST fullpidl;	/* relative to the Desktop folder
				   (like a full path-name, because the
				   desktop is the root on Windows) */
  SFGAOF attrib;
#else
  int attrib;
#endif
};

/* the root of the file-system */
static FileItem *rootitem = NULL;
static HashTable *hash_fileitems = NULL;

/* hash-table for thumbnails */
static HashTable *hash_thumbnail = NULL;

#ifdef USE_PIDLS
  static LPMALLOC shl_imalloc = NULL;
  static IShellFolder *shl_idesktop = NULL;
#endif

/* local auxiliary routines */
static FileItem *fileitem_new(FileItem *parent);
static void fileitem_free(FileItem *fileitem);
static void fileitem_insert_child_sorted(FileItem *fileitem, FileItem *child);
static int fileitem_cmp(FileItem *fi1, FileItem *fi2);

/* a more easy PIDLs interface (without using the SH* & IL* routines of W2K) */
#ifdef USE_PIDLS
  static void update_by_pidl(FileItem *fileitem);
  static LPITEMIDLIST concat_pidl(LPITEMIDLIST pidlHead, LPITEMIDLIST pidlTail);
  static UINT get_pidl_size(LPITEMIDLIST pidl);
  static LPITEMIDLIST get_next_pidl(LPITEMIDLIST pidl);
  static LPITEMIDLIST get_last_pidl(LPITEMIDLIST pidl);
  static LPITEMIDLIST clone_pidl(LPITEMIDLIST pidl);
  static LPITEMIDLIST remove_last_pidl(LPITEMIDLIST pidl);
  static void free_pidl(LPITEMIDLIST pidl);
  static char *get_key_for_pidl(LPITEMIDLIST pidl);

  static FileItem *get_fileitem_by_fullpidl(LPITEMIDLIST pidl, bool create_if_not);
  static void put_fileitem(FileItem *fileitem);
#else
  static FileItem *get_fileitem_by_path(const char *path, bool create_if_not);
  static void for_each_child_callback(const char *filename, int attrib, int param);
  static char *remove_backslash(char *filename);
  static char *get_key_for_filename(const char *filename);
  static void put_fileitem(FileItem *fileitem);
#endif

/**
 * Initializes the file-system module to navigate the file-system.
 */
bool file_system_init(void)
{
#ifdef USE_PIDLS
  /* get the IMalloc interface */
  SHGetMalloc(&shl_imalloc);

  /* get desktop IShellFolder interface */
  SHGetDesktopFolder(&shl_idesktop);
#endif

  hash_fileitems = hash_new(512);
  hash_thumbnail = hash_new(512);
  get_root_fileitem();

  return TRUE;
}
 
/**
 * Shutdowns the file-system module.
 */
void file_system_exit(void)
{
  if (rootitem)
    fileitem_free(rootitem);

  if (hash_fileitems)
    hash_free(hash_fileitems, NULL);

  if (hash_thumbnail)
    hash_free(hash_thumbnail, (void *)destroy_bitmap);

#ifdef USE_PIDLS
  /* relase desktop IShellFolder interface */
  IShellFolder_Release(shl_idesktop);
    
  /* release IMalloc interface */
  IMalloc_Release(shl_imalloc);
  shl_imalloc = NULL;
#endif
}

FileItem *get_root_fileitem(void)
{
  FileItem *fileitem;

  if (rootitem)
    return rootitem;

  fileitem = fileitem_new(NULL);
  if (!fileitem)
    return NULL;

  rootitem = fileitem;

#ifdef USE_PIDLS
  {
    /* get the desktop PIDL */
    LPITEMIDLIST pidl = NULL;

    if (SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl) != S_OK) {
      /* TODO do something better */
      assert(FALSE);
      exit(1);
    }
    fileitem->pidl = pidl;
    fileitem->fullpidl = pidl;
    fileitem->attrib = SFGAO_FOLDER;
    IShellFolder_GetAttributesOf(shl_idesktop, 1,
				 (LPCITEMIDLIST *)&pidl,
				 &fileitem->attrib);

    update_by_pidl(fileitem);
  }
#else
  {
    char buf[MAX_PATH];

#if defined HAVE_DRIVES
    ustrcpy(buf, "C:\\");
#else
    ustrcpy(buf, "/");
#endif

    fileitem->filename = jstrdup(buf);
    fileitem->displayname = jstrdup(buf);
    fileitem->attrib = FA_DIREC;
  }
#endif

  /* insert the file-item in the hash-table */
  put_fileitem(fileitem);
  return fileitem;
}

FileItem *get_fileitem_from_path(const char *path)
{
  FileItem *fileitem = NULL;

#ifdef USE_PIDLS
  {
    ULONG cbEaten;
    WCHAR wStr[MAX_PATH];
    LPITEMIDLIST fullpidl = NULL;
    SFGAOF attrib = SFGAO_FOLDER;

    if (*path == 0)
      return get_root_fileitem();

    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, path, ustrlen(path)+1, wStr, MAX_PATH);
    if (IShellFolder_ParseDisplayName(shl_idesktop,
				      NULL, NULL,
				      wStr, &cbEaten,
				      &fullpidl,
				      &attrib) != S_OK) {
      return NULL;
    }

    fileitem = get_fileitem_by_fullpidl(fullpidl, TRUE);
    free_pidl(fullpidl);
  }
#else
  {
    char buf[MAX_PATH];

    /* return HOME location */
    
    ustrcpy(buf, path);
    remove_backslash(buf);
    fileitem = get_fileitem_by_path(buf, TRUE);
  }
#endif

  return fileitem;
}

bool fileitem_is_folder(FileItem *fileitem)
{
  assert(fileitem != NULL);

  return IS_FOLDER(fileitem);
}

bool fileitem_is_browsable(FileItem *fileitem)
{
  assert(fileitem != NULL);
  assert(fileitem->filename != NULL);

#ifdef USE_PIDLS
  return IS_FOLDER(fileitem)
    && (ustrcmp(get_extension(fileitem->filename), "zip") != 0)
    && (fileitem->filename[0] != ':' ||
	ustrcmp(fileitem->filename, MYPC_CSLID) == 0);
#else
  return IS_FOLDER(fileitem);
#endif
}

const char *fileitem_get_keyname(FileItem *fileitem)
{
  assert(fileitem != NULL);
  assert(fileitem->keyname != NULL);

  return fileitem->keyname;
}

const char *fileitem_get_filename(FileItem *fileitem)
{
  assert(fileitem != NULL);
  assert(fileitem->filename != NULL);

  return fileitem->filename;
}

const char *fileitem_get_displayname(FileItem *fileitem)
{
  assert(fileitem != NULL);
  assert(fileitem->displayname != NULL);

  return fileitem->displayname;
}

FileItem *fileitem_get_parent(FileItem *fileitem)
{
  assert(fileitem != NULL);

  if (fileitem == rootitem)
    return NULL;
  else {
    assert(fileitem->parent != NULL);
    return fileitem->parent;
  }
}

JList fileitem_get_children(FileItem *fileitem)
{
  assert(fileitem != NULL);

  if (!fileitem->children && IS_FOLDER(fileitem)) {
    fileitem->children = jlist_new();

/*     printf("Loading files for %p (%s)\n", fileitem, fileitem->displayname); fflush(stdout); */
#ifdef USE_PIDLS
    {
      IShellFolder *pFolder = NULL;

      if (fileitem == rootitem)
	pFolder = shl_idesktop;
      else
	IShellFolder_BindToObject(shl_idesktop,
				  fileitem->fullpidl,
				  NULL,
				  &IID_IShellFolder,
				  (LPVOID *)&pFolder);

      if (pFolder != NULL) {
	IEnumIDList *pEnum = NULL;
	ULONG c, fetched;

	/* get the interface to enumerate subitems */
	IShellFolder_EnumObjects(pFolder, win_get_window(),
				 SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum);

	if (pEnum != NULL) {
	  LPITEMIDLIST itempidl[256];
	  SFGAOF attribs[256];

	  /* enumerate the items in the folder */
	  while (IEnumIDList_Next(pEnum, 256, itempidl, &fetched) == S_OK &&
		 fetched > 0) {
	    /* request the SFGAO_FOLDER attribute to know what of the
	       item is a folder */
	    for (c=0; c<fetched; ++c)
	      attribs[c] = SFGAO_FOLDER;

	    if (IShellFolder_GetAttributesOf(pFolder, fetched,
					     (LPCITEMIDLIST *)itempidl, attribs) != S_OK) {
	      for (c=0; c<fetched; ++c)
		attribs[c] = 0;
	    }

	    /* generate the FileItems */
	    for (c=0; c<fetched; ++c) {
	      FileItem *child;
	      LPITEMIDLIST fullpidl = concat_pidl(fileitem->fullpidl,
						  itempidl[c]);

	      child = get_fileitem_by_fullpidl(fullpidl, FALSE);
	      if (!child) {
		child = fileitem_new(fileitem);

		child->pidl = itempidl[c];
		child->fullpidl = fullpidl;
		child->attrib = attribs[c];

		update_by_pidl(child);
		put_fileitem(child);
	      }
	      else {
		assert(child->parent == fileitem);
		free_pidl(fullpidl);
		free_pidl(itempidl[c]);
	      }

	      fileitem_insert_child_sorted(fileitem, child);
	    }
	  }

	  IEnumIDList_Release(pEnum);
	}

	if (pFolder != shl_idesktop)
	  IShellFolder_Release(pFolder);
      }
    }
#else
    {
      char buf[MAX_PATH], path[MAX_PATH], tmp[32];

      ustrcpy(path, fileitem->filename);
      put_backslash(path);

      replace_filename(buf,
		       path,
		       uconvert_ascii("*.*", tmp),
		       sizeof(buf));

      for_each_file(buf, FA_TO_SHOW,
		    for_each_child_callback,
		    (int)fileitem);	/* TODO warning with 64bits */
    }
#endif
  }

  return fileitem->children;
}

bool filename_has_extension(const char *filename, const char *list_of_extensions)
{
  const char *extension = get_extension(filename);
  bool ret = FALSE;
  if (extension) {
    char *extdup = ustrlwr(jstrdup(extension));
    int extsz = ustrlen(extdup);
    char *p = ustrstr(list_of_extensions, extdup);
    if ((p != NULL) &&
	(p[extsz] == 0 || p[extsz] == ',') &&
	(p == list_of_extensions || *(p-1) == ',')) {
      ret = TRUE;
    }
    jfree(extdup);
  }
  return ret;
}

bool fileitem_has_extension(FileItem *fileitem, const char *list_of_extensions)
{
  return filename_has_extension(fileitem_get_filename(fileitem),
				list_of_extensions);
}

BITMAP *fileitem_get_thumbnail(FileItem *fileitem)
{
  assert(fileitem != NULL);

  return hash_lookup(hash_thumbnail, fileitem->filename);
}

void fileitem_set_thumbnail(FileItem *fileitem, BITMAP *thumbnail)
{
  BITMAP *current_thumbnail;

  assert(fileitem != NULL);

  current_thumbnail = hash_lookup(hash_thumbnail, fileitem->filename);
  if (current_thumbnail) {
    destroy_bitmap(current_thumbnail);
    hash_remove(hash_thumbnail, fileitem->filename);
  }

  hash_insert(hash_thumbnail, fileitem->filename, thumbnail);
}

static FileItem *fileitem_new(FileItem *parent)
{
  FileItem *fileitem = jnew(FileItem, 1);
  if (!fileitem)
    return NULL;

  fileitem->keyname = NULL;
  fileitem->filename = NULL;
  fileitem->displayname = NULL;
  fileitem->parent = parent;
  fileitem->children = NULL;
  fileitem->last_update = -1;
#ifdef USE_PIDLS
  fileitem->pidl = NULL;
  fileitem->fullpidl = NULL;
  fileitem->attrib = 0;
#else
  fileitem->attrib = 0;
#endif

  return fileitem;
}

static void fileitem_free(FileItem *fileitem)
{
  assert(fileitem != NULL);

#ifdef USE_PIDLS
  if (fileitem->fullpidl &&
      fileitem->fullpidl != fileitem->pidl) {
    free_pidl(fileitem->fullpidl);
    fileitem->fullpidl = NULL;
  }

  if (fileitem->pidl) {
    free_pidl(fileitem->pidl);
    fileitem->pidl = NULL;
  }
#endif

  /* this isn't neccessary (if fileitem_free is called with the
                            root-item at the first time)...

     if (fileitem->parent)
       jlist_remove(fileitem->parent->children, fileitem);
  */

  if (fileitem->keyname)
    jfree(fileitem->keyname);

  if (fileitem->filename)
    jfree(fileitem->filename);

  if (fileitem->displayname)
    jfree(fileitem->displayname);

  if (fileitem->children) {
    JLink link, next;
    JI_LIST_FOR_EACH_SAFE(fileitem->children, link, next) {
      fileitem_free(link->data);
    }
    jlist_free(fileitem->children);
  }

  jfree(fileitem);
}

static void fileitem_insert_child_sorted(FileItem *fileitem, FileItem *child)
{
  JLink link;
  bool inserted = FALSE;
  JI_LIST_FOR_EACH(fileitem->children, link) {
    if (fileitem_cmp((FileItem *)link->data, child) > 0) {
      jlist_insert_before(fileitem->children, link, child);
      inserted = TRUE;
      break;
    }
  }
  if (!inserted)
    jlist_append(fileitem->children, child);
}

/* fileitem_cmp:
 *  ustricmp for filenames: makes sure that eg "foo.bar" comes before
 *  "foo-1.bar", and also that "foo9.bar" comes before "foo10.bar".
 */
static int fileitem_cmp(FileItem *fi1, FileItem *fi2)
{
  if (IS_FOLDER(fi1)) {
    if (!IS_FOLDER(fi2))
      return -1;
  }
  else if (IS_FOLDER(fi2))
    return 1;

#ifndef USE_PIDLS
  {
    int c1, c2;
    int x1, x2;
    char *t1, *t2;
    const char *s1 = fi1->displayname;
    const char *s2 = fi2->displayname;

    for (;;) {
      c1 = utolower(ugetxc(&s1));
      c2 = utolower(ugetxc(&s2));

      if ((c1 >= '0') && (c1 <= '9') && (c2 >= '0') && (c2 <= '9')) {
	x1 = ustrtol(s1 - ucwidth(c1), &t1, 10);
	x2 = ustrtol(s2 - ucwidth(c2), &t2, 10);
	if (x1 != x2)
	  return x1 - x2;
	else if (t1 - s1 != t2 - s2)
	  return (t2 - s2) - (t1 - s1);
	s1 = t1;
	s2 = t2;
      }
      else if (c1 != c2) {
	if (!c1)
	  return -1;
	else if (!c2)
	  return 1;
	else if (c1 == '.')
	  return -1;
	else if (c2 == '.')
	  return 1;
	return c1 - c2;
      }

      if (!c1)
	return 0;
    }
  }
#endif

  return -1;
}

/********************************************************************/
/* PIDLS: Only for Win32                                            */
/********************************************************************/

#ifdef USE_PIDLS

/* updates the names of the file-item through its PIDL */
static void update_by_pidl(FileItem *fileitem)
{
  STRRET strret;
  TCHAR pszName[MAX_PATH];
  IShellFolder *pFolder = NULL;

  if (fileitem->filename) {
    jfree(fileitem->filename);
    fileitem->filename = NULL;
  }

  if (fileitem->displayname) {
    jfree(fileitem->displayname);
    fileitem->displayname = NULL;
  }

  if (fileitem == rootitem)
    pFolder = shl_idesktop;
  else {
    assert(fileitem->parent != NULL);
    IShellFolder_BindToObject(shl_idesktop,
			      fileitem->parent->fullpidl,
			      NULL,
			      &IID_IShellFolder,
			      (LPVOID *)&pFolder);
  }

  /****************************************/
  /* get the file name */

  if (pFolder &&
      IShellFolder_GetDisplayNameOf(pFolder,
				    fileitem->pidl,
				    SHGDN_NORMAL | SHGDN_FORPARSING,
				    &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->pidl, pszName, MAX_PATH);
    fileitem->filename = jstrdup(pszName);
  }
  else if (IShellFolder_GetDisplayNameOf(shl_idesktop,
					 fileitem->fullpidl,
					 SHGDN_NORMAL | SHGDN_FORPARSING,
					 &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->fullpidl, pszName, MAX_PATH);
    fileitem->filename = jstrdup(pszName);
  }
  else
    fileitem->filename = jstrdup("ERR");


  /****************************************/
  /* get the name to display */

  if (pFolder &&
      IShellFolder_GetDisplayNameOf(pFolder,
				    fileitem->pidl,
				    SHGDN_INFOLDER,
				    &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->pidl, pszName, MAX_PATH);
    fileitem->displayname = jstrdup(pszName);
  }
  else if (IShellFolder_GetDisplayNameOf(shl_idesktop,
					 fileitem->fullpidl,
					 SHGDN_INFOLDER,
					 &strret) == S_OK) {
    StrRetToBuf(&strret, fileitem->fullpidl, pszName, MAX_PATH);
    fileitem->displayname = jstrdup(pszName);
  }
  else
    fileitem->displayname = jstrdup("ERR");

  if (pFolder && pFolder != shl_idesktop) {
    IShellFolder_Release(pFolder);
  }
}

static LPITEMIDLIST concat_pidl(LPITEMIDLIST pidlHead, LPITEMIDLIST pidlTail)
{
  LPITEMIDLIST pidlNew;
  UINT cb1, cb2;

  assert(pidlHead != NULL);
  assert(pidlTail != NULL);

  cb1 = get_pidl_size(pidlHead) - sizeof(pidlHead->mkid.cb);
  cb2 = get_pidl_size(pidlTail);

  pidlNew = (LPITEMIDLIST)IMalloc_Alloc(shl_imalloc, cb1 + cb2);
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
    pidlNew = (LPITEMIDLIST)IMalloc_Alloc(shl_imalloc, sz);
    CopyMemory(pidlNew, pidlLast, sz);
  }

  return pidlNew;
}

static LPITEMIDLIST clone_pidl(LPITEMIDLIST pidl)
{
  ULONG sz = get_pidl_size(pidl);
  LPITEMIDLIST pidlNew = (LPITEMIDLIST)IMalloc_Alloc(shl_imalloc, sz);

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
  IMalloc_Free(shl_imalloc, pidl);
}

static char *get_key_for_pidl(LPITEMIDLIST pidl)
{
#if 0
  char *key = jmalloc(get_pidl_size(pidl)+1);
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
  TCHAR pszName[MAX_PATH];
  char key[4096];
  int len;

  ustrcpy(key, empty_string);

  /* go pidl by pidl from the fullpidl to the root (desktop) */
/*   printf("***\n"); fflush(stdout); */
  pidl = clone_pidl(pidl);
  while (pidl->mkid.cb > 0) {
    if (IShellFolder_GetDisplayNameOf(shl_idesktop,
				      pidl,
				      SHGDN_INFOLDER | SHGDN_FORPARSING,
				      &strret) == S_OK) {
      StrRetToBuf(&strret, pidl, pszName, MAX_PATH);

/*       printf("+ %s\n", pszName); fflush(stdout); */

      len = ustrlen(pszName);
      if (len > 0 && ustrncmp(key, pszName, len) != 0) {
	if (*key) {
	  if (pszName[len-1] != '\\') {
	    memmove(key+len+1, key, ustrlen(key)+1);
	    key[len] = '\\';
	  }
	  else
	    memmove(key+len, key, ustrlen(key)+1);
	}
	else
	  key[len] = 0;

	memcpy(key, pszName, len);
      }
    }
    remove_last_pidl(pidl);
  }
  free_pidl(pidl);

/*   printf("=%s\n***\n", key); fflush(stdout); */
  return jstrdup(key);
#endif
}

static FileItem *get_fileitem_by_fullpidl(LPITEMIDLIST fullpidl, bool create_if_not)
{
  char *key;
  FileItem *fileitem;

  key = get_key_for_pidl(fullpidl);
  fileitem = hash_lookup(hash_fileitems, key);
  jfree(key);

  if (fileitem)
    return fileitem;

  if (!create_if_not)
    return NULL;

  /* new file-item */
  fileitem = fileitem_new(NULL);
  fileitem->fullpidl = clone_pidl(fullpidl);

  fileitem->attrib = SFGAO_FOLDER;
  IShellFolder_GetAttributesOf(shl_idesktop, 1,
			       (LPCITEMIDLIST *)&fileitem->fullpidl,
			       &fileitem->attrib);

  {
    LPITEMIDLIST parent_fullpidl = clone_pidl(fileitem->fullpidl);
    remove_last_pidl(parent_fullpidl);

    fileitem->pidl = get_last_pidl(fileitem->fullpidl);
    fileitem->parent = get_fileitem_by_fullpidl(parent_fullpidl, TRUE);

    free_pidl(parent_fullpidl);
  }

  update_by_pidl(fileitem);
  put_fileitem(fileitem);

  return fileitem;
}

static void put_fileitem(FileItem *fileitem)
{
  assert(fileitem->filename != NULL);
  assert(fileitem->keyname == NULL);

  fileitem->keyname = get_key_for_pidl(fileitem->fullpidl);

  /* insert this file-item in the hash-table */
  hash_insert(hash_fileitems, fileitem->keyname, fileitem);
}

#else

/********************************************************************/
/* Allegro for_each_file: Portable                                  */
/********************************************************************/

static FileItem *get_fileitem_by_path(const char *path, bool create_if_not)
{
  char *key;
  FileItem *fileitem;
  int attrib;

  key = get_key_for_filename(path);
  fileitem = hash_lookup(hash_fileitems, key);
  jfree(key);

  if (fileitem)
    return fileitem;

  if (!create_if_not)
    return NULL;

  /* get the attributes of the file */
  attrib = 0;
  if (!file_exists(path, FA_ALL, &attrib)) {
    if (!ji_dir_exists(path))
      return NULL;
    attrib = FA_DIREC;
  }

  /* new file-item */
  fileitem = fileitem_new(NULL);

  fileitem->filename = jstrdup(path);
  fileitem->displayname = jstrdup(get_filename(path));
  fileitem->attrib = attrib;

  /* get the parent */
  {
    char parent_path[MAX_PATH];
    replace_filename(parent_path, path, "", sizeof(parent_path));
    remove_backslash(parent_path);
    fileitem->parent = get_fileitem_by_path(parent_path, TRUE);
  }

  put_fileitem(fileitem);

  return fileitem;
}

static void for_each_child_callback(const char *filename, int attrib, int param)
{
  FileItem *fileitem = (FileItem *)param;
  FileItem *child;
  const char *filename_without_path = get_filename(filename);

  if (*filename_without_path == '.' &&
      (ustrcmp(filename_without_path, ".") == 0 ||
       ustrcmp(filename_without_path, "..") == 0))
    return;

  child = get_fileitem_by_path(filename, FALSE);
  if (!child) {
    child = fileitem_new(fileitem);

    child->filename = jstrdup(filename);
    child->displayname = jstrdup(filename_without_path);
    child->attrib = attrib;

    put_fileitem(child);
  }
  else {
    assert(child->parent == fileitem);
  }

  fileitem_insert_child_sorted(fileitem, child);
}

static char *remove_backslash(char *filename)
{
  int len = ustrlen(filename);

  if (len > 0 &&
      (filename[len-1] == '/' ||
       filename[len-1] == OTHER_PATH_SEPARATOR)) {
#ifdef HAVE_DRIVES
    /* if the name is C:\ or something like that, the backslash isn't
       removed */
    if (len == 3 && filename[1] == DEVICE_SEPARATOR)
      return filename;
#else
    /* this is just the root '/' slash */
    if (len == 1)
      return filename;
#endif
    filename[len-1] = 0;
  }
  return filename;
}

static char *get_key_for_filename(const char *filename)
{
  char buf[MAX_PATH];

  ustrcpy(buf, filename);

#if !defined CASE_SENSITIVE
  ustrlwr(buf);
#endif
  fix_filename_slashes(buf);

  return jstrdup(buf);
}

static void put_fileitem(FileItem *fileitem)
{
  assert(fileitem->filename != NULL);
  assert(fileitem->keyname == NULL);

  fileitem->keyname = get_key_for_filename(fileitem->filename);

  /* insert this file-item in the hash-table */
  hash_insert(hash_fileitems, fileitem->keyname, fileitem);
}

#endif
