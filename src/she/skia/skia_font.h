// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

namespace she {

class SkiaFont : public Font {
public:
  enum DestroyFlag {
    None = 0,
    DeleteThis = 1,
  };

  SkiaFont(DestroyFlag flag) : m_flag(flag) {
  }

  void dispose() override {
    if (m_flag & DeleteThis)
      delete this;
  }

  int height() const override {
    return 8;
  }

  int charWidth(int chr) const override {
    return 8;
  }

  int textLength(const char* chr) const override {
    return 8;
  }

  bool isScalable() const override {
    return false;
  }

  void setSize(int size) override {
  }

  void* nativeHandle() override {
    return nullptr;
  }

private:
  DestroyFlag m_flag;
};

} // namespace she
