============
 ASE Manual
============

:Author: David Capello
:Contact: davidcapello@gmail.com
:Date: March 2010

.. contents::

----------
 Tutorial
----------

This chapter explains how to use ASE step by step. Here you learn ASE
terminology and how to use the different screen elements to created
your own sprites. All other chapters are extensive reference of the
application to see its functionality in depth.

Sprites
=======

You can start creating a `sprite <http://en.wikipedia.org/wiki/Sprite_(computer_graphics)>`__
with ``File > New`` option.

``TODO new sprite screenshot``

In this dialog you select:

- The width and height of the new sprite in pixels.

  .. note::

    ASE works with `raster graphics <http://en.wikipedia.org/wiki/Raster_image>`__,
    where images are conformed by a
    `grid of pixels <http://en.wikipedia.org/wiki/Pixel>`__,
    little squares distributed in a rectangular way. When you
    see a photo in your computer, you almost does not notice the
    existence of pixels (they are too small). But when you see little
    images in your mobile device or in a
    `handheld video game <http://en.wikipedia.org/wiki/Handheld_game_device>`__,
    you can appreciate the pixels. ASE is a program to see and manipulate
    pixels, to created this kind of little images.

- The color mode, which basically says how many colors you will be able to put in the image.

  .. note::

    With **RGB** images you can have an independent color for each pixel, in
    this way each little pixel has its Red, Green, Blue and Alpha
    (opacity) values. **Indexed** sprites have a special element associated:
    a color palette (with a maximum of 256 colors), in this way each
    pixel has a palette entry associated, if you change the palette
    color, all pixels associated to that color will change their aspect.
    You can choose the number of color for the sprite palette (from 2 to
    256).

    For `pixel art <http://en.wikipedia.org/wiki/Pixel_art>`__ your selection
    should be Indexed.

- The background color to be used in the sprite.

  .. note::

    ASE sprites have layers, when you create a new sprite, it is created
    with just one layer. The background color specified the color to be
    used to clear this first layer content. There a two kind of sprites
    in ASE: 1) sprites with a background layer (a layer at the bottom
    that cannot be moved), and 2) sprites without a background layer
    (all layers are transparent). If you specified "Transparent" as
    background color, you will obtain a sprite with a layer that is
    completely transparent initially (you will see a checked-background
    indicating "no background here"). In the other side, if you
    specified a color as background, you will get a sprite with the
    background layer painted with that color.

After creating the sprite, you will see it in the **current editor**. The
current editor is where you can draw (the center of the screen).
In next sections you will see how to split the current editor in
various other editors, so you will take care of current editor
significance.

  ``TODO screenshot of new sprite created``

Colors
======

In ASE you draw pressing the buttons of the mouse. Left mouse button
draws with the foreground color, and right mouse button uses the
background color. The **background color** is a very special color, it is
used in various operations that are not related to drawing with the
right mouse button. E.g. when you cut or clear a portion of image
(``Edit > Cut``, or ``Edit > Clear``) the selected pixels are cleared with
the background color.

``TODO screenshot of color bar``

Picking Colors
--------------

You can pick colors from the image using ``Alt+mouse click``. Using
``Alt+left click`` you will choose the foreground color. With ``Alt+right
click`` you choose the background color.

In some platforms (Linux or MacOS) you can have some problems
using the ``Alt`` key together with mouse clicks, so you can
use ``I key`` to use the `Eyedropper Tool`_.

Drawing
=======

You have created a new sprite, now you want to draw. You need to know two things:

 - Where you draw: the current editor shows the selected sprite
   in tabs.

   ``TODO screenshot a selected tab and the editor showing the sprite``

 - With what you draw: there are various elements that are used when you draw in the sprite:
   tool, color, pen, and other configuration that modifies how you draw (e.g. snap to grid,
   tiled mode, fill or not the shape, etc.).

You will notice that a sprite is not just one plain image. It can have
frames and layers. So in next sections, the current editor will take
more importance when you want to manage various sprites, with frames,
and layers at the same time. Right now let's keep it simple, just one
sprite with one image.

To draw you can use one of the tools at the right of the screen:

``TODO screenshot of tool bar``

By default the `Pencil Tool`_ is selected (if it is not selected, you
can press the |pencil icon| icon to select it). The pencil is one of the most
basic tools: You press the left mouse button, hold it, drag the mouse
and then release the same button. This will draw a freehanded trace
using the selected foreground color.

.. |pencil icon| image:: pencil_icon.png

Canceling trace
---------------

If you do not like the last trace you drew, you can press ``Ctrl+Z``
or select ``Edit > Undo`` menu option to undo it. Also, you can cancel
the trace before releasing left mouse button (in this case there are
no need to Undo):

 1. while you are pressing left mouse button,
 2. press right button,
 3. then release left button,
 4. and finally release the right mouse button.

