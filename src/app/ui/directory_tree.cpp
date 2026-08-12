// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/ui/directory_tree.h"

#include "app/i18n/strings.h"
#include "app/ui/skin/skin_theme.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "ui/menu.h"

namespace app {

using namespace ui;

DirectoryTreeNode::DirectoryTreeNode(const std::string& text,
                                     const std::string& path,
                                     const std::function<void(PathTreeNode*)>& loadCallback,
                                     const SkinPartPtr& icon)
  : PathTreeNode(text, path, icon)
  , m_loadCallback(loadCallback)
  , m_loaded(false)
{
  setCollapsed(true);
}

void DirectoryTreeNode::onToggleCollapse()
{
  if (!m_loaded) {
    m_loadCallback(this);
    m_loaded = true;
  }

  TreeNode::onToggleCollapse();
}

bool DirectoryTreeNode::onHasChildren() const
{
  if (m_loaded)
    return firstChild() != nullptr;

  return true;
}

DirectoryTree::DirectoryTree(const std::string& path, const std::vector<std::string>& extensions)
  : m_path(path)
  , m_extensions(extensions)
{
  ASSERT(base::is_directory(path));
  switchDirectory(path);

  m_doubleClickConn = DoubleClickItem.connect(&DirectoryTree::showItem, this);
  m_rightClickConn = RightClickItem.connect(&DirectoryTree::showPopup, this);
}

void DirectoryTree::switchDirectory(const std::string& path)
{
  auto* root = new PathTreeNode(base::get_file_path(path), path);
  root->setPath(path);
  addNodesForPath(root);
  setRoot(root);
  m_path = path;
}

void DirectoryTree::setExtensionFilter(const std::vector<std::string>& extensions)
{
  if (m_extensions != extensions) {
    m_extensions = extensions;
    switchDirectory(m_path);
  }
}

void DirectoryTree::toggleCollapse(TreeNode* node, bool)
{
  // Disables recursive collapsing - TODO: Implement a depth limit
  Tree::toggleCollapse(node, false);
}

void DirectoryTree::showPopup()
{
  const auto* node = static_cast<PathTreeNode*>(selected());
  if (!node)
    return;

  Menu menu;
  MenuItem open(Strings::directory_tree_open());
  MenuItem openFolder(Strings::directory_tree_open_folder());

  open.setEnabled(!node->hasChildren());

  menu.addChild(&open);
  menu.addChild(&openFolder);

  for (auto* item : menu.children())
    item->processMnemonicFromText();

  const auto& path = node->path();
  open.Click.connect([path] { base::launcher::open_file(path); });
  openFolder.Click.connect([path] { base::launcher::open_folder(path); });

  menu.showPopup(mousePosInDisplay(), display());
}

void DirectoryTree::showItem()
{
  if (const auto* selectedPath = static_cast<PathTreeNode*>(selected())) {
    if (!selectedPath->hasChildren())
      base::launcher::open_file(selectedPath->path());
  }
}

bool DirectoryTree::shouldInclude(const std::string& path) const
{
  const auto& ext = base::string_to_lower(base::get_file_extension(path));
  for (const auto& comp : m_extensions) {
    if (ext == comp)
      return true;
  }

  return false;
}

bool DirectoryTree::isRelevantToFilter(const std::string& path) const
{
  if (base::is_file(path) && shouldInclude(path))
    return true;

  for (const auto& fn : base::list_files(path)) {
    auto full = base::join_path(path, fn);
    if (base::is_directory(full) && isRelevantToFilter(full))
      return true;

    if (shouldInclude(full))
      return true;
  }

  return false;
}

void DirectoryTree::addNodesForPath(PathTreeNode* node)
{
  const auto* theme = SkinTheme::get(this);
  const auto folderIcon = theme->parts.iconTreeFolder();
  const auto fileIcon = theme->parts.iconTreeFile();
  const auto& path = node->path();

  for (const auto& fn : base::list_files(path)) {
    auto full = base::join_path(path, fn);
    PathTreeNode* newNode = nullptr;
    if (!m_extensions.empty() && !isRelevantToFilter(full))
      continue;

    if (base::is_file(full)) {
      newNode = new PathTreeNode(fn, full, fileIcon);
      newNode->setTooltip(full);
    }
    else if (base::is_directory(full)) {
      if (base::list_files(full).empty()) {
        newNode = new PathTreeNode(fn, full, fileIcon);
      }
      else {
        auto load = [this](PathTreeNode* sub) { addNodesForPath(sub); };
        newNode = new DirectoryTreeNode(fn, full, load, folderIcon);
      }
    }
    if (newNode) {
      node->addChild(newNode);
    }
  }
}

} // namespace app
