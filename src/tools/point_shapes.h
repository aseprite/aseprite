/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

class NonePointShape : public ToolPointShape
{
public:
  void transformPoint(IToolLoop* loop, int x, int y)
  {
    // Do nothing
  }
  void getModifiedArea(IToolLoop* loop, int x, int y, Rect& area)
  {
    // Do nothing
  }
};

class PixelPointShape : public ToolPointShape
{
public:
  void transformPoint(IToolLoop* loop, int x, int y)
  {
    doInkHline(x, y, x, loop);
  }
  void getModifiedArea(IToolLoop* loop, int x, int y, Rect& area)
  {
    area = Rect(x, y, 1, 1);
  }
};

class PenPointShape : public ToolPointShape
{
public:
  void transformPoint(IToolLoop* loop, int x, int y)
  {
    Pen* pen = loop->getPen();
    register PenScanline *scanline = pen->get_scanline();
    register int h = pen->get_size();
    register int c = h/2;

    x -= c;
    y -= c;

    for (c=0; c<h; c++) {
      if (scanline->state)
	doInkHline(x+scanline->x1, y+c, x+scanline->x2, loop);
      ++scanline;
    }
  }
  void getModifiedArea(IToolLoop* loop, int x, int y, Rect& area)
  {
    Pen* pen = loop->getPen();
    int size = pen->get_size();
    area = Rect(x-size/2, y-size/2, size, size);
  }
};

class FloodFillPointShape : public ToolPointShape
{
public:
  bool isFloodFill() { return true; }

  void transformPoint(IToolLoop* loop, int x, int y)
  {
    algo_floodfill(loop->getSrcImage(), x, y, loop->getTolerance(), loop, (AlgoHLine)doInkHline);
  }
  void getModifiedArea(IToolLoop* loop, int x, int y, Rect& area)
  {
    area = Rect(0, 0, 9999, 9999);
  }
};

class SprayPointShape : public ToolPointShape
{
  PenPointShape m_subPointShape;

public:

  bool isSpray() { return true; }

  void transformPoint(IToolLoop* loop, int x, int y)
  {
    int spray_width = loop->getSprayWidth();
    int spray_speed = loop->getSpraySpeed();
    int c, u, v, times = (spray_width*spray_width/4) * spray_speed / 100;

    // In Windows, rand() has a RAND_MAX too small
#if RAND_MAX <= 0xffff
    fixed angle, radius;

    for (c=0; c<times; c++) {
      angle = itofix(rand() * 256 / RAND_MAX);
      radius = itofix(rand() * (spray_width*10) / RAND_MAX) / 10;
      u = fixtoi(fixmul(radius, fixcos(angle)));
      v = fixtoi(fixmul(radius, fixsin(angle)));

      m_subPointShape.transformPoint(loop, x+u, y+v);
    }
#else
    fixed angle, radius;

    for (c=0; c<times; c++) {
      angle = rand();
      radius = rand() % itofix(spray_width);
      u = fixtoi(fixmul(radius, fixcos(angle)));
      v = fixtoi(fixmul(radius, fixsin(angle)));
      m_subPointShape.transformPoint(loop, x+u, y+v);
    }
#endif
  }

  void getModifiedArea(IToolLoop* loop, int x, int y, Rect& area)
  {
    int spray_width = loop->getSprayWidth();
    Point p1(x-spray_width, y-spray_width);
    Point p2(x+spray_width, y+spray_width);

    Rect area1;
    Rect area2;
    m_subPointShape.getModifiedArea(loop, p1.x, p1.y, area1);
    m_subPointShape.getModifiedArea(loop, p2.x, p2.y, area2);

    area = area1.createUnion(area2);
  }
};
