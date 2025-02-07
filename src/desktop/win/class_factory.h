// Desktop Integration
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DESKTOP_CLASS_FACTORY_H_INCLUDED
#define DESKTOP_CLASS_FACTORY_H_INCLUDED

#include <cassert>

#include <objbase.h>
#include <shlwapi.h>

namespace desktop {

class ClassFactoryDelegate {
public:
  virtual ~ClassFactoryDelegate() {}
  virtual HRESULT lockServer(const bool lock) = 0;
  virtual HRESULT createInstance(REFIID riid, void** ppvObject) = 0;
};

class ClassFactory : public IClassFactory {
public:
  ClassFactory(ClassFactoryDelegate* delegate) : m_ref(1), m_delegate(delegate)
  {
    assert(m_delegate);
    m_delegate->lockServer(true);
  }

  ~ClassFactory()
  {
    m_delegate->lockServer(false);
    delete m_delegate;
  }

  // IUnknown
  IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
  {
    static const QITAB qit[] = { QITABENT(ClassFactory, IClassFactory), { 0 } };
    return QISearch(this, qit, riid, ppv);
  }

  IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_ref); }

  IFACEMETHODIMP_(ULONG) Release()
  {
    long ref = InterlockedDecrement(&m_ref);
    if (ref == 0)
      delete this;
    return ref;
  }

  // IClassFactory
  IFACEMETHODIMP CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
  {
    if (punkOuter)
      return CLASS_E_NOAGGREGATION;
    else
      return m_delegate->createInstance(riid, ppv);
  }

  IFACEMETHODIMP LockServer(BOOL lock) { return m_delegate->lockServer(lock ? true : false); }

private:
  long m_ref;
  ClassFactoryDelegate* m_delegate;
};

} // namespace desktop

#endif
