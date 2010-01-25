// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VACA_BASE_H
#define VACA_BASE_H

// Windows is the default Vaca platform
#if !defined(VACA_ALLEGRO) && !defined(VACA_GTK)
  #define VACA_WINDOWS
#endif

#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#pragma warning(disable: 4355)
#pragma warning(disable: 4996)

#include <algorithm>
#include <stdarg.h>
#include <string>

#ifdef VACA_WINDOWS
  #include <windows.h>
  #include <commctrl.h>
#endif

#include "Vaca/Enum.h"

// memory leaks
#ifdef MEMORY_LEAK_DETECTOR
  #include "debug_new.h"
#endif

namespace Vaca {

#define VACA_VERSION     0
#define VACA_SUB_VERSION 0
#define VACA_WIP_VERSION 8

/**
   @def VACA_MAIN()
   @brief Defines the name and arguments that the main routine
   of the program should contain.

   You can use it as:
   @code
   int VACA_MAIN()
   {
     ...
   }
   @endcode

   @win32
     It is the signature of @msdn{WinMain}. In other
     operating systems this could be @c "main(int argc, char* argv[])".
   @endwin32
*/
#ifdef VACA_WINDOWS
  #define VACA_MAIN()				\
    PASCAL WinMain(HINSTANCE hInstance,		\
		   HINSTANCE hPrevInstance,	\
		   LPSTR lpCmdLine,		\
		   int nCmdShow)
#else
  #define VACA_MAIN()				\
    int main(int argc, char* argv[])
#endif

/**
   @def VACA_DLL
   @brief Used to export/import symbols to/from the dynamic library.
 */
#ifdef VACA_WINDOWS
  #ifdef VACA_STATIC
    #define VACA_DLL
  #else
    #ifdef VACA_SRC
      #define VACA_DLL __declspec(dllexport)
    #else
      #define VACA_DLL __declspec(dllimport)
    #endif
  #endif
#else
  #define VACA_DLL
#endif

/**
   Returns the minimum of @a x and @a y.

   @note It is just like @b std::min, but there are problems
	 in MSVC++ because a macro named @b min.

   @see max_value, clamp_value
*/
template<typename T>
inline T min_value(T x, T y)
{
  return x < y ? x: y;
}

/**
   Returns the maximum of @a x and @a y.

   @note It is just like @b std::max, but there are problems
	 in MSVC++ because a macro named @b max.

   @see min_value, clamp_value
*/
template<typename T>
inline T max_value(T x, T y)
{
  return x > y ? x: y;
}

/**
   Limits the posible values of @a x to the speficied range.

   If @a x is great than @a high, then @a high is returned,
   if @a x is less than @a low, then @a low is returned.
   In other case, @a x is in the range, and @a x is returned.

   @see min_value, max_value
*/
template<typename T>
inline T clamp_value(T x, T low, T high)
{
  return x > high ? high: (x < low ? low: x);
}

/**
   A wide character used in a String.

   @see String, @ref page_tn_008
*/
typedef wchar_t Char;

/**
   String type used through the Vaca API.

   It is a std::wstring.

   @see Char, @ref page_tn_008
*/
typedef std::wstring String;

/**
   An identifier for an application's Command.

   @see Widget#onCommand, Command
*/
typedef unsigned int CommandId;

/**
   An identifier for a Thread.
*/
typedef unsigned int ThreadId;

// ======================================================================

/**
   It's like a namespace for Orientation.

   @see Orientation
*/
struct OrientationEnum
{
  enum enumeration {
    Horizontal,
    Vertical
  };
  static const enumeration default_value = Horizontal;
};

/**
   Horizontal or vertical orientation.

   One of the following values:
   @li Orientation::Horizontal (default)
   @li Orientation::Vertical
*/
typedef Enum<OrientationEnum> Orientation;

// ======================================================================

/**
   It's like a namespace for TextAlign.

   @see TextAlign
*/
struct TextAlignEnum
{
  enum enumeration {
    Left,
    Center,
    Right
  };
  static const enumeration default_value = Left;
};

/**
   Horizontal alignment.

   One of the following values:
   @li TextAlign::Left (default)
   @li TextAlign::Center
   @li TextAlign::Right
*/
typedef Enum<TextAlignEnum> TextAlign;

// ======================================================================

/**
   It's like a namespace for VerticalAlign.

   @see VerticalAlign
*/
struct VerticalAlignEnum
{
  enum enumeration {
    Top,
    Middle,
    Bottom
  };
  static const enumeration default_value = Top;
};

/**
   Vertical alignment.

   One of the following values:
   @li VerticalAlign::Top
   @li VerticalAlign::Middle
   @li VerticalAlign::Bottom
*/
typedef Enum<VerticalAlignEnum> VerticalAlign;

// ======================================================================

/**
   It's like a namespace for Side.

   @see Side
*/
struct SideEnum
{
  enum enumeration {
    Left,
    Top,
    Right,
    Bottom
  };
  static const enumeration default_value = Left;
};

/**
   A side.

   One of the following values:
   @li Side::Left
   @li Side::Top
   @li Side::Right
   @li Side::Bottom
*/
typedef Enum<SideEnum> Side;

// ======================================================================

/**
   It's like a namespace for Sides.

   @see Sides
*/
struct SidesEnumSet
{
  enum {
    None   = 0,
    Left   = 1,
    Top    = 2,
    Right  = 4,
    Bottom = 8,
    All    = Left | Top | Right | Bottom
  };
};

/**
   A set of sides.

   Zero or more of the following values:
   @li Sides::Left
   @li Sides::Top
   @li Sides::Right
   @li Sides::Bottom
*/
typedef EnumSet<SidesEnumSet> Sides;

// ======================================================================

/**
   It's like a namespace for CardinalDirection.

   @see CardinalDirection
*/
struct CardinalDirectionEnum
{
  enum enumeration {
    North,
    Northeast,
    East,
    Southeast,
    South,
    Southwest,
    West,
    Northwest
  };
  static const enumeration default_value = North;
};

/**
   A cardinal direction.

   One of the following values:
   @li CardinalDirection::North
   @li CardinalDirection::Northeast
   @li CardinalDirection::East
   @li CardinalDirection::Southeast
   @li CardinalDirection::South
   @li CardinalDirection::Southwest
   @li CardinalDirection::West
   @li CardinalDirection::Northwest
*/
typedef Enum<CardinalDirectionEnum> CardinalDirection;

// ======================================================================

/**
   Removes an @a element from the specified STL @a container.

   This routine removes the first ocurrence of @a element in @a container.
   It is just a helper function to avoid cryptic STL code.

   @tparam ContainerType A STL container type.

   @param container The container to be modified.
   @param element The element to be removed from the container.
*/
template<typename ContainerType>
void remove_from_container(ContainerType& container,
			   typename ContainerType::const_reference element)
{
  typename ContainerType::iterator
    it = std::find(container.begin(),
		   container.end(),
		   element);

  if (it != container.end())
    container.erase(it);
}

// ======================================================================
// Classes

class Anchor;
class AnchorLayout;
class Application;
class BandedDockArea;
class BasicDockArea;
class Bix;
class BoxConstraint;
class BoxLayout;
class Brush;
class Button;
class ButtonBase;
class CancelableEvent;
class CheckBox;
class ChildEvent;
class ClientLayout;
class Clipboard;
class CloseEvent;
class Color;
class ColorDialog;
class ComboBox;
class Command;
class CommandEvent;
class CommandsClient;
class CommonDialog;
class Component;
class ConditionVariable;
class Constraint;
class Cursor;
class CustomButton;
class CustomLabel;
class Dialog;
class DockArea;
class DockBar;
class DockFrame;
class DockInfo;
class DragListBox;
class DropFilesEvent;
class Event;
class Exception;
class FileDialog;
class FindTextDialog;
class FocusEvent;
class Font;
class FontDialog;
class FontMetrics;
class Frame;
class Graphics;
class GraphicsPath;
class GroupBox;
class HttpRequest;
class HttpRequestException;
class Icon;
class Image;
class ImageHandle;
class ImageList;
class ImagePixels;
class KeyEvent;
class Label;
class Layout;
class LayoutEvent;
class LinkLabel;
class ListBox;
class ListColumn;
class ListItem;
class ListView;
class ListViewEvent;
class MdiChild;
class MdiClient;
class MdiFrame;
class MdiListMenu;
class Menu;
class MenuBar;
class MenuItem;
class MenuItemEvent;
class MenuSeparator;
class Message;
class MouseEvent;
class MsgBox;
class Mutex;
class Node;
class NonCopyable;
class OpenFileDialog;
class PaintEvent;
class Pen;
class Point;
class PopupMenu;
class PreferredSizeEvent;
class ProgressBar;
class RadioButton;
class RadioGroup;
class ReBar;
class ReBarBand;
class Rect;
class Referenceable;
class Region;
class ResizeEvent;
class ResourceId;
class SaveFileDialog;
class SciEdit;
class SciRegister;
class ScopedLock;
class ScreenGraphics;
class ScrollEvent;
class ScrollInfo;
class Separator;
class SetCursorEvent;
class Size;
class Slider;
class SpinButton;
class Spinner;
class SplitBar;
class StatusBar;
class System;
class Tab;
class TabBase;
class TabPage;
class TextEdit;
class Thread;
class TimePoint;
class Timer;
class ToggleButton;
class ToolBar;
class ToolButton;
class ToolSet;
class TreeNode;
class TreeView;
class TreeViewEvent;
class TreeViewIterator;
class Widget;
class WidgetClassName;

template<class T>
class SharedPtr;

// ======================================================================
// Smart Pointers

/**
   @defgroup smart_pointers Smart Pointers
   @{
 */

typedef SharedPtr<Anchor> AnchorPtr;
typedef SharedPtr<AnchorLayout> AnchorLayoutPtr;
typedef SharedPtr<BandedDockArea> BandedDockAreaPtr;
typedef SharedPtr<BasicDockArea> BasicDockAreaPtr;
typedef SharedPtr<Bix> BixPtr;
typedef SharedPtr<BoxConstraint> BoxConstraintPtr;
typedef SharedPtr<BoxLayout> BoxLayoutPtr;
typedef SharedPtr<Button> ButtonPtr;
typedef SharedPtr<ButtonBase> ButtonBasePtr;
typedef SharedPtr<CheckBox> CheckBoxPtr;
typedef SharedPtr<ClientLayout> ClientLayoutPtr;
typedef SharedPtr<ColorDialog> ColorDialogPtr;
typedef SharedPtr<ComboBox> ComboBoxPtr;
typedef SharedPtr<Command> CommandPtr;
typedef SharedPtr<CommonDialog> CommonDialogPtr;
typedef SharedPtr<Component> ComponentPtr;
typedef SharedPtr<Constraint> ConstraintPtr;
typedef SharedPtr<CustomButton> CustomButtonPtr;
typedef SharedPtr<CustomLabel> CustomLabelPtr;
typedef SharedPtr<Dialog> DialogPtr;
typedef SharedPtr<DockArea> DockAreaPtr;
typedef SharedPtr<DockBar> DockBarPtr;
typedef SharedPtr<DockFrame> DockFramePtr;
typedef SharedPtr<DockInfo> DockInfoPtr;
typedef SharedPtr<DragListBox> DragListBoxPtr;
typedef SharedPtr<FileDialog> FileDialogPtr;
typedef SharedPtr<FindTextDialog> FindTextDialogPtr;
typedef SharedPtr<FontDialog> FontDialogPtr;
typedef SharedPtr<Frame> FramePtr;
typedef SharedPtr<GroupBox> GroupBoxPtr;
typedef SharedPtr<Label> LabelPtr;
typedef SharedPtr<Layout> LayoutPtr;
typedef SharedPtr<LinkLabel> LinkLabelPtr;
typedef SharedPtr<ListBox> ListBoxPtr;
typedef SharedPtr<ListItem> ListItemPtr;
typedef SharedPtr<ListView> ListViewPtr;
typedef SharedPtr<MdiChild> MdiChildPtr;
typedef SharedPtr<MdiClient> MdiClientPtr;
typedef SharedPtr<MdiFrame> MdiFramePtr;
typedef SharedPtr<MdiListMenu> MdiListMenuPtr;
typedef SharedPtr<Menu> MenuPtr;
typedef SharedPtr<MenuBar> MenuBarPtr;
typedef SharedPtr<MenuItem> MenuItemPtr;
typedef SharedPtr<MenuSeparator> MenuSeparatorPtr;
typedef SharedPtr<OpenFileDialog> OpenFileDialogPtr;
typedef SharedPtr<PopupMenu> PopupMenuPtr;
typedef SharedPtr<ProgressBar> ProgressBarPtr;
typedef SharedPtr<RadioButton> RadioButtonPtr;
typedef SharedPtr<ReBar> ReBarPtr;
typedef SharedPtr<SaveFileDialog> SaveFileDialogPtr;
typedef SharedPtr<SciEdit> SciEditPtr;
typedef SharedPtr<Separator> SeparatorPtr;
typedef SharedPtr<Slider> SliderPtr;
typedef SharedPtr<SpinButton> SpinButtonPtr;
typedef SharedPtr<Spinner> SpinnerPtr;
typedef SharedPtr<SplitBar> SplitBarPtr;
typedef SharedPtr<StatusBar> StatusBarPtr;
typedef SharedPtr<Tab> TabPtr;
typedef SharedPtr<TabBase> TabBasePtr;
typedef SharedPtr<TabPage> TabPagePtr;
typedef SharedPtr<TextEdit> TextEditPtr;
typedef SharedPtr<ToggleButton> ToggleButtonPtr;
typedef SharedPtr<ToolBar> ToolBarPtr;
typedef SharedPtr<ToolSet> ToolSetPtr;
typedef SharedPtr<TreeNode> TreeNodePtr;
typedef SharedPtr<TreeView> TreeViewPtr;
typedef SharedPtr<Widget> WidgetPtr;

/** @} */

} // namespace Vaca

#endif // VACA_BASE_H
