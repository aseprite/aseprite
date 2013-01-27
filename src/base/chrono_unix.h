// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include <time.h>
#include <sys/time.h>

class base::Chrono::ChronoImpl {
public:
  ChronoImpl() {
    reset();
  }

  void reset() {
    gettimeofday(&m_point, NULL);
  }

  double elapsed() const {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (double)(now.tv_sec + (double)now.tv_usec/1000000) -
           (double)(m_point.tv_sec + (double)m_point.tv_usec/1000000);
  }

private:
  struct timeval m_point;
};
