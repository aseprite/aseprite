/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdlib>

#include "app/commands/filters/convolution_matrix_stock.h"

#include "base/unique_ptr.h"
#include "filters/convolution_matrix.h"
#include "app/resource_finder.h"
#include "app/util/filetoks.h"

namespace app {

ConvolutionMatrixStock::ConvolutionMatrixStock()
{
  reloadStock();
}

ConvolutionMatrixStock::~ConvolutionMatrixStock()
{
  cleanStock();
}

SharedPtr<ConvolutionMatrix> ConvolutionMatrixStock::getByName(const char* name)
{
  for (const_iterator it = begin(), end = this->end(); it != end; ++it) {
    if (strcmp((*it)->getName(), name) == 0)
      return *it;
  }
  return SharedPtr<ConvolutionMatrix>(0);
}

namespace {
  class FileDestroyer {
  public:
    static void destroy(FILE* ptr) { fclose(ptr); }
  };
}

void ConvolutionMatrixStock::reloadStock()
{
#define READ_TOK() {                                    \
    if (!tok_read(f, buf, leavings, sizeof (leavings))) \
      break;                                            \
  }

#define READ_INT(var) {                         \
    READ_TOK();                                 \
    var = strtol(buf, NULL, 10);                \
  }

  const char *names[] = { "convmatr.usr",
                          "convmatr.gen",
                          "convmatr.def", NULL };
  char *s, buf[256], leavings[4096];
  int i, c, x, y, w, h, div, bias;
  SharedPtr<ConvolutionMatrix> matrix;
  base::UniquePtr<FILE, int(*)(FILE*)> f;
  std::string name;

  cleanStock();

  for (i=0; names[i]; i++) {
    ResourceFinder rf;
    rf.findInDataDir(names[i]);

    while (const char* path = rf.next()) {
      // Open matrices stock file
      f.reset(fopen(path, "r"), fclose);
      if (!f)
        continue;

      tok_reset_line_num();

      strcpy(leavings, "");

      // Read the matrix name
      while (tok_read(f, buf, leavings, sizeof(leavings))) {
        // Name of the matrix
        name = buf;

        // Width and height
        READ_INT(w);
        READ_INT(h);

        if ((w <= 0) || (w > 32) ||
            (h <= 0) || (h > 32))
          break;

        // Create the matrix data
        matrix.reset(new ConvolutionMatrix(w, h));
        matrix->setName(name.c_str());

        // Centre
        READ_INT(x);
        READ_INT(y);

        if ((x < 0) || (x >= w) ||
            (y < 0) || (y >= h))
          break;

        matrix->setCenterX(x);
        matrix->setCenterY(y);

        // Data
        READ_TOK();                    // Jump the `{' char
        if (*buf != '{')
          break;

        c = 0;
        div = 0;
        for (y=0; y<h; ++y) {
          for (x=0; x<w; ++x) {
            READ_TOK();
            int value = strtod(buf, NULL) * ConvolutionMatrix::Precision;
            div += value;

            matrix->value(x, y) = value;
          }
        }

        READ_TOK();                    // Jump the `}' char
        if (*buf != '}')
          break;

        if (div > 0)
          bias = 0;
        else if (div == 0) {
          div = ConvolutionMatrix::Precision;
          bias = 128;
        }
        else {
          div = ABS(div);
          bias = 255;
        }

        // Div
        READ_TOK();
        if (strcmp(buf, "auto") != 0)
          div = strtod(buf, NULL) * ConvolutionMatrix::Precision;

        matrix->setDiv(div);

        // Bias
        READ_TOK();
        if (strcmp(buf, "auto") != 0)
          bias = strtod(buf, NULL);

        matrix->setBias(bias);

        // Target
        READ_TOK();

        Target target = 0;
        for (s=buf; *s; s++) {
          switch (*s) {
            case 'r': target |= TARGET_RED_CHANNEL; break;
            case 'g': target |= TARGET_GREEN_CHANNEL; break;
            case 'b': target |= TARGET_BLUE_CHANNEL; break;
            case 'a': target |= TARGET_ALPHA_CHANNEL; break;
          }
        }

        if ((target & (TARGET_RED_CHANNEL |
                       TARGET_GREEN_CHANNEL |
                       TARGET_BLUE_CHANNEL)) != 0) {
          target |= TARGET_GRAY_CHANNEL;
        }

        matrix->setDefaultTarget(target);

        // Insert the new matrix in the list
        m_matrices.push_back(matrix);
      }
    }
  }
}

void ConvolutionMatrixStock::cleanStock()
{
  m_matrices.clear();
}

} // namespace app
