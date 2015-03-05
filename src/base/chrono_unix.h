// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <ctime>
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
