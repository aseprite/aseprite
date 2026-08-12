// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TREE_DIRECTORY_H_INCLUDED
#define APP_UI_TREE_DIRECTORY_H_INCLUDED

#include "app/ui/tree.h"

namespace app {

class PathTreeNode : public TreeNode {
public:
  explicit PathTreeNode(const std::string& text,
                        const std::string& path,
                        const SkinPartPtr& icon = nullptr)
    : TreeNode(text, icon)
    , m_path(path) {};
  const std::string& path() const { return m_path; };
  void setPath(const std::string& path) { m_path = path; }

private:
  std::string m_path;
};

class DirectoryTreeNode : public PathTreeNode {
public:
  explicit DirectoryTreeNode(const std::string& text,
                             const std::string& path,
                             const std::function<void(PathTreeNode*)>& loadCallback,
                             const SkinPartPtr& icon);

protected:
  void onToggleCollapse() override;
  bool onHasChildren() const override;

private:
  std::function<void(PathTreeNode*)> m_loadCallback;
  bool m_loaded;
};

class DirectoryTree : public Tree {
public:
  explicit DirectoryTree(const std::string& path, const std::vector<std::string>& extensions = {});

  void switchDirectory(const std::string& path);
  void setExtensionFilter(const std::vector<std::string>& extensions);

protected:
  void toggleCollapse(TreeNode* node, bool) override;
  void showPopup();
  void showItem();

private:
  bool shouldInclude(const std::string& path) const;
  bool isRelevantToFilter(const std::string& path) const;
  void addNodesForPath(PathTreeNode* node);

  std::string m_path;
  std::vector<std::string> m_extensions;

  obs::scoped_connection m_rightClickConn;
  obs::scoped_connection m_doubleClickConn;
};

} // namespace app

#endif // APP_UI_TREE_DIRECTORY_H_INCLUDED
