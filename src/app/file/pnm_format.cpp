// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef HAVE_CONFIG
  #include "config.h"
#endif

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>

namespace app {

using namespace base;

enum PnmSubformat : int8_t {
  PNM_PLAIN = 0,
  PNM_BINARY = 3,
};

enum PnmFormat : int8_t {
  FORMAT_PBM = 1,
  FORMAT_PGM = 2,
  FORMAT_PPM = 3,
  FORMAT_PAM = 4,
};

// Shared among PBM, PGM, PPM, but not PAM
class PnmOptions : public FormatOptions {
public:
  PnmOptions() : subformat(PnmSubformat::PNM_BINARY) {}
  PnmSubformat subformat;
};

class PbmFormat : public FileFormat {
  const char* onGetName() const override { return "pbm"; }

  void onGetExtensions(base::paths& exts) const override { exts.push_back("pbm"); }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::PBM_IMAGE; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_ENCODE_ABSTRACT_IMAGE |
           FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
};

class PgmFormat : public FileFormat {
  const char* onGetName() const override { return "pgm"; }

  void onGetExtensions(base::paths& exts) const override { exts.push_back("pgm"); }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::PGM_IMAGE; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_SUPPORT_GRAY | FILE_ENCODE_ABSTRACT_IMAGE |
           FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
};

class PpmFormat : public FileFormat {
  const char* onGetName() const override { return "ppm"; }

  void onGetExtensions(base::paths& exts) const override { exts.push_back("ppm"); }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::PPM_IMAGE; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_SUPPORT_RGB | FILE_ENCODE_ABSTRACT_IMAGE |
           FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
};

class PamFormat : public FileFormat {
  const char* onGetName() const override { return "pam"; }

