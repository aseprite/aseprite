// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/ui/tree.h"

#include "base/time.h"
#include "skin/skin_theme.h"
#include "text/font_metrics.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"
#include "ui/utf8_range_builder.h"
#include "ui/view.h"

using namespace text;

namespace app {

using namespace ui;
using namespace app::skin;

TreeNode::TreeNode(const std::string& text, const SkinPartPtr& icon)
  : m_text(text)
  , m_icon(icon)
  , m_collapsed(false)
  , m_parent(nullptr)
  , m_prev(nullptr)
  , m_next(nullptr)
  , m_firstChild(nullptr)
  , m_lastChild(nullptr)
{
}

TreeNode::~TreeNode()
{
  m_parent = nullptr;
  TreeNode* next = m_firstChild;
  while (next) {
    TreeNode* child = next;
    next = next->next();
    delete child;
  }
}

void TreeNode::setText(const std::string& text)
{
  m_text = text;
  m_blob.reset();
}

int TreeNode::depth() const
{
  int depth = 0;
  for (const TreeNode* i = this->parent(); i; i = i->parent())
    depth += 1;
  return depth;
}

void TreeNode::attachTo(TreeNode* parent, TreeNode* prev, TreeNode* next)
{
  m_parent = parent;
  m_prev = prev;
  m_next = next;
}

void TreeNode::addChild(TreeNode* child)
{
  if (!child)
    return;

  if (m_firstChild) {
    auto* oldLast = m_lastChild;
    oldLast->setNext(child);
    child->attachTo(this, oldLast, nullptr);
    m_lastChild = child;
  }
  else {
    m_firstChild = child;
    m_lastChild = child;
    child->attachTo(this, nullptr, nullptr);
  }
}

TreeNode* TreeNode::nextInTree() const
{
  if (!isCollapsed() && firstChild())
    return firstChild();

  if (next())
    return next();

  const TreeNode* node = parent();
  while (node && !node->next())
    node = node->parent();

  if (!node)
    return nullptr;

  return node->next();
}

TreeNode* TreeNode::prevInTree() const
{
  if (prev() && prev()->lastChild() && !prev()->isCollapsed()) {
    TreeNode* prevSub = nullptr;
    for (TreeNode* sub = prev()->lastChild(); sub; sub = sub->nextInTree()) {
      if (sub == this)
        return prevSub;
      prevSub = sub;
    }
  }

  if (prev())
    return prev();

  if (parent())
    return parent();

  return nullptr;
}

Tree::Tree() : Widget(kTreeWidget), m_root(nullptr), m_selected(nullptr), m_lastCharTick(0)
{
  enableFlags(CTRL_RIGHT_CLICK);
  setFocusStop(true);
  initTheme();
}

void Tree::setRoot(TreeNode* root)
{
  const bool hadSelection = (m_selected != nullptr);
  m_selected = nullptr;
  m_root.reset(root);

  if (hadSelection)
    Change();

  invalidate();
}

void Tree::setSelected(TreeNode* node, bool scrollToNode)
{
  bool changed = (m_selected != node);

  m_selected = node;

  if (auto* view = View::getView(this); scrollToNode && node && view && view->hasScrollBars()) {
    // Fast paths first
    if (m_selected == root()) {
      view->setViewScroll(gfx::Point(0, 0));
    }
    else if (m_selected->hasChildren() && m_selected == m_root->lastChild()) {
      view->setViewScroll(gfx::Point(0, view->getScrollableSize().h));
    }
    else {
      const auto fontHeight = font()->metrics(nullptr);
      gfx::Rect nodeRect(
        gfx::Point(m_selected->depth() * m_themeCache.depthSpacing, border().top()),
        gfx::Size(fontHeight, fontHeight));

      for (const TreeNode* i = root(); i; i = i->nextInTree()) {
        if (i == m_selected)
          break;
        nodeRect.y += nodeHeight(i);
      }

      if (m_selected->textBlob())
        nodeRect.h = m_selected->textBlob()->bounds().size().h;

      if (m_selected->icon())
        nodeRect.w += m_selected->icon()->size().w + m_themeCache.itemSpacing;

      gfx::Rect viewRect(view->viewportBounds());
      viewRect.offset(-bounds().origin());

      if (!viewRect.contains(nodeRect)) {
        gfx::Point point(viewRect.x, nodeRect.y);

        if (nodeRect.y2() > viewRect.y2())
          point.y = nodeRect.y - (viewRect.h - nodeRect.h - (m_themeCache.itemSpacing * 2));

        if (viewRect.x > nodeRect.x) {
          point.x = nodeRect.x;

          // Keep our y-scroll if we're moving in X
          if (viewRect.y > point.y && point.y < viewRect.y2())
            point.y = view->viewScroll().y;
        }

        view->setViewScroll(point);
      }
    }
  }

  if (changed) {
    Change();
    invalidate();
  }
}

void Tree::toggleCollapse(TreeNode* node, const bool recursive)
{
  node->toggleCollapse();

  if (recursive) {
    const int startingDepth = node->depth();
    for (TreeNode* j = node->firstChild(); j; j = j->nextInTree()) {
      if (j->depth() < startingDepth)
        break;

      if (j->hasChildren()) {
        j->toggleCollapse();
        j = j->firstChild();
      }
    }
  }

  if (auto* view = View::getView(this))
    view->updateView();

  invalidate();
}

void Tree::selectLast()
{
  if (!m_root)
    return;

  TreeNode* last = m_root->lastChild();
  for (TreeNode* node = last; node; node = node->nextInTree())
    last = node;
  setSelected(last, true);
}

// Helper to calculate node height when we have a drawn text blob (might vary with spritesheet fonts
// and unicode characters)
int Tree::nodeHeight(const TreeNode* node) const
{
  return node->textBlob() ? node->textBlob()->bounds().h + (m_themeCache.itemSpacing * 2) :
                            m_themeCache.rowHeight;
}

bool Tree::onMouseMessage(const MouseMessage* mouseMsg)
{
  auto* view = View::getView(this);
  if (!view)
    return false;

  gfx::Rect viewRect(view->viewportBounds());
  viewRect.offset(-bounds().origin());

  const gfx::PointF offsetPosition(mouseMsg->position() - childrenBounds().origin() -
                                   gfx::Point(border().left(), 0));
  int nodeY = border().top();
  // TODO: We could probably cache the first visible node every time we scroll
  // so we can avoid going through the entire tree?
  for (TreeNode* i = root(); i; i = i->nextInTree()) {
    const int nextNodeY = nodeY + nodeHeight(i);
    if (offsetPosition.y >= nodeY && offsetPosition.y <= nextNodeY) {
      if (mouseMsg->left() || mouseMsg->right())
        setSelected(i, true);

      switch (mouseMsg->type()) {
        case kMouseDownMessage: {
          if (mouseMsg->left() && i->hasChildren() &&
              (offsetPosition.x <= i->depth() * m_themeCache.depthSpacing)) {
            toggleCollapse(i, mouseMsg->shiftPressed());
            return true;
          }

          if (mouseMsg->right()) {
            RightClickItem();
            return true;
          }
          break;
        }
        case kMouseMoveMessage: {
          if (!i->tooltip().empty()) {
            if (!m_tooltipManager)
              m_tooltipManager = std::make_unique<AppTooltipManager>();

            gfx::Rect nodeBounds(0, nodeY, clientBounds().w, nodeHeight(i));
            nodeBounds.offset(bounds().origin());
            m_tooltipManager->showAreaTooltip(this, nodeBounds, i->tooltip(), ui::TOP);
            return true;
          }
          break;
        }
      }

      return true;
    }

    nodeY = nextNodeY;
    if (nodeY > viewRect.y2())
      return false; // Outside of view bounds already.
  }

  return false;
}

bool Tree::onKeyDown(const KeyMessage* keyMsg)
{
  switch (keyMsg->scancode()) {
    case kKeyEnter: {
      if (m_selected) {
        if (m_selected->hasChildren() && m_selected->isCollapsed())
          toggleCollapse(m_selected, keyMsg->shiftPressed());
        DoubleClickItem();
      }
      return true;
    }
    case kKeyRight:
      if (m_selected && m_selected->hasChildren() && m_selected->isCollapsed()) {
        toggleCollapse(m_selected, keyMsg->shiftPressed());
        return true;
      }

      [[fallthrough]];
    case kKeyDown: {
      if (m_selected)
        setSelected(m_selected->nextInTree(), true);
      if (!m_selected)
        setSelected(root(), true);
      return true;
    }
    case kKeyLeft: {
      if (m_selected) {
        if (m_selected->hasChildren()) {
          if (!m_selected->isCollapsed()) {
            toggleCollapse(m_selected, keyMsg->shiftPressed());
          }
          else if (m_selected != root()) {
            setSelected(m_selected->prevInTree(), true);
          }
        }
        else if (m_selected != root()) {
          setSelected(m_selected->parent());
        }
        return true;
      }
      break;
    }
    case kKeyUp: {
      if (m_selected) {
        if (m_selected != root()) {
          setSelected(m_selected->prevInTree(), true);
        }
        else {
          selectLast();
        }
        return true;
      }
      break;
    }
    case kKeyHome:   setSelected(root(), true); return true;
    case kKeyEnd:    selectLast(); return true;
    case kKeyPageUp: {
      if (auto* view = View::getView(this)) {
        if (view->verticalBar()->isVisible()) {
          gfx::Rect viewRect(view->viewportBounds());
          viewRect.offset(-bounds().origin());
          if (!m_selected) {
            view->setViewScroll(view->viewScroll() - gfx::Point(0, viewRect.h));
            return true;
          }

          int nodeY = border().top();
          int selectedNodeY = 0;
          for (const TreeNode* i = root(); i; i = i->nextInTree()) {
            nodeY += nodeHeight(i);
            if (m_selected == i) {
              selectedNodeY = nodeY;
              break;
            }
          }
          ASSERT(selectedNodeY);
          if (selectedNodeY > 0) {
            nodeY = border().top();

            for (TreeNode* i = root(); i; i = i->nextInTree()) {
              nodeY += nodeHeight(i);

              if (nodeY > selectedNodeY - viewRect.h) {
                setSelected(i, true);
                return true;
              }
            }
          }
        }
      }
      setSelected(root(), false);
      return true;
    }
    case kKeyPageDown: {
      if (auto* view = View::getView(this)) {
        if (view->verticalBar()->isVisible()) {
          gfx::Rect viewRect(view->viewportBounds());
          viewRect.offset(-bounds().origin());
          if (!m_selected) {
            view->setViewScroll(view->viewScroll() + gfx::Point(0, viewRect.h));
            return true;
          }

          int nodeY = border().top();
          int selectedNodeY = 0;
          for (TreeNode* i = root(); i; i = i->nextInTree()) {
            const int h = nodeHeight(i);
            if (i == m_selected)
              selectedNodeY = nodeY;
            nodeY += h;

            if (selectedNodeY > 0 && nodeY > selectedNodeY + viewRect.h) {
              setSelected(i, true);
              view->setViewScroll(view->viewScroll() + gfx::Point(0, viewRect.h - h));
              return true;
            }
          }
        }
      }
      selectLast();
      return true;
    }
    default: break;
  }

  // Text filtering
  if (!keyMsg->isDeadKey() && keyMsg->unicodeChar() >= 32) {
    const bool inTime = (base::current_tick() - m_lastCharTick) < 1500;
    if (!inTime) {
      m_findString.clear();
      m_lastCharTick = base::current_tick();
    }

    m_findString += base::string_to_lower(base::codepoint_to_utf8(keyMsg->unicodeChar()));

    TreeNode* start = root();
    if (m_selected)
      start = inTime ? m_selected : m_selected->nextInTree();
    for (TreeNode* node = start; node; node = node->nextInTree()) {
      if (!node->text().empty() && base::string_to_lower(node->text()).find(m_findString) == 0) {
        setSelected(node, true);
        return true;
      }
    }
  }

  return false;
}

bool Tree::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kKeyDownMessage: {
      if (hasFocus() && isEnabled()) {
        const auto* keyMsg = static_cast<KeyMessage*>(msg);

        if (m_selected && m_selected->keyDown(keyMsg))
          return true;

        if (onKeyDown(keyMsg)) {
          invalidate();
          return true;
        }
      }
      break;
    }
    case kDoubleClickMessage: {
      if (isEnabled() && !hasCapture() && m_selected) {
        if (m_selected->hasChildren())
          toggleCollapse(m_selected);

        DoubleClickItem();
        invalidate();
        return true;
      }
      break;
    }
    case kMouseDownMessage: captureMouse(); [[fallthrough]];
    case kMouseMoveMessage:
      if (!m_root || !isEnabled())
        break;