In this way you cancel the trace you was drawing (the whole trace will
disappear instantaneously). You can do the same procedure inverting the
mouse buttons. E.g. if you start with right mouse button (background
color), you can cancel using left button.

Zooming
=======

You can zoom using the ``mouse wheel`` or just pressing the numbers ``1``, ``2``,
``3``, ``4``, ``5``, or ``6`` in the keyboard. When you zoom, the pixel above
the mouse cursor will be centered in the current editor.

You can zoom while you are drawing too.

Scrolling
=========

To scroll the image you can press the ``Space bar`` key (and keep it pressed) and
then drag the image with the mouse button.

When you are drawing you will notice that moving the mouse outside the
bounds of the editor will scroll the image to continue.
``TODO configuration about smooth/big step scroll``

Selecting
=========

Moving Parts
------------

Layers
======

The Background Layer
--------------------

Transparent Layers 
------------------

Animation
=========

Frames
------

Cels
----

As each sprite has a set of layers and frames, you can imagine them
as a grid, where layers are rows and frames are columns. Each little
cell of this grid is called: cel. A cel is an image located in specific
layer, in a specific frame, with a specific position (x, y)
and with other properties like "opacity" (transparency level).

[TODO Animation editor screenshot]

Empty Cel
---------

.. _empty cel:

When a transparent layer is completely invisible in a specific frame
(it does not contain any pixel of any color), you are in an empty cel.
It means this cel is not consuming any memory because its image
does not even exist.

You can remove a non-empty cel using the clear_ command in a transparent
layer.

Multiple Editors
================

Current Editor
--------------

-------
 Tools
-------

Selection Tools
===============

Rectangular Marquee Tool
------------------------

With this tool you can select rectangular regions in the sprite. You
select a rectangular portion of sprite pressing left mouse button,
moving the mouse, and finally releasing the same button. If you repeat this same
operation over and over again you can add more rectangles to the
selected area. If you move the mouse over the selected region, you can drag-and-drop
this portion of sprite using the left mouse button.

Using the right mouse button you can substract rectangles from selection.

  .. admonition:: Summary

    **Left button**: Outside the selection adds rectangles;
    inside the selection *drag-and-drop* it.

    **Right button**: Subtracts rectangles from selection.

Lasso Tool
----------

With this tool you can select free hand drawn contours. You
press the mouse button, move it to draw a contour, and then
when you release the button, a line will close the contour
from the starting point to the end point. The are will be
selected (if you used the left mouse button) or deselected
(if you used the right mouse button).

  .. admonition:: Summary

    **Left button**: Outside the selection adds contours;
    inside the selection *drag-and-drop* it.

    **Right button**: Subtracts contours from selection.

Polygonal Lasso Tool
--------------------

  .. admonition:: Summary

    **Left button**: Outside the selection adds polygons;
    inside the selection *drag-and-drop* it.

    **Right button**: Subtracts polygons from selection.

Magic Wand Tool
---------------

With this tool you can select a continuous area filled with the same
color.

  .. admonition:: Summary

    **Left button**: Select the adjacent region of clicked color.


Pencil Tool
===========

  .. admonition:: Summary

    **Left button**: Paint a freehanded trace with foreground color.

    **Right button**: Paint a freehanded trace with background color.

Spray Tool
==========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Eraser Tool
===========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Eyedropper Tool
===============

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Hand Tool
=========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Move Tool
=========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Paint Bucket Tool
=================

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Line Tool
=========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Curve Tool
==========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Rectangle Tool
==============

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Ellipse Tool
============

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Contour Tool
============

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Polygon Tool
============

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Blur Tool
=========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

Jumble Tool
===========

  .. admonition:: Summary

    **Left button**: .

    **Right button**: .

-------
 Menus
-------

In this chapter you have explained each functionality of ASE that is
accessible from menus. Some options have a keyboard shortcut associated
to be quickly executed.

File
====

New
---

.. admonition:: Keyboard shortcut:

   Ctrl+N

Creates a new sprite.

Open
----

.. admonition:: Keyboard shortcut:

   Ctrl+O

Opens an existent sprite in the disk.

Open Recent
-----------

Save
----

.. admonition:: Keyboard shortcut:

   Ctrl+S

Save As
-------

.. admonition:: Keyboard shortcut:

   Ctrl+Shift+S

Save Copy As
------------

.. admonition:: Keyboard shortcut:

   Ctrl+Shift+C

Close
-----

.. admonition:: Keyboard shortcut:

   Ctrl+W

Closes the current sprite. If there are modifications in the sprite, you will see a confirmation dialog.

``TODO close warning screenshot``

You can close sprites pressing the middle mouse button above a tab.

``TODO closing through tab + middle mouse button screenshot``

Close All
---------

.. admonition:: Keyboard shortcut:

   Ctrl+Shift+N

Capture
-------

