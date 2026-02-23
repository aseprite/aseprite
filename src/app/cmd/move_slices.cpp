
#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/move_slices.h"

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
MoveSlices::MoveSlices(Sprite* sprite, int delta, int frame, int targetFrame)
  : WithSprite(sprite), delta(delta), frame(frame), targetFrame(targetFrame), m_size(0)
{
}

void MoveSlices::onExecute()
{
  Sprite* sprite = this->sprite();

  if (delta == -1) {

    // delete frame
    moveSlicesBack(sprite, frame);
  } else if (delta == 1) {

    // insert frame
    moveSlicesForward(sprite, frame);
  } else {

    // move frame
    moveSlicesBack(sprite, frame);
    moveSlicesForward(sprite, targetFrame);

    // restore keys
    if (m_size > 0) {
      while (m_stream.tellp() > 0) {

        // read data
        ObjectId sliceId = read32(m_stream);
        frame_t frameId = read32(m_stream);
        const SliceKey sk = read_slicekey(m_stream);

        // find slice
        for (Slice* slice : sprite->slices())
          if (slice->id() == sliceId)
            slice->insert(targetFrame, sk);
      }
      m_stream.str(std::string());
      m_stream.clear();
      m_size = 0;
    }
  }

  sprite->incrementVersion();
}

void MoveSlices::onUndo()
{
  Sprite* sprite = this->sprite();

  if (delta == -1) {

    // delete frame
    moveSlicesForward(sprite, frame);

    // restore deleted slices if present
    if (m_size > 0) {
      while (m_stream.tellp() > 0) {

        // read data
        ObjectId sliceId = read32(m_stream);
        frame_t frameId = read32(m_stream);
        const SliceKey sk = read_slicekey(m_stream);

        // find slice
        for (Slice* slice : sprite->slices())
          if (slice->id() == sliceId)
            slice->insert(frameId, sk);
      }
      m_stream.str(std::string());
      m_stream.clear();
      m_size = 0;
    }
  } else if (delta == 1) {

    // insert frame
    moveSlicesBack(sprite, frame);
  } else {

    // move frame
    moveSlicesBack(sprite, targetFrame);
    moveSlicesForward(sprite, frame);

    // restore keys
    if (m_size > 0) {
      while (m_stream.tellp() > 0) {

        // read data
        ObjectId sliceId = read32(m_stream);
        frame_t frameId = read32(m_stream);
        const SliceKey sk = read_slicekey(m_stream);

        // find slice
        for (Slice* slice : sprite->slices())
          if (slice->id() == sliceId)
            slice->insert(frame, sk);

      }
      m_stream.str(std::string());
      m_stream.clear();
      m_size = 0;
    }
  }

  sprite->incrementVersion();
}

void MoveSlices::moveSlicesBack(Sprite* sprite, frame_t frame)
{
  for (Slice* slice : sprite->slices()) {

    auto it = slice->getIteratorByFrame(frame);
    if (it->frame() == frame) {
      
      // has key on frame

      // store slice key
      const SliceKey* sk = slice->getByFrame(frame);
      write32(m_stream, slice->id());
      write32(m_stream, frame);
      write_slicekey(m_stream, *sk);
      m_size = size_t(m_stream.tellp());

      // find next key
      it++;
      auto nextFrame = it->frame();
      slice->remove(frame);
      it = slice->getIteratorByFrame(nextFrame);
    } else if (it->frame() < frame) {
      
      // key is before current frame
      it++;
    }

    while (it != slice->end() && frame <= it->frame()) {

      frame_t f = it->frame();

      // make new key from old key
      const SliceKey* sk = slice->getByFrame(f);
      SliceKey* k = new SliceKey(sk->bounds(), sk->center(), sk->pivot());
      
      // swap keys
      slice->remove(f);
      slice->insert(f - 1, *k);

      // find next key
      it = slice->getIteratorByFrame(f);
      it++;
      if (f >= it->frame())
        break;
    }

    slice->incrementVersion();
  }
}
void MoveSlices::moveSlicesForward(Sprite* sprite, frame_t frame)
{
  for (Slice* slice : sprite->slices()) {
    auto it = slice->getIteratorByFrame(slice->toFrame());
    while (it != slice->end() && frame <= it->frame()) {

      frame_t f = it->frame();

      // make new key from old key
      const SliceKey* sk = slice->getByFrame(f);
      SliceKey* k = new SliceKey(sk->bounds(), sk->center(), sk->pivot());
      
      // swap keys
      slice->remove(f);
      slice->insert(f + 1, *k);

      // find next key
      it = slice->getIteratorByFrame(f + 1);
      it--;
      if (frame > it->frame())
        break;
    }

    slice->incrementVersion();
  }
}

}} // namespace app::cmd
