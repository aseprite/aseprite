// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TOOLSET_TREE_H_INCLUDED
#define APP_UI_TOOLSET_TREE_H_INCLUDED

#include "app/ui/tree.h"
#include "ui/timer.h"

namespace app {

class ToolsetTreeNode : public TreeNode {
public:
  using TreeNode::TreeNode;

  bool isVisible() const { return m_visible; }
  void setVisible(bool visible) { m_visible = visible; }

  const std::string& id() const { return m_id; }
  void setId(const std::string& id) { m_id = id; }

  bool isRemovable() const { return m_removable; }
  void setRemovable(bool removable) { m_removable = removable; }

  void removeChild(TreeNode* child);
  void insertChildBefore(TreeNode* child, TreeNode* before);

  ToolsetTreeNode* parent() const { return static_cast<ToolsetTreeNode*>(TreeNode::parent()); }
  ToolsetTreeNode* next() const { return static_cast<ToolsetTreeNode*>(TreeNode::next()); }
  ToolsetTreeNode* prev() const { return static_cast<ToolsetTreeNode*>(TreeNode::prev()); }
  ToolsetTreeNode* firstChild() const
  {
    return static_cast<ToolsetTreeNode*>(TreeNode::firstChild());
  }
  ToolsetTreeNode* nextInTree() const
  {
    return static_cast<ToolsetTreeNode*>(TreeNode::nextInTree());
  }

private:
  std::string m_id;
  bool m_removable = false;
  bool m_visible = true;
};

class ToolsetTree : public Tree {
public:
  ToolsetTree();

  ToolsetTreeNode* root() const { return static_cast<ToolsetTreeNode*>(Tree::root()); }

protected:
  bool onProcessMessage(Message* msg) override;
  void onPaint(PaintEvent& ev) override;
  void onInitTheme(InitThemeEvent& ev) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void toggleCollapse(TreeNode* node, bool recursive = false) override;
  ToolsetTreeNode* selected() const { return static_cast<ToolsetTreeNode*>(Tree::selected()); }

private:
  enum class DropPos { Before, Inside, After };

  int nodeHeight(const TreeNode* node) const;
  ToolsetTreeNode* nodeAtY(int y, int* outNodeY = nullptr) const;

  struct {
    int rowHeight = 0;
    int itemSpacing = 0;
    int depthSpacing = 0;
    Theme::TextColors textColors;
    skin::SkinPartPtr openEye;
    skin::SkinPartPtr closedEye;
  } m_themeCache;

  Timer m_dragTimer;
  ToolsetTreeNode* m_dragCandidate = nullptr;
  ToolsetTreeNode* m_dragNode = nullptr;
  ToolsetTreeNode* m_dropTarget = nullptr;
  DropPos m_dropPos = DropPos::Before;
  gfx::Point m_dragMousePos;
  gfx::Point m_dragOffset;
  ToolsetTreeNode* m_hotNode = nullptr;
};

} // namespace app

#endif // APP_UI_TOOLSET_TREE_H_INCLUDED
