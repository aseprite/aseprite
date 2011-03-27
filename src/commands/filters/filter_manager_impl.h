/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED
#define COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED

#include <cstring>
#include <stdlib.h>

#include "base/exception.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "document_wrappers.h"

class Filter;
class Image;
class Layer;
class Mask;
class Sprite;

class InvalidAreaException : public base::Exception
{
public:
  InvalidAreaException() throw()
  : base::Exception("The current mask/area to apply the effect is completelly invalid.") { }
};

class NoImageException : public base::Exception
{
public:
  NoImageException() throw()
  : base::Exception("There are not an active image to apply the effect.\n"
		    "Please select a layer/cel with an image and try again.") { }
};

class FilterManagerImpl : public FilterManager
			, public FilterIndexedData
{
public:
  // Interface to report progress to the user and take input from him
  // to cancel the whole process.
  class IProgressDelegate {
  public:
    virtual ~IProgressDelegate() { }

    // Called to report the progress of the filter (with progress from 0.0 to 1.0).
    virtual void reportProgress(float progress) = 0;

    // Should return true if the user wants to cancel the filter.
    virtual bool isCancelled() = 0;
  };

  FilterManagerImpl(Document* document, Filter* filter);
  ~FilterManagerImpl();

  void setProgressDelegate(IProgressDelegate* progressDelegate);

  int getImgType() const;

  void setTarget(Target target);

  void begin();
  void beginForPreview();
  bool applyStep();
  void applyToTarget();

  Document* getDocument() const { return m_document; }
  Sprite* getSprite() const { return m_sprite; }
  Image* getDestinationImage() const { return m_dst; }

  // Updates the current editor to show the progress of the preview.
  void flush();

  // FilterManager implementation
  const void* getSourceAddress();
  void* getDestinationAddress();
  int getWidth() { return m_w; }
  Target getTarget() { return m_target; }
  FilterIndexedData* getIndexedData() { return this; }
  bool skipPixel();
  const Image* getSourceImage() { return m_src; }
  int getX() { return m_x; }
  int getY() { return m_y+m_row; }

  // FilterIndexedData implementation
  Palette* getPalette();
  RgbMap* getRgbMap();
  
private:
  void init(const Layer* layer, Image* image, int offset_x, int offset_y);
  void apply();
  void applyToImage(Layer* layer, Image* image, int x, int y);
  bool updateMask(Mask* mask, const Image* image);

  Document* m_document;
  Sprite* m_sprite;
  Filter* m_filter;
  Image* m_src;
  Image* m_dst;
  int m_row;
  int m_x, m_y, m_w, m_h;
  int m_offset_x, m_offset_y;
  Mask* m_mask;
  Mask* m_preview_mask;
  unsigned char* m_mask_address;
  div_t m_d;
  Target m_targetOrig;		// Original targets
  Target m_target;		// Filtered targets

  // Hooks
  float m_progressBase;
  float m_progressWidth;
  IProgressDelegate* m_progressDelegate;
};

#endif
