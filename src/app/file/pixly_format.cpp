// Aseprite
// Copyright (C) 2016  Carlo "zED" Caputo
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
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

#include <cmath>
#include <cctype>

#include "png.h"

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

static void report_png_error(png_structp png_ptr, png_const_charp error)
{
  ((FileOp*)png_get_error_ptr(png_ptr))->setError("libpng: %s\n", error);
}

template<typename Any> static Any* check(Any* a, Any* alt = NULL) {
  if(a == NULL) {
    if(alt == NULL) {
      throw Exception("bad structure");
    } else {
      return alt;
    }
  } else {
    return a;
  }
}

template<typename Number> static Number check_number(const char* c_str) {
  if(c_str == NULL) {
    throw Exception("value not found");
  } else {
    std::string str = c_str;
    if(str.empty()) {
      throw Exception("value empty");
    }
    std::string::const_iterator it = str.begin();
    while (it != str.end() && (std::isdigit(*it) || *it == '.')) ++it;
    if(it != str.end()) {
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
    if(version < 1.5) {
      throw Exception("version 1.5 or above required");
    }

    TiXmlElement* xmlInfo = check(xmlAnim->FirstChild("Info"))->ToElement();

    int layerCount  = check_number<int>(xmlInfo->Attribute("layerCount"));
    int frameWidth  = check_number<int>(xmlInfo->Attribute("frameWidth"));
    int frameHeight = check_number<int>(xmlInfo->Attribute("frameHeight"));

    UniquePtr<Sprite> sprite(new Sprite(IMAGE_RGB, frameWidth, frameHeight, 0));

    TiXmlElement* xmlFrames = check(xmlAnim->FirstChild("Frames"))->ToElement();
    int imageCount = check_number<int>(xmlFrames->Attribute("length"));

    if(layerCount <= 0 || imageCount <= 0) {
      throw Exception("No cels found");
    }

    int frameCount = imageCount / layerCount;
    sprite->setTotalFrames(frame_t(frameCount));
    sprite->setDurationForAllFrames(200);

    for(int i=0; i<layerCount; i++) {
      sprite->folder()->addLayer(new LayerImage(sprite));
    }

    // load image sheet
    Document* png = load_document(nullptr, base::replace_extension(fop->filename(),"png").c_str());
    fop->setProgress(0.5);

    Image* sheet = check(check(check(check(check(png)->sprite())->layer(0))->cel(0))->image()); // do I need to check it all?

    if(sheet->pixelFormat() != IMAGE_RGB) {
      throw("Pixly loader requires a RGBA PNG");
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
      if(duration) {
        sprite->setFrameDuration(frame, base::convert_to<int>(std::string(duration)));
      }

      visible[(int)layer_index] += (int)(std::string(check(xmlFrame->Attribute("visible"),"false")) == "true");

      int x0 = check_number<int>(xmlRegion->Attribute("x"));
      int y0_up = check_number<int>(xmlRegion->Attribute("y")); // inverted

      if(y0_up < 0 || y0_up + frameHeight > sheetHeight || x0 < 0 || x0 + frameWidth > sheetWidth) {
        throw Exception("looking for cels outside the bounds of the PNG");
      }

      // TODO link identical neighbors
      // TODO if all transparent make empty
      // TODO trim cel image

      base::UniquePtr<Cel> cel;
      ImageRef image(Image::create(IMAGE_RGB, frameWidth, frameHeight));

      // Convert rows_pointer into the doc::Image
      for (int y = 0; y < frameHeight; y++) {
        // RGB_ALPHA
        int y0_down = sheetHeight-1 - y0_up - (frameHeight-1) + y;
        uint32_t* src_begin = (uint32_t*)sheet->getPixelAddress(x0           , y0_down);
        uint32_t* src_end   = (uint32_t*)sheet->getPixelAddress(x0+frameWidth, y0_down);
        uint32_t* dst_begin = (uint32_t*)image->getPixelAddress(0, y);

        std::copy(src_begin, src_end, dst_begin);
      }

      cel.reset(new Cel(frame, image));
      static_cast<LayerImage*>(layer)->addCel(cel);
      cel.release();

      xmlFrame = xmlFrame->NextSiblingElement();
      fop->setProgress(0.5 + 0.5 * ((float)(index+1) / (float)imageCount));
    }

    for(int i=0; i<layerCount; i++) {
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

  int width, height, y;
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytepp rows_pointer;
  int color_type = 0;

  // TODO use save_document(nullptr, tmpDoc)

  /* open the file */
  FileHandle xml_handle(open_file_with_exception(fop->filename(), "wb"));
  FILE* xml_fp = xml_handle.get();

  FileHandle handle(open_file_with_exception(base::replace_extension(fop->filename(),"png"), "wb"));
  FILE* fp = handle.get();

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also check that
   * the library version is compatible with the one used at compile time,
   * in case we are using dynamically linked libraries.  REQUIRED.
   */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)fop,
                                    report_png_error, report_png_error);
  if (png_ptr == NULL) {
    return false;
  }

  /* Allocate/initialize the image information data.  REQUIRED */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr, NULL);
    return false;
  }

  /* Set error handling.  REQUIRED if you aren't supplying your own
   * error handling functions in the png_create_write_struct() call.
   */
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* If we get here, we had a problem reading the file */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return false;
  }

  /* set up the output control if you are using standard C streams */
  png_init_io(png_ptr, fp);

  /* Set the image information here.  Width and height are up to 2^31,
   * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
   * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
   * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
   * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
   * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
   * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
   */

  int frameCount = sprite->totalFrames();
  int layerCount = sprite->folder()->getLayersCount();
  int imageCount = frameCount * layerCount;

  int frameWidth = sprite->width();
  int frameHeight = sprite->height();

  int squareSide = (int)ceil(sqrt(imageCount));

  width  = squareSide * frameWidth;
  height = squareSide * frameHeight;
  color_type = PNG_COLOR_TYPE_RGB_ALPHA;

  png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  /* Write the file header information. */
  png_write_info(png_ptr, info_ptr);

  /* pack pixels into bytes */
  png_set_packing(png_ptr);

  rows_pointer = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytep) * height);
  for (y = 0; y < height; y++) {
    size_t size = png_get_rowbytes(png_ptr, info_ptr);
    rows_pointer[y] = (png_bytep)png_malloc(png_ptr, size);
    memset(rows_pointer[y], 0, size);
    fop->setProgress(0.1 * (double)(y+1) / (double)height);
  }

  // TODO XXX beware the required typo on Pixly xml: "totalCollumns" (sic)
  fprintf(xml_fp,
    "<PixlyAnimation version=\"1.5\">\n"
    "\t<Info "
    "sheetWidth=\"%d\" sheetHeight=\"%d\" "
    "totalCollumns=\"%d\" totalRows=\"%d\" "
    "frameWidth=\"%d\" frameHeight=\"%d\" "
    "layerCount=\"%d\"/>\n"
    "\t<Frames length=\"%d\">\n",
    width, height,
    squareSide, squareSide,
    frameWidth, frameHeight,
    layerCount, imageCount
  );


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
      fprintf(xml_fp,
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

          for (y = 0; y < celHeight; y++) {
            /* RGB_ALPHA */
            uint32_t* src_address = (uint32_t*)image->getPixelAddress(0, y);
            uint8_t* dst_address = rows_pointer[(height - 1) - y0 - (frameHeight - 1) + celY + y] + ((x0 + celX) * 4);
            int x;
            unsigned int c;

            for (x=0; x<celWidth; x++) {
              c = *(src_address++);
              *(dst_address++) = rgba_getr(c);
              *(dst_address++) = rgba_getg(c);
              *(dst_address++) = rgba_getb(c);
              *(dst_address++) = rgba_geta(c);
            } // x
          } // y

        } // image
      } // cel

      fop->setProgress(0.1 + 0.8 * (double)(index+1) / (double)imageCount);

    } // layer
  } // frame

  fprintf(xml_fp,
      "\t</Frames>\n"
      "</PixlyAnimation>\n"
   );

  /* If you are only writing one row at a time, this works */
  for (y = 0; y < height; y++) {
    /* write the line */
    png_write_rows(png_ptr, rows_pointer+y, 1);

    fop->setProgress(0.9 + 0.1 * (double)(y+1) / (double)height);
  }

  for (y = 0; y < height; y++) {
    png_free(png_ptr, rows_pointer[y]);
  }
  png_free(png_ptr, rows_pointer);

  /* It is REQUIRED to call this to finish writing the rest of the file */
  png_write_end(png_ptr, info_ptr);

  /* clean up after the write, and free any memory allocated */
  png_destroy_write_struct(&png_ptr, &info_ptr);

  /* all right */
  return true;
}
#endif

} // namespace app
