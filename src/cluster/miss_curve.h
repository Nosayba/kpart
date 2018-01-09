/** $lic$
 * Copyright (C) 2012-2016 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work. If you use code from Jigsaw
 * (src/jigsaw*, src/gmon*, src/lookahead*), we request that you reference our
 * papers ("Jigsaw: Scalable Software-Defined Caches", Beckmann and Sanchez,
 * PACT-22, September 2013 and "Scaling Distributed Cache Hierarchies through
 * Computation and Data Co-Scheduling", HPCA-21, Feb 2015). If you use code
 * from Talus (src/talus*), we request that you reference our paper ("Talus: A
 * Simple Way to Remove Cliffs in Cache Performance", Beckmann and Sanchez,
 * HPCA-21, February 2015).
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdint.h>
#include <vector>
//#include "log.h"
#include <assert.h>

class MissCurve {
public:
  typedef uint32_t data_t;
  virtual data_t x(uint32_t bucket) const { return bucket; }
  virtual data_t y(uint32_t bucket) const = 0;
  virtual uint32_t getNumAccesses() const {
    if (getDomain() > 0)
      return y(0u);
    else
      return 0;
  }
  virtual uint32_t getDomain() const = 0;
  virtual data_t getMaxX() const { return x(getDomain() - 1); }
  virtual bool isEmpty() const = 0;

  // interpolating version
  double y(double bucket) const {
    auto q = (uint32_t) std::floor(bucket);
    auto r = std::fmod(bucket, 1.);

    if (q + 1 == getDomain()) {
      return this->y(q);
    }

    assert(q + 1 < getDomain());
    assert(0. <= r && r < 1.);

    auto m0 = (double) this->y(q);
    auto m1 = (double) this->y(q + 1);
    auto interpolated = m0 * (1 - r) + m1 * r;
    assert(interpolated >= 0.);
    return interpolated;
  }
  int32_t y(int32_t bucket) const { return (int32_t) y((uint32_t) bucket); }

  // at does not operate on indices, it operates on real x-values
  data_t at(data_t _x, uint32_t startHint) const {
    uint32_t b;
    for (b = startHint; b < getDomain(); b++) {
      if (x(b) > _x)
        break;
    }
    --b;
    return y(b);
  }
};

class RawMissCurve : public MissCurve {
public:
  RawMissCurve(std::vector<data_t> &&_yvals, std::vector<data_t> *_xvals)
      : yvals(_yvals), xvals(_yvals.size(), 0) {
    if (_xvals) {
      assert(_xvals->size() == xvals.size());
      for (uint32_t i = 0; i < xvals.size(); i++) {
        xvals[i] = _xvals->at(i);
      }
    } else {
      for (uint32_t i = 0; i < xvals.size(); i++) {
        xvals[i] = i;
      }
    }
  }
  explicit RawMissCurve(uint32_t buckets)
      : yvals(buckets + 1, 0), xvals(buckets + 1, 0) {
    for (uint32_t i = 0; i < xvals.size(); i++) {
      xvals[i] = i;
    }
  }
  // make a copy
  RawMissCurve(const MissCurve &orig)
      : yvals(orig.getDomain(), 0), xvals(orig.getDomain(), 0) {
    for (uint32_t i = 0; i < yvals.size(); i++) {
      yvals[i] = orig.y(i);
      xvals[i] = orig.x(i);
    }
  }
  RawMissCurve(RawMissCurve &&that) : yvals(that.yvals), xvals(that.xvals) {}
  RawMissCurve() {}

  static RawMissCurve interpolate(const MissCurve &sparse);
  static RawMissCurve convexify(const MissCurve &curve);
  static RawMissCurve combinedMissCurve(const MissCurve &curve1,
                                        const MissCurve &curve2);

  void times(double factor);
  void times(const RawMissCurve &that);
  void plus(data_t addend);
  void plus(const RawMissCurve &that);

  void scale(double samplingRate);

  void resize(uint32_t s) {
    xvals.resize(s);
    yvals.resize(s);
  }

  RawMissCurve(const RawMissCurve &that)
      : yvals(that.yvals), xvals(that.xvals) {}
  RawMissCurve &operator=(const RawMissCurve &that) {
    xvals = that.xvals;
    yvals = that.yvals;
    return *this;
  }

  void addMarginOfSafety(double scale);

  data_t x(uint32_t bucket) const {
    assert(bucket < getDomain());
    return xvals[bucket];
  }
  data_t y(uint32_t bucket) const {
    assert(bucket < getDomain());
    return yvals[bucket];
  }
  uint32_t getDomain() const { return yvals.size(); }
  bool isEmpty() const { return yvals.empty(); }

  std::vector<data_t> yvals;
  std::vector<data_t> xvals;
};

__attribute__((
    unused)) static std::ostream &operator<<(std::ostream &os,
                                             const RawMissCurve &curve) {
  // os << "Raw Miss Curve with " << curve.getDomain() << " buckets:" <<
  // std::endl;
  for (uint32_t b = 0; b < curve.getDomain(); b++) {
    os << "(" << curve.x(b) << ", " << curve.y(b) << ") ";
  }
  return os;
}
