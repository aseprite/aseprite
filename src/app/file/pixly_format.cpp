// Aseprite
// Copyright (C) 2016  Carlo "zED" Caputo
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//
// Based on the code of David Capello

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/xml_document.h"
#include "base/file_handle.h"
#include "base/convert_to.h"
#include "base/path.h"
#include "doc/doc.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/primitives.h"

#include <cmath>
#include <cctype>

namespace app {

using namespace base;

class PixlyFormat : public FileFormat {
  const char* onGetName() const override { return "anim"; }
  const char* onGetExtensions() const override { return "anim"; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_LAYERS |
      FILE_SUPPORT_FRAMES;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreatePixlyFormat()
{
  return new PixlyFormat;
}

template<typename Any> static Any* check(Any* a, Any* alt = NULL) {
  if (a == NULL) {
    if (alt == NULL) {
      throw Exception("bad structure");
    }
    else {
      return alt;
    }
  }
  else {
    return a;
  }
}

template<typename Number> static Number check_number(const char* c_str) {
  if (c_str == NULL) {
    throw Exception("value not found");
  }
  else {
    std::string str = c_str;
    if (str.empty()) {
      throw Exception("value empty");
    }
    std::string::const_iterator it = str.begin();
    while (it != str.end() && (std::isdigit(*it) || *it == '.')) ++it;
    if (it != str.end()) {
      throw Exception("value not a number");
    }
    return base::convert_to<Number>(str);
  }
}


bool PixlyFormat::onLoad(FileOp* fop)
{
  try {
    // load XML metadata
    XmlDocumentRef doc = open_xml(fop->filename());
    TiXmlHandle xml(doc.get());
    fop->setProgress(0.25);

    TiXmlElement* xmlAnim = check(xml.FirstChild("PixlyAnimation").ToElement());
    double version = check_number<double>(xmlAnim->Attribute("version"));
    if (version < 1.5) {
      throw Exception("version 1.5 or above required");
    }

    TiXmlElement* xmlInfo = check(xmlAnim->FirstChild("Info"))->ToElement();

    int layerCount  = check_number<int>(xmlInfo->Attribute("layerCount"));
    int frameWidth  = check_number<int>(xmlInfo->Attribute("frameWidth"));
    int frameHeight = check_number<int>(xmlInfo->Attribute("frameHeight"));

    UniquePtr<Sprite> sprite(new Sprite(IMAGE_RGB, frameWidth, frameHeight, 0));

    TiXmlElement* xmlFrames = check(xmlAnim->FirstChild("Frames"))->ToElement();
    int imageCount = check_number<int>(xmlFrames->Attribute("length"));

    if (layerCount <= 0 || imageCount <= 0) {
      throw Exception("No cels found");
    }

    int frameCount = imageCount / layerCount;
    sprite->setTotalFrames(frame_t(frameCount));
    sprite->setDurationForAllFrames(200);

    for (int i=0; i<layerCount; i++) {
      sprite->folder()->addLayer(new LayerImage(sprite));
    }

    // load image sheet
    Document* sheet_doc = load_document(nullptr, base::replace_extension(fop->filename(),"png").c_str());
    fop->setProgress(0.5);

    if (sheet_doc == NULL) {
      throw Exception("Pixly loader requires a valid PNG file");
    }

    Image* sheet = sheet_doc->sprite()->layer(0)->cel(0)->image();

    if (sheet->pixelFormat() != IMAGE_RGB) {
      throw Exception("Pixly loader requires a RGBA PNG");
    }

    int sheetWidth = sheet->width();
    int sheetHeight = sheet->height();

    // slice cels from sheet
    std::vector<int> visible(layerCount, 0);

    TiXmlElement* xmlFrame = check(xmlFrames->FirstChild("Frame"))->ToElement();
    while (xmlFrame) {
      TiXmlElement* xmlRegion = check(xmlFrame->FirstChild("Region"))->ToElement();
      TiXmlElement* xmlIndex = check(xmlFrame->FirstChild("Index"))->ToElement();

      int index = check_number<int>(xmlIndex->Attribute("linear"));
      frame_t frame(index / layerCount);
      LayerIndex layer_index(index % layerCount);
      Layer *layer = sprite->indexToLayer(layer_index);

      const char * duration = xmlFrame->Attribute("duration");
      if (duration) {
        sprite->setFrameDuration(frame, base::convert_to<int>(std::string(duration)));
      }

      visible[(int)layer_index] += (int)(std::string(check(xmlFrame->Attribute("visible"),"false")) == "true");

      int x0 = check_number<int>(xmlRegion->Attribute("x"));
      int y0_up = check_number<int>(xmlRegion->Attribute("y")); // inverted

      if (y0_up < 0 || y0_up + frameHeight > sheetHeight || x0 < 0 || x0 + frameWidth > sheetWidth) {
        throw Exception("looking for cels outside the bounds of the PNG");
      }

      // read cel images
      ImageRef image(Image::create(IMAGE_RGB, frameWidth, frameHeight));

      for (int y = 0; y < frameHeight; y++) {
        // RGB_ALPHA
        int y0_down = sheetHeight-1 - y0_up - (frameHeight-1) + y;
        uint32_t* src_begin = (uint32_t*)sheet->getPixelAddress(x0, y0_down);
        uint32_t* src_end   = src_begin + frameWidth;
        uint32_t* dst_begin = (uint32_t*)image->getPixelAddress(0, y);

        std::copy(src_begin, src_end, dst_begin);
      }

      // make cel trimmed or empty
      gfx::Rect bounds;
      if (algorithm::shrink_bounds(image.get(), bounds, image->maskColor())) {
        ImageRef trim_image(crop_image(image.get(),
                                  bounds.x, bounds.y,
                                  bounds.w, bounds.h,
                                  image->maskColor()));


        Cel* cel = NULL;
        if ((int)frame > 0) {
          // link identical neighbors
          Cel *prev_cel = static_cast<LayerImage*>(layer)->cel(frame-1);
          if (prev_cel && prev_cel->x() == bounds.x && prev_cel->y() == bounds.y) {
            Image *prev_image = prev_cel->image();
            if (prev_image && doc::count_diff_between_images(prev_image, trim_image.get()) == 0) {
              cel = Cel::createLink(prev_cel);
              cel->setFrame(frame);
            } // count_diff_between_images
          } // prev_cel
        } // frame > 0

        if (cel == NULL) {
          cel = new Cel(frame, trim_image);
          cel->setPosition(bounds.x, bounds.y);
        }

        static_cast<LayerImage*>(layer)->addCel(cel);

      }

      xmlFrame = xmlFrame->NextSiblingElement();
      fop->setProgress(0.5 + 0.5 * ((float)(index+1) / (float)imageCount));
    }

    for (int i=0; i<layerCount; i++) {
      LayerIndex layer_index(i);
      Layer *layer = sprite->indexToLayer(layer_index);
      layer->setVisible(visible[i] > frameCount/2);
    }

    fop->createDocument(sprite);
    sprite.release();
  }
  catch(Exception &e) {
    fop->setError((std::string("Pixly file format: ")+std::string(e.what())+"\n").c_str());
    return false;
  }

  return true;
}

#ifdef ENABLE_SAVE
bool PixlyFormat::onSave(FileOp* fop)
{
  const Sprite* sprite = fop->document()->sprite();

  auto it = sprite->folder()->getLayerBegin(),
       end = sprite->folder()->getLayerEnd();
  for (; it != end; ++it) { // layers
    Layer *layer = *it;
    if (!layer->isImage()) {
      fop->setError("Pixly .anim file format does not support layer folders\n");
      return false;
    }
  }

  // account sheet size
  int frameCount = sprite->totalFrames();
  int layerCount = sprite->folder()->getLayersCount();
  int imageCount = frameCount * layerCount;

  int frameWidth = sprite->width();
  int frameHeight = sprite->height();

  int squareSide = (int)ceil(sqrt(imageCount));

  int sheetWidth  = squareSide * frameWidth;
  int sheetHeight = squareSide * frameHeight;

  // create PNG document
  Sprite* sheet_sprite = new Sprite(IMAGE_RGB, sheetWidth, sheetHeight, 256);
  LayerImage* sheet_layer = new LayerImage(sheet_sprite);
  sheet_sprite->folder()->addLayer(sheet_layer);
  UniquePtr<Document> sheet_doc(new Document(sheet_sprite));
  ImageRef sheet_image(Image::create(IMAGE_RGB, sheetWidth, sheetHeight));
  Image* sheet = sheet_image.get();
  sheet_layer->addCel(new Cel(0, sheet_image));

  // save XML header
  FileHandle handle(open_file_with_exception(fop->filename(), "wb"));
  FILE* fp = handle.get();

  // TODO XXX beware the required typo on Pixly xml: "totalCollumns" (sic)
  fprintf(fp,
    "<PixlyAnimation version=\"1.5\">\n"
    "\t<Info "
    "sheetWidth=\"%d\" sheetHeight=\"%d\" "
    "totalCollumns=\"%d\" totalRows=\"%d\" "
    "frameWidth=\"%d\" frameHeight=\"%d\" "
    "layerCount=\"%d\"/>\n"
    "\t<Frames length=\"%d\">\n",
    sheetWidth, sheetWidth,
    squareSide, squareSide,
    frameWidth, frameHeight,
    layerCount, imageCount
  );

  // write cels on XML and PNG
  int index = 0;
  for (frame_t frame(0); frame<sprite->totalFrames(); ++frame) {
    auto it = sprite->folder()->getLayerBegin(),
         end = sprite->folder()->getLayerEnd();
    for (; it != end; ++it, ++index) { // layers
      Layer *layer = *it;

      int col = index % squareSide;
      int row = index / squareSide;

      int x0 = col * frameWidth;
      int y0 = row * frameHeight; // inverted

      int duration = sprite->frameDuration(frame);

      // TODO XXX beware the required typo on Pixly xml: "collumn" (sic)
      fprintf(fp,
        "\t\t<Frame duration=\"%d\" visible=\"%s\">\n"
        "\t\t\t<Region x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/>\n"
        "\t\t\t<Index linear=\"%d\" collumn=\"%d\" row=\"%d\"/>\n"
        "\t\t</Frame>\n",
        duration, layer->isVisible() ? "true" : "false",
        x0, y0, frameWidth, frameHeight,
        index, col, row
      );

      const Cel* cel = layer->cel(frame);
      if (cel) {
        const Image* image = cel->image();
        if (image) {
          int celX = cel->x();
          int celY = cel->y();
          int celWidth = image->width();
          int celHeight = image->height();

          for (int y = 0; y < celHeight; y++) {
            // RGB_ALPHA
            int y0_down = (sheetHeight - 1) - y0 - (frameHeight - 1) + celY + y;
            uint32_t* src_begin = (uint32_t*)image->getPixelAddress(0, y);
            uint32_t* src_end   = src_begin + celWidth;
            uint32_t* dst_begin = (uint32_t*)sheet->getPixelAddress(x0 + celX, y0_down);

            std::copy(src_begin, src_end, dst_begin);
          }

        } // image
      } // cel

      fop->setProgress(0.25 + 0.5 * (double)(index+1) / (double)imageCount);

    } // layer
  } // frame

  // close files
  fprintf(fp,
      "\t</Frames>\n"
      "</PixlyAnimation>\n"
   );

  sheet_doc->setFilename(base::replace_extension(fop->filename(),"png"));
  save_document(nullptr, sheet_doc);

  return true;
}
#endif

} // namespace app
