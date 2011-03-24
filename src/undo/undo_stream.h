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

#ifndef UNDO_UNDO_STREAM_H_INCLUDED
#define UNDO_UNDO_STREAM_H_INCLUDED

class UndoStream
{
public:
  typedef ChunksList::iterator iterator;

  UndoStream(UndoHistory* undo)
  {
    m_undo = undo;
    m_size = 0;
  }

  ~UndoStream()
  {
    clear();
  }

  UndoHistory* getUndo() const
  {
    return m_undo;
  }

  iterator begin() { return m_chunks.begin(); }
  iterator end() { return m_chunks.end(); }
  bool empty() const { return m_chunks.empty(); }

  void clear()
  {
    iterator it = begin();
    iterator end = this->end();
    for (; it != end; ++it)
      undo_chunk_free(*it);

    m_size = 0;
    m_chunks.clear();
  }

  int getMemSize() const
  {
    return m_size;
  }

  UndoChunk* popChunk(bool tail)
  {
    UndoChunk* chunk;
    iterator it;

    if (!empty()) {
      if (!tail)
	it = begin();
      else
	it = --end();

      chunk = *it;
      m_chunks.erase(it);
      m_size -= chunk->size;
    }
    else
      chunk = NULL;

    return chunk;
  }

  void pushChunk(UndoChunk* chunk)
  {
    m_chunks.insert(begin(), chunk);
    m_size += chunk->size;
  }

private:
  UndoHistory* m_undo;
  ChunksList m_chunks;
  int m_size;
};

#endif
