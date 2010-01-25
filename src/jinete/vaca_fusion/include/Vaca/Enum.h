// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VACA_ENUM_H
#define VACA_ENUM_H

namespace Vaca {

/**
   This class is used to define enumerations "a la" C++0x.

   "Base" policy must be like this:
   @code
   struct Base {
     enum enumeration { ... };
     static const enumeration default_value = ...;
   };
   @endcode

   Example of how to use this:
   @code
   struct NumbersEnum
   {
     enum enumeration {
       Zero,
       One,
       Two,
       Three,
     };
     static const enumeration default_value = Zero;
   };
   typedef Enum<NumbersEnum> Numbers;

   main() {
     Numbers n1, n2 = Numbers::One;
     n1 = n2;
     n2 = Numbers::Two;
   }
   @endcode
*/
template<typename Base>
struct Enum : public Base
{
  typedef typename Base::enumeration enumeration;

  Enum() : m_value(Base::default_value)
  { }

  Enum(enumeration value) : m_value(value)
  { }

  operator enumeration() const
  { return m_value; }

  Enum<Base> &operator=(enumeration value)
  { m_value = value;
    return *this; }

private:
  enumeration m_value;
};

/**
   This class is used to define sets of enumerated values.

   "Base" policy must be like this:
   @code
   struct Base {
     enum { ... };
   };
   @endcode

   A EnumSet doesn't need a @c default_value like a Enum because the
   default value is zero which means: a empty set.

   Example of how to use this:
   @code
   struct ColorsEnumSet
   {
     enum {
       Red = 1,
       Blue = 2,
       Yellow = 4,
       Magenta = Red | Blue
     };
   };
   typedef EnumSet<ColorsEnumSet> Colors;

   main() {
     Colors red, blue, magenta;
     red = Colors::Red;
     blue = Colors::Blue;
     magenta = red | blue;
     if (magenta == Colors::Magenta) { ... }
   }
   @endcode
*/
template<typename Base>
struct EnumSet : public Base
{
  EnumSet() : m_value(0)
  { }

  EnumSet(int value) : m_value(value)
  { }

  operator int() const
  { return m_value; }

  EnumSet<Base> operator|(int value)
  { return m_value | value; }

  EnumSet<Base> operator&(int value)
  { return m_value & value; }

  EnumSet<Base> operator^(int value)
  { return m_value ^ value; }

  EnumSet<Base> &operator=(int value)
  { m_value = value;
    return *this; }

  EnumSet<Base> &operator|=(int value)
  { m_value |= value;
    return *this; }

  EnumSet<Base> &operator&=(int value)
  { m_value &= value;
    return *this; }

  EnumSet<Base> &operator^=(int value)
  { m_value ^= value;
    return *this; }

private:
  int m_value;
};

} // namespace Vaca

#endif // VACA_ENUM_H