Screen Shot
'''''''''''

Record Screen
'''''''''''''

Exit
----

Edit
====

Undo
----

.. admonition:: Keyboard shortcut:

   Ctrl+U

Redo
----

.. admonition:: Keyboard shortcut:

   Ctrl+R

Cut
---

.. admonition:: Keyboard shortcut:

   Ctrl+X

Copy
----

.. admonition:: Keyboard shortcut:

   Ctrl+C

Paste
-----

.. admonition:: Keyboard shortcut:

   Ctrl+V

Clear
-----

.. admonition:: Keyboard shortcut:

   Del (or Backspace)

This command has different behavior depending in which layer you use it:

 - In the background layer: If there are something selected, the
   selected region is cleared with the background color. If nothing
   is selected the entire cel is cleared with the background color.
 - In a transparent layer: If there are something selected, the
   selected region is cleared with transparent color. If nothing
   is selected the entire cel is removed from the layer, so an
   `empty cel`_ is left.

Flip Horizontal
---------------

.. admonition:: Keyboard shortcut:

   Shift+H

Flip Vertical
-------------

.. admonition:: Keyboard shortcut:

   Shift+V

Replace Color
-------------

.. admonition:: Keyboard shortcut:

   Shift+R

Invert
------

.. admonition:: Keyboard shortcut:

   Ctrl+I

Sprite
======

Properties
----------

Color Mode
----------

RGB Color
'''''''''

Grayscale
'''''''''

Indexed (No Dithering)
''''''''''''''''''''''

Indexed (Ordered Dither)
''''''''''''''''''''''''

Duplicate
---------

Sprite Size
-----------

Canvas Size
-----------

Rotate Canvas
-------------

180
'''

90 CW
'''''

90 CCW
''''''

Flip Canvas Horizontal
''''''''''''''''''''''

Flip Canvas Vertical
''''''''''''''''''''

Crop
----

Trim
----

Layer
=====

Properties
----------

New Layer
---------

Remove Layer
------------

Background from Layer
---------------------

Layer from Background
---------------------

Duplicate
---------

Merge Down
----------

Flatten
-------


Frame
=====

Properties
----------

New Frame
---------

Remove Frame
------------

Jump to
-------

First Frame
'''''''''''

Previous Frame
''''''''''''''

Next Frame
''''''''''

Last Frame
''''''''''

Play Animation
--------------

Cel
===

Properties
----------


Mask
====

All
---

Deselect
--------

Reselect
--------

Inverse
-------

Color Range
-----------

Load from MSK file
------------------

Save to MSK file
----------------

View
====

Tools
=====

Help
====

--------------------
 Keyboard Shortcuts
--------------------

---------------
 Customization
---------------

Here you have some explanation about how to customize your own copy of
ASE. Take care of all modifications you made in configuration files, and
make sure you have a backup copy of everything.

Most of the customizable UI elements are taken from ``<ase-folder>/data/gui.xml`` file.

Keyboard Shortcuts
==================

::

  <gui>

    <keyboard>
    </keyboard>

  </gui>

Menus
=====

::

  <gui>

    <menus>
    </menus>

  </gui>

Tools
=====

In the ``data/gui.xml`` file you will found a the following sections:

::

  <gui>

    <tools>
      <group ... >
        <tool ... />
      </group>
    </tools>

  </gui>

In the ``<tools>`` section you have the set of available tools in ASE
separated by groups (``<group>`` elements). Each group has a set of tools (``<tool>`` elements). 

<group>
-------

::

   <group id="..."
          text="...">

     <tool ... />
     <tool ... />
     <tool ... />
   </group>

<tool>
------

::

   <tool id="..."
         text="..."
         ink="..."
         controller="..."
         pointshape="..."
         intertwine="..."
       />

The ``id`` attribute is used to identify the tool, it must be unique
between all tools.  The ``text`` attribute specified the name of the
tool shown to the user.

The ``ink`` attribute indicates what the tool does, e.g. paint,
select, pick a color, etc. Available inks are:

 - selection
 - paint (paint with fg or bg, depending if left or right button was used)
 - paint_fg
 - paint_bg
 - eraser
 - replace_fg_with_bg
 - replace_bg_with_fg
 - pick_fg
 - pick_bg
 - scroll
 - move
 - blur
 - jumble

The ``controller`` specifies how mouse buttons are controlled.
Available controllers are:

 - freehand (e.g. pencil)
 - point_by_point (e.g. polygon)
 - one_point (e.g. paint bucket)
 - two_points (e.g. lines, rectangles, etc.)
 - four_points (e.g. bezier lines)

The ``pointshape`` is the way a mouse point will be converted to an area
in the image. Available points:

 - pixel
 - pen
 - floodfill
 - spray

The ``intertwiner`` says how mouse points should be joined.
Available intertwiners:

 - none
 - as_lines
 - as_bezier
 - as_rectangles
 - as_ellipses