      if (onMouseMessage(static_cast<MouseMessage*>(msg))) {
        invalidate();
        return true;
      }
      break;
    case kMouseUpMessage: {
      if (hasCapture())
        releaseMouse();
      break;
    }
    case kMouseWheelMessage: {
      View::scrollByMessage(this, msg);
      if (m_tooltipManager)
        m_tooltipManager->closeTooltip();
      invalidate();
      break;
    }
  }
  return Widget::onProcessMessage(msg);
}

void Tree::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();

  const auto& textColors = m_themeCache.textColors;
  const Paint& textPaint = isEnabled() ? textColors.text : textColors.disabledText;
  const Paint& backgroundPaint = isEnabled() ? textColors.background :
                                               textColors.disabledBackground;
  const Paint& selectedPaint = textColors.selectedText;
  const Paint& selectedBackgroundPaint = textColors.selectedBackground;

  const int itemSpacing = m_themeCache.itemSpacing;
  const int depthSpacing = m_themeCache.depthSpacing;

  g->drawRect(g->getClipBounds(), backgroundPaint);

  gfx::PointF point(clientChildrenBounds().origin() + gfx::PointF(border().size()));
  const gfx::Rect clipBounds = g->getClipBounds();
  const auto fontHeight = font()->metrics(nullptr);
  const auto fullWidth = size().w;

  for (TreeNode* node = root(); node; node = node->nextInTree()) {
    const bool draw = clipBounds.intersects(
      gfx::Rect(point.x, point.y, fullWidth, nodeHeight(node)));

    if (draw && !node->textBlob()) {
      Utf8RangeBuilder rangeBuilder(node->text().size());
      node->setTextBlob(
        TextBlob::MakeWithShaper(theme()->fontMgr(), font(), node->text(), &rangeBuilder));
    }

    const auto height = node->textBlob() ? node->textBlob()->bounds().h : fontHeight;

    if (draw) {
      const auto depth = node->depth();
      const int currentDepthSpacing = depth * depthSpacing;

      // Collapse button
      if (node->hasChildren()) {
        const auto* skinTheme = SkinTheme::get(this);
        const auto part = node->isCollapsed() ? skinTheme->parts.iconTreeExpand() :
                                                skinTheme->parts.iconTreeCollapse();
        const auto buttonPoint = gfx::Point(
          point.x + (currentDepthSpacing - part->size().w - itemSpacing),
          guiscaled_center(point.y, height, part->size().h));
        g->drawColoredRgbaSurface(part->bitmap(0),
                                  textColors.text.color(),
                                  buttonPoint.x,
                                  buttonPoint.y);
      }

      // Icon
      auto textPoint = point + gfx::Point(currentDepthSpacing, 0);
      if (node->icon()) {
        const gfx::Point nodePoint(textPoint.x + itemSpacing,
                                   guiscaled_center(textPoint.y, height, node->icon()->size().w));
        g->drawColoredRgbaSurface(node->icon()->bitmap(0),
                                  textColors.text.color(),
                                  nodePoint.x,
                                  nodePoint.y);
        textPoint.x += node->icon()->size().w + (itemSpacing * 2);
      }

      // Selection background rect
      if (node == m_selected && isEnabled()) {
        g->drawRect(
          gfx::RectF(textPoint - gfx::Point(itemSpacing / 2, itemSpacing / 2),
                     node->textBlob()->bounds().size() + gfx::SizeF(itemSpacing, itemSpacing)),
          selectedBackgroundPaint);
      }

      // Text
      g->drawTextBlob(node->textBlob(),
                      textPoint,
                      node == m_selected && isEnabled() ? selectedPaint : textPaint);
    }

    point.y += height + (itemSpacing * 2);
  }
}

