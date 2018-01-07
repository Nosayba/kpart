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
#include "miss_curve.h"
#include <iostream>

namespace interp {

template <typename T> inline T linear(T y0, T y1, T alpha) {
  return y0 + (y1 - y0) * alpha;
}

template <typename T> inline T logLinear(T y0, T y1, T alpha) {
  return y0 * pow(y1 / y0, alpha);
}

template <typename T, T (*f)(T, T, T)>
inline T twoD(T x0, T y0, T x1, T y1, T x) {
  T alpha = (x - x0) / (x1 - x0);
  return f(y0, y1, alpha);
}
}

RawMissCurve RawMissCurve::interpolate(const MissCurve &sparse) {
  const auto interpolate = interp::twoD<double, interp::linear>;

  RawMissCurve dense(sparse.getMaxX());

  for (uint32_t sx = 0; sx < sparse.getDomain() - 1; sx++) {
    uint32_t sx0 = sparse.x(sx);
    uint32_t sx1 = sparse.x(sx + 1);
    uint32_t sy0 = sparse.y(sx);
    uint32_t sy1 = sparse.y(sx + 1);

    // info("Interpolating sx: %d --- %d -> %d x %d -> %d", sx, sx0, sx1, sy0,
    // sy1);

    for (uint32_t dx = sx0; dx <= sx1; dx++) {
      dense.yvals[dx] = (uint32_t) interpolate(sx0, sy0, sx1, sy1, dx);
    }
  }

  return dense;
}

void RawMissCurve::addMarginOfSafety(double scale) {
  // Scale miss curve along x-axis
  assert(scale >= 1.0);
  for (uint32_t i = 0; i < xvals.size(); i++) {
    xvals[i] *= scale;
  }
}

void RawMissCurve::scale(double samplingRate) {
  if (samplingRate == 1.) {
    return;
  }

  double scalingFactor = 1. / samplingRate;
  for (uint32_t i = 0; i < xvals.size(); i++) {
    xvals[i] *= scalingFactor;
    yvals[i] *= scalingFactor;
  }
}

void RawMissCurve::times(double factor) {
  for (uint32_t i = 0; i < xvals.size(); i++) {
    yvals[i] *= factor;
  }
}

void RawMissCurve::times(const RawMissCurve &that) {
  assert(that.getDomain() == getDomain());
  for (uint32_t i = 0; i < xvals.size(); i++) {
    assert(xvals[i] == that.x(i));
    yvals[i] *= that.y(i);
  }
}

void RawMissCurve::plus(data_t addend) {
  for (uint32_t i = 0; i < xvals.size(); i++) {
    yvals[i] += addend;
  }
}

void RawMissCurve::plus(const RawMissCurve &that) {
  assert(that.getDomain() == getDomain());
  for (uint32_t i = 0; i < xvals.size(); i++) {
    assert(xvals[i] == that.x(i));
    yvals[i] += that.y(i);
  }
}

RawMissCurve RawMissCurve::convexify(const MissCurve &in) {
  typedef std::make_signed<MissCurve::data_t>::type sdata_t;

  uint32_t hull[in.getDomain()];
  hull[0] = 0;
  uint32_t hullSize = 1;

  for (uint32_t i = 1; i < in.getDomain(); i++) {
    sdata_t x2 = in.x(i);
    sdata_t y2 = in.y(i);

    while (hullSize > 1) {
      uint32_t i1 = hull[hullSize - 1];
      uint32_t i0 = hull[hullSize - 2];

      sdata_t x1 = in.x(i1);
      sdata_t y1 = in.y(i1);
      sdata_t x0 = in.x(i0);
      sdata_t y0 = in.y(i0);

      bool remove =
          std::abs((y2 - y1) * (x2 - x0)) >= std::abs((y2 - y0) * (x2 - x1));

      if (remove) {
        hullSize--;
      } else {
        break;
      }
    }

    hull[hullSize] = i;
    hullSize++;
  }

  RawMissCurve out(hullSize - 1);
  for (uint32_t i = 0; i < hullSize; i++) {
    out.xvals[i] = in.x(hull[i]);
    out.yvals[i] = in.y(hull[i]);
  }

  return out;
}