  void onGetExtensions(base::paths& exts) const override
  {
    exts.push_back("pnm");
    exts.push_back("pam");
  }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::PAM_IMAGE; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_SUPPORT_RGB | FILE_SUPPORT_RGBA |
           FILE_SUPPORT_GRAY | FILE_SUPPORT_GRAYA | FILE_ENCODE_ABSTRACT_IMAGE;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreatePbmFormat()
{
  return new PbmFormat;
}

FileFormat* CreatePgmFormat()
{
  return new PgmFormat;
}

FileFormat* CreatePpmFormat()
{
  return new PpmFormat;
}

FileFormat* CreatePamFormat()
{
  return new PamFormat;
}

static int ppmMagic(std::ifstream& istream, FileOp* fop)
{
  char magic[3];
  istream.get(magic, 3);
  if (magic[0] == 'P' && '1' <= magic[1] && magic[1] <= '7') {
    return magic[1];
  }
  fop->setError("Not a PNM file\n");
  return 0;
}

// skip whitespace *or* comments in file header
static void whitespace(std::ifstream& istream)
{
  while (istream && !istream.eof()) {
    char c = istream.peek();
    if (c == '#') {
      istream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    else if (std::iswspace(c)) {
      istream.get();
    }
    else {
      break;
    }
  }
  if (istream.eof()) {
    istream.setstate(std::ios_base::failbit);
  }
}

class PlainLoader {
  int readBytes;
  int readVals;
  int totalVals;

public:
  PlainLoader(int totalVals) : readBytes(0), readVals(0), totalVals(totalVals) {}

  // seek to next non-whitespace position in file, skipping whitespace;
  // this handles showing progress and stopping early
  bool nextVal(std::ifstream& istream, FileOp* fop)
  {
    if (!istream) {
      return false;
    }

    readBytes += istream.gcount();
    char c;
    while (true) {
      // totally arbitrary, check progress every 4KiB
      if ((readBytes & 4095) == 0) {
        fop->setProgress((double)readVals / (double)totalVals);
        if (fop->isStop()) {
          return false;
        }
      }

      c = istream.peek();
      if (!std::iswspace(c)) {
        readVals += 1;
        return true;
      }
      istream.get();
      if (!istream || istream.eof()) {
        istream.setstate(std::ios_base::failbit);
        return false;
      }
      readBytes += 1;
    }
  }
};

static ImageRef loadPbmHeader(std::ifstream& istream, FileOp* fop)
{
  int width;
  int height;
  whitespace(istream);
  istream >> width;
  whitespace(istream);
  istream >> height;
  if (!istream || width <= 0 || height <= 0) {
    fop->setError("Invalid image size\n");
    return nullptr;
  }
  if (!std::iswspace(istream.get())) {
    fop->setError("Invalid PNM header\n");
    return nullptr;
  }

  // FIXME: IMAGE_BITMAP not supported
  ImageRef image = fop->sequenceImageToLoad(IMAGE_INDEXED, width, height);
  fop->sequenceSetNColors(2);
  fop->sequenceSetColor(0, 0, 0, 0);
  fop->sequenceSetColor(1, 255, 255, 255);
  return image;
}

// returns (width, height, maxval)
// maxval = 0 indicates error
static std::tuple<int, int, uint16_t> loadPgmPpmHeader(std::ifstream& istream, FileOp* fop)
{
  int width;
  int height;
  uint16_t maxval;
  whitespace(istream);
  istream >> width;
  whitespace(istream);
  istream >> height;
  if (!istream || width <= 0 || height <= 0) {
    fop->setError("Invalid image size\n");
    return std::tuple(0, 0, 0);
  }
  whitespace(istream);
  istream >> maxval;

  // note: we could *technically* support maxval > 255 by just making a big,
  // arbitrary palette, but since most people who do this want 16-bit grayscale,
  // we're just going to disallow it
  if (!istream || maxval == 0 || maxval > 255) {
    fop->setError("Unsupported MAXVAL = %d for PNM image\n", maxval);
    return std::tuple(0, 0, 0);
  }
  if (!std::iswspace(istream.get())) {
    fop->setError("Invalid PNM header\n");
    return std::tuple(0, 0, 0);
  }

  return std::tuple(width, height, maxval);
}

// returns (image, maxval)
static std::tuple<ImageRef, uint16_t> loadPgmHeader(std::ifstream& istream,
                                                    FileOp* fop,
                                                    PnmFormat format)
{
  if (format == PnmFormat::FORMAT_PBM) {
    fop->setError("Cannot load PGM image as bitmap\n");
    return std::tuple(nullptr, 0);
  }

  auto header = loadPgmPpmHeader(istream, fop);
  int width = std::get<0>(header);
  int height = std::get<1>(header);
  uint16_t maxval = std::get<2>(header);
  if (maxval == 0) {
    return std::tuple(nullptr, 0);
  }

  PixelFormat fmt;
  switch (maxval) {
    case 1:
      // FIXME: IMAGE_BITMAP not supported
      fmt = IMAGE_INDEXED;
      fop->sequenceSetNColors(2);
      fop->sequenceSetColor(0, 0, 0, 0);
      fop->sequenceSetColor(1, 255, 255, 255);
      break;
    case 255: fmt = IMAGE_GRAYSCALE; break;
    default:
      // the file doesn't provide a palette,
      // but assume the user will add their own after opening
      fmt = IMAGE_INDEXED;
      fop->sequenceSetNColors(maxval);
      for (int c = 0; c < maxval; c++) {
        fop->sequenceSetColor(c, c, c, c);
      }
  }

  ImageRef image = fop->sequenceImageToLoad(fmt, width, height);
  return std::tuple(image, maxval);
}

// doesn't need maxval because we require maxval = 255
static ImageRef loadPpmHeader(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  if (format < PnmFormat::FORMAT_PPM) {
    fop->setError("Cannot load PPM image as %s\n",
                  format == PnmFormat::FORMAT_PBM ? "bitmap" : "grayscale");
    return nullptr;
  }

  auto header = loadPgmPpmHeader(istream, fop);
  int width = std::get<0>(header);
  int height = std::get<1>(header);
  uint16_t maxval = std::get<2>(header);
  if (maxval == 0) {
    return nullptr;
  }

  if (maxval != 255) {
    fop->setError("Unsupported MAXVAL = %d for PPM image\n", maxval);
    return nullptr;
  }

  ImageRef image = fop->sequenceImageToLoad(IMAGE_RGB, width, height);
  return image;
}

enum TuplType : uint8_t {
  BLACKANDWHITE,
  BLACKANDWHITE_ALPHA,
  GRAYSCALE,
  GRAYSCALE_ALPHA,
  RGB,
  RGB_ALPHA,
};

// returns (image, type, maxval)
// we only need maxval for grayscale
static std::tuple<ImageRef, TuplType, uint16_t> loadPamHeader(std::ifstream& istream,
                                                              FileOp* fop,
                                                              PnmFormat format)
{
  std::optional<int> width;
  std::optional<int> height;
  std::optional<uint16_t> depth;
  std::optional<uint16_t> maxval;
  int tuplTypeLen = 0;
  char tuplType[21];

  while (true) {
    whitespace(istream);
    char cmd[9];
    int cmdLen;
    for (cmdLen = 0; cmdLen < 9; ++cmdLen) {
      cmd[cmdLen] = istream.get();
      if (!istream || istream.eof()) {
        fop->setError("Invalid PAM header\n");
        return std::tuple(nullptr, RGB, 0);
      }
      if (std::iswspace(cmd[cmdLen])) {
        break;
      }
    }

    if (cmdLen > 8) {
      cmd[8] = '\0';
      fop->setError("Unknown PAM directive: \"%s...\"\n", cmd);
      return std::tuple(nullptr, RGB, 0);
    }

    cmd[cmdLen] = '\0';

    bool valid = false;
    switch (cmd[0]) {
      case 'W':
        if (strcmp(cmd, "WIDTH") == 0) {
          if (width.has_value()) {
            fop->setError("PAM image specified multiple WIDTH\n");
            return std::tuple(nullptr, RGB, 0);
          }
          valid = true;
          istream >> width.emplace();
        }
        break;
      case 'H':
        if (strcmp(cmd, "HEIGHT") == 0) {
          if (height.has_value()) {
            fop->setError("PAM image specified multiple HEIGHT\n");
            return std::tuple(nullptr, RGB, 0);
          }
          valid = true;
          istream >> height.emplace();
        }
        break;
      case 'D':
        if (strcmp(cmd, "DEPTH") == 0) {
          if (depth.has_value()) {
            fop->setError("PAM image specified multiple DEPTH\n");
            return std::tuple(nullptr, RGB, 0);
          }
          valid = true;
          istream >> depth.emplace();
        }
        break;
      case 'M':
        if (strcmp(cmd, "MAXVAL") == 0) {
          if (maxval.has_value()) {
            fop->setError("PAM image specified multiple MAXVAL\n");
            return std::tuple(nullptr, RGB, 0);
          }
          valid = true;
          istream >> maxval.emplace();
        }
        break;
      case 'T':
        if (strcmp(cmd, "TUPLTYPE") == 0) {
          valid = true;

          // maximum type we care about is 19 characters,
          // and the spec defines the types as being concatenated if specified
          // multiple times
          if (tuplTypeLen < 20) {
            istream.gcount();
            istream.getline(tuplType + tuplTypeLen, 20 - tuplTypeLen, '\n');
            tuplTypeLen += istream.gcount();

            // if this happens, we *didn't* read a new line,
            // since that got included in the count;
            // it also means that we have read over 19 characters
            if (tuplType[tuplTypeLen - 1] != '\0') {
              istream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
          }
        }
        break;
      case 'E':
        if (strcmp(cmd, "ENDHDR") == 0) {
          goto endhdr;
        }
        break;

      default:;
    }

    if (!istream || istream.eof()) {
      fop->setError("Invalid PAM header\n");
      return std::tuple(nullptr, RGB, 0);
    }

    if (!valid) {
      fop->setError("Unknown PAM directive \"%s\"\n", cmd);
      return std::tuple(nullptr, RGB, 0);
    }
  }
endhdr:

  if (!istream || istream.eof()) {
    fop->setError("Invalid PAM header\n");
    return std::tuple(nullptr, RGB, 0);
  }

  if (!width.has_value() || !height.has_value() || !depth.has_value() || !maxval.has_value()) {
    fop->setError("PAM image did not provide all fields\n");
    return std::tuple(nullptr, RGB, 0);
  }

  if (width <= 0 || height <= 0) {
    fop->setError("Invalid image size\n");
    return std::tuple(nullptr, RGB, 0);
  }

  // see comment in loadPgmPpmHeader about maxval > 255
  if (maxval <= 0 || maxval > 255) {
    fop->setError("Unsupported MAXVAL = %d in PAM image\n", maxval);
    return std::tuple(nullptr, RGB, 0);
  }

  // verify TUPLTYPE
  tuplType[tuplTypeLen] = '\0';
  TuplType knownType;
  switch (tuplType[0]) {
    case 'B':
      if (strcmp(tuplType, "BLACKANDWHITE") == 0) {
        knownType = BLACKANDWHITE;
      }
      else if (strcmp(tuplType, "BLACKANDWHITE_ALPHA") == 0) {
        knownType = BLACKANDWHITE_ALPHA;
      }
      else {
        break;
      }
      if (maxval != 1) {
        fop->setError("PAM image had TUPLTYPE = %s but MAXVAL = %d\n", tuplType, maxval.value());
        return std::tuple(nullptr, RGB, 0);
      }
      if (depth != 1 + (knownType == BLACKANDWHITE ? 0 : 1)) {
        fop->setError("PAM image had TUPLTYPE = %s but DEPTH = %d\n", tuplType, depth.value());
        return std::tuple(nullptr, RGB, 0);
      }
      goto validType;
    case 'G':
      if (strcmp(tuplType, "GRAYSCALE") == 0) {
        knownType = GRAYSCALE;
      }
      else if (strcmp(tuplType, "GRAYSCALE_ALPHA") == 0) {
        knownType = GRAYSCALE_ALPHA;
      }
      else {
        break;
      }
      if (depth != 1 + (knownType == GRAYSCALE ? 0 : 1)) {
        fop->setError("PAM image had TUPLTYPE = %s but DEPTH = %d\n", tuplType, depth.value());
        return std::tuple(nullptr, RGB, 0);
      }

      // only GRAYSCALE gets custom maxval, not GRAYSCALE_ALPHA
      if (knownType == GRAYSCALE_ALPHA && maxval != 255) {
        fop->setError("PAM image had TUPLTYPE = %s but MAXVAL = %d\n", tuplType, depth.value());
        return std::tuple(nullptr, RGB, 0);
      }
      goto validType;
    case 'R':
      if (strcmp(tuplType, "RGB") == 0) {
        knownType = RGB;
      }
      else if (strcmp(tuplType, "RGB_ALPHA") == 0) {
        knownType = RGB_ALPHA;
      }
      else {
        break;
      }
      if (depth != 3 + (knownType == RGB ? 0 : 1)) {
        fop->setError("PAM image had TUPLTYPE = %s but DEPTH = %d\n", tuplType, depth.value());
        return std::tuple(nullptr, RGB, 0);
      }
      if (maxval != 255) {
        fop->setError("PAM image had TUPLTYPE = %s but MAXVAL = %d\n", tuplType, depth.value());
        return std::tuple(nullptr, RGB, 0);
      }
      goto validType;

    default:;
  }

  fop->setError("PAM image had unknown TUPLTYPE = %s\n", tuplType);
  return std::tuple(nullptr, RGB, 0);

validType:
  PixelFormat fmt;
  switch (knownType) {
    // alpha requires PAM format
    case BLACKANDWHITE_ALPHA:
    case GRAYSCALE_ALPHA:
    case RGB_ALPHA:
      if (format != FORMAT_PAM) {
        fop->setError("Cannot load %s image as non-PAM image\n", tuplType);
        return std::tuple(nullptr, RGB, 0);
      }
      break;

    // always okay
    case BLACKANDWHITE: break;

    // grayscale can't be loaded if we wanted a bitmap
    case GRAYSCALE:
      if (format == FORMAT_PBM) {
        fop->setError("Cannot load GRAYSCALE image as bitmap image\n");
        return std::tuple(nullptr, RGB, 0);
      }
      break;

    // RGB can't be loaded if we wanted grayscale or bitmap
    case RGB:
      if (format < FORMAT_PPM) {
        fop->setError("Cannot load %s image as %s image\n",
                      tuplType,
                      format == FORMAT_PBM ? "bitmap" : "grayscale");
        return std::tuple(nullptr, RGB, 0);
      }
  }
  switch (knownType) {
    case BLACKANDWHITE: fmt = IMAGE_BITMAP; break;

    // Aseprite doesn't have bitmap + alpha, so, give it a special palette
    // of (black + transparent, white + transparent, black, white)
    case BLACKANDWHITE_ALPHA:
      fmt = IMAGE_INDEXED;
      fop->sequenceSetHasAlpha(true);
      fop->sequenceSetNColors(4);
      fop->sequenceSetColor(0, 0, 0, 0);
      fop->sequenceSetAlpha(0, 0);
      fop->sequenceSetColor(1, 255, 255, 255);
      fop->sequenceSetAlpha(0, 0);
      fop->sequenceSetColor(2, 0, 0, 0);
      fop->sequenceSetAlpha(2, 1);
      fop->sequenceSetColor(3, 255, 255, 255);
      fop->sequenceSetAlpha(3, 1);
      break;

    case GRAYSCALE_ALPHA: fop->sequenceSetHasAlpha(true); [[fallthrough]];
    case GRAYSCALE:       fmt = IMAGE_GRAYSCALE; break;

    case RGB_ALPHA:       fop->sequenceSetHasAlpha(true); [[fallthrough]];
    case RGB:             fmt = IMAGE_RGB;
  }

  ImageRef image = fop->sequenceImageToLoad(fmt, width.value(), height.value());
  return std::tuple(image, knownType, maxval.value());
}

// P1 magic
static bool loadPbmPlain(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  ImageRef image = loadPbmHeader(istream, fop);
  if (!image) {
    return false;
  }

  auto loader = PlainLoader(image->width() * image->height());

  for (int row = 0; row < image->height(); ++row) {
    uint8_t* pixelAddress = image->getPixelAddress(0, row);
    for (int col = 0; col < image->width(); ++col) {
      if (!loader.nextVal(istream, fop)) {
        fop->setError("Failed to load pixel (%d, %d)\n", col, row);
        return false;
      }

      char pixel = istream.get();
      if (!istream || (pixel != '0' && pixel != '1')) {
        fop->setError("Failed to load pixel (%d, %d)\n", col, row);
        return false;
      }

      // PBM uses 0 for white, 1 for black
      *(pixelAddress++) = (uint8_t)(pixel == '0');
    }
  }

  if (format == FORMAT_PAM) {
  }
  else {
    auto opts = fop->formatOptionsOfDocument<PnmOptions>();
    opts->subformat = PnmSubformat::PNM_PLAIN;
    fop->setLoadedFormatOptions(opts);
  }

  return true;
}

// P4 magic
static bool loadPbmBinary(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  ImageRef image = loadPbmHeader(istream, fop);
  if (!image) {
    return false;
  }

  // PBM pads rows to a whole number of bytes
  int widthBytes = (image->width() / 8) + (int)(image->width() % 8 > 0);
  char* rowBuffer = (char*)malloc(widthBytes);
  for (int row = 0; row < image->height(); ++row) {
    istream.get(rowBuffer, sizeof(rowBuffer));
    if (!istream) {
      fop->setError("Failed to load row %d\n", row);
      return false;
    }

    uint8_t* pixel = image->getPixelAddress(0, row);
    uint8_t* rowCursor = (uint8_t*)rowBuffer;

    // load full bytes
    for (int b = 0; b < image->width() / 8; ++b) {
      for (int bit = 0; bit < 8; ++bit) {
        // PBM uses 0 for white, 1 for black
        *(pixel++) = (uint8_t)((((*rowCursor) >> bit) & 1) == 0);
      }
      ++rowCursor;
    }

    // load last, potentially partial byte
    for (int bit = 7; bit >= (image->width() % 8); --bit) {
      // PBM uses 0 for white, 1 for black
      *(pixel++) = (uint8_t)((((*rowCursor) >> bit) & 1) == 0);
    }

    fop->setProgress(((double)row / (double)image->height()));
    if (fop->isStop()) {
      break;
    }
  }

  free(rowBuffer);

  if (format == FORMAT_PAM) {
  }
  else {
    auto opts = fop->formatOptionsOfDocument<PnmOptions>();
    opts->subformat = PnmSubformat::PNM_BINARY;
    fop->setLoadedFormatOptions(opts);
  }
  return true;
}

// P2 magic
static bool loadPgmPlain(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  auto header = loadPgmHeader(istream, fop, format);
  ImageRef image = std::get<0>(header);
  int maxval = std::get<1>(header);
  if (!image) {
    return false;
  }

  auto loader = PlainLoader(image->width() * image->height());

  for (int row = 0; row < image->height(); ++row) {
    uint16_t* pixel = (uint16_t*)image->getPixelAddress(0, row);
    for (int col = 0; col < image->width(); ++col) {
      if (!loader.nextVal(istream, fop)) {
        fop->setError("Failed to load pixel (%d, %d)\n", col, row);
        return false;
      }

      uint16_t val;
      istream >> val;
      if (!istream || val > maxval) {
        fop->setError("Failed to load pixel (%d, %d)\n", col, row);
        return false;
      }

      *(pixel++) = graya(val, 255);
    }
  }

  if (format == FORMAT_PAM) {
  }
  else {
    auto opts = fop->formatOptionsOfDocument<PnmOptions>();
    opts->subformat = PnmSubformat::PNM_PLAIN;
    fop->setLoadedFormatOptions(opts);
  }

  return true;
}

// P5 magic
static bool loadPgmBinary(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  auto header = loadPgmHeader(istream, fop, format);
  ImageRef image = std::get<0>(header);
  uint16_t maxval = std::get<1>(header);
  if (!image) {
    return false;
  }

  char* rowBuffer = (char*)malloc(image->width());
  for (int row = 0; row < image->height(); ++row) {
    istream.get(rowBuffer, sizeof(rowBuffer));
    if (!istream) {
      fop->setError("Failed to load row %d\n", row);
      return false;
    }

    // we can't directly put the pixels since there's no alpha
    uint8_t* rowCursor = (uint8_t*)rowBuffer;
    uint16_t* pixel = (uint16_t*)image->getPixelAddress(0, row);
    for (int col = 0; col < image->width(); ++col) {
      if (*rowCursor > maxval) {
        fop->setError("Failed to load pixel (%d, %d)\n", col, row);
        return false;
      }
      *(pixel++) = graya(*(rowCursor++), 255);
    }

    fop->setProgress(((double)row / (double)image->height()));
    if (fop->isStop()) {
      break;
    }
  }

  free(rowBuffer);

  if (format == FORMAT_PAM) {
  }
  else {
    auto opts = fop->formatOptionsOfDocument<PnmOptions>();
    opts->subformat = PnmSubformat::PNM_BINARY;
    fop->setLoadedFormatOptions(opts);
  }

  return true;
}

// P3 magic
static bool loadPpmPlain(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  ImageRef image = loadPpmHeader(istream, fop, format);
  if (!image) {
    return false;
  }

  auto loader = PlainLoader(image->width() * image->height() * 3);

  for (int row = 0; row < image->height(); ++row) {
    uint32_t* pixel = (uint32_t*)image->getPixelAddress(0, row);
    for (int col = 0; col < image->width(); ++col) {
      if (!loader.nextVal(istream, fop)) {
        fop->setError("Failed to load pixel (%d, %d)\n", col, row);
        return false;
      }

      uint16_t r, g, b;
      istream >> r >> g >> b;
      if (!istream || r > 255 || g > 255 || b > 255) {
        fop->setError("Failed to load pixel (%d, %d)\n", col, row);
        return false;
      }
      *(pixel++) = rgba(r, g, b, 255);
    }
  }

  if (format == FORMAT_PAM) {
  }
  else {
    auto opts = fop->formatOptionsOfDocument<PnmOptions>();
    opts->subformat = PnmSubformat::PNM_PLAIN;
    fop->setLoadedFormatOptions(opts);
  }

  return true;
}

// P6 magic
static bool loadPpmBinary(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  ImageRef image = loadPpmHeader(istream, fop, format);
  if (!image) {
    return false;
  }

  char* rowBuffer = (char*)malloc((size_t)image->width() * 3);
  for (int row = 0; row < image->height(); ++row) {
    istream.get(rowBuffer, sizeof(rowBuffer));
    if (!istream) {
      fop->setError("Failed to load row %d\n", row);
      return false;
    }

    uint32_t* pixel = (uint32_t*)image->getPixelAddress(0, row);
    uint8_t* rowCursor = (uint8_t*)rowBuffer;
    for (int col = 0; col < image->width(); ++col) {
      uint8_t r, g, b;
      r = *(rowCursor++);
      g = *(rowCursor++);
      b = *(rowCursor++);
      *(pixel++) = rgba(r, g, b, 255);
    }

    fop->setProgress(((double)row / (double)image->height()));
    if (fop->isStop()) {
      break;
    }
  }

  free(rowBuffer);

  if (format == FORMAT_PAM) {
  }
  else {
    auto opts = fop->formatOptionsOfDocument<PnmOptions>();
    opts->subformat = PnmSubformat::PNM_BINARY;
    fop->setLoadedFormatOptions(opts);
  }

  return true;
}

// P7 magic
static bool loadPam(std::ifstream& istream, FileOp* fop, PnmFormat format)
{
  auto header = loadPamHeader(istream, fop, format);
  ImageRef image = std::get<0>(header);
  TuplType tuplType = std::get<1>(header);

  // only useful for grayscale
  uint16_t maxval = std::get<2>(header);

  if (!image) {
    return false;
  }

  uint8_t bytesPerPixel = 0;
  switch (tuplType) {
    case BLACKANDWHITE:
    case GRAYSCALE:           bytesPerPixel = 1; break;

    case BLACKANDWHITE_ALPHA:
    case GRAYSCALE_ALPHA:     bytesPerPixel = 2; break;

    case RGB:                 bytesPerPixel = 3; break;

    case RGB_ALPHA:           bytesPerPixel = 4; break;
  }

  char* rowBuffer = (char*)malloc((size_t)image->width() * bytesPerPixel);
  for (int row = 0; row < image->height(); ++row) {
    istream.get(rowBuffer, sizeof(rowBuffer));
    if (!istream) {
      return false;
    }

    uint8_t* rowCursor = (uint8_t*)rowBuffer;
    int c;
    uint8_t* pixel;
    for (int col = 0; col < image->width(); ++col) {
      pixel = image->getPixelAddress(col, row);
      switch (tuplType) {
        case BLACKANDWHITE:
          if (*rowCursor > 1) {
            fop->setError("Failed to load pixel (%d, %d)\n", col, row);
            return false;
          }
          *pixel = *(rowCursor++);
          break;

        case BLACKANDWHITE_ALPHA:
          // see loadPamHeader for creation of palette
          c = *(rowCursor++);
          c |= *(rowCursor++) << 1;
          *(pixel++) = c;
          break;

        case GRAYSCALE:
          if (*rowCursor > maxval) {
            fop->setError("Failed to load pixel (%d, %d)\n", col, row);
            return false;
          }
          *(pixel++) = *(rowCursor++);
          *(pixel++) = 255;
          break;

        case GRAYSCALE_ALPHA:
          *(pixel++) = *(rowCursor++);
          *(pixel++) = *(rowCursor++);
          break;

        case RGB:
          *(pixel++) = *(rowCursor++);
          *(pixel++) = *(rowCursor++);
          *(pixel++) = *(rowCursor++);
          *(pixel++) = 255;
          break;

        case RGB_ALPHA:
          *(pixel++) = *(rowCursor++);
          *(pixel++) = *(rowCursor++);
          *(pixel++) = *(rowCursor++);
          *(pixel++) = *(rowCursor++);
          break;
      }
    }

    fop->setProgress(((double)row / (double)image->height()));
    if (fop->isStop()) {
      break;
    }
  }

  free(rowBuffer);

  if (format == FORMAT_PAM) {
  }
  else {
    auto opts = fop->formatOptionsOfDocument<PnmOptions>();
    opts->subformat = PnmSubformat::PNM_BINARY;
    fop->setLoadedFormatOptions(opts);
  }

  return true;
}

static bool loadPnm(FileOp* fop, PnmFormat format)
{
  std::ifstream istream(fop->filename(), std::ifstream::in | std::ifstream::binary);
  if (!istream) {
    return false;
  }
  switch (ppmMagic(istream, fop)) {
    case '1': return loadPbmPlain(istream, fop, format);
    case '2': return loadPgmPlain(istream, fop, format);
    case '3': return loadPpmPlain(istream, fop, format);
    case '4': return loadPbmBinary(istream, fop, format);
    case '5': return loadPgmBinary(istream, fop, format);
    case '6': return loadPpmBinary(istream, fop, format);
    case '7': return loadPam(istream, fop, format);
    default:  return false;
  }
}

static bool savePnm(FileOp* fop, PnmFormat format)
{
  ImageRef image = fop->sequenceImageToSave();

  // verify that image is valid for format
  switch (format) {
    case FORMAT_PBM:
      if (fop->sequenceGetHasAlpha()) {
        fop->setError("PBM does not support alpha");
        return false;
      }
      if (image->pixelFormat() != IMAGE_BITMAP) {
        fop->setError("PBM format requires bitmap image");
        return false;
      }
      break;
    case FORMAT_PGM:
      if (fop->sequenceGetHasAlpha()) {
        fop->setError("PGM does not support alpha");
        return false;
      }
      switch (image->pixelFormat()) {
        case IMAGE_GRAYSCALE: break;
        case IMAGE_INDEXED:   {
          const Palette* palette = fop->sequenceGetPalette();
          for (int i = 0; i < fop->sequenceGetNColors(); ++i) {
            if (palette->getEntry(i) != rgba(i, i, i, 255)) {
              fop->setError("indexed PGM requires palette indices to match grayscale colors");
              return false;
            }
          }
          break;
        }
        default: fop->setError("PGM format requires RGB or indexed image"); return false;
      }
      break;
    case FORMAT_PPM:
      if (fop->sequenceGetHasAlpha()) {
        fop->setError("PPM does not support alpha");
        return false;
      }
      if (image->pixelFormat() != IMAGE_RGB) {
        fop->setError("PPM format requires RGB image");
      }
      break;
    case FORMAT_PAM:
      switch (image->pixelFormat()) {
        case IMAGE_TILEMAP: fop->setError("PAM format does not allow tilemaps"); return false;
        case IMAGE_INDEXED: {
          const Palette* palette = fop->sequenceGetPalette();
          if (palette->size() != 4 || palette->getEntry(0) != rgba(0, 0, 0, 0) ||
              palette->getEntry(1) != rgba(255, 255, 255, 0) ||
              palette->getEntry(2) != rgba(0, 0, 0, 255) ||
              palette->getEntry(3) != rgba(255, 255, 255, 255)) {
            fop->setError("indexed PAM must be BLACKANDWHITE_ALPHA");
            return false;
          }
        }
      }
      break;
  }

  char magic;
  switch (format) {
    case FORMAT_PBM: magic = '1'; break;
    case FORMAT_PGM: magic = '2'; break;
    case FORMAT_PPM: magic = '3'; break;
    case FORMAT_PAM: magic = '4'; break;
  }
  if (format != FORMAT_PAM) {
    auto opts = fop->formatOptionsForSaving<PnmOptions>();
    switch (opts->subformat) {
      case PNM_BINARY: magic += 3; break;
      case PNM_PLAIN:  break;
    }
  }
  std::ofstream ostream(fop->filename(), std::ofstream::out | std::ofstream::binary);
  ostream << 'P' << magic << '\n';

  switch (format) {
    case FORMAT_PBM:
    case FORMAT_PGM:
    case FORMAT_PPM: ostream << image->width() << " " << image->height() << "\n"; break;
    case FORMAT_PAM:
      ostream << "WIDTH " << image->width() << "\n";
      ostream << "HEIGHT " << image->height() << "\n";
      break;
  }
  switch (format) {
    case FORMAT_PGM: ostream << fop->sequenceGetNColors() << "\n"; break;
    case FORMAT_PPM: ostream << "255\n"; break;
    case FORMAT_PAM:
      switch (image->pixelFormat()) {
        case IMAGE_BITMAP:
          ostream << "MAXVAL 1\n";
          ostream << "DEPTH 1\n";
          ostream << "TUPLTYPE BLACKANDWHITE\n";
          break;
        case IMAGE_INDEXED:
          ostream << "MAXVAL 1\n";
          ostream << "DEPTH 2\n";
          ostream << "TUPLTYPE BLACKANDWHITE_ALPHA\n";
          break;
        case IMAGE_GRAYSCALE:
          ostream << "MAXVAL 255\n";
          if (fop->sequenceGetHasAlpha()) {
            ostream << "DEPTH 2\n";
            ostream << "TUPLTYPE GRAYSCALE_ALPHA\n";
          }
          else {
            ostream << "DEPTH 1\n";
            ostream << "TUPLTYPE GRAYSCALE\n";
          }
          break;
        case IMAGE_RGB:
          ostream << "MAXVAL 255\n";
          if (fop->sequenceGetHasAlpha()) {
            ostream << "DEPTH 4\n";
            ostream << "TUPLTYPE RGB_ALPHA\n";
          }
          else {
            ostream << "DEPTH 3\n";
            ostream << "TUPLTYPE RGB\n";
          }
          break;
      }
      ostream << "ENDHDR\n";
      break;
  }

  switch (magic) {
    case '1':
      for (int r = 0, idx = 0; r < image->height(); ++r) {
        for (int c = 0; c < image->width(); ++c) {
          ostream << (image->getPixel(r, c) == 0 ? "1" : "0");
          if (idx >= 70) {
            ostream << "\n";
            idx = 0;
          }
          else {
            ostream << " ";
            idx += 2;
          }
        }
      }
      ostream << "\n";
      break;
    case '2':
      for (int r = 0, idx = 0; r < image->height(); ++r) {
        for (int c = 0; c < image->width(); ++c) {
          ostream << std::setw(3) << image->getPixel(r, c);
          if (idx >= 70) {
            ostream << "\n";
            idx = 0;
          }
          else {
            ostream << " ";
            idx += 4;
          }
        }
      }
      ostream << "\n";
      break;
    case '3':
      for (int r = 0, idx = 0; r < image->height(); ++r) {
        for (int c = 0; c < image->width(); ++c) {
          int pixel = image->getPixel(r, c);
          ostream << std::setw(3) << rgba_getr(pixel) << " ";
          ostream << std::setw(3) << rgba_getg(pixel) << " ";
          ostream << std::setw(3) << rgba_getb(pixel);
          if (idx >= 70) {
            ostream << "\n";
            idx = 0;
          }
          else {
            ostream << "  ";
            // component numbers + space, extra space
            idx += (3 * 4) + 1;
          }
        }
      }
      ostream << "\n";
      break;
    case '4':
      for (int r = 0; r < image->height(); ++r) {
        for (int c = 0; c < image->width(); c += 8) {
          char packed = 0;
          for (int idx = 0; idx < 8; ++idx) {
            if (c + idx < image->width()) {
              packed |= (char)(image->getPixel(r, c + idx) == 0);
            }
            packed <<= 1;
          }
          ostream.put(packed);
        }
      }
      break;
    case '5':
      for (int r = 0; r < image->height(); ++r) {
        for (int c = 0; c < image->width(); ++c) {
          ostream.put((char)image->getPixel(r, c));
        }
      }
      break;
    case '6':
      for (int r = 0; r < image->height(); ++r) {
        for (int c = 0; c < image->width(); ++c) {
          int pixel = image->getPixel(r, c);
          ostream.put(rgba_getr(pixel));
          ostream.put(rgba_getg(pixel));
          ostream.put(rgba_getb(pixel));
        }
      }
      break;
    case '7':
      for (int r = 0; r < image->height(); ++r) {
        for (int c = 0; c < image->width(); ++c) {
          int pixel = image->getPixel(r, c);
          switch (image->pixelFormat()) {
            case IMAGE_BITMAP:
              // BLACKANDWHITE
              ostream.put(pixel);
              break;
            case IMAGE_INDEXED:
              // BLACKANDWHITE_ALPHA
              ostream.put((char)((pixel & 1) == 1));
              ostream.put((char)((pixel & 2) == 2));
              break;
            case IMAGE_GRAYSCALE:
              // GRAYSCALE
              ostream.put(graya_getv(pixel));
              if (fop->sequenceGetHasAlpha()) {
                // GRAYSCALE_ALPHA
                ostream.put(graya_geta(pixel));
              }
              break;
            case IMAGE_RGB:
              // RGB
              ostream.put(rgba_getr(pixel));
              ostream.put(rgba_getg(pixel));
              ostream.put(rgba_getb(pixel));
              if (fop->sequenceGetHasAlpha()) {
                // RGB_ALPHA
                ostream.put(rgba_geta(pixel));
              }
              break;
          }
        }
      }
      break;
    default:
      // magic is only 1-7
      break;
  }
  return false;
}

bool PbmFormat::onLoad(FileOp* fop)
{
  return loadPnm(fop, PnmFormat::FORMAT_PBM);
}

bool PgmFormat::onLoad(FileOp* fop)
{
  return loadPnm(fop, PnmFormat::FORMAT_PGM);
}

bool PpmFormat::onLoad(FileOp* fop)
{
  return loadPnm(fop, PnmFormat::FORMAT_PPM);
}

bool PamFormat::onLoad(FileOp* fop)
{
  return loadPnm(fop, PnmFormat::FORMAT_PAM);
}

FormatOptionsPtr PbmFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<PnmOptions>();
  return opts;
}

FormatOptionsPtr PgmFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<PnmOptions>();
  return opts;
}

FormatOptionsPtr PpmFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<PnmOptions>();
  return opts;
}

#ifdef ENABLE_SAVE

bool PbmFormat::onSave(FileOp* fop)
{
  return savePnm(fop, FORMAT_PBM);
}

bool PgmFormat::onSave(FileOp* fop)
{
  return savePnm(fop, FORMAT_PGM);
}

bool PpmFormat::onSave(FileOp* fop)
{
  return savePnm(fop, FORMAT_PPM);
}

bool PamFormat::onSave(FileOp* fop)
{
  return savePnm(fop, FORMAT_PAM);
}

#endif // ENABLE_SAVE

} // namespace app