void Tree::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  auto* theme = SkinTheme::get(this);
  const auto iconPart = theme->parts.iconTreeCollapse();

  m_themeCache.itemSpacing = theme->dimensions.treeItemSpacing();
  m_themeCache.rowHeight = font()->metrics(nullptr) + (m_themeCache.itemSpacing * 2);
  m_themeCache.textColors = theme->getTextColors(this);
  m_themeCache.depthSpacing = iconPart ? iconPart->size().w + (m_themeCache.itemSpacing * 3) :
                                         m_themeCache.itemSpacing * 6;

  // Invalidate text blobs in case we've changed the font
  for (TreeNode* node = root(); node; node = node->nextInTree())
    node->setTextBlob(nullptr);

  if (auto* view = View::getView(this))
    view->updateView();
}

void Tree::onSizeHint(SizeHintEvent& ev)
{
  gfx::Size hint = border().size() + m_themeCache.itemSpacing;
  for (const TreeNode* node = root(); node; node = node->nextInTree()) {
    int width = 0;
    if (node->textBlob()) {
      width = std::ceil(node->textBlob()->bounds().w);
    }
    else {
      width = std::ceil(font()->textLength(node->text()));
    }

    if (node->icon())
      width += node->icon()->size().w + m_themeCache.itemSpacing;

    width += (node->depth() * m_themeCache.depthSpacing) + (border().size().w * 2);

    hint.w = std::max<int>(hint.w, width);
    hint.h += nodeHeight(node);
  }

  ev.setSizeHint(hint);
}

} // namespace app
