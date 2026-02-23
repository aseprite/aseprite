
#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/adjust_slice_keys.h"

#include "base/serialization.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/frame.h"
#include "doc/slice.h"
#include "doc/slice_io.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;
using namespace base::serialization::little_endian;
AdjustSliceKeys::AdjustSliceKeys(Sprite* sprite, int frame)
  : WithSprite(sprite), frame(frame), m_size(0)
{
}
DeleteSliceKeys::DeleteSliceKeys(Sprite* sprite, int frame)
  : AdjustSliceKeys(sprite, frame)
{
}
InsertSliceKeys::InsertSliceKeys(Sprite* sprite, int frame)
  : AdjustSliceKeys(sprite, frame)
{
}
MoveSliceKeys::MoveSliceKeys(Sprite* sprite, int frame, int targetFrame)
  : AdjustSliceKeys(sprite, frame), targetFrame(targetFrame)
{
}

void DeleteSliceKeys::onExecute()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysBack(sprite, frame);

  sprite->incrementVersion();
}
void InsertSliceKeys::onExecute()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysForward(sprite, frame);

  sprite->incrementVersion();
}

void MoveSliceKeys::onExecute()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysBack(sprite, frame);
  moveSliceKeysForward(sprite, targetFrame);

  // restore keys
  if (m_size > 0) {
    while (m_stream.tellp() > 0) {

      // read data from stream
      ObjectId sliceId = read32(m_stream);
      frame_t frameId = read32(m_stream);
      const SliceKey sk = read_slicekey(m_stream);

      // find slice
      for (Slice* slice : sprite->slices())
        if (slice->id() == sliceId)
          slice->insert(targetFrame, sk);
    }

    // reinitialize stream
    m_stream.str(std::string());
    m_stream.clear();
    m_size = 0;
  }

  sprite->incrementVersion();
}

void DeleteSliceKeys::onUndo()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysForward(sprite, frame);

  // restore deleted slices if present
  if (m_size > 0) {
    while (m_stream.tellp() > 0) {

      // read data from stream
      ObjectId sliceId = read32(m_stream);
      frame_t frameId = read32(m_stream);
      const SliceKey sk = read_slicekey(m_stream);

      // find slice
      for (Slice* slice : sprite->slices())
        if (slice->id() == sliceId)
          slice->insert(frameId, sk);
    }

    // reinitialize stream
    m_stream.str(std::string());
    m_stream.clear();
    m_size = 0;
  }

  sprite->incrementVersion();
}

void InsertSliceKeys::onUndo()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysBack(sprite, frame);

  sprite->incrementVersion();
}

void MoveSliceKeys::onUndo()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysBack(sprite, targetFrame);
  moveSliceKeysForward(sprite, frame);

  // restore deleted slices if present
  if (m_size > 0) {
    while (m_stream.tellp() > 0) {

      // read data from stream
      ObjectId sliceId = read32(m_stream);
      frame_t frameId = read32(m_stream);
      const SliceKey sk = read_slicekey(m_stream);

      // find slice
      for (Slice* slice : sprite->slices())
        if (slice->id() == sliceId)
          slice->insert(frame, sk);

    }

    // reinitialize stream
    m_stream.str(std::string());
    m_stream.clear();
    m_size = 0;
  }

  sprite->incrementVersion();
}

void AdjustSliceKeys::moveSliceKeysBack(Sprite* sprite, frame_t from)
{
  for (Slice* slice : sprite->slices()) {

    auto it = slice->getIteratorByFrame(from);
    if (it->frame() == from) {

      // store slice key
      const SliceKey* sk = slice->getByFrame(from);
      write32(m_stream, slice->id());
      write32(m_stream, from);
      write_slicekey(m_stream, *sk);
      m_size = size_t(m_stream.tellp());

      // find next key
      it++;
      auto nextFrame = it->frame();
      slice->remove(from);
      it = slice->getIteratorByFrame(nextFrame);
    } else if (it->frame() < from) {
      
      // advance to next key after frame
      it++;
    }

    for ( ;it != slice->end() && it->frame() >= from; it++) {
      frame_t f = it->frame();
      it->setFrame(f - 1);
    }

    slice->incrementVersion();
  }
}
void AdjustSliceKeys::moveSliceKeysForward(Sprite* sprite, frame_t from)
{
  for (Slice* slice : sprite->slices()) {
    
    for (auto it = slice->getIteratorByFrame(slice->toFrame()); it->frame() >= from; it--) {
      frame_t f = it->frame();
      it->setFrame(f + 1);
    }

    slice->incrementVersion();
  }
}

}} // namespace app::cmd
